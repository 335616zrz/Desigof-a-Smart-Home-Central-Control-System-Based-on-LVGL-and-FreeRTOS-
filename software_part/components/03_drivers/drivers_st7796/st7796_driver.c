/* ST7796 SPI LCD 驱动：同步/异步绘制、统一旋转与背光控制
 * - 默认零日志；可用 -DDRV_LOG_ENABLE=1 开启
 * - 要求：传入像素为 RGB565；异步接口要求像素缓冲为“大端字节序”
 */
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_config.h"
#include "st7796_driver.h"

/* ---- 可选日志（默认关闭） ---- */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  static const char *DRV_LOG_TAG = "DRV_ST7796";
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif

static spi_device_handle_t s_dev;
static st7796_config_t     s_cfg;
static int s_cur_w, s_cur_h;
static bool s_bl_inited = false;
static ledc_channel_t s_bl_ch    = LEDC_CHANNEL_0;
static ledc_timer_t   s_bl_timer = LEDC_TIMER_0;
static uint8_t s_rotation = 0;

/* 若你的模组需要固定偏移，可在此调整 */
static int s_xofs = 0, s_yofs = 0;

/* device 队列深度（与 add_device 一致） */
static int s_dev_queue_size = 7;

/* ---------------- SPI 小工具 ---------------- */
static inline void gpio_out(int io, int level){ if (io >= 0) gpio_set_level(io, level); }

static esp_err_t send_cmd(uint8_t cmd){
    gpio_out(s_cfg.pin_dc, 0);
    spi_transaction_t t = { .flags = 0, .length = 8, .tx_buffer = &cmd };
    return spi_device_transmit(s_dev, &t);
}
static esp_err_t send_data(const void *data, size_t len){
    if (!len) return ESP_OK;
    gpio_out(s_cfg.pin_dc, 1);
    spi_transaction_t t = { .flags = 0, .length = len * 8, .tx_buffer = data };
    return spi_device_transmit(s_dev, &t);
}

/* 一行 RGB565 → BE 发送（同步路径用） */
static esp_err_t send_pixels16_be(const uint16_t *src, int count)
{
    static uint8_t rowbuf[480 * 2];                 /* 若更宽可调大 */
    if (count <= 0) return ESP_OK;
    if (count > 480) count = 480;

    for (int i = 0; i < count; ++i) {
        uint16_t c = src[i];
        rowbuf[(i << 1)    ] = (uint8_t)(c >> 8);
        rowbuf[(i << 1) + 1] = (uint8_t)(c & 0xFF);
    }
    return send_data(rowbuf, count * 2);
}

/* 地址窗：固定 2A=列，2B=行；XY 交换交给 MADCTL 的 MV 位 */
static void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
    uint8_t buf[4];
    x0 += s_xofs; x1 += s_xofs; y0 += s_yofs; y1 += s_yofs;

    send_cmd(0x2A); buf[0]=x0>>8; buf[1]=x0; buf[2]=x1>>8; buf[3]=x1; send_data(buf,4);
    send_cmd(0x2B); buf[0]=y0>>8; buf[1]=y0; buf[2]=y1>>8; buf[3]=y1; send_data(buf,4);
    send_cmd(0x2C);
}

/* ---------------- 复位与寄存器 ---------------- */
static void st7796_hw_reset(void){
    if (s_cfg.pin_rst >= 0) {
        gpio_out(s_cfg.pin_rst, 1); vTaskDelay(pdMS_TO_TICKS(5));
        gpio_out(s_cfg.pin_rst, 0); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_out(s_cfg.pin_rst, 1); vTaskDelay(pdMS_TO_TICKS(120));
    } else {
        send_cmd(0x01); vTaskDelay(pdMS_TO_TICKS(120));
    }
}
static void st7796_init_regs(void){
    /* 解锁扩展命令 */
    send_cmd(0xF0); { uint8_t a[]={0xC3}; send_data(a,1); }
    send_cmd(0xF0); { uint8_t b[]={0x96}; send_data(b,1); }

    /* 像素格式：RGB565 */
    send_cmd(0x3A); { uint8_t fmt[]={0x55}; send_data(fmt,1); }

    /* 反相（不少模组观感更佳） */
    send_cmd(0x21);

    /* 走原厂推荐的常规寄存器 */
    send_cmd(0xB4); { uint8_t inv[]={0x01}; send_data(inv,1); }
    send_cmd(0xB7); { uint8_t vs[]={0xC6}; send_data(vs,1); }
    send_cmd(0xE8); { uint8_t e8[]={0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33}; send_data(e8,sizeof(e8)); }

    /* 退出睡眠 + 开显示 */
    send_cmd(0x11); vTaskDelay(pdMS_TO_TICKS(120));
    send_cmd(0x29); vTaskDelay(pdMS_TO_TICKS(20));
}

