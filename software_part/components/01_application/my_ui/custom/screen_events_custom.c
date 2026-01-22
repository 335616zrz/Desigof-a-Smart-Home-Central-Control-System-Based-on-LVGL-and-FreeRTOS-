// custom/screen_events_custom.c
#include "lvgl.h"
#include "time_ui.h"

static void timeui_guard_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *root = lv_event_get_target(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START || code == LV_EVENT_DELETE) {
        // 只在这个 root 正好是当前绑定 label 所在的屏时才解绑
        time_ui_detach_if_screen(root);
    }
}

void screen_register_timeui_guard(lv_obj_t *root)
{
    if (!root) return;
    lv_obj_add_event_cb(root, timeui_guard_cb, LV_EVENT_ALL, NULL);
}

