/* components/01_application/my_ui/custom/wifi_ui.c */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "wifi_ui.h"
#include "time_ui.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "ota_update.h"
#include "ota_upgrade_dialog.h"
#include "weather_service_client.h"
#include "weather_persist.h"
#include "weather_city.h"
#include "startup_ready.h"
#include "net_wifi_sntp.h"
#include "lvgl_port.h"

/* 兼容：如果没有内置符号宏，就用安全的占位文本，不依赖字体 */
#ifndef LV_SYMBOL_WIFI
#define LV_SYMBOL_WIFI ""
#endif
#ifndef LV_SYMBOL_KEY
#define LV_SYMBOL_KEY "[lock]"
#endif

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h" /* xTaskCreate*WithCaps / vTaskDeleteWithCaps */

static const char *TAG = "wifi_ui";

/* =================== 保存网络（NVS） =================== */

typedef struct {
    char ssid[33];  /* 32 + '\0' */
    char pwd[65];   /* 64 + '\0' */
} saved_net_t;

#define SAVED_MAX   16
#define NVS_NS      "wifiui"
#define NVS_KEY_CNT "cnt"
/* 仅保存 MRU 连接 hint（用于上电加速连接），不改变已保存列表的 blob 格式 */
#define NVS_KEY_MRU_CH    "mru_ch"    /* u8: 1~13 */
#define NVS_KEY_MRU_BSSID "mru_bssid" /* blob: 6 bytes */

static saved_net_t *s_saved = NULL;
static uint8_t      s_saved_count = 0;
static SemaphoreHandle_t s_saved_mtx;

/* 载入全部保存网络 */
static void saved_load_from_nvs(void)
{
    if (!s_saved_mtx) s_saved_mtx = xSemaphoreCreateMutex();
    xSemaphoreTake(s_saved_mtx, portMAX_DELAY);

    free(s_saved);
    s_saved = NULL;
    s_saved_count = 0;

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t cnt = 0;
        (void)nvs_get_u8(h, NVS_KEY_CNT, &cnt);
        if (cnt > SAVED_MAX) cnt = SAVED_MAX;

        s_saved = (saved_net_t *)calloc(cnt ? cnt : 1, sizeof(saved_net_t));
        if (s_saved) {
            for (uint8_t i = 0; i < cnt; ++i) {
                char key[8];
                snprintf(key, sizeof(key), "e%02u", (unsigned)i);
                size_t sz = sizeof(saved_net_t);
                saved_net_t tmp = {0};
                if (nvs_get_blob(h, key, &tmp, &sz) == ESP_OK && sz == sizeof(saved_net_t)) {
                    s_saved[s_saved_count++] = tmp;
                }
            }
        }
        nvs_close(h);
    }

    xSemaphoreGive(s_saved_mtx);
}

/* 将当前内存的保存列表写回 NVS */
static void saved_store_to_nvs(void)
{
    if (!s_saved_mtx) s_saved_mtx = xSemaphoreCreateMutex();
    xSemaphoreTake(s_saved_mtx, portMAX_DELAY);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        esp_err_t e = nvs_set_u8(h, NVS_KEY_CNT, s_saved_count);
        if (e != ESP_OK) ESP_LOGW(TAG, "nvs_set_u8(cnt) err %s", esp_err_to_name(e));
        /* 写入 e00..eNN；把多余旧键清理掉 */
        for (uint8_t i = 0; i < s_saved_count; ++i) {
            char key[8];
            snprintf(key, sizeof(key), "e%02u", (unsigned)i);
            e = nvs_set_blob(h, key, &s_saved[i], sizeof(saved_net_t));
            if (e != ESP_OK) ESP_LOGW(TAG, "nvs_set_blob(%s) err %s", key, esp_err_to_name(e));
        }
        for (uint8_t i = s_saved_count; i < SAVED_MAX; ++i) {
            char key[8];
            snprintf(key, sizeof(key), "e%02u", (unsigned)i);
            nvs_erase_key(h, key); /* 忽略返回值 */
        }
        e = nvs_commit(h);
        if (e != ESP_OK) ESP_LOGW(TAG, "nvs_commit err %s", esp_err_to_name(e));
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "nvs_open(RW) err %s", esp_err_to_name(err));
    }

    xSemaphoreGive(s_saved_mtx);
}

