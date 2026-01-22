// Chatbot WebSocket client: connect to services/Chatbot (Node.js) `/ws`
#include "chatbot_client.h"
#include "sdkconfig.h"

#if CONFIG_MY_UI_CHATBOT_ENABLE

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

#include "lvgl_port.h"  // for lvgl_port_lock()
#include "esp_heap_caps.h"
#include "server_root_cert.h"
#include "esp_netif_types.h"
#include "esp_netif.h"
#include "mbedtls/base64.h"
#include "tts_player.h"
#include "audio_player.h"
#include "startup_ready.h"
#include "ota_update.h"
#include "custom.h"      // AppTLS_Lock / AppTLS_Unlock
#include "audio_thread.h"
#include "chat_ui.h"

static const char *TAG = "CHATBOT";

// ===== 可配置项（Kconfig 提供，见同目录 Kconfig.projbuild） =====
#ifndef CONFIG_MY_UI_CHATBOT_HOST
#define CONFIG_MY_UI_CHATBOT_HOST "192.168.1.2"
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PORT
#define CONFIG_MY_UI_CHATBOT_PORT 3333
#endif
#ifndef CONFIG_MY_UI_CHATBOT_TLS
#define CONFIG_MY_UI_CHATBOT_TLS 0
#endif
#ifndef CONFIG_MY_UI_CHATBOT_PATH
#define CONFIG_MY_UI_CHATBOT_PATH "/ws"
#endif

typedef struct { char *text; } outbound_msg_t; // UTF-8

static esp_websocket_client_handle_t s_client = NULL;
static bool s_ip_ready = false;
static bool s_ws_started = false;
static QueueHandle_t s_txq = NULL;
static TaskHandle_t  s_task = NULL;
static QueueHandle_t s_rxq  = NULL;   // JSON重组后的接收队列
static TaskHandle_t  s_rx_task = NULL; // 解析线程（Core0）
static lv_ui *s_ui = NULL;             // 由 bind_ui 传入

// 组装 ws://host:port/path 或 wss://
static void make_ws_url(char *out, size_t outlen)
{
    const char *scheme = CONFIG_MY_UI_CHATBOT_TLS ? "wss" : "ws";
    // Only used for logging; actual client config will pass host/port/path separately
    snprintf(out, outlen, "%s://%s:%d%s", scheme,
             CONFIG_MY_UI_CHATBOT_HOST, (int)CONFIG_MY_UI_CHATBOT_PORT,
             CONFIG_MY_UI_CHATBOT_PATH);
}

// 若错过了 IP 事件，主动查询当前 STA 是否已拿到 IP
static void refresh_ip_ready(void)
{
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) sta = esp_netif_get_handle_from_ifkey("WIFI_STA");
    if (!sta) return;
    esp_netif_ip_info_t ip = {0};
    /* Keep it accurate across Wi-Fi toggles: clear the flag when IP is lost. */
    s_ip_ready = (esp_netif_get_ip_info(sta, &ip) == ESP_OK && ip.ip.addr != 0);
}

void chatbot_client_bind_ui(lv_ui *ui)
{
    s_ui = ui;
}

// 前置声明，避免在启用 -Werror 下出现隐式声明告警
static bool ensure_client_started(void);
static void on_ip_ready(void *arg, esp_event_base_t base, int32_t id, void *data);

