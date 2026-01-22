#include "app_inmp441.h"

#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_attr.h"

#include "mem_utils.h"
#include "asr_client.h"
#include "server_root_cert.h"
#include "net_wifi_sntp.h"
#include "ota_update.h"
#include "inmp441_driver.h"
#include "tts_player.h"
#include "audio_aec.h"
#include "audio_ns.h"
#include "audio_i2s.h"

#include "sdkconfig.h"

/* 单次读取样本数：16000 Hz 时约 16 ms 音频 */
#ifndef INMP441_APP_SAMPLES
#define INMP441_APP_SAMPLES 256
#endif

/* ==================== AEC 配置 ==================== */
#ifndef INMP441_AEC_ENABLE
#define INMP441_AEC_ENABLE 1
#endif
#ifndef INMP441_AEC_REF_BUFFER_MS
#define INMP441_AEC_REF_BUFFER_MS 200  /* 参考信号缓冲区大小 */
#endif

/* ==================== NS（噪声抑制）配置 ==================== */
#ifndef INMP441_NS_ENABLE
#define INMP441_NS_ENABLE 0  /* 暂时禁用：内存压力较大且可能导致误触发 */
#endif
#ifndef INMP441_NS_MODE
#define INMP441_NS_MODE AUDIO_NS_MODE_MEDIUM  /* 0=Mild, 1=Medium, 2=Aggressive */
#endif

/* ==================== 音频分片聚合配置 ==================== */
/* 聚合目标：80ms @ 16kHz = 1280 samples，降低网络抖动，WS 消息从 ~60/s 降到 ~12/s */
#ifndef INMP441_AGGREGATE_SAMPLES
#define INMP441_AGGREGATE_SAMPLES 1280
#endif

static const char *TAG = "INMP441_APP";

/* ==================== ASR Client ==================== */
static asr_client_handle_t s_asr = NULL;
static volatile bool       s_tts_playing  = false;

/* ==================== ASR 启用/禁用控制 ==================== */
/* 默认禁用 ASR，只在进入 GPT 界面时启用 */
static volatile bool s_asr_enabled = false;

/* ==================== PTT（按住说话/松开发送） ==================== */
/* 由 UI 控制：PTT 模式下仅在按下期间推送音频，松开立即发送 stop */
static volatile bool s_asr_ptt_mode = false;
static volatile bool s_asr_ptt_pressed = false;

/* ==================== VAD（RMS 能量阈值 + 静音收口） ==================== */
#ifndef INMP441_VAD_RMS_THRESHOLD
/* 经验阈值：PCM16 RMS（0..32768），可按板子增益/环境微调 */
#define INMP441_VAD_RMS_THRESHOLD 200
#endif
#ifndef INMP441_VAD_SILENCE_DURATION_MS
/* 静音持续时间达到该值后发送 stop，触发句级 final */
#define INMP441_VAD_SILENCE_DURATION_MS 1500
#endif

static bool    s_vad_speaking         = false;
static int64_t s_vad_silence_start_ms = 0;

/* ==================== 音频分片聚合缓冲区 ==================== */
EXT_RAM_BSS_ATTR static int16_t s_aggregate_buf[INMP441_AGGREGATE_SAMPLES];
static size_t  s_aggregate_pos = 0;

/* ==================== 音频录制（诊断用） ==================== */
#ifndef INMP441_APP_RECORD_ENABLE
#define INMP441_APP_RECORD_ENABLE 0
#endif

#if INMP441_APP_RECORD_ENABLE
#define RECORD_SAMPLE_RATE 16000
#define RECORD_MAX_DURATION_SEC 30

static FILE *s_record_file = NULL;
static volatile bool s_record_active = false;
static size_t s_record_samples_written = 0;
static size_t s_record_samples_limit = 0;
static char s_record_filepath[64] = {0};