/* 按 SSID 查找下标，不存在返回 -1 */
static int saved_find_by_ssid(const char *ssid)
{
    if (!ssid || !ssid[0]) return -1;
    for (uint8_t i = 0; i < s_saved_count; ++i) {
        if (strcmp(s_saved[i].ssid, ssid) == 0) return (int)i;
    }
    return -1;
}

/* 插入/更新为 MRU（放到 0），必要时淘汰最后一项 */
static void saved_add_or_update(const char *ssid, const char *pwd)
{
    if (!ssid || !ssid[0]) return;

    if (!s_saved) {
        s_saved = (saved_net_t *)calloc(SAVED_MAX, sizeof(saved_net_t));
        if (!s_saved) return;
    }

    /* 先找是否存在 */
    int idx = saved_find_by_ssid(ssid);
    saved_net_t entry = {0};
    strlcpy(entry.ssid, ssid, sizeof(entry.ssid));
    if (pwd) strlcpy(entry.pwd,  pwd,  sizeof(entry.pwd));

    if (idx >= 0) {
        /* 移到最前（MRU） */
        for (int i = idx; i > 0; --i) s_saved[i] = s_saved[i-1];
        s_saved[0] = entry;
    } else {
        /* 插入到 0，右移，其余可能被淘汰 */
        if (s_saved_count < SAVED_MAX) ++s_saved_count;
        for (int i = s_saved_count - 1; i > 0; --i) s_saved[i] = s_saved[i-1];
        s_saved[0] = entry;
    }
    saved_store_to_nvs();
}

/* =================== UI & 扫描 =================== */

static lv_ui *s_ui = NULL;

/* 扫描结果缓存 */
static wifi_ap_record_t *s_aps = NULL;
static uint16_t s_ap_num = 0;
static int s_selected = -1;

static SemaphoreHandle_t s_scan_mtx;
static bool s_wifi_on = false; /* 记录期望Wi‑Fi开关状态，跨界面复用 */
static bool s_scanning = false; /* 扫描节流：一次只允许一个扫描进行 */
static bool s_handlers_registered = false; /* 事件回调是否已注册 */

/* “已保存网络”列表态 */
static bool s_listing_saved = false;
static int  s_selected_saved = -1;
/* 从“已保存”项进入连接对话使用的暂存 */
static char s_pending_ssid[33];
static char s_pending_pwd[65];
static bool s_from_saved = false;

/* ---------- 小工具：异步更新 LVGL ---------- */
typedef struct { char text[96]; } text_msg_t;

static void _lv_set_label_async_cb(void *p)
{
    text_msg_t *m = (text_msg_t *)p;
    if (s_ui && s_ui->wifi_main_lbl_status) {
        lv_label_set_text(s_ui->wifi_main_lbl_status, m->text);
    }
    free(m);
}

static void ui_set_status_async(const char *fmt, ...)
{
    text_msg_t *m = (text_msg_t *)calloc(1, sizeof(text_msg_t));
    if (!m) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(m->text, sizeof(m->text), fmt, ap);
    va_end(ap);
    if (!lvgl_port_lock(50)) {
        free(m);
        return;
    }
    lv_result_t res = lv_async_call(_lv_set_label_async_cb, m);
    lvgl_port_unlock();
    if (res != LV_RESULT_OK) {
        free(m);
    }
}

/* （按你的要求：界面不改变。保留空位，后续需要时再启用） */

/* ---------- 列表生成与点击（扫描） ---------- */

