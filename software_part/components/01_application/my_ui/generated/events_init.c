/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lvgl.h"
#include "gui_guider.h"          // 声明 guider_ui 和各控件句柄
#include "wifi_ui.h"
#include "custom.h"
#include "widgets_init.h"   // 声明 screen_7_lang_btn_event_handler 等自定义回调
#include "music_ui.h"
#include "esp_log.h"
#include "custom.h"        // env_mode_* for 室内/室外切换
#include "weather_persist.h"
#include "chatbot_client.h"
#include "chat_ui.h"
#include "weather_city.h"
#include "weather_service_client.h"
#include "weather_persist.h"
#include "time_ui.h"
#include "tts_player.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

/* 小工具函数：启用/禁用对象（并顺带调节透明度） */
static void set_enabled(lv_obj_t *obj, bool enabled)
{
    if(!obj) return;
    if(enabled) {
        lv_obj_remove_state(obj, LV_STATE_DISABLED);
        lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);   // 100% 可见
    } else {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        lv_obj_set_style_opa(obj, LV_OPA_50, 0);      // 灰掉一点
    }
}

/* 递归启用/禁用（List 的子项也一起处理） */
static void set_enabled_deep(lv_obj_t *parent, bool enabled)
{
    if(!parent) return;
    set_enabled(parent, enabled);
    uint32_t n = lv_obj_get_child_cnt(parent);
    for(uint32_t i = 0; i < n; i++) {
        set_enabled_deep(lv_obj_get_child(parent, i), enabled);
    }
}

/* ======================== Screen ======================== */

static void screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {
        /* 进入首页时，若首页四项还是占位值，则立即触发一次天气拉取，
         * 确保用户切换到首页后看到真实内容（不必等到5分钟周期）。*/
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        if (ui) {
            const char *t5 = ui->screen_label_5 ? lv_label_get_text(ui->screen_label_5) : NULL;
            const char *t9 = ui->screen_label_9 ? lv_label_get_text(ui->screen_label_9) : NULL;
            const char *t11= ui->screen_label_11? lv_label_get_text(ui->screen_label_11): NULL;
            const char *t14= ui->screen_label_14? lv_label_get_text(ui->screen_label_14): NULL;
            bool empty = false;
            if (!t5 || strcmp(t5, "--") == 0) empty = true;
            if (!t9 || strcmp(t9, "--") == 0) empty = true;
            if (!t11|| strcmp(t11, "—— ——") == 0) empty = true;
            if (!t14|| strcmp(t14, "—") == 0) empty = true;
            if (empty) {
                weather_bind_ui(ui);
                uint32_t prov=0,pref=0,county=0;
                (void)weather_persist_load(&prov,&pref,&county);
                uint32_t adcode = county ? county : (pref ? pref : prov);
                if (!adcode) {
                    adcode = weather_city_get_selected_county_code();
                    if (!adcode) adcode = weather_city_get_selected_pref_code();
                    if (!adcode) adcode = weather_city_get_selected_province_code();
                }
                if (adcode) {
                    weather_set_last_adcode(adcode);
                    (void)weather_update_for_adcode(adcode);
                }
            }
            /* 每次首页加载时，同步一次“室内/室外”文本到当前模式，避免
             * 经过启动页或主题切换后仍停留在默认的“室外”字样。*/
            if (ui->screen_label_12 && lv_obj_is_valid(ui->screen_label_12)) {
                const char *txt = env_mode_is_indoor() ? "室\n内" : "室\n外";
                lv_label_set_text(ui->screen_label_12, txt);
            }
        }
        break;
    }
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_TOP:
        {
            lv_indev_wait_release(lv_indev_active());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.screen_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_OVER_TOP, 100, 100, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void screen_arc_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        // Backlight_Brightness Event
        break;
    }
    default:
        break;
    }
}

/* 首页“室内/室外”切换：点击 screen_label_12 在两种模式间切换，
 * - 室外：使用天气服务的数据刷新 screen_label_5 / 9；
 * - 室内：使用 SHT40 数据刷新 screen_label_5 / 9（当前值在 env_sht40_task 中更新）。*/
static void screen_label_12_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *lbl = lv_event_get_target(e);
    if (!lbl) return;

    bool was_indoor = env_mode_is_indoor();
    bool indoor = !was_indoor;
    env_mode_set_indoor(indoor);
    (void)weather_persist_save_env_mode(indoor);

    /* 切回“室外”时，立即用最近一次天气数据覆盖首页温湿度数字，
     * 避免标签继续停留在室内 SHT40 数值上，需要等下一次天气拉取才更新。*/
    if (!indoor) {
        weather_home_apply_from_cache();
    }

    if (indoor) {
        /* 室内 */
        lv_label_set_text(lbl, "室\n内");
    } else {
        /* 室外 */
        lv_label_set_text(lbl, "室\n外");
    }
}

static void screen_sw_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_t * status_obj = lv_event_get_target(e);
        (void)status_obj; // 当前未使用
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, guider_ui.screen_1_del, &guider_ui.screen_del, setup_scr_home_screen_dark, LV_SCR_LOAD_ANIM_NONE, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen, screen_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_arc_1, screen_arc_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_sw_1, screen_sw_1_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_label_12) {
        lv_obj_add_event_cb(ui->screen_label_12, screen_label_12_event_handler, LV_EVENT_ALL, ui);
        /* 首次构建首页时亦同步一次“室内/室外”文本，防止默认模板值与
         * 已保存模式不一致（例如上次停留在室内，但模板仍为“室外”）。*/
        const char *txt = env_mode_is_indoor() ? "室\n内" : "室\n外";
        lv_label_set_text(ui->screen_label_12, txt);
    }
}

/* ======================== Screen 1 ======================== */