/* WAV 文件头结构（44 字节） */
typedef struct __attribute__((packed)) {
    char     riff[4];        /* "RIFF" */
    uint32_t file_size;      /* 文件大小 - 8 */
    char     wave[4];        /* "WAVE" */
    char     fmt[4];         /* "fmt " */
    uint32_t fmt_size;       /* 16 for PCM */
    uint16_t audio_format;   /* 1 = PCM */
    uint16_t num_channels;   /* 1 = mono */
    uint32_t sample_rate;    /* 16000 */
    uint32_t byte_rate;      /* sample_rate * num_channels * bits_per_sample/8 */
    uint16_t block_align;    /* num_channels * bits_per_sample/8 */
    uint16_t bits_per_sample;/* 16 */
    char     data[4];        /* "data" */
    uint32_t data_size;      /* 音频数据大小 */
} wav_header_t;

static void write_wav_header(FILE *f, uint32_t data_size)
{
    wav_header_t hdr = {
        .riff = {'R', 'I', 'F', 'F'},
        .file_size = data_size + 36,
        .wave = {'W', 'A', 'V', 'E'},
        .fmt = {'f', 'm', 't', ' '},
        .fmt_size = 16,
        .audio_format = 1,
        .num_channels = 1,
        .sample_rate = RECORD_SAMPLE_RATE,
        .byte_rate = RECORD_SAMPLE_RATE * 1 * 2,
        .block_align = 1 * 2,
        .bits_per_sample = 16,
        .data = {'d', 'a', 't', 'a'},
        .data_size = data_size
    };
    fseek(f, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, f);
}
#endif /* INMP441_APP_RECORD_ENABLE */

/* ==================== AEC 处理缓冲区 ==================== */
#if INMP441_AEC_ENABLE
static bool s_aec_initialized = false;
static int s_aec_frame_size = 0;
/* AEC 输入/输出缓冲区 */
static int16_t *s_aec_ref_buf = NULL;   /* 参考信号缓冲区 */
static int16_t *s_aec_out_buf = NULL;   /* AEC 输出缓冲区 */
/* AEC 帧累积缓冲区（用于处理样本数与帧大小不匹配的情况） */
static int16_t *s_aec_mic_accum = NULL;
static size_t s_aec_mic_accum_pos = 0;
static int16_t *s_aec_processed = NULL;  /* AEC 处理后的数据 */
static size_t s_aec_processed_pos = 0;
static size_t s_aec_processed_len = 0;

/**
 * @brief 对麦克风数据进行 AEC 处理
 *
 * 将输入样本累积到内部缓冲区，每当累积够一帧就进行 AEC 处理，
 * 然后从处理后的缓冲区返回数据。
 *
 * @param mic_in 输入的麦克风数据
 * @param out 输出缓冲区（可与 mic_in 相同）
 * @param samples 样本数
 * @return 实际处理并输出的样本数
 */
static size_t aec_process_samples(const int16_t *mic_in, int16_t *out, size_t samples)
{
    if (!s_aec_initialized || s_aec_frame_size <= 0 ||
        !s_aec_mic_accum || !s_aec_ref_buf || !s_aec_out_buf || !s_aec_processed) {
        /* AEC 未就绪，直接复制 */
        if (out != mic_in) {
            memcpy(out, mic_in, samples * sizeof(int16_t));
        }
        return samples;
    }

    size_t out_pos = 0;

    /* 1) 先输出之前处理好但尚未返回的数据 */
    while (out_pos < samples && s_aec_processed_pos < s_aec_processed_len) {
        out[out_pos++] = s_aec_processed[s_aec_processed_pos++];
    }
    if (s_aec_processed_pos >= s_aec_processed_len) {
        s_aec_processed_pos = 0;
        s_aec_processed_len = 0;
    }

    /* 2) 将新输入累积到 mic_accum */
    size_t in_pos = 0;
    while (in_pos < samples) {
        /* 累积麦克风数据 */
        while (in_pos < samples && (int)s_aec_mic_accum_pos < s_aec_frame_size) {
            s_aec_mic_accum[s_aec_mic_accum_pos++] = mic_in[in_pos++];
        }

        /* 累积够一帧，进行 AEC 处理 */
        if ((int)s_aec_mic_accum_pos >= s_aec_frame_size) {
            /* 获取参考信号 */
            audio_i2s_get_ref_signal(s_aec_ref_buf, s_aec_frame_size);

            /* 调用 AEC 处理 */
            size_t processed = audio_aec_process(s_aec_mic_accum, s_aec_ref_buf,
                                                  s_aec_out_buf, s_aec_frame_size);

            /* 将处理结果复制到 processed 缓冲区 */
            if (processed > 0) {
                memcpy(s_aec_processed, s_aec_out_buf, processed * sizeof(int16_t));
                s_aec_processed_len = processed;
                s_aec_processed_pos = 0;
            }

            /* 重置累积计数 */
            s_aec_mic_accum_pos = 0;

            /* 输出刚处理好的数据 */
            while (out_pos < samples && s_aec_processed_pos < s_aec_processed_len) {
                out[out_pos++] = s_aec_processed[s_aec_processed_pos++];
            }
            if (s_aec_processed_pos >= s_aec_processed_len) {
                s_aec_processed_pos = 0;
                s_aec_processed_len = 0;
            }
        }
    }

    return out_pos;
}
#endif

