#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取与 FT6336 / SHT40 共用的 I2C 总线句柄。
 *
 * - 使用 board_config.h 中的 BOARD_TOUCH_* 宏配置 I2C0；
 * - 只会在首次调用时创建总线；后续复用同一 bus；
 * - 调用方仅负责 add/remove device，不要删除 bus 本身。
 */
esp_err_t i2c_touch_bus_get(i2c_master_bus_handle_t *out_bus);

#ifdef __cplusplus
}
#endif

