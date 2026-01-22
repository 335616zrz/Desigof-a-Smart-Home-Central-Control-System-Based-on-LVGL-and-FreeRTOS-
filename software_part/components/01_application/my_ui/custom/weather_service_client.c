#include "weather_service_client.h"
#include "sdkconfig.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "esp_timer.h"
#include "audio_thread.h"
#include "custom.h"   // env_mode_is_indoor()
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===== 可配置项（如未在 Kconfig 中提供，使用默认值） =====
#ifndef CONFIG_MY_UI_WEATHER_HOST
#define CONFIG_MY_UI_WEATHER_HOST "192.168.1.2"
#endif
#ifndef CONFIG_MY_UI_WEATHER_PORT
#define CONFIG_MY_UI_WEATHER_PORT 3001
#endif
#ifndef CONFIG_MY_UI_WEATHER_SCHEME
#define CONFIG_MY_UI_WEATHER_SCHEME 0 // 0=http, 1=https (目前本地服务是 http)
#endif

static const char *TAG = "WEATHER";
static lv_ui *s_ui = NULL;
static uint32_t s_last_adcode = 0;
static esp_timer_handle_t s_timer = NULL;            // 5 分钟长周期刷新

// 首页温湿度数字的最近一次“天气服务”数据缓存（不带单位）
static char s_home_temp[16] = {0};
static char s_home_humi[16] = {0};
static bool s_home_has_live = false;
// —— 开机短重试相关 ——
static esp_timer_handle_t s_short_timer = NULL;      // one-shot 计时器
static uint8_t            s_short_idx   = 0;         // 当前序号（2s→5s→10s→20s）
static bool               s_short_active= false;     // 标记短重试是否激活
static volatile bool      s_any_success = false;     // 最近一次请求成功标记

static void short_retry_arm_next(void);
static void short_retry_stop_safe(void);

void weather_bind_ui(lv_ui *ui) { s_ui = ui; }

typedef struct { uint32_t adcode; } req_args_t;

static bool http_get_json(const char *url, char **out, int *out_len)
{
    *out = NULL; *out_len = 0;
    esp_http_client_config_t cfg = { .url = url, .timeout_ms = 8000 };

    /* 串行化 TLS 握手（即使是 HTTP 也加锁，避免与其它服务竞争网络资源） */
    AppTLS_Lock();
    esp_http_client_handle_t cli = esp_http_client_init(&cfg);
    AppTLS_Unlock();
    if (!cli) return false;
    if (esp_http_client_open(cli, 0) != ESP_OK) { esp_http_client_cleanup(cli); return false; }
    int content_len = esp_http_client_fetch_headers(cli);
    int alloc = (content_len > 0 && content_len < (512*1024)) ? (content_len + 1) : (8*1024);
    char *buf = (char*)malloc(alloc);
    if (!buf) { esp_http_client_close(cli); esp_http_client_cleanup(cli); return false; }
    int r, off = 0;
    while ((r = esp_http_client_read(cli, buf + off, alloc - 1 - off)) > 0) {
        off += r;
        if (off > alloc - 1024) { // 扩容
            int nalloc = alloc * 2;
            char *nb = (char*)realloc(buf, nalloc);
            if (!nb) { free(buf); esp_http_client_close(cli); esp_http_client_cleanup(cli); return false; }
            buf = nb; alloc = nalloc;
        }
    }
    buf[off] = '\0';
    esp_http_client_close(cli);
    esp_http_client_cleanup(cli);
    *out = buf; *out_len = off;
    return true;
}

static void ui_set_label_text(lv_obj_t *lbl, const char *txt)
{
    if (!lbl) return;
    /* 防御：仅对“有效的 Label 对象”写入，避免错误的指针或类型导致崩溃 */
    if (!lv_obj_is_valid(lbl)) return;
    extern const lv_obj_class_t lv_label_class; /* 由 LVGL 提供 */
    if (!lv_obj_check_type(lbl, &lv_label_class)) return;
    lv_label_set_text(lbl, txt ? txt : "");
}