/* ==================== NS 处理缓冲区 ==================== */
#if INMP441_NS_ENABLE
static bool s_ns_initialized = false;
static int s_ns_frame_size = 0;
/* NS 帧累积缓冲区 */
static int16_t *s_ns_in_accum = NULL;
static size_t s_ns_in_accum_pos = 0;
static int16_t *s_ns_out_buf = NULL;
static int16_t *s_ns_processed = NULL;
static size_t s_ns_processed_pos = 0;
static size_t s_ns_processed_len = 0;

/**
 * @brief 对音频数据进行 NS 处理
 *
 * 将输入样本累积到内部缓冲区，每当累积够一帧就进行 NS 处理，
 * 然后从处理后的缓冲区返回数据。
 *
 * @param in 输入数据
 * @param out 输出缓冲区（可与 in 相同）
 * @param samples 样本数
 * @return 实际处理并输出的样本数
 */
static size_t ns_process_samples(const int16_t *in, int16_t *out, size_t samples)
{
    if (!s_ns_initialized || s_ns_frame_size <= 0 ||
        !s_ns_in_accum || !s_ns_out_buf || !s_ns_processed) {
        /* NS 未就绪，直接复制 */
        if (out != in) {
            memcpy(out, in, samples * sizeof(int16_t));
        }
        return samples;
    }

    size_t out_pos = 0;

    /* 1) 先输出之前处理好但尚未返回的数据 */
    while (out_pos < samples && s_ns_processed_pos < s_ns_processed_len) {
        out[out_pos++] = s_ns_processed[s_ns_processed_pos++];
    }
    if (s_ns_processed_pos >= s_ns_processed_len) {
        s_ns_processed_pos = 0;
        s_ns_processed_len = 0;
    }

    /* 2) 将新输入累积 */
    size_t in_pos = 0;
    while (in_pos < samples) {
        /* 累积输入数据 */
        while (in_pos < samples && (int)s_ns_in_accum_pos < s_ns_frame_size) {
            s_ns_in_accum[s_ns_in_accum_pos++] = in[in_pos++];
        }

        /* 累积够一帧，进行 NS 处理 */
        if ((int)s_ns_in_accum_pos >= s_ns_frame_size) {
            /* 调用 NS 处理 */
            size_t processed = audio_ns_process(s_ns_in_accum, s_ns_out_buf, s_ns_frame_size);

            /* 将处理结果复制到 processed 缓冲区 */
            if (processed > 0) {
                memcpy(s_ns_processed, s_ns_out_buf, processed * sizeof(int16_t));
                s_ns_processed_len = processed;
                s_ns_processed_pos = 0;
            }

            /* 重置累积计数 */
            s_ns_in_accum_pos = 0;

            /* 输出刚处理好的数据 */
            while (out_pos < samples && s_ns_processed_pos < s_ns_processed_len) {
                out[out_pos++] = s_ns_processed[s_ns_processed_pos++];
            }
            if (s_ns_processed_pos >= s_ns_processed_len) {
                s_ns_processed_pos = 0;
                s_ns_processed_len = 0;
            }
        }
    }

    return out_pos;
}
#endif