static void screen_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {
        /* 深色首页同样做一次即时填充检查 */
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        if (ui) {
            const char *t5 = ui->screen_1_label_5 ? lv_label_get_text(ui->screen_1_label_5) : NULL;
            const char *t9 = ui->screen_1_label_9 ? lv_label_get_text(ui->screen_1_label_9) : NULL;
            const char *t11= ui->screen_1_label_11? lv_label_get_text(ui->screen_1_label_11): NULL;
            const char *t14= ui->screen_1_label_14? lv_label_get_text(ui->screen_1_label_14): NULL;
            bool empty = false;
            if (!t5 || strcmp(t5, "--") == 0) empty = true;
            if (!t9 || strcmp(t9, "--") == 0) empty = true;
            if (!t11|| strcmp(t11, "—— ——") == 0) empty = true;
            if (!t14|| strcmp(t14, "—") == 0) empty = true;
            if (empty) {
                weather_bind_ui(ui);
                uint32_t prov=0,pref=0,county=0;
                (void)weather_persist_load(&prov,&pref,&county);
                uint32_t adcode = county ? county : (pref ? pref : prov);
                if (!adcode) {
                    adcode = weather_city_get_selected_county_code();
                    if (!adcode) adcode = weather_city_get_selected_pref_code();
                    if (!adcode) adcode = weather_city_get_selected_province_code();
                }
                if (adcode) {
                    weather_set_last_adcode(adcode);
                    (void)weather_update_for_adcode(adcode);
                }
            }
            /* 每次进入深色首页时，同步一次模式标签文本，确保与当前
             * env_mode 保持一致（避免在浅色页切换模式后，深色页仍显示旧文案）。*/
            if (ui->screen_1_label_12 && lv_obj_is_valid(ui->screen_1_label_12)) {
                const char *txt = env_mode_is_indoor() ? "室\n内" : "室\n外";
                lv_label_set_text(ui->screen_1_label_12, txt);
            }
        }
        break;
    }
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_TOP:
        {
            lv_indev_wait_release(lv_indev_active());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del, &guider_ui.screen_1_del, setup_scr_app_screen_dark, LV_SCR_LOAD_ANIM_OVER_TOP, 100, 100, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void screen_1_arc_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        // Backlight_Brightness Event
        break;
    }
    default:
        break;
    }
}

static void screen_1_sw_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_t * status_obj = lv_event_get_target(e);
        (void)status_obj; // 当前未使用
        ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_1_del, setup_scr_home_screen, LV_SCR_LOAD_ANIM_NONE, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_1 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_1, screen_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_1_arc_1, screen_1_arc_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_1_sw_1, screen_1_sw_1_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_1_label_12) {
        lv_obj_add_event_cb(ui->screen_1_label_12, screen_label_12_event_handler, LV_EVENT_ALL, ui);
        /* 深色首页在创建时同步一次“室内/室外”文字到当前模式 */
        const char *txt = env_mode_is_indoor() ? "室\n内" : "室\n外";
        lv_label_set_text(ui->screen_1_label_12, txt);
    }
}

/* ======================== Screen 3 ======================== */

static void screen_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_active());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_3_del, setup_scr_home_screen, LV_SCR_LOAD_ANIM_OVER_RIGHT, 100, 100, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void screen_3_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.wifi_main, guider_ui.wifi_main_del, &guider_ui.screen_3_del, setup_scr_wifi_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_img_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.wifi_main, guider_ui.wifi_main_del, &guider_ui.screen_3_del, setup_scr_wifi_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.MusicScreen, guider_ui.MusicScreen_del, &guider_ui.screen_3_del, setup_scr_music_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_img_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.MusicScreen, guider_ui.MusicScreen_del, &guider_ui.screen_3_del, setup_scr_music_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_btn_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_7, guider_ui.screen_7_del, &guider_ui.screen_3_del, setup_scr_screen_7, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_img_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_7, guider_ui.screen_7_del, &guider_ui.screen_3_del, setup_scr_screen_7, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

/* Weather entry (App screen button 4) */
static void screen_3_btn_4_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.weather_main, guider_ui.weather_main_del,
                              &guider_ui.screen_3_del, setup_scr_weather_screen,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

static void screen_3_img_4_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.weather_main, guider_ui.weather_main_del,
                              &guider_ui.screen_3_del, setup_scr_weather_screen,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

/* Calendar entry (App screen button 5) */
static void screen_3_btn_5_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.calendar_main, guider_ui.calendar_main_del,
                              &guider_ui.screen_3_del, setup_scr_calendar_screen,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

static void screen_3_img_5_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.calendar_main, guider_ui.calendar_main_del,
                              &guider_ui.screen_3_del, setup_scr_calendar_screen,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

static void screen_3_btn_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_5, guider_ui.screen_5_del, &guider_ui.screen_3_del, setup_scr_author_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_img_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_5, guider_ui.screen_5_del, &guider_ui.screen_3_del, setup_scr_author_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

/* 切换音乐数据源：HTTPS 索引 <-> SD 卡文件 */
static void screen_3_btn_7_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        music_ui_toggle_source();
    }
}

static void screen_3_img_7_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        music_ui_toggle_source();
    }
}

void events_init_screen_3 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_3, screen_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_1, screen_3_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_1, screen_3_img_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_2, screen_3_btn_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_2, screen_3_img_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_3, screen_3_btn_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_3, screen_3_img_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_4, screen_3_btn_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_4, screen_3_img_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_5, screen_3_btn_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_5, screen_3_img_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_6, screen_3_btn_6_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_6, screen_3_img_6_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_7, screen_3_btn_7_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_img_7, screen_3_img_7_event_handler, LV_EVENT_ALL, ui);
}

/* ======================== Screen 4 ======================== */

static void screen_4_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_active());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, guider_ui.screen_1_del, &guider_ui.screen_4_del, setup_scr_home_screen_dark, LV_SCR_LOAD_ANIM_OVER_RIGHT, 100, 100, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

/* 深色 App 中的“切换”按钮：与亮色版一致，切换音乐来源 HTTP<->SD */
static void screen_4_btn_7_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        music_ui_toggle_source();
    }
}

static void screen_4_img_7_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        music_ui_toggle_source();
    }
}

static void screen_4_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.wifi_main_dark, guider_ui.wifi_main_dark_del, &guider_ui.screen_4_del, setup_scr_wifi_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_img_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.wifi_main_dark, guider_ui.wifi_main_dark_del, &guider_ui.screen_4_del, setup_scr_wifi_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.MusicScreen_dark, guider_ui.MusicScreen_dark_del, &guider_ui.screen_4_del, setup_scr_music_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_btn_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 深色 App：进入深色 GPT 聊天屏 */
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_7, guider_ui.screen_7_del,
                              &guider_ui.screen_4_del, setup_scr_screen_7_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_img_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 深色 App：图标点击同样进入深色 GPT 聊天屏 */
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_7, guider_ui.screen_7_del,
                              &guider_ui.screen_4_del, setup_scr_screen_7_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