static void apply_live_to_ui(cJSON *root)
{
    const cJSON *lives = cJSON_GetObjectItemCaseSensitive(root, "lives");
    if (!cJSON_IsArray(lives)) {
        ESP_LOGW(TAG, "live JSON missing 'lives' array");
        return;
    }
    const cJSON *live = cJSON_GetArrayItem(lives, 0);
    if (!cJSON_IsObject(live)) {
        ESP_LOGW(TAG, "live JSON: lives[0] is not an object");
        return;
    }
    const char *province = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "province"));
    const char *city     = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "city"));
    const char *weather  = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "weather"));
    const char *temp     = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "temperature"));
    const char *winddir  = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "winddirection"));
    const char *windpow  = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "windpower"));
    const char *humi     = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "humidity"));
    const char *rtime    = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(live, "reporttime"));
    LV_UNUSED(rtime); // 右上角时间由 time_ui 负责，这里不使用 reporttime

    char loc[64];
    char tbuf[16];
    char wdirbuf[32];
    char wpowbuf[32];
    char humibuf[32];
    snprintf(loc, sizeof(loc), "%s %s", province ? province : "—", city ? city : "—");
    snprintf(tbuf, sizeof(tbuf), "%s°", temp ? temp : "--");
    snprintf(wdirbuf, sizeof(wdirbuf), "风向 %s", winddir ? winddir : "—");
    snprintf(wpowbuf, sizeof(wpowbuf), "风力 %s", windpow ? windpow : "—");
    if (humi && humi[0] != '\0') snprintf(humibuf, sizeof(humibuf), "湿度 %s%%", humi);
    else snprintf(humibuf, sizeof(humibuf), "湿度 —%%");

    // 记录一份供首页“室外模式”使用的缓存（不带单位）
    if (temp && temp[0]) {
        snprintf(s_home_temp, sizeof(s_home_temp), "%s", temp);
    } else {
        s_home_temp[0] = '\0';
    }
    if (humi && humi[0]) {
        snprintf(s_home_humi, sizeof(s_home_humi), "%s", humi);
    } else {
        s_home_humi[0] = '\0';
    }
    s_home_has_live = true;

    /* Live data often returns much faster than forecast (especially with caching),
     * and the UI may still be doing a big screen transition/flush at this moment.
     * If we bail out too early here, users will see "forecast updated but live stuck".
     * Wait longer and retry so live eventually catches up. */
    const uint32_t lock_ms = 1500;
    if (!lvgl_port_lock(lock_ms)) {
        ESP_LOGW(TAG, "apply_live_to_ui: lvgl lock timeout (%ums), skip", (unsigned)lock_ms);
        return;
    }
    // Weather screen bindings（仅当天气页已创建并且控件为 label 时才写）
    ui_set_label_text(s_ui->weather_live_loc, loc);
    /* 说明：天气页右上角时间由首页时间镜像显示，这里不再写入 reporttime，
     * 避免覆盖实时时钟。实时校准由 SNTP + time_ui 负责。*/
    ui_set_label_text(s_ui->weather_live_temp, tbuf);
    ui_set_label_text(s_ui->weather_live_cond, weather ? weather : "—");
    ui_set_label_text(s_ui->weather_chip_winddir_lbl, wdirbuf);
    ui_set_label_text(s_ui->weather_chip_windpow_lbl, wpowbuf);
    ui_set_label_text(s_ui->weather_chip_humi_lbl, humibuf);
    // Home screen mirrors（浅色）
    if (s_ui->screen_label_11) ui_set_label_text(s_ui->screen_label_11, loc);                   // 位置到首页
    if (s_ui->screen_label_14) ui_set_label_text(s_ui->screen_label_14, weather ? weather : "—"); // 天气现象到首页
    /* 温湿度数字：仅在当前为“室外”模式时更新；室内模式下由 SHT40 驱动更新 */
    if (!env_mode_is_indoor()) {
        if (s_ui->screen_label_5)  ui_set_label_text(s_ui->screen_label_5,  temp ? temp : "--");      // 温度数字到首页（不带°）
        if (s_ui->screen_label_9)  ui_set_label_text(s_ui->screen_label_9,  humi ? humi : "--");      // 湿度数字到首页（不带%）
    }
    // Home screen mirrors（深色）
    if (s_ui->screen_1_label_11) ui_set_label_text(s_ui->screen_1_label_11, loc);
    if (s_ui->screen_1_label_14) ui_set_label_text(s_ui->screen_1_label_14, weather ? weather : "—");
    /* 深色首页温湿度数字同样仅在“室外模式”下由天气服务驱动；
     * 室内模式下这两个数字由 SHT40 驱动负责。*/
    if (!env_mode_is_indoor()) {
        if (s_ui->screen_1_label_5)  ui_set_label_text(s_ui->screen_1_label_5,  temp ? temp : "--");
        if (s_ui->screen_1_label_9)  ui_set_label_text(s_ui->screen_1_label_9,  humi ? humi : "--");
    }
    lvgl_port_unlock();

    // 标记一次成功，供“开机短重试”判断是否需要继续
    s_any_success = true;
}