static inline int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static inline void aggregate_reset(void)
{
    s_aggregate_pos = 0;
}

static inline void vad_reset(void)
{
    s_vad_speaking = false;
    s_vad_silence_start_ms = 0;
    /* VAD 重置时也清空聚合缓冲区，避免残留数据污染下一轮识别 */
    aggregate_reset();
}

static inline bool is_tts_active(void)
{
    /* 兼容两种情况：
     * 1) ASR 连接本身会收到 tts.start/tts.complete
     * 2) 设备侧存在独立的 Chatbot WS（chatbot_client）负责 TTS 播放：用 tts_player_is_active 共享状态
     */
    return s_tts_playing || tts_player_is_active();
}

/* Weak hook: UI layer can override to consume ASR final text (e.g., append to GPT chat + trigger LLM). */
__attribute__((weak)) void app_inmp441_on_asr_final(const char *utf8_text)
{
    (void)utf8_text;
}

/* ==================== ASR client glue (PSRAM middleware) ==================== */

static void inmp441_asr_event_cb(const asr_event_data_t *event, void *user_ctx)
{
    (void)user_ctx;
    if (!event) return;

    switch (event->type) {
    case ASR_EVENT_CONNECTED:
        ESP_LOGI(TAG, "ASR connected");
        vad_reset();
        /* Non-PTT: send start immediately. PTT mode sends start on press. */
        if (!s_asr_ptt_mode) {
            (void)asr_client_send_start(s_asr);
        }
        break;
    case ASR_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "ASR disconnected");
        vad_reset();
        break;
    case ASR_EVENT_PARTIAL_RESULT:
        if (event->text) ESP_LOGD(TAG, "ASR partial: %s", event->text);
        break;
    case ASR_EVENT_FINAL_RESULT:
        if (event->text && event->text[0]) {
            ESP_LOGI(TAG, "ASR final: %s", event->text);
            app_inmp441_on_asr_final(event->text);
        }
        break;
    case ASR_EVENT_TTS_START:
        s_tts_playing = true;
        vad_reset();
        break;
    case ASR_EVENT_TTS_COMPLETE:
        s_tts_playing = false;
        vad_reset();
        break;
    case ASR_EVENT_ERROR:
        ESP_LOGW(TAG, "ASR error: %s", event->error_msg ? event->error_msg : "(unknown)");
        break;
    default:
        break;
    }
}