/* Dark App: Weather entry (button 4) */
static void screen_4_btn_4_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.weather_main, guider_ui.weather_main_del,
                              &guider_ui.screen_4_del, setup_scr_weather_screen_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

/* Dark App: Calendar entry (button 5) */
static void screen_4_btn_5_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.calendar_main, guider_ui.calendar_main_del,
                              &guider_ui.screen_4_del, setup_scr_calendar_screen_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

static void screen_4_img_5_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.calendar_main, guider_ui.calendar_main_del,
                              &guider_ui.screen_4_del, setup_scr_calendar_screen_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}

static void screen_4_img_4_event_handler (lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.weather_main, guider_ui.weather_main_del,
                              &guider_ui.screen_4_del, setup_scr_weather_screen_dark,
                              LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
    }
}
static void screen_4_img_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.MusicScreen_dark, guider_ui.MusicScreen_dark_del, &guider_ui.screen_4_del, setup_scr_music_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_btn_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_6, guider_ui.screen_6_del, &guider_ui.screen_4_del, setup_scr_author_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_4_img_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_6, guider_ui.screen_6_del, &guider_ui.screen_4_del, setup_scr_author_screen_dark, LV_SCR_LOAD_ANIM_MOVE_TOP, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_4 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_4, screen_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_btn_1, screen_4_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_img_1, screen_4_img_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_btn_2, screen_4_btn_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_img_2, screen_4_img_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_btn_3, screen_4_btn_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_img_3, screen_4_img_3_event_handler, LV_EVENT_ALL, ui);
    /* Dark App: Weather entry (button 4 and its image) */
    if (ui->screen_4_btn_4) lv_obj_add_event_cb(ui->screen_4_btn_4, screen_4_btn_4_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_4_img_4) lv_obj_add_event_cb(ui->screen_4_img_4, screen_4_img_4_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_4_btn_5) lv_obj_add_event_cb(ui->screen_4_btn_5, screen_4_btn_5_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_4_img_5) lv_obj_add_event_cb(ui->screen_4_img_5, screen_4_img_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_btn_6, screen_4_btn_6_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_4_img_6, screen_4_img_6_event_handler, LV_EVENT_ALL, ui);
    /* 深色 App：切换音乐数据源（按钮 7 与其图标） */
    if (ui->screen_4_btn_7) lv_obj_add_event_cb(ui->screen_4_btn_7, screen_4_btn_7_event_handler, LV_EVENT_ALL, ui);
    if (ui->screen_4_img_7) lv_obj_add_event_cb(ui->screen_4_img_7, screen_4_img_7_event_handler, LV_EVENT_ALL, ui);
}

/* ======================== Screen 5 ======================== */

static void screen_5_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.screen_5_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_5 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_5_btn_1, screen_5_btn_1_event_handler, LV_EVENT_ALL, ui);
}

/* ======================== Screen 6 ======================== */

static void screen_6_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del, &guider_ui.screen_6_del, setup_scr_app_screen_dark, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_6 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_6_btn_1, screen_6_btn_1_event_handler, LV_EVENT_ALL, ui);
}

/* ======================== Weather ======================== */
static void weather_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del,
                              &guider_ui.weather_main_del, setup_scr_app_screen,
                              LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
    }
}

/* Weather (dark) */
static void weather_back_event_handler_dark(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del,
                              &guider_ui.weather_main_del, setup_scr_app_screen_dark,
                              LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
    }
}

