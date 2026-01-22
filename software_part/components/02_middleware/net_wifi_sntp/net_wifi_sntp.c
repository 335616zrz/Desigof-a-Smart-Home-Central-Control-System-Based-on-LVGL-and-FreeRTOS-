#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "net_wifi_sntp.h"
#include "sdkconfig.h"

static const char *TAG = "net_wifi_sntp";

/* 事件位 */
#define WIFI_GOT_IP_BIT   (1 << 0)
#define WIFI_FAIL_BIT     (1 << 1)
#define SNTP_SYNC_BIT     (1 << 2)

static EventGroupHandle_t s_net_event_group;
static TimerHandle_t      s_reconnect_timer;
static SemaphoreHandle_t  s_wifi_lock        = NULL;
static bool               s_user_disconnected = false;
static uint32_t           s_backoff_attempt   = 0;
static volatile int64_t   s_ignore_disconn_until_us = 0;
static bool               s_using_channel_hint = false; /* STA config channel != 0 from MRU hint */

/* ---- 从 wifi_ui 的 NVS 列表读取 MRU 网络（若存在） ----
 *
 * wifi_ui 保存格式（components/01_application/my_ui/custom/wifi_ui.c）：
 * - namespace: "wifiui"
 * - key "cnt": u8
 * - key "e00": blob(saved_net_t) 作为 MRU（最新使用）
 */
typedef struct {
    char ssid[33];  /* 32 + '\0' */
    char pwd[65];   /* 64 + '\0' */
} wifiui_saved_net_t;

static bool load_wifi_mru_from_nvs(char *ssid_out, size_t ssid_out_sz,
                                  char *pwd_out,  size_t pwd_out_sz)
{
    if (!ssid_out || ssid_out_sz == 0) return false;
    if (!pwd_out  || pwd_out_sz  == 0) return false;

    ssid_out[0] = '\0';
    pwd_out[0]  = '\0';

    nvs_handle_t h;
    esp_err_t e = nvs_open("wifiui", NVS_READONLY, &h);
    if (e != ESP_OK) return false;

    uint8_t cnt = 0;
    e = nvs_get_u8(h, "cnt", &cnt);
    if (e != ESP_OK || cnt == 0) {
        nvs_close(h);
        return false;
    }

    wifiui_saved_net_t mru = {0};
    size_t sz = sizeof(mru);
    e = nvs_get_blob(h, "e00", &mru, &sz);
    nvs_close(h);

    if (e != ESP_OK || sz != sizeof(mru) || mru.ssid[0] == '\0') return false;

    strlcpy(ssid_out, mru.ssid, ssid_out_sz);
    strlcpy(pwd_out,  mru.pwd,  pwd_out_sz);
    return true;
}

static bool load_wifi_mru_channel_hint_from_nvs(uint8_t *channel_out)
{
    if (!channel_out) return false;
    *channel_out = 0;

    nvs_handle_t h;
    esp_err_t e = nvs_open("wifiui", NVS_READONLY, &h);
    if (e != ESP_OK) return false;

    uint8_t ch = 0;
    e = nvs_get_u8(h, "mru_ch", &ch);
    nvs_close(h);
    if (e != ESP_OK) return false;
    if (ch < 1 || ch > 13) return false;
    *channel_out = ch;
    return true;
}

static void clear_wifi_mru_channel_hint_in_nvs(void)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open("wifiui", NVS_READWRITE, &h);
    if (e != ESP_OK) return;
    (void)nvs_erase_key(h, "mru_ch");
    (void)nvs_commit(h);
    nvs_close(h);
}

static bool clear_sta_channel_and_reconnect(void)
{
    wifi_config_t cur = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &cur) != ESP_OK) {
        return false;
    }

    if (cur.sta.channel == 0) {
        return false;
    }

    const uint8_t old_ch = cur.sta.channel;
    cur.sta.channel = 0; /* allow full scan */

    esp_err_t e = esp_wifi_set_config(WIFI_IF_STA, &cur);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "clear channel hint: set_config failed: %s", esp_err_to_name(e));
        return false;
    }

    s_using_channel_hint = false;
    clear_wifi_mru_channel_hint_in_nvs();

    /* 立即重连：避免退避等待，提升“上电自连”体验 */
    ESP_LOGW(TAG, "clear channel hint: ch=%u -> 0, reconnect now", (unsigned)old_ch);
    (void)esp_wifi_connect();
    return true;
}