static bool inmp441_asr_ensure_started(void)
{
    if (ota_update_is_running()) {
        if (s_asr) {
            ESP_LOGW(TAG, "OTA running -> stop ASR client to free bandwidth");
            (void)asr_client_stop(s_asr);
        }
        return false;
    }

    if (!net_wait_ip(pdMS_TO_TICKS(10))) {
        return false;
    }

    if (!s_asr) {
        asr_client_config_t cfg = ASR_CLIENT_DEFAULT_CONFIG();
        cfg.cert_pem = server_root_cert_pem;
        /* Keep legacy behavior: only do ASR, no LLM/TTS on this connection. */
        cfg.disable_llm = true;
        cfg.disable_tts = true;
        cfg.final_only = true;
        cfg.max_samples_per_chunk = INMP441_AGGREGATE_SAMPLES;

        s_asr = asr_client_init(&cfg, inmp441_asr_event_cb, NULL);
        if (!s_asr) {
            ESP_LOGE(TAG, "asr_client_init failed");
            return false;
        }
    }

    if (asr_client_is_connected(s_asr)) {
        return true;
    }

    (void)asr_client_start(s_asr);

    /* Wait up to ~1s for connect (best-effort). */
    int retry = 10;
    while (retry-- > 0 && !asr_client_is_connected(s_asr)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return asr_client_is_connected(s_asr);
}

/* ==================== ASR 启用/禁用 API ==================== */

void app_inmp441_asr_set_enabled(bool enabled)
{
    if (s_asr_enabled == enabled) {
        return;  /* 状态没变，不操作 */
    }

    s_asr_enabled = enabled;
    ESP_LOGI(TAG, "ASR %s", enabled ? "enabled" : "disabled");

    if (!enabled) {
        /* 保险：离开语音输入场景时，清掉 PTT 按下状态，避免下一次进入卡在“按下” */
        s_asr_ptt_pressed = false;
        /* 禁用 ASR 时断开连接，释放带宽/句柄（保留对象以便下次快速启用） */
        if (s_asr) {
            ESP_LOGI(TAG, "Stopping ASR client");
            (void)asr_client_stop(s_asr);
        }
        /* 重置 VAD 状态 */
        vad_reset();
    }
    /* 启用时由主循环自动连接 */
}

bool app_inmp441_asr_is_enabled(void)
{
    return s_asr_enabled;
}

void app_inmp441_asr_set_ptt_mode(bool enabled)
{
    s_asr_ptt_mode = enabled;
    if (!enabled) {
        /* 退出 PTT 模式时清掉按下状态，避免误触发 stop */
        s_asr_ptt_pressed = false;
    }
}

void app_inmp441_asr_ptt_set_pressed(bool pressed)
{
    s_asr_ptt_pressed = pressed;
    /* If user is pressing-to-talk, try to connect ASAP to reduce "press & release with no result". */
    if (pressed && s_asr_enabled && !asr_client_is_connected(s_asr)) {
        (void)inmp441_asr_ensure_started();
    }
}

bool app_inmp441_asr_ptt_is_pressed(void)
{
    return s_asr_ptt_pressed;
}

void app_inmp441_task(void *arg)
{
    (void)arg;

    inmp441_config_t cfg = {
        .sample_rate_hz = 16000,                  // 16 kHz 采样，足够语音/环境噪声分析
        .use_apll       = true,
        .channel        = INMP441_CHANNEL_LEFT,   // 若板上 LR 接到右声道，可改为 RIGHT
        .dma_frame_num  = 0,                      // 使用驱动默认
        .dma_desc_num   = 0,
    };

    esp_err_t er = inmp441_init(&cfg);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "INMP441 init failed: %s", esp_err_to_name(er));
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "INMP441 task started at %d Hz", cfg.sample_rate_hz);

#if INMP441_AEC_ENABLE
    /* 初始化 AEC 模块 */
    audio_aec_config_t aec_cfg = AUDIO_AEC_DEFAULT_CONFIG();
    if (audio_aec_init(&aec_cfg) == ESP_OK) {
        s_aec_initialized = true;
        s_aec_frame_size = audio_aec_get_frame_size();
        ESP_LOGI(TAG, "AEC initialized, frame_size=%d", s_aec_frame_size);

        /* 分配 AEC 缓冲区（使用 PSRAM 减少内部 RAM 压力） */
        if (s_aec_frame_size > 0) {
            const size_t buf_bytes = (size_t)s_aec_frame_size * sizeof(int16_t);

            /* Prefer PSRAM (big, non-DMA). Fallback to internal if PSRAM is unavailable. */
            s_aec_ref_buf = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);
            s_aec_out_buf = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);
            s_aec_mic_accum = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);
            s_aec_processed = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);

            if (!s_aec_ref_buf || !s_aec_out_buf || !s_aec_mic_accum || !s_aec_processed) {
                ESP_LOGE(TAG, "Failed to allocate AEC buffers");
                s_aec_initialized = false;
                SAFE_HEAP_FREE(s_aec_ref_buf);
                SAFE_HEAP_FREE(s_aec_out_buf);
                SAFE_HEAP_FREE(s_aec_mic_accum);
                SAFE_HEAP_FREE(s_aec_processed);
            } else {
                LOG_PTR_LOCATION(TAG, "aec_ref_buf", s_aec_ref_buf);
                LOG_PTR_LOCATION(TAG, "aec_out_buf", s_aec_out_buf);
                LOG_PTR_LOCATION(TAG, "aec_mic_accum", s_aec_mic_accum);
                LOG_PTR_LOCATION(TAG, "aec_processed", s_aec_processed);
                s_aec_mic_accum_pos = 0;
                s_aec_processed_pos = 0;
                s_aec_processed_len = 0;
            }
        }

        /* 启用 I2S TX 参考信号捕获 */
        if (s_aec_initialized) {
            if (!audio_i2s_aec_ref_enable(true, INMP441_AEC_REF_BUFFER_MS)) {
                ESP_LOGW(TAG, "Failed to enable AEC ref capture");
            } else {
                ESP_LOGI(TAG, "AEC ref capture enabled (%d ms buffer)", INMP441_AEC_REF_BUFFER_MS);
            }
        }
    } else {
        ESP_LOGW(TAG, "AEC init failed, running without echo cancellation");
        s_aec_initialized = false;
    }
