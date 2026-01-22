/**
 * @file audio_aec.c
 * @brief 独立 AEC (回声消除) 实现
 *
 * 使用 ESP-SR 的 afe_aec API 进行回声消除
 */

#include "audio_aec.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_afe_aec.h"
#include <string.h>

static const char *TAG = "audio_aec";

/* AEC 句柄 */
static afe_aec_handle_t *s_aec_handle = NULL;
static int s_frame_size = 0;

/* 内部缓冲区（交错格式：Mic + Ref） */
static int16_t *s_interleaved_buf = NULL;
static size_t s_interleaved_buf_size = 0;

esp_err_t audio_aec_init(const audio_aec_config_t *config)
{
    if (s_aec_handle) {
        ESP_LOGW(TAG, "AEC already initialized");
        return ESP_OK;
    }

    audio_aec_config_t cfg = AUDIO_AEC_DEFAULT_CONFIG();
    if (config) {
        cfg = *config;
    }

    /* 创建 AEC 实例
     * 输入格式 "MR" = Mic(1通道) + Reference(1通道)
     * filter_length: 滤波器长度，越大效果越好但 CPU 负载越高
     * AFE_TYPE_SR: 语音识别场景
     * AFE_MODE_HIGH_PERF: 高性能模式
     */
    s_aec_handle = afe_aec_create("MR", cfg.filter_length, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    if (!s_aec_handle) {
        ESP_LOGE(TAG, "afe_aec_create failed");
        return ESP_FAIL;
    }

    /* 获取帧大小 */
    s_frame_size = afe_aec_get_chunksize(s_aec_handle);
    if (s_frame_size <= 0) {
        ESP_LOGE(TAG, "Invalid frame size: %d", s_frame_size);
        afe_aec_destroy(s_aec_handle);
        s_aec_handle = NULL;
        return ESP_FAIL;
    }

    /* 分配交错缓冲区 (Mic + Ref, 每帧 2 通道)
     * 优先使用 PSRAM 减少内部 RAM 压力 */
    s_interleaved_buf_size = s_frame_size * 2 * sizeof(int16_t);
    s_interleaved_buf = heap_caps_aligned_calloc(16, 1, s_interleaved_buf_size,
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_interleaved_buf) {
        /* 回退到内部 RAM */
        ESP_LOGW(TAG, "PSRAM alloc failed, fallback to internal RAM");
        s_interleaved_buf = heap_caps_aligned_calloc(16, 1, s_interleaved_buf_size,
                                                      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (!s_interleaved_buf) {
        ESP_LOGE(TAG, "Failed to allocate interleaved buffer");
        afe_aec_destroy(s_aec_handle);
        s_aec_handle = NULL;
        s_frame_size = 0;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "AEC initialized: filter_len=%d, frame_size=%d samples",
             cfg.filter_length, s_frame_size);
    return ESP_OK;
}

void audio_aec_deinit(void)
{
    if (s_interleaved_buf) {
        heap_caps_free(s_interleaved_buf);
        s_interleaved_buf = NULL;
        s_interleaved_buf_size = 0;
    }

    if (s_aec_handle) {
        afe_aec_destroy(s_aec_handle);
        s_aec_handle = NULL;
        s_frame_size = 0;
        ESP_LOGI(TAG, "AEC deinitialized");
    }
}

size_t audio_aec_process(const int16_t *mic_data, const int16_t *ref_data,
                         int16_t *out_data, size_t samples)
{
    if (!s_aec_handle || !mic_data || !out_data || samples == 0) {
        return 0;
    }

    /* AEC 要求固定帧大小，这里按帧处理 */
    if ((int)samples != s_frame_size) {
        ESP_LOGW(TAG, "Sample count mismatch: got %u, expected %d",
                 (unsigned)samples, s_frame_size);
        /* 如果样本数不匹配，直接复制麦克风数据（不做 AEC） */
        memcpy(out_data, mic_data, samples * sizeof(int16_t));
        return samples;
    }

    /* 准备交错格式数据：[mic0, ref0, mic1, ref1, ...] */
    for (int i = 0; i < s_frame_size; i++) {
        s_interleaved_buf[i * 2] = mic_data[i];
        s_interleaved_buf[i * 2 + 1] = ref_data ? ref_data[i] : 0;
    }

    /* 调用 AEC 处理 */
    size_t out_bytes = afe_aec_process(s_aec_handle, s_interleaved_buf, out_data);
    size_t out_samples = out_bytes / sizeof(int16_t);

    return out_samples;
}

int audio_aec_get_frame_size(void)
{
    return s_frame_size;
}

bool audio_aec_is_initialized(void)
{
    return s_aec_handle != NULL;
}

void audio_aec_reset(void)
{
    if (s_aec_handle) {
        /* ESP-SR AEC 没有提供 reset API，需要重新创建 */
        ESP_LOGW(TAG, "AEC reset not supported, recreating...");
        audio_aec_config_t cfg = AUDIO_AEC_DEFAULT_CONFIG();
        audio_aec_deinit();
        audio_aec_init(&cfg);
    }
}