static void ap_item_clicked_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= s_ap_num) return;

    s_listing_saved = false;
    s_selected_saved = -1;

    s_selected = (int)idx;
    wifi_ap_record_t *ap = &s_aps[idx];

    /* 设置弹窗标题与密码框显示 */
    char title[64];
    snprintf(title, sizeof(title), "Connect to %s", (const char*)ap->ssid);
    lv_label_set_text(s_ui->wifi_main_pwd_title, title);

    bool open = (ap->authmode == WIFI_AUTH_OPEN);
    if (open) {
        lv_obj_add_flag(s_ui->wifi_main_pwd_ta, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(s_ui->wifi_main_pwd_ta, "");
    } else {
        lv_obj_clear_flag(s_ui->wifi_main_pwd_ta, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(s_ui->wifi_main_pwd_ta, "");
    }

    /* 显示遮罩与对话框，并把焦点给到输入框以触发键盘 */
    if (s_ui->wifi_main_overlay) lv_obj_clear_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_ui->wifi_main_dlg_pwd) lv_obj_clear_flag(s_ui->wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
    /* 按需弹键盘：仅当用户点击输入框时，键盘事件回调才会触发 */
}

/* 将扫描结果填到列表（在 LVGL 线程里调用） */
static void _lv_fill_list_cb(void *unused)
{
    LV_UNUSED(unused);
    if (!s_ui || !s_ui->wifi_main_ap_list) return;

    s_listing_saved = false;
    s_selected_saved = -1;

    /* 清空旧内容 */
    lv_obj_clean(s_ui->wifi_main_ap_list);

    for (uint16_t i = 0; i < s_ap_num; ++i) {
        const wifi_ap_record_t *ap = &s_aps[i];

        char line[96];
        /* 带个“锁”提示是否加密、附带 RSSI/信道 */
        const char *lock = (ap->authmode == WIFI_AUTH_OPEN) ? "" : LV_SYMBOL_KEY " ";
        snprintf(line, sizeof(line), "%s%s  ch%u  %ddBm",
                 lock, (const char*)ap->ssid, ap->primary, ap->rssi);

        lv_obj_t *btn = lv_list_add_button(s_ui->wifi_main_ap_list, LV_SYMBOL_WIFI, line);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, ap_item_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
}

/* ---------- 列表生成与点击（已保存） ---------- */

static void saved_item_clicked_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    intptr_t idx = (intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= s_saved_count) return;

    s_selected = -1;              /* 不使用扫描项 */
    s_listing_saved = true;
    s_selected_saved = (int)idx;
    s_from_saved = true;

    /* 准备弹窗 */
    const saved_net_t *sn = &s_saved[idx];
    strlcpy(s_pending_ssid, sn->ssid, sizeof(s_pending_ssid));
    strlcpy(s_pending_pwd,  sn->pwd,  sizeof(s_pending_pwd));

    char title[64];
    snprintf(title, sizeof(title), "Connect to %s", s_pending_ssid);
    lv_label_set_text(s_ui->wifi_main_pwd_title, title);

    lv_obj_clear_flag(s_ui->wifi_main_pwd_ta, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(s_ui->wifi_main_pwd_ta, s_pending_pwd);

    if (s_ui->wifi_main_overlay) lv_obj_clear_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_ui->wifi_main_dlg_pwd) lv_obj_clear_flag(s_ui->wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
    /* 按需弹键盘：仅当用户点击输入框时，键盘事件回调才会触发 */
}

/* 将已保存网络填到列表 */
static void _lv_fill_saved_list_cb(void *unused)
{
    LV_UNUSED(unused);
    if (!s_ui || !s_ui->wifi_main_ap_list) return;

    /* 清空旧内容 */
    lv_obj_clean(s_ui->wifi_main_ap_list);
    s_listing_saved = true;
    s_selected_saved = -1;

    for (uint16_t i = 0; i < s_saved_count; ++i) {
        const saved_net_t *sn = &s_saved[i];

        char line[96];
        snprintf(line, sizeof(line), "%s  (saved)", sn->ssid);

        lv_obj_t *btn = lv_list_add_button(s_ui->wifi_main_ap_list, LV_SYMBOL_WIFI, line);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, saved_item_clicked_cb, LV_EVENT_CLICKED, NULL);
    }

    ui_set_status_async("Saved networks: %u", (unsigned)s_saved_count);
}

/* “Saved networks” 按钮回调 */
static void btn_saved_clicked_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    /* 重新从 NVS 拉一遍，确保和其他逻辑写入保持一致 */
    saved_load_from_nvs();
    if (!lvgl_port_lock(50)) {
        return;
    }
    (void)lv_async_call(_lv_fill_saved_list_cb, NULL);
    lvgl_port_unlock();
}

/* ---------- Wi-Fi 驱动事件 ---------- */

static void scan_done_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    uint16_t num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&num));

    xSemaphoreTake(s_scan_mtx, portMAX_DELAY);
    free(s_aps);
    s_ap_num = num;
    s_aps = (wifi_ap_record_t *)calloc(s_ap_num ? s_ap_num : 1, sizeof(wifi_ap_record_t));
    if (s_aps && s_ap_num) {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&s_ap_num, s_aps));
    } else {
        s_ap_num = 0;
    }
    xSemaphoreGive(s_scan_mtx);

    s_scanning = false;
    ui_set_status_async("Found %u APs", (unsigned) s_ap_num);
    if (!lvgl_port_lock(50)) {
        return;
    }
    (void)lv_async_call(_lv_fill_list_cb, NULL);
    lvgl_port_unlock();
}