void weather_home_apply_from_cache(void)
{
    if (!s_ui) return;
    if (!s_home_has_live) return;
    /* 室内模式下由 SHT40 驱动负责首页展示，这里不覆盖 */
    if (env_mode_is_indoor()) return;

    if (!lvgl_port_lock(100)) {
        return;
    }

    /* 浅色首页 */
    if (s_ui->screen_label_5) {
        ui_set_label_text(s_ui->screen_label_5,
                          s_home_temp[0] ? s_home_temp : "--");
    }
    if (s_ui->screen_label_9) {
        ui_set_label_text(s_ui->screen_label_9,
                          s_home_humi[0] ? s_home_humi : "--");
    }

    /* 深色首页镜像 */
    if (s_ui->screen_1_label_5) {
        ui_set_label_text(s_ui->screen_1_label_5,
                          s_home_temp[0] ? s_home_temp : "--");
    }
    if (s_ui->screen_1_label_9) {
        ui_set_label_text(s_ui->screen_1_label_9,
                          s_home_humi[0] ? s_home_humi : "--");
    }

    lvgl_port_unlock();
}

static inline const char *week_to_cn(const char *wk)
{
    if (!wk || !wk[0]) return "周?";
    switch (wk[0]) {
        case '1': return "周1"; // Mon
        case '2': return "周2";
        case '3': return "周3";
        case '4': return "周4";
        case '5': return "周5";
        case '6': return "周6";
        case '7': return "周7"; // Sun
        default:  return "周?";
    }
}

static void apply_forecast_to_ui(cJSON *root)
{
    const cJSON *fcs = cJSON_GetObjectItemCaseSensitive(root, "forecasts");
    if (!cJSON_IsArray(fcs)) return;
    const cJSON *fc0 = cJSON_GetArrayItem(fcs, 0);
    if (!cJSON_IsObject(fc0)) return;
    const cJSON *casts = cJSON_GetObjectItemCaseSensitive(fc0, "casts");
    if (!cJSON_IsArray(casts)) return;

    // 每张卡片的四个标签：日期、天气、温度、风
    lv_obj_t *date_lbls[4] = { s_ui->weather_main_fc_row0_lbl,  s_ui->weather_main_fc_row1_lbl,
                               s_ui->weather_main_fc_row2_lbl,  s_ui->weather_main_fc_row3_lbl };
    lv_obj_t *cond_lbls[4] = { s_ui->weather_main_fc_row0_cond, s_ui->weather_main_fc_row1_cond,
                               s_ui->weather_main_fc_row2_cond, s_ui->weather_main_fc_row3_cond };
    lv_obj_t *temp_lbls[4] = { s_ui->weather_main_fc_row0_temp, s_ui->weather_main_fc_row1_temp,
                               s_ui->weather_main_fc_row2_temp, s_ui->weather_main_fc_row3_temp };
    lv_obj_t *wind_lbls[4] = { s_ui->weather_main_fc_row0_wind, s_ui->weather_main_fc_row1_wind,
                               s_ui->weather_main_fc_row2_wind, s_ui->weather_main_fc_row3_wind };

    char buf_date[64];
    char buf_cond[48];
    char buf_temp[32];
    char buf_wind[64];

    int avail = cJSON_GetArraySize(casts);
    if (avail > 4) avail = 4;
    for (int i = 0; i < avail; i++) {
        const cJSON *c = cJSON_GetArrayItem(casts, i);
        if (!cJSON_IsObject(c)) continue;

        const char *date = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "date"));
        const char *week = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "week"));
        const char *dw   = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "dayweather"));
        const char *nw   = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "nightweather"));
        const char *dtmp = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "daytemp"));
        const char *ntmp = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "nighttemp"));
        const char *dwd  = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "daywind"));
        const char *nwd  = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "nightwind"));
        const char *dpow = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "daypower"));
        const char *npow = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(c, "nightpower"));

        // 日期格式：2025-11-14·周5（不加空格）
        snprintf(buf_date, sizeof(buf_date), "%s·%s", date ? date : "--/--", week_to_cn(week));
        snprintf(buf_cond, sizeof(buf_cond), "%s/%s", dw ? dw : "—", nw ? nw : "—");
        // 温度：夜低 ~ 日高（与 UI 一致）
        snprintf(buf_temp, sizeof(buf_temp), "%s° ~ %s°", ntmp ? ntmp : "—", dtmp ? dtmp : "—");
        // 风向风力：风<日>/<夜> · 力<日>-<夜>（有缺失则用单端或"—"）
        if (dpow && npow)
            snprintf(buf_wind, sizeof(buf_wind), "风 %s/%s\n力 %s/%s", dwd?dwd:"—", nwd?nwd:"—", dpow, npow);
        else if (dpow)
            snprintf(buf_wind, sizeof(buf_wind), "风 %s/%s\n力 %s", dwd?dwd:"—", nwd?nwd:"—", dpow);
        else if (npow)
            snprintf(buf_wind, sizeof(buf_wind), "风 %s/%s\n力 %s", dwd?dwd:"—", nwd?nwd:"—", npow);
        else
            snprintf(buf_wind, sizeof(buf_wind), "风 %s/%s\n力 —", dwd?dwd:"—", nwd?nwd:"—");

        if (!lvgl_port_lock(100)) continue;
        ui_set_label_text(date_lbls[i], buf_date);
        ui_set_label_text(cond_lbls[i], buf_cond);
        ui_set_label_text(temp_lbls[i], buf_temp);
        ui_set_label_text(wind_lbls[i], buf_wind);
        lvgl_port_unlock();
    }
    // 若不足 4 天，清空剩余卡片
    for (int i = avail; i < 4; i++) {
        if (!lvgl_port_lock(100)) continue;
        ui_set_label_text(date_lbls[i], "--/-- · 周?");
        ui_set_label_text(cond_lbls[i], "—/—");
        ui_set_label_text(temp_lbls[i], "—° ~ —°");
        ui_set_label_text(wind_lbls[i], "风—/— · 力—");
        lvgl_port_unlock();
    }

    // 也视为一次成功（即便 live 失败，forecast 成功也能停止短重试）
    s_any_success = true;
}

