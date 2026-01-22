#include "app_env_sht40.h"

#include <math.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h" /* vTaskDeleteWithCaps */

#include "esp_err.h"
#include "esp_log.h"

#include "board_config.h"
#include "sht40_driver.h"
#include "iot_mqtt.h"

/* ====================== SHT40 滤波管线：median3 + 自适应 EMA + deadband ====================== */

typedef struct {
    float buf[3];
    int   index;
    bool  filled;
} median3_t;

static inline void median3_init(median3_t *m)
{
    m->index  = 0;
    m->filled = false;
}

static float median3_push(median3_t *m, float x)
{
    m->buf[m->index] = x;
    m->index = (m->index + 1) % 3;

    if (!m->filled && m->index == 0) {
        m->filled = true;
    }

    if (!m->filled) {
        return x;
    }

    float a = m->buf[0], b = m->buf[1], c = m->buf[2];

    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    if (a > b) { float t = a; a = b; b = t; }

    return b;
}

typedef struct {
    float alpha_base;
    float alpha_max;
    float threshold;
    float value;
    bool  initialized;
} adaptive_ema_t;

static inline void adaptive_ema_init(adaptive_ema_t *f,
                                     float alpha_base,
                                     float alpha_max,
                                     float threshold)
{
    f->alpha_base  = alpha_base;
    f->alpha_max   = alpha_max;
    f->threshold   = threshold;
    f->value       = 0.0f;
    f->initialized = false;
}

static float adaptive_ema_update(adaptive_ema_t *f, float x)
{
    if (!f->initialized) {
        f->value       = x;
        f->initialized = true;
        return x;
    }

    float error = fabsf(x - f->value);
    float alpha = f->alpha_base;

    if (error > f->threshold) {
        float scale = error / f->threshold;
        if (scale > 3.0f) scale = 3.0f;
        alpha = f->alpha_base * scale;
        if (alpha > f->alpha_max) alpha = f->alpha_max;
    }

    f->value = alpha * x + (1.0f - alpha) * f->value;
    return f->value;
}

typedef struct {
    float last_out;
    bool  initialized;
} hold_filter_t;

static inline void hold_init(hold_filter_t *h)
{
    h->initialized = false;
}

static float hold_update(hold_filter_t *h, float in, float threshold)
{
    if (!h->initialized) {
        h->last_out    = in;
        h->initialized = true;
        return in;
    }

    if (fabsf(in - h->last_out) >= threshold) {
        h->last_out = in;
    }
    return h->last_out;
}

/* 滤波状态：温度/湿度各一组 */
static median3_t       s_temp_med;
static median3_t       s_rh_med;
static adaptive_ema_t  s_temp_ema;
static adaptive_ema_t  s_rh_ema;
static hold_filter_t   s_temp_hold;
static hold_filter_t   s_rh_hold;

/* 对外导出的最近一次 UI 友好数值（室内环境） */
static float           s_temp_ui = 0.0f;
static float           s_rh_ui   = 0.0f;
static bool            s_has_sample = false;

/* 访问 s_temp_ui/s_rh_ui 的简易临界区保护 */
static portMUX_TYPE    s_env_mux = portMUX_INITIALIZER_UNLOCKED;

static void env_sht40_filters_init(void)
{
    median3_init(&s_temp_med);
    median3_init(&s_rh_med);

    /* 自适应 EMA 参数：
     * - 温度：基准 alpha≈0.12（约 8s），最高 0.4，1.5°C 起加速；
     * - 湿度：基准 alpha≈0.06（约 15s），最高 0.25，8%RH 起加速。 */
    adaptive_ema_init(&s_temp_ema, 0.12f, 0.40f, 1.5f);
    adaptive_ema_init(&s_rh_ema,   0.06f, 0.25f, 8.0f);

    hold_init(&s_temp_hold);
    hold_init(&s_rh_hold);
    s_temp_ui     = 0.0f;
    s_rh_ui       = 0.0f;
    s_has_sample  = false;
}

static bool env_sht40_read_sample(float *t_c, float *rh)
{
    float raw_t = 0.0f;
    float raw_h = 0.0f;

    if (sht40_read(&raw_t, &raw_h) == ESP_OK &&
        raw_t >= -40.0f && raw_t <= 125.0f &&
        raw_h >=   0.0f && raw_h <= 100.0f) {
        if (t_c) *t_c = raw_t;
        if (rh)  *rh  = raw_h;
        return true;
    }
    return false;
}

