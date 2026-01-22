/**
 * @file audio_ns.c
 * @brief 独立 NS (噪声抑制) 实现
 *
 * 使用 ESP-SR 的 ns_pro API 进行噪声抑制
 */

#include "audio_ns.h"
#include "esp_log.h"
#include "esp_ns.h"
#include <string.h>

static const char *TAG = "audio_ns";

/* NS 句柄 */
static ns_handle_t s_ns_handle = NULL;
static int s_frame_size = 0;
static audio_ns_mode_t s_mode = AUDIO_NS_MODE_MEDIUM;
static int s_sample_rate = 16000;

esp_err_t audio_ns_init(const audio_ns_config_t *config)
{
    if (s_ns_handle) {
        ESP_LOGW(TAG, "NS already initialized");
        return ESP_OK;
    }

    audio_ns_config_t cfg = AUDIO_NS_DEFAULT_CONFIG();
    if (config) {
        cfg = *config;
    }

    /* ns_pro 仅支持 10ms 帧长 */
    if (cfg.frame_ms != 10) {
        ESP_LOGW(TAG, "ns_pro only supports 10ms frame, forcing to 10ms");
        cfg.frame_ms = 10;
    }

    /* 仅支持 16kHz */
    if (cfg.sample_rate != 16000) {
        ESP_LOGW(TAG, "NS only supports 16kHz, forcing to 16000");
        cfg.sample_rate = 16000;
    }

    s_mode = cfg.mode;
    s_sample_rate = cfg.sample_rate;

    /* 创建 NS 实例（使用更强的 ns_pro 算法） */
    s_ns_handle = ns_pro_create(cfg.frame_ms, (int)cfg.mode, cfg.sample_rate);
    if (!s_ns_handle) {
        ESP_LOGE(TAG, "ns_pro_create failed");
        return ESP_FAIL;
    }

    /* 计算帧大小：10ms @ 16kHz = 160 samples */
    s_frame_size = (cfg.sample_rate * cfg.frame_ms) / 1000;

    ESP_LOGI(TAG, "NS initialized: mode=%d, frame_size=%d samples (%d ms)",
             (int)cfg.mode, s_frame_size, cfg.frame_ms);
    return ESP_OK;
}

void audio_ns_deinit(void)
{
    if (s_ns_handle) {
        ns_destroy(s_ns_handle);
        s_ns_handle = NULL;
        s_frame_size = 0;
        ESP_LOGI(TAG, "NS deinitialized");
    }
}

size_t audio_ns_process(const int16_t *in_data, int16_t *out_data, size_t samples)
{
    if (!s_ns_handle || !in_data || !out_data || samples == 0) {
        return 0;
    }

    /* NS 要求固定帧大小 */
    if ((int)samples != s_frame_size) {
        ESP_LOGW(TAG, "Sample count mismatch: got %u, expected %d",
                 (unsigned)samples, s_frame_size);
        /* 如果样本数不匹配，直接复制（不做 NS） */
        memcpy(out_data, in_data, samples * sizeof(int16_t));
        return samples;
    }

    /* 调用 NS 处理 */
    ns_process(s_ns_handle, (int16_t *)in_data, out_data);

    return samples;
}

int audio_ns_get_frame_size(void)
{
    return s_frame_size;
}

bool audio_ns_is_initialized(void)
{
    return s_ns_handle != NULL;
}

esp_err_t audio_ns_set_mode(audio_ns_mode_t mode)
{
    if (!s_ns_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    /* ns_pro 不支持运行时更改模式，需要重新创建 */
    if (mode == s_mode) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Changing NS mode from %d to %d (requires reinit)", (int)s_mode, (int)mode);

    /* 销毁旧实例 */
    ns_destroy(s_ns_handle);

    /* 创建新实例 */
    s_ns_handle = ns_pro_create(10, (int)mode, s_sample_rate);
    if (!s_ns_handle) {
        ESP_LOGE(TAG, "ns_pro_create failed during mode change");
        s_frame_size = 0;
        return ESP_FAIL;
    }

    s_mode = mode;
    return ESP_OK;
}

audio_ns_mode_t audio_ns_get_mode(void)
{
    return s_mode;
}
