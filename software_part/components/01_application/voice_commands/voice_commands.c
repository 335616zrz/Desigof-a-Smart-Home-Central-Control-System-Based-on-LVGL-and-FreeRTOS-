#include "voice_commands.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "voice_afe.h"
#include "server_root_cert.h"
#include "net_wifi_sntp.h"
#include "ota_update.h"
#include "app_env_sht40.h"
#include "audio_player.h"
#include "asr_client.h"

/* Chatbot WebSocket 连接参数（与现有 my_ui/chatbot_client 一致） */
#ifndef CONFIG_MY_UI_CHATBOT_HOST
#define CONFIG_MY_UI_CHATBOT_HOST "servers.local"
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PORT
#define CONFIG_MY_UI_CHATBOT_PORT 443
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PATH
#define CONFIG_MY_UI_CHATBOT_PATH "/ws"
#endif
#ifndef CONFIG_MY_UI_CHATBOT_TLS
#define CONFIG_MY_UI_CHATBOT_TLS 1
#endif

static const char *TAG = "VOICE_CMD";

/* 命令词表：与 MultiNet 模型一一对应（索引即 cmd_id） */
static const char *s_cmd_phrases[] = {
    "连接网络",      // 0
    "深色界面",      // 1
    "浅色界面",      // 2
    "播放网络歌曲",  // 3
    "播放本地音乐",  // 4
    "室内多少度",    // 5
    "外面什么天气",  // 6
};

/* ==================== ASR Client (shared middleware) ==================== */
static asr_client_handle_t s_asr = NULL;

static void vc_asr_event_cb(const asr_event_data_t *event, void *user_ctx)
{
    (void)user_ctx;
    if (!event) return;

    switch (event->type) {
    case ASR_EVENT_CONNECTED:
        ESP_LOGI(TAG, "ASR connected");
        (void)asr_client_send_start(s_asr);
        break;
    case ASR_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "ASR disconnected");
        break;
    case ASR_EVENT_PARTIAL_RESULT:
        if (event->text) ESP_LOGD(TAG, "ASR partial: %s", event->text);
        break;
    case ASR_EVENT_FINAL_RESULT:
        if (event->text) ESP_LOGI(TAG, "ASR final: %s", event->text);
        break;
    case ASR_EVENT_ERROR:
        ESP_LOGW(TAG, "ASR error: %s", event->error_msg ? event->error_msg : "(unknown)");
        break;
    default:
        break;
    }
}

static bool vc_asr_ensure_started(void)
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
        /* Voice commands only need ASR; disable LLM/TTS to save bandwidth and heap. */
        cfg.disable_llm = true;
        cfg.disable_tts = true;
        s_asr = asr_client_init(&cfg, vc_asr_event_cb, NULL);
        if (!s_asr) {
            ESP_LOGE(TAG, "asr_client_init failed");
            return false;
        }
    }

    if (asr_client_is_connected(s_asr)) {
        return true;
    }

    (void)asr_client_start(s_asr);

    /* Wait up to ~1s for connect (best-effort, non-fatal). */
    int retry = 10;
    while (retry-- > 0 && !asr_client_is_connected(s_asr)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return asr_client_is_connected(s_asr);
}

static void __attribute__((unused)) vc_pcm_sink(const int16_t *pcm, size_t samples, void *user_ctx)
{
    (void)user_ctx;
    if (ota_update_is_running()) {
        return;
    }
    if (!net_wait_ip(pdMS_TO_TICKS(10))) {
        return;
    }
    if (!vc_asr_ensure_started()) {
        return;
    }
    (void)asr_client_send_audio(s_asr, pcm, samples);
}

/* ==================== 业务动作（默认弱符号；项目可自行覆盖更复杂逻辑） ==================== */
__attribute__((weak)) void vc_action_connect_last_wifi(void)
{
    wifi_config_t cfg = {0};
    (void)esp_wifi_get_config(WIFI_IF_STA, &cfg);
    ESP_LOGI(TAG, "connect WiFi ssid=%s", (char *)cfg.sta.ssid);
    esp_err_t e = net_force_reconnect();
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "net_force_reconnect failed: %s", esp_err_to_name(e));
    }
}

__attribute__((weak)) void vc_action_set_theme_dark(void)
{
    ESP_LOGI(TAG, "TODO: switch UI to dark theme");
}

__attribute__((weak)) void vc_action_set_theme_light(void)
{
    ESP_LOGI(TAG, "TODO: switch UI to light theme");
}

__attribute__((weak)) void vc_action_play_http_music(void)
{
    ESP_LOGI(TAG, "TODO: start HTTP stream playback");
}

__attribute__((weak)) void vc_action_play_sd_music(void)
{
    ESP_LOGI(TAG, "TODO: start SD card music playback");
}

__attribute__((weak)) void vc_action_report_room_temp(void)
{
    float t = 0, rh = 0;
    if (app_env_sht40_get_latest(&t, &rh)) {
        ESP_LOGI(TAG, "室内温度 %.1f°C 湿度 %.0f%%", t, rh);
    } else {
        ESP_LOGW(TAG, "尚无 SHT40 数据");
    }
}

__attribute__((weak)) void vc_action_report_weather(void)
{
    ESP_LOGI(TAG, "TODO: query weather service and speak result");
}

static void __attribute__((unused)) vc_on_command(int cmd_id, const char *phrase, void *user_ctx)
{
    (void)user_ctx;
    const char *ph = (phrase && phrase[0]) ? phrase :
                     ((cmd_id >=0 && cmd_id < (int)(sizeof(s_cmd_phrases)/sizeof(s_cmd_phrases[0])))
                        ? s_cmd_phrases[cmd_id] : "?");
    ESP_LOGI(TAG, "cmd %d: %s", cmd_id, ph);

    switch (cmd_id) {
    case 0: vc_action_connect_last_wifi(); break;
    case 1: vc_action_set_theme_dark();    break;
    case 2: vc_action_set_theme_light();   break;
    case 3: vc_action_play_http_music();   break;
    case 4: vc_action_play_sd_music();     break;
    case 5: vc_action_report_room_temp();  break;
    case 6: vc_action_report_weather();    break;
    default:
        ESP_LOGW(TAG, "cmd id %d not mapped", cmd_id);
        break;
    }
}

/* ==================== init ==================== */
esp_err_t voice_commands_init(void)
{
#if !CONFIG_VOICE_AFE_ENABLE
    ESP_LOGW(TAG, "VOICE_AFE_ENABLE is disabled; voice commands not started");
    return ESP_OK;
#else
    voice_afe_config_t cfg = {
        .sample_rate_hz = 16000,
        .enable_remote  = true,
        .on_cmd         = vc_on_command,
        .on_cmd_ctx     = NULL,
        .pcm_sink       = vc_pcm_sink,
        .pcm_sink_ctx   = NULL,
    };
    esp_err_t er = voice_afe_init(&cfg);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "voice_afe_init failed: %s", esp_err_to_name(er));
        return er;
    }
    voice_afe_set_state(VOICE_AFE_STATE_REMOTE_ASR);
    ESP_LOGI(TAG, "voice_commands ready (remote streaming ON)");
    return ESP_OK;
#endif
}
