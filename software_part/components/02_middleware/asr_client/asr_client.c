#include "asr_client.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"

#include "cJSON.h"
#include "mbedtls/base64.h"

#include "mem_utils.h"

static const char *TAG = "asr_client";

struct asr_client_s {
    esp_websocket_client_handle_t ws;
    asr_client_config_t           config;
    asr_event_cb_t                event_cb;
    void                         *user_ctx;

    bool started;
    volatile bool connected;

    SemaphoreHandle_t lock;

    /* persistent extra headers (must outlive ws config) */
    char headers[160];

    /* RX reassembly (PSRAM, fixed cap) */
    char   *rx_buf;
    size_t  rx_cap;
    size_t  rx_expected;

    /* TX buffers (PSRAM) */
    size_t max_samples;
    unsigned char *b64_buf;
    size_t b64_cap;
    unsigned char *json_buf;
    size_t json_cap;
};

static void emit_evt(asr_client_handle_t h, asr_event_type_t type,
                     const char *text, const char *err)
{
    if (!h || !h->event_cb) return;
    const asr_event_data_t evt = {
        .type = type,
        .text = text,
        .error_msg = err,
    };
    /* Never hold internal locks when calling user callback. */
    h->event_cb(&evt, h->user_ctx);
}

static void handle_server_json(asr_client_handle_t h, const char *json, int len)
{
    if (!h || !json || len <= 0) return;

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) {
        ESP_LOGW(TAG, "json parse failed");
        emit_evt(h, ASR_EVENT_ERROR, NULL, "json_parse_failed");
        return;
    }

    const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (!cJSON_IsString(type) || !type->valuestring) {
        cJSON_Delete(root);
        return;
    }
    const char *t = type->valuestring;

    if (strncmp(t, "asr.", 4) == 0) {
        const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
        const char *s = (cJSON_IsString(text) && text->valuestring) ? text->valuestring : NULL;
        if (s && s[0]) {
            const bool is_final = (strcmp(t, "asr.final") == 0);
            if (is_final) {
                emit_evt(h, ASR_EVENT_FINAL_RESULT, s, NULL);
            } else {
                if (!h->config.final_only) {
                    emit_evt(h, ASR_EVENT_PARTIAL_RESULT, s, NULL);
                }
            }
        }
        cJSON_Delete(root);
        return;
    }

    if (strncmp(t, "tts.", 4) == 0) {
        if (strcmp(t, "tts.start") == 0 || strcmp(t, "tts.meta") == 0) {
            emit_evt(h, ASR_EVENT_TTS_START, NULL, NULL);
        } else if (strcmp(t, "tts.complete") == 0 || strcmp(t, "tts.error") == 0) {
            emit_evt(h, ASR_EVENT_TTS_COMPLETE, NULL, NULL);
        }
        cJSON_Delete(root);
        return;
    }

    if (strcmp(t, "error") == 0) {
        const cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, "detail");
        const char *d = (cJSON_IsString(detail) && detail->valuestring) ? detail->valuestring : "server_error";
        emit_evt(h, ASR_EVENT_ERROR, NULL, d);
        cJSON_Delete(root);
        return;
    }

    /* ignore other message types */
    cJSON_Delete(root);
}