/* 打开/关闭省市县选择 */
static void weather_city_open_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        if (ui && ui->weather_city_modal) {
            lv_obj_clear_flag(ui->weather_city_modal, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void weather_city_cancel_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        if (ui && ui->weather_city_modal) {
            lv_obj_add_flag(ui->weather_city_modal, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void weather_city_ok_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        if (!ui) return;
        char prov[64] = {0}, pref[64] = {0}, county[64] = {0}, label[128] = {0};
        if (ui->weather_city_dd_prov) lv_dropdown_get_selected_str(ui->weather_city_dd_prov, prov, sizeof(prov));
        if (ui->weather_city_dd_pref) lv_dropdown_get_selected_str(ui->weather_city_dd_pref, pref, sizeof(pref));
        if (ui->weather_city_dd_county) lv_dropdown_get_selected_str(ui->weather_city_dd_county, county, sizeof(county));
        lv_snprintf(label, sizeof(label), "城市：%s/%s/%s",
                    prov[0]?prov:"—", pref[0]?pref:"—", county[0]?county:"—");
        if (ui->weather_main_city) lv_label_set_text(ui->weather_main_city, label);
        if (ui->weather_city_modal) {
            lv_obj_add_flag(ui->weather_city_modal, LV_OBJ_FLAG_HIDDEN);
        }
        // 选择持久化并触发根据 adcode 拉取天气
        uint32_t prov_code=0, pref_code=0, county_code=0;
        weather_city_get_selected_codes(&prov_code, &pref_code, &county_code);
        (void)weather_persist_save(prov_code, pref_code, county_code);

        uint32_t adcode = county_code;
        if (!adcode) {
            // 若未选到区县，退而求其次用市级/省级 code
            adcode = pref_code;
            if (!adcode) adcode = prov_code;
        }
        weather_bind_ui(ui);
        weather_set_last_adcode(adcode);
        (void)weather_update_for_adcode(adcode);
    }
}

void events_init_weather_main(lv_ui *ui)
{
    if (!ui) return;
    // 初始化城市三联动（基于嵌入 CSV）
    weather_city_setup(ui);
    weather_bind_ui(ui);
    // 读取上次选择并应用
    uint32_t prov=0,pref=0,county=0;
    (void)weather_persist_load(&prov,&pref,&county);
    if (prov || pref || county) {
        weather_city_select_codes(ui, prov, pref, county);
        // 更新顶栏城市标签
        char pv[64]={0}, pf[64]={0}, ct[64]={0}, label[160]={0};
        if (ui->weather_city_dd_prov) lv_dropdown_get_selected_str(ui->weather_city_dd_prov, pv, sizeof(pv));
        if (ui->weather_city_dd_pref) lv_dropdown_get_selected_str(ui->weather_city_dd_pref, pf, sizeof(pf));
        if (ui->weather_city_dd_county) lv_dropdown_get_selected_str(ui->weather_city_dd_county, ct, sizeof(ct));
        lv_snprintf(label, sizeof(label), "城市：%s/%s/%s", pv[0]?pv:"—", pf[0]?pf:"—", ct[0]?ct:"—");
        if (ui->weather_main_city) lv_label_set_text(ui->weather_main_city, label);
    }
    // 首次进入即按“上次选择的 adcode”拉取（优先县级）
    uint32_t adcode = county ? county : (pref ? pref : prov);
    if (!adcode) {
        // 兜底：按当前默认选择（初始化后会选中第 1 项）
        adcode = weather_city_get_selected_county_code();
        if (!adcode) { adcode = weather_city_get_selected_pref_code(); }
        if (!adcode) { adcode = weather_city_get_selected_province_code(); }
    }
    if (adcode) {
        weather_set_last_adcode(adcode);
        (void)weather_update_for_adcode(adcode);
        // 后台每 5 分钟刷新一次
        (void)weather_periodic_start(5 * 60 * 1000);
    }
    if (ui->weather_main_btn_back)
        lv_obj_add_event_cb(ui->weather_main_btn_back, weather_back_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_main_img_back)
        lv_obj_add_event_cb(ui->weather_main_img_back, weather_back_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn)
        lv_obj_add_event_cb(ui->weather_city_btn, weather_city_open_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn_cancel)
        lv_obj_add_event_cb(ui->weather_city_btn_cancel, weather_city_cancel_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn_ok)
        lv_obj_add_event_cb(ui->weather_city_btn_ok, weather_city_ok_event_handler, LV_EVENT_ALL, ui);
}

void events_init_weather_main_dark(lv_ui *ui)
{
    if (!ui) return;
    // 初始化城市三联动（基于嵌入 CSV）
    weather_city_setup(ui);
    weather_bind_ui(ui);
    // 读取上次选择并应用
    uint32_t prov=0,pref=0,county=0;
    (void)weather_persist_load(&prov,&pref,&county);
    if (prov || pref || county) {
        weather_city_select_codes(ui, prov, pref, county);
        char pv[64]={0}, pf[64]={0}, ct[64]={0}, label[160]={0};
        if (ui->weather_city_dd_prov) lv_dropdown_get_selected_str(ui->weather_city_dd_prov, pv, sizeof(pv));
        if (ui->weather_city_dd_pref) lv_dropdown_get_selected_str(ui->weather_city_dd_pref, pf, sizeof(pf));
        if (ui->weather_city_dd_county) lv_dropdown_get_selected_str(ui->weather_city_dd_county, ct, sizeof(ct));
        lv_snprintf(label, sizeof(label), "城市：%s/%s/%s", pv[0]?pv:"—", pf[0]?pf:"—", ct[0]?ct:"—");
        if (ui->weather_main_city) lv_label_set_text(ui->weather_main_city, label);
    }
    uint32_t adcode = county ? county : (pref ? pref : prov);
    if (!adcode) {
        adcode = weather_city_get_selected_county_code();
        if (!adcode) { adcode = weather_city_get_selected_pref_code(); }
        if (!adcode) { adcode = weather_city_get_selected_province_code(); }
    }
    if (adcode) {
        weather_set_last_adcode(adcode);
        (void)weather_update_for_adcode(adcode);
        (void)weather_periodic_start(5 * 60 * 1000);
    }
    if (ui->weather_main_btn_back)
        lv_obj_add_event_cb(ui->weather_main_btn_back, weather_back_event_handler_dark, LV_EVENT_ALL, ui);
    if (ui->weather_main_img_back)
        lv_obj_add_event_cb(ui->weather_main_img_back, weather_back_event_handler_dark, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn)
        lv_obj_add_event_cb(ui->weather_city_btn, weather_city_open_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn_cancel)
        lv_obj_add_event_cb(ui->weather_city_btn_cancel, weather_city_cancel_event_handler, LV_EVENT_ALL, ui);
    if (ui->weather_city_btn_ok)
        lv_obj_add_event_cb(ui->weather_city_btn_ok, weather_city_ok_event_handler, LV_EVENT_ALL, ui);
}

/* ======================== Calendar ======================== */
static void calendar_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del,
                              &guider_ui.calendar_main_del, setup_scr_app_screen,
                              LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
    }
}

void events_init_calendar_main(lv_ui *ui)
{
    if (!ui) return;
    if (ui->calendar_btn_back)
        lv_obj_add_event_cb(ui->calendar_btn_back, calendar_back_event_handler, LV_EVENT_ALL, ui);
    if (ui->calendar_img_back)
        lv_obj_add_event_cb(ui->calendar_img_back, calendar_back_event_handler, LV_EVENT_ALL, ui);
}

/* Calendar (dark): 从日历页返回深色 App Screen */
static void calendar_back_event_handler_dark(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del,
                              &guider_ui.calendar_main_del, setup_scr_app_screen_dark,
                              LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
    }
}

void events_init_calendar_main_dark(lv_ui *ui)
{
    if (!ui) return;
    if (ui->calendar_btn_back)
        lv_obj_add_event_cb(ui->calendar_btn_back, calendar_back_event_handler_dark, LV_EVENT_ALL, ui);
    if (ui->calendar_img_back)
        lv_obj_add_event_cb(ui->calendar_img_back, calendar_back_event_handler_dark, LV_EVENT_ALL, ui);
}

/* ======================== Screen 7 ======================== */

/* GPT 输入模式：false=文本输入，true=语音输入（按住说话） */
static bool s_gpt_voice_mode = false;

