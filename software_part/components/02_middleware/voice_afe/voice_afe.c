#include "voice_afe.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "inmp441_driver.h"
#include "driver/i2s_std.h"

#ifndef CONFIG_VOICE_AFE_PCM_BUFFER_SAMPLES
#define CONFIG_VOICE_AFE_PCM_BUFFER_SAMPLES 256
#endif

#if CONFIG_VOICE_AFE_USE_GMF
/* 可选：启用 ESP-SR + gmf_ai_audio */
#include "esp_gmf_afe.h"
#include "esp_gmf_afe_manager.h"
#include "esp_afe_config.h"
#include "esp_afe_sr_iface.h"
#include "model_path.h"
#include "esp_mn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_speech_commands.h"
#endif

static const char *TAG = "VOICE_AFE";

static voice_afe_state_t        s_state          = VOICE_AFE_STATE_IDLE;
static bool                     s_remote_enabled = false;
static voice_afe_cmd_handler_t  s_cmd_cb         = NULL;
static void                    *s_cmd_ctx        = NULL;
static voice_afe_pcm_sink_t     s_pcm_sink       = NULL;
static void                    *s_pcm_ctx        = NULL;
static int                      s_sample_rate    = 16000;

/*========================== legacy fallback (无 GMF) =========================*/
#if !CONFIG_VOICE_AFE_USE_GMF
static TaskHandle_t s_legacy_task = NULL;

