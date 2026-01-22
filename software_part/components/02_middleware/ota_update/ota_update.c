#include "ota_update.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "net_wifi_sntp.h"
#include "sdkconfig.h"

#include "server_root_cert.h"

static const char *TAG = "OTA_UPDATE";

#ifndef CONFIG_APP_OTA_ENABLE
#define CONFIG_APP_OTA_ENABLE 0
#endif

#ifndef CONFIG_APP_OTA_SKIP_CN_CHECK
#define CONFIG_APP_OTA_SKIP_CN_CHECK 0
#endif

#ifndef CONFIG_APP_OTA_TLS_INSECURE
#define CONFIG_APP_OTA_TLS_INSECURE 0
#endif

#ifndef CONFIG_APP_OTA_HTTP_TIMEOUT_MS
#define CONFIG_APP_OTA_HTTP_TIMEOUT_MS 15000
#endif

#ifndef CONFIG_APP_OTA_HTTP_RX_BUFFER_SIZE
#define CONFIG_APP_OTA_HTTP_RX_BUFFER_SIZE 16384
#endif

#ifndef CONFIG_APP_OTA_LOOP_DELAY_MS
#define CONFIG_APP_OTA_LOOP_DELAY_MS 0
#endif

#ifndef CONFIG_APP_OTA_BULK_FLASH_ERASE
#define CONFIG_APP_OTA_BULK_FLASH_ERASE 0
#endif

#ifndef CONFIG_APP_OTA_BUFFER_CAPS_INTERNAL
#define CONFIG_APP_OTA_BUFFER_CAPS_INTERNAL 0
#endif

/* Optional TLS mutex from my_ui/custom (weak-linked) */
void AppTLS_Lock(void) __attribute__((weak));
void AppTLS_Unlock(void) __attribute__((weak));

static TaskHandle_t     s_task        = NULL;
static TaskHandle_t     s_check_task  = NULL;
static ota_update_cb_t  s_cb          = NULL;
static void            *s_cb_ctx      = NULL;
static volatile int     s_progress    = -1;
static esp_err_t        s_last_error  = ESP_OK;
static char             s_new_version[32] = {0};
static volatile ota_update_check_result_t s_check_result = OTA_UPDATE_CHECK_UNKNOWN;
static esp_err_t        s_check_error  = ESP_OK;
static bool             s_user_cancelled = false;

static void set_check_result(ota_update_check_result_t r, esp_err_t err)
{
    s_check_result = r;
    s_check_error = err;
}

static void notify_evt(ota_update_event_t evt, int payload)
{
    if (s_cb) {
        s_cb(evt, payload, s_cb_ctx);
    }
}

static void set_progress(int p)
{
    if (p < -1) p = -1;
    if (p > 100) p = 100;
    if (p == s_progress) {
        return;
    }
    s_progress = p;
    notify_evt(OTA_UPDATE_EVT_PROGRESS, p);
}

static bool url_is_https(const char *url)
{
    return url && (strncasecmp(url, "https://", 8) == 0);
}

static void ota_task(void *arg)
{
    char *url = (char *)arg;
    s_last_error = ESP_OK;
    s_progress = -1;
    s_new_version[0] = '\0';
    /* 更新任务开始时，检查结果不再可信 */
    set_check_result(OTA_UPDATE_CHECK_UNKNOWN, ESP_OK);
    bool restart_after_success = false;

    ESP_LOGI(TAG, "OTA task start, url=%s", url ? url : "(null)");
    notify_evt(OTA_UPDATE_EVT_START, 0);

    if (!url || !url[0]) {
        s_last_error = ESP_ERR_INVALID_ARG;
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        goto out;
    }

    if (!net_wait_ip(pdMS_TO_TICKS(60 * 1000))) {
        s_last_error = ESP_ERR_TIMEOUT;
        ESP_LOGE(TAG, "Wi-Fi not ready (no IP)");
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        goto out;
    }

    const bool prev_low_latency = net_is_low_latency();
    (void)net_set_low_latency(true);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .timeout_ms = CONFIG_APP_OTA_HTTP_TIMEOUT_MS,
        .keep_alive_enable = true,
        .buffer_size = CONFIG_APP_OTA_HTTP_RX_BUFFER_SIZE,
    };

    /* TLS options */
    if (url_is_https(url)) {
#if CONFIG_APP_OTA_TLS_INSECURE
        http_cfg.cert_pem = NULL;
        http_cfg.use_global_ca_store = false;
        http_cfg.skip_cert_common_name_check = true;
        ESP_LOGW(TAG, "TLS insecure mode enabled (DO NOT USE IN PRODUCTION)");
#else
        http_cfg.cert_pem = server_root_cert_pem;
#if CONFIG_APP_OTA_SKIP_CN_CHECK
        http_cfg.skip_cert_common_name_check = true;
#else
        http_cfg.skip_cert_common_name_check = false;
#endif
#endif
    }

    const TickType_t loop_delay_ticks = pdMS_TO_TICKS(CONFIG_APP_OTA_LOOP_DELAY_MS);
    int last_logged_bucket = -1;
    const int log_step_pct = 10;

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
        .bulk_flash_erase = CONFIG_APP_OTA_BULK_FLASH_ERASE,
    };