static void sta_conn_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    ui_set_status_async("Connecting…");
}

static void sta_disconn_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    /* 与视觉文案保持一致：未连接时显示 Not connected */
    ui_set_status_async("Not connected");
}

#if CONFIG_APP_OTA_ENABLE
#endif

static void got_ip_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *e = (const ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&e->ip_info.ip), IP2STR(&e->ip_info.netmask), IP2STR(&e->ip_info.gw));

        ui_set_status_async("Connected");

        /* 启动阶段：Wi‑Fi + IP 就绪 */
        startup_ready_set(STARTUP_READY_WIFI);

        /* 记录 MRU 的信道 hint（可用于上电加速连接，尤其是 .local 业务链路依赖 Wi‑Fi 就绪） */
        wifi_ap_record_t ap = {0};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            nvs_handle_t h;
            if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
                if (ap.primary >= 1 && ap.primary <= 13) {
                    (void)nvs_set_u8(h, NVS_KEY_MRU_CH, ap.primary);
                }
                (void)nvs_set_blob(h, NVS_KEY_MRU_BSSID, ap.bssid, 6);
                (void)nvs_commit(h);
                nvs_close(h);
            }
            ESP_LOGI(TAG, "MRU hint saved: ch=%u", (unsigned)ap.primary);
        }

        static bool sntp_done = false;
        if (!sntp_done) {
            time_ui_start_sntp(NULL);   // 传 NULL => 使用 "CST-8" 作为默认时区
            sntp_done = true;
        }

        /* 一并触发天气拉取，更新首页四个字段（位置/现象/温度数值/湿度数值） */
        weather_bind_ui(&guider_ui);
        uint32_t prov=0, pref=0, county=0;
        (void)weather_persist_load(&prov, &pref, &county);
        uint32_t adcode = county ? county : (pref ? pref : prov);
        if (!adcode) {
            /* 若未选择过，则用城市选择模块当前默认（若未初始化也无所谓，adcode=0 则跳过） */
            adcode = weather_city_get_selected_county_code();
            if (!adcode) adcode = weather_city_get_selected_pref_code();
            if (!adcode) adcode = weather_city_get_selected_province_code();
        }
        if (adcode) {
            weather_set_last_adcode(adcode);
            (void)weather_update_for_adcode(adcode);
            (void)weather_periodic_start(5 * 60 * 1000);   // 每 5 分钟自动刷新
            // 开机短重试：处理 .local 初期不可解析的场景（2s→5s→10s→20s）
            weather_short_retry_bootstrap();
        }

        /* OTA 自动检查已移至 app_main.c 的 on_got_ip 回调中，延迟 10 秒启动以避免 TLS 冲突 */
    }
}

/* ---------- 根据当前连接状态刷新状态文案 ---------- */
static void refresh_status_label_from_runtime(void)
{
    if (!s_ui || !s_ui->wifi_main_lbl_status) return;
    if (!s_wifi_on) {
        lv_label_set_text(s_ui->wifi_main_lbl_status, "Wi-Fi OFF");
        return;
    }
    wifi_ap_record_t ap;
    esp_err_t ok = esp_wifi_sta_get_ap_info(&ap);
    if (ok == ESP_OK) {
        lv_label_set_text(s_ui->wifi_main_lbl_status, "Connected");
    } else {
        lv_label_set_text(s_ui->wifi_main_lbl_status, "Not connected");
    }
}

/* ---------- 对外/内部连接流程 ---------- */

static void wifi_connect_to(const char *ssid, const char *pwd)
{
    if (!ssid || !ssid[0]) return;

    esp_err_t err = net_connect_to(ssid, pwd);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "net_connect_to failed: %s", esp_err_to_name(err));
        ui_set_status_async("Wi-Fi connect err: %s", esp_err_to_name(err));
        return;
    }

    ui_set_status_async("Connecting to %s…", ssid);

    /* 写入保存列表（MRU），空密码也会保存（开放网络） */
    saved_add_or_update(ssid, pwd ? pwd : "");

    /* 关闭弹窗 */
    if (s_ui && s_ui->wifi_main_overlay)
        lv_obj_add_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
}