/* ---------------- 对外接口 ---------------- */
esp_err_t st7796_init(const st7796_config_t *cfg){
    if (!cfg) return ESP_ERR_INVALID_ARG;
    s_cfg = *cfg;

    /* DC/RST/BL */
    const int outs[] = { s_cfg.pin_dc, s_cfg.pin_rst, s_cfg.pin_bl };
    for (size_t i = 0; i < sizeof(outs)/sizeof(outs[0]); ++i){
        if (outs[i] >= 0){
            gpio_config_t io = { .mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << outs[i] };
            (void)gpio_config(&io);
        }
    }
    /* 先关背光，避免初始化期间出现白屏/闪屏 */
    gpio_out(s_cfg.pin_bl, 0);

    /* SPI bus + device */
    spi_bus_config_t bus = {
        .sclk_io_num     = s_cfg.pin_sclk,
        .mosi_io_num     = s_cfg.pin_mosi,
        .miso_io_num     = (s_cfg.pin_miso >= 0) ? s_cfg.pin_miso : -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = s_cfg.width * 2 * 20,  // 行缓冲 20 行
    };
    esp_err_t er = spi_bus_initialize(s_cfg.spi_host, &bus, SPI_DMA_CH_AUTO);
    if (er != ESP_OK) return er;

    spi_device_interface_config_t dev = {
        .clock_speed_hz = s_cfg.spi_hz,
        .mode           = 0,
        .spics_io_num   = s_cfg.pin_cs,
        .queue_size     = s_dev_queue_size,
    };
    er = spi_bus_add_device(s_cfg.spi_host, &dev, &s_dev);
    if (er != ESP_OK) { (void)spi_bus_free(s_cfg.spi_host); return er; }

    st7796_hw_reset();
    st7796_init_regs();

    s_cur_w = s_cfg.width;
    s_cur_h = s_cfg.height;
    st7796_set_rotation(s_cfg.rotation);

    /* 背光（PWM 可选） */
    if (s_cfg.pin_bl >= 0){
        ledc_timer_config_t t = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .timer_num        = s_bl_timer,
            .duty_resolution  = LEDC_TIMER_10_BIT,
            .freq_hz          = 20000,
            .clk_cfg          = LEDC_AUTO_CLK
        };
        (void)ledc_timer_config(&t);

        ledc_channel_config_t c = {
            .gpio_num   = s_cfg.pin_bl,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = s_bl_ch,
            .timer_sel  = s_bl_timer,
            .duty       = 0,
            .hpoint     = 0
        };
        (void)ledc_channel_config(&c);

        s_bl_inited = true;
        /* 默认先保持背光关闭：由 UI 层在启动屏准备好后再打开，避免上电白屏闪烁 */
        st7796_set_backlight(0);
    }

    /* 上电清屏 */
    st7796_fill_rect(0, 0, s_cur_w, s_cur_h, 0x0000);
    DRV_LOGI("LCD ready %dx%d rot=%d", s_cur_w, s_cur_h, s_cfg.rotation);
    return ESP_OK;
}

void st7796_set_rotation(uint8_t r){
    s_rotation = (r & 3);

    /* MADCTL: MY(0x80) MX(0x40) MV(0x20) BGR(0x08) */
    uint8_t mad = 0x08;  /* BGR=1 */
    switch (s_rotation){
        case 0: mad |= 0x00; s_cur_w = s_cfg.width;  s_cur_h = s_cfg.height; break;
        case 1: mad |= 0x60; s_cur_w = s_cfg.height; s_cur_h = s_cfg.width;  break; /* MX|MV */
        case 2: mad |= 0xC0; s_cur_w = s_cfg.width;  s_cur_h = s_cfg.height; break; /* MX|MY */
        case 3: mad |= 0xA0; s_cur_w = s_cfg.height; s_cur_h = s_cfg.width;  break; /* MY|MV */
    }
#if BOARD_LCD_MIRROR_X
    mad ^= 0x40;
#endif
#if BOARD_LCD_MIRROR_Y
    mad ^= 0x80;
#endif
    send_cmd(0x36); (void)send_data(&mad, 1);
}

void st7796_set_backlight(uint8_t percent){
    if (!s_bl_inited) return;
    if (percent > 100) percent = 100;
    uint32_t duty = (percent * 1023U) / 100U;  // 10bit
    (void)ledc_set_duty(LEDC_LOW_SPEED_MODE, s_bl_ch, duty);
    (void)ledc_update_duty(LEDC_LOW_SPEED_MODE, s_bl_ch);
}

int st7796_width(void){ return s_cur_w; }
int st7796_height(void){ return s_cur_h; }