#if CONFIG_APP_OTA_BUFFER_CAPS_INTERNAL
    ota_cfg.buffer_caps = MALLOC_CAP_INTERNAL;
#endif

    esp_https_ota_handle_t handle = NULL;
    /* 串行化“TLS 握手阶段”即可：
     * - esp_https_ota_begin 内会创建连接并完成 TLS 握手（最容易与其它任务竞争）
     * - 后续 perform() 的数据拉取/写 flash 不应长期占用互斥锁，否则会阻塞音乐/Chatbot 等正常业务 */
    if (AppTLS_Lock) AppTLS_Lock();
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (AppTLS_Unlock) AppTLS_Unlock();
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_https_ota_begin failed: %s", esp_err_to_name(err));
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        goto tls_out;
    }

    /* Optional version check */
    esp_app_desc_t new_app = {0};
    err = esp_https_ota_get_img_desc(handle, &new_app);
    if (err == ESP_OK) {
        strlcpy(s_new_version, new_app.version, sizeof(s_new_version));
        const esp_app_desc_t *cur = esp_app_get_description();
        const char *cur_ver = cur ? cur->version : "?";
        ESP_LOGI(TAG, "Current ver=%s, New ver=%s", cur_ver, new_app.version);
        if (cur && strncmp(cur->version, new_app.version, sizeof(cur->version)) == 0) {
#if !CONFIG_APP_OTA_ALLOW_SAME_VERSION
            ESP_LOGW(TAG, "Same version detected, skip OTA (enable APP_OTA_ALLOW_SAME_VERSION to force)");
            set_check_result(OTA_UPDATE_CHECK_NO_UPDATE, ESP_OK);
            notify_evt(OTA_UPDATE_EVT_SKIPPED, 0);
            (void)esp_https_ota_abort(handle);
            goto tls_out;
#else
            ESP_LOGW(TAG, "Same version detected, continuing OTA anyway (forced)");
#endif
        }
        set_check_result(OTA_UPDATE_CHECK_HAS_UPDATE, ESP_OK);
    } else {
#if CONFIG_APP_OTA_AUTO_START && !CONFIG_APP_OTA_ALLOW_SAME_VERSION
        /* 自动更新 + 禁止同版本更新时：
         * 如果读不到镜像描述（版本号），就不要盲目下载整包，避免上电被 OTA 长时间占用网络。 */
        ESP_LOGW(TAG, "Auto-start OTA: failed to read new image descriptor (%s), abort to avoid blind download",
                 esp_err_to_name(err));
        s_last_error = err;
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        (void)esp_https_ota_abort(handle);
        goto tls_out;
#endif
    }

    for (;;) {
        err = esp_https_ota_perform(handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            int read = esp_https_ota_get_image_len_read(handle);
            int total = esp_https_ota_get_image_size(handle);
            if (read >= 0 && total > 0) {
                int pct = (int)((int64_t)read * 100 / total);
                set_progress(pct);
                if (log_step_pct > 0 && pct >= 0) {
                    int bucket = pct / log_step_pct;
                    if (bucket != last_logged_bucket) {
                        last_logged_bucket = bucket;
                        ESP_LOGI(TAG, "OTA progress: %d%% (%d/%d bytes)",
                                 bucket * log_step_pct, read, total);
                    }
                }
            } else {
                set_progress(-1);
            }
            if (loop_delay_ticks > 0) {
                vTaskDelay(loop_delay_ticks);
            } else {
                taskYIELD();
            }
            continue;
        }
        break;
    }

    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_https_ota_perform failed: %s", esp_err_to_name(err));
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        (void)esp_https_ota_abort(handle);
        goto tls_out;
    }

    if (!esp_https_ota_is_complete_data_received(handle)) {
        s_last_error = ESP_FAIL;
        ESP_LOGE(TAG, "OTA image incomplete");
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        (void)esp_https_ota_abort(handle);
        goto tls_out;
    }

    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_https_ota_finish failed: %s", esp_err_to_name(err));
        notify_evt(OTA_UPDATE_EVT_ERROR, (int)s_last_error);
        goto tls_out;
    }

    set_progress(100);
    notify_evt(OTA_UPDATE_EVT_DONE, 0);
    ESP_LOGI(TAG, "OTA progress: 100%%");
    ESP_LOGI(TAG, "OTA success, restarting...");
    restart_after_success = true;