/* 后台任务：执行 Wi-Fi 停止操作，避免阻塞 LVGL 线程 */
static void wifi_stop_task(void *arg)
{
    (void)arg;
    /* 停止自动重连，再停止 Wi-Fi */
    net_disconnect();
    (void)esp_wifi_stop();

    /* 在 LVGL 线程中更新 UI */
    if (lvgl_port_lock(100)) {
        ui_set_status_async("Wi-Fi OFF");

        /* 关闭时清空列表与扫描缓存，避免残留项 */
        if (s_ui && s_ui->wifi_main_ap_list) {
            lv_obj_clean(s_ui->wifi_main_ap_list);
        }
        if (s_aps) {
            xSemaphoreTake(s_scan_mtx, portMAX_DELAY);
            free(s_aps);
            s_aps = NULL;
            s_ap_num = 0;
            xSemaphoreGive(s_scan_mtx);
        }
        s_selected = -1;
        s_listing_saved = false;
        s_selected_saved = -1;
        s_from_saved = false;
        s_pending_ssid[0] = 0;
        s_pending_pwd[0] = 0;

        /* 收起可能尚未关闭的遮罩/对话框与清空临时消息 */
        if (s_ui) {
            if (s_ui->wifi_main_dlg_pwd) lv_obj_add_flag(s_ui->wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
            if (s_ui->wifi_main_overlay) lv_obj_add_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
            if (s_ui->wifi_main_msg_label) lv_label_set_text(s_ui->wifi_main_msg_label, "");
        }
        lvgl_port_unlock();
    }
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
    vTaskDeleteWithCaps(NULL);
#else
    vTaskDelete(NULL);
#endif
}

void wifi_ui_toggle(bool on)
{
    if (on == s_wifi_on) {
        /* 状态未变化：不重复 start/stop，避免切屏回来误触发断连 */
        return;
    }
    s_wifi_on = on;

    if (on) {
        /* 由 net_wifi_sntp 统一管理连接；这里仅恢复联网并触发一次连接 */
        /* 先恢复自动重连/清除 user_disconnected；若 Wi-Fi 尚未 start，connect 可能返回 NOT_STARTED，后续 start 会触发连接 */
        (void)net_force_reconnect();

        esp_err_t se = esp_wifi_start();
        if (se != ESP_OK && se != ESP_ERR_INVALID_STATE) {
            ui_set_status_async("Wi-Fi start err %d", (int)se);
            return;
        }

        ui_set_status_async("Wi-Fi ON");
    } else {
        /* 在后台任务中执行 Wi-Fi 停止操作，避免阻塞 LVGL 线程导致 UI 卡死 */
        ui_set_status_async("Stopping Wi-Fi...");
        BaseType_t ret = pdFAIL;
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
        ret = xTaskCreatePinnedToCoreWithCaps(
            wifi_stop_task, "wifi_stop",
            3072, NULL, 5, NULL, 1,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
        ret = xTaskCreatePinnedToCore(wifi_stop_task, "wifi_stop", 3072, NULL, 5, NULL, 1);
#endif
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create wifi_stop task");
            ui_set_status_async("Wi-Fi stop failed");
        } else {
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
            ESP_LOGI(TAG, "wifi_stop: PSRAM stack (3KB)");
#endif
        }
    }
}

void wifi_ui_scan(void)
{
    /* 确保 STA 模式已启动（已启动返回 INVALID_STATE，当成功处理） */
    (void)esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t se = esp_wifi_start();
    if (se != ESP_OK && se != ESP_ERR_INVALID_STATE) {
        ui_set_status_async("Wi-Fi start err %d", (int)se);
        return;
    }

    wifi_scan_config_t cfg = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,              /* 全信道 */
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    s_listing_saved = false;
    s_selected_saved = -1;

    if (s_scanning) {
        ui_set_status_async("Scanning…");
        return;
    }
    s_scanning = true;
    ui_set_status_async("Scanning…");
    esp_err_t e = esp_wifi_scan_start(&cfg, false);   /* 异步，等 WIFI_EVENT_SCAN_DONE */
    if (e != ESP_OK) {
        s_scanning = false;
        ui_set_status_async("Scan err %d", (int)e);
    }
}

void wifi_ui_back(void)
{
    if (s_ui && s_ui->wifi_main_overlay)
        lv_obj_add_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
}

/* 反注册事件与清理缓存（按需调用；当前未被自动调用） */
void wifi_ui_deinit(void)
{
    /* 反注册 ESP 事件回调 */
    if (s_handlers_registered) {
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,        scan_done_handler);
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,    sta_conn_handler);
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, sta_disconn_handler);
        esp_event_handler_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP,         got_ip_handler);
        s_handlers_registered = false;
    }

    /* 清理扫描缓存 */
    if (s_scan_mtx) xSemaphoreTake(s_scan_mtx, portMAX_DELAY);
    free(s_aps); s_aps = NULL; s_ap_num = 0; s_selected = -1;
    if (s_scan_mtx) xSemaphoreGive(s_scan_mtx);
    s_scanning = false;

    /* 可选：释放保存网络内存（下次 init 会再装载） */
    if (s_saved_mtx) xSemaphoreTake(s_saved_mtx, portMAX_DELAY);
    free(s_saved); s_saved = NULL; s_saved_count = 0;
    if (s_saved_mtx) xSemaphoreGive(s_saved_mtx);

    /* 关闭遮罩/对话框 */
    if (s_ui) {
        if (s_ui->wifi_main_dlg_pwd) lv_obj_add_flag(s_ui->wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        if (s_ui->wifi_main_overlay) lv_obj_add_flag(s_ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 供“连接”按钮调用：兼容扫描项/已保存项两种来源 */
void wifi_ui_connect_from_dialog(void)
{
    const char *pwd_typed = lv_textarea_get_text(s_ui->wifi_main_pwd_ta);

    if (s_from_saved && s_pending_ssid[0]) {
        wifi_connect_to(s_pending_ssid, pwd_typed ? pwd_typed : "");
        s_from_saved = false;
        s_pending_ssid[0] = 0;
        s_pending_pwd[0] = 0;
        return;
    }

    if (s_selected < 0 || s_selected >= s_ap_num) return;

    wifi_ap_record_t *ap = &s_aps[s_selected];
    const char *pwd = NULL;
    if (ap->authmode != WIFI_AUTH_OPEN) pwd = pwd_typed ? pwd_typed : "";

    wifi_connect_to((const char *)ap->ssid, pwd);
}

/* ---------- NVS 兜底 ---------- */
static esp_err_t ensure_nvs_ready(void)
{
    static bool done = false;
    if (done) return ESP_OK;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* 分区满了或版本升级，按 IDF 推荐流程擦除后重试 */
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret == ESP_OK) done = true;
    return ret;
}

/* 可选：从别处直接显示“已保存网络” */
void wifi_ui_show_saved(void)
{
    saved_load_from_nvs();
    if (!lvgl_port_lock(50)) {
        return;
    }
    (void)lv_async_call(_lv_fill_saved_list_cb, NULL);
    lvgl_port_unlock();
}

bool wifi_ui_is_connected(void)
{
    // 优先用 netif 是否有 IP，若失败再用驱动查询
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta) {
        esp_netif_ip_info_t ipi;
        if (esp_netif_get_ip_info(sta, &ipi) == ESP_OK && ipi.ip.addr != 0) {
            return true;
        }
    }
    wifi_ap_record_t ap;
    return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}

bool wifi_ui_connect_mru_saved_once(uint32_t wait_ms)
{
    /* 若已经连上（有 IP 或已关联到 AP），不要在启动阶段反复改配置/断开，避免扰动连接 */
    if (wifi_ui_is_connected()) {
        ESP_LOGI(TAG, "MRU connect skipped: already connected");
        return true;
    }

    // 刷一遍保存列表，读取 MRU（下标 0）
    saved_load_from_nvs();
    if (s_saved_count == 0) {
        ESP_LOGW(TAG, "MRU connect: no saved networks (NVS empty)");
        return false;
    }

    const char *ssid = s_saved[0].ssid;
    const char *pwd  = s_saved[0].pwd;
    if (!ssid || !ssid[0]) {
        ESP_LOGW(TAG, "MRU connect: invalid saved SSID");
        return false;
    }

    ESP_LOGI(TAG, "MRU connect: ssid=\"%s\" (wait=%u ms)", ssid, (unsigned)wait_ms);

    /* 如果 net_wifi_sntp 已经在启动阶段把 STA 配置为相同 SSID，则不要再走
     * net_connect_to() 的“断开->改 config->重连”，避免把驱动从 connecting 状态硬拉回去。
     * 这能显著降低上电阶段“长时间找不到 AP”的概率。 */
    wifi_config_t cur = {0};
    esp_err_t ge = esp_wifi_get_config(WIFI_IF_STA, &cur);
    if (ge == ESP_OK && strncmp((const char *)cur.sta.ssid, ssid, sizeof(cur.sta.ssid)) == 0) {
        ESP_LOGI(TAG, "MRU connect: STA already configured to \"%s\", kick reconnect only", ssid);
        (void)net_force_reconnect();
    } else {
        /* 连接一次；wifi_connect_to 会确保 STA 启动、设置 config 并调用 esp_wifi_connect */
        wifi_connect_to(ssid, pwd);
    }

    // 等待拿到 IP（最多 wait_ms）
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(wait_ms ? wait_ms : 8000);
    while (xTaskGetTickCount() < deadline) {
        if (wifi_ui_is_connected()) return true;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return wifi_ui_is_connected();
}

/* ---------- 初始化：注册事件与回调 ---------- */
void wifi_ui_init(lv_ui *ui)
{
    s_ui = ui;
    if (!s_scan_mtx)  s_scan_mtx  = xSemaphoreCreateMutex();
    if (!s_saved_mtx) s_saved_mtx = xSemaphoreCreateMutex();

    /* NVS 就绪（供保存网络使用） */
    ESP_ERROR_CHECK(ensure_nvs_ready());
    saved_load_from_nvs();

    /* —— 幂等拉起网络栈（已初始化则返回 ESP_ERR_INVALID_STATE，视为 OK）—— */
    esp_err_t e;

    // 1) netif 基础
    e = esp_netif_init();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(e);

    // 2) 默认事件循环
    e = esp_event_loop_create_default();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(e);

    // 3) 确保默认的 STA netif 存在
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) {
        sta = esp_netif_create_default_wifi_sta();
        if (sta == NULL) {
            ESP_LOGE(TAG, "create_default_wifi_sta failed");
            abort();
        }
    }

    // 4) Wi-Fi 驱动初始化
    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&wcfg);
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(e);

    /* 事件回调（只注册一次） */
    if (!s_handlers_registered) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,        scan_done_handler,   NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,    sta_conn_handler,    NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, sta_disconn_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP,         got_ip_handler,      NULL));
        s_handlers_registered = true;
    }

    /* 绑定“Saved networks”按钮 */
    if (s_ui && s_ui->wifi_main_btn_saved) {
        lv_obj_add_event_cb(s_ui->wifi_main_btn_saved, btn_saved_clicked_cb, LV_EVENT_CLICKED, NULL);
    }

    /* 初始清空列表 */
    if (s_ui && s_ui->wifi_main_ap_list) lv_obj_clean(s_ui->wifi_main_ap_list);

    /* 同步 UI 开关到当前期望状态（不触发驱动），再派发一次事件让生成器逻辑刷新可用态/文案 */
    if (s_ui->wifi_main_sw_1 && lv_obj_is_valid(s_ui->wifi_main_sw_1)) {
        if (s_wifi_on) lv_obj_add_state(s_ui->wifi_main_sw_1, LV_STATE_CHECKED);
        else           lv_obj_clear_state(s_ui->wifi_main_sw_1, LV_STATE_CHECKED);
        lv_obj_send_event(s_ui->wifi_main_sw_1, LV_EVENT_VALUE_CHANGED, NULL);
    }

    /* 刷新一次状态文案，避免切屏回到页面后仍显示 "Not connected" */
    refresh_status_label_from_runtime();

    /* 如果此时已经拿到了 IP（例如 UI 初始化较晚），主动触发一次 got_ip 逻辑，
     * 这样 SNTP/天气拉取会立即生效，首页温湿度等也会刷新。*/
    esp_netif_t *sta_now = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_now) {
        esp_netif_ip_info_t ipi;
        if (esp_netif_get_ip_info(sta_now, &ipi) == ESP_OK && ipi.ip.addr != 0) {
            ip_event_got_ip_t fake = { .ip_info = ipi };
            got_ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &fake);
        }
    }

    /* 保持原界面与生成器效果一致（其他不做外观改动） */
}
