#include "app_ws2812.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#if !CONFIG_APP_WS2812_LIGHTWEIGHT_JSON
#include "cJSON.h"
#endif

#include "board_config.h"
#include "iot_mqtt.h"
#include "ws2812b_driver.h"

static const char *TAG = "APP_WS2812";

/* ============================== Tunables ============================== */

/* 夜灯白光亮度（0..255）。32~64 通常更适合作为夜灯。 */
#ifndef APP_WS2812_NIGHTLIGHT_BRIGHTNESS
#define APP_WS2812_NIGHTLIGHT_BRIGHTNESS 48
#endif

/* 告警红灯亮度（0..255）。 */
#ifndef APP_WS2812_ALARM_BRIGHTNESS
#define APP_WS2812_ALARM_BRIGHTNESS 96
#endif

/* 告警闪烁节奏：红亮 300ms + 熄灭 700ms（约 1Hz，不刺眼但足够醒目）。 */
#ifndef APP_WS2812_ALARM_ON_MS
#define APP_WS2812_ALARM_ON_MS  300
#endif
#ifndef APP_WS2812_ALARM_OFF_MS
#define APP_WS2812_ALARM_OFF_MS 700
#endif

/* 应用任务刷新周期（越小越“跟手”，但没必要过高频）。 */
#ifndef APP_WS2812_TICK_MS
#define APP_WS2812_TICK_MS  50
#endif

/* ============================== State ============================== */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static bool s_night_light_on = false;
static bool s_alarm_active = false;
static bool s_state_dirty = true; /* publish initial state when MQTT is ready */

/* ============================== Lightweight JSON Parser (zero-alloc) ============================== */
#if CONFIG_APP_WS2812_LIGHTWEIGHT_JSON

/**
 * @brief 在 JSON 字符串中查找指定 key 对应的布尔值（零分配）
 * @param json   JSON 字符串（不需要 null-terminated，但需要 len）
 * @param len    JSON 字符串长度
 * @param key    要查找的键名（如 "on", "active", "nightLightOn"）
 * @param out    输出布尔值
 * @return       true 如果找到并成功解析，false 否则
 *
 * 支持格式：
 *   "key":true / "key":false / "key":1 / "key":0
 *   "key": true（带空格）
 */
static bool json_find_bool(const char *json, int len, const char *key, bool *out)
{
    if (!json || len <= 0 || !key || !out) return false;

    /* 构造搜索模式: "key" */
    char pattern[32];
    int plen = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (plen <= 0 || plen >= (int)sizeof(pattern)) return false;

    /* 在 json 中查找 pattern */
    const char *p = json;
    const char *end = json + len;
    while (p < end) {
        const char *found = (const char *)memmem(p, (size_t)(end - p), pattern, (size_t)plen);
        if (!found) return false;

        /* 跳过 "key" 和后面的冒号/空格 */
        const char *val = found + plen;
        while (val < end && (*val == ' ' || *val == ':' || *val == '\t')) {
            val++;
        }
        if (val >= end) return false;

        /* 解析值 */
        if (end - val >= 4 && memcmp(val, "true", 4) == 0) {
            *out = true;
            return true;
        }
        if (end - val >= 5 && memcmp(val, "false", 5) == 0) {
            *out = false;
            return true;
        }
        if (*val == '1') {
            *out = true;
            return true;
        }
        if (*val == '0') {
            *out = false;
            return true;
        }

        /* 没匹配上，继续往后找（可能是嵌套结构中的同名 key） */
        p = found + 1;
    }
    return false;
}

/**
 * @brief 在 JSON 字符串中查找 "cmd" 字段的字符串值
 * @param json   JSON 字符串
 * @param len    JSON 字符串长度
 * @param out    输出缓冲区
 * @param out_sz 输出缓冲区大小
 * @return       true 如果找到，false 否则
 */
