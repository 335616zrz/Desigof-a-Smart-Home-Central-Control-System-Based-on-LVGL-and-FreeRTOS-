/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#include "fonts.h"

/*
 * UI Font Alias (per your rules)
 * - 12–15 号：所有非 lv_font_cn_12 的字体名别名到 lv_font_cn_12
 * - 16–20 号：所有非 lv_font_cn_18 的字体名别名到 lv_font_cn_18
 * - 21–25 号：所有非 lv_font_cn_24 的字体名别名到 lv_font_cn_24
 * - 26–30 号：所有非 lv_font_cn_30 的字体名别名到 lv_font_cn_30
 * 注意：不改动 lv_font_cn_num_30
 */
#ifndef UI_FONT_ALIAS_APPLIED
#define UI_FONT_ALIAS_APPLIED 1

/* 12–15 -> cn_12 */
#define lv_font_HarmonyOSHans_12                 lv_font_cn_12
#define lv_font_SourceHanSerifSC_Regular_12      lv_font_cn_12
#define lv_font_montserratMedium_12              lv_font_cn_12
#define lv_font_montserratMedium_13              lv_font_cn_12
#define lv_font_montserratMedium_14              lv_font_cn_12
#define lv_font_montserratMedium_15              lv_font_cn_12
#define lv_font_arial_12                         lv_font_cn_12
#define lv_font_arial_14                         lv_font_cn_12

/* 16–20 -> cn_18 */
#define lv_font_HarmonyOSHans_16                 lv_font_cn_18
#define lv_font_HarmonyOSHans_20                 lv_font_cn_18
#define lv_font_SourceHanSerifSC_Regular_16      lv_font_cn_18
#define lv_font_SourceHanSerifSC_Regular_17      lv_font_cn_18
#define lv_font_SourceHanSerifSC_Regular_18      lv_font_cn_18
#define lv_font_SourceHanSerifSC_Regular_19      lv_font_cn_18
#define lv_font_SourceHanSerifSC_Regular_20      lv_font_cn_18
#define lv_font_montserratMedium_16              lv_font_cn_18
#define lv_font_montserratMedium_17              lv_font_cn_18
#define lv_font_montserratMedium_18              lv_font_cn_18
#define lv_font_montserratMedium_19              lv_font_cn_18
#define lv_font_montserratMedium_20              lv_font_cn_18

/* 21–25 -> cn_24 */
#define lv_font_HarmonyOSHans_24                 lv_font_cn_24
#define lv_font_SourceHanSerifSC_Regular_21      lv_font_cn_24
#define lv_font_SourceHanSerifSC_Regular_22      lv_font_cn_24
#define lv_font_SourceHanSerifSC_Regular_23      lv_font_cn_24
#define lv_font_SourceHanSerifSC_Regular_24      lv_font_cn_24
#define lv_font_SourceHanSerifSC_Regular_25      lv_font_cn_24
#define lv_font_montserratMedium_21              lv_font_cn_24
#define lv_font_montserratMedium_22              lv_font_cn_24
#define lv_font_montserratMedium_23              lv_font_cn_24
#define lv_font_montserratMedium_24              lv_font_cn_24
#define lv_font_montserratMedium_25              lv_font_cn_24
#define lv_font_Acme_Regular_24                  lv_font_cn_24
#define lv_font_Alatsi_Regular_25                lv_font_cn_24

/* 26–30 -> cn_30 (不改动 lv_font_cn_num_30) */
#define lv_font_SourceHanSerifSC_Regular_26      lv_font_cn_30
#define lv_font_SourceHanSerifSC_Regular_27      lv_font_cn_30
#define lv_font_SourceHanSerifSC_Regular_28      lv_font_cn_30
#define lv_font_SourceHanSerifSC_Regular_29      lv_font_cn_30
#define lv_font_SourceHanSerifSC_Regular_30      lv_font_cn_30
#define lv_font_montserratMedium_26              lv_font_cn_30
#define lv_font_montserratMedium_27              lv_font_cn_30
#define lv_font_montserratMedium_28              lv_font_cn_30
#define lv_font_montserratMedium_29              lv_font_cn_30
#define lv_font_montserratMedium_30              lv_font_cn_30
#define lv_font_Antonio_Regular_28               lv_font_cn_30
#define lv_font_Acme_Regular_28                  lv_font_cn_30
#define lv_font_Alatsi_Regular_30                lv_font_cn_30
#define lv_font_arial_28                         lv_font_cn_30
#define lv_font_FontAwesome5_28                  lv_font_cn_30

