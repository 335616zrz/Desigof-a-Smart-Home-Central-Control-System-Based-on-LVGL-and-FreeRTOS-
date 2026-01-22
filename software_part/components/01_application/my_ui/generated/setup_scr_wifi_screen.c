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



void setup_scr_wifi_screen(lv_ui *ui)
{
    //Write codes wifi_main
    ui->wifi_main = lv_obj_create(NULL);
    lv_obj_set_size(ui->wifi_main, 480, 320);
    lv_obj_set_scrollbar_mode(ui->wifi_main, LV_SCROLLBAR_MODE_OFF);

    //Write style for wifi_main, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_cont_1
    ui->wifi_main_cont_1 = lv_obj_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_cont_1, 0, 0);
    lv_obj_set_size(ui->wifi_main_cont_1, 480, 320);
    lv_obj_set_scrollbar_mode(ui->wifi_main_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for wifi_main_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_cont_2
    ui->wifi_main_cont_2 = lv_obj_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_cont_2, 40, 0);
    lv_obj_set_size(ui->wifi_main_cont_2, 440, 60);
    lv_obj_set_scrollbar_mode(ui->wifi_main_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for wifi_main_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_cont_2, lv_color_hex(0x86e0e9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_img_1
    ui->wifi_main_img_1 = lv_image_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_img_1, 0, 2);
    lv_obj_set_size(ui->wifi_main_img_1, 30, 30);
    lv_obj_add_flag(ui->wifi_main_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->wifi_main_img_1, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->wifi_main_img_1, 50,50);
    lv_image_set_rotation(ui->wifi_main_img_1, 0);

    //Write style for wifi_main_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->wifi_main_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->wifi_main_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_btn_1
    ui->wifi_main_btn_1 = lv_button_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_btn_1, 2, 2);
    lv_obj_set_size(ui->wifi_main_btn_1, 30, 30);
    ui->wifi_main_btn_1_label = lv_label_create(ui->wifi_main_btn_1);
    lv_label_set_text(ui->wifi_main_btn_1_label, "");
    lv_label_set_long_mode(ui->wifi_main_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->wifi_main_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->wifi_main_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->wifi_main_btn_1_label, LV_PCT(100));

    //Write style for wifi_main_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_btn_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_sw_1
    ui->wifi_main_sw_1 = lv_switch_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_sw_1, 350, 10);
    lv_obj_set_size(ui->wifi_main_sw_1, 88, 34);

    //Write style for wifi_main_sw_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_sw_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_sw_1, lv_color_hex(0xe6e2e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_sw_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_sw_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_sw_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_sw_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_sw_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_sw_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for wifi_main_sw_1, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
    lv_obj_set_style_bg_opa(ui->wifi_main_sw_1, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui->wifi_main_sw_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_sw_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_border_width(ui->wifi_main_sw_1, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

    //Write style for wifi_main_sw_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_sw_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_sw_1, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_sw_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_sw_1, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_sw_1, 10, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes wifi_main_lbl_status
    ui->wifi_main_lbl_status = lv_label_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_lbl_status, 152, 21);
    lv_obj_set_size(ui->wifi_main_lbl_status, 161, 32);
    lv_label_set_text(ui->wifi_main_lbl_status, "Not connected");
    lv_label_set_long_mode(ui->wifi_main_lbl_status, LV_LABEL_LONG_WRAP);

    //Write style for wifi_main_lbl_status, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_lbl_status, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_lbl_status, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_lbl_status, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_lbl_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_lbl_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_btn_scan
    ui->wifi_main_btn_scan = lv_button_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_btn_scan, 62, 260);
    lv_obj_set_size(ui->wifi_main_btn_scan, 100, 51);
    ui->wifi_main_btn_scan_label = lv_label_create(ui->wifi_main_btn_scan);
    lv_label_set_text(ui->wifi_main_btn_scan_label, "Scan");
    lv_label_set_long_mode(ui->wifi_main_btn_scan_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->wifi_main_btn_scan_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->wifi_main_btn_scan, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->wifi_main_btn_scan_label, LV_PCT(100));

    //Write style for wifi_main_btn_scan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_btn_scan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_btn_scan, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_btn_scan, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_btn_scan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_btn_scan, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_btn_scan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_btn_scan, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_btn_scan, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_btn_scan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_btn_scan, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_btn_saved
    ui->wifi_main_btn_saved = lv_button_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_btn_saved, 288, 255);
    lv_obj_set_size(ui->wifi_main_btn_saved, 143, 55);
    ui->wifi_main_btn_saved_label = lv_label_create(ui->wifi_main_btn_saved);
    lv_label_set_text(ui->wifi_main_btn_saved_label, "Saved networks");
    lv_label_set_long_mode(ui->wifi_main_btn_saved_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->wifi_main_btn_saved_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->wifi_main_btn_saved, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->wifi_main_btn_saved_label, LV_PCT(100));

    //Write style for wifi_main_btn_saved, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_btn_saved, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_btn_saved, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_btn_saved, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_btn_saved, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_btn_saved, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_btn_saved, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_btn_saved, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_btn_saved, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_btn_saved, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_btn_saved, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_label_1
    ui->wifi_main_label_1 = lv_label_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_label_1, 52, 14);
    lv_obj_set_size(ui->wifi_main_label_1, 100, 32);
    lv_label_set_text(ui->wifi_main_label_1, "Wi-Fi");
    lv_label_set_long_mode(ui->wifi_main_label_1, LV_LABEL_LONG_WRAP);

    //Write style for wifi_main_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_label_1, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_ap_list
    ui->wifi_main_ap_list = lv_list_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_ap_list, 40, 75);
    lv_obj_set_size(ui->wifi_main_ap_list, 440, 170);
    lv_obj_set_scrollbar_mode(ui->wifi_main_ap_list, LV_SCROLLBAR_MODE_OFF);
    ui->wifi_main_ap_list_item0 = lv_list_add_button(ui->wifi_main_ap_list, LV_SYMBOL_WIFI, "HomeFiber_5G");
    ui->wifi_main_ap_list_item1 = lv_list_add_button(ui->wifi_main_ap_list, LV_SYMBOL_WIFI, "Cafe_FreeWiFi");
    ui->wifi_main_ap_list_item2 = lv_list_add_button(ui->wifi_main_ap_list, LV_SYMBOL_WIFI, "OfficeNet");

    //Write style state: LV_STATE_DEFAULT for &style_wifi_main_ap_list_main_main_default
    static lv_style_t style_wifi_main_ap_list_main_main_default;
    static bool style_wifi_main_ap_list_main_main_default_inited = false;
    if (!style_wifi_main_ap_list_main_main_default_inited) {
        lv_style_init(&style_wifi_main_ap_list_main_main_default);
        lv_style_set_pad_top(&style_wifi_main_ap_list_main_main_default, 5);
        lv_style_set_pad_left(&style_wifi_main_ap_list_main_main_default, 5);
        lv_style_set_pad_right(&style_wifi_main_ap_list_main_main_default, 5);
        lv_style_set_pad_bottom(&style_wifi_main_ap_list_main_main_default, 5);
        lv_style_set_bg_opa(&style_wifi_main_ap_list_main_main_default, 255);
        lv_style_set_bg_color(&style_wifi_main_ap_list_main_main_default, lv_color_hex(0xffffff));
        lv_style_set_bg_grad_dir(&style_wifi_main_ap_list_main_main_default, LV_GRAD_DIR_NONE);
        lv_style_set_border_width(&style_wifi_main_ap_list_main_main_default, 1);
        lv_style_set_border_opa(&style_wifi_main_ap_list_main_main_default, 255);
        lv_style_set_border_color(&style_wifi_main_ap_list_main_main_default, lv_color_hex(0xe1e6ee));
        lv_style_set_border_side(&style_wifi_main_ap_list_main_main_default, LV_BORDER_SIDE_FULL);
        lv_style_set_radius(&style_wifi_main_ap_list_main_main_default, 3);
        lv_style_set_shadow_width(&style_wifi_main_ap_list_main_main_default, 0);
        style_wifi_main_ap_list_main_main_default_inited = true;
    }
    lv_obj_add_style(ui->wifi_main_ap_list, &style_wifi_main_ap_list_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_wifi_main_ap_list_main_scrollbar_default
    static lv_style_t style_wifi_main_ap_list_main_scrollbar_default;
    static bool style_wifi_main_ap_list_main_scrollbar_default_inited = false;
    if (!style_wifi_main_ap_list_main_scrollbar_default_inited) {
        lv_style_init(&style_wifi_main_ap_list_main_scrollbar_default);
        lv_style_set_radius(&style_wifi_main_ap_list_main_scrollbar_default, 3);
        lv_style_set_bg_opa(&style_wifi_main_ap_list_main_scrollbar_default, 255);
        lv_style_set_bg_color(&style_wifi_main_ap_list_main_scrollbar_default, lv_color_hex(0xffffff));
        lv_style_set_bg_grad_dir(&style_wifi_main_ap_list_main_scrollbar_default, LV_GRAD_DIR_NONE);
        style_wifi_main_ap_list_main_scrollbar_default_inited = true;
    }
    lv_obj_add_style(ui->wifi_main_ap_list, &style_wifi_main_ap_list_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_wifi_main_ap_list_extra_btns_main_default
    static lv_style_t style_wifi_main_ap_list_extra_btns_main_default;
    static bool style_wifi_main_ap_list_extra_btns_main_default_inited = false;
    if (!style_wifi_main_ap_list_extra_btns_main_default_inited) {
        lv_style_init(&style_wifi_main_ap_list_extra_btns_main_default);
        lv_style_set_pad_top(&style_wifi_main_ap_list_extra_btns_main_default, 5);
        lv_style_set_pad_left(&style_wifi_main_ap_list_extra_btns_main_default, 5);
        lv_style_set_pad_right(&style_wifi_main_ap_list_extra_btns_main_default, 5);
        lv_style_set_pad_bottom(&style_wifi_main_ap_list_extra_btns_main_default, 5);
        lv_style_set_border_width(&style_wifi_main_ap_list_extra_btns_main_default, 0);
        lv_style_set_text_color(&style_wifi_main_ap_list_extra_btns_main_default, lv_color_hex(0x0D3055));
        lv_style_set_text_font(&style_wifi_main_ap_list_extra_btns_main_default, &lv_font_montserratMedium_12);
        lv_style_set_text_opa(&style_wifi_main_ap_list_extra_btns_main_default, 255);
        lv_style_set_radius(&style_wifi_main_ap_list_extra_btns_main_default, 3);
        lv_style_set_bg_opa(&style_wifi_main_ap_list_extra_btns_main_default, 255);
        lv_style_set_bg_color(&style_wifi_main_ap_list_extra_btns_main_default, lv_color_hex(0xffffff));
        lv_style_set_bg_grad_dir(&style_wifi_main_ap_list_extra_btns_main_default, LV_GRAD_DIR_NONE);
        style_wifi_main_ap_list_extra_btns_main_default_inited = true;
    }
    lv_obj_add_style(ui->wifi_main_ap_list_item2, &style_wifi_main_ap_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->wifi_main_ap_list_item1, &style_wifi_main_ap_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->wifi_main_ap_list_item0, &style_wifi_main_ap_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_wifi_main_ap_list_extra_texts_main_default
    static lv_style_t style_wifi_main_ap_list_extra_texts_main_default;
    static bool style_wifi_main_ap_list_extra_texts_main_default_inited = false;
    if (!style_wifi_main_ap_list_extra_texts_main_default_inited) {
        lv_style_init(&style_wifi_main_ap_list_extra_texts_main_default);
        lv_style_set_pad_top(&style_wifi_main_ap_list_extra_texts_main_default, 5);
        lv_style_set_pad_left(&style_wifi_main_ap_list_extra_texts_main_default, 5);
        lv_style_set_pad_right(&style_wifi_main_ap_list_extra_texts_main_default, 5);
        lv_style_set_pad_bottom(&style_wifi_main_ap_list_extra_texts_main_default, 5);
        lv_style_set_border_width(&style_wifi_main_ap_list_extra_texts_main_default, 0);
        lv_style_set_text_color(&style_wifi_main_ap_list_extra_texts_main_default, lv_color_hex(0x0D3055));
        lv_style_set_text_font(&style_wifi_main_ap_list_extra_texts_main_default, &lv_font_montserratMedium_12);
        lv_style_set_text_opa(&style_wifi_main_ap_list_extra_texts_main_default, 255);
        lv_style_set_radius(&style_wifi_main_ap_list_extra_texts_main_default, 3);
        lv_style_set_transform_width(&style_wifi_main_ap_list_extra_texts_main_default, 0);
        lv_style_set_bg_opa(&style_wifi_main_ap_list_extra_texts_main_default, 255);
        lv_style_set_bg_color(&style_wifi_main_ap_list_extra_texts_main_default, lv_color_hex(0xffffff));
        lv_style_set_bg_grad_dir(&style_wifi_main_ap_list_extra_texts_main_default, LV_GRAD_DIR_NONE);
        style_wifi_main_ap_list_extra_texts_main_default_inited = true;
    }

    //Write codes wifi_main_overlay
    ui->wifi_main_overlay = lv_obj_create(ui->wifi_main);
    lv_obj_set_pos(ui->wifi_main_overlay, 0, 0);
    lv_obj_set_size(ui->wifi_main_overlay, 480, 320);
    lv_obj_set_scrollbar_mode(ui->wifi_main_overlay, LV_SCROLLBAR_MODE_OFF);
    /* 重复隐藏标志去重：保留一次，外观不变 */
    lv_obj_add_flag(ui->wifi_main_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->wifi_main_overlay, LV_OBJ_FLAG_CLICKABLE);

    //Write style for wifi_main_overlay, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_overlay, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_overlay, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_overlay, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_overlay, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_overlay, 181, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_overlay, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_overlay, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_overlay, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_dlg_pwd
    ui->wifi_main_dlg_pwd = lv_obj_create(ui->wifi_main_overlay);
    lv_obj_set_pos(ui->wifi_main_dlg_pwd, 60, 70);
    lv_obj_set_size(ui->wifi_main_dlg_pwd, 360, 180);
    lv_obj_set_scrollbar_mode(ui->wifi_main_dlg_pwd, LV_SCROLLBAR_MODE_OFF);

    //Write style for wifi_main_dlg_pwd, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_dlg_pwd, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_dlg_pwd, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_dlg_pwd, lv_color_hex(0x959596), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_dlg_pwd, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_dlg_pwd, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_dlg_pwd, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_dlg_pwd, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_dlg_pwd, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_dlg_pwd, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_dlg_pwd, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_dlg_pwd, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_dlg_pwd, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_dlg_pwd, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_msg_label
    ui->wifi_main_msg_label = lv_label_create(ui->wifi_main_dlg_pwd);
    lv_obj_set_pos(ui->wifi_main_msg_label, 38, 44);
    lv_obj_set_size(ui->wifi_main_msg_label, 298, 17);
    lv_label_set_text(ui->wifi_main_msg_label, "");
    lv_label_set_long_mode(ui->wifi_main_msg_label, LV_LABEL_LONG_WRAP);

    //Write style for wifi_main_msg_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_msg_label, lv_color_hex(0xf00000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_msg_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_msg_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_msg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_msg_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_btn_connect
    ui->wifi_main_btn_connect = lv_button_create(ui->wifi_main_dlg_pwd);
    lv_obj_set_pos(ui->wifi_main_btn_connect, 220, 120);
    lv_obj_set_size(ui->wifi_main_btn_connect, 96, 42);
    ui->wifi_main_btn_connect_label = lv_label_create(ui->wifi_main_btn_connect);
    lv_label_set_text(ui->wifi_main_btn_connect_label, "Connect");
    lv_label_set_long_mode(ui->wifi_main_btn_connect_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->wifi_main_btn_connect_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->wifi_main_btn_connect, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->wifi_main_btn_connect_label, LV_PCT(100));

    //Write style for wifi_main_btn_connect, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_btn_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_btn_connect, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_btn_connect, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_btn_connect, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_btn_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_btn_connect, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_btn_connect, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_btn_connect, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_btn_connect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_btn_connect, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_btn_connect, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_btn_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_btn_connect, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_btn_back
    ui->wifi_main_btn_back = lv_button_create(ui->wifi_main_dlg_pwd);
    lv_obj_set_pos(ui->wifi_main_btn_back, 40, 121);
    lv_obj_set_size(ui->wifi_main_btn_back, 96, 42);
    ui->wifi_main_btn_back_label = lv_label_create(ui->wifi_main_btn_back);
    lv_label_set_text(ui->wifi_main_btn_back_label, "Back");
    lv_label_set_long_mode(ui->wifi_main_btn_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->wifi_main_btn_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->wifi_main_btn_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->wifi_main_btn_back_label, LV_PCT(100));

    //Write style for wifi_main_btn_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_btn_back, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_btn_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_btn_back, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_btn_back, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_btn_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_btn_back, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_btn_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_btn_back, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_btn_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_main_pwd_ta
    ui->wifi_main_pwd_ta = lv_textarea_create(ui->wifi_main_dlg_pwd);
    lv_obj_set_pos(ui->wifi_main_pwd_ta, 48, 79);
    lv_obj_set_size(ui->wifi_main_pwd_ta, 265, 30);
    lv_textarea_set_text(ui->wifi_main_pwd_ta, "Hello World");
    lv_textarea_set_placeholder_text(ui->wifi_main_pwd_ta, "Password");
    lv_textarea_set_password_bullet(ui->wifi_main_pwd_ta, "*");
    lv_textarea_set_password_mode(ui->wifi_main_pwd_ta, true);
    lv_textarea_set_one_line(ui->wifi_main_pwd_ta, true);
    lv_textarea_set_accepted_chars(ui->wifi_main_pwd_ta, "");
    lv_textarea_set_max_length(ui->wifi_main_pwd_ta, 32);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->wifi_main_pwd_ta, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for wifi_main_pwd_ta, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->wifi_main_pwd_ta, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_pwd_ta, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_pwd_ta, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_main_pwd_ta, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_pwd_ta, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_pwd_ta, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_pwd_ta, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_pwd_ta, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->wifi_main_pwd_ta, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->wifi_main_pwd_ta, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->wifi_main_pwd_ta, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->wifi_main_pwd_ta, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_pwd_ta, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_pwd_ta, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_pwd_ta, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_pwd_ta, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_pwd_ta, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for wifi_main_pwd_ta, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_main_pwd_ta, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_main_pwd_ta, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_main_pwd_ta, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_pwd_ta, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes wifi_main_pwd_title
    ui->wifi_main_pwd_title = lv_label_create(ui->wifi_main_dlg_pwd);
    lv_obj_set_pos(ui->wifi_main_pwd_title, 19, 13);
    lv_obj_set_size(ui->wifi_main_pwd_title, 325, 20);
    lv_label_set_text(ui->wifi_main_pwd_title, "Connect to …");
    lv_label_set_long_mode(ui->wifi_main_pwd_title, LV_LABEL_LONG_WRAP);

    //Write style for wifi_main_pwd_title, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_main_pwd_title, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_main_pwd_title, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_main_pwd_title, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_main_pwd_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_main_pwd_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of wifi_main.


    //Update current screen layout.
    lv_obj_update_layout(ui->wifi_main);

    //Init events for screen.
    events_init_wifi_main(ui);
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