static bool json_find_cmd(const char *json, int len, char *out, size_t out_sz)
{
    if (!json || len <= 0 || !out || out_sz == 0) return false;

    const char *pattern = "\"cmd\"";
    const char *found = (const char *)memmem(json, (size_t)len, pattern, 5);
    if (!found) return false;

    /* 跳过 "cmd" 和后面的冒号/空格/引号 */
    const char *end = json + len;
    const char *p = found + 5;
    while (p < end && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    if (p >= end || *p != '"') return false;
    p++; /* 跳过开头引号 */

    /* 找到结束引号 */
    const char *q = p;
    while (q < end && *q != '"') q++;
    if (q >= end) return false;

    size_t vlen = (size_t)(q - p);
    if (vlen >= out_sz) vlen = out_sz - 1;
    memcpy(out, p, vlen);
    out[vlen] = '\0';
    return true;
}

#else /* !CONFIG_APP_WS2812_LIGHTWEIGHT_JSON: 使用 cJSON */

static inline bool json_get_bool(const cJSON *obj, const char *key, bool *out)
{
    const cJSON *it = cJSON_GetObjectItem(obj, key);
    if (!it) return false;
    if (cJSON_IsBool(it)) {
        *out = cJSON_IsTrue(it);
        return true;
    }
    if (cJSON_IsNumber(it)) {
        *out = (it->valuedouble != 0.0);
        return true;
    }
    return false;
}

#endif /* CONFIG_APP_WS2812_LIGHTWEIGHT_JSON */

static void ws2812_apply_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    /* driver 内部会按 GRB 排列发送 */
    (void)ws2812b_set_rgb(r, g, b);
}

static void ws2812_off(void)
{
    (void)ws2812b_clear();
}

/* MQTT command handler:
 * - {"cmd":"night_light","on":true}
 * - {"cmd":"alarm","active":true}
 * 兼容字段：nightLight / alarm / nightLightOn / alarmActive */
