#include "ota_upgrade_dialog.h"

#include <string.h>

#include "fonts.h"

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ota_update.h"
#endif

typedef struct {
    bool       dark;
    lv_obj_t  *modal;
    lv_obj_t  *panel;
    lv_obj_t  *lbl_title;
    lv_obj_t  *lbl_ver;
    lv_obj_t  *lbl_status;
    lv_obj_t  *bar;
    lv_obj_t  *lbl_pct;
    lv_obj_t  *btn_ok;
    lv_obj_t  *lbl_ok;
    lv_obj_t  *btn_cancel;
    lv_obj_t  *lbl_cancel;
    lv_timer_t *timer;

#if !defined(ESP_PLATFORM)
    int sim_progress;
    bool sim_check_running;
    bool sim_upgrade_running;
    uint32_t sim_check_deadline_ms;
    bool sim_has_update;
    char sim_target_ver[32];
#endif
} ota_dlg_t;

static ota_dlg_t s_dlg_light = {.dark = false};
static ota_dlg_t s_dlg_dark  = {.dark = true};

static lv_timer_t *s_auto_popup_timer = NULL;
static bool        s_auto_popup_done  = false;

static ota_dlg_t *dlg_get(bool dark)
{
    return dark ? &s_dlg_dark : &s_dlg_light;
}

static bool obj_ok(lv_obj_t *obj)
{
    return obj && lv_obj_is_valid(obj);
}

static void dlg_timer_stop(ota_dlg_t *dlg)
{
    if (!dlg) return;
    if (dlg->timer) {
        lv_timer_del(dlg->timer);
        dlg->timer = NULL;
    }
}