void chatbot_client_start(void)
{
    if (!s_txq) {
        s_txq = xQueueCreate(4, sizeof(outbound_msg_t));
        if (!s_txq) {
            ESP_LOGE(TAG, "create tx queue failed");
        }
    }
    // 等待获得 IP 再尝试连接，避免在未拿到 IP 时解析失败
    esp_event_handler_instance_t h = NULL;
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        on_ip_ready, NULL, &h);
    ESP_LOGI(TAG, "ws: registered IP event handler");
    // 若启动时已拿到 IP，主动尝试一次建连（避免错过首次 IP 事件）
    (void)ensure_client_started();

    // 启动解析线程（CPU1: 恢复原始配置）
    if (!s_rxq) s_rxq = xQueueCreate(8, sizeof(char *));
    if (s_rxq && !s_rx_task) {
        // 先声明原型
        void chatbot_rx_task(void *arg);
        // 提升解析线程优先级到 6，避免在大文本/高频分片时落后于回调线程。
        // 使用 audio_thread_create 使任务栈优先放在 PSRAM，减轻内部 RAM 压力。
        esp_err_t rc = audio_thread_create((audio_thread_t *)&s_rx_task,
                                           "chatbot_rx",
                                           chatbot_rx_task,
                                           NULL,
                                           4096,
                                           6,
                                           true,  /* stack in PSRAM when possible */
                                           1);
        if (rc != ESP_OK) {
            s_rx_task = NULL;
            ESP_LOGE(TAG, "create rx task failed");
        }
    }
}

static void handle_server_json(const char *json, int len)
{
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) return;
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (cJSON_IsString(type) && type->valuestring) {
        const char *t = type->valuestring;
        if (strcmp(t, "llm.start") == 0) {
            ESP_LOGI(TAG, "llm.start");
            chat_ui_assistant_start();
        } else if (strcmp(t, "llm.delta") == 0) {
            const cJSON *delta = cJSON_GetObjectItemCaseSensitive(root, "delta");
            if (cJSON_IsString(delta) && delta->valuestring) {
                /* llm.delta can be very frequent; keep it at DEBUG to avoid UART/log overhead
                 * impacting realtime audio (TTS). */
                ESP_LOGD(TAG, "llm.delta len=%u", (unsigned)strlen(delta->valuestring));
                chat_ui_assistant_append(delta->valuestring);
            }
        } else if (strcmp(t, "llm.final") == 0) {
            const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
            if (cJSON_IsString(text) && text->valuestring) {
                ESP_LOGI(TAG, "llm.final len=%u", (unsigned)strlen(text->valuestring));
                chat_ui_assistant_finish(text->valuestring);
            } else {
                chat_ui_assistant_finish(NULL);
            }
            ESP_LOGI(TAG, "llm.final done");
        } else if (strcmp(t, "error") == 0) {
            const cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, "detail");
            if (cJSON_IsString(detail) && detail->valuestring) {
                ESP_LOGE(TAG, "server error: %s", detail->valuestring);
            } else {
                ESP_LOGE(TAG, "server error (no detail)");
            }
            chat_ui_assistant_start();
            chat_ui_assistant_append("[服务器错误] ");
            if (cJSON_IsString(detail) && detail->valuestring) {
                chat_ui_assistant_append(detail->valuestring);
            }
            chat_ui_assistant_finish(NULL);
        } else if (strcmp(t, "session") == 0) {
            // ignore
        } else if (strncmp(t, "tts.", 4) == 0) {
            // 设备端直接播放 CosyVoice 的 PCM16 单声道分片。
            // 约定：GPT 语音与音乐完全互斥，TTS 播放期间不再自动恢复音乐。
            static int s_tts_fs = 0;
            static uint32_t s_tts_chunk_count = 0;    // 仅对前若干分片打印详细日志
            static TickType_t s_tts_last_log = 0;     // 节流打印间隔
            if (strcmp(t, "tts.start") == 0) {
                // lazy: 等 meta 再 begin
                ESP_LOGI(TAG, "tts.start");
            } else if (strcmp(t, "tts.meta") == 0) {
                const cJSON *sr = cJSON_GetObjectItemCaseSensitive(root, "sampleRate");
                int fs = (cJSON_IsNumber(sr) && sr->valueint > 0) ? sr->valueint : 24000;
                s_tts_fs = fs;
                /* 若在非 GPT 场景误收到 TTS 事件，仍保证与音乐互斥：
                 * 一旦发现音乐处于活跃状态，则直接停止音乐管线。*/
                ap_state_t st = ap_get_state();
                if (st == AP_STATE_PLAYING ||
                    st == AP_STATE_PAUSED  ||
                    st == AP_STATE_LOADING) {
                    ESP_LOGI(TAG, "tts.meta: stop music for TTS (state_before=%d)", (int)st);
                    (void)ap_stop();
                }
                bool brc = tts_player_begin(s_tts_fs);
                ESP_LOGI(TAG, "tts.meta: fs=%d begin rc=%d", s_tts_fs, (int)brc);
                s_tts_chunk_count = 0; // 新一轮统计
                s_tts_last_log = xTaskGetTickCount();
            } else if (strcmp(t, "tts.chunk") == 0) {
                const cJSON *b64 = cJSON_GetObjectItemCaseSensitive(root, "chunk");
                if (cJSON_IsString(b64) && b64->valuestring) {
                    const char *s = b64->valuestring;
                    // base64 decode
                    size_t slen = strlen(s);
                    size_t outcap = (slen / 4 + 1) * 3;
                    uint8_t *buf = (uint8_t *)malloc(outcap);
                    if (buf) {
                        size_t olen = 0;
                        if (mbedtls_base64_decode(buf, outcap, &olen, (const unsigned char *)s, slen) == 0 && olen > 0) {
                            bool fed = tts_player_feed(buf, olen); // feed copies internally
                            // 前 4 个分片必打；之后每 ~1s 打一条；若送入失败则立即打
                            TickType_t now = xTaskGetTickCount();
                            bool should_log = (s_tts_chunk_count < 4) || (now - s_tts_last_log) > pdMS_TO_TICKS(1000) || !fed;
                            if (should_log) {
                                ESP_LOGI(TAG, "tts.chunk: %u bytes (feed rc=%d)", (unsigned)olen, (int)fed);
                                s_tts_last_log = now;
                            }
                            s_tts_chunk_count++;
                        }
                        free(buf);
                    }
                }
            } else if (strcmp(t, "tts.complete") == 0 || strcmp(t, "tts.error") == 0) {
                extern void tts_player_end(void);
                ESP_LOGI(TAG, "%s", t);
                tts_player_end();
            }
        } else if (strncmp(t, "asr.", 4) == 0) {
            // 语音识别事件：设备侧 UI 暂不展示
        }
    }
    cJSON_Delete(root);
}