/* 扩展：未收录的 41 号英文字体统一别名到数字中文 30 号，避免链接缺失 */
#define lv_font_montserratMedium_41              lv_font_cn_num_30

#endif /* UI_FONT_ALIAS_APPLIED */
typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_cont_1;
	lv_obj_t *screen_cont_2;
	lv_obj_t *screen_label_1;
	lv_obj_t *screen_label_2;
	lv_obj_t *screen_label_3;
	lv_obj_t *screen_label_4;
	lv_obj_t *screen_img_1;
	lv_obj_t *screen_label_5;
	lv_obj_t *screen_label_6;
	lv_obj_t *screen_img_2;
	lv_obj_t *screen_img_3;
	lv_obj_t *screen_label_9;
	lv_obj_t *screen_label_10;
	lv_obj_t *screen_label_11;
	lv_obj_t *screen_label_12;
	lv_obj_t *screen_label_13;
	lv_obj_t *screen_label_14;
	lv_obj_t *screen_arc_1;
	lv_obj_t *screen_label_15;
	lv_obj_t *screen_sw_1;
	lv_obj_t *screen_label_16;
	lv_obj_t *screen_img_4;
	lv_obj_t *screen_img_5;
	lv_obj_t *screen_1;
	bool screen_1_del;
	lv_obj_t *screen_1_cont_1;
	lv_obj_t *screen_1_cont_2;
	lv_obj_t *screen_1_label_1;
	lv_obj_t *screen_1_label_2;
	lv_obj_t *screen_1_label_3;
	lv_obj_t *screen_1_label_4;
	lv_obj_t *screen_1_img_1;
	lv_obj_t *screen_1_label_5;
	lv_obj_t *screen_1_label_6;
	lv_obj_t *screen_1_img_2;
	lv_obj_t *screen_1_img_3;
	lv_obj_t *screen_1_label_9;
	lv_obj_t *screen_1_label_10;
	lv_obj_t *screen_1_label_11;
	lv_obj_t *screen_1_label_12;
	lv_obj_t *screen_1_label_13;
	lv_obj_t *screen_1_label_14;
	lv_obj_t *screen_1_arc_1;
	lv_obj_t *screen_1_label_15;
	lv_obj_t *screen_1_sw_1;
	lv_obj_t *screen_1_label_16;
	lv_obj_t *screen_1_img_4;
	lv_obj_t *screen_1_img_5;
	lv_obj_t *screen_3;
	bool screen_3_del;
	lv_obj_t *screen_3_cont_1;
	lv_obj_t *screen_3_cont_3;
	lv_obj_t *screen_3_label_7;
	lv_obj_t *screen_3_cont_2;
	lv_obj_t *screen_3_btn_1;
	lv_obj_t *screen_3_btn_1_label;
	lv_obj_t *screen_3_img_1;
	lv_obj_t *screen_3_label_1;
	lv_obj_t *screen_3_btn_2;
	lv_obj_t *screen_3_btn_2_label;
	lv_obj_t *screen_3_img_2;
	lv_obj_t *screen_3_label_2;
	lv_obj_t *screen_3_btn_3;
	lv_obj_t *screen_3_btn_3_label;
	lv_obj_t *screen_3_img_3;
	lv_obj_t *screen_3_label_3;
	lv_obj_t *screen_3_btn_4;
	lv_obj_t *screen_3_btn_4_label;
	lv_obj_t *screen_3_img_4;
	lv_obj_t *screen_3_label_4;
	lv_obj_t *screen_3_btn_5;
	lv_obj_t *screen_3_btn_5_label;
	lv_obj_t *screen_3_img_5;
	lv_obj_t *screen_3_label_5;
	lv_obj_t *screen_3_btn_6;
	lv_obj_t *screen_3_btn_6_label;
	lv_obj_t *screen_3_img_6;
	lv_obj_t *screen_3_label_6;
		lv_obj_t *screen_3_btn_7;
		lv_obj_t *screen_3_btn_7_label;
		lv_obj_t *screen_3_img_7;
		lv_obj_t *screen_3_label_8;
		lv_obj_t *screen_4;
		bool screen_4_del;
	lv_obj_t *screen_4_cont_1;
	lv_obj_t *screen_4_cont_3;
	lv_obj_t *screen_4_btn_1;
	lv_obj_t *screen_4_btn_1_label;
	lv_obj_t *screen_4_img_1;
	lv_obj_t *screen_4_label_1;
	lv_obj_t *screen_4_btn_2;
	lv_obj_t *screen_4_btn_2_label;
	lv_obj_t *screen_4_img_2;
	lv_obj_t *screen_4_label_2;
	lv_obj_t *screen_4_btn_3;
	lv_obj_t *screen_4_btn_3_label;
	lv_obj_t *screen_4_img_3;
	lv_obj_t *screen_4_label_3;
	lv_obj_t *screen_4_btn_4;
	lv_obj_t *screen_4_btn_4_label;
	lv_obj_t *screen_4_img_4;
	lv_obj_t *screen_4_label_4;
	lv_obj_t *screen_4_btn_5;
	lv_obj_t *screen_4_btn_5_label;
	lv_obj_t *screen_4_img_5;
	lv_obj_t *screen_4_label_5;
	lv_obj_t *screen_4_btn_6;
	lv_obj_t *screen_4_btn_6_label;
	lv_obj_t *screen_4_img_6;
	lv_obj_t *screen_4_label_6;
	lv_obj_t *screen_4_cont_2;
	lv_obj_t *screen_4_label_7;
		lv_obj_t *screen_4_btn_7;
		lv_obj_t *screen_4_btn_7_label;
		lv_obj_t *screen_4_img_7;
		lv_obj_t *screen_4_label_8;
		lv_obj_t *screen_5;
		bool screen_5_del;
	lv_obj_t *screen_5_cont_1;
	lv_obj_t *screen_5_img_1;
	lv_obj_t *screen_5_btn_1;
	lv_obj_t *screen_5_btn_1_label;
	lv_obj_t *screen_5_label_1;
	lv_obj_t *screen_5_label_2;
	lv_obj_t *screen_5_label_3;
	lv_obj_t *screen_5_label_4;
	lv_obj_t *screen_5_label_5;
	lv_obj_t *screen_5_label_6;
	lv_obj_t *screen_5_label_7;
	lv_obj_t *screen_5_label_8;
	lv_obj_t *screen_6;
	bool screen_6_del;
	lv_obj_t *screen_6_cont_1;
	lv_obj_t *screen_6_img_1;
	lv_obj_t *screen_6_btn_1;
	lv_obj_t *screen_6_btn_1_label;
	lv_obj_t *screen_6_label_1;
	lv_obj_t *screen_6_label_2;
	lv_obj_t *screen_6_label_3;
	lv_obj_t *screen_6_label_4;
	lv_obj_t *screen_6_label_5;
	lv_obj_t *screen_6_label_6;
	lv_obj_t *screen_6_label_7;
	lv_obj_t *screen_6_label_8;
	lv_obj_t *screen_7;
	bool screen_7_del;
	lv_obj_t *screen_7_cont_1;

	lv_obj_t *screen_7_img_1;
	lv_obj_t *screen_7_btn_1;
	lv_obj_t *screen_7_btn_1_label;
    /* GPT 聊天新增控件 */
    lv_obj_t *screen_7_chat_input_cont;   // 底部输入区域容器
    lv_obj_t *screen_7_mode_btn;          // 文本/语音输入切换按钮
    lv_obj_t *screen_7_mode_btn_label;    // 切换按钮文字
    lv_obj_t *screen_7_voice_btn;         // 语音输入按钮（占位：按住说话）
    lv_obj_t *screen_7_voice_btn_label;   // 语音输入按钮文字
    lv_obj_t *screen_7_input_ta;          // 文本输入框（可呼出键盘）
    lv_obj_t *screen_7_send_btn;          // 发送按钮
    lv_obj_t *screen_7_send_btn_label;    // 发送按钮文字
    lv_obj_t *screen_7_lang_btn;          // 中/En 切换按钮
    lv_obj_t *screen_7_lang_btn_label;    // 切换按钮文字
	    lv_obj_t *screen_7_chat_list;         // 聊天消息列表容器（可滚动，左右气泡）
