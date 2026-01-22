/**
 * @file audio_ns.h
 * @brief 独立 NS (噪声抑制) 模块
 *
 * 基于 ESP-SR 的 ns_pro API，不需要 model 分区
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NS 抑制模式
 */
typedef enum {
    AUDIO_NS_MODE_MILD       = 0,   /**< 轻度抑制 */
    AUDIO_NS_MODE_MEDIUM     = 1,   /**< 中度抑制 */
    AUDIO_NS_MODE_AGGRESSIVE = 2,   /**< 强力抑制 */
} audio_ns_mode_t;

/**
 * @brief NS 配置
 */
typedef struct {
    audio_ns_mode_t mode;       /**< 抑制模式，默认 MEDIUM */
    int sample_rate;            /**< 采样率，仅支持 16000 Hz */
    int frame_ms;               /**< 帧长度（毫秒），仅支持 10ms */
} audio_ns_config_t;

/**
 * @brief 默认 NS 配置
 */
#define AUDIO_NS_DEFAULT_CONFIG() { \
    .mode = AUDIO_NS_MODE_MEDIUM,   \
    .sample_rate = 16000,           \
    .frame_ms = 10,                 \
}

/**
 * @brief 初始化 NS 模块
 *
 * @param config NS 配置，NULL 使用默认配置
 * @return esp_err_t
 */
esp_err_t audio_ns_init(const audio_ns_config_t *config);

/**
 * @brief 反初始化 NS 模块
 */
void audio_ns_deinit(void);

/**
 * @brief 处理单帧音频（噪声抑制）
 *
 * @param in_data 输入数据 (16-bit PCM, 16kHz)
 * @param out_data 输出数据缓冲区 (16-bit PCM)
 * @param samples 样本数（必须等于帧大小）
 * @return size_t 输出样本数，0 表示失败
 */
size_t audio_ns_process(const int16_t *in_data, int16_t *out_data, size_t samples);

/**
 * @brief 获取 NS 帧大小（每次处理的样本数）
 *
 * @return int 帧大小，0 表示未初始化
 */
int audio_ns_get_frame_size(void);

/**
 * @brief 检查 NS 是否已初始化
 *
 * @return true 已初始化
 * @return false 未初始化
 */
bool audio_ns_is_initialized(void);

/**
 * @brief 设置 NS 抑制模式（运行时调整）
 *
 * @param mode 新的抑制模式
 * @return esp_err_t
 */
esp_err_t audio_ns_set_mode(audio_ns_mode_t mode);

/**
 * @brief 获取当前 NS 抑制模式
 *
 * @return audio_ns_mode_t 当前模式
 */
audio_ns_mode_t audio_ns_get_mode(void);

#ifdef __cplusplus
}
#endif
