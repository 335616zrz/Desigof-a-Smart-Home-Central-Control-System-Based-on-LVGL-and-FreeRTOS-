/**
 * components/01_application/my_ui/custom/time_ui.c
 */

#include "time_ui.h"
#include "esp_log.h"

#include <time.h>
#include <string.h>
#include <sys/time.h>

/* ===== SNTP 头文件二选一：优先 esp_sntp，其次 lwIP sntp ===== */
#ifndef __has_include
#define __has_include(x) 0
#endif

#if __has_include("esp_sntp.h")
  /* ESP-IDF 5.x 提供的封装（推荐） */
  #include "esp_sntp.h"
  #define TIMEUI_USE_ESP_SNTP 1
#else
  /* 旧路径/未启用封装时走 lwIP 原生接口 */
  #include "lwip/apps/sntp.h"
  #define TIMEUI_USE_ESP_SNTP 0
#endif

/* 统一一组“timeui_sntp_*”别名，避免到处写 #if */
#if TIMEUI_USE_ESP_SNTP
  #define timeui_sntp_setoperatingmode           esp_sntp_setoperatingmode
  #define timeui_sntp_setservername              esp_sntp_setservername
  #define timeui_sntp_init                       esp_sntp_init
  #define timeui_sntp_stop                       esp_sntp_stop
  #define timeui_sntp_enabled                    esp_sntp_enabled
  #define timeui_sntp_set_time_sync_notification_cb esp_sntp_set_time_sync_notification_cb
#else
  #define timeui_sntp_setoperatingmode           sntp_setoperatingmode
  #define timeui_sntp_setservername              sntp_setservername
  #define timeui_sntp_init                       sntp_init
  #define timeui_sntp_stop                       sntp_stop
  #define timeui_sntp_enabled                    sntp_enabled
  /* 旧版有的 'sntp_set_time_sync_notification_cb' 在部分配置中不存在——安全起见不调用 */
#endif
/* ============================================================= */

static const char *TAG = "time_ui";

/* 只在日期/星期变化时改文本，降低 LVGL 刷新量 */
static int s_last_mday = -1;
static int s_last_wday = -1;

/* 绑定到界面的两个标签指针（左：时间HH:MM:SS，右：日期YYYY-MM-DD） */
static lv_obj_t   *s_lbl_time = NULL;
static lv_obj_t   *s_lbl_date = NULL;
/* 每秒刷新一次的 LVGL 定时器（运行在 LVGL 线程内） */
static lv_timer_t *s_timer    = NULL;
/* 可选：星期标签 */
static lv_obj_t   *s_lbl_week = NULL;
/* 一组“YYYY-MM-DD HH:MM:SS”合并显示的附加标签（最多 8 个，够用且不分配堆内存） */
#define TIMEUI_MAX_DATETIME_LABELS 8
static lv_obj_t   *s_lbl_datetime[TIMEUI_MAX_DATETIME_LABELS];
static int         s_lbl_datetime_cnt = 0;

/* SNTP 同步回调：同步后立刻刷新一次并重置“上次值”，保证当天/星期立刻正确 */
static void _on_time_sync(struct timeval *tv)
{
    LV_UNUSED(tv);
    s_last_mday = -1;
    s_last_wday = -1;
    time_ui_refresh_now();
}

/* 任一被删除时把指针清空，避免后续回调访问悬空对象 */
static void _on_obj_delete(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;

    lv_obj_t *obj = lv_event_get_target(e);
    if (obj == s_lbl_time) s_lbl_time = NULL;
    if (obj == s_lbl_date) s_lbl_date = NULL;
    if (obj == s_lbl_week) s_lbl_week = NULL;

    /* 从 datetime 列表中移除此对象 */
    for (int i = 0; i < s_lbl_datetime_cnt; ++i) {
        if (s_lbl_datetime[i] == obj) {
            /* 紧凑移除 */
            for (int j = i + 1; j < s_lbl_datetime_cnt; ++j) s_lbl_datetime[j - 1] = s_lbl_datetime[j];
            s_lbl_datetime_cnt--;
            break;
        }
    }
}