static void ws_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)base;
    asr_client_handle_t h = (asr_client_handle_t)handler_args;
    esp_websocket_event_data_t *e = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        h->connected = true;
        ESP_LOGI(TAG, "ws connected");
        emit_evt(h, ASR_EVENT_CONNECTED, NULL, NULL);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "ws disconnected");
        h->connected = false;
        h->rx_expected = 0;
        emit_evt(h, ASR_EVENT_DISCONNECTED, NULL, NULL);
        break;
    case WEBSOCKET_EVENT_DATA:
        if (!e || e->op_code != WS_TRANSPORT_OPCODES_TEXT || !e->data_ptr || e->data_len <= 0) {
            break;
        }
        /* Reassemble fragmented JSON text into fixed PSRAM buffer. */
        {
            const size_t total = e->payload_len ? (size_t)e->payload_len : (size_t)e->data_len;
            const size_t off   = (size_t)e->payload_offset;

            if (off == 0) {
                if (total + 1 > h->rx_cap) {
                    ESP_LOGE(TAG, "RX payload too large: %u > cap %u",
                             (unsigned)total, (unsigned)h->rx_cap);
                    h->rx_expected = 0;
                    emit_evt(h, ASR_EVENT_ERROR, NULL, "rx_payload_too_large");
                    break;
                }
                h->rx_expected = total;
            }

            if (h->rx_expected == 0) {
                break;
            }
            if (off + (size_t)e->data_len + 1 > h->rx_cap) {
                ESP_LOGE(TAG, "RX overflow: off=%u len=%u cap=%u",
                         (unsigned)off, (unsigned)e->data_len, (unsigned)h->rx_cap);
                h->rx_expected = 0;
                emit_evt(h, ASR_EVENT_ERROR, NULL, "rx_overflow");
                break;
            }

            memcpy(h->rx_buf + off, e->data_ptr, (size_t)e->data_len);
            if (off + (size_t)e->data_len >= h->rx_expected) {
                h->rx_buf[h->rx_expected] = '\0';
                handle_server_json(h, h->rx_buf, (int)h->rx_expected);
                h->rx_expected = 0;
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        if (e) {
            ESP_LOGE(TAG,
                     "ws error: type=%d tls_esp=%d tls_stack=%d vfy=%d ws_status=%d sock_errno=%d",
                     (int)e->error_handle.error_type,
                     (int)e->error_handle.esp_tls_last_esp_err,
                     (int)e->error_handle.esp_tls_stack_err,
                     (int)e->error_handle.esp_tls_cert_verify_flags,
                     (int)e->error_handle.esp_ws_handshake_status_code,
                     (int)e->error_handle.esp_transport_sock_errno);
        } else {
            ESP_LOGE(TAG, "ws error (no detail)");
        }
        h->connected = false;
        h->rx_expected = 0;
        emit_evt(h, ASR_EVENT_ERROR, NULL, "ws_error");
        break;
    default:
        break;
    }
}

static esp_err_t build_ws_config(asr_client_handle_t h, esp_websocket_client_config_t *out)
{
    if (!h || !out) return ESP_ERR_INVALID_ARG;

    /* Default fallbacks (keep in sync with existing app configs) */
#ifndef CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS
#define CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS 10000
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC
#define CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC 10
#endif
#ifndef CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS
#define CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS 8000
#endif

    memset(out, 0, sizeof(*out));

    const asr_client_config_t *cfg = &h->config;

    out->host = cfg->host;
    out->port = cfg->port;
    out->path = cfg->path;
    out->buffer_size = 2048;
    out->disable_auto_reconnect = false;
    out->reconnect_timeout_ms = CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS;
    out->subprotocol = "json";
    out->network_timeout_ms = CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS;
    out->ping_interval_sec = CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC;

    /* Minimal Origin header for compatibility with some proxies. */
    const int n = snprintf(h->headers, sizeof(h->headers),
                           "Origin: https://%s\r\n", cfg->host ? cfg->host : "localhost");
    if (n <= 0 || n >= (int)sizeof(h->headers)) {
        h->headers[0] = '\0';
    }
    out->headers = h->headers[0] ? h->headers : NULL;

    if (cfg->use_tls) {
        out->transport = WEBSOCKET_TRANSPORT_OVER_SSL;

#if CONFIG_MY_UI_CHATBOT_TLS_INSECURE
        out->cert_pem = NULL;
        out->use_global_ca_store = false;
        out->skip_cert_common_name_check = true;
        ESP_LOGW(TAG, "TLS insecure mode enabled");
#else
        ESP_RETURN_ON_FALSE(cfg->cert_pem != NULL, ESP_ERR_INVALID_ARG, TAG,
                            "TLS enabled but cert_pem is NULL");
        out->cert_pem = cfg->cert_pem;
        out->use_global_ca_store = false;
#if CONFIG_MY_UI_CHATBOT_TLS_SKIP_CN
        out->skip_cert_common_name_check = true;
#else
        out->skip_cert_common_name_check = false;
#endif
#endif /* TLS insecure */
    } else {
        out->transport = WEBSOCKET_TRANSPORT_OVER_TCP;
    }

    return ESP_OK;
}

