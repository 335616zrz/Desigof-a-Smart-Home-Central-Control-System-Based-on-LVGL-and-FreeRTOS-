#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise weather client resources.
 */
esp_err_t weather_client_init(void);

/**
 * @brief Fetch current weather description text.
 *
 * @param description Buffer to receive human-readable weather info.
 * @param desc_len    Buffer length.
 * @return ESP_OK on success, otherwise esp_err_t from HTTP client.
 */
esp_err_t weather_fetch(char *description, size_t desc_len);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_CLIENT_H