static const char *wifi_disc_reason_str(wifi_err_reason_t reason)
{
    switch (reason) {
    case WIFI_REASON_UNSPECIFIED:          return "UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE:          return "AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE:           return "AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE:         return "ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY:        return "ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED:           return "NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED:          return "NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE:          return "ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED:     return "ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:  return "DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD: return "DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC: return "BSS_TRANSITION_DISASSOC";
    case WIFI_REASON_IE_INVALID:           return "IE_INVALID";
    case WIFI_REASON_MIC_FAILURE:          return "MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: return "GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:   return "IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID: return "GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID: return "PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID:         return "AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION: return "UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP:   return "INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED:   return "802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED: return "CIPHER_SUITE_REJECTED";
    case WIFI_REASON_BEACON_TIMEOUT:       return "BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND:          return "NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL:            return "AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL:           return "ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:    return "HANDSHAKE_TIMEOUT";
    default:                               return "UNKNOWN";
    }
}

/* ---- SNTP 停止后等待其真正退出，避免“运行中改模式”触发断言 ---- */
static void stop_sntp_blocking(void)
{
    if (!esp_sntp_enabled()) {
        return;
    }

    ESP_LOGI(TAG, "SNTP stop request (task=%s)", pcTaskGetName(NULL));
    esp_sntp_stop();

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(200);
    while (esp_sntp_enabled() && (int)(deadline - xTaskGetTickCount()) > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (esp_sntp_enabled()) {
        ESP_LOGW(TAG, "SNTP stop timeout, skip reconfigure to avoid assert");
    }
}

/* ---- NVS 初始化（必要时擦除重试） ---- */
static inline void nvs_init_or_reinit(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }
}

/* ---- 指数退避计算（带随机抖动） ---- */
static inline uint32_t calc_backoff_ms(void)
{
    const uint32_t base = CONFIG_NET_WIFI_BACKOFF_BASE_MS;
    const uint32_t maxd = CONFIG_NET_WIFI_BACKOFF_MAX_MS;
    uint64_t delay = (uint64_t)base << s_backoff_attempt;     // base*2^attempt
    if (delay > maxd) delay = maxd;
    const uint32_t jitter = (uint32_t)(esp_random() % (base / 2u + 1u));
    return (uint32_t)delay + jitter;
}

/* ---- SNTP 回调：收到时间同步通知 ---- */
static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    xEventGroupSetBits(s_net_event_group, SNTP_SYNC_BIT);
}

/* ---- 启动/刷新 SNTP（Got IP 后调用） ---- */
static void start_or_restart_sntp(void)
{
    const bool running_before = esp_sntp_enabled();
    if (running_before) {
        ESP_LOGW(TAG, "SNTP already running, skip reconfigure (task=%s)", pcTaskGetName(NULL));
        return;
    }

    ESP_LOGI(TAG, "Start SNTP sync (task=%s), servers: %s, %s, %s",
             pcTaskGetName(NULL),
             CONFIG_NET_SNTP_SERVER_1,
             CONFIG_NET_SNTP_SERVER_2,
             CONFIG_NET_SNTP_SERVER_3);
    /* 本地时区 & 同步策略：
       - IMMED 直写更省 CPU；如需平滑可切换为 SMOOTH */
    setenv("TZ", CONFIG_NET_SNTP_TZ, 1);
    tzset();

    stop_sntp_blocking();                                /* 保证下方 setoperatingmode 不在运行时调用 */
    if (esp_sntp_enabled()) return;                      /* 避免触发断言，留待下次重新触发 */

    esp_sntp_set_time_sync_notification_cb(sntp_sync_cb);

#if defined(CONFIG_NET_SNTP_SYNC_INTERVAL_MS) && (CONFIG_NET_SNTP_SYNC_INTERVAL_MS > 0)
    esp_sntp_set_sync_interval(CONFIG_NET_SNTP_SYNC_INTERVAL_MS);
#endif

    esp_sntp_setservername(0, CONFIG_NET_SNTP_SERVER_1);
    esp_sntp_setservername(1, CONFIG_NET_SNTP_SERVER_2);
    esp_sntp_setservername(2, CONFIG_NET_SNTP_SERVER_3);

    esp_sntp_init();
}

