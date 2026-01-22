#pragma once
/**
 * @file inmp441_driver.h
 * @brief INMP441 数字麦克风（I2S）简易驱动封装（基于 ESP-IDF v5 i2s_std 接口）
 *
 * 数据约定：
 * - 麦克风输出单声道 24-bit I2S 数据（2's complement，MSB first）；
 * - 本驱动内部以 32-bit 槽位采集，并提供：
 *      - `inmp441_read_raw()`：返回原始 32-bit 槽位数据；
 *      - `inmp441_read_pcm16()`：转换为 16-bit 单声道 PCM（便于与现有播放链路对接）。
 *
 * 典型接线（参考 INMP441 Datasheet 与本工程板卡）：
 * - BCLK  -> BOARD_I2S_MIC_BCLK
 * - LRCLK -> BOARD_I2S_MIC_LRCLK (WS)
 * - DOUT  -> BOARD_I2S_MIC_DATA
 * - LR    -> 固定接 GND = 左声道；或 VCC = 右声道；
 *           若焊到 MCU 引脚，则在 board_config.h 中配置 BOARD_I2S_MIC_LR_SEL，
 *           驱动即可在运行时切换左右声道。
 * - EN    -> 可选上电使能脚，配置为 BOARD_I2S_MIC_CHIPEN（>=0 时由驱动拉高）。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 麦克风逻辑声道（对应 INMP441 的 LR 脚） */
typedef enum {
    INMP441_CHANNEL_LEFT  = 0, /**< LR=0，数据出现在 I2S 左通道 */
    INMP441_CHANNEL_RIGHT = 1, /**< LR=1，数据出现在 I2S 右通道 */
} inmp441_channel_t;

/** 初始化配置 */
typedef struct {
    int  sample_rate_hz;      /**< 采样率（必填，如 16000 / 24000 / 32000 / 48000） */
    bool use_apll;            /**< 1=优先使用 APLL 作为时钟源（若 SoC 支持） */
    inmp441_channel_t channel;/**< 麦克风输出到 I2S 的哪个声道 */

    /* 高级选项（通常保持为 0 即可使用默认值） */
    int dma_frame_num;        /**< 每个 DMA descriptor 帧数；0=使用内部默认值 */
    int dma_desc_num;         /**< DMA descriptor 数量；0=使用内部默认值 */
} inmp441_config_t;

/**
 * @brief 初始化 INMP441 I2S 接收通道
 *
 * - 配置 I2S 标准模式为 Philips I2S，32-bit 槽位，单声道；
 * - 使用 board_config.h 中的 BOARD_I2S_MIC_* 管脚；
 * - 若定义了 BOARD_I2S_MIC_CHIPEN 且 >=0，则在此函数中拉高以使能麦克风；
 * - 若定义了 BOARD_I2S_MIC_LR_SEL 且 >=0，则根据 channel 设置 LR 电平；
 *
 * @return ESP_OK 成功；否则返回 ESP_ERR_xxx 错误码。
 */
esp_err_t inmp441_init(const inmp441_config_t *cfg);

/**
 * @brief 释放 I2S 资源并关闭麦克风
 */
void inmp441_deinit(void);

/**
 * @brief 运行时切换采样率
 *
 * @return ESP_OK 成功；若驱动未初始化或底层 reconfig 失败则返回错误码。
 */
esp_err_t inmp441_set_sample_rate(int fs_hz);

/**
 * @brief 获取当前采样率（Hz）
 * @return 当前采样率；未初始化时返回 0
 */
int inmp441_get_sample_rate(void);

/**
 * @brief 设置麦克风逻辑声道并同步 LR_SEL 引脚（如可用）
 *
 * 仅更新 I2S 槽掩码与 LR_SEL GPIO 电平，不会重新创建通道。
 */
esp_err_t inmp441_set_channel(inmp441_channel_t ch);

/**
 * @brief 获取当前逻辑声道
 */
inmp441_channel_t inmp441_get_channel(void);

/**
 * @brief 判断麦克风通道是否已准备好接收数据
 */
bool inmp441_is_ready(void);

/**
 * @brief 读取原始 32-bit I2S 样本（单声道）
 *
 * @param[out] buf        存放读取结果的 int32_t 缓冲区
 * @param[in]  n_samples  期望读取的样本数
 * @param[in]  timeout    每次底层读取调用的阻塞超时（FreeRTOS ticks）
 * @return 实际读取的样本数（0 表示超时或错误）
 */
size_t inmp441_read_raw(int32_t *buf, size_t n_samples, TickType_t timeout);

/**
 * @brief 读取并转换为 16-bit 单声道 PCM
 *
 * 内部根据 INMP441 的 24-bit 输出，对 32-bit 槽位数据做符号扩展和缩放，
 * 以便上层直接使用标准 int16_t PCM。
 *
 * @param[out] buf        存放读取结果的 int16_t 缓冲区
 * @param[in]  n_samples  期望读取的样本数
 * @param[in]  timeout    每次底层读取调用的阻塞超时（FreeRTOS ticks）
 * @return 实际读取的样本数（0 表示超时或错误）
 */
size_t inmp441_read_pcm16(int16_t *buf, size_t n_samples, TickType_t timeout);

#ifdef __cplusplus
}
#endif