#endif

#if INMP441_NS_ENABLE
    /* 初始化 NS 模块 */
    audio_ns_config_t ns_cfg = AUDIO_NS_DEFAULT_CONFIG();
    ns_cfg.mode = INMP441_NS_MODE;
    if (audio_ns_init(&ns_cfg) == ESP_OK) {
        s_ns_initialized = true;
        s_ns_frame_size = audio_ns_get_frame_size();
        ESP_LOGI(TAG, "NS initialized, frame_size=%d, mode=%d", s_ns_frame_size, (int)ns_cfg.mode);

        /* 分配 NS 缓冲区 */
        if (s_ns_frame_size > 0) {
            const size_t buf_bytes = (size_t)s_ns_frame_size * sizeof(int16_t);

            s_ns_in_accum = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);
            s_ns_out_buf = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);
            s_ns_processed = (int16_t *)PSRAM_MALLOC_FALLBACK(buf_bytes, TAG);

            if (!s_ns_in_accum || !s_ns_out_buf || !s_ns_processed) {
                ESP_LOGE(TAG, "Failed to allocate NS buffers");
                s_ns_initialized = false;
                SAFE_HEAP_FREE(s_ns_in_accum);
                SAFE_HEAP_FREE(s_ns_out_buf);
                SAFE_HEAP_FREE(s_ns_processed);
            } else {
                LOG_PTR_LOCATION(TAG, "ns_in_accum", s_ns_in_accum);
                LOG_PTR_LOCATION(TAG, "ns_out_buf", s_ns_out_buf);
                LOG_PTR_LOCATION(TAG, "ns_processed", s_ns_processed);
                s_ns_in_accum_pos = 0;
                s_ns_processed_pos = 0;
                s_ns_processed_len = 0;
            }
        }
    } else {
        ESP_LOGW(TAG, "NS init failed, running without noise suppression");
        s_ns_initialized = false;
    }
