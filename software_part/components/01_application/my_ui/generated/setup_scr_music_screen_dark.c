/*
 * Dark variant auto-generated from music screen.
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

#include "music_ui.h"
#include "music_volume.h"
#include "music_progress.h"
#include "audio_player.h"
#include "esp_random.h"

extern lv_ui *g_music_screen_ui;

/* callbacks defined in setup_scr_music_screen.c */
extern void music_ctrl_heart_event_cb(lv_event_t *e);
extern void music_ctrl_repeat_one_event_cb(lv_event_t *e);
extern void music_ctrl_repeat_all_event_cb(lv_event_t *e);
extern void music_ctrl_shuffle_event_cb(lv_event_t *e);
extern void music_ctrl_prev_event_cb(lv_event_t *e);
extern void music_ctrl_next_event_cb(lv_event_t *e);
extern void music_ctrl_play_event_cb(lv_event_t *e);
extern void vol_evt_cb(lv_event_t *e);
extern void list_scroll_cb(lv_event_t *e);
extern void prog_evt_cb(lv_event_t *e);
extern void update_mode_icons(void);
extern void update_heart_icon(void);
/* From light theme implementation */
extern void MusicScreen_UpdateProgress(uint32_t cur_ms, uint32_t total_ms);
void setup_scr_music_screen_dark(lv_ui *ui)
{
    /* 深色音乐页同样在进入时执行一次音频场景切换，清理残留 GPT 语音 */
    AppAudio_OnEnterMusicScreen();

    //Write codes MusicScreen
    /* 关键：与亮色主题保持一致，记录当前 UI 指针，
       供 MusicScreen_UpdateProgress / UpdatePlayState 使用通用字段访问。*/
    extern lv_ui *g_music_screen_ui;
    g_music_screen_ui = ui;

    ui->MusicScreen_dark = lv_obj_create(NULL);
    lv_obj_set_size(ui->MusicScreen_dark, 480, 320);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_root
    ui->MusicScreen_dark_music_root = lv_obj_create(ui->MusicScreen_dark);
    lv_obj_set_pos(ui->MusicScreen_dark_music_root, 0, 0);
    lv_obj_set_size(ui->MusicScreen_dark_music_root, 480, 320);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_root, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_root, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_root, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_root, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_root, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_top
    ui->MusicScreen_dark_music_top = lv_obj_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_music_top, 75, 0);
    lv_obj_set_size(ui->MusicScreen_dark_music_top, 405, 60);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_top, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->MusicScreen_dark_music_top, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->MusicScreen_dark_music_top, LV_OBJ_FLAG_GESTURE_BUBBLE);

    //Write style for MusicScreen_music_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_top, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_top, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_title
    ui->MusicScreen_dark_music_title = lv_label_create(ui->MusicScreen_dark_music_top);
    lv_obj_set_pos(ui->MusicScreen_dark_music_title, 5, 15);
    /* 与亮色主题保持一致的布局：左侧 260 像素为标题，右侧留给来源标签 */
    lv_obj_set_size(ui->MusicScreen_dark_music_title, 260, 30);
    lv_label_set_text(ui->MusicScreen_dark_music_title, "Track 01 - Artist");
    lv_label_set_long_mode(ui->MusicScreen_dark_music_title, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(ui->MusicScreen_dark_music_title, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(ui->MusicScreen_dark_music_title, LV_OBJ_FLAG_CLICKABLE);

    //Write style for MusicScreen_music_title, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_music_title, lv_color_hex(0xe999e0), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_music_title, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_music_title, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_music_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_source_label (dark)
    ui->MusicScreen_dark_source_label = lv_label_create(ui->MusicScreen_dark_music_top);
    lv_obj_set_pos(ui->MusicScreen_dark_source_label, 275, 20);
    lv_obj_set_size(ui->MusicScreen_dark_source_label, 125, 30);
    lv_label_set_text(ui->MusicScreen_dark_source_label, "当前来源：网络");
    lv_label_set_long_mode(ui->MusicScreen_dark_source_label, LV_LABEL_LONG_CLIP);

    //Write style for MusicScreen_source_label (dark), Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 采用与暗色应用中心相近的淡紫色作为次要信息文字颜色 */
    lv_obj_set_style_text_color(ui->MusicScreen_dark_source_label, lv_color_hex(0xb17aab), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_source_label, &lv_font_HarmonyOSHans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_source_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_source_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_mid
    ui->MusicScreen_dark_music_mid = lv_obj_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_music_mid, 0, 65);
    lv_obj_set_size(ui->MusicScreen_dark_music_mid, 480, 165);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_mid, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_mid, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_mid, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_mid, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_mid, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_list
    ui->MusicScreen_dark_music_list = lv_list_create(ui->MusicScreen_dark_music_mid);
    lv_obj_set_pos(ui->MusicScreen_dark_music_list, 80, 5);
    lv_obj_set_size(ui->MusicScreen_dark_music_list, 395, 150);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_list, LV_SCROLLBAR_MODE_OFF);
    ui->MusicScreen_dark_music_list_item0 = lv_list_add_button(ui->MusicScreen_dark_music_list, LV_SYMBOL_AUDIO, "Track 01");
    ui->MusicScreen_dark_music_list_item1 = lv_list_add_button(ui->MusicScreen_dark_music_list, LV_SYMBOL_AUDIO, "Track 02");
    ui->MusicScreen_dark_music_list_item2 = lv_list_add_button(ui->MusicScreen_dark_music_list, LV_SYMBOL_AUDIO, "Track 03");
    ui->MusicScreen_dark_music_list_item3 = lv_list_add_button(ui->MusicScreen_dark_music_list, LV_SYMBOL_AUDIO, "Track 04");
    ui->MusicScreen_dark_music_list_item4 = lv_list_add_button(ui->MusicScreen_dark_music_list, LV_SYMBOL_AUDIO, "Track 05");

    // 残月效果：监听滚动
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_list, list_scroll_cb, LV_EVENT_SCROLL, NULL);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_main_main_default
    static lv_style_t style_MusicScreen_music_list_main_main_default;
    ui_init_style(&style_MusicScreen_music_list_main_main_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_main_main_default, 111);
    lv_style_set_bg_color(&style_MusicScreen_music_list_main_main_default, lv_color_hex(0xf3c2ed));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_radius(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_shadow_width(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_pad_top(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_main_main_default, 5);
    lv_obj_add_style(ui->MusicScreen_dark_music_list, &style_MusicScreen_music_list_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_main_scrollbar_default
    static lv_style_t style_MusicScreen_music_list_main_scrollbar_default;
    ui_init_style(&style_MusicScreen_music_list_main_scrollbar_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_main_scrollbar_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&style_MusicScreen_music_list_main_scrollbar_default, 3);
    lv_obj_add_style(ui->MusicScreen_dark_music_list, &style_MusicScreen_music_list_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_extra_btns_main_default
    static lv_style_t style_MusicScreen_music_list_extra_btns_main_default;
    ui_init_style(&style_MusicScreen_music_list_extra_btns_main_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_extra_btns_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_MusicScreen_music_list_extra_btns_main_default, 0);
    lv_style_set_radius(&style_MusicScreen_music_list_extra_btns_main_default, 3);
    lv_style_set_text_color(&style_MusicScreen_music_list_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_MusicScreen_music_list_extra_btns_main_default, &lv_font_montserratMedium_24);
    lv_style_set_text_opa(&style_MusicScreen_music_list_extra_btns_main_default, 255);
    lv_style_set_pad_top(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_obj_add_style(ui->MusicScreen_dark_music_list_item4, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_dark_music_list_item3, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_dark_music_list_item2, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_dark_music_list_item1, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_dark_music_list_item0, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_extra_texts_main_default
    static lv_style_t style_MusicScreen_music_list_extra_texts_main_default;
    ui_init_style(&style_MusicScreen_music_list_extra_texts_main_default);
    lv_style_set_pad_top(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_MusicScreen_music_list_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_MusicScreen_music_list_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_MusicScreen_music_list_extra_texts_main_default, &lv_font_montserratMedium_24);
    lv_style_set_text_opa(&style_MusicScreen_music_list_extra_texts_main_default, 255);
    lv_style_set_radius(&style_MusicScreen_music_list_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_MusicScreen_music_list_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_extra_texts_main_default, lv_color_hex(0x000000));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_extra_texts_main_default, LV_GRAD_DIR_NONE);

    /* 在创建完 ui->MusicScreen_dark_music_list 之后放这段（放前/后都行） */
    static lv_style_t style_list_items_txt24;
    static bool style_list_items_inited = false;
    if (!style_list_items_inited) {
    	ui_init_style(&style_list_items_txt24);
    	lv_style_set_text_font(&style_list_items_txt24, &lv_font_montserratMedium_24); // 24号
    	lv_style_set_text_color(&style_list_items_txt24, lv_color_hex(0x0D3055));   // 你的配色
    	style_list_items_inited = true;
    }

    /* 关键：把样式挂到 items 部分 */
    lv_obj_add_style(ui->MusicScreen_dark_music_list,
                 	&style_list_items_txt24,
                 	LV_PART_ITEMS | LV_STATE_DEFAULT);

    music_ui_attach_list(ui->MusicScreen_dark_music_list);

    /* 暗色主题选中高亮（微亮底色） */
    static lv_style_t style_list_item_checked_dark;
    static bool style_item_checked_dark_inited = false;
    if (!style_item_checked_dark_inited) {
        ui_init_style(&style_list_item_checked_dark);
        lv_style_set_bg_opa(&style_list_item_checked_dark, 96);
        lv_style_set_bg_color(&style_list_item_checked_dark, lv_color_hex(0x222222));
        lv_style_set_radius(&style_list_item_checked_dark, 4);
        style_item_checked_dark_inited = true;
    }
    lv_obj_add_style(ui->MusicScreen_dark_music_list,
                     &style_list_item_checked_dark,
                     LV_PART_ITEMS | LV_STATE_CHECKED);

    //Write codes MusicScreen_music_bar_vol
    ui->MusicScreen_dark_music_bar_vol = lv_bar_create(ui->MusicScreen_dark_music_mid);
    lv_obj_set_pos(ui->MusicScreen_dark_music_bar_vol, 30, 5);
    lv_obj_set_size(ui->MusicScreen_dark_music_bar_vol, 20, 150);
    lv_obj_set_style_anim_duration(ui->MusicScreen_dark_music_bar_vol, 1000, 0);
    lv_bar_set_mode(ui->MusicScreen_dark_music_bar_vol, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->MusicScreen_dark_music_bar_vol, 0, 100);
    lv_bar_set_value(ui->MusicScreen_dark_music_bar_vol, music_volume_get_percent(), LV_ANIM_OFF);

    // 让音量条可拖
    lv_obj_add_flag(ui->MusicScreen_dark_music_bar_vol, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_bar_vol, vol_evt_cb, LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_bar_vol, vol_evt_cb, LV_EVENT_PRESSING, NULL);

    music_volume_bind_slider(ui->MusicScreen_dark_music_bar_vol);

    //Write style for MusicScreen_music_bar_vol, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_bar_vol, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_bar_vol, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_bar_vol, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_bar_vol, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_bar_vol, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_bar_vol, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_bar_vol, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_bar_vol, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_bar_vol, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_bar_vol, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_bottom_a
    ui->MusicScreen_dark_music_bottom_a = lv_obj_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_music_bottom_a, 80, 240);
    lv_obj_set_size(ui->MusicScreen_dark_music_bottom_a, 400, 30);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_bottom_a, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_bottom_a, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_bottom_a, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_bottom_a, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_bottom_a, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_time_total
    ui->MusicScreen_dark_music_time_total = lv_label_create(ui->MusicScreen_dark_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_dark_music_time_total, 325, 0);
    lv_obj_set_size(ui->MusicScreen_dark_music_time_total, 65, 25);
    lv_label_set_text(ui->MusicScreen_dark_music_time_total, "00:00");
    lv_label_set_long_mode(ui->MusicScreen_dark_music_time_total, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_time_total, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_music_time_total, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_music_time_total, &lv_font_HarmonyOSHans_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_music_time_total, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_music_time_total, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_slider_prog
    ui->MusicScreen_dark_music_slider_prog = lv_slider_create(ui->MusicScreen_dark_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_dark_music_slider_prog, 70, 5);
    lv_obj_set_size(ui->MusicScreen_dark_music_slider_prog, 250, 10);
    lv_slider_set_range(ui->MusicScreen_dark_music_slider_prog, 0, 100);
    lv_slider_set_mode(ui->MusicScreen_dark_music_slider_prog, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->MusicScreen_dark_music_slider_prog, 0, LV_ANIM_OFF);

    // 进度条拖动/释放事件（VALUE_CHANGED 实时显示；RELEASED 可做 seek）
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_slider_prog, prog_evt_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_music_slider_prog, prog_evt_cb, LV_EVENT_RELEASED,      NULL);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_slider_prog, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_slider_prog, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->MusicScreen_dark_music_slider_prog, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_slider_prog, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_slider_prog, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_slider_prog, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_slider_prog, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_slider_prog, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_time_elapsed
    ui->MusicScreen_dark_music_time_elapsed = lv_label_create(ui->MusicScreen_dark_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_dark_music_time_elapsed, 0, 0);
    lv_obj_set_size(ui->MusicScreen_dark_music_time_elapsed, 65, 25);
    lv_label_set_text(ui->MusicScreen_dark_music_time_elapsed, "00:00");
    lv_label_set_long_mode(ui->MusicScreen_dark_music_time_elapsed, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_time_elapsed, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_music_time_elapsed, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_music_time_elapsed, &lv_font_HarmonyOSHans_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_music_time_elapsed, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_music_time_elapsed, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_ctrl_row
    ui->MusicScreen_dark_music_ctrl_row = lv_obj_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_music_ctrl_row, 0, 280);
    lv_obj_set_size(ui->MusicScreen_dark_music_ctrl_row, 480, 40);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_dark_music_ctrl_row, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_ctrl_row, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_ctrl_row, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_dark_music_ctrl_row, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_dark_music_ctrl_row, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_heart
    ui->MusicScreen_dark_img_heart = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_heart, 30, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_heart, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_heart, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_heart, &_ui_img_heart_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_heart, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_heart, 0);

    //Write style for MusicScreen_img_heart, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_heart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_heart, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_repeat_one
    ui->MusicScreen_dark_img_repeat_one = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_repeat_one, 90, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_repeat_one, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_repeat_one, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_repeat_one, &_ui_img_repeat_one_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_repeat_one, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_repeat_one, 0);

    //Write style for MusicScreen_img_repeat_one, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_repeat_one, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_repeat_one, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_prev
    ui->MusicScreen_dark_img_prev = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_prev, 150, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_prev, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_prev, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_prev, &_ui_img_prev_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_prev, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_prev, 0);

    //Write style for MusicScreen_img_prev, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_prev, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_prev, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_imgbtn_play
    ui->MusicScreen_dark_imgbtn_play = lv_imagebutton_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_imgbtn_play, 210, 0);
    lv_obj_set_size(ui->MusicScreen_dark_imgbtn_play, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_imgbtn_play, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->MusicScreen_dark_imgbtn_play, LV_OBJ_FLAG_CHECKABLE);
    lv_imagebutton_set_src(ui->MusicScreen_dark_imgbtn_play, LV_IMAGEBUTTON_STATE_RELEASED, &_ui_img_play_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_dark_imgbtn_play, LV_IMAGEBUTTON_STATE_PRESSED, &_ui_img_play_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_dark_imgbtn_play, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, &_ui_img_pause_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_dark_imgbtn_play, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, &_ui_img_pause_png_RGB565A8_40x40, NULL, NULL);
    ui->MusicScreen_dark_imgbtn_play_label = lv_label_create(ui->MusicScreen_dark_imgbtn_play);
    lv_label_set_text(ui->MusicScreen_dark_imgbtn_play_label, "");
    lv_label_set_long_mode(ui->MusicScreen_dark_imgbtn_play_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->MusicScreen_dark_imgbtn_play_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->MusicScreen_dark_imgbtn_play, 0, LV_STATE_DEFAULT);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->MusicScreen_dark_imgbtn_play, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_imgbtn_play, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_imgbtn_play, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_imgbtn_play, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_IMAGEBUTTON_STATE_RELEASED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_imgbtn_play, 0, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_imgbtn_play, 255, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);

    //Write codes MusicScreen_img_next
    ui->MusicScreen_dark_img_next = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_next, 270, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_next, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_next, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_next, &_ui_img_next_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_next, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_next, 0);

    //Write style for MusicScreen_img_next, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_next, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_next, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_shuffle
    ui->MusicScreen_dark_img_shuffle = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_shuffle, 330, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_shuffle, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_shuffle, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_shuffle, &_ui_img_shuffle_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_shuffle, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_shuffle, 0);

    //Write style for MusicScreen_img_shuffle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_shuffle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_shuffle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_repeat_all
    ui->MusicScreen_dark_img_repeat_all = lv_image_create(ui->MusicScreen_dark_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_dark_img_repeat_all, 390, 0);
    lv_obj_set_size(ui->MusicScreen_dark_img_repeat_all, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_dark_img_repeat_all, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_repeat_all, &_ui_img_repeat_all_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_dark_img_repeat_all, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_repeat_all, 0);

    //Write style for MusicScreen_img_repeat_all, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_repeat_all, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_repeat_all, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui->MusicScreen_dark_img_heart, music_ctrl_heart_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_repeat_one, music_ctrl_repeat_one_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_repeat_all, music_ctrl_repeat_all_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_shuffle, music_ctrl_shuffle_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_prev, music_ctrl_prev_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_img_next, music_ctrl_next_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_dark_imgbtn_play, music_ctrl_play_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Write codes MusicScreen_music_vlo_label
    ui->MusicScreen_dark_music_vlo_label = lv_label_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_music_vlo_label, 15, 235);
    lv_obj_set_size(ui->MusicScreen_dark_music_vlo_label, 50, 25);
    lv_label_set_text(ui->MusicScreen_dark_music_vlo_label, "音量");
    lv_label_set_long_mode(ui->MusicScreen_dark_music_vlo_label, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_vlo_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_music_vlo_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_music_vlo_label, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_music_vlo_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_music_vlo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_btn_1
    ui->MusicScreen_dark_btn_1 = lv_button_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_btn_1, 15, 10);
    lv_obj_set_size(ui->MusicScreen_dark_btn_1, 30, 30);
    ui->MusicScreen_dark_btn_1_label = lv_label_create(ui->MusicScreen_dark_btn_1);
    lv_label_set_text(ui->MusicScreen_dark_btn_1_label, "");
    lv_label_set_long_mode(ui->MusicScreen_dark_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->MusicScreen_dark_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->MusicScreen_dark_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->MusicScreen_dark_btn_1_label, LV_PCT(100));

    //Write style for MusicScreen_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_dark_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicScreen_dark_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->MusicScreen_dark_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->MusicScreen_dark_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->MusicScreen_dark_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_dark_btn_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_dark_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_dark_btn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_dark_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_dark_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_dark_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_1
    ui->MusicScreen_dark_img_1 = lv_image_create(ui->MusicScreen_dark_music_root);
    lv_obj_set_pos(ui->MusicScreen_dark_img_1, 13, 10);
    lv_obj_set_size(ui->MusicScreen_dark_img_1, 30, 30);
    lv_obj_add_flag(ui->MusicScreen_dark_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_dark_img_1, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->MusicScreen_dark_img_1, 50,50);
    lv_image_set_rotation(ui->MusicScreen_dark_img_1, 0);

    //Write style for MusicScreen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_dark_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_dark_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of MusicScreen.

    /* 复用公用字段，确保控制逻辑与回调共用 */
    ui->MusicScreen = ui->MusicScreen_dark;
    ui->MusicScreen_music_root = ui->MusicScreen_dark_music_root;
    ui->MusicScreen_music_top = ui->MusicScreen_dark_music_top;
    ui->MusicScreen_music_title = ui->MusicScreen_dark_music_title;
    ui->MusicScreen_music_mid = ui->MusicScreen_dark_music_mid;
    ui->MusicScreen_music_list = ui->MusicScreen_dark_music_list;
    ui->MusicScreen_music_list_item0 = ui->MusicScreen_dark_music_list_item0;
    ui->MusicScreen_music_list_item1 = ui->MusicScreen_dark_music_list_item1;
    ui->MusicScreen_music_list_item2 = ui->MusicScreen_dark_music_list_item2;
    ui->MusicScreen_music_list_item3 = ui->MusicScreen_dark_music_list_item3;
    ui->MusicScreen_music_list_item4 = ui->MusicScreen_dark_music_list_item4;
    ui->MusicScreen_music_bar_vol = ui->MusicScreen_dark_music_bar_vol;
    ui->MusicScreen_music_bottom_a = ui->MusicScreen_dark_music_bottom_a;
    ui->MusicScreen_music_time_total = ui->MusicScreen_dark_music_time_total;
    ui->MusicScreen_music_slider_prog = ui->MusicScreen_dark_music_slider_prog;
    ui->MusicScreen_music_time_elapsed = ui->MusicScreen_dark_music_time_elapsed;
    ui->MusicScreen_music_ctrl_row = ui->MusicScreen_dark_music_ctrl_row;
    ui->MusicScreen_img_heart = ui->MusicScreen_dark_img_heart;
    ui->MusicScreen_img_repeat_one = ui->MusicScreen_dark_img_repeat_one;
    ui->MusicScreen_img_prev = ui->MusicScreen_dark_img_prev;
    ui->MusicScreen_imgbtn_play = ui->MusicScreen_dark_imgbtn_play;
    ui->MusicScreen_imgbtn_play_label = ui->MusicScreen_dark_imgbtn_play_label;
    ui->MusicScreen_img_next = ui->MusicScreen_dark_img_next;
    ui->MusicScreen_img_shuffle = ui->MusicScreen_dark_img_shuffle;
    ui->MusicScreen_img_repeat_all = ui->MusicScreen_dark_img_repeat_all;
    ui->MusicScreen_music_vlo_label = ui->MusicScreen_dark_music_vlo_label;
    ui->MusicScreen_btn_1 = ui->MusicScreen_dark_btn_1;
    ui->MusicScreen_btn_1_label = ui->MusicScreen_dark_btn_1_label;
    ui->MusicScreen_img_1 = ui->MusicScreen_dark_img_1;
    /* 顶部来源标签：通用字段指向暗色控件，以复用 MusicScreen_UpdateSourceLabel */
    ui->MusicScreen_source_label = ui->MusicScreen_dark_source_label;

    g_music_screen_ui = ui;

    update_mode_icons();
    update_heart_icon();