static void dlg_set_btn_enabled(lv_obj_t *btn, bool enabled)
{
    if (!obj_ok(btn)) return;
    if (enabled) {
        lv_obj_remove_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void dlg_set_text(lv_obj_t *lbl, const char *txt)
{
    if (!obj_ok(lbl)) return;
    lv_label_set_text(lbl, txt ? txt : "");
}

static void dlg_update_version_from_ota(ota_dlg_t *dlg)
{
#if defined(ESP_PLATFORM) && CONFIG_APP_OTA_ENABLE
    const char *new_ver = ota_update_get_new_version();
    if (new_ver && new_ver[0] && obj_ok(dlg->lbl_ver)) {
        char buf[64];
        lv_snprintf(buf, sizeof(buf), "目标版本：%s", new_ver);
        dlg_set_text(dlg->lbl_ver, buf);
    }
#else
    (void)dlg;
#endif
}

static void dlg_progress_set(ota_dlg_t *dlg, int pct)
{
    if (!dlg) return;
    if (!obj_ok(dlg->bar) || !obj_ok(dlg->lbl_pct)) return;

    if (pct < 0) {
        lv_bar_set_value(dlg->bar, 0, LV_ANIM_OFF);
        dlg_set_text(dlg->lbl_pct, "--%");
        return;
    }
    if (pct > 100) pct = 100;
    lv_bar_set_value(dlg->bar, pct, LV_ANIM_OFF);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d%%", pct);
    dlg_set_text(dlg->lbl_pct, buf);
}

static void dlg_finish_to_closable(ota_dlg_t *dlg, const char *status_text)
{
    if (!dlg) return;
    dlg_set_text(dlg->lbl_status, status_text);
    dlg_set_btn_enabled(dlg->btn_cancel, true);
    dlg_set_btn_enabled(dlg->btn_ok, false);
    if (obj_ok(dlg->lbl_cancel)) {
        dlg_set_text(dlg->lbl_cancel, "关闭");
    }
}

static void ota_timer_cb(lv_timer_t *t)
{
    ota_dlg_t *dlg = (ota_dlg_t *)lv_timer_get_user_data(t);
    if (!dlg) return;

    if (!obj_ok(dlg->modal) || !obj_ok(dlg->bar) || !obj_ok(dlg->lbl_status) ||
        !obj_ok(dlg->lbl_pct) || !obj_ok(dlg->lbl_ver) ||
        !obj_ok(dlg->btn_ok) || !obj_ok(dlg->btn_cancel)) {
        dlg_timer_stop(dlg);
        return;
    }

    dlg_update_version_from_ota(dlg);

#if defined(ESP_PLATFORM)
#if CONFIG_APP_OTA_ENABLE
    const bool update_running = ota_update_is_running();
    const bool check_running  = ota_update_check_is_running();
    const ota_update_check_result_t check_res = ota_update_get_check_result();

    /* ---- OTA 正在升级：显示进度 ---- */
    if (update_running) {
        if (obj_ok(dlg->bar)) lv_obj_clear_flag(dlg->bar, LV_OBJ_FLAG_HIDDEN);
        if (obj_ok(dlg->lbl_pct)) lv_obj_clear_flag(dlg->lbl_pct, LV_OBJ_FLAG_HIDDEN);

        dlg_set_text(dlg->lbl_status, "升级中…");
        dlg_set_btn_enabled(dlg->btn_ok, false);
        dlg_set_btn_enabled(dlg->btn_cancel, false);

        const int pct = ota_update_get_progress_percent();
        dlg_progress_set(dlg, pct);
        return;
    }

    /* ---- OTA 不在升级，但升级UI仍显示：说明刚结束（成功/失败/跳过） ---- */
    const bool upgrade_ui_active = obj_ok(dlg->bar) && !lv_obj_has_flag(dlg->bar, LV_OBJ_FLAG_HIDDEN);
    if (upgrade_ui_active) {
        const int pct = ota_update_get_progress_percent();
        const esp_err_t last = ota_update_get_last_error();
        if (last != ESP_OK) {
            char buf[96];
            lv_snprintf(buf, sizeof(buf), "升级失败：%s", esp_err_to_name(last));
            dlg_finish_to_closable(dlg, buf);
        } else if (pct < 0) {
            dlg_finish_to_closable(dlg, "已是最新版本，无需升级");
        } else if (pct >= 100) {
            dlg_finish_to_closable(dlg, "升级完成，设备即将重启…");
        } else {
            dlg_finish_to_closable(dlg, "升级结束");
        }
        dlg_timer_stop(dlg);
        return;
    }

    /* ---- 仅检查更新（未开始升级） ---- */
    if (check_running) {
        dlg_set_text(dlg->lbl_status, "正在检查更新…");
        dlg_set_btn_enabled(dlg->btn_ok, false);
        dlg_set_btn_enabled(dlg->btn_cancel, true);
        if (obj_ok(dlg->lbl_cancel)) dlg_set_text(dlg->lbl_cancel, "取消");
        return;
    }

    switch (check_res) {
    case OTA_UPDATE_CHECK_HAS_UPDATE:
        dlg_set_text(dlg->lbl_status, "发现新版本，是否升级？");
        dlg_set_btn_enabled(dlg->btn_ok, true);
        dlg_set_btn_enabled(dlg->btn_cancel, true);
        if (obj_ok(dlg->lbl_cancel)) dlg_set_text(dlg->lbl_cancel, "取消");
        break;
    case OTA_UPDATE_CHECK_NO_UPDATE:
        dlg_finish_to_closable(dlg, "已是最新版本，无需更新");
        break;
    case OTA_UPDATE_CHECK_ERROR: {
        const esp_err_t err = ota_update_get_check_error();
        char buf[96];
        lv_snprintf(buf, sizeof(buf), "检查更新失败：%s", esp_err_to_name(err));
        dlg_finish_to_closable(dlg, buf);
        break;
    }
    default:
        dlg_set_text(dlg->lbl_status, "正在检查更新…");
        dlg_set_btn_enabled(dlg->btn_ok, false);
        dlg_set_btn_enabled(dlg->btn_cancel, true);
        if (obj_ok(dlg->lbl_cancel)) dlg_set_text(dlg->lbl_cancel, "取消");
        break;
    }
#else
    dlg_finish_to_closable(dlg, "OTA 未启用（menuconfig -> OTA Options）");
    dlg_timer_stop(dlg);
#endif
#else
    /* ===================== Simulator ===================== */
    const uint32_t now = lv_tick_get();

    /* ---- Simulate "check version" ---- */
    if (dlg->sim_check_running) {
        if (now >= dlg->sim_check_deadline_ms) {
            dlg->sim_check_running = false;

            /* Default: always have a new version until we "upgrade" once */
            if (dlg->sim_target_ver[0] == '\0') {
                lv_snprintf(dlg->sim_target_ver, sizeof(dlg->sim_target_ver), "0.0.27");
            }
            if (!dlg->sim_upgrade_running && dlg->sim_has_update) {
                char buf[64];
                lv_snprintf(buf, sizeof(buf), "目标版本：%s", dlg->sim_target_ver);
                dlg_set_text(dlg->lbl_ver, buf);
                dlg_set_text(dlg->lbl_status, "发现新版本，是否升级？");
                dlg_set_btn_enabled(dlg->btn_ok, true);
                dlg_set_btn_enabled(dlg->btn_cancel, true);
                if (obj_ok(dlg->lbl_cancel)) dlg_set_text(dlg->lbl_cancel, "取消");
            } else {
                dlg_finish_to_closable(dlg, "已是最新版本，无需更新");
            }

            /* Stop polling until user clicks "确认" */
            dlg_timer_stop(dlg);
        }
        return;
    }

    /* ---- Simulate upgrading progress ---- */
    if (dlg->sim_upgrade_running) {
        if (dlg->sim_progress < 100) {
            dlg->sim_progress += 2;
            if (dlg->sim_progress > 100) dlg->sim_progress = 100;
            dlg_progress_set(dlg, dlg->sim_progress);
        }

        if (dlg->sim_progress >= 100) {
            dlg->sim_upgrade_running = false;
            dlg->sim_has_update = false; /* now "up to date" */
            dlg_finish_to_closable(dlg, "升级完成（模拟）");
            dlg_timer_stop(dlg);
        }
        return;
    }
#endif
}

static void dlg_start_timer(ota_dlg_t *dlg)
{
    if (!dlg) return;
    dlg_timer_stop(dlg);
    dlg->timer = lv_timer_create(ota_timer_cb, 200, dlg);
    lv_timer_ready(dlg->timer);
}

static void on_btn_cancel(lv_event_t *e)
{
    ota_dlg_t *dlg = (ota_dlg_t *)lv_event_get_user_data(e);
    if (!dlg) return;
    dlg_timer_stop(dlg);
#if defined(ESP_PLATFORM) && CONFIG_APP_OTA_ENABLE
    ota_mark_user_cancelled();
#endif
#if !defined(ESP_PLATFORM)
    dlg->sim_check_running = false;
    dlg->sim_upgrade_running = false;
#endif
    if (obj_ok(dlg->modal)) {
        lv_obj_add_flag(dlg->modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_btn_ok(lv_event_t *e)
{
    ota_dlg_t *dlg = (ota_dlg_t *)lv_event_get_user_data(e);
    if (!dlg) return;

    if (!obj_ok(dlg->modal)) return;

#if defined(ESP_PLATFORM) && CONFIG_APP_OTA_ENABLE
    /* 若仍在检查更新，确认不可用（双保险） */
    if (ota_update_check_is_running()) {
        dlg_set_text(dlg->lbl_status, "正在检查更新…");
        dlg_set_btn_enabled(dlg->btn_ok, false);
        return;
    }

    /* 若当前已判定“无更新”，确认不应可用（双保险） */
    if (ota_update_get_check_result() != OTA_UPDATE_CHECK_HAS_UPDATE) {
        dlg_finish_to_closable(dlg, "已是最新版本，无需更新");
        return;
    }
#endif

    /* Start OTA first (so UI won't enter progress mode on failure/busy) */
#if defined(ESP_PLATFORM)
#if CONFIG_APP_OTA_ENABLE
    if (!ota_update_is_running()) {
        esp_err_t rc = ota_update_start(CONFIG_APP_OTA_URL);
        if (rc != ESP_OK) {
            char buf[96];
            lv_snprintf(buf, sizeof(buf), "启动 OTA 失败：%s", esp_err_to_name(rc));
            dlg_finish_to_closable(dlg, buf);
            return;
        }
    }
#else
    dlg_finish_to_closable(dlg, "OTA 未启用（menuconfig -> OTA Options）");
    return;
#endif
#else
    dlg->sim_progress = 0;
    dlg->sim_upgrade_running = true;
    dlg_progress_set(dlg, 0);
#endif

    /* UI: switch to progress mode */
    if (obj_ok(dlg->bar)) lv_obj_clear_flag(dlg->bar, LV_OBJ_FLAG_HIDDEN);
    if (obj_ok(dlg->lbl_pct)) lv_obj_clear_flag(dlg->lbl_pct, LV_OBJ_FLAG_HIDDEN);

    dlg_set_text(dlg->lbl_status, "升级中…");
    dlg_set_btn_enabled(dlg->btn_ok, false);
    dlg_set_btn_enabled(dlg->btn_cancel, false);

    dlg_start_timer(dlg);
}

static void on_open_btn(lv_event_t *e)
{
    ota_dlg_t *dlg = (ota_dlg_t *)lv_event_get_user_data(e);
    if (!dlg) return;
    ota_upgrade_dialog_show(dlg->dark, NULL);
}

void ota_upgrade_dialog_create(lv_obj_t *parent, bool dark_mode)
{
    ota_dlg_t *dlg = dlg_get(dark_mode);
    if (!dlg) return;

    /* Recreate if previous instance became invalid (screen deleted/rebuilt) */
    if (obj_ok(dlg->modal)) {
        return;
    }

    dlg_timer_stop(dlg);
    memset(dlg, 0, sizeof(*dlg));
    dlg->dark = dark_mode;

    dlg->modal = lv_obj_create(parent);
    lv_obj_set_pos(dlg->modal, 0, 0);
    lv_obj_set_size(dlg->modal, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(dlg->modal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(dlg->modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(dlg->modal, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dlg->modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->panel = lv_obj_create(dlg->modal);
    lv_obj_set_size(dlg->panel, 420, 220);
    /* 使用绝对坐标（TOP_LEFT），方便做“从下往上弹出”的 y 动画 */
    lv_obj_set_style_align(dlg->panel, LV_ALIGN_TOP_LEFT, 0);
    lv_obj_set_pos(dlg->panel, 30, 50);
    lv_obj_clear_flag(dlg->panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(dlg->panel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dlg->panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    const lv_color_t panel_bg = dark_mode ? lv_color_hex(0x1f1f1f) : lv_color_hex(0xffffff);
    const lv_color_t text_col = dark_mode ? lv_color_hex(0xffffff) : lv_color_hex(0x0d3055);
    lv_obj_set_style_bg_opa(dlg->panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->panel, panel_bg, LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->lbl_title = lv_label_create(dlg->panel);
    lv_obj_set_pos(dlg->lbl_title, 16, 8);
    lv_label_set_text(dlg->lbl_title, "OTA 升级");
    lv_label_set_long_mode(dlg->lbl_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(dlg->lbl_title, &lv_font_cn_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_title, text_col, LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->lbl_ver = lv_label_create(dlg->panel);
    lv_obj_set_pos(dlg->lbl_ver, 16, 44);
    lv_label_set_text(dlg->lbl_ver, "目标版本：--");
    lv_label_set_long_mode(dlg->lbl_ver, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(dlg->lbl_ver, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_ver, text_col, LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->lbl_status = lv_label_create(dlg->panel);
    lv_obj_set_pos(dlg->lbl_status, 16, 72);
    lv_label_set_text(dlg->lbl_status, "点击确认开始升级");
    lv_label_set_long_mode(dlg->lbl_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(dlg->lbl_status, 388);
    lv_obj_set_style_text_font(dlg->lbl_status, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_status, dark_mode ? lv_color_hex(0xbdbdbd) : lv_color_hex(0x5b7997),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->bar = lv_bar_create(dlg->panel);
    lv_obj_set_pos(dlg->bar, -4, 112);
    lv_obj_set_size(dlg->bar, 388, 18);
    lv_bar_set_range(dlg->bar, 0, 100);
    lv_bar_set_value(dlg->bar, 0, LV_ANIM_OFF);
    lv_obj_add_flag(dlg->bar, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_radius(dlg->bar, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dlg->bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->bar, lv_color_hex(0xe6e6e6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dlg->bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dlg->bar, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dlg->bar, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->bar, lv_color_hex(0x2195f6), LV_PART_INDICATOR | LV_STATE_DEFAULT);

    dlg->lbl_pct = lv_label_create(dlg->panel);
    lv_obj_set_pos(dlg->lbl_pct, -4, 134);
    lv_label_set_text(dlg->lbl_pct, "0%");
    lv_obj_add_flag(dlg->lbl_pct, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(dlg->lbl_pct, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_pct, text_col, LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->btn_cancel = lv_button_create(dlg->panel);
    lv_obj_set_pos(dlg->btn_cancel, 40, 150);
    lv_obj_set_size(dlg->btn_cancel, 150, 40);
    lv_obj_set_style_radius(dlg->btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dlg->btn_cancel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dlg->btn_cancel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->btn_cancel, dark_mode ? lv_color_hex(0x3a3a3a) : lv_color_hex(0xd9d9d9),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(dlg->btn_cancel, on_btn_cancel, LV_EVENT_CLICKED, dlg);

    dlg->lbl_cancel = lv_label_create(dlg->btn_cancel);
    lv_label_set_text(dlg->lbl_cancel, "取消");
    lv_obj_center(dlg->lbl_cancel);
    lv_obj_set_style_text_font(dlg->lbl_cancel, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_cancel, dark_mode ? lv_color_hex(0xffffff) : lv_color_hex(0x000000),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    dlg->btn_ok = lv_button_create(dlg->panel);
    lv_obj_set_pos(dlg->btn_ok, 230, 150);
    lv_obj_set_size(dlg->btn_ok, 150, 40);
    lv_obj_set_style_radius(dlg->btn_ok, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dlg->btn_ok, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dlg->btn_ok, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dlg->btn_ok, lv_color_hex(0x2195f6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(dlg->btn_ok, on_btn_ok, LV_EVENT_CLICKED, dlg);

    dlg->lbl_ok = lv_label_create(dlg->btn_ok);
    lv_label_set_text(dlg->lbl_ok, "确认");
    lv_obj_center(dlg->lbl_ok);
    lv_obj_set_style_text_font(dlg->lbl_ok, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dlg->lbl_ok, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ota_upgrade_dialog_bind_open_button(lv_obj_t *btn, bool dark_mode)
{
    ota_dlg_t *dlg = dlg_get(dark_mode);
    if (!dlg) return;
    if (!obj_ok(btn)) return;
    lv_obj_add_event_cb(btn, on_open_btn, LV_EVENT_CLICKED, dlg);
}

void ota_upgrade_dialog_show(bool dark_mode, const char *target_version)
{
    ota_dlg_t *dlg = dlg_get(dark_mode);
    if (!dlg) return;
    if (!obj_ok(dlg->modal)) return;

    dlg_timer_stop(dlg);

#if !defined(ESP_PLATFORM)
    dlg->sim_progress = 0;
    dlg->sim_upgrade_running = false;
    dlg->sim_check_running = true;
    dlg->sim_check_deadline_ms = lv_tick_get() + 800;
    if (dlg->sim_target_ver[0] == '\0') {
        dlg->sim_has_update = true;
    }
#endif

    /* Reset UI state */
    dlg_set_btn_enabled(dlg->btn_ok, false);
    dlg_set_btn_enabled(dlg->btn_cancel, true);
    if (obj_ok(dlg->lbl_cancel)) dlg_set_text(dlg->lbl_cancel, "取消");

    if (obj_ok(dlg->bar)) lv_obj_add_flag(dlg->bar, LV_OBJ_FLAG_HIDDEN);
    if (obj_ok(dlg->lbl_pct)) lv_obj_add_flag(dlg->lbl_pct, LV_OBJ_FLAG_HIDDEN);
    dlg_progress_set(dlg, 0);

    /* Version text */
    if (obj_ok(dlg->lbl_ver)) {
        char buf[64];
        const char *ver = (target_version && target_version[0]) ? target_version : "--";
#if !defined(ESP_PLATFORM)
        if (dlg->sim_target_ver[0]) {
            ver = dlg->sim_target_ver;
        }
#endif
        lv_snprintf(buf, sizeof(buf), "目标版本：%s", ver);
        dlg_set_text(dlg->lbl_ver, buf);
    }
    dlg_update_version_from_ota(dlg);

    dlg_set_text(dlg->lbl_status, "正在检查更新…");

    lv_obj_clear_flag(dlg->modal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dlg->modal);

    /* 从下向上弹出（只做显示动画；关闭仍为立即隐藏） */
    if (obj_ok(dlg->panel)) {
        lv_obj_update_layout(dlg->modal);
        lv_obj_update_layout(dlg->panel);

        const lv_coord_t pw = lv_obj_get_width(dlg->modal);
        const lv_coord_t ph = lv_obj_get_height(dlg->modal);
        const lv_coord_t w  = lv_obj_get_width(dlg->panel);
        const lv_coord_t h  = lv_obj_get_height(dlg->panel);

        const lv_coord_t target_x = (pw - w) / 2;
        const lv_coord_t target_y = (ph - h) / 2;
        const lv_coord_t start_y  = ph; /* panel 顶部在屏幕底部之外 */

        /* 取消历史动画，避免重复触发时“跳动” */
        lv_anim_del(dlg->panel, (lv_anim_exec_xcb_t)lv_obj_set_y);

        lv_obj_set_pos(dlg->panel, target_x, start_y);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, dlg->panel);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_values(&a, start_y, target_y);
        lv_anim_set_time(&a, 300);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }

#if defined(ESP_PLATFORM) && CONFIG_APP_OTA_ENABLE
    /* 打开弹窗即检查更新（不写 flash）
     * - 若已有“有新版本”的结论，为避免重复检查导致确认按钮不可用，优先使用缓存结论。 */
    if (!ota_update_is_running()) {
        const bool check_running = ota_update_check_is_running();
        const ota_update_check_result_t res = ota_update_get_check_result();

        if (!check_running && res == OTA_UPDATE_CHECK_HAS_UPDATE) {
            dlg_set_text(dlg->lbl_status, "发现新版本，是否升级？");
            dlg_set_btn_enabled(dlg->btn_ok, true);
        } else {
            dlg_set_text(dlg->lbl_status, "正在检查更新…");
            dlg_set_btn_enabled(dlg->btn_ok, false);

            if (!check_running) {
                (void)ota_update_check_start(CONFIG_APP_OTA_URL);
            }
        }
    }
#endif

    dlg_start_timer(dlg);
}

void ota_upgrade_dialog_hide(bool dark_mode)
{
    ota_dlg_t *dlg = dlg_get(dark_mode);
    if (!dlg) return;
    dlg_timer_stop(dlg);
    if (obj_ok(dlg->modal)) {
        lv_obj_add_flag(dlg->modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void auto_popup_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (s_auto_popup_done) {
        s_auto_popup_timer = NULL;
        lv_timer_del(t);
        return;
    }

#if defined(ESP_PLATFORM) && CONFIG_APP_OTA_ENABLE
    if (ota_update_is_running()) {
        s_auto_popup_timer = NULL;
        lv_timer_del(t);
        return;
    }

    if (ota_update_check_is_running()) {
        return; /* wait */
    }

    if (!ota_should_auto_popup()) {
        s_auto_popup_timer = NULL;
        lv_timer_del(t);
        return;
    }

    const ota_update_check_result_t res = ota_update_get_check_result();
    if (res == OTA_UPDATE_CHECK_HAS_UPDATE) {
        /* 优先弹出浅色；若未创建，则尝试深色 */
        if (obj_ok(s_dlg_light.modal)) {
            ota_upgrade_dialog_show(false, NULL);
            s_auto_popup_done = true;
        } else if (obj_ok(s_dlg_dark.modal)) {
            ota_upgrade_dialog_show(true, NULL);
            s_auto_popup_done = true;
        } else {
            return; /* dialog not ready yet */
        }
    } else if (res == OTA_UPDATE_CHECK_NO_UPDATE || res == OTA_UPDATE_CHECK_ERROR) {
        s_auto_popup_timer = NULL;
        lv_timer_del(t);
        return;
    } else {
        /* UNKNOWN: treat as "not ready yet" for a while */
        return;
    }
#else
    /* Simulator: no auto popup */
    s_auto_popup_timer = NULL;
    lv_timer_del(t);
    return;
#endif

    /* done */
    s_auto_popup_timer = NULL;
    lv_timer_del(t);
}

void ota_upgrade_dialog_arm_auto_popup(void)
{
    /* Only arm once per boot/IP-ready trigger */
    if (s_auto_popup_timer) {
        return;
    }
    s_auto_popup_done = false;
    s_auto_popup_timer = lv_timer_create(auto_popup_timer_cb, 500, NULL);
    lv_timer_ready(s_auto_popup_timer);
}
