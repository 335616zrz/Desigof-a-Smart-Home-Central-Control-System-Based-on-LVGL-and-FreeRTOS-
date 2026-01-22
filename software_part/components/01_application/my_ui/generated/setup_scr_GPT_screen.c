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
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "pinyin_dict_simplified.h"



void setup_scr_screen_7(lv_ui *ui)
{
    /* 进入 GPT 聊天页时，暂停/停止当前音乐播放，避免与 Chatbot 语音/网络互相干扰 */
    AppAudio_OnEnterGPTScreen();

    //Write codes screen_7
    ui->screen_7 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_7, 480, 320);
    lv_obj_set_scrollbar_mode(ui->screen_7, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_7_cont_1
    ui->screen_7_cont_1 = lv_obj_create(ui->screen_7);
    lv_obj_set_pos(ui->screen_7_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_7_cont_1, 480, 320);
    lv_obj_set_scrollbar_mode(ui->screen_7_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_7_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_7_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_7_img_1
    ui->screen_7_img_1 = lv_image_create(ui->screen_7);
    lv_obj_set_pos(ui->screen_7_img_1, 0, 2);
    lv_obj_set_size(ui->screen_7_img_1, 30, 30);
    lv_obj_add_flag(ui->screen_7_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_7_img_1, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->screen_7_img_1, 50,50);
    lv_image_set_rotation(ui->screen_7_img_1, 0);

    //Write style for screen_7_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_7_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_7_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_7_btn_1
    ui->screen_7_btn_1 = lv_button_create(ui->screen_7);
    lv_obj_set_pos(ui->screen_7_btn_1, 2, 2);
    lv_obj_set_size(ui->screen_7_btn_1, 30, 30);
    ui->screen_7_btn_1_label = lv_label_create(ui->screen_7_btn_1);
    lv_label_set_text(ui->screen_7_btn_1_label, "");
    lv_label_set_long_mode(ui->screen_7_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_7_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_7_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_7_btn_1_label, LV_PCT(100));

    //Write style for screen_7_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_7_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_7_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_7_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_7_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_7_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_7_btn_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_7_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_7_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_7_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_7_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---------------- GPT 聊天 UI（不改动已有 img/btn） ---------------- */
    /* 输入区放上方：避免弹出键盘时遮挡输入框 */
    ui->screen_7_chat_list = lv_obj_create(ui->screen_7);
    /* 聊天消息区域（下方，触底；内部用左右气泡显示） */
    lv_obj_set_pos(ui->screen_7_chat_list, 12, 120);
    lv_obj_set_size(ui->screen_7_chat_list, 456, 200);
    lv_obj_set_scrollbar_mode(ui->screen_7_chat_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(ui->screen_7_chat_list, LV_DIR_VER);
    lv_obj_set_flex_flow(ui->screen_7_chat_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ui->screen_7_chat_list, 8, 0);
    lv_obj_set_style_bg_opa(ui->screen_7_chat_list, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_chat_list, lv_color_hex(0xEAF4FF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_7_chat_list, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_7_chat_list, lv_color_hex(0xFFEAF1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_7_chat_list, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_7_chat_list, lv_color_hex(0xDCE7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_7_chat_list, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_7_chat_list, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_7_chat_list, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_7_chat_list, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_7_chat_list, 10, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 上方输入容器（圆角，内部放文本框与发送按钮） */
    ui->screen_7_chat_input_cont = lv_obj_create(ui->screen_7);
    lv_obj_set_size(ui->screen_7_chat_input_cont, 456, 68);
    lv_obj_set_pos(ui->screen_7_chat_input_cont, 12, 44);
    /* 聊天区域初始为空 */
    lv_obj_set_scrollbar_mode(ui->screen_7_chat_input_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(ui->screen_7_chat_input_cont, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_7_chat_input_cont, lv_color_hex(0xDCE7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_7_chat_input_cont, 18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_chat_input_cont, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_chat_input_cont, lv_color_hex(0xEAF4FF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_7_chat_input_cont, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_7_chat_input_cont, lv_color_hex(0xFFEAF1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_7_chat_input_cont, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_chat_input_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->screen_7_chat_input_cont, lv_color_hex(0x0B1A2B), LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 文本/语音输入切换按钮（左侧小按钮） */
    ui->screen_7_mode_btn = lv_button_create(ui->screen_7_chat_input_cont);
    lv_obj_set_pos(ui->screen_7_mode_btn, 8, 12);
    lv_obj_set_size(ui->screen_7_mode_btn, 44, 44);
    lv_obj_set_style_radius(ui->screen_7_mode_btn, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_mode_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_mode_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_7_mode_btn, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_7_mode_btn, lv_color_hex(0xDCE7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_mode_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->screen_7_mode_btn_label = lv_label_create(ui->screen_7_mode_btn);
    lv_label_set_text(ui->screen_7_mode_btn_label, "语音");
    lv_obj_center(ui->screen_7_mode_btn_label);
    lv_obj_set_style_text_font(ui->screen_7_mode_btn_label, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_7_mode_btn_label, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 语音输入按钮（默认隐藏，通过切换按钮显示） */
    ui->screen_7_voice_btn = lv_button_create(ui->screen_7_chat_input_cont);
    lv_obj_set_pos(ui->screen_7_voice_btn, 60, 12);
    lv_obj_set_size(ui->screen_7_voice_btn, 386, 44);
    lv_obj_set_style_radius(ui->screen_7_voice_btn, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_voice_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_voice_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_7_voice_btn, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_7_voice_btn, lv_color_hex(0xDCE7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_voice_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->screen_7_voice_btn_label = lv_label_create(ui->screen_7_voice_btn);
    lv_label_set_text(ui->screen_7_voice_btn_label, "按住说话");
    lv_obj_center(ui->screen_7_voice_btn_label);
    lv_obj_set_style_text_font(ui->screen_7_voice_btn_label, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_7_voice_btn_label, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_flag(ui->screen_7_voice_btn, LV_OBJ_FLAG_HIDDEN);

    /* 文本输入框（支持多行与中文显示） */
    ui->screen_7_input_ta = lv_textarea_create(ui->screen_7_chat_input_cont);
    lv_obj_set_pos(ui->screen_7_input_ta, 60, 12);
    lv_obj_set_size(ui->screen_7_input_ta, 280, 44);
    lv_textarea_set_one_line(ui->screen_7_input_ta, false);
    lv_textarea_set_max_length(ui->screen_7_input_ta, 2048);
    lv_textarea_set_placeholder_text(ui->screen_7_input_ta, "输入文字与助手聊天，按 Enter 发送，Shift+Enter 换行");
    lv_obj_set_scrollbar_mode(ui->screen_7_input_ta, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_text_color(ui->screen_7_input_ta, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_7_input_ta, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_input_ta, 0, LV_PART_MAIN|LV_STATE_DEFAULT); /* 透明以融入容器 */
    lv_obj_set_style_border_width(ui->screen_7_input_ta, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_7_input_ta, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_7_input_ta, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_7_input_ta, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_7_input_ta, 6, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 发送按钮（右侧圆角高亮） */
    ui->screen_7_send_btn = lv_button_create(ui->screen_7_chat_input_cont);
    lv_obj_set_pos(ui->screen_7_send_btn, 350, 10);
    lv_obj_set_size(ui->screen_7_send_btn, 96, 40);
    ui->screen_7_send_btn_label = lv_label_create(ui->screen_7_send_btn);
    lv_label_set_text(ui->screen_7_send_btn_label, "发送");
    lv_obj_center(ui->screen_7_send_btn_label);
    lv_obj_set_style_radius(ui->screen_7_send_btn, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_7_send_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_7_send_btn, lv_color_hex(0x1583FF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_7_send_btn, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_7_send_btn, lv_color_hex(0x63B3FF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_7_send_btn, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_7_send_btn, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_7_send_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_7_send_btn, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->screen_7_send_btn, lv_color_hex(0x0B1A2B), LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 中/En 切换按钮（输入框左侧） */
    /* 原屏内“中/En”按钮移除改为集成到键盘上；保持句柄有效但不显示 */
    ui->screen_7_lang_btn = lv_button_create(ui->screen_7_chat_input_cont);
    lv_obj_add_flag(ui->screen_7_lang_btn, LV_OBJ_FLAG_HIDDEN);
    ui->screen_7_lang_btn_label = lv_label_create(ui->screen_7_lang_btn);
    lv_label_set_text(ui->screen_7_lang_btn_label, "En");
    lv_obj_set_style_text_font(ui->screen_7_lang_btn_label, &lv_font_cn_18, 0);

#if LV_USE_KEYBOARD
    /* GPT 屏专用键盘：挂在屏内，减少顶层重绘 */
    ui->screen_7_kb = lv_keyboard_create(ui->screen_7);
    lv_obj_add_flag(ui->screen_7_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(ui->screen_7_kb, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_7_kb, &lv_font_cn_18, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui->screen_7_kb, kb_event_cb, LV_EVENT_ALL, NULL); /* hide on READY/CANCEL */

    /* Use a custom keyboard map which includes "中/En" as a normal key (not floating above). */
    static const char * const gpt_kb_map[] = {
        "1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_BACKSPACE, "\n",
        "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
        "_", "-", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
        LV_SYMBOL_CLOSE, "中/En", LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
    };
    lv_buttonmatrix_set_map(ui->screen_7_kb, gpt_kb_map);

    /* Ensure the "中/En" key is handled before LVGL's default keyboard handler. */
    lv_obj_remove_event_cb(ui->screen_7_kb, lv_keyboard_def_event_cb);
    lv_obj_add_event_cb(ui->screen_7_kb, kb_lang_key_event_cb, LV_EVENT_VALUE_CHANGED, ui);
    lv_obj_add_event_cb(ui->screen_7_kb, lv_keyboard_def_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* No floating language button anymore. Keep pointers cleared to avoid stale usage. */
    ui->screen_7_kb_lang_btn = NULL;
    ui->screen_7_kb_lang_btn_label = NULL;
    /* 文本框支持键盘：聚焦弹出屏内键盘 */
    lv_obj_add_event_cb(ui->screen_7_input_ta, ta_event_cb, LV_EVENT_ALL, ui->screen_7_kb);
#endif

#if LV_USE_IME_PINYIN
    /* 配置拼音 IME：挂在屏内，绑定屏内键盘 */
    ui->screen_7_ime = lv_ime_pinyin_create(ui->screen_7);
    lv_ime_pinyin_set_keyboard(ui->screen_7_ime, ui->screen_7_kb);
    lv_ime_pinyin_set_dict(ui->screen_7_ime, app_pinyin_dict_simplified); /* 简体词库 */
    lv_obj_set_style_text_font(ui->screen_7_ime, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 优化候选区：固定在屏内键盘上方，禁用滚动，放大命中面积 */
    lv_obj_t *kb   = ui->screen_7_kb;
    lv_obj_t *cand = lv_ime_pinyin_get_cand_panel(ui->screen_7_ime);
    lv_obj_set_size(cand, LV_PCT(100), 44);
    lv_obj_align_to(cand, kb, LV_ALIGN_OUT_TOP_MID, 0, -2);
    lv_obj_clear_flag(cand, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cand, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_style_bg_opa  (cand, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(cand, lv_color_hex(0xF7F8FA), 0);
    lv_obj_set_style_border_width(cand, 1, 0);
    lv_obj_set_style_border_color(cand, lv_color_hex(0xD0D7DE), 0);
    lv_obj_set_style_pad_all(cand, 4, 0);
    lv_obj_set_style_text_font(cand, &lv_font_cn_18, LV_PART_ITEMS);
    lv_obj_set_style_pad_left (cand, 10, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(cand, 10, LV_PART_ITEMS);
    lv_obj_set_style_pad_top  (cand, 8,  LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(cand, 8, LV_PART_ITEMS);
    lv_obj_set_style_min_width(cand, 64, LV_PART_ITEMS);
    lv_obj_set_style_bg_color (cand, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color (cand, lv_color_hex(0x1583FF), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(cand, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_PRESSED);
    /* 层级在键盘前即可，无需移到顶层 */

    /* 默认英文直输：隐藏候选栏，并把 IME 切到不会处理字母的模式 */
    lv_ime_pinyin_set_mode(ui->screen_7_ime, LV_IME_PINYIN_MODE_K9_NUMBER);
    lv_obj_add_flag(cand, LV_OBJ_FLAG_HIDDEN);
#endif

    //The custom code of screen_7.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_7);

    //Init events for screen.
    events_init_screen_7(ui);
}
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
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