static void websocket_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args; (void)base;
    esp_websocket_event_data_t *e = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "wss connected");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "wss disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        if (e && e->op_code == WS_TRANSPORT_OPCODES_TEXT && e->data_ptr && e->data_len > 0) {
            /* 重组整条 JSON：放入队列交给 Core0 解析线程，避免阻塞回调 */
            static char   *s_rxbuf = NULL;
            static size_t  s_rxcap = 0;
            static size_t  s_expected = 0;
            size_t total = e->payload_len ? e->payload_len : e->data_len;
            size_t off   = e->payload_offset; /* 从 0 开始 */
            if (off == 0) {
                if (s_rxcap < total) {
                    free(s_rxbuf);
                    s_rxbuf = (char *)malloc(total + 1);
                    s_rxcap = s_rxbuf ? (total + 1) : 0;
                }
                s_expected = total;
            }
            if (s_rxbuf && s_rxcap >= (off + e->data_len + 1)) {
                memcpy(s_rxbuf + off, e->data_ptr, e->data_len);
                if (off + e->data_len >= s_expected) {
                    s_rxbuf[s_expected] = '\0';
                    if (s_rxq) {
                        char *copy = (char *)malloc(s_expected + 1);
                        if (copy) {
                            memcpy(copy, s_rxbuf, s_expected + 1);
                            if (xQueueSend(s_rxq, &copy, 0) != pdTRUE) { free(copy); }
                        }
                    }
                    s_expected = 0;
                }
            } else {
                if (s_rxq) {
                    char *copy = (char *)malloc(e->data_len + 1);
                    if (copy) {
                        memcpy(copy, e->data_ptr, e->data_len);
                        copy[e->data_len] = '\0';
                        if (xQueueSend(s_rxq, &copy, 0) != pdTRUE) { free(copy); }
                    }
                }
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        if (e) {
            ESP_LOGE(TAG, "wss error: type=%d tls_esp=%d tls_stack=%d vfy=%d ws_status=%d sock_errno=%d",
                     (int)e->error_handle.error_type,
                     (int)e->error_handle.esp_tls_last_esp_err,
                     (int)e->error_handle.esp_tls_stack_err,
                     (int)e->error_handle.esp_tls_cert_verify_flags,
                     (int)e->error_handle.esp_ws_handshake_status_code,
                     (int)e->error_handle.esp_transport_sock_errno);
        } else {
            ESP_LOGE(TAG, "wss error (no detail)");
        }
        break;
    default: break;
    }
}