tls_out:
    (void)net_set_low_latency(prev_low_latency);

    if (s_last_error == ESP_OK && restart_after_success) {
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }

out:
    free(url);
    s_task = NULL;
    vTaskDelete(NULL);
}

static void ota_check_task(void *arg)
{
    char *url = (char *)arg;
    s_new_version[0] = '\0';
    s_check_error = ESP_OK;
    set_check_result(OTA_UPDATE_CHECK_UNKNOWN, ESP_OK);

    ESP_LOGI(TAG, "OTA check start, url=%s", url ? url : "(null)");

    if (!url || !url[0]) {
        set_check_result(OTA_UPDATE_CHECK_ERROR, ESP_ERR_INVALID_ARG);
        goto out;
    }

    if (!net_wait_ip(pdMS_TO_TICKS(60 * 1000))) {
        set_check_result(OTA_UPDATE_CHECK_ERROR, ESP_ERR_TIMEOUT);
        ESP_LOGE(TAG, "OTA check: Wi-Fi not ready (no IP)");
        goto out;
    }
    ESP_LOGI(TAG, "OTA check: Network ready, starting HTTPS connection");

    const bool prev_low_latency = net_is_low_latency();
    (void)net_set_low_latency(true);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .timeout_ms = CONFIG_APP_OTA_HTTP_TIMEOUT_MS,
        .keep_alive_enable = true,
        .buffer_size = CONFIG_APP_OTA_HTTP_RX_BUFFER_SIZE,
    };

    if (url_is_https(url)) {
#if CONFIG_APP_OTA_TLS_INSECURE
        http_cfg.cert_pem = NULL;
        http_cfg.use_global_ca_store = false;
        http_cfg.skip_cert_common_name_check = true;
        ESP_LOGW(TAG, "OTA check: TLS insecure mode enabled (DO NOT USE IN PRODUCTION)");
#else
        http_cfg.cert_pem = server_root_cert_pem;
#if CONFIG_APP_OTA_SKIP_CN_CHECK
        http_cfg.skip_cert_common_name_check = true;
#else
        http_cfg.skip_cert_common_name_check = false;
#endif
#endif
    }

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
        .bulk_flash_erase = false,
    };

#if CONFIG_APP_OTA_BUFFER_CAPS_INTERNAL
    ota_cfg.buffer_caps = MALLOC_CAP_INTERNAL;