#endif

    /* 等待 Wi-Fi 拿到 IP（net_wifi_sntp 管理 Wi-Fi + SNTP），最长等待 30 秒 */
    if (!net_wait_ip(pdMS_TO_TICKS(30000))) {
        ESP_LOGW(TAG, "Wi-Fi not ready within 30s, ASR client will retry when enabled");
    } else {
        ESP_LOGI(TAG, "Wi-Fi ready, ASR client will connect when enabled (GPT screen)");
    }
    /* 注意：ASR 默认禁用，只有进入 GPT 界面时才会启用并连接 */

    int16_t buf[INMP441_APP_SAMPLES];
    /* 本地音量日志：仅在状态变化或间隔到达时打印，避免刷屏 */
    const char *last_state = NULL;
    TickType_t  last_log_tick = xTaskGetTickCount();

    bool ptt_streaming = false; /* PTT 模式下，本轮是否已发送 start（按下->start，松开->stop） */

    for (;;) {
        /* OTA 期间完全暂停音频采样任务，释放 I2S/DMA/CPU 资源 */
        if (ota_update_is_running()) {
            /* 必要时主动断开 ASR 连接，避免占用带宽 */
            if (s_asr) {
                ESP_LOGW(TAG, "OTA running -> stop ASR client and pause audio sampling");
                (void)asr_client_stop(s_asr);
            }
            vad_reset();
            ptt_streaming = false;
            /* 暂停期间睡眠，避免 I2S 读取占用资源 */
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        size_t n = inmp441_read_pcm16(buf, INMP441_APP_SAMPLES, pdMS_TO_TICKS(500));
        if (n == 0) {
            ESP_LOGW(TAG, "no samples read within timeout");
            continue;
        }

#if INMP441_AEC_ENABLE
        /* 应用 AEC 处理：消除回声 */
        if (s_aec_initialized) {
            n = aec_process_samples(buf, buf, n);
        }
#endif

#if INMP441_NS_ENABLE
        /* 应用 NS 处理：抑制噪声 */
        if (s_ns_initialized) {
            n = ns_process_samples(buf, buf, n);
        }
#endif

        /* AEC/NS 攒帧期间可能返回 0 样本，跳过后续处理避免除零 */
        if (n == 0) {
            continue;
        }

        bool ptt_mode = s_asr_ptt_mode;
        bool ptt_pressed = s_asr_ptt_pressed;

        /* ASR 功能：只在启用时处理 */
        if (s_asr_enabled) {
            /* 后台尝试保持 ASR 连接（每秒一次） */
            if (!asr_client_is_connected(s_asr)) {
                static TickType_t s_last_try = 0;
                TickType_t now = xTaskGetTickCount();
                if (now - s_last_try > pdMS_TO_TICKS(1000)) {
                    s_last_try = now;
                    (void)inmp441_asr_ensure_started();
                }
            }

            /* 若已连上 Chatbot ASR，则进行音频分片聚合后推送 */
            if (asr_client_is_connected(s_asr) && (!is_tts_active() || (ptt_mode && ptt_pressed))) {
                if (ptt_mode) {
                    /* PTT 模式：只有按下期间才推流。按下 -> 发送 start；松开 -> flush + stop */
                    if (ptt_pressed) {
                        if (!ptt_streaming) {
                            /* 开始一轮新的语音输入：清理上一轮残留并发送 start */
                            vad_reset();
                            (void)asr_client_send_start(s_asr);
                            ptt_streaming = true;
                        }

                        /* 将当前读取的样本追加到聚合缓冲区 */
                        size_t to_copy = n;
                        if (s_aggregate_pos + to_copy > INMP441_AGGREGATE_SAMPLES) {
                            to_copy = INMP441_AGGREGATE_SAMPLES - s_aggregate_pos;
                        }
                        if (to_copy > 0) {
                            memcpy(&s_aggregate_buf[s_aggregate_pos], buf, to_copy * sizeof(int16_t));
                            s_aggregate_pos += to_copy;
                        }

                        /* 聚合缓冲区满时发送（80ms @ 16kHz） */
                        if (s_aggregate_pos >= INMP441_AGGREGATE_SAMPLES) {
                            (void)asr_client_send_audio(s_asr, s_aggregate_buf, s_aggregate_pos);
                            aggregate_reset();
                        }
                    } else {
                        if (ptt_streaming) {
                            /* 松开：把残留的一小段也发掉，再发送 stop，触发 final */
                            if (s_aggregate_pos > 0) {
                                (void)asr_client_send_audio(s_asr, s_aggregate_buf, s_aggregate_pos);
                                aggregate_reset();
                            }
                            (void)asr_client_send_stop(s_asr);
                            vad_reset();
                            ptt_streaming = false;
                        } else {
                            /* 未按下：确保缓冲区为空，避免残留 */
                            aggregate_reset();
                        }
                    }
                } else {
                    /* 非 PTT：维持原有持续推流行为 */
                    /* 将当前读取的样本追加到聚合缓冲区 */
                    size_t to_copy = n;
                    if (s_aggregate_pos + to_copy > INMP441_AGGREGATE_SAMPLES) {
                        to_copy = INMP441_AGGREGATE_SAMPLES - s_aggregate_pos;
                    }
                    if (to_copy > 0) {
                        memcpy(&s_aggregate_buf[s_aggregate_pos], buf, to_copy * sizeof(int16_t));
                        s_aggregate_pos += to_copy;
                    }

                    /* 聚合缓冲区满时发送（80ms @ 16kHz） */
                    if (s_aggregate_pos >= INMP441_AGGREGATE_SAMPLES) {
                        (void)asr_client_send_audio(s_asr, s_aggregate_buf, s_aggregate_pos);
                        aggregate_reset();
                    }
                }
            } else {
                /* 未连接或 TTS 活跃：若 PTT 正在推流，视为被打断，清状态等待下一次触发 */
                if (ptt_streaming) {
                    vad_reset();
                    ptt_streaming = false;
                }
            }
        } else {
            /* ASR 禁用时，确保 PTT 推流状态清空 */
            if (ptt_streaming) {
                vad_reset();
                ptt_streaming = false;
            }
        }

        /* 计算 RMS 与峰值，并给出一个简单的音量等级与“是否有明显声音”的判断 */
        int32_t peak = 0;
        double  sum_sq = 0.0;

        for (size_t i = 0; i < n; ++i) {
            int32_t s = buf[i];
            int32_t a = (s < 0) ? -s : s;
            if (a > peak) peak = a;
            sum_sq += (double)s * (double)s;
        }

        double rms_pcm = sqrt(sum_sq / (double)n);        // PCM16 RMS（0..32768）
        double rms = rms_pcm / 32768.0;                   // 归一化到 0..1
        if (rms < 1e-6) {
            rms = 1e-6; // 避免 log(0)
        }
        double dbfs = 20.0 * log10(rms);  // 0 dBFS ~ 满幅，负值为正常

        /* VAD：检测到"说完了"后发送 stop，让服务端以句为单位做 final
         * - 非 PTT：使用 VAD 静音收口
         * - PTT：由 UI “松开”事件发送 stop，这里不做 VAD */
        if (!s_asr_enabled) {
            /* ASR 禁用时不做 VAD */
        } else if (ptt_mode) {
            /* PTT 模式：不在这里发送 stop，避免按住期间被 VAD 提前截断 */
        } else if (is_tts_active()) {
            /* TTS 播放期间强制清空 VAD 状态，避免回声导致误判 */
            vad_reset();
        } else if (asr_client_is_connected(s_asr)) {
            if (rms_pcm >= (double)INMP441_VAD_RMS_THRESHOLD) {
                s_vad_speaking = true;
                s_vad_silence_start_ms = 0;
            } else if (s_vad_speaking) {
                int64_t tnow = now_ms();
                if (s_vad_silence_start_ms == 0) {
                    s_vad_silence_start_ms = tnow;
                } else if ((tnow - s_vad_silence_start_ms) >= (int64_t)INMP441_VAD_SILENCE_DURATION_MS) {
                    /* 发送 stop 前，先把聚合缓冲区中残留的数据发送出去 */
                    if (s_aggregate_pos > 0) {
                        (void)asr_client_send_audio(s_asr, s_aggregate_buf, s_aggregate_pos);
                        aggregate_reset();
                    }
                    (void)asr_client_send_stop(s_asr);
                    vad_reset();
                }
            }
        } else {
            /* 未连接时不累积状态，避免重连后误触发 stop */
            vad_reset();
        }

        int level = (int)((double)peak * 100.0 / 32767.0);
        if (level > 100) level = 100;
        if (level < 0)   level = 0;

        const char *state = "silence";
        if (level > 10)  state = "quiet";
        if (level > 30)  state = "normal";
        if (level > 60)  state = "loud";

        /* 仅在状态变化或每 2 秒强制打一条日志，降低频率 */
        TickType_t now = xTaskGetTickCount();
        bool state_changed = (state != last_state);
        bool time_passed  = (now - last_log_tick) > pdMS_TO_TICKS(2000);

        if (state_changed || time_passed) {
            ESP_LOGI(TAG,
                     "samples=%u, peak=%" PRIi32 " (level=%d/100), rms=%.4f, dbfs=%.1f dBFS, state=%s",
                     (unsigned)n, peak, level, rms, dbfs, state);
            last_state    = state;
            last_log_tick = now;
        }
    }
}
