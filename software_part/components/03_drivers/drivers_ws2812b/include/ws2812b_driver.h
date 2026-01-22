#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio_num;            /* DIN 引脚 */
    int led_num;             /* 灯珠数量（本项目=1） */
    uint32_t resolution_hz;  /* RMT 分辨率，推荐 10MHz */
} ws2812b_config_t;

/**
 * @brief 初始化 WS2812B 驱动（基于 ESP-IDF led_strip + RMT）
 *
 * @note 可安全重复调用；重复调用会返回 ESP_OK。
 */
esp_err_t ws2812b_init(const ws2812b_config_t *cfg);

/** @brief 释放资源（可选）。 */
esp_err_t ws2812b_deinit(void);

/** @brief 设置单颗灯珠颜色（RGB）。 */
esp_err_t ws2812b_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/** @brief 清除（关灯）。 */
esp_err_t ws2812b_clear(void);

#ifdef __cplusplus
}
#endif

