#pragma once
#include "esp_timer.h"

/* LVGL v9 基本配置：RGB565 + 16位字节交换（SPI屏常见） */
#define LV_COLOR_DEPTH              16
#define LV_COLOR_16_SWAP            0

/* 使用系统时间（esp_timer）作为 LVGL Tick 源（ms） */
#define LV_TICK_CUSTOM              1
#define LV_TICK_CUSTOM_INCLUDE      "esp_timer.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR ((uint32_t)(esp_timer_get_time()/1000ULL))

/* 适当精简日志 */
#define LV_USE_LOG                  0