static void voice_afe_legacy_task(void *arg)
{
    (void)arg;

    inmp441_config_t cfg = {
        .sample_rate_hz = s_sample_rate,
        .use_apll       = true,
        .channel        = INMP441_CHANNEL_LEFT,
        .dma_frame_num  = 0,
        .dma_desc_num   = 0,
    };

    if (inmp441_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "legacy INMP441 init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "legacy PCM bridge running @%d Hz", s_sample_rate);

    int16_t buf[CONFIG_VOICE_AFE_PCM_BUFFER_SAMPLES];
    for (;;) {
        size_t n = inmp441_read_pcm16(buf, CONFIG_VOICE_AFE_PCM_BUFFER_SAMPLES, pdMS_TO_TICKS(500));
        if (n == 0) {
            ESP_LOGW(TAG, "legacy read timeout");
            continue;
        }

        if (s_pcm_sink && s_remote_enabled) {
            s_pcm_sink(buf, n, s_pcm_ctx);
        }
    }
}
#endif /* !CONFIG_VOICE_AFE_USE_GMF */

/*========================== GMF / ESP-SR 路径 ==========================*/
#if CONFIG_VOICE_AFE_USE_GMF
static esp_gmf_afe_manager_handle_t s_mgr  = NULL;
static esp_gmf_obj_handle_t         s_afe  = NULL;
static srmodel_list_t              *s_models = NULL;
static size_t                       s_chunk_bytes = 0;
static model_iface_data_t          *s_mn_model_data = NULL;
static const esp_mn_iface_t        *s_mn_iface = NULL;

/* 从 INMP441 读取 PCM，交给 AFE manager */
static int32_t afe_read_cb(void *buffer, int buf_sz, void *user_ctx, uint32_t ticks)
{
    (void)user_ctx;
    size_t samples = buf_sz / sizeof(int16_t);
    size_t got = inmp441_read_pcm16((int16_t *)buffer, samples, pdMS_TO_TICKS(ticks));
    return got * sizeof(int16_t);
}

/* AFE fetch 结果：可同时用于远端 ASR 推流 */
static void afe_result_cb(afe_fetch_result_t *result, void *user_ctx)
{
    (void)user_ctx;
    if (!result || !result->data || result->data_size <= 0) return;

    if (s_pcm_sink && s_remote_enabled && s_state == VOICE_AFE_STATE_REMOTE_ASR) {
        size_t samples = result->data_size / sizeof(int16_t);
        s_pcm_sink(result->data, samples, s_pcm_ctx);
    }
}

/* 事件：唤醒/结束/命令词 */
static void afe_event_cb(esp_gmf_element_handle_t el, esp_gmf_afe_evt_t *event, void *user_ctx)
{
    (void)el;
    (void)user_ctx;
    if (!event) return;

    switch (event->type) {
    case ESP_GMF_AFE_EVT_WAKEUP_START:
        s_state = VOICE_AFE_STATE_WAIT_CMD;
        ESP_LOGI(TAG, "Wake word detected");
        break;
    case ESP_GMF_AFE_EVT_WAKEUP_END:
        if (s_state == VOICE_AFE_STATE_WAIT_CMD) {
            s_state = VOICE_AFE_STATE_IDLE;
        }
        break;
    case ESP_GMF_AFE_EVT_VCMD_DECTECTED: {
        s_state = VOICE_AFE_STATE_LOCAL_EXEC;
        esp_gmf_afe_vcmd_info_t *info = (esp_gmf_afe_vcmd_info_t *)event->event_data;
        int cmd_id = info ? info->phrase_id : -1;
        const char *phrase = (info && info->str[0]) ? info->str : NULL;
        if (s_cmd_cb) {
            s_cmd_cb(cmd_id, phrase, s_cmd_ctx);
        }
        s_state = VOICE_AFE_STATE_IDLE;
        break;
    }
    default:
        break;
    }
}
#endif /* CONFIG_VOICE_AFE_USE_GMF */

/*========================== 公共接口实现 ==========================*/
esp_err_t voice_afe_init(const voice_afe_config_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    s_sample_rate    = cfg->sample_rate_hz > 0 ? cfg->sample_rate_hz : 16000;
    s_remote_enabled = cfg->enable_remote;
    s_cmd_cb         = cfg->on_cmd;
    s_cmd_ctx        = cfg->on_cmd_ctx;
    s_pcm_sink       = cfg->pcm_sink;
    s_pcm_ctx        = cfg->pcm_sink_ctx;

#if CONFIG_VOICE_AFE_USE_GMF
    /* 1. 加载模型分区 */
    s_models = esp_srmodel_init("model");
    if (!s_models) {
        ESP_LOGE(TAG, "esp_srmodel_init failed, 请确认存在 model 分区并烧录模型");
        return ESP_FAIL;
    }

    /* 2. AFE 配置（单路 Mic，16k，SR 模式，高性能） */
    afe_config_t *afe_cfg = afe_config_init("M", s_models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    if (!afe_cfg) {
        ESP_LOGE(TAG, "afe_config_init failed");
        return ESP_FAIL;
    }
    afe_cfg->wakenet_init      = true;
    afe_cfg->vad_init          = true;
    afe_cfg->vad_mode          = VAD_MODE_3;
    afe_cfg->vad_min_noise_ms  = 600;
    afe_cfg->wakenet_model_name   = esp_srmodel_filter(s_models, "wn", "chs");   /* 中文唤醒 */
    afe_cfg->wakenet_model_name_2 = NULL;
    afe_config_check(afe_cfg);

    /* 3. Multinet：使用模型名创建并写入命令表（7 条中文命令） */
    char *mn_model_name = esp_srmodel_filter(s_models, "mn", "chs");
    if (!mn_model_name) {
        ESP_LOGE(TAG, "未找到中文 multinet 模型，请确认 model 分区已烧入官方模型");
        return ESP_FAIL;
    }
    s_mn_iface = esp_mn_handle_from_name(mn_model_name);
    if (!s_mn_iface) {
        ESP_LOGE(TAG, "esp_mn_handle_from_name(%s) failed", mn_model_name);
        return ESP_FAIL;
    }
    if (strcmp(MULTINET_MODEL_NAME, "NULL") == 0) {
        ESP_LOGE(TAG, "MULTINET_COEFF 未启用（检查 menuconfig: Enable Chinese multinet2_single_recognition）");
        return ESP_FAIL;
    }
    s_mn_model_data = s_mn_iface->create(mn_model_name, 5000);  /* 5000ms 超时 */
    if (!s_mn_model_data) {
        ESP_LOGE(TAG, "multinet create failed");
        return ESP_FAIL;
    }
    esp_mn_commands_clear();
    esp_mn_commands_alloc(s_mn_iface, s_mn_model_data);
    const char *cmds[] = {
        "连接网络",
        "深色界面",
        "浅色界面",
        "播放网络歌曲",
        "播放本地音乐",
        "室内多少度",
        "外面什么天气",
    };
    for (int i = 0; i < (int)(sizeof(cmds)/sizeof(cmds[0])); ++i) {
        esp_mn_commands_add(i, cmds[i]);
    }
    esp_mn_error_t *err = esp_mn_commands_update();
    if (err && err->num > 0) {
        ESP_LOGW(TAG, "esp_mn_commands_update warning: %d phrases failed", err->num);
        for (int i = 0; i < err->num; i++) {
            if (err->phrases[i]) {
                ESP_LOGW(TAG, "  Failed phrase: %s", err->phrases[i]->string ? err->phrases[i]->string : "NULL");
            }
        }
    }

    /* 4. 创建 AFE manager（负责 feed/fetch 任务和 I2S 读回调） */
    esp_gmf_afe_manager_cfg_t mgr_cfg = DEFAULT_GMF_AFE_MANAGER_CFG(
        afe_cfg,
        afe_read_cb, NULL,
        afe_result_cb, NULL);
    esp_gmf_err_t ge = esp_gmf_afe_manager_create(&mgr_cfg, &s_mgr);
    if (ge != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "afe_manager_create failed err=%d", ge);
        return ESP_FAIL;
    }
    esp_gmf_afe_manager_enable_features(s_mgr, ESP_AFE_FEATURE_WAKENET, true);
    esp_gmf_afe_manager_enable_features(s_mgr, ESP_AFE_FEATURE_VAD, true);

    /* 4. 创建 GMF AFE 元素（负责唤醒/命令检测 & 事件） */
    esp_gmf_afe_cfg_t afe_el_cfg = DEFAULT_GMF_AFE_CFG(s_mgr, afe_event_cb, NULL, s_models);
    afe_el_cfg.vcmd_detect_en = true;
    afe_el_cfg.vcmd_timeout   = 5000;
    afe_el_cfg.mn_language    = "cn";
    ge = esp_gmf_afe_init(&afe_el_cfg, &s_afe);
    if (ge != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "esp_gmf_afe_init failed err=%d", ge);
        return ESP_FAIL;
    }

    /* 5. 启动命令检测 */
    esp_gmf_afe_vcmd_detection_begin(s_afe);

    /* 6. 查询 chunk 大小（用于日志） */
    size_t chunk = 0;
    esp_gmf_afe_manager_get_chunk_size(s_mgr, &chunk);
    s_chunk_bytes = chunk * sizeof(int16_t);

    ESP_LOGI(TAG, "GMF AFE started @%d Hz, chunk=%u samples", s_sample_rate, (unsigned)chunk);
    s_state = VOICE_AFE_STATE_IDLE;
    return ESP_OK;
#else
    const uint32_t stack_words = 8192; /* legacy路径：需做 Base64/WS，栈加大避免溢出 */
    BaseType_t rc = xTaskCreatePinnedToCore(
        voice_afe_legacy_task, "voice_afe_legacy",
        stack_words, NULL, 8, &s_legacy_task, 0);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "create legacy task failed");
        return ESP_FAIL;
    }
    return ESP_OK;
#endif
}

void voice_afe_set_remote_enabled(bool enable)
{
    s_remote_enabled = enable;
}

bool voice_afe_remote_enabled(void)
{
    return s_remote_enabled;
}

void voice_afe_set_state(voice_afe_state_t s)
{
    s_state = s;
}

voice_afe_state_t voice_afe_get_state(void)
{
    return s_state;
}

void voice_afe_set_cmd_handler(voice_afe_cmd_handler_t cb, void *user_ctx)
{
    s_cmd_cb  = cb;
    s_cmd_ctx = user_ctx;
}

void voice_afe_set_pcm_sink(voice_afe_pcm_sink_t cb, void *user_ctx)
{
    s_pcm_sink = cb;
    s_pcm_ctx  = user_ctx;
}
