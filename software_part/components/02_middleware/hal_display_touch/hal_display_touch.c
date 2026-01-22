#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"  // v5.0.8: 使用driver/gpio.h（v5.1+才有esp_driver_gpio.h）

#include "board_config.h"
#include "st7796_driver.h"
#include "ft6336_driver.h"

#include "hal_display_touch.h"

#ifndef HAL_TOUCH_LOG
#define HAL_TOUCH_LOG 0   // 设为 1 可临时打开调试日志
#endif

/* ---------------- 可调工程参数（可在 sdkconfig.h 前置宏覆盖） ---------------- */
#ifndef HAL_TOUCH_TASK_STACK
#define HAL_TOUCH_TASK_STACK   2048   /* 降栈占用：2KB 足够 */
#endif
#ifndef HAL_TOUCH_TASK_PRIO
#define HAL_TOUCH_TASK_PRIO    5
#endif
#ifndef HAL_TOUCH_TASK_CORE
#define HAL_TOUCH_TASK_CORE    1      /* 跟 UI 同核，减少跨核互斥 */
#endif
#ifndef HAL_TOUCH_IDLE_POLLS
#define HAL_TOUCH_IDLE_POLLS   3      /* 抬起后额外轮询次数，滤掉尾巴 */
#endif
#ifndef HAL_TOUCH_POLL_MS
#define HAL_TOUCH_POLL_MS      10     /* 触摸采样间隔 */
#endif
/* -------------------------------------------------------------------------- */

static const char *TAG = "HAL";

static TaskHandle_t s_touch_task_hdl = NULL;  /* 触摸任务句柄（ISR 通知对象） */
static bool         s_isr_installed  = false;
static uint8_t      s_cur_rotation   = (uint8_t)BOARD_LCD_ROTATION;

/* 便捷：限制到当前屏幕尺寸 */
static inline void clamp_to_screen(int *x, int *y)
{
    int w = st7796_width();
    int h = st7796_height();
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x >= w) *x = w - 1;
    if (*y >= h) *y = h - 1;
}

/* ---- INT 回调：FT6336 INT 低电平有效，使用“直接任务通知”最低开销 ---- */
static void IRAM_ATTR touch_isr(void *arg)
{
    (void)arg;
    BaseType_t hp = pdFALSE;
    if (s_touch_task_hdl) vTaskNotifyGiveFromISR(s_touch_task_hdl, &hp);
    if (hp == pdTRUE) portYIELD_FROM_ISR();
}

/* ---- 触摸任务：被 ISR 唤醒，抬起后做少量延时轮询再休眠 ---- */
static void touch_task(void *arg)
{
    (void)arg;
    int rx, ry;     // 驱动层坐标
    int sx, sy;     // 屏幕坐标

    for (;;) {
        /* 等待中断通知 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int idle_cnt = 0;
        for (;;) {
            if (ft6336_read_point(&rx, &ry)) {
                idle_cnt = 0;
                sx = rx; sy = ry;
                clamp_to_screen(&sx, &sy);

#if HAL_TOUCH_LOG
                ESP_LOGI(TAG,
                         "RAW(%d,%d) -> SCR(%d,%d)  W=%d H=%d rot=%u mirX=%d mirY=%d",
                         rx, ry, sx, sy,
                         st7796_width(), st7796_height(),
                         (unsigned)((BOARD_LCD_ROTATION + BOARD_TOUCH_ROT_OFFSET) & 3),
                         (int)BOARD_TOUCH_MIRROR_X, (int)BOARD_TOUCH_MIRROR_Y);
#endif
                /* 若你需要把坐标喂给 LVGL，可在 lvgl_port 的 indev_read 回调里读取 ft6336 */
            } else {
                if (++idle_cnt >= HAL_TOUCH_IDLE_POLLS) break;  /* “抬起”退出内层循环 */
            }
            vTaskDelay(pdMS_TO_TICKS(HAL_TOUCH_POLL_MS));
        }
        /* 回到下一次中断睡眠 */
    }
}

