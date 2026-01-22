/**
 * @file iot_mqtt.h
 * @brief Lightweight MQTT uplink helper for publishing telemetry to EMQX.
 *
 * Topic format (aligned with IoT Cloud Platform backend):
 *   devices/<device_id>/state
 *
 * Payload format (JSON, minimal):
 *   {"temperature": 23.4, "humidity": 55.2}
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*iot_mqtt_message_cb_t)(const char *topic,
                                     const char *payload,
                                     int payload_len,
                                     void *user_ctx);

/** Start MQTT client (non-blocking). Safe to call multiple times. */
esp_err_t iot_mqtt_start(void);

/** Whether MQTT is currently connected to the broker. */
bool iot_mqtt_is_connected(void);

/** Publish SHT40 telemetry to the default topic. */
esp_err_t iot_mqtt_publish_env(float temp_c, float rh_percent);

/** Generic publish helper (QoS/retain configurable). */
esp_err_t iot_mqtt_publish(const char *topic, const char *payload, int qos, int retain);

/** Publish JSON payload to `devices/<device_id>/state` (used for device "reported" state). */
esp_err_t iot_mqtt_publish_device_state(const char *json_payload, int retain);

/**
 * @brief Register callback for inbound MQTT messages.
 *
 * @note Callback runs in esp-mqtt event loop context; keep it short.
 * @note `topic` is a temporary, null-terminated string valid only within the callback.
 */
esp_err_t iot_mqtt_register_message_callback(iot_mqtt_message_cb_t cb, void *user_ctx);

#ifdef __cplusplus
}
#endif