/* ---- 定时器回调：执行一次重连（Timer 任务上下文） ---- */
static void reconnect_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (s_user_disconnected) return;
    /* 若已关联到 AP，就不要重复 connect（会刷 "sta is connected..." 警告） */
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        return;
    }
    esp_wifi_connect();  /* 驱动内部仍会做 failure_retry_cnt 的短重试 */
}

/* ---- Wi-Fi/IP 事件处理 ---- */
static void wifi_ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;

    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wi-Fi STA start, begin connect");
            if (!s_user_disconnected) {
                esp_wifi_connect();
            } else {
                ESP_LOGI(TAG, "Wi-Fi STA start, user_disconnected=1, skip auto-connect");
            }
            return;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Wi-Fi connected, waiting for IP");
            /* 关联成功即可取消退避定时器，避免后续重复 connect */
            if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);
            return;
        case WIFI_EVENT_STA_DISCONNECTED: {
            const wifi_event_sta_disconnected_t *e = (const wifi_event_sta_disconnected_t *)data;
            xEventGroupClearBits(s_net_event_group, WIFI_GOT_IP_BIT);

            const int64_t now_us = esp_timer_get_time();
            if (s_ignore_disconn_until_us > 0 && now_us < s_ignore_disconn_until_us) {
                /* 典型场景：UI 触发切换 AP 时我们会先主动断开一次；
                 * 只忽略“离开类”断连原因，避免把 NO_AP_FOUND/AUTH_FAIL 等真实故障也吞掉。 */
                const wifi_err_reason_t r = e ? (wifi_err_reason_t)e->reason : WIFI_REASON_UNSPECIFIED;
                s_ignore_disconn_until_us = 0;
                if (r == WIFI_REASON_AUTH_LEAVE ||
                    r == WIFI_REASON_ASSOC_LEAVE ||
                    r == WIFI_REASON_STA_LEAVING) {
                    xEventGroupClearBits(s_net_event_group, WIFI_FAIL_BIT);
                    ESP_LOGI(TAG, "Wi-Fi disconnected (switching AP), skip backoff reconnect (reason=%d)", (int)r);
                    return;
                }
            }

            xEventGroupSetBits(s_net_event_group, WIFI_FAIL_BIT);
            if (s_user_disconnected) return;

            const wifi_err_reason_t r = e ? (wifi_err_reason_t)e->reason : WIFI_REASON_UNSPECIFIED;

            /* 若我们是带“信道 hint”启动的，并且出现 NO_AP_FOUND/BEACON_TIMEOUT，
             * 很可能是路由器自动换信道导致 hint 失效。此时清掉 channel 限制并立即重连。*/
            if (s_using_channel_hint &&
                (r == WIFI_REASON_NO_AP_FOUND || r == WIFI_REASON_BEACON_TIMEOUT)) {
                if (clear_sta_channel_and_reconnect()) {
                    /* 已经主动重连，不再走指数退避 */
                    return;
                }
            }

            if (s_backoff_attempt < 31) s_backoff_attempt++;
            const uint32_t delay_ms = calc_backoff_ms();
            ESP_LOGW(TAG,
                     "Wi-Fi disconnected, reason=%d(%s), retry in %" PRIu32 " ms (attempt=%" PRIu32 ")",
                     (int)r, wifi_disc_reason_str(r), delay_ms, s_backoff_attempt);

            if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);
            xTimerChangePeriod(s_reconnect_timer, pdMS_TO_TICKS(delay_ms), 0);
            xTimerStart(s_reconnect_timer, 0);
            return;
        }
        default:
            break;
        }
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits  (s_net_event_group, WIFI_GOT_IP_BIT);
        xEventGroupClearBits(s_net_event_group, WIFI_FAIL_BIT);

        s_backoff_attempt = 0;
        if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);

        start_or_restart_sntp();
        return;
    }
}