#if CONFIG_APP_ENABLE_MUSIC
    MusicScreen_UpdatePlayState(ap_get_state() == AP_STATE_PLAYING);
#else
    MusicScreen_UpdatePlayState(false);
#endif

    /* 根据当前模式（HTTP / SD 卡）刷新一次来源标签文本 */
    MusicScreen_UpdateSourceLabel(music_ui_is_using_sdcard());

    /* 同步当前曲目信息，避免返回本页后标题被默认文本覆盖 */
    do {
        int idx = music_ui_current_index();
        if (idx < 0) break;

        const char *title = music_ui_title_at(idx);
        if (ui->MusicScreen_music_title && lv_obj_is_valid(ui->MusicScreen_music_title) && title && title[0]) {
            lv_label_set_text(ui->MusicScreen_music_title, title);
        }

        if (ui->MusicScreen_music_time_total && lv_obj_is_valid(ui->MusicScreen_music_time_total)) {
            const char *dur = music_ui_duration_at(idx);
            if (dur && dur[0]) {
                lv_label_set_text(ui->MusicScreen_music_time_total, dur);
            } else {
                /* 复用浅色页里的 mm:ss 工具：交由进度函数统一刷新 */
            }
        }

        /* 用进度模块的当前数值刷新新建控件的显示 */
        extern uint32_t music_progress_elapsed_ms(void);
        extern uint32_t music_progress_total_ms(void);
        MusicScreen_UpdateProgress(music_progress_elapsed_ms(), music_progress_total_ms());
    } while (0);

    //Update current screen layout.
    lv_obj_update_layout(ui->MusicScreen_dark);

    //Init events for screen.
    events_init_MusicScreen_dark(ui);

    ui->MusicScreen_dark_del = false;
}
/*
 * Dark variant auto-generated from music screen.
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

#include "music_ui.h"
#include "music_volume.h"
#include "music_progress.h"
#include "audio_player.h"
#include "esp_random.h"

extern lv_ui *g_music_screen_ui;

/* callbacks defined in setup_scr_music_screen.c */
extern void music_ctrl_heart_event_cb(lv_event_t *e);
extern void music_ctrl_repeat_one_event_cb(lv_event_t *e);
extern void music_ctrl_repeat_all_event_cb(lv_event_t *e);
extern void music_ctrl_shuffle_event_cb(lv_event_t *e);
extern void music_ctrl_prev_event_cb(lv_event_t *e);
extern void music_ctrl_next_event_cb(lv_event_t *e);
extern void music_ctrl_play_event_cb(lv_event_t *e);
extern void vol_evt_cb(lv_event_t *e);
extern void list_scroll_cb(lv_event_t *e);
extern void prog_evt_cb(lv_event_t *e);
extern void update_mode_icons(void);
extern void update_heart_icon(void);
/* From light theme implementation */
extern void MusicScreen_UpdateProgress(uint32_t cur_ms, uint32_t total_ms);