static void on_mqtt_message(const char *topic, const char *payload, int payload_len, void *user_ctx)
{
    (void)user_ctx;
    if (!payload || payload_len <= 0) return;

#if CONFIG_APP_WS2812_LIGHTWEIGHT_JSON
    /* ======== 轻量级解析：零 malloc ======== */
    bool changed = false;

    /* 尝试解析 "cmd" 字段 */
    char cmd[24] = {0};
    if (json_find_cmd(payload, payload_len, cmd, sizeof(cmd))) {
        if (strcmp(cmd, "night_light") == 0) {
            bool on = false;
            if (json_find_bool(payload, payload_len, "on", &on)) {
                taskENTER_CRITICAL(&s_mux);
                changed = (s_night_light_on != on);
                s_night_light_on = on;
                if (changed) s_state_dirty = true;
                taskEXIT_CRITICAL(&s_mux);
                ESP_LOGI(TAG, "MQTT cmd night_light=%d (topic=%s)", (int)on, topic ? topic : "");
            }
        } else if (strcmp(cmd, "alarm") == 0) {
            bool active = false;
            if (json_find_bool(payload, payload_len, "active", &active)) {
                taskENTER_CRITICAL(&s_mux);
                changed = (s_alarm_active != active);
                s_alarm_active = active;
                if (changed) s_state_dirty = true;
                taskEXIT_CRITICAL(&s_mux);
                ESP_LOGW(TAG, "MQTT cmd alarm_active=%d (topic=%s)", (int)active, topic ? topic : "");
            }
        }
    } else {
        /* 兼容：直接字段 */
        bool v = false;
        if (json_find_bool(payload, payload_len, "nightLightOn", &v) ||
            json_find_bool(payload, payload_len, "nightLight", &v)) {
            taskENTER_CRITICAL(&s_mux);
            changed = (s_night_light_on != v);
            s_night_light_on = v;
            if (changed) s_state_dirty = true;
            taskEXIT_CRITICAL(&s_mux);
            ESP_LOGI(TAG, "MQTT night_light=%d (compat) topic=%s", (int)v, topic ? topic : "");
        }
        v = false;
        if (json_find_bool(payload, payload_len, "alarmActive", &v) ||
            json_find_bool(payload, payload_len, "alarm", &v)) {
            taskENTER_CRITICAL(&s_mux);
            changed = (s_alarm_active != v);
            s_alarm_active = v;
            if (changed) s_state_dirty = true;
            taskEXIT_CRITICAL(&s_mux);
            ESP_LOGW(TAG, "MQTT alarm_active=%d (compat) topic=%s", (int)v, topic ? topic : "");
        }
    }
    (void)changed;

#else /* !CONFIG_APP_WS2812_LIGHTWEIGHT_JSON: 使用 cJSON */
    char *buf = (char *)malloc((size_t)payload_len + 1U);
    if (!buf) return;
    memcpy(buf, payload, (size_t)payload_len);
    buf[payload_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        return;
    }

    bool changed = false;

    const cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (cJSON_IsString(cmd) && cmd->valuestring) {
        if (strcmp(cmd->valuestring, "night_light") == 0) {
            bool on = false;
            if (json_get_bool(root, "on", &on)) {
                taskENTER_CRITICAL(&s_mux);
                changed |= (s_night_light_on != on);
                s_night_light_on = on;
                if (changed) s_state_dirty = true;
                taskEXIT_CRITICAL(&s_mux);
                ESP_LOGI(TAG, "MQTT cmd night_light=%d (topic=%s)", (int)on, topic ? topic : "");
            }
        } else if (strcmp(cmd->valuestring, "alarm") == 0) {
            bool active = false;
            if (json_get_bool(root, "active", &active)) {
                taskENTER_CRITICAL(&s_mux);
                changed |= (s_alarm_active != active);
                s_alarm_active = active;
                if (changed) s_state_dirty = true;
                taskEXIT_CRITICAL(&s_mux);
                ESP_LOGW(TAG, "MQTT cmd alarm_active=%d (topic=%s)", (int)active, topic ? topic : "");
            }
        }
    } else {
        /* 兼容：直接字段 */
        bool v = false;
        if (json_get_bool(root, "nightLightOn", &v) || json_get_bool(root, "nightLight", &v)) {
            taskENTER_CRITICAL(&s_mux);
            changed |= (s_night_light_on != v);
            s_night_light_on = v;
            if (changed) s_state_dirty = true;
            taskEXIT_CRITICAL(&s_mux);
            ESP_LOGI(TAG, "MQTT night_light=%d (compat) topic=%s", (int)v, topic ? topic : "");
        }
        if (json_get_bool(root, "alarmActive", &v) || json_get_bool(root, "alarm", &v)) {
            taskENTER_CRITICAL(&s_mux);
            changed |= (s_alarm_active != v);
            s_alarm_active = v;
            if (changed) s_state_dirty = true;
            taskEXIT_CRITICAL(&s_mux);
            ESP_LOGW(TAG, "MQTT alarm_active=%d (compat) topic=%s", (int)v, topic ? topic : "");
        }
    }

    (void)changed;
    cJSON_Delete(root);
#endif /* CONFIG_APP_WS2812_LIGHTWEIGHT_JSON */
}

static void publish_reported_state_if_needed(void)
{
    bool dirty = false;
    bool night_on = false;
    bool alarm_active = false;
    taskENTER_CRITICAL(&s_mux);
    dirty = s_state_dirty;
    night_on = s_night_light_on;
    alarm_active = s_alarm_active;
    taskEXIT_CRITICAL(&s_mux);
    if (!dirty) return;

    if (!iot_mqtt_is_connected()) {
        return; /* keep dirty, retry later */
    }

    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"deviceId\":\"%s\",\"data\":{\"nightLightOn\":%s,\"alarmActive\":%s}}",
             CONFIG_IOT_MQTT_DEVICE_ID,
             night_on ? "true" : "false",
             alarm_active ? "true" : "false");

    if (iot_mqtt_publish_device_state(payload, 1 /*retain*/) == ESP_OK) {
        taskENTER_CRITICAL(&s_mux);
        /* Avoid losing a change that happens during publish. */
        if (s_state_dirty &&
            s_night_light_on == night_on &&
            s_alarm_active == alarm_active) {
            s_state_dirty = false;
        }
        taskEXIT_CRITICAL(&s_mux);
        ESP_LOGI(TAG, "reported state published: nightLight=%d alarm=%d", (int)night_on, (int)alarm_active);
    }
}