static bool ensure_client_started(void)
{
    refresh_ip_ready();
    if (!s_ip_ready) return false; // 未获得 IP，先不建连

    /* OTA 期间优先下载固件：暂停 Chatbot WebSocket，避免占用带宽/握手资源 */
    if (ota_update_is_running()) {
        if (s_client && s_ws_started) {
            ESP_LOGW(TAG, "OTA running -> stop Chatbot websocket to free bandwidth");
            (void)esp_websocket_client_stop(s_client);
            s_ws_started = false;
        }
        return false;
    }

    /* 不再依赖 STARTUP_READY_MUSIC：
     * - 音乐索引可能失败/超时（不应阻塞 Chatbot）
     * - TLS 并发通过 AppTLS_Lock 串行化握手即可 */
    if (s_client && esp_websocket_client_is_connected(s_client)) return true;

    // Prefer explicit host/port/path setup to avoid any URI parsing corner cases
    // 宏兜底：当 sdkconfig 里没有这些选项时，使用默认值
    #ifndef CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS
    #define CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS 10000
    #endif
    #ifndef CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC
    #define CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC 15
    #endif
    #ifndef CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS
    #define CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS 8000
    #endif

    esp_websocket_client_config_t cfg = {
        .host = CONFIG_MY_UI_CHATBOT_HOST,
        .port = CONFIG_MY_UI_CHATBOT_PORT,
        .path = CONFIG_MY_UI_CHATBOT_PATH,
        .buffer_size = 2048,
        .disable_auto_reconnect = false,
        .reconnect_timeout_ms = CONFIG_MY_UI_CHATBOT_RECONNECT_TIMEOUT_MS,
        .subprotocol = "json",
        .network_timeout_ms = CONFIG_MY_UI_CHATBOT_NET_TIMEOUT_MS,
        .ping_interval_sec = CONFIG_MY_UI_CHATBOT_PING_INTERVAL_SEC,
        .keep_alive_enable =
            #ifdef CONFIG_MY_UI_CHATBOT_KEEPALIVE
            true
            #else
            false
            #endif
    };
    if (CONFIG_MY_UI_CHATBOT_TLS) {
        cfg.transport = WEBSOCKET_TRANSPORT_OVER_SSL;
    } else {
        cfg.transport = WEBSOCKET_TRANSPORT_OVER_TCP;
    }
    if (CONFIG_MY_UI_CHATBOT_TLS) {
        // 证书验证策略：优先校验证书链；可选跳过 CN；可选完全不校验（仅开发期）
        #if CONFIG_MY_UI_CHATBOT_TLS_INSECURE
        cfg.cert_pem = NULL; // 不提供 CA，等价于跳过证书链校验（仅开发期）
        cfg.use_global_ca_store = false;
        cfg.skip_cert_common_name_check = true;
        ESP_LOGW(TAG, "TLS INSECURE enabled: skipping all certificate checks (DEV ONLY)");
        #else
        cfg.cert_pem = server_root_cert_pem;
        cfg.skip_cert_common_name_check =
            #if CONFIG_MY_UI_CHATBOT_TLS_SKIP_CN
            true
            #else
            false
            #endif
        ;
        #endif
    }

    if (!s_client) {
        char url[192];
        make_ws_url(url, sizeof(url));
        ESP_LOGI(TAG, "connect %s (TLS=%d)", url, (int)CONFIG_MY_UI_CHATBOT_TLS);

        // 为了兼容某些服务端的 Origin 校验，添加 Origin 头
        // 注意：esp_websocket_client 使用 cfg.headers 时需要常驻内存字符串
        static char s_headers[128];
        snprintf(s_headers, sizeof(s_headers), "Origin: https://%s\r\n", CONFIG_MY_UI_CHATBOT_HOST);
        cfg.headers = s_headers;
        // TCP keep-alive 细节配置
        #ifdef CONFIG_MY_UI_CHATBOT_KEEPALIVE
        cfg.keep_alive_idle = CONFIG_MY_UI_CHATBOT_KEEPALIVE_IDLE;
        cfg.keep_alive_interval = CONFIG_MY_UI_CHATBOT_KEEPALIVE_INTERVAL;
        cfg.keep_alive_count = CONFIG_MY_UI_CHATBOT_KEEPALIVE_COUNT;
        #endif

        s_client = esp_websocket_client_init(&cfg);
        if (!s_client) { ESP_LOGE(TAG, "ws client init failed"); return false; }
        esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, websocket_event, NULL);
    }

    if (!s_ws_started) {
        char url[192];
        make_ws_url(url, sizeof(url));
        ESP_LOGI(TAG, "ws start %s (TLS=%d)", url, (int)CONFIG_MY_UI_CHATBOT_TLS);

        /* 串行化 TLS：避免与音乐索引等同时握手，统一通过 AppTLS 互斥。 */
        AppTLS_Lock();
        esp_err_t err = esp_websocket_client_start(s_client);
        AppTLS_Unlock();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "wss start failed: %s", esp_err_to_name(err));
            return false;
        }
        s_ws_started = true;
        int retry = 30;
        while (retry-- > 0 && !esp_websocket_client_is_connected(s_client)) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        ESP_LOGI(TAG, "wss connected=%d", (int)esp_websocket_client_is_connected(s_client));
    }
    return esp_websocket_client_is_connected(s_client);
}

