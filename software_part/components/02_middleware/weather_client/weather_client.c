#include "weather_client.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "WEATHER_CLIENT";

esp_err_t weather_client_init(void)
{
    ESP_LOGI(TAG, "Weather client initialized");
    return ESP_OK;
}

esp_err_t weather_fetch(char *description, size_t desc_len)
{
    if (description == NULL || desc_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {
        .url = CONFIG_WEATHER_CLIENT_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = CONFIG_WEATHER_CLIENT_TIMEOUT_MS,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    const int content_length = esp_http_client_fetch_headers(client);
    size_t buf_len = 2048;
    if (content_length > 0 && content_length < 4096) {
        buf_len = content_length + 1;
    }

    char *buffer = calloc(1, buf_len);
    if (!buffer) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }

    int data_read = esp_http_client_read_response(client, buffer, buf_len - 1);
    if (data_read < 0) {
        ESP_LOGW(TAG, "HTTP read failed: %s", esp_err_to_name(data_read));
        free(buffer);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return data_read;
    }
    buffer[data_read] = '\0';

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGD(TAG, "Weather raw response (%d bytes): %s", data_read, buffer);

    esp_err_t parse_ret = ESP_FAIL;
    cJSON *root = cJSON_Parse(buffer);
    if (root) {
        const cJSON *weather = cJSON_GetObjectItem(root, "weather");
        if (cJSON_IsString(weather) && weather->valuestring) {
            strlcpy(description, weather->valuestring, desc_len);
            parse_ret = ESP_OK;
        } else {
            ESP_LOGW(TAG, "Missing `weather` field in response");
            parse_ret = ESP_ERR_INVALID_RESPONSE;
        }
        cJSON_Delete(root);
    } else {
        ESP_LOGW(TAG, "JSON parse failed");
        parse_ret = ESP_ERR_INVALID_RESPONSE;
    }

    free(buffer);
    return parse_ret;
}
