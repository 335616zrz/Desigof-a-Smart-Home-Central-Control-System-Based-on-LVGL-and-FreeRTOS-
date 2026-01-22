/**
 * FT6336/FT6336U 触摸驱动（ESP-IDF v5+ i2c_master 总线/设备模型）
 * - 仅驱动逻辑：初始化、读取 P1、旋转/镜像映射
 * - 默认零日志；如需排障，在 CMake 中加 -DDRV_LOG_ENABLE=1
 */
#include "ft6336_driver.h"
#include "board_config.h"
#include "i2c_touch_bus.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---- 可选日志（默认关闭） ---- */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  static const char *DRV_LOG_TAG = "DRV_FT6336";
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif

/* 镜像策略（可被 board_config.h 覆盖） */
#ifndef BOARD_TOUCH_MIRROR_X
#define BOARD_TOUCH_MIRROR_X  0
#endif
#ifndef BOARD_TOUCH_MIRROR_Y
#define BOARD_TOUCH_MIRROR_Y  0
#endif
#ifndef FT_MIRROR_X
#define FT_MIRROR_X  BOARD_TOUCH_MIRROR_X
#endif
#ifndef FT_MIRROR_Y
#define FT_MIRROR_Y  BOARD_TOUCH_MIRROR_Y
#endif

#ifndef FT6336_I2C_ADDR_DEFAULT
#define FT6336_I2C_ADDR_DEFAULT  0x38
#endif

/* 寄存器 */
#define FT_REG_TD_STATUS      0x02
#define FT_REG_P1_XH          0x03
#define FT_REG_CHIP_ID        0xA8

typedef struct {
    int      port;
    int      rst;
    uint32_t hz;
    uint8_t  addr;
    int      raw_w, raw_h;     /* IC“自然方向”尺寸 */
    uint8_t  rot;              /* 与 LCD 一致的旋转（0..3） */
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
} ft_ctx_t;

static ft_ctx_t s_ctx = {0};
/* 兼容旧接口（保留占位） */
static int s_w = 0, s_h = 0, s_rot = 0;

/* ---------------- I2C 基础 ---------------- */
static inline esp_err_t ft_i2c_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_ctx.dev, &reg, 1, data, len, 1000);
}

/* ---------------- 坐标映射 ---------------- */
static inline void map_rotate_xy(int tx, int ty, int *x, int *y)
{
    const int rw = s_ctx.raw_w, rh = s_ctx.raw_h;
    int ox = tx, oy = ty;

    switch (s_ctx.rot & 3) {
        case 0: break;
        case 1: ox = ty;            oy = rw - 1 - tx; break;
        case 2: ox = rw - 1 - tx;   oy = rh - 1 - ty; break;
        case 3: ox = rh - 1 - ty;   oy = tx;          break;
    }
    const int out_w = (s_ctx.rot & 1) ? rh : rw;
    const int out_h = (s_ctx.rot & 1) ? rw : rh;

#if FT_MIRROR_X
    ox = out_w - 1 - ox;
#endif
#if FT_MIRROR_Y
    oy = out_h - 1 - oy;
#endif

    if (ox < 0) ox = 0; else if (ox >= out_w) ox = out_w - 1;
    if (oy < 0) oy = 0; else if (oy >= out_h) oy = out_h - 1;

    *x = ox; *y = oy;
}

/* ---------------- 对外接口 ---------------- */
esp_err_t ft6336_init(const ft6336_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    /* 若重复 init，先移除旧设备；I2C 总线由 i2c_touch_bus 统一管理，不在此删除 */
    if (s_ctx.dev) {
        (void)i2c_master_bus_rm_device(s_ctx.dev);
        s_ctx.dev = NULL;
    }
    s_ctx.bus   = NULL;

    s_ctx.port  = cfg->i2c_port;
    s_ctx.rst   = cfg->rst_io;
    s_ctx.hz    = cfg->i2c_hz ? cfg->i2c_hz : BOARD_TOUCH_I2C_HZ;
    s_ctx.addr  = cfg->i2c_addr ? cfg->i2c_addr : FT6336_I2C_ADDR_DEFAULT;
    s_ctx.raw_w = cfg->width;
    s_ctx.raw_h = cfg->height;
    s_ctx.rot   = (cfg->rotation & 3);

    /* 1) 获取与 SHT40 共享的 I2C 总线（I2C0, GPIO17/18） */
    esp_err_t er = i2c_touch_bus_get(&s_ctx.bus);
    if (er != ESP_OK) return er;

    /* 2) 设备 */
    if (s_ctx.hz > 400000) s_ctx.hz = 400000;
    i2c_device_config_t devcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = s_ctx.addr,
        .scl_speed_hz    = s_ctx.hz,
    };
    er = i2c_master_bus_add_device(s_ctx.bus, &devcfg, &s_ctx.dev);
    if (er != ESP_OK) { s_ctx.dev = NULL; return er; }

    /* 3) 复位（可选） */
    if (s_ctx.rst >= 0) {
        gpio_config_t rstcfg = {
            .pin_bit_mask = 1ULL << s_ctx.rst,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = 1,
            .pull_down_en = 0,
            .intr_type = GPIO_INTR_DISABLE,
        };
        (void)gpio_config(&rstcfg);
        gpio_set_level(s_ctx.rst, 0); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(s_ctx.rst, 1); vTaskDelay(pdMS_TO_TICKS(50));
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* 4) 探测（可选） */
    uint8_t chip_id = 0;
    if (ft_i2c_read_reg(FT_REG_CHIP_ID, &chip_id, 1) == ESP_OK) {
        DRV_LOGI("ID=0x%02X raw=%dx%d rot=%u", chip_id, s_ctx.raw_w, s_ctx.raw_h, (unsigned)s_ctx.rot);
    }

    return ESP_OK;
}

void ft6336_deinit(void)
{
    if (s_ctx.dev) {
        (void)i2c_master_bus_rm_device(s_ctx.dev);
        s_ctx.dev = NULL;
    }
    /* I2C 总线由 i2c_touch_bus 统一管理，这里不删除 */
    s_ctx.bus = NULL;
}

bool ft6336_read_point(int *x, int *y)
{
    uint8_t buf[7];
    if (ft_i2c_read_reg(FT_REG_TD_STATUS, buf, sizeof(buf)) != ESP_OK) return false;

    if ((buf[0] & 0x0F) == 0) return false; /* 无触点 */

    /* P1: XH[3:0], XL, YH[3:0], YL, ... */
    uint16_t rx = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
    uint16_t ry = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];

    int sx, sy; map_rotate_xy((int)rx, (int)ry, &sx, &sy);
    if (x) { *x = sx; } 
    if (y) { *y = sy; }
    return true;
}

void ft6336_set_rotation(uint8_t rotation) { s_ctx.rot = (rotation & 3); }

/* 兼容保留：当前驱动不使用它们 */
void ft6336_set_screen_info(int w, int h, int rotation)
{ s_w=w; s_h=h; s_rot=rotation; (void)s_w; (void)s_h; (void)s_rot; }