static void gpt_apply_input_mode(lv_ui *ui, bool voice_mode)
{
    if (!ui) return;

    s_gpt_voice_mode = voice_mode;

    /* 1) 切换按钮文案：语音模式下显示“键盘”，文本模式下显示“语音” */
    if (ui->screen_7_mode_btn_label && lv_obj_is_valid(ui->screen_7_mode_btn_label)) {
        lv_label_set_text(ui->screen_7_mode_btn_label, voice_mode ? "键盘" : "语音");
    }

    /* 2) 控件显隐 */
    if (ui->screen_7_voice_btn && lv_obj_is_valid(ui->screen_7_voice_btn)) {
        if (voice_mode) lv_obj_clear_flag(ui->screen_7_voice_btn, LV_OBJ_FLAG_HIDDEN);
        else            lv_obj_add_flag  (ui->screen_7_voice_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui->screen_7_input_ta && lv_obj_is_valid(ui->screen_7_input_ta)) {
        if (voice_mode) lv_obj_add_flag  (ui->screen_7_input_ta, LV_OBJ_FLAG_HIDDEN);
        else            lv_obj_clear_flag(ui->screen_7_input_ta, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui->screen_7_send_btn && lv_obj_is_valid(ui->screen_7_send_btn)) {
        if (voice_mode) lv_obj_add_flag  (ui->screen_7_send_btn, LV_OBJ_FLAG_HIDDEN);
        else            lv_obj_clear_flag(ui->screen_7_send_btn, LV_OBJ_FLAG_HIDDEN);
    }

#if LV_USE_KEYBOARD
    /* 语音模式下隐藏屏内键盘，并清理输入框焦点，避免键盘悬空显示 */
    if (voice_mode) {
        if (ui->screen_7_kb && lv_obj_is_valid(ui->screen_7_kb)) {
            lv_obj_add_flag(ui->screen_7_kb, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui->screen_7_input_ta && lv_obj_is_valid(ui->screen_7_input_ta)) {
            lv_obj_clear_state(ui->screen_7_input_ta, LV_STATE_FOCUSED);
            lv_indev_reset(NULL, ui->screen_7_input_ta);
        }
    }
#endif

#if LV_USE_IME_PINYIN
    /* 语音模式下也隐藏候选栏，避免残留 UI */
    if (ui->screen_7_ime && lv_obj_is_valid(ui->screen_7_ime)) {
        lv_obj_t *cand = lv_ime_pinyin_get_cand_panel(ui->screen_7_ime);
        if (cand && lv_obj_is_valid(cand)) {
            if (voice_mode) {
                lv_obj_add_flag(cand, LV_OBJ_FLAG_HIDDEN);
            } else {
                /* 恢复候选栏显示策略：
                 * - 若当前处于英文直输（按钮文案为 "En"），候选栏应保持隐藏；
                 * - 否则默认显示（中文拼音）。*/
                bool hide = false;
                const char *lang_txt = NULL;
                if (ui->screen_7_kb_lang_btn_label && lv_obj_is_valid(ui->screen_7_kb_lang_btn_label)) {
                    lang_txt = lv_label_get_text(ui->screen_7_kb_lang_btn_label);
                } else if (ui->screen_7_lang_btn_label && lv_obj_is_valid(ui->screen_7_lang_btn_label)) {
                    lang_txt = lv_label_get_text(ui->screen_7_lang_btn_label);
                }
                if (lang_txt && strcmp(lang_txt, "En") == 0) hide = true;
                if (hide) lv_obj_add_flag(cand, LV_OBJ_FLAG_HIDDEN);
                else      lv_obj_clear_flag(cand, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
#endif

    /* 3) PTT/ASR：
     * - 语音模式：启用 ASR + PTT（按住说话/松开发送）
     * - 文本模式：确保 PTT 松开并关闭 ASR */
    AppASR_PTT_SetPressed(false);
    if (ui->screen_7_voice_btn_label && lv_obj_is_valid(ui->screen_7_voice_btn_label)) {
        lv_label_set_text(ui->screen_7_voice_btn_label, "按住说话");
    }
    AppASR_SetPTTMode(voice_mode);
    AppASR_SetEnabled(voice_mode);
}

static void screen_7_mode_btn_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    gpt_apply_input_mode(ui, !s_gpt_voice_mode);
}

static void screen_7_voice_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);

    switch (code) {
    case LV_EVENT_PRESSED:
        /* Barge-in: if TTS is playing, stop it immediately so ASR can start without waiting. */
        if (tts_player_is_active()) {
            tts_player_stop_now();
            (void)chatbot_abort(); // best-effort; may fail if WS not connected
        }
        if (ui && ui->screen_7_voice_btn_label && lv_obj_is_valid(ui->screen_7_voice_btn_label)) {
            lv_label_set_text(ui->screen_7_voice_btn_label, "松开发送");
        }
        AppASR_PTT_SetPressed(true);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
        if (ui && ui->screen_7_voice_btn_label && lv_obj_is_valid(ui->screen_7_voice_btn_label)) {
            lv_label_set_text(ui->screen_7_voice_btn_label, "按住说话");
        }
        AppASR_PTT_SetPressed(false);
        break;
    default:
        break;
    }
}

static void screen_7_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 离开 GPT 界面时禁用 ASR */
        AppAudio_OnLeaveGPTScreen();
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.screen_7_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

/* ===== GPT 聊天发送实现 ===== */
static void screen_7_send_now(lv_ui *ui)
{
    if (!ui || !ui->screen_7_input_ta) return;
    const char *text = lv_textarea_get_text(ui->screen_7_input_ta);
    if (!text || text[0] == '\0') return;
    /* 注意：先发送再清空输入框。lv_textarea_get_text() 返回内部缓冲指针，
     * 若先清空则会把传给 chatbot_send_text() 的内容变成空字符串。*/

    /* 先把用户消息追加到聊天区域（右侧气泡） */
    chat_ui_add_user(text);

    /* 异步发送到 Chatbot 服务器（可在菜单使能后生效） */
#if CONFIG_MY_UI_CHATBOT_ENABLE
    chatbot_send_text(text);
#else
    (void)chatbot_send_text; // 未启用时避免未使用警告
#endif
    /* 发送触发后再清空输入框 */
    lv_textarea_set_text(ui->screen_7_input_ta, "");
}

static void screen_7_send_btn_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        screen_7_send_now(ui);
    }
}

static void screen_7_kb_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_READY) return;
    lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if (!kb || !ui) return;
    lv_obj_t *ta = lv_keyboard_get_textarea(kb);
    if (ta == ui->screen_7_input_ta) {
        screen_7_send_now(ui);
    }
}

static void screen_7_input_ta_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);
        screen_7_send_now(ui);
    }
}