/* ---- 对外 API ---- */
bool display_touch_init(void)
{
    /* --- LCD --- */
    st7796_config_t lcd = {
        .spi_host = BOARD_LCD_SPI_HOST,
        .pin_sclk = BOARD_LCD_PIN_SCLK,
        .pin_mosi = BOARD_LCD_PIN_MOSI,
        .pin_miso = (BOARD_LCD_PIN_MISO >= 0) ? BOARD_LCD_PIN_MISO : -1,
        .pin_cs   = BOARD_LCD_PIN_CS,
        .pin_dc   = BOARD_LCD_PIN_DC,
        .pin_rst  = BOARD_LCD_PIN_RST,
        .pin_bl   = BOARD_LCD_PIN_BL,
        .spi_hz   = BOARD_LCD_SPI_HZ,
        .width    = BOARD_LCD_HRES,   // 320
        .height   = BOARD_LCD_VRES,   // 480
        .rotation = BOARD_LCD_ROTATION
    };
    if (st7796_init(&lcd) != ESP_OK) {
        ESP_LOGE(TAG, "st7796 init failed");
        return false;
    }

    /* --- TOUCH ---
     * 传入“传感器自然方向”尺寸（FT6336 常见 320x480），
     * 旋转=LCD_ROTATION + 补偿（默认 0）
     */
    ft6336_config_t touch = {
        .i2c_port = BOARD_TOUCH_I2C_PORT,
        .sda_io   = BOARD_TOUCH_SDA,
        .scl_io   = BOARD_TOUCH_SCL,
        .rst_io   = BOARD_TOUCH_RST,
        .i2c_hz   = BOARD_TOUCH_I2C_HZ,
        .i2c_addr = BOARD_TOUCH_ADDR,
        .width    = BOARD_LCD_HRES,
        .height   = BOARD_LCD_VRES,
        .rotation = (BOARD_LCD_ROTATION + BOARD_TOUCH_ROT_OFFSET) & 3
    };
    if (ft6336_init(&touch) != ESP_OK) {
        ESP_LOGE(TAG, "ft6336 init failed");
        return false;
    }

    /* --- INT 引脚：下降沿触发 --- */
    gpio_config_t icfg = {
        .pin_bit_mask = 1ULL << BOARD_TOUCH_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&icfg));

    /* --- 建触摸任务，再挂 ISR，防止早到中断丢失 --- */
    BaseType_t ok = xTaskCreatePinnedToCore(
        touch_task, "touch_task",
        HAL_TOUCH_TASK_STACK, NULL, HAL_TOUCH_TASK_PRIO,
        &s_touch_task_hdl, HAL_TOUCH_TASK_CORE);
    if (ok != pdPASS || s_touch_task_hdl == NULL) {
        ESP_LOGE(TAG, "create touch task failed");
        return false;
    }

    if (!s_isr_installed) {
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        s_isr_installed = true;
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOARD_TOUCH_INT, touch_isr, NULL));

    ESP_LOGI(TAG,
             "Display+Touch OK. LCD rot=%d, TOUCH rot=%d (offset=%d), mirX=%d mirY=%d",
             (int)BOARD_LCD_ROTATION,
             (int)((BOARD_LCD_ROTATION + BOARD_TOUCH_ROT_OFFSET) & 3),
             (int)BOARD_TOUCH_ROT_OFFSET,
             (int)BOARD_TOUCH_MIRROR_X, (int)BOARD_TOUCH_MIRROR_Y);
    return true;
}

/* 简单转发给 LCD 驱动 */
void lcd_fill_rect(int x, int y, int w, int h, uint16_t c) { st7796_fill_rect(x, y, w, h, c); }
void lcd_draw_bitmap(int x, int y, int w, int h, const uint16_t *p) { st7796_draw_bitmap(x, y, w, h, p); }

/* 触摸读取：直接用驱动层读到的坐标（驱动已做旋转/镜像映射） */
bool touch_get_point(int *x, int *y)
{
    return ft6336_read_point(x, y);
}

/* ---------------- 运行时旋转控制 ---------------- */

void hal_screen_set_rotation(display_rotation_t rot)
{
    uint8_t r = ((uint8_t)rot) & 3;
    s_cur_rotation = r;

    /* 1) 切 LCD */
    st7796_set_rotation(r);

    /* 2) 同步触摸（额外偏移 + 镜像 由驱动/board 宏处理） */
    ft6336_set_rotation((r + BOARD_TOUCH_ROT_OFFSET) & 3);

    /* 3) 若需要同步 LVGL，可在 lvgl_port 中提供钩子（避免此处强耦合） */
    ESP_LOGI(TAG, "rotation set -> %u (LCD+Touch synced)", (unsigned)r);
}

void hal_screen_toggle_orientation(void)
{
    /* 把 0/2 视作竖屏，1/3 视作横屏：竖<->横 切换 */
    switch (s_cur_rotation & 3) {
        case 0: case 2: hal_screen_set_rotation(DISP_ROT_90);  break;
        case 1: case 3: hal_screen_set_rotation(DISP_ROT_0);   break;
        default:         hal_screen_set_rotation(DISP_ROT_0);   break;
    }
}