static void weather_task(void *arg)
{
    req_args_t *a = (req_args_t*)arg;
    char url[160];
    const char *scheme = CONFIG_MY_UI_WEATHER_SCHEME ? "https" : "http";
    // live
    snprintf(url, sizeof(url), "%s://%s:%d/api/weather?city=%u&extensions=base&output=JSON",
             scheme, CONFIG_MY_UI_WEATHER_HOST, (int)CONFIG_MY_UI_WEATHER_PORT, (unsigned)a->adcode);
    ESP_LOGI(TAG, "GET %s", url);
    char *body = NULL; int blen = 0;
    if (http_get_json(url, &body, &blen)) {
        cJSON *root = cJSON_ParseWithLength(body, blen);
        if (root) { apply_live_to_ui(root); cJSON_Delete(root); }
        free(body);
    } else {
        ESP_LOGW(TAG, "live request failed");
    }
    // forecast
    snprintf(url, sizeof(url), "%s://%s:%d/api/weather?city=%u&extensions=all&output=JSON",
             scheme, CONFIG_MY_UI_WEATHER_HOST, (int)CONFIG_MY_UI_WEATHER_PORT, (unsigned)a->adcode);
    ESP_LOGI(TAG, "GET %s", url);
    body = NULL; blen = 0;
    if (http_get_json(url, &body, &blen)) {
        cJSON *root = cJSON_ParseWithLength(body, blen);
        if (root) { apply_forecast_to_ui(root); cJSON_Delete(root); }
        free(body);
    } else {
        ESP_LOGW(TAG, "forecast request failed");
    }
    free(a);
    // 若短重试正在进行并且已经有成功结果，停止短重试
    // 设置标志即可，不在任务中操作 esp_timer
    if (s_short_active && s_any_success) {
        ESP_LOGI(TAG, "short-retry: success, will stop on next callback check");
        // s_any_success 已经设置，下次回调会检测到并停止
    }
    vTaskDelete(NULL);
}