void events_init_screen_7 (lv_ui *ui)
{
    chat_ui_set_active(ui, false /* light */);
    lv_obj_add_event_cb(ui->screen_7_btn_1, screen_7_btn_1_event_handler, LV_EVENT_ALL, ui);
    /* 让左上角返回图标也可点击返回，和按钮保持一致（与天气页一致的 UX） */
    if (ui->screen_7_img_1) {
        lv_obj_add_event_cb(ui->screen_7_img_1, screen_7_btn_1_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_voice_btn) {
        lv_obj_add_event_cb(ui->screen_7_voice_btn, screen_7_voice_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_send_btn) {
        lv_obj_add_event_cb(ui->screen_7_send_btn, screen_7_send_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_mode_btn) {
        lv_obj_add_event_cb(ui->screen_7_mode_btn, screen_7_mode_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_lang_btn) {
        lv_obj_add_event_cb(ui->screen_7_lang_btn, screen_7_lang_btn_event_handler, LV_EVENT_CLICKED, ui);
    }
    if (ui->g_kb_top_layer) {
        lv_obj_add_event_cb(ui->g_kb_top_layer, screen_7_kb_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_input_ta) {
        lv_obj_add_event_cb(ui->screen_7_input_ta, screen_7_input_ta_event_handler, LV_EVENT_ALL, ui);
    }

    /* 默认以“上次的输入模式”恢复界面；首次进入默认为文本输入 */
    gpt_apply_input_mode(ui, s_gpt_voice_mode);
}

/* Screen 7 (dark GPT) */
static void screen_7_btn_1_dark_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 离开 GPT 界面时禁用 ASR */
        AppAudio_OnLeaveGPTScreen();
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del,
                              &guider_ui.screen_7_del, setup_scr_app_screen_dark,
                              LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_7_dark (lv_ui *ui)
{
    if (!ui) return;
    chat_ui_set_active(ui, true /* dark */);
    lv_obj_add_event_cb(ui->screen_7_btn_1, screen_7_btn_1_dark_event_handler, LV_EVENT_ALL, ui);
    /* 让左上角返回图标同样返回到深色 App Screen */
    if (ui->screen_7_img_1) {
        lv_obj_add_event_cb(ui->screen_7_img_1, screen_7_btn_1_dark_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_voice_btn) {
        lv_obj_add_event_cb(ui->screen_7_voice_btn, screen_7_voice_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_send_btn) {
        lv_obj_add_event_cb(ui->screen_7_send_btn, screen_7_send_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_mode_btn) {
        lv_obj_add_event_cb(ui->screen_7_mode_btn, screen_7_mode_btn_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_lang_btn) {
        lv_obj_add_event_cb(ui->screen_7_lang_btn, screen_7_lang_btn_event_handler, LV_EVENT_CLICKED, ui);
    }
    if (ui->g_kb_top_layer) {
        lv_obj_add_event_cb(ui->g_kb_top_layer, screen_7_kb_event_handler, LV_EVENT_ALL, ui);
    }
    if (ui->screen_7_input_ta) {
        lv_obj_add_event_cb(ui->screen_7_input_ta, screen_7_input_ta_event_handler, LV_EVENT_ALL, ui);
    }

    /* 默认以“上次的输入模式”恢复界面；首次进入默认为文本输入 */
    gpt_apply_input_mode(ui, s_gpt_voice_mode);
}

/* ======================== Screen 8 ======================== */

static void MusicScreen_root_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) {
            ESP_LOGW("MusicScreen", "Root gesture without indev");
            return;
        }
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        int cur_idx = music_ui_current_index();
        ESP_LOGD("MusicScreen", "Root gesture dir=%d idx=%d (spectrum open disabled on root)", (int)dir, cur_idx);
        /* 保留根区域其它手势的可能性，但不再从根区域左滑打开频谱。
         * 需求：只允许在 MusicScreen_music_top 的左滑进入频谱屏幕。
         */
        (void)cur_idx; // 暂未使用
    }
}

static void MusicScreen_title_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    int cur_idx = music_ui_current_index();
    lv_obj_t *target = lv_event_get_target(e);
    /* 如果手势发生在标题标签本身，则阻止继续冒泡到父容器，避免重复触发 */
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    if (ui && target == ui->MusicScreen_music_title) {
        lv_event_stop_bubbling(e);
    }
    ESP_LOGI("MusicScreen", "Title gesture target=%p dir=%d idx=%d", (void *)target, (int)dir, cur_idx);
    if (dir == LV_DIR_LEFT) {
        if (MusicScreen_OpenSpectrum(false)) {
            ESP_LOGI("MusicScreen", "Spectrum opened from light screen");
            lv_indev_wait_release(indev);
        } else {
            ESP_LOGW("MusicScreen", "Spectrum open rejected (light screen)");
        }
    }
}

/* 兜底：音乐页顶栏/标题在未产生 LV_EVENT_GESTURE 时，按下-抬起位移识别左滑 */
static void MusicScreen_title_swipe_fallback_handler(lv_event_t *e)
{
    static lv_coord_t swipe_start_x = LV_COORD_MIN;

    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    /* 若事件直接来自标题标签本身，阻止冒泡到父容器，避免重复触发 */
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    bool is_title = (ui && target == ui->MusicScreen_music_title);

    switch (code) {
    case LV_EVENT_PRESSED:
        swipe_start_x = p.x;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    case LV_EVENT_PRESS_LOST:
        swipe_start_x = LV_COORD_MIN;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    case LV_EVENT_RELEASED:
    {
        if (swipe_start_x != LV_COORD_MIN) {
            lv_coord_t delta = p.x - swipe_start_x;
            if (delta < -40) {
                if (MusicScreen_OpenSpectrum(false)) {
                    lv_indev_wait_release(indev);
                }
            }
        }
        swipe_start_x = LV_COORD_MIN;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    }
    default:
        break;
    }
}

static void MusicScreen_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.MusicScreen_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 500, false, true);
        break;
    }
    default:
        break;
    }
}

static void MusicScreen_img_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.MusicScreen_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_MusicScreen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->MusicScreen_btn_1, MusicScreen_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen_img_1, MusicScreen_img_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen, MusicScreen_root_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen_music_top, MusicScreen_title_event_handler, LV_EVENT_ALL, ui);
    /* 兜底：在顶栏也增加按下-抬起位移识别 */
    lv_obj_add_event_cb(ui->MusicScreen_music_top, MusicScreen_title_swipe_fallback_handler, LV_EVENT_ALL, ui);
    /* 需求：在标题标签本身也支持左滑进入频谱屏幕。
     * 之前仅在顶栏容器(MusicScreen_music_top)上监听手势，
     * 若手势没有冒泡或被其它控件截获，会导致无法进入。
     * 这里直接给标题标签(MusicScreen_music_title)也绑定相同处理。 */
    if (ui->MusicScreen_music_title) {
        lv_obj_add_event_cb(ui->MusicScreen_music_title, MusicScreen_title_event_handler, LV_EVENT_ALL, ui);
        lv_obj_add_event_cb(ui->MusicScreen_music_title, MusicScreen_title_swipe_fallback_handler, LV_EVENT_ALL, ui);
    }
}