#if LV_USE_IME_PINYIN
	    lv_obj_t *screen_7_pinyin_ime;        // 拼音输入法（中英支持）
#endif
    /* GPT 屏专用：屏内容器上的键盘与 IME（减少顶层重绘） */
    lv_obj_t *screen_7_kb;
#if LV_USE_IME_PINYIN
    lv_obj_t *screen_7_ime;
#endif
    lv_obj_t *screen_7_kb_lang_btn;
    lv_obj_t *screen_7_kb_lang_btn_label;
	lv_obj_t *MusicScreen;
	bool MusicScreen_del;
	lv_obj_t *MusicScreen_music_root;
	lv_obj_t *MusicScreen_music_top;
	lv_obj_t *MusicScreen_music_title;
	lv_obj_t *MusicScreen_source_label;
	lv_obj_t *MusicScreen_music_mid;
	lv_obj_t *MusicScreen_music_list;
	lv_obj_t *MusicScreen_music_list_item0;
	lv_obj_t *MusicScreen_music_list_item1;
	lv_obj_t *MusicScreen_music_list_item2;
	lv_obj_t *MusicScreen_music_list_item3;
	lv_obj_t *MusicScreen_music_list_item4;
	lv_obj_t *MusicScreen_music_bar_vol;
	lv_obj_t *MusicScreen_music_bottom_a;
	lv_obj_t *MusicScreen_music_time_total;
	lv_obj_t *MusicScreen_music_slider_prog;
	lv_obj_t *MusicScreen_music_time_elapsed;
	lv_obj_t *MusicScreen_music_ctrl_row;
	lv_obj_t *MusicScreen_img_heart;
	lv_obj_t *MusicScreen_img_repeat_one;
	lv_obj_t *MusicScreen_img_prev;
	lv_obj_t *MusicScreen_imgbtn_play;
	lv_obj_t *MusicScreen_imgbtn_play_label;
	lv_obj_t *MusicScreen_img_next;
	lv_obj_t *MusicScreen_img_shuffle;
	lv_obj_t *MusicScreen_img_repeat_all;
	lv_obj_t *MusicScreen_music_vlo_label;
	lv_obj_t *MusicScreen_btn_1;
	lv_obj_t *MusicScreen_btn_1_label;
	lv_obj_t *MusicScreen_img_1;
	lv_obj_t *MusicSpectrum;
	bool MusicSpectrum_del;
	lv_obj_t *MusicSpectrum_root;
	lv_obj_t *MusicSpectrum_panel;
	lv_obj_t *MusicSpectrum_color_bar;
	lv_obj_t *MusicSpectrum_chart_bars;
	lv_obj_t *MusicSpectrum_chart_line;
	lv_obj_t *MusicScreen_dark;
	bool MusicScreen_dark_del;
	lv_obj_t *MusicScreen_dark_music_root;
	lv_obj_t *MusicScreen_dark_music_top;
	lv_obj_t *MusicScreen_dark_music_title;
	lv_obj_t *MusicScreen_dark_source_label;
	lv_obj_t *MusicScreen_dark_music_mid;
	lv_obj_t *MusicScreen_dark_music_list;
	lv_obj_t *MusicScreen_dark_music_list_item0;
	lv_obj_t *MusicScreen_dark_music_list_item1;
	lv_obj_t *MusicScreen_dark_music_list_item2;
	lv_obj_t *MusicScreen_dark_music_list_item3;
	lv_obj_t *MusicScreen_dark_music_list_item4;
	lv_obj_t *MusicScreen_dark_music_bar_vol;
	lv_obj_t *MusicScreen_dark_music_bottom_a;
	lv_obj_t *MusicScreen_dark_music_time_total;
	lv_obj_t *MusicScreen_dark_music_slider_prog;
	lv_obj_t *MusicScreen_dark_music_time_elapsed;
	lv_obj_t *MusicScreen_dark_music_ctrl_row;
	lv_obj_t *MusicScreen_dark_img_heart;
	lv_obj_t *MusicScreen_dark_img_repeat_one;
	lv_obj_t *MusicScreen_dark_img_prev;
	lv_obj_t *MusicScreen_dark_imgbtn_play;
	lv_obj_t *MusicScreen_dark_imgbtn_play_label;
	lv_obj_t *MusicScreen_dark_img_next;
	lv_obj_t *MusicScreen_dark_img_shuffle;
	lv_obj_t *MusicScreen_dark_img_repeat_all;
	lv_obj_t *MusicScreen_dark_music_vlo_label;
	lv_obj_t *MusicScreen_dark_btn_1;
	lv_obj_t *MusicScreen_dark_btn_1_label;
	lv_obj_t *MusicScreen_dark_img_1;
	lv_obj_t *wifi_main;
	bool wifi_main_del;
	lv_obj_t *wifi_main_cont_1;
	lv_obj_t *wifi_main_cont_2;
	lv_obj_t *wifi_main_img_1;
	lv_obj_t *wifi_main_btn_1;
	lv_obj_t *wifi_main_btn_1_label;
	lv_obj_t *wifi_main_sw_1;
	lv_obj_t *wifi_main_lbl_status;
	lv_obj_t *wifi_main_btn_scan;
	lv_obj_t *wifi_main_btn_scan_label;
	lv_obj_t *wifi_main_btn_saved;
	lv_obj_t *wifi_main_btn_saved_label;
	lv_obj_t *wifi_main_label_1;
	lv_obj_t *wifi_main_ap_list;
	lv_obj_t *wifi_main_ap_list_item0;
	lv_obj_t *wifi_main_ap_list_item1;
	lv_obj_t *wifi_main_ap_list_item2;
	lv_obj_t *wifi_main_overlay;
	lv_obj_t *wifi_main_dlg_pwd;
	lv_obj_t *wifi_main_msg_label;
	lv_obj_t *wifi_main_btn_connect;
	lv_obj_t *wifi_main_btn_connect_label;
	lv_obj_t *wifi_main_btn_back;
	lv_obj_t *wifi_main_btn_back_label;
	lv_obj_t *wifi_main_pwd_ta;
	lv_obj_t *wifi_main_pwd_title;
	lv_obj_t *wifi_main_dark;
	bool wifi_main_dark_del;
	lv_obj_t *wifi_main_dark_cont_1;
	lv_obj_t *wifi_main_dark_cont_2;
	lv_obj_t *wifi_main_dark_img_1;
	lv_obj_t *wifi_main_dark_btn_1;
	lv_obj_t *wifi_main_dark_btn_1_label;
	lv_obj_t *wifi_main_dark_sw_1;
	lv_obj_t *wifi_main_dark_lbl_status;
	lv_obj_t *wifi_main_dark_btn_scan;
	lv_obj_t *wifi_main_dark_btn_scan_label;
	lv_obj_t *wifi_main_dark_btn_saved;
	lv_obj_t *wifi_main_dark_btn_saved_label;
	lv_obj_t *wifi_main_dark_label_1;
	lv_obj_t *wifi_main_dark_ap_list;
	lv_obj_t *wifi_main_dark_ap_list_item0;
	lv_obj_t *wifi_main_dark_ap_list_item1;
	lv_obj_t *wifi_main_dark_ap_list_item2;
	lv_obj_t *wifi_main_dark_overlay;
	lv_obj_t *wifi_main_dark_dlg_pwd;
	lv_obj_t *wifi_main_dark_msg_label;
	lv_obj_t *wifi_main_dark_btn_connect;
	lv_obj_t *wifi_main_dark_btn_connect_label;
	lv_obj_t *wifi_main_dark_btn_back;
	lv_obj_t *wifi_main_dark_btn_back_label;
	lv_obj_t *wifi_main_dark_pwd_ta;
	lv_obj_t *wifi_main_dark_pwd_title;
  lv_obj_t *g_kb_top_layer;
  /* 键盘内置语言切换键（集成在键盘上） */
  lv_obj_t *g_kb_lang_btn;
  lv_obj_t *g_kb_lang_btn_label;
  /* Weather (light) */
  lv_obj_t *weather_main;
  bool      weather_main_del;
  lv_obj_t *weather_main_cont_top;
  lv_obj_t *weather_main_btn_back;
  lv_obj_t *weather_main_btn_back_label;
  lv_obj_t *weather_main_img_back;
  lv_obj_t *weather_main_title;
  lv_obj_t *weather_main_city;
  lv_obj_t *weather_main_temp;
  lv_obj_t *weather_main_temp_unit;
  lv_obj_t *weather_main_cond;
  lv_obj_t *weather_main_icon;
  lv_obj_t *weather_main_humi_icon;
  lv_obj_t *weather_main_humi_val;
  lv_obj_t *weather_main_humi_unit;
  /* 实况容器（中上圆角卡片） */
  lv_obj_t *weather_main_live_cont;
  /* 实况容器内元素（按新版卡片样式） */
  lv_obj_t *weather_live_loc;      /* 左上：省 市/区 */
  lv_obj_t *weather_live_time;     /* 右上：时间 */
  lv_obj_t *weather_live_temp;     /* 左侧大号温度（含°） */
  lv_obj_t *weather_live_cond;     /* 右侧：现象 */
  lv_obj_t *weather_chip_winddir;  /* 底部：风向（圆角胶囊） */
  lv_obj_t *weather_chip_winddir_lbl;
  lv_obj_t *weather_chip_windpow;  /* 底部：风力 */
  lv_obj_t *weather_chip_windpow_lbl;
  lv_obj_t *weather_chip_humi;     /* 底部：湿度 */
  lv_obj_t *weather_chip_humi_lbl;
  /* 预测容器（下半区圆角卡片集合） */
  lv_obj_t *weather_main_forecast;
  lv_obj_t *weather_main_forecast_title;   /* "未来预报" 标题 */
  lv_obj_t *weather_main_fc_row0;
  lv_obj_t *weather_main_fc_row0_lbl;      /* 日期 */
  lv_obj_t *weather_main_fc_row0_cond;     /* 天气（大字） */
  lv_obj_t *weather_main_fc_row0_temp;     /* 温度行 */
  lv_obj_t *weather_main_fc_row0_wind;     /* 风向风力行 */
  lv_obj_t *weather_main_fc_row1;
  lv_obj_t *weather_main_fc_row1_lbl;
  lv_obj_t *weather_main_fc_row1_cond;
  lv_obj_t *weather_main_fc_row1_temp;
  lv_obj_t *weather_main_fc_row1_wind;
  lv_obj_t *weather_main_fc_row2;
  lv_obj_t *weather_main_fc_row2_lbl;
  lv_obj_t *weather_main_fc_row2_cond;
  lv_obj_t *weather_main_fc_row2_temp;
  lv_obj_t *weather_main_fc_row2_wind;
  lv_obj_t *weather_main_fc_row3;
  lv_obj_t *weather_main_fc_row3_lbl;
  lv_obj_t *weather_main_fc_row3_cond;
  lv_obj_t *weather_main_fc_row3_temp;
  lv_obj_t *weather_main_fc_row3_wind;
  /* 城市选择 UI */
  lv_obj_t *weather_city_btn;        /* 顶栏右侧点击区域（含文字标签 weather_main_city） */
  lv_obj_t *weather_city_modal;      /* 遮罩层 */
  lv_obj_t *weather_city_panel;      /* 弹窗容器 */
  lv_obj_t *weather_city_dd_prov;    /* 省 */
  lv_obj_t *weather_city_dd_pref;    /* 市 */
  lv_obj_t *weather_city_dd_county;  /* 县 */
  lv_obj_t *weather_city_btn_ok;
  lv_obj_t *weather_city_btn_cancel;
  /* Calendar (light) */
  lv_obj_t *calendar_main;
  bool      calendar_main_del;
  lv_obj_t *calendar_header_cont;
  lv_obj_t *calendar_img_back;
  lv_obj_t *calendar_btn_back;
  lv_obj_t *calendar_btn_back_label;
  lv_obj_t *calendar_title;
  lv_obj_t *calendar_mode_month_btn;
  lv_obj_t *calendar_mode_month_lbl;
  lv_obj_t *calendar_mode_year_btn;
  lv_obj_t *calendar_mode_year_lbl;
  lv_obj_t *calendar_today_label;
  lv_obj_t *calendar_nav_month_cont;
  lv_obj_t *calendar_nav_month_prev_btn;
  lv_obj_t *calendar_nav_month_prev_lbl;
  lv_obj_t *calendar_nav_month_label;
  lv_obj_t *calendar_nav_month_next_btn;
  lv_obj_t *calendar_nav_month_next_lbl;
  lv_obj_t *calendar_nav_year_cont;
  lv_obj_t *calendar_nav_year_prev_btn;
  lv_obj_t *calendar_nav_year_prev_lbl;
  lv_obj_t *calendar_nav_year_label;
  lv_obj_t *calendar_nav_year_next_btn;
  lv_obj_t *calendar_nav_year_next_lbl;
  lv_obj_t *calendar_body_cont;
  lv_obj_t *calendar_month_view_cont;
  lv_obj_t *calendar_month_weekdays;
  lv_obj_t *calendar_month_days;
  lv_obj_t *calendar_year_view_cont;
  lv_obj_t *calendar_year_grid;
  lv_obj_t *calendar_detail_panel;
  lv_obj_t *calendar_selected_date_label;
  lv_obj_t *calendar_selected_lunar_label;
  lv_obj_t *calendar_events_list;
  lv_obj_t *calendar_event_input;
  lv_obj_t *calendar_event_add_btn;
  lv_obj_t *calendar_event_add_btn_label;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_bottom_layer(void);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_home_screen(lv_ui *ui);