bool weather_update_for_adcode(uint32_t adcode)
{
    if (!adcode) return false;
    req_args_t *a = (req_args_t*)malloc(sizeof(req_args_t));
    if (!a) return false;
    a->adcode = adcode;
    s_last_adcode = adcode;
    // CPU1: 网络任务恢复原始配置
    esp_err_t rc = audio_thread_create(NULL,
                                       "weather_task",
                                       weather_task,
                                       a,
                                       4096,
                                       5,
                                       true,  /* stack in PSRAM when possible */
                                       1);
    if (rc != ESP_OK) {
        free(a);
        return false;
    }
    return true;
}

void weather_set_last_adcode(uint32_t adcode)
{
    s_last_adcode = adcode;
}

static void timer_cb(void *arg)
{
    uint32_t ad = s_last_adcode;
    if (ad) {
        ESP_LOGI(TAG, "periodic refresh: adcode=%u", (unsigned)ad);
        (void)weather_update_for_adcode(ad);
    } else {
        ESP_LOGW(TAG, "periodic refresh skipped: no last adcode");
    }
}

bool weather_periodic_start(uint32_t period_ms)
{
    if (period_ms == 0) period_ms = 300000; // default 5 min
    if (s_timer) {
        esp_timer_stop(s_timer);
        esp_timer_delete(s_timer);
        s_timer = NULL;
    }
    const esp_timer_create_args_t args = {
        .callback = timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "weather_periodic"
    };
    if (esp_timer_create(&args, &s_timer) != ESP_OK) return false;
    return esp_timer_start_periodic(s_timer, (uint64_t)period_ms * 1000ULL) == ESP_OK;
}

void weather_periodic_stop(void)
{
    if (s_timer) {
        esp_timer_stop(s_timer);
        esp_timer_delete(s_timer);
        s_timer = NULL;
    }
}

// ===== 开机短重试实现 =====

/* 安全停止短重试 - 可从任何上下文调用 */
static void short_retry_stop_safe(void)
{
    if (!s_short_active) return;

    s_short_active = false;
    s_short_idx = 0;

    /* 停止定时器，但不删除 - 删除操作由 weather_short_retry_stop() 完成 */
    if (s_short_timer) {
        esp_timer_stop(s_short_timer);
        ESP_LOGI(TAG, "short-retry: stopped");
    }
}

static void short_retry_cb(void *arg)
{
    (void)arg;
    if (!s_short_active) return;
    if (s_any_success) {
        // 已有成功结果，停止短重试
        short_retry_stop_safe();
        return;
    }
    uint32_t ad = s_last_adcode;
    if (ad) {
        ESP_LOGI(TAG, "short-retry[%u]: adcode=%u", (unsigned)s_short_idx, (unsigned)ad);
        (void)weather_update_for_adcode(ad);
    } else {
        ESP_LOGW(TAG, "short-retry: no adcode yet, skip this tick");
    }
    // 安排下一次
    short_retry_arm_next();
}

static void short_retry_arm_next(void)
{
    static const uint32_t s_delays_ms[] = { 2000, 5000, 10000, 20000 };
    const uint32_t n = sizeof(s_delays_ms)/sizeof(s_delays_ms[0]);
    if (s_short_idx >= n) {
        // 结束短重试窗口
        ESP_LOGI(TAG, "short-retry: finished (no success)");
        short_retry_stop_safe();
        return;
    }

    if (!s_short_timer) {
        const esp_timer_create_args_t args = {
            .callback = short_retry_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "weather_boot_retry"
        };
        if (esp_timer_create(&args, &s_short_timer) != ESP_OK) {
            ESP_LOGE(TAG, "short-retry: timer create failed");
            s_short_active = false;
            return;
        }
    } else {
        (void)esp_timer_stop(s_short_timer);
    }

    const uint64_t us = (uint64_t)s_delays_ms[s_short_idx] * 1000ULL;
    s_short_idx++;
    (void)esp_timer_start_once(s_short_timer, us);
}

void weather_short_retry_bootstrap(void)
{
    s_any_success  = false;
    s_short_idx    = 0;
    s_short_active = true;
    short_retry_arm_next();
}

void weather_short_retry_stop(void)
{
    /* 停止并删除esp_timer（在非回调上下文中调用此函数是安全的） */
    if (s_short_timer) {
        esp_timer_stop(s_short_timer);
        esp_timer_delete(s_short_timer);
        s_short_timer = NULL;
        ESP_LOGI(TAG, "short-retry: timer deleted");
    }
    s_short_active = false;
    s_short_idx = 0;
}