static void MusicScreen_dark_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del, &guider_ui.MusicScreen_dark_del, setup_scr_app_screen_dark, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 500, false, true);
        break;
    }
    default:
        break;
    }
}

static void MusicScreen_dark_img_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del, &guider_ui.MusicScreen_dark_del, setup_scr_app_screen_dark, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void MusicScreen_dark_root_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) {
            ESP_LOGW("MusicScreen", "Dark root gesture without indev");
            return;
        }
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        int cur_idx = music_ui_current_index();
        ESP_LOGD("MusicScreen", "Dark root gesture dir=%d idx=%d (spectrum open disabled on root)", (int)dir, cur_idx);
        /* 深色主题同样不允许从根区域左滑打开频谱，仅保留 top 区域入口 */
        (void)cur_idx; // 暂未使用
    }
}

static void MusicScreen_dark_title_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    int cur_idx = music_ui_current_index();
    lv_obj_t *target = lv_event_get_target(e);
    /* 与明亮主题相同：若在标题标签上识别到手势，阻止冒泡，避免重复处理 */
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    if (ui && target == ui->MusicScreen_music_title) {
        lv_event_stop_bubbling(e);
    }
    ESP_LOGI("MusicScreen", "Dark title gesture target=%p dir=%d idx=%d", (void *)target, (int)dir, cur_idx);
    if (dir == LV_DIR_LEFT) {
        if (MusicScreen_OpenSpectrum(true)) {
            ESP_LOGI("MusicScreen", "Spectrum opened from dark screen");
            lv_indev_wait_release(indev);
        } else {
            ESP_LOGW("MusicScreen", "Spectrum open rejected (dark screen)");
        }
    }
}

/* 兜底（暗色）：音乐页顶栏/标题按下-抬起位移识别左滑 */
static void MusicScreen_dark_title_swipe_fallback_handler(lv_event_t *e)
{
    static lv_coord_t swipe_start_x = LV_COORD_MIN;

    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    bool is_title = (ui && target == ui->MusicScreen_music_title);

    switch (code) {
    case LV_EVENT_PRESSED:
        swipe_start_x = p.x;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    case LV_EVENT_PRESS_LOST:
        swipe_start_x = LV_COORD_MIN;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    case LV_EVENT_RELEASED:
    {
        if (swipe_start_x != LV_COORD_MIN) {
            lv_coord_t delta = p.x - swipe_start_x;
            if (delta < -40) {
                if (MusicScreen_OpenSpectrum(true)) {
                    lv_indev_wait_release(indev);
                }
            }
        }
        swipe_start_x = LV_COORD_MIN;
        if (is_title) lv_event_stop_bubbling(e);
        break;
    }
    default:
        break;
    }
}

void events_init_MusicScreen_dark (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->MusicScreen_dark_btn_1, MusicScreen_dark_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_1, MusicScreen_dark_img_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen_dark, MusicScreen_dark_root_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_top, MusicScreen_dark_title_event_handler, LV_EVENT_ALL, ui);
    /* 兜底：暗色顶栏也增加按下-抬起位移识别 */
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_top, MusicScreen_dark_title_swipe_fallback_handler, LV_EVENT_ALL, ui);
    /* 同步：暗色主题下也在标题标签支持左滑进入频谱。 */
    if (ui->MusicScreen_music_title) {
        lv_obj_add_event_cb(ui->MusicScreen_music_title, MusicScreen_dark_title_event_handler, LV_EVENT_ALL, ui);
        lv_obj_add_event_cb(ui->MusicScreen_music_title, MusicScreen_dark_title_swipe_fallback_handler, LV_EVENT_ALL, ui);
    }
}

static void MusicSpectrum_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) {
            break;
        }
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        ESP_LOGI("MusicScreen", "Spectrum gesture dir=%d visible=%d", (int)dir, (int)MusicScreen_IsSpectrumVisible());
        if (dir == LV_DIR_RIGHT) {
            bool was_visible = MusicScreen_IsSpectrumVisible();
            MusicScreen_CloseSpectrum();
            if (was_visible) {
                ESP_LOGI("MusicScreen", "Spectrum closed");
                lv_indev_wait_release(indev);
            }
        }
        break;
    }
    default:
        break;
    }
}

static void MusicSpectrum_swipe_fallback_handler(lv_event_t *e)
{
    static lv_coord_t swipe_start_x = LV_COORD_MIN;

    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    switch (code) {
    case LV_EVENT_PRESSED:
        swipe_start_x = p.x;
        break;
    case LV_EVENT_PRESS_LOST:
        swipe_start_x = LV_COORD_MIN;
        break;
    case LV_EVENT_RELEASED:
    {
        if (swipe_start_x != LV_COORD_MIN) {
            lv_coord_t delta = p.x - swipe_start_x;
            if (delta > 40) {
                bool was_visible = MusicScreen_IsSpectrumVisible();
                MusicScreen_CloseSpectrum();
                if (was_visible) {
                    lv_indev_wait_release(indev);
                }
            }
        }
        swipe_start_x = LV_COORD_MIN;
        break;
    }
    default:
        break;
    }
}

void events_init_MusicSpectrum(lv_ui *ui)
{
    if (!ui || !ui->MusicSpectrum) {
        return;
    }
    lv_obj_add_event_cb(ui->MusicSpectrum, MusicSpectrum_event_handler, LV_EVENT_ALL, ui);
    if (ui->MusicSpectrum_root) {
        lv_obj_add_event_cb(ui->MusicSpectrum_root, MusicSpectrum_event_handler, LV_EVENT_ALL, ui);
        lv_obj_add_event_cb(ui->MusicSpectrum_root, MusicSpectrum_swipe_fallback_handler, LV_EVENT_ALL, ui);
    }
    lv_obj_add_event_cb(ui->MusicSpectrum, MusicSpectrum_swipe_fallback_handler, LV_EVENT_ALL, ui);
}

/* ======================== Wi-Fi Main ======================== */