asr_client_handle_t asr_client_init(const asr_client_config_t *config,
                                    asr_event_cb_t cb, void *ctx)
{
    ESP_RETURN_ON_FALSE(cb != NULL, NULL, TAG, "event_cb required");

    MEM_DIAG("asr_init_start");

    asr_client_handle_t h = (asr_client_handle_t)calloc(1, sizeof(*h));
    ESP_RETURN_ON_FALSE(h != NULL, NULL, TAG, "OOM for handle");

    h->config = config ? *config : (asr_client_config_t)ASR_CLIENT_DEFAULT_CONFIG();
    h->event_cb = cb;
    h->user_ctx = ctx;

    if (h->config.rx_buf_size == 0) {
        h->config.rx_buf_size = 64 * 1024;
    }
    if (h->config.max_samples_per_chunk == 0) {
        h->config.max_samples_per_chunk = 2048;
    }
    if (!h->config.language) {
        h->config.language = "zh";
    }

    h->max_samples = h->config.max_samples_per_chunk;
    const size_t max_bytes = h->max_samples * sizeof(int16_t);
    const size_t b64_need = ((max_bytes + 2) / 3) * 4 + 8; /* + NUL slack */

    h->rx_cap = h->config.rx_buf_size;
    h->b64_cap = b64_need;
    h->json_cap = h->b64_cap + 256;

    h->rx_buf = (char *)PSRAM_MALLOC(h->rx_cap);
    h->b64_buf = (unsigned char *)PSRAM_MALLOC(h->b64_cap);
    h->json_buf = (unsigned char *)PSRAM_MALLOC(h->json_cap);
    if (!h->rx_buf || !h->b64_buf || !h->json_buf) {
        ESP_LOGE(TAG, "PSRAM alloc failed: rx=%p b64=%p json=%p",
                 h->rx_buf, h->b64_buf, h->json_buf);
        goto fail;
    }

    LOG_PTR_LOCATION(TAG, "rx_buf", h->rx_buf);
    LOG_PTR_LOCATION(TAG, "b64_buf", h->b64_buf);
    LOG_PTR_LOCATION(TAG, "json_buf", h->json_buf);
    ESP_LOGI(TAG, "buffers: rx=%uKB b64=%uB json=%uB (max_samples=%u)",
             (unsigned)(h->rx_cap / 1024),
             (unsigned)h->b64_cap,
             (unsigned)h->json_cap,
             (unsigned)h->max_samples);

    h->lock = xSemaphoreCreateMutex();
    if (!h->lock) {
        ESP_LOGE(TAG, "create mutex failed");
        goto fail;
    }

    esp_websocket_client_config_t ws_cfg;
    if (build_ws_config(h, &ws_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "build ws config failed");
        goto fail;
    }

    h->ws = esp_websocket_client_init(&ws_cfg);
    if (!h->ws) {
        ESP_LOGE(TAG, "ws client init failed");
        goto fail;
    }

    esp_websocket_register_events(h->ws, WEBSOCKET_EVENT_ANY, ws_event, h);

    MEM_DIAG("asr_init_done");
    return h;

fail:
    if (h) {
        if (h->ws) {
            esp_websocket_client_destroy(h->ws);
            h->ws = NULL;
        }
        if (h->lock) {
            vSemaphoreDelete(h->lock);
            h->lock = NULL;
        }
        SAFE_HEAP_FREE(h->rx_buf);
        SAFE_HEAP_FREE(h->b64_buf);
        SAFE_HEAP_FREE(h->json_buf);
        free(h);
    }
    MEM_DIAG("asr_init_fail");
    return NULL;
}

void asr_client_deinit(asr_client_handle_t h)
{
    if (!h) return;

    (void)asr_client_stop(h);
    if (h->ws) {
        esp_websocket_client_destroy(h->ws);
        h->ws = NULL;
    }

    if (h->lock) {
        vSemaphoreDelete(h->lock);
        h->lock = NULL;
    }

    SAFE_HEAP_FREE(h->rx_buf);
    SAFE_HEAP_FREE(h->b64_buf);
    SAFE_HEAP_FREE(h->json_buf);
    free(h);

    MEM_DIAG("asr_deinit");
}