/* ---- 公有 API ---- */
esp_err_t net_start(void)
{
    nvs_init_or_reinit();

    if (!s_net_event_group) {
        s_net_event_group = xEventGroupCreate();
        if (!s_net_event_group) return ESP_ERR_NO_MEM;
    }

    if (!s_reconnect_timer) {
        s_reconnect_timer = xTimerCreate("wifi_reconnect",
                                         pdMS_TO_TICKS(1000), pdFALSE,
                                         NULL, reconnect_timer_cb);
        if (!s_reconnect_timer) return ESP_ERR_NO_MEM;
    }
    if (!s_wifi_lock) {
        s_wifi_lock = xSemaphoreCreateMutex();
        if (!s_wifi_lock) return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());

    /* 事件环可能已存在，忽略重复创建 */
    const esp_err_t loop_ret = esp_event_loop_create_default();
    if (loop_ret != ESP_OK && loop_ret != ESP_ERR_INVALID_STATE) return loop_ret;

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

    /* 仅 RAM 存储：避免频繁写 NVS */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,  IP_EVENT_STA_GOT_IP, &wifi_ip_event_handler, NULL));

    wifi_config_t s = { 0 };
    char mru_ssid[33] = {0};
    char mru_pwd[65]  = {0};
    const bool has_mru = load_wifi_mru_from_nvs(mru_ssid, sizeof(mru_ssid), mru_pwd, sizeof(mru_pwd));
    if (has_mru) {
        strlcpy((char *)s.sta.ssid, mru_ssid, sizeof(s.sta.ssid));
        strlcpy((char *)s.sta.password, mru_pwd, sizeof(s.sta.password));
        uint8_t mru_ch = 0;
        if (load_wifi_mru_channel_hint_from_nvs(&mru_ch)) {
            s.sta.channel = mru_ch; /* channel hint: accelerates scan/connect on boot */
            s_using_channel_hint = true;
            ESP_LOGI(TAG, "Wi-Fi boot target SSID (MRU): %s (ch hint=%u)", mru_ssid, (unsigned)mru_ch);
        } else {
            s_using_channel_hint = false;
            ESP_LOGI(TAG, "Wi-Fi boot target SSID (MRU): %s", mru_ssid);
        }
    } else {
        strlcpy((char *)s.sta.ssid,     CONFIG_NET_WIFI_SSID,     sizeof(s.sta.ssid));
        strlcpy((char *)s.sta.password, CONFIG_NET_WIFI_PASSWORD, sizeof(s.sta.password));
        s_using_channel_hint = false;
        ESP_LOGI(TAG, "Wi-Fi boot target SSID (Kconfig): %s", CONFIG_NET_WIFI_SSID);
    }

    /* WPA3/PMF 兼容：capable=true, required=false（WPA2 也可用） */
    s.sta.pmf_cfg.capable = true;
    s.sta.pmf_cfg.required = false;

    if (s.sta.password[0]) {
        s.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        s.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    s.sta.failure_retry_cnt    = CONFIG_NET_WIFI_FAILURE_RETRY;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s));

#if defined(CONFIG_NET_WIFI_POWER_SAVE) && CONFIG_NET_WIFI_POWER_SAVE
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
#else
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
#endif

    s_user_disconnected = false;
    s_backoff_attempt   = 0;
    xEventGroupClearBits(s_net_event_group, WIFI_GOT_IP_BIT | WIFI_FAIL_BIT | SNTP_SYNC_BIT);

    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

bool net_wait_ip(TickType_t ticks_to_wait)
{
    const EventBits_t bits = xEventGroupWaitBits(s_net_event_group, WIFI_GOT_IP_BIT, pdFALSE, pdTRUE, ticks_to_wait);
    return (bits & WIFI_GOT_IP_BIT) != 0;
}

