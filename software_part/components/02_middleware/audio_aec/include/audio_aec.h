/**
 * @file audio_aec.h
 * @brief 独立 AEC (回声消除) 模块
 *
 * 基于 ESP-SR 的 afe_aec API，不需要 model 分区
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AEC 配置
 */
typedef struct {
    int filter_length;      /**< 滤波器长度，推荐 4 (ESP32-S3) */
    int frame_size;         /**< 帧大小（样本数），0 = 使用默认值 */
} audio_aec_config_t;

/**
 * @brief 默认 AEC 配置
 */
#define AUDIO_AEC_DEFAULT_CONFIG() { \
    .filter_length = 4,              \
    .frame_size = 0,                 \
}

/**
 * @brief 初始化 AEC 模块
 *
 * @param config AEC 配置，NULL 使用默认配置
 * @return esp_err_t
 */
esp_err_t audio_aec_init(const audio_aec_config_t *config);

/**
 * @brief 反初始化 AEC 模块
 */
void audio_aec_deinit(void);

/**
 * @brief 处理单帧音频（回声消除）
 *
 * @param mic_data 麦克风输入数据 (16-bit PCM, 16kHz)
 * @param ref_data 参考信号数据 (16-bit PCM, 16kHz，来自 I2S TX)
 * @param out_data 输出数据缓冲区 (16-bit PCM)
 * @param samples 样本数
 * @return size_t 输出样本数，0 表示失败
 */
size_t audio_aec_process(const int16_t *mic_data, const int16_t *ref_data,
                         int16_t *out_data, size_t samples);

/**
 * @brief 获取 AEC 帧大小（每次处理的样本数）
 *
 * @return int 帧大小，0 表示未初始化
 */
int audio_aec_get_frame_size(void);

/**
 * @brief 检查 AEC 是否已初始化
 *
 * @return true 已初始化
 * @return false 未初始化
 */
bool audio_aec_is_initialized(void);

/**
 * @brief 重置 AEC 状态（清除内部滤波器状态）
 */
void audio_aec_reset(void);

#ifdef __cplusplus
}
#endif