static void wifi_main_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.wifi_main_del, setup_scr_app_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_sw_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        /* 顶部开关状态 */
        bool on = lv_obj_has_state(guider_ui.wifi_main_sw_1, LV_STATE_CHECKED);

        /* 先更新 UI 可用性 */
        if(on) {
            set_enabled(guider_ui.wifi_main_btn_scan,  true);
            set_enabled(guider_ui.wifi_main_btn_saved, true);
            set_enabled_deep(guider_ui.wifi_main_ap_list, true);
            lv_label_set_text(guider_ui.wifi_main_lbl_status, "Not connected");
        } else {
            set_enabled(guider_ui.wifi_main_btn_scan,  false);
            set_enabled(guider_ui.wifi_main_btn_saved, false);
            set_enabled_deep(guider_ui.wifi_main_ap_list, false);
            lv_obj_add_flag(guider_ui.wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(guider_ui.wifi_main_msg_label, "");
            lv_label_set_text(guider_ui.wifi_main_lbl_status, "Wi-Fi OFF");
        }

        /* 调用实际的 Wi-Fi 开关 */
        wifi_ui_toggle(on);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_btn_scan_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_lbl_status, "Scanning...");
        wifi_ui_scan();                        // 真实扫描，完成后由回调填充列表
        break;
    }
    default:
        break;
    }
}

/* 生成器自带的三项示例按钮：保留但通常会被 wifi_ui_init() 的 lv_obj_clean() 删除 */
static void wifi_main_ap_list_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_pwd_title, "Connect to HomeFiber_5G");
        lv_textarea_set_text(guider_ui.wifi_main_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_ap_list_item1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_pwd_title, "Connect to Cafe_FreeWiFi");
        lv_textarea_set_text(guider_ui.wifi_main_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_ap_list_item2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_pwd_title, "Connect to OfficeNet");
        lv_textarea_set_text(guider_ui.wifi_main_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_btn_connect_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 交给 wifi_ui 执行真实连接（会自动收起弹窗/更新状态） */
        wifi_ui_connect_from_dialog();
        break;
    }
    default:
        break;
    }
}

static void wifi_main_btn_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        wifi_ui_back();     // 收起对话框与遮罩
        break;
    }
    default:
        break;
    }
}

void events_init_wifi_main (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->wifi_main_btn_1,       wifi_main_btn_1_event_handler,       LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_sw_1,        wifi_main_sw_1_event_handler,        LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_btn_scan,    wifi_main_btn_scan_event_handler,    LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_ap_list_item0, wifi_main_ap_list_item0_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_ap_list_item1, wifi_main_ap_list_item1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_ap_list_item2, wifi_main_ap_list_item2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_btn_connect, wifi_main_btn_connect_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_btn_back,    wifi_main_btn_back_event_handler,    LV_EVENT_ALL, ui);

    /* 注册 Wi-Fi 事件与填充逻辑（会清空示例列表项） */
    wifi_ui_init(ui);
}

/* ======================== Wi-Fi Main Dark ======================== */

static void wifi_main_dark_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_4, guider_ui.screen_4_del, &guider_ui.wifi_main_dark_del, setup_scr_app_screen_dark, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 100, false, true);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_sw_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        /* 顶部开关状态 */
        bool on = lv_obj_has_state(guider_ui.wifi_main_dark_sw_1, LV_STATE_CHECKED);

        /* 先更新 UI 可用性 */
        if(on) {
            set_enabled(guider_ui.wifi_main_dark_btn_scan,  true);
            set_enabled(guider_ui.wifi_main_dark_btn_saved, true);
            set_enabled_deep(guider_ui.wifi_main_dark_ap_list, true);
            lv_label_set_text(guider_ui.wifi_main_dark_lbl_status, "Not connected");
        } else {
            set_enabled(guider_ui.wifi_main_dark_btn_scan,  false);
            set_enabled(guider_ui.wifi_main_dark_btn_saved, false);
            set_enabled_deep(guider_ui.wifi_main_dark_ap_list, false);
            lv_obj_add_flag(guider_ui.wifi_main_dark_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(guider_ui.wifi_main_dark_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(guider_ui.wifi_main_dark_msg_label, "");
            lv_label_set_text(guider_ui.wifi_main_dark_lbl_status, "Wi-Fi OFF");
        }

        /* 调用实际的 Wi-Fi 开关 */
        wifi_ui_toggle(on);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_btn_scan_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_dark_lbl_status, "Scanning...");
        wifi_ui_scan();                        // 真实扫描，完成后由回调填充列表
        break;
    }
    default:
        break;
    }
}

/* 生成器自带的三项示例按钮：保留但通常会被 wifi_ui_init() 的 lv_obj_clean() 删除 */
static void wifi_main_dark_ap_list_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_dark_pwd_title, "Connect to HomeFiber_5G");
        lv_textarea_set_text(guider_ui.wifi_main_dark_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_dark_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dark_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_dark_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_ap_list_item1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_dark_pwd_title, "Connect to Cafe_FreeWiFi");
        lv_textarea_set_text(guider_ui.wifi_main_dark_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_dark_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dark_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_dark_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_ap_list_item2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_label_set_text(guider_ui.wifi_main_dark_pwd_title, "Connect to OfficeNet");
        lv_textarea_set_text(guider_ui.wifi_main_dark_pwd_ta, "");
        lv_obj_clear_flag(guider_ui.wifi_main_dark_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(guider_ui.wifi_main_dark_dlg_pwd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(guider_ui.wifi_main_dark_pwd_ta, LV_STATE_FOCUSED);
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_btn_connect_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 交给 wifi_ui 执行真实连接（会自动收起弹窗/更新状态） */
        wifi_ui_connect_from_dialog();
        break;
    }
    default:
        break;
    }
}

static void wifi_main_dark_btn_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        wifi_ui_back();     // 收起对话框与遮罩
        break;
    }
    default:
        break;
    }
}

void events_init_wifi_main_dark (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->wifi_main_dark_btn_1,       wifi_main_dark_btn_1_event_handler,       LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_sw_1,        wifi_main_dark_sw_1_event_handler,        LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_btn_scan,    wifi_main_dark_btn_scan_event_handler,    LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_ap_list_item0, wifi_main_dark_ap_list_item0_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_ap_list_item1, wifi_main_dark_ap_list_item1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_ap_list_item2, wifi_main_dark_ap_list_item2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_btn_connect, wifi_main_dark_btn_connect_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->wifi_main_dark_btn_back,    wifi_main_dark_btn_back_event_handler,    LV_EVENT_ALL, ui);

    /* 注册 Wi-Fi 事件与填充逻辑（会清空示例列表项） */
    wifi_ui_init(ui);
}

/* ======================== Stub ======================== */
void events_init(lv_ui *ui)
{
    LV_UNUSED(ui);
}