bool net_wait_time_synced(TickType_t ticks_to_wait)
{
    EventBits_t bits = xEventGroupWaitBits(s_net_event_group, SNTP_SYNC_BIT, pdFALSE, pdTRUE, ticks_to_wait);
    if (bits & SNTP_SYNC_BIT) return true;

    /* 兜底：检查本地时间是否已有效（避免因回调丢失而卡住） */
    time_t now = 0; time(&now);
    struct tm tm_now = {0};
    localtime_r(&now, &tm_now);
    return (tm_now.tm_year + 1900) > 2016;
}

bool net_is_connected(void)
{
    return (xEventGroupGetBits(s_net_event_group) & WIFI_GOT_IP_BIT) != 0;
}

void net_disconnect(void)
{
    if (s_wifi_lock) xSemaphoreTake(s_wifi_lock, portMAX_DELAY);
    s_user_disconnected = true;
    s_ignore_disconn_until_us = 0;
    if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);
    (void)esp_wifi_disconnect();
    (void)esp_sntp_stop();
    xEventGroupClearBits(s_net_event_group, WIFI_GOT_IP_BIT | SNTP_SYNC_BIT);
    if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
}

esp_err_t net_force_reconnect(void)
{
    if (s_wifi_lock) xSemaphoreTake(s_wifi_lock, portMAX_DELAY);

    if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);
    s_user_disconnected = false;
    s_backoff_attempt   = 0;

    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
        return ESP_OK;
    }

    esp_err_t ret = esp_wifi_connect();
    if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
    return ret;
}

esp_err_t net_connect_to(const char *ssid, const char *password)
{
    if (!ssid || ssid[0] == '\0') return ESP_ERR_INVALID_ARG;
    if (!s_net_event_group) return ESP_ERR_INVALID_STATE;

    if (s_wifi_lock) xSemaphoreTake(s_wifi_lock, portMAX_DELAY);

    /* 终止任何退避重连，改由本次显式连接流程接管 */
    if (xTimerIsTimerActive(s_reconnect_timer)) xTimerStop(s_reconnect_timer, 0);
    s_user_disconnected = false;
    s_backoff_attempt   = 0;

    /* 若当前已连到相同 SSID，则不重复触发 connect（减少噪声） */
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        if (strncmp((const char *)ap.ssid, ssid, sizeof(ap.ssid)) == 0) {
            if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
            return ESP_OK;
        }
    }

    /* 先断开当前连接（若有）。断连事件会在 3s 窗口内被忽略退避调度。 */
    s_ignore_disconn_until_us = esp_timer_get_time() + 3 * 1000 * 1000;
    (void)esp_wifi_disconnect();

    /* 配置目标网络 */
    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    cfg.sta.pmf_cfg.capable  = true;
    cfg.sta.pmf_cfg.required = false;
    if (password && password[0]) {
        strlcpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password));
        cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    cfg.sta.failure_retry_cnt = CONFIG_NET_WIFI_FAILURE_RETRY;
    /* 显式连接属于“用户选择”，不要携带旧的 channel hint */
    s_using_channel_hint = false;

    (void)esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (err != ESP_OK) {
        if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
        return err;
    }

    /* 确保 Wi-Fi 已启动；若此时刚启动，STA_START 事件会触发 connect */
    esp_err_t se = esp_wifi_start();
    if (se != ESP_OK && se != ESP_ERR_INVALID_STATE) {
        if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
        return se;
    }

    if (se == ESP_ERR_INVALID_STATE) {
        err = esp_wifi_connect();
    } else {
        err = ESP_OK;
    }

    if (s_wifi_lock) xSemaphoreGive(s_wifi_lock);
    return err;
}

esp_err_t net_set_low_latency(bool enable)
{
    const wifi_ps_type_t ps = enable ? WIFI_PS_NONE : WIFI_PS_MIN_MODEM;
    return esp_wifi_set_ps(ps);
}

bool net_is_low_latency(void)
{
    wifi_ps_type_t cur = WIFI_PS_NONE;
    if (esp_wifi_get_ps(&cur) != ESP_OK) return false;
    return (cur == WIFI_PS_NONE);
}
