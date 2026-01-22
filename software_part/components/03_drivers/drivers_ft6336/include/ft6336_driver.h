#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int i2c_port;      // I2C_NUM_0 / I2C_NUM_1
    int sda_io;        // SDA GPIO
    int scl_io;        // SCL GPIO
    int rst_io;        // 复位脚，没接可填 -1
    uint32_t i2c_hz;   // 100000 或 400000
    uint8_t  i2c_addr; // 7bit，默认 0x38
    int width;         // IC 自然方向宽（常 320）
    int height;        // IC 自然方向高（常 480）
    uint8_t rotation;  // 0..3，与 LCD 一致
} ft6336_config_t;

esp_err_t ft6336_init(const ft6336_config_t *cfg);
void      ft6336_deinit(void);
void      ft6336_set_rotation(uint8_t rotation);
/** 读取一次坐标（按下返回 true，并给出 x/y；否则 false） */
bool      ft6336_read_point(int *x, int *y);

/* 可选：供 HAL 在旋转后更新驱动内部的宽高/方向 */
void      ft6336_set_screen_info(int w, int h, int rotation);

#ifdef __cplusplus
}
#endif