void app_ws2812_task(void *arg)
{
    (void)arg;

    /* ========== 延迟初始化 ==========
     * 让音频 I2S 和其他关键组件先初始化，避免 RMT 驱动过早占用内部堆
     * 导致后续 DMA 分配失败。等待 5 秒后再初始化 RMT。
     */
    ESP_LOGI(TAG, "WS2812 task started, waiting 5s for I2S/DMA init to complete...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    ws2812b_config_t cfg = {
        .gpio_num = BOARD_WS2812_GPIO,
        .led_num = BOARD_WS2812_LED_NUM,
        .resolution_hz = 10 * 1000 * 1000,
    };

    esp_err_t er = ws2812b_init(&cfg);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "ws2812b_init failed: %s", esp_err_to_name(er));
        vTaskDelete(NULL);
        return;
    }

    /* 注册 MQTT 下行回调（由 iot_mqtt 统一接收 MQTT_EVENT_DATA） */
    (void)iot_mqtt_register_message_callback(on_mqtt_message, NULL);

    ESP_LOGI(TAG, "WS2812 task started (nightLight=%d alarm=%d)",
             (int)s_night_light_on, (int)s_alarm_active);

    bool last_alarm_active = false;
    bool alarm_phase_on = false;
    int64_t next_toggle_ms = 0;

    uint8_t last_r = 0, last_g = 0, last_b = 0;

    while (1) {
        bool night_on = false;
        bool alarm_on = false;

        taskENTER_CRITICAL(&s_mux);
        night_on = s_night_light_on;
        alarm_on = s_alarm_active;
        taskEXIT_CRITICAL(&s_mux);

        int64_t now_ms = esp_timer_get_time() / 1000;

        uint8_t target_r = 0, target_g = 0, target_b = 0;

        if (alarm_on) {
            if (!last_alarm_active) {
                /* 刚进入告警：立即亮红 */
                alarm_phase_on = true;
                next_toggle_ms = now_ms + APP_WS2812_ALARM_ON_MS;
            } else if (next_toggle_ms > 0 && now_ms >= next_toggle_ms) {
                alarm_phase_on = !alarm_phase_on;
                next_toggle_ms = now_ms + (alarm_phase_on ? APP_WS2812_ALARM_ON_MS : APP_WS2812_ALARM_OFF_MS);
            }

            if (alarm_phase_on) {
                target_r = (uint8_t)APP_WS2812_ALARM_BRIGHTNESS;
                target_g = 0;
                target_b = 0;
            } else {
                target_r = target_g = target_b = 0;
            }
        } else {
            /* 退出告警：复位闪烁状态 */
            alarm_phase_on = false;
            next_toggle_ms = 0;

            if (night_on) {
                uint8_t v = (uint8_t)APP_WS2812_NIGHTLIGHT_BRIGHTNESS;
                target_r = v;
                target_g = v;
                target_b = v;
            } else {
                target_r = target_g = target_b = 0;
            }
        }

        last_alarm_active = alarm_on;

        if (target_r != last_r || target_g != last_g || target_b != last_b) {
            last_r = target_r;
            last_g = target_g;
            last_b = target_b;

            if (target_r == 0 && target_g == 0 && target_b == 0) {
                ws2812_off();
            } else {
                ws2812_apply_rgb(target_r, target_g, target_b);
            }
        }

        /* Publish reported state on any logical state change (not blink phase). */
        publish_reported_state_if_needed();

        vTaskDelay(pdMS_TO_TICKS(APP_WS2812_TICK_MS));
    }
}