static bool send_text_now(const char *text)
{
    if (!text || !*text) return false;
    int wait_ip_ms = 3000;
    while (!s_ip_ready && wait_ip_ms > 0) {
        refresh_ip_ready();
        vTaskDelay(pdMS_TO_TICKS(100));
        wait_ip_ms -= 100;
    }
    if (!ensure_client_started()) {
        // 等待最多 3 秒建立连接
        int wait_ms = 3000;
        while (wait_ms > 0 && !esp_websocket_client_is_connected(s_client)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            wait_ms -= 100;
        }
        if (!esp_websocket_client_is_connected(s_client)) {
            ESP_LOGW(TAG, "not connected: ip_ready=%d ws_started=%d", (int)s_ip_ready, (int)s_ws_started);
            return false;
        }
    }
    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        ESP_LOGE(TAG, "json create failed (OOM?)");
        return false;
    }
    cJSON_AddStringToObject(obj, "type", "text");
    cJSON_AddStringToObject(obj, "text", text);
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    if (!json) {
        ESP_LOGE(TAG, "json print failed (OOM?)");
        return false;
    }
    ESP_LOGD(TAG, "ws send len=%d", (int)strlen(json));
    int ok = esp_websocket_client_send_text(s_client, json, strlen(json), 8000);
    if (ok < 0) ESP_LOGW(TAG, "wss send rc=%d", ok);
    free(json);
    return ok >= 0;
}

