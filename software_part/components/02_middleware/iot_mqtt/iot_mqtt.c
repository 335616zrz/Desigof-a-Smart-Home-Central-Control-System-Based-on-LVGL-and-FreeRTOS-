#include "iot_mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"

#include "net_wifi_sntp.h"

#ifndef CONFIG_IOT_MQTT_ENABLE
#define CONFIG_IOT_MQTT_ENABLE 0
#endif

static const char *TAG = "IOT_MQTT";

#if CONFIG_IOT_MQTT_ENABLE
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected                    = false;
static bool s_time_synced                  = false;
static int64_t s_last_time_unsynced_warn_us = 0;

static iot_mqtt_message_cb_t s_msg_cb = NULL;
static void *s_msg_cb_ctx = NULL;

static void mqtt_subscribe_default_topics(void)
{
    if (!s_client) return;

    char topic_cmd[96];
    snprintf(topic_cmd, sizeof(topic_cmd), "devices/%s/command/#", CONFIG_IOT_MQTT_DEVICE_ID);

    int id1 = esp_mqtt_client_subscribe(s_client, topic_cmd, 1);
    int id2 = esp_mqtt_client_subscribe(s_client, "devices/broadcast/command/#", 1);

    ESP_LOGI(TAG, "📥 Subscribed: %s (msg_id=%d)", topic_cmd, id1);
    ESP_LOGI(TAG, "📥 Subscribed: %s (msg_id=%d)", "devices/broadcast/command/#", id2);
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "✅ MQTT Connected to broker successfully!");
        ESP_LOGI(TAG, "   Broker: %s", CONFIG_IOT_MQTT_BROKER_URI);
        ESP_LOGI(TAG, "   Device: %s", CONFIG_IOT_MQTT_DEVICE_ID);
        mqtt_subscribe_default_topics();
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "❌ MQTT Disconnected from broker (auto-reconnect enabled)");
        break;
    case MQTT_EVENT_ERROR:
        s_connected = false;
        ESP_LOGE(TAG, "❌ MQTT event error: transport %d", (int)event->error_handle->error_type);
        if (event->error_handle->esp_tls_last_esp_err != 0) {
            ESP_LOGE(TAG, "   TLS error: 0x%x", event->error_handle->esp_tls_last_esp_err);
        }
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "📤 MQTT message published successfully, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        if (s_msg_cb && event->topic && event->data) {
            /* esp-mqtt 提供的 topic/data 默认不是 '\0' 结尾 */
            char *topic = NULL;
            const int tlen = event->topic_len;
            if (tlen > 0 && tlen < 192) {
                char topic_buf[192];
                memcpy(topic_buf, event->topic, (size_t)tlen);
                topic_buf[tlen] = '\0';
                s_msg_cb(topic_buf, event->data, event->data_len, s_msg_cb_ctx);
            } else if (tlen > 0) {
                topic = (char *)malloc((size_t)tlen + 1U);
                if (topic) {
                    memcpy(topic, event->topic, (size_t)tlen);
                    topic[tlen] = '\0';
                    s_msg_cb(topic, event->data, event->data_len, s_msg_cb_ctx);
                    free(topic);
                }
            }
        }
        break;
    default:
        break;
    }
}

esp_err_t iot_mqtt_start(void)
{
    if (s_client) {
        ESP_LOGI(TAG, "MQTT client already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "🚀 Starting MQTT client...");
    ESP_LOGI(TAG, "   Broker URI: %s", CONFIG_IOT_MQTT_BROKER_URI);
    ESP_LOGI(TAG, "   Username: %s", CONFIG_IOT_MQTT_USERNAME);
    ESP_LOGI(TAG, "   Device ID: %s", CONFIG_IOT_MQTT_DEVICE_ID);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_IOT_MQTT_BROKER_URI,
        .credentials = {
            .username = CONFIG_IOT_MQTT_USERNAME,
            .authentication.password = CONFIG_IOT_MQTT_PASSWORD,
        },
        .session = {
            .keepalive = 30,
            .disable_clean_session = false,
        },
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "❌ esp_mqtt_client_init failed");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t er = esp_mqtt_client_start(s_client);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "❌ esp_mqtt_client_start failed: %s", esp_err_to_name(er));
        return er;
    }

    ESP_LOGI(TAG, "⏳ MQTT client started, waiting for connection...");
    return ESP_OK;
}