esp_err_t asr_client_start(asr_client_handle_t h)
{
    ESP_RETURN_ON_FALSE(h != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    ESP_RETURN_ON_FALSE(h->ws != NULL, ESP_ERR_INVALID_STATE, TAG, "ws not init");

    if (h->started) return ESP_OK;

    xSemaphoreTake(h->lock, portMAX_DELAY);
    esp_err_t err = esp_websocket_client_start(h->ws);
    if (err == ESP_OK) {
        h->started = true;
    }
    xSemaphoreGive(h->lock);
    return err;
}

esp_err_t asr_client_stop(asr_client_handle_t h)
{
    ESP_RETURN_ON_FALSE(h != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    if (!h->started) return ESP_OK;

    xSemaphoreTake(h->lock, portMAX_DELAY);
    (void)esp_websocket_client_stop(h->ws);
    h->started = false;
    h->connected = false;
    h->rx_expected = 0;
    xSemaphoreGive(h->lock);
    return ESP_OK;
}

bool asr_client_is_connected(asr_client_handle_t h)
{
    return h && h->connected;
}

esp_err_t asr_client_send_start(asr_client_handle_t h)
{
    ESP_RETURN_ON_FALSE(h && h->ws, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(h->connected, ESP_ERR_INVALID_STATE, TAG, "not connected");

    xSemaphoreTake(h->lock, portMAX_DELAY);

    int n = 0;
    /* Build a compact JSON message (avoid cJSON allocations). */
    n = snprintf((char *)h->json_buf, h->json_cap,
                 "{\"type\":\"start\",\"sampleRate\":%d,\"language\":\"%s\"%s%s%s}",
                 h->config.sample_rate,
                 h->config.language ? h->config.language : "zh",
                 h->config.disable_llm ? ",\"disableLLM\":true" : "",
                 h->config.disable_tts ? ",\"disableTTS\":true" : "",
                 h->config.final_only ? ",\"asrFinalOnly\":true" : "");

    if (n <= 0 || (size_t)n >= h->json_cap) {
        xSemaphoreGive(h->lock);
        return ESP_ERR_INVALID_SIZE;
    }

    int rc = esp_websocket_client_send_text(h->ws, (const char *)h->json_buf, n, 8000);
    xSemaphoreGive(h->lock);
    return (rc >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t asr_client_send_stop(asr_client_handle_t h)
{
    ESP_RETURN_ON_FALSE(h && h->ws, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(h->connected, ESP_ERR_INVALID_STATE, TAG, "not connected");

    static const char kStop[] = "{\"type\":\"stop\"}";

    xSemaphoreTake(h->lock, portMAX_DELAY);
    int rc = esp_websocket_client_send_text(h->ws, kStop, (int)sizeof(kStop) - 1, 8000);
    xSemaphoreGive(h->lock);
    return (rc >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t asr_client_send_audio(asr_client_handle_t h,
                                const int16_t *pcm16, size_t samples)
{
    ESP_RETURN_ON_FALSE(h && h->ws, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(h->connected, ESP_ERR_INVALID_STATE, TAG, "not connected");
    ESP_RETURN_ON_FALSE(pcm16 && samples > 0, ESP_ERR_INVALID_ARG, TAG, "invalid audio");

    if (samples > h->max_samples) {
        ESP_LOGE(TAG, "samples %u > max %u",
                 (unsigned)samples, (unsigned)h->max_samples);
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t bytes = samples * sizeof(int16_t);
    size_t b64_len = 0;

    xSemaphoreTake(h->lock, portMAX_DELAY);

    int rc = mbedtls_base64_encode(h->b64_buf, h->b64_cap, &b64_len,
                                   (const unsigned char *)pcm16, bytes);
    if (rc != 0 || b64_len == 0 || b64_len + 1 >= h->b64_cap) {
        xSemaphoreGive(h->lock);
        ESP_LOGW(TAG, "base64 encode failed rc=%d len=%u", rc, (unsigned)b64_len);
        return ESP_ERR_INVALID_SIZE;
    }
    h->b64_buf[b64_len] = '\0';

    int json_len = snprintf((char *)h->json_buf, h->json_cap,
                            "{\"type\":\"audio\",\"chunk\":\"%s\",\"sampleRate\":%d}",
                            (const char *)h->b64_buf, h->config.sample_rate);
    if (json_len <= 0 || (size_t)json_len >= h->json_cap) {
        xSemaphoreGive(h->lock);
        ESP_LOGW(TAG, "audio json overflow");
        return ESP_ERR_INVALID_SIZE;
    }

    rc = esp_websocket_client_send_text(h->ws, (const char *)h->json_buf, json_len, 8000);
    xSemaphoreGive(h->lock);
    return (rc >= 0) ? ESP_OK : ESP_FAIL;
}