#endif

    esp_https_ota_handle_t handle = NULL;
    ESP_LOGI(TAG, "OTA check: Attempting TLS connection to %s", url);

    /* 堆诊断：打印 OTA 开始前的内存状态 */
    ESP_LOGW(TAG, "[OTA_DIAG] Before esp_https_ota_begin:");
    ESP_LOGW(TAG, "  DMA free=%lu largest=%lu",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_DMA),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    ESP_LOGW(TAG, "  Internal free=%lu largest=%lu",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGW(TAG, "  PSRAM free=%lu largest=%lu",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    ESP_LOGW(TAG, "  OTA buffer_size config=%d bytes", CONFIG_APP_OTA_HTTP_RX_BUFFER_SIZE);

    if (AppTLS_Lock) AppTLS_Lock();
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (AppTLS_Unlock) AppTLS_Unlock();
    ESP_LOGI(TAG, "OTA check: TLS connection result: %s", esp_err_to_name(err));

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA check: esp_https_ota_begin failed: %s", esp_err_to_name(err));
        set_check_result(OTA_UPDATE_CHECK_ERROR, err);
        goto tls_out;
    }
    ESP_LOGI(TAG, "OTA check: Reading firmware descriptor");

    esp_app_desc_t new_app = {0};
    err = esp_https_ota_get_img_desc(handle, &new_app);
    ESP_LOGI(TAG, "OTA check: Descriptor read result: %s", esp_err_to_name(err));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA check: get_img_desc failed: %s", esp_err_to_name(err));
        set_check_result(OTA_UPDATE_CHECK_ERROR, err);
        (void)esp_https_ota_abort(handle);
        goto tls_out;
    }

    strlcpy(s_new_version, new_app.version, sizeof(s_new_version));

    const esp_app_desc_t *cur = esp_app_get_description();
    const char *cur_ver = cur ? cur->version : "?";
    ESP_LOGI(TAG, "OTA check: Current ver=%s, Remote ver=%s", cur_ver, new_app.version);

    if (cur && strncmp(cur->version, new_app.version, sizeof(cur->version)) == 0) {
        set_check_result(OTA_UPDATE_CHECK_NO_UPDATE, ESP_OK);
    } else {
        set_check_result(OTA_UPDATE_CHECK_HAS_UPDATE, ESP_OK);
    }

    (void)esp_https_ota_abort(handle);

tls_out:
    (void)net_set_low_latency(prev_low_latency);

out:
    free(url);
    s_check_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t ota_update_start(const char *url)
{
#if !CONFIG_APP_OTA_ENABLE
    (void)url;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_task || s_check_task) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!url || !url[0]) {
        return ESP_ERR_INVALID_ARG;
    }

    char *dup = strdup(url);
    if (!dup) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
        ota_task,
        "ota_update",
        8192,
        dup,
        15,
        &s_task,
        1);  /* CPU1: 与网络协议栈分离，让CPU0专注UI渲染 */
    if (ok != pdPASS) {
        free(dup);
        s_task = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
#endif
}

bool ota_update_is_running(void)
{
    return s_task != NULL;
}

esp_err_t ota_update_check_start(const char *url)
{
#if !CONFIG_APP_OTA_ENABLE
    (void)url;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_task || s_check_task) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!url || !url[0]) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Reset cached state for this check */
    s_new_version[0] = '\0';
    s_check_error = ESP_OK;
    set_check_result(OTA_UPDATE_CHECK_UNKNOWN, ESP_OK);

    char *dup = strdup(url);
    if (!dup) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
        ota_check_task,
        "ota_check",
        6144,
        dup,
        10,
        &s_check_task,
        1);
    if (ok != pdPASS) {
        free(dup);
        s_check_task = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
#endif
}

bool ota_update_check_is_running(void)
{
    return s_check_task != NULL;
}

ota_update_check_result_t ota_update_get_check_result(void)
{
    return s_check_result;
}

esp_err_t ota_update_get_check_error(void)
{
    return s_check_error;
}

void ota_update_set_callback(ota_update_cb_t cb, void *user_ctx)
{
    s_cb = cb;
    s_cb_ctx = user_ctx;
}

int ota_update_get_progress_percent(void)
{
    return s_progress;
}

esp_err_t ota_update_get_last_error(void)
{
    return s_last_error;
}

const char *ota_update_get_new_version(void)
{
    return s_new_version;
}

void ota_mark_user_cancelled(void)
{
    s_user_cancelled = true;
}

bool ota_should_auto_popup(void)
{
    if (s_user_cancelled) return false;
    return s_check_result == OTA_UPDATE_CHECK_HAS_UPDATE;
}