static void chatbot_tx_task(void *arg)
{
    (void)arg;
    outbound_msg_t msg;
    uint32_t idle_ticks = 0;
    for (;;) {
        if (xQueueReceive(s_txq, &msg, pdMS_TO_TICKS(200)) == pdTRUE) {
            ESP_LOGI(TAG, "tx_task: got text len=%u, ensuring connect", (unsigned)strlen(msg.text));
            if (!send_text_now(msg.text)) {
                ESP_LOGW(TAG, "send failed");
            }
            free(msg.text);
            idle_ticks = 0;
        } else {
            if (++idle_ticks >= 25) { // ~5s 保活/自愈一次
                bool ok = ensure_client_started();
                ESP_LOGD(TAG, "keepalive ensure connect rc=%d", (int)ok);
                idle_ticks = 0;
            }
        }
    }
}

// RX 任务：从 s_rxq 取完整 JSON 做解析（Core0）
void chatbot_rx_task(void *arg)
{
    (void)arg;
    for (;;) {
        char *json = NULL;
        if (xQueueReceive(s_rxq, &json, portMAX_DELAY) == pdTRUE && json) {
            handle_server_json(json, (int)strlen(json));
            free(json);
        }
    }
}


static void on_ip_ready(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;
    if (id == IP_EVENT_STA_GOT_IP) {
        s_ip_ready = true;
        if (ota_update_is_running()) {
            ESP_LOGW(TAG, "ip ready but OTA running -> delay ws connect");
            return;
        }
        ESP_LOGI(TAG, "ip ready -> ensure ws connect");
        (void)ensure_client_started();
    }
}

bool chatbot_send_text(const char *utf8_text)
{
    if (!s_txq) chatbot_client_start();
    if (!s_txq) return false;
    size_t tlen = utf8_text ? strlen(utf8_text) : 0;
    ESP_LOGI(TAG, "send_text queued len=%u", (unsigned)tlen);
    outbound_msg_t m = { .text = strdup(utf8_text ? utf8_text : "") };
    if (!m.text) return false;
    if (xQueueSend(s_txq, &m, 0) != pdTRUE) {
        free(m.text);
        return false;
    }
    // 懒创建发送任务（CPU1: 恢复原始配置）
    if (!s_task) {
        esp_err_t rc = audio_thread_create((audio_thread_t *)&s_task,
                                           "chatbot_ws",
                                           chatbot_tx_task,
                                           NULL,
                                           4096,
                                           6,
                                           true,  /* stack in PSRAM when possible */
                                           1);
        if (rc != ESP_OK) {
            s_task = NULL;
            ESP_LOGE(TAG, "create ws task failed");
        }
    }
    return true;
}

bool chatbot_abort(void)
{
    /* Best-effort: only send abort if WS is already connected, avoid blocking UI. */
    if (!s_client || !esp_websocket_client_is_connected(s_client)) {
        return false;
    }
    static const char *json = "{\"type\":\"abort\"}";
    int ok = esp_websocket_client_send_text(s_client, json, (int)strlen(json), 1000);
    if (ok < 0) {
        ESP_LOGW(TAG, "abort send rc=%d", ok);
    } else {
        ESP_LOGI(TAG, "abort sent");
    }
    return ok >= 0;
}

#else /* !CONFIG_MY_UI_CHATBOT_ENABLE (stubs) */

#include "lvgl.h"
#include "esp_log.h"
static const char *TAG = "CHATBOT";
void chatbot_client_bind_ui(lv_ui *ui) { (void)ui; }
void chatbot_client_start(void) { ESP_LOGI(TAG, "stub: disabled by Kconfig"); }
bool chatbot_send_text(const char *utf8_text) { (void)utf8_text; ESP_LOGW(TAG, "stub: disabled by Kconfig"); return false; }
bool chatbot_abort(void) { ESP_LOGW(TAG, "stub: disabled by Kconfig"); return false; }

#endif /* CONFIG_MY_UI_CHATBOT_ENABLE */
