#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "sdkconfig.h" /* ensure Kconfig macros are visible */

#ifdef __cplusplus
extern "C" {
#endif

/* Kconfig fallbacks (component can be built standalone) */
#ifndef CONFIG_MY_UI_CHATBOT_HOST
#define CONFIG_MY_UI_CHATBOT_HOST "localhost"
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PORT
#define CONFIG_MY_UI_CHATBOT_PORT 443
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PATH
#define CONFIG_MY_UI_CHATBOT_PATH "/ws"
#endif
#ifndef CONFIG_MY_UI_CHATBOT_TLS
#define CONFIG_MY_UI_CHATBOT_TLS 1
#endif

typedef struct {
    const char *host;
    int         port;
    const char *path;
    bool        use_tls;
    const char *cert_pem;
    int         sample_rate;
    const char *language;
    bool        disable_llm;
    bool        disable_tts;
    bool        final_only;

    /* PSRAM buffer config */
    size_t      rx_buf_size;           /* default 64KB */
    size_t      max_samples_per_chunk; /* max samples per audio message */
} asr_client_config_t;

#define ASR_CLIENT_DEFAULT_CONFIG() { \
    .host = CONFIG_MY_UI_CHATBOT_HOST, \
    .port = CONFIG_MY_UI_CHATBOT_PORT, \
    .path = CONFIG_MY_UI_CHATBOT_PATH, \
    .use_tls = CONFIG_MY_UI_CHATBOT_TLS, \
    .cert_pem = NULL, \
    .sample_rate = 16000, \
    .language = "zh", \
    .disable_llm = false, \
    .disable_tts = false, \
    .final_only = false, \
    .rx_buf_size = 64 * 1024, \
    .max_samples_per_chunk = 2048, \
}

typedef enum {
    ASR_EVENT_CONNECTED,
    ASR_EVENT_DISCONNECTED,
    ASR_EVENT_PARTIAL_RESULT,
    ASR_EVENT_FINAL_RESULT,
    ASR_EVENT_TTS_START,
    ASR_EVENT_TTS_COMPLETE,
    ASR_EVENT_ERROR,
} asr_event_type_t;

typedef struct {
    asr_event_type_t type;
    const char      *text;
    const char      *error_msg;
} asr_event_data_t;

typedef void (*asr_event_cb_t)(const asr_event_data_t *event, void *user_ctx);

typedef struct asr_client_s *asr_client_handle_t;

asr_client_handle_t asr_client_init(const asr_client_config_t *config,
                                    asr_event_cb_t cb, void *ctx);
void asr_client_deinit(asr_client_handle_t handle);

esp_err_t asr_client_start(asr_client_handle_t handle);
esp_err_t asr_client_stop(asr_client_handle_t handle);

esp_err_t asr_client_send_audio(asr_client_handle_t handle,
                                const int16_t *pcm16, size_t samples);
esp_err_t asr_client_send_start(asr_client_handle_t handle);
esp_err_t asr_client_send_stop(asr_client_handle_t handle);

bool asr_client_is_connected(asr_client_handle_t handle);

#ifdef __cplusplus
}
#endif