bool iot_mqtt_is_connected(void)
{
    return s_connected;
}

esp_err_t iot_mqtt_register_message_callback(iot_mqtt_message_cb_t cb, void *user_ctx)
{
    s_msg_cb = cb;
    s_msg_cb_ctx = user_ctx;
    return ESP_OK;
}

esp_err_t iot_mqtt_publish(const char *topic, const char *payload, int qos, int retain)
{
    if (!topic || !topic[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!payload) {
        payload = "";
    }
    if (!s_client) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    if (qos < 0) qos = 0;
    if (qos > 2) qos = 2;

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0 /*len*/,
                                        qos, retain ? 1 : 0);
    return (msg_id < 0) ? ESP_FAIL : ESP_OK;
}

esp_err_t iot_mqtt_publish_device_state(const char *json_payload, int retain)
{
    char topic[96];
    snprintf(topic, sizeof(topic), "devices/%s/state", CONFIG_IOT_MQTT_DEVICE_ID);
    return iot_mqtt_publish(topic, json_payload, 1 /*qos*/, retain ? 1 : 0);
}

esp_err_t iot_mqtt_publish_env(float temp_c, float rh_percent)
{
    if (!s_client) {
        ESP_LOGW(TAG, "⚠️  MQTT client not initialized, skipping publish");
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_connected) {
        ESP_LOGW(TAG, "⚠️  MQTT not connected, skipping publish");
        return ESP_ERR_INVALID_STATE;
    }

    /* 避免在 SNTP 未同步时发布“1970-01-01...”时间戳 */
    if (!s_time_synced) {
        if (!net_wait_time_synced(0)) {
            const int64_t now_us = esp_timer_get_time();
            if (now_us - s_last_time_unsynced_warn_us > 5 * 1000000LL) {
                s_last_time_unsynced_warn_us = now_us;
                ESP_LOGW(TAG, "⏳ SNTP time not synced yet, skip publish to avoid epoch timestamp");
            }
            return ESP_ERR_TIMEOUT;
        }
        s_time_synced = true;
        ESP_LOGI(TAG, "🕒 SNTP time synced, MQTT telemetry timestamps enabled");
    }

    char topic[96];
    snprintf(topic, sizeof(topic), "devices/%s/state", CONFIG_IOT_MQTT_DEVICE_ID);

    /* 获取当前时间戳 (ISO 8601格式) */
    time_t now = 0;
    struct tm timeinfo = {0};
    char timestamp[32] = {0};

    time(&now);
    gmtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);

    /* 构建云平台期望的 JSON 格式 */
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"deviceId\":\"%s\",\"timestamp\":\"%s\",\"data\":{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}}",
             CONFIG_IOT_MQTT_DEVICE_ID,
             timestamp,
             (double)temp_c,
             (double)rh_percent,
             1013.25); /* 默认气压值，如果有传感器可以传入真实值 */

    ESP_LOGI(TAG, "📡 Publishing to MQTT...");
    ESP_LOGI(TAG, "   Topic: %s", topic);
    ESP_LOGI(TAG, "   Temp: %.2f°C, Humidity: %.2f%%", (double)temp_c, (double)rh_percent);

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0 /*len*/,
                                         1 /*qos*/, 0 /*retain*/);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "❌ Publish failed! topic=%s", topic);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✅ Publish queued, msg_id=%d", msg_id);
    ESP_LOGI(TAG, "   Payload: %s", payload);
    return ESP_OK;
}

#else /* CONFIG_IOT_MQTT_ENABLE */

esp_err_t iot_mqtt_start(void)
{
    ESP_LOGW(TAG, "IOT_MQTT_ENABLE is disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

bool iot_mqtt_is_connected(void) { return false; }

esp_err_t iot_mqtt_register_message_callback(iot_mqtt_message_cb_t cb, void *user_ctx)
{
    (void)cb;
    (void)user_ctx;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t iot_mqtt_publish(const char *topic, const char *payload, int qos, int retain)
{
    (void)topic;
    (void)payload;
    (void)qos;
    (void)retain;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t iot_mqtt_publish_device_state(const char *json_payload, int retain)
{
    (void)json_payload;
    (void)retain;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t iot_mqtt_publish_env(float temp_c, float rh_percent)
{
    (void)temp_c;
    (void)rh_percent;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif /* CONFIG_IOT_MQTT_ENABLE */
