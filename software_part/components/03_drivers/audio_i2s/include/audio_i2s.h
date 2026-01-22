#pragma once
/**
 * @file audio_i2s.h
 * @brief 16-bit 立体声 PCM 到 I2S 的极简输出层（ESP-IDF i2s_std 驱动封装）
 *
 * 数据约定：
 * - 采样位宽：16-bit
 * - 通道：立体声
 * - 排布：交错（LRLR...）
 * - 单位：write 的 n_frames 为“帧”（1 帧 = L16 + R16 = 4 字节）
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 音量上下限（百分比） */
#define AUDIO_I2S_VOL_MIN 0
#define AUDIO_I2S_VOL_MAX 100

/**
 * @brief 初始化 I2S TX 通道并启用输出
 * @param sample_rate_hz 采样率（如 44100、48000）
 * @return true 成功，false 失败
 */
bool audio_i2s_init(int sample_rate_hz);

/**
 * @brief 释放 I2S 资源、关闭通道
 */
void audio_i2s_deinit(void);

/**
 * @brief 写入 PCM 数据（阻塞直到写完或错误）
 * @param st_lr     交错立体声 int16_t 缓冲区指针（长度为 n_frames * 2）
 * @param n_frames  写入帧数（1 帧 = L16 + R16）
 * @return 实际写入的帧数（发生错误则可能小于 n_frames 或为 0）
 */
size_t audio_i2s_write(const int16_t *st_lr, size_t n_frames);

/**
 * @brief 运行时切换采样率（会调用 reconfig 接口）
 * @param fs_hz 新采样率
 * @return true 成功，false 失败
 */
bool audio_i2s_set_sample_rate(int fs_hz);

/**
 * @brief 获取当前 I2S 采样率
 * @return 当前采样率，未初始化时返回 0
 */
int audio_i2s_get_sample_rate(void);

/**
 * @brief 设置静音
 * @param mute true 静音，false 取消静音
 */
void audio_i2s_set_mute(bool mute);

/**
 * @brief 获取静音状态
 * @return true 静音，false 非静音
 */
bool audio_i2s_get_mute(void);

/**
 * @brief 设置音量（百分比）
 * @param vol 0–100
 */
void audio_i2s_set_volume_percent(int vol);

/**
 * @brief 获取当前音量（百分比）
 * @return 0–100
 */
int audio_i2s_get_volume_percent(void);

/**
 * @brief 暂停 I2S 通道（disable 通道时钟）
 * @return true 成功，false 失败或状态不合法
 */
bool audio_i2s_pause(void);

/**
 * @brief 恢复 I2S 通道（enable 通道时钟）
 * @return true 成功，false 失败或状态不合法
 */
bool audio_i2s_resume(void);

/**
 * @brief I2S 是否可写（通道存在且已 enable）
 * @return true 可写，false 不可写
 */
bool audio_i2s_is_ready(void);

/*==================== AEC 参考信号接口 ====================*/

/**
 * @brief 启用 AEC 参考信号捕获
 *
 * 启用后，audio_i2s_write() 会将播放数据保存到内部环形缓冲区，
 * 供 AEC 模块通过 audio_i2s_get_ref_signal() 获取。
 *
 * @param enable true 启用，false 禁用
 * @param buffer_ms 缓冲区大小（毫秒），建议 100-200ms
 * @return true 成功，false 失败
 */
bool audio_i2s_aec_ref_enable(bool enable, int buffer_ms);

/**
 * @brief 获取 AEC 参考信号
 *
 * 从内部环形缓冲区读取最近播放的音频数据，用于 AEC 回声消除。
 * 注意：返回的是单声道 16kHz 数据（从立体声降采样）。
 *
 * @param out_buf 输出缓冲区
 * @param samples 请求的样本数
 * @return size_t 实际读取的样本数
 */
size_t audio_i2s_get_ref_signal(int16_t *out_buf, size_t samples);

/**
 * @brief 清空 AEC 参考信号缓冲区
 */
void audio_i2s_aec_ref_clear(void);

/**
 * @brief 检查是否有可用的参考信号
 *
 * @return size_t 缓冲区中可用的样本数
 */
size_t audio_i2s_aec_ref_available(void);

#ifdef __cplusplus
}
#endif