void st7796_fill_rect(int x, int y, int w, int h, uint16_t c){
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= s_cur_w || y >= s_cur_h) return;
    if (x + w > s_cur_w) w = s_cur_w - x;
    if (y + h > s_cur_h) h = s_cur_h - y;

    set_addr_window((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

    static uint16_t line16[480];              /* 若更宽可调大 */
    int line_px = (w > 480) ? 480 : w;

    for (int i = 0; i < line_px; ++i) line16[i] = c;
    for (int row = 0; row < h; ++row) (void)send_pixels16_be(line16, line_px);
}

void st7796_draw_bitmap(int x, int y, int w, int h, const uint16_t *buf){
    if (w <= 0 || h <= 0 || !buf) return;
    if (x < 0) { int cut = -x; x = 0; w -= cut; buf += cut; }
    if (y < 0) { int cut = -y; y = 0; h -= cut; buf += (size_t)cut * w; }
    if (x >= s_cur_w || y >= s_cur_h) return;
    if (x + w > s_cur_w) w = s_cur_w - x;
    if (y + h > s_cur_h) h = s_cur_h - y;

    set_addr_window((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
    for (int row = 0; row < h; ++row) {
        const uint16_t *line = buf + (size_t)row * w;
        (void)send_pixels16_be(line, w);
    }
}

/* ---------------- 异步刷新 ---------------- */
typedef struct {
    int x, y, w, h;
    const uint16_t *buf_be;          /* 必须 DMA 可达（建议内部 RAM 或 LVGL DMA 缓冲） */
    st7796_trans_done_cb_t cb;
    void *user;
    spi_transaction_t *trans_array;  /* h 个事务（逐行） */
} st_async_ctx_t;

static void st_async_worker(void *param)
{
    st_async_ctx_t *ctx = (st_async_ctx_t *)param;

    set_addr_window((uint16_t)ctx->x, (uint16_t)ctx->y,
                    (uint16_t)(ctx->x + ctx->w - 1),
                    (uint16_t)(ctx->y + ctx->h - 1));
    gpio_out(s_cfg.pin_dc, 1);

    for (int row = 0; row < ctx->h; ++row) {
        spi_transaction_t *t = &ctx->trans_array[row];
        memset(t, 0, sizeof(*t));
        t->length    = (size_t)ctx->w * 16;  /* bits */
        t->tx_buffer = (const void *)(ctx->buf_be + (size_t)row * ctx->w);
        if (spi_device_queue_trans(s_dev, t, portMAX_DELAY) != ESP_OK) {
            /* 回收已排队事务后退出 */
            for (int i = 0; i < row; ++i) {
                spi_transaction_t *ret;
                (void)spi_device_get_trans_result(s_dev, &ret, portMAX_DELAY);
            }
            if (ctx->cb) ctx->cb(ctx->user);
            free(ctx->trans_array); free(ctx); vTaskDelete(NULL); return;
        }
    }
    for (int k = 0; k < ctx->h; ++k) {
        spi_transaction_t *ret = NULL;
        (void)spi_device_get_trans_result(s_dev, &ret, portMAX_DELAY);
    }
    if (ctx->cb) ctx->cb(ctx->user);
    free(ctx->trans_array); free(ctx);
    vTaskDelete(NULL);
}

esp_err_t st7796_draw_bitmap_async(int x, int y, int w, int h,
                                   const uint16_t *buf_be,
                                   st7796_trans_done_cb_t done_cb, void *user)
{
    if (w <= 0 || h <= 0 || buf_be == NULL) return ESP_ERR_INVALID_ARG;

    if (x < 0) { int cut = -x; x = 0; w -= cut; buf_be += cut; }
    if (y < 0) { int cut = -y; y = 0; h -= cut; buf_be += (size_t)cut * w; }
    if (x >= s_cur_w || y >= s_cur_h) return ESP_OK;
    if (x + w > s_cur_w) w = s_cur_w - x;
    if (y + h > s_cur_h) h = s_cur_h - y;
    if (w <= 0 || h <= 0) return ESP_OK;

    st_async_ctx_t *ctx = (st_async_ctx_t *)calloc(1, sizeof(st_async_ctx_t));
    if (!ctx) return ESP_ERR_NO_MEM;
    ctx->x = x; ctx->y = y; ctx->w = w; ctx->h = h;
    ctx->buf_be = buf_be; ctx->cb = done_cb; ctx->user = user;

    ctx->trans_array = (spi_transaction_t *)calloc((size_t)h, sizeof(spi_transaction_t));
    if (!ctx->trans_array) { free(ctx); return ESP_ERR_NO_MEM; }

    const uint32_t stack = 2048;
    BaseType_t ok = xTaskCreatePinnedToCore(st_async_worker, "st77xx_async",
                                            stack, ctx, 5, NULL, tskNO_AFFINITY);
    if (ok != pdPASS) { free(ctx->trans_array); free(ctx); return ESP_FAIL; }
    return ESP_OK;
}
