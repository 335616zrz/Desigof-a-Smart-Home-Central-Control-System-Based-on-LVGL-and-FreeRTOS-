#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

/* ST7796 初始化参数 */
typedef struct {
    int width;                   /* 物理朝向宽（常 320） */
    int height;                  /* 物理朝向高（常 480） */
    uint8_t rotation;            /* 0/1/2/3 -> 0/90/180/270 */

    /* SPI 主机与速率 */
    spi_host_device_t spi_host;  /* SPI2_HOST / SPI3_HOST */
    int spi_hz;                  /* 例如 40*1000*1000 */

    /* SPI 引脚 */
    int pin_sclk, pin_mosi, pin_miso, pin_cs;

    /* 控制脚 */
    int pin_dc;                  /* D/CX */
    int pin_rst;                 /* -1 表示未接 */

    /* 背光（可选，-1 表示未接） */
    int pin_bl;
} st7796_config_t;

/* 异步传输完成回调（任务上下文） */
typedef void (*st7796_trans_done_cb_t)(void *user);

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t st7796_init(const st7796_config_t *cfg);
void      st7796_set_rotation(uint8_t r);
void      st7796_set_backlight(uint8_t percent); /* 0..100 */

int       st7796_width(void);
int       st7796_height(void);

/* 同步绘制：RGB565（小端），内部转 BE 再发 */
void      st7796_fill_rect(int x, int y, int w, int h, uint16_t c);
void      st7796_draw_bitmap(int x, int y, int w, int h, const uint16_t *buf);

/* 异步绘制：buf_be 必须为“大端 RGB565”，适合 LVGL 配置了 LV_COLOR_16_SWAP=1 的像素缓存 */
esp_err_t st7796_draw_bitmap_async(int x, int y, int w, int h,
                                   const uint16_t *buf_be,
                                   st7796_trans_done_cb_t done_cb, void *user);

#ifdef __cplusplus
}
#endif