bool app_env_sht40_get_latest(float *t_c, float *rh)
{
    bool has = false;
    taskENTER_CRITICAL(&s_env_mux);
    has = s_has_sample;
    if (has) {
        if (t_c) *t_c = s_temp_ui;
        if (rh)  *rh  = s_rh_ui;
    }
    taskEXIT_CRITICAL(&s_env_mux);
    return has;
}

void app_env_sht40_task(void *arg)
{
    (void)arg;
    static const char *TAG = "ENV_SHT40";

    sht40_config_t cfg = {
        .i2c_port  = BOARD_SHT4X_I2C_PORT,
        .sda_io    = BOARD_TOUCH_SDA,
        .scl_io    = BOARD_TOUCH_SCL,
        .i2c_hz    = BOARD_SHT4X_I2C_HZ,
        .i2c_addr  = BOARD_SHT4X_ADDR,
        .precision = SHT40_PRECISION_HIGH,
        .heater    = SHT40_HEATER_OFF,
    };

    esp_err_t er = sht40_init(&cfg);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "SHT40 init failed: %s", esp_err_to_name(er));
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_APP_ENV_SHT40_USE_PSRAM_STACK
        vTaskDeleteWithCaps(NULL);
#else
        vTaskDelete(NULL);
#endif
    }

    env_sht40_filters_init();

    /* 给传感器和 I2C 总线一点额外时间稳定，避免上电后首包偶发 NACK */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* MQTT 发布计数器：按配置的周期发送 */
    uint32_t mqtt_counter = 0;
    const uint32_t mqtt_period_ms = CONFIG_IOT_MQTT_PUB_PERIOD_MS;
    const uint32_t mqtt_interval_ticks = (mqtt_period_ms / 1000); // 采样周期为1Hz

    for (;;) {
        float raw_t = 0.0f;
        float raw_h = 0.0f;

        if (!env_sht40_read_sample(&raw_t, &raw_h)) {
            ESP_LOGW(TAG, "SHT40 sample invalid or read failed");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        /* 1) median-of-3 去除单点尖刺 */
        float t_med = median3_push(&s_temp_med, raw_t);
        float h_med = median3_push(&s_rh_med,   raw_h);

        /* 2) 自适应 EMA 平滑 */
        float t_f = adaptive_ema_update(&s_temp_ema, t_med);
        float h_f = adaptive_ema_update(&s_rh_ema,   h_med);

        /* 3) UI deadband：小于阈值的微小波动不刷新输出 */
        float t_ui = hold_update(&s_temp_hold, t_f, 0.1f); // 0.1°C
        float h_ui = hold_update(&s_rh_hold,   h_f, 0.5f); // 0.5%RH

        /* 4) 更新导出值并按需打日志（仅在变化较明显时打印，减小 LOG 噪声） */
        taskENTER_CRITICAL(&s_env_mux);
        s_temp_ui    = t_ui;
        s_rh_ui      = h_ui;
        s_has_sample = true;
        taskEXIT_CRITICAL(&s_env_mux);

        /* 仅当温度或湿度相对上一次日志有明显变化时才打印 */
        static float s_last_log_t  = 0.0f;
        static float s_last_log_rh = 0.0f;
        static bool  s_have_log    = false;

        float dt  = fabsf(t_ui  - s_last_log_t);
        float drh = fabsf(h_ui  - s_last_log_rh);
        const float T_THRESH  = 0.05f;  // 0.05°C 以上变化才记录
        const float RH_THRESH = 0.3f;   // 0.3%RH 以上变化才记录

        if (!s_have_log || dt >= T_THRESH || drh >= RH_THRESH) {
            ESP_LOGI(TAG, "T=%.2f C, RH=%.1f %%",
                     (double)t_ui, (double)h_ui);
            s_last_log_t  = t_ui;
            s_last_log_rh = h_ui;
            s_have_log    = true;
        }

        /* MQTT 发布：按配置的周期上报 */
        mqtt_counter++;
        if (mqtt_counter >= mqtt_interval_ticks) {
            mqtt_counter = 0;
#if CONFIG_IOT_MQTT_ENABLE
            if (iot_mqtt_is_connected()) {
                esp_err_t pub_ret = iot_mqtt_publish_env(t_ui, h_ui);
                if (pub_ret != ESP_OK) {
                    if (pub_ret == ESP_ERR_TIMEOUT) {
                        ESP_LOGI(TAG, "MQTT publish skipped: SNTP time not synced yet");
                    } else {
                        ESP_LOGW(TAG, "MQTT publish failed: %s", esp_err_to_name(pub_ret));
                    }
                }
            }
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 Hz 采样
    }
}