/* 安全设置文本：对象为空或无效则跳过；无效则顺便清空指针 */
static inline void _label_set_text_safe(lv_obj_t **plbl, const char *txt)
{
    lv_obj_t *lbl = *plbl;
    if (!lbl) return;
#if LVGL_VERSION_MAJOR >= 8
    if (!lv_obj_is_valid(lbl)) { *plbl = NULL; return; }
#endif
    lv_label_set_text(lbl, txt);
}

static inline const char *cn_week_str(int wday)
{
    static const char *W[] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
    if (wday < 0 || wday > 6) wday = 0;
    return W[wday];
}

/* 每秒回调里：时间每秒更新；日期仅在日变更时更新；星期仅在变更时更新 */
static void _timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    // ---- HH:MM:SS ----
    char buf_time[16];
    lv_snprintf(buf_time, sizeof(buf_time), "%02d:%02d:%02d",
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    _label_set_text_safe(&s_lbl_time, buf_time);

    // ---- YYYY-MM-DD（仅日变化时更新）----
    if (tm.tm_mday != s_last_mday) {
        char buf_date[20];
        lv_snprintf(buf_date, sizeof(buf_date), "%04d-%02d-%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        _label_set_text_safe(&s_lbl_date, buf_date);
        s_last_mday = tm.tm_mday;
    }

    // ---- 星期X（仅变化时更新）----
    if (s_lbl_week && lv_obj_is_valid(s_lbl_week)) {
        if (tm.tm_wday != s_last_wday) {
            _label_set_text_safe(&s_lbl_week, cn_week_str(tm.tm_wday));
            s_last_wday = tm.tm_wday;
        }
    }

    /* 同步更新所有“YYYY-MM-DD HH:MM:SS”合并标签 */
    char buf_datetime[32];
    if (s_last_mday < 0) {
        /* 如果还没刷新过日期，先构造一次 */
        char buf_date[20];
        lv_snprintf(buf_date, sizeof(buf_date), "%04d-%02d-%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        lv_snprintf(buf_datetime, sizeof(buf_datetime), "%s %02d:%02d:%02d",
                    buf_date, tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        /* 若已刷新过日期，可直接组合 */
        char buf_date[20];
        lv_snprintf(buf_date, sizeof(buf_date), "%04d-%02d-%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        lv_snprintf(buf_datetime, sizeof(buf_datetime), "%s %02d:%02d:%02d",
                    buf_date, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    for (int i = 0; i < s_lbl_datetime_cnt; ++i) {
        _label_set_text_safe(&s_lbl_datetime[i], buf_datetime);
    }
}

/* 仅创建/重建一次定时器 */
void time_ui_init(void)
{
    if (s_timer) {
        lv_timer_del(s_timer);
        s_timer = NULL;
    }

    s_last_mday = -1;
    s_last_wday = -1;

    s_timer = lv_timer_create(_timer_cb, 1000, NULL);
    lv_timer_ready(s_timer);   // 立即跑一次
    ESP_LOGI(TAG, "time_ui initialized");
}

/* 绑定两个已有的标签到本模块（幂等防抖） */
void time_ui_attach(lv_obj_t *label_time, lv_obj_t *label_date)
{
    if (s_lbl_time == label_time && s_lbl_date == label_date) {
        return;
    }

    s_lbl_time = label_time;
    s_lbl_date = label_date;

    if (s_lbl_time) lv_obj_add_event_cb(s_lbl_time, _on_obj_delete, LV_EVENT_DELETE, NULL);
    if (s_lbl_date) lv_obj_add_event_cb(s_lbl_date, _on_obj_delete, LV_EVENT_DELETE, NULL);

    _timer_cb(NULL);
    ESP_LOGI(TAG, "time_ui attached to new ui");
}

/* 提供 detach：屏幕卸载时显式解绑，避免后续回调访问 */
void time_ui_detach(void)
{
    s_lbl_time = NULL;
    s_lbl_date = NULL;
    s_lbl_week = NULL;
    s_lbl_datetime_cnt = 0;
    // 不清 last_*，保持“只在变化时刷新”的优化
}

/* 兼容 GUI Guider 生成代码里调用的旧符号名 */
void time_ui_bind_labels(lv_obj_t *label_time, lv_obj_t *label_date)
{
    time_ui_attach(label_time, label_date);
}

void time_ui_bind_week_label(lv_obj_t *lbl_week)
{
    s_lbl_week = lbl_week;
    if (s_lbl_week) {
        lv_obj_add_event_cb(s_lbl_week, _on_obj_delete, LV_EVENT_DELETE, NULL);
    }
    s_last_wday = -1;
    time_ui_refresh_now();
}

void time_ui_bind_datetime_label(lv_obj_t *lbl_datetime)
{
    if (!lbl_datetime) return;
    /* 重复绑定忽略 */
    for (int i = 0; i < s_lbl_datetime_cnt; ++i) {
        if (s_lbl_datetime[i] == lbl_datetime) return;
    }
    if (s_lbl_datetime_cnt < TIMEUI_MAX_DATETIME_LABELS) {
        s_lbl_datetime[s_lbl_datetime_cnt++] = lbl_datetime;
        lv_obj_add_event_cb(lbl_datetime, _on_obj_delete, LV_EVENT_DELETE, NULL);
        time_ui_refresh_now();
    }
}

void time_ui_refresh_now(void)
{
    if (s_timer) {
        lv_timer_ready(s_timer);
    } else {
        _timer_cb(NULL);
    }
}

/* 只在 root 正是当前绑定的 label 所在的屏时才执行 detach */
void time_ui_detach_if_screen(lv_obj_t *root)
{
    if (!root) return;

    bool hit = false;
    if (s_lbl_time && lv_obj_is_valid(s_lbl_time) && lv_obj_get_screen(s_lbl_time) == root) hit = true;
    if (s_lbl_date && lv_obj_is_valid(s_lbl_date) && lv_obj_get_screen(s_lbl_date) == root) hit = true;
    if (s_lbl_week && lv_obj_is_valid(s_lbl_week) && lv_obj_get_screen(s_lbl_week) == root) hit = true;

    if (hit) {
        time_ui_detach();
    }
}

/* 拿到 IP 后启动 SNTP；tz_str 例如 "CST-8"，传 NULL 用默认 */
void time_ui_start_sntp(const char *tz_str)
{
    /* 设置时区 */
    const char *tz = tz_str ? tz_str : "CST-8";   /* 中国常用 */
    setenv("TZ", tz, 1);
    tzset();

    const bool already_running = timeui_sntp_enabled();

    /* 若 net_wifi_sntp 已经启动 SNTP，这里只更新回调/时区，避免重复重启导致断言 */
    if (already_running) {
#if TIMEUI_USE_ESP_SNTP
        timeui_sntp_set_time_sync_notification_cb(_on_time_sync);
#endif
        ESP_LOGI(TAG, "SNTP already running, TZ=%s", tz);
        return;
    }

    /* 可按需修改/增减 NTP 服务器 */
    timeui_sntp_setservername(0, "ntp.aliyun.com");
    timeui_sntp_setservername(1, "pool.ntp.org");
    timeui_sntp_setservername(2, "time1.cloud.tencent.com");

#if TIMEUI_USE_ESP_SNTP
    /* 仅在提供回调 API 的情况下注册同步回调 */
    timeui_sntp_set_time_sync_notification_cb(_on_time_sync);
#endif

    timeui_sntp_init();
    ESP_LOGI(TAG, "SNTP started, TZ=%s", tz);
}