void setup_scr_home_screen_dark(lv_ui *ui);
void setup_scr_app_screen(lv_ui *ui);
void setup_scr_app_screen_dark(lv_ui *ui);
void setup_scr_author_screen(lv_ui *ui);
void setup_scr_author_screen_dark(lv_ui *ui);
void setup_scr_screen_7(lv_ui *ui);
void setup_scr_screen_7_dark(lv_ui *ui);
void setup_scr_music_screen(lv_ui *ui);
void setup_scr_music_screen_dark(lv_ui *ui);
void setup_scr_music_spectrum(lv_ui *ui);
void setup_scr_wifi_screen(lv_ui *ui);
void setup_scr_wifi_screen_dark(lv_ui *ui);
void setup_scr_weather_screen(lv_ui *ui);
void setup_scr_weather_screen_dark(lv_ui *ui);
void setup_scr_calendar_screen(lv_ui *ui);
void setup_scr_calendar_screen_dark(lv_ui *ui);
LV_IMAGE_DECLARE(_Thermometer_RGB565A8_49x57);
LV_IMAGE_DECLARE(_Robot_RGB565A8_80x82);
LV_IMAGE_DECLARE(_Hygrometer_RGB565A8_49x57);
LV_IMAGE_DECLARE(_wifi_RGB565A8_22x26);
LV_IMAGE_DECLARE(_battery_RGB565A8_33x33);
LV_IMAGE_DECLARE(_version_update_RGB565A8_50x50);
LV_IMAGE_DECLARE(_Set_UP_RGB565A8_60x60);
LV_IMAGE_DECLARE(_music_RGB565A8_60x60);
LV_IMAGE_DECLARE(_GPT_RGB565A8_60x60);
LV_IMAGE_DECLARE(_Weather_RGB565A8_60x60);
LV_IMAGE_DECLARE(_Calendar_RGB565A8_60x60);
LV_IMAGE_DECLARE(_Author_RGB565A8_60x60);
LV_IMAGE_DECLARE(_return_RGB565A8_30x30);
LV_IMAGE_DECLARE(_ui_img_heart_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_repeat_one_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_prev_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_play_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_pause_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_next_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_shuffle_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_ui_img_repeat_all_png_RGB565A8_40x40);
LV_IMAGE_DECLARE(_map_RGB565A8_60x60);
LV_IMAGE_DECLARE(_change_over_RGB565A8_60x60);

LV_FONT_DECLARE(lv_font_montserratMedium_41)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_24)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_25)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_22)
LV_FONT_DECLARE(lv_font_montserratMedium_30)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_28)
LV_FONT_DECLARE(lv_font_montserratMedium_28)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_24)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_30)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_HarmonyOSHans_24)
LV_FONT_DECLARE(lv_font_HarmonyOSHans_20)
LV_FONT_DECLARE(lv_font_HarmonyOSHans_12)
/* Chinese font targets used by alias rules */
LV_FONT_DECLARE(lv_font_cn_12)
LV_FONT_DECLARE(lv_font_cn_18)
LV_FONT_DECLARE(lv_font_cn_24)
LV_FONT_DECLARE(lv_font_cn_30)
/* Keep the numeric font as-is (do not alias) */
LV_FONT_DECLARE(lv_font_cn_num_30)
LV_FONT_DECLARE(lv_font_montserratMedium_26)
LV_FONT_DECLARE(lv_font_SourceHanSerifSC_Regular_16)


#ifdef __cplusplus
}
#endif
#endif
