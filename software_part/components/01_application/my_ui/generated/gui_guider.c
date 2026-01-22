/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "widgets_init.h"
#include "wifi_ui.h"
#include "splash_screen.h"
#include "esp_log.h"

void ui_init_style(lv_style_t * style)
{
    if (style->prop_cnt > 1)
        lv_style_reset(style);
    else
        lv_style_init(style);
}

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del)
{
    lv_obj_t * act_scr = lv_screen_active();
    ESP_LOGI("GUI", "ui_load_scr_animation: new_scr_del=%d, is_clean=%d, auto_del=%d, *new_scr=%p",
             new_scr_del, is_clean, auto_del, *new_scr);

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "gg_external_data.h"
    if(auto_del) {
        gg_edata_task_clear(act_scr);
    }
#endif
    if (auto_del && is_clean) {
        lv_obj_clean(act_scr);
    }
    if (new_scr_del) {
        ESP_LOGI("GUI", "ui_load_scr_animation: calling setup_scr()");
        setup_scr(ui);
        ESP_LOGI("GUI", "ui_load_scr_animation: after setup_scr(), *new_scr=%p", *new_scr);
    }
    /* 性能优化：禁用屏幕切换动画，避免LVGL内存碎片化导致看门狗超时 */
    if (*new_scr && lv_obj_is_valid(*new_scr)) {
        ESP_LOGI("GUI", "ui_load_scr_animation: calling lv_screen_load_anim with valid screen");
        lv_screen_load_anim(*new_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, auto_del);
    } else {
        ESP_LOGE("GUI", "ui_load_scr_animation: *new_scr is NULL or invalid!");
    }
    *old_scr_del = auto_del;
}

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, var);
    lv_anim_set_exec_cb(&anim, exec_cb);
    lv_anim_set_values(&anim, start_value, end_value);
    lv_anim_set_time(&anim, duration);
    lv_anim_set_delay(&anim, delay);
    lv_anim_set_path_cb(&anim, path_cb);
    lv_anim_set_repeat_count(&anim, repeat_cnt);
    lv_anim_set_repeat_delay(&anim, repeat_delay);
    lv_anim_set_playback_time(&anim, playback_time);
    lv_anim_set_playback_delay(&anim, playback_delay);
    if (start_cb) {
        lv_anim_set_start_cb(&anim, start_cb);
    }
    if (ready_cb) {
        lv_anim_set_completed_cb(&anim, ready_cb);
    }
    if (deleted_cb) {
        lv_anim_set_deleted_cb(&anim, deleted_cb);
    }
    lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{

    ui->screen_del = true;
    ui->screen_1_del = true;
    ui->screen_3_del = true;
    ui->screen_4_del = true;
    ui->screen_5_del = true;
    ui->screen_6_del = true;
    ui->screen_7_del = true;
    ui->MusicScreen_del = true;
    ui->MusicSpectrum_del = true;
    ui->MusicScreen_dark_del = true;
    ui->wifi_main_del = true;
    ui->wifi_main_dark_del = true;
    ui->weather_main_del = true;
    ui->calendar_main_del = true;
}

void setup_bottom_layer(void)
{
    lv_theme_apply(lv_layer_bottom());
}

void setup_ui(lv_ui *ui)
{
    ESP_LOGI("GUI", "setup_ui: start");
    setup_bottom_layer();
    ESP_LOGI("GUI", "setup_ui: after setup_bottom_layer");
    init_scr_del_flag(ui);
    ESP_LOGI("GUI", "setup_ui: after init_scr_del_flag");
    init_keyboard(ui);
    ESP_LOGI("GUI", "setup_ui: after init_keyboard");

    /* 提前初始化 Wi‑Fi UI 模块以注册网络事件（获取 IP 后会启动 SNTP、触发天气拉取，
     * 即使尚未进入 Wi‑Fi 页面也不影响）。该函数内部已做幂等/空指针保护。 */
    wifi_ui_init(ui);
    ESP_LOGI("GUI", "setup_ui: after wifi_ui_init");

    /* 显示启动屏（动画 + 进度条），并自动尝试一次连接"已保存网络(MRU)"。
     * - 若成功，got_ip 回调会拉起 SNTP/天气；
     * - 无论成功失败，尝试完成后都会切回首页，用户可手动操作。*/
    ESP_LOGI("GUI", "setup_ui: calling splash_show_and_autoconnect");
    splash_show_and_autoconnect(ui);
    ESP_LOGI("GUI", "setup_ui: done");
}

void video_play(lv_ui *ui)
{

}

void init_keyboard(lv_ui *ui)
{
    ui->g_kb_top_layer = lv_keyboard_create(lv_layer_top());
    lv_obj_add_event_cb(ui->g_kb_top_layer, kb_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(ui->g_kb_top_layer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(ui->g_kb_top_layer, &lv_font_SourceHanSerifSC_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
}
