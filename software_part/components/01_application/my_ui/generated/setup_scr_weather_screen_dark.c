/*
 * Light Weather Screen for LVGL v9.x
 * Layout: top bar (back + title), hero (temp + cond + city + icon),
 *         humidity row, 4-line simple forecast list.
 */

#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "time_ui.h"

/* 早期实现中，天气页时间通过定时器从首页 mirror 过来；
 * 现在已由 time_ui_bind_datetime_label() 统一驱动此标签，以下代码保留为文档，不参与编译。 */
#if 0
// 让天气页时间跟随首页时间（screen_label_1）实时变化
static lv_timer_t *s_weather_time_mirror = NULL;
static void _weather_time_mirror_cb(lv_timer_t *t)
{
    if (!t) return;
    lv_ui *ui = (lv_ui*)lv_timer_get_user_data(t);
    if (!ui) return;
    if (!ui->weather_live_time || !lv_obj_is_valid(ui->weather_live_time)) return;
    if (!ui->screen_label_1 || !lv_obj_is_valid(ui->screen_label_1)) return; // 时间 HH:MM:SS
    if (!ui->screen_label_2 || !lv_obj_is_valid(ui->screen_label_2)) return; // 日期 YYYY-MM-DD

    const char *time_txt = lv_label_get_text(ui->screen_label_1);
    const char *date_txt = lv_label_get_text(ui->screen_label_2);
    if (!time_txt || !date_txt) return;

    char merged[32];
    // 目标格式：YYYY-MM-DD HH:MM:SS
    lv_snprintf(merged, sizeof(merged), "%s %s", date_txt, time_txt);

    // 避免重复刷新
    const char *dst = lv_label_get_text(ui->weather_live_time);
    if (!dst || strcmp(dst, merged) != 0) {
        lv_label_set_text(ui->weather_live_time, merged);
    }
}

static void _weather_screen_on_delete(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (s_weather_time_mirror) {
        lv_timer_del(s_weather_time_mirror);
        s_weather_time_mirror = NULL;
    }
}
#endif

void setup_scr_weather_screen_dark(lv_ui *ui)
{
    // Root screen
    ui->weather_main = lv_obj_create(NULL);
    lv_obj_set_size(ui->weather_main, 480, 320);
    lv_obj_set_scrollbar_mode(ui->weather_main, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main, LV_DIR_NONE);
    // Screen bg
    lv_obj_set_style_bg_opa(ui->weather_main, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

    // Top bar container
    ui->weather_main_cont_top = lv_obj_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_cont_top, 0, 0);
    lv_obj_set_size(ui->weather_main_cont_top, 480, 60);
    lv_obj_set_scrollbar_mode(ui->weather_main_cont_top, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_cont_top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_cont_top, LV_DIR_NONE);
    lv_obj_set_style_border_width(ui->weather_main_cont_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_cont_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_cont_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_cont_top, lv_color_hex(0x152c3a), LV_PART_MAIN|LV_STATE_DEFAULT);

    // Back image and button (hit area)
/*
    ui->weather_main_img_back = lv_image_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_img_back, 0, 2);
    lv_obj_set_size(ui->weather_main_img_back, 30, 30);
    lv_obj_add_flag(ui->weather_main_img_back, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->weather_main_img_back, &_return_RGB565A8_30x30);
*/    
    //Write codes weather_main_img_back
    ui->weather_main_img_back = lv_image_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_img_back, 2, 15);
    lv_obj_set_size(ui->weather_main_img_back, 30, 30);
    lv_obj_add_flag(ui->weather_main_img_back, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->weather_main_img_back, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->weather_main_img_back, 50,50);
    lv_image_set_rotation(ui->weather_main_img_back, 0);

    //Write style for weather_main_img_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->weather_main_img_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->weather_main_img_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    
/*
    ui->weather_main_btn_back = lv_button_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_btn_back, 2, 2);
    lv_obj_set_size(ui->weather_main_btn_back, 30, 30);
    ui->weather_main_btn_back_label = lv_label_create(ui->weather_main_btn_back);
    lv_label_set_text(ui->weather_main_btn_back_label, "");
    lv_obj_align(ui->weather_main_btn_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(ui->weather_main_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
*/    
    //Write codes weather_main_btn_back
    ui->weather_main_btn_back = lv_button_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_btn_back, 4, 15);
    lv_obj_set_size(ui->weather_main_btn_back, 30, 30);
    ui->weather_main_btn_back_label = lv_label_create(ui->weather_main_btn_back);
    lv_label_set_text(ui->weather_main_btn_back_label, "");
    lv_label_set_long_mode(ui->weather_main_btn_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->weather_main_btn_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->weather_main_btn_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_btn_back_label, LV_PCT(100));

    //Write style for weather_main_btn_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->weather_main_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_btn_back, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->weather_main_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_btn_back, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->weather_main_btn_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_btn_back, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->weather_main_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_btn_back, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_main_btn_back, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->weather_main_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_btn_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    // Title
    ui->weather_main_title = lv_label_create(ui->weather_main_cont_top);
    lv_obj_set_pos(ui->weather_main_title, 20, 0);
    lv_label_set_text(ui->weather_main_title, "天气 Weather");
    lv_obj_set_style_text_color(ui->weather_main_title, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_main_title, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(ui->weather_main_title, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_title, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_title, LV_DIR_NONE);

    // Live container (rounded) for realtime weather
    ui->weather_main_live_cont = lv_obj_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_live_cont, 12, 65);
    lv_obj_set_size(ui->weather_main_live_cont, 456, 100);
    lv_obj_set_style_radius(ui->weather_main_live_cont, 18, LV_PART_MAIN|LV_STATE_DEFAULT);
    // 固定区域：不显示滚动条、不允许滚动
    lv_obj_set_scrollbar_mode(ui->weather_main_live_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_live_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_live_cont, LV_DIR_NONE);
    // 使用浅蓝色纯色背景（不使用渐变）
    lv_obj_set_style_bg_opa(ui->weather_main_live_cont, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_live_cont, lv_color_hex(0x121a24), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->weather_main_live_cont, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_live_cont, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_live_cont, lv_color_hex(0x2a3d52), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_live_cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 顶部：位置与时间 */
    ui->weather_live_loc = lv_label_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_live_loc, 10, -5);
    lv_label_set_text(ui->weather_live_loc, "—— ——");
    lv_obj_set_style_text_color(ui->weather_live_loc, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_live_loc, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_live_time = lv_label_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_live_time, 250, -5);
    // 初始显示完整格式，随后由 time_ui 统一刷新
    lv_label_set_text(ui->weather_live_time, "---- -- --:--:--");
    lv_obj_set_style_text_color(ui->weather_live_time, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_live_time, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    // 确保 time_ui 已初始化，然后绑定天气页的“日期+时间”标签
    time_ui_init();
    // 绑定到 time_ui，让其每秒刷新为“YYYY-MM-DD HH:MM:SS”
    time_ui_bind_datetime_label(ui->weather_live_time);

    // 左侧温度（含°），使用 30 号字体
    ui->weather_live_temp = lv_label_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_live_temp, 16, 25);
    lv_label_set_text(ui->weather_live_temp, "--°");
    lv_obj_set_style_text_color(ui->weather_live_temp, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_live_temp, &lv_font_cn_num_30, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 右侧现象 */
    ui->weather_live_cond = lv_label_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_live_cond, 350, 25);
    lv_label_set_text(ui->weather_live_cond, "—");
    lv_obj_set_style_text_color(ui->weather_live_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_live_cond, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 旧的 condition 文本保留占位，但不再使用（apply_live_to_ui 更新 weather_live_cond）
    ui->weather_main_cond = lv_label_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_main_cond, 16, 70);
    lv_label_set_text(ui->weather_main_cond, "");
    lv_obj_set_style_text_color(ui->weather_main_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_main_cond, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);

    // City selector button on top bar (right)
    ui->weather_city_btn = lv_button_create(ui->weather_main_cont_top);
    lv_obj_set_pos(ui->weather_city_btn, 200, -5);
    lv_obj_set_size(ui->weather_city_btn, 260, 40);
    lv_obj_set_scrollbar_mode(ui->weather_city_btn, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_city_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_city_btn, LV_DIR_NONE);
    lv_obj_set_style_bg_opa(ui->weather_city_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_city_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_main_city = lv_label_create(ui->weather_city_btn);
    lv_label_set_text(ui->weather_main_city, "城市：———（省/市/县）");
    lv_obj_align(ui->weather_main_city, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui->weather_main_city, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_main_city, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(ui->weather_main_city, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_city, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_city, LV_DIR_NONE);

    // 按要求去除图标（原来类似 screen_3_img_4 的样式），不再创建 weather_main_icon

    /* 底部三枚胶囊：风向、风力、湿度 */
    ui->weather_chip_winddir = lv_button_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_chip_winddir, 30, 53);
    lv_obj_set_size(ui->weather_chip_winddir, 92, 26);
    // 不可点击，不冒泡
    lv_obj_clear_flag(ui->weather_chip_winddir, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->weather_chip_winddir, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_radius(ui->weather_chip_winddir, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_chip_winddir, lv_color_hex(0x0e1720), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_chip_winddir, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_chip_winddir, lv_color_hex(0x27435a), LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_chip_winddir_lbl = lv_label_create(ui->weather_chip_winddir);
    lv_label_set_text(ui->weather_chip_winddir_lbl, "风向 —");
    lv_obj_center(ui->weather_chip_winddir_lbl);
    lv_obj_set_style_text_font(ui->weather_chip_winddir_lbl, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_chip_winddir_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_chip_windpow = lv_button_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_chip_windpow, 130, 53);
    lv_obj_set_size(ui->weather_chip_windpow, 92, 26);
    lv_obj_clear_flag(ui->weather_chip_windpow, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui->weather_chip_windpow, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_chip_windpow, lv_color_hex(0x0e1720), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_chip_windpow, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_chip_windpow, lv_color_hex(0x27435a), LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_chip_windpow_lbl = lv_label_create(ui->weather_chip_windpow);
    lv_label_set_text(ui->weather_chip_windpow_lbl, "风力 —");
    lv_obj_center(ui->weather_chip_windpow_lbl);
    lv_obj_set_style_text_font(ui->weather_chip_windpow_lbl, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_chip_windpow_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_chip_humi = lv_button_create(ui->weather_main_live_cont);
    lv_obj_set_pos(ui->weather_chip_humi, 230, 53);
    lv_obj_set_size(ui->weather_chip_humi, 104, 26);
    lv_obj_clear_flag(ui->weather_chip_humi, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui->weather_chip_humi, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_chip_humi, lv_color_hex(0x0e1720), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_chip_humi, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_chip_humi, lv_color_hex(0x27435a), LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_chip_humi_lbl = lv_label_create(ui->weather_chip_humi);
    lv_label_set_text(ui->weather_chip_humi_lbl, "湿度 —%");
    lv_obj_center(ui->weather_chip_humi_lbl);
    lv_obj_set_style_text_font(ui->weather_chip_humi_lbl, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_chip_humi_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

    // Forecast container (rounded, lower half, flex 2x2 cards)
    ui->weather_main_forecast = lv_obj_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_main_forecast, 12, 170);
    /* 单行 4 卡片 + 顶部标题，容器稍微加高到 120 */
    lv_obj_set_size(ui->weather_main_forecast, 456, 150);
    lv_obj_set_style_radius(ui->weather_main_forecast, 18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_forecast, 233, LV_PART_MAIN|LV_STATE_DEFAULT);
    // 浅蓝色背景，和截图一致的淡色卡片区底色
    lv_obj_set_style_bg_color(ui->weather_main_forecast, lv_color_hex(0x0f2130), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_forecast, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->weather_main_forecast, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 左右各 4px，使 4×100 宽卡片正好满行无溢出；保留上下 8px */
    lv_obj_set_style_pad_left(ui->weather_main_forecast, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->weather_main_forecast, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 父容器不用 Flex，避免标题参与排版 */
    lv_obj_set_style_pad_row(ui->weather_main_forecast, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui->weather_main_forecast, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_layout(ui->weather_main_forecast, LV_LAYOUT_NONE);
    /* 固定化：容器不允许滚动 */
    lv_obj_set_scrollbar_mode(ui->weather_main_forecast, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_forecast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_forecast, LV_DIR_NONE);

    /* 标题：未来预报（容器左上角） */
    ui->weather_main_forecast_title = lv_label_create(ui->weather_main_forecast);
    lv_obj_set_pos(ui->weather_main_forecast_title, 8, 0);
    lv_label_set_text(ui->weather_main_forecast_title, "未来预报");
    lv_obj_set_style_text_font(ui->weather_main_forecast_title, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_forecast_title, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 单行 4 卡片固定化：不使用列表或 Flex，直接按坐标摆放 */

    // 4 small cards
    ui->weather_main_fc_row0 = lv_obj_create(ui->weather_main_forecast);
    lv_obj_set_pos(ui->weather_main_fc_row0, 5, 22);
    lv_obj_set_size(ui->weather_main_fc_row0, 105, 115);
    lv_obj_set_style_radius(ui->weather_main_fc_row0, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row0, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row0, lv_color_hex(0x182635), LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 固定化：卡片不允许滚动 */
    lv_obj_set_scrollbar_mode(ui->weather_main_fc_row0, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_fc_row0, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_fc_row0, LV_DIR_NONE);
    // 白底、浅蓝细边、轻阴影，圆角卡片
    lv_obj_set_style_border_width(ui->weather_main_fc_row0, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row0, lv_color_hex(0x274d6b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->weather_main_fc_row0, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->weather_main_fc_row0, lv_color_hex(0x1b64b1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui->weather_main_fc_row0, 40, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 行内四个文本：日期(12)、天气(18 粗)、温度(12)、风向风力(12) */
    ui->weather_main_fc_row0_lbl = lv_label_create(ui->weather_main_fc_row0);   /* 日期 */
    lv_obj_set_pos(ui->weather_main_fc_row0_lbl, -15, -8);
    lv_obj_set_width(ui->weather_main_fc_row0_lbl, 100);
    lv_label_set_text(ui->weather_main_fc_row0_lbl, "2025-11-14·周5");
    lv_obj_set_style_text_font(ui->weather_main_fc_row0_lbl, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row0_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    /* 日期做成浅底圆角 Chip，静态宽度 */
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row0_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row0_lbl, lv_color_hex(0x253244), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_fc_row0_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row0_lbl, lv_color_hex(0xD6E7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_fc_row0_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->weather_main_fc_row0_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->weather_main_fc_row0_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->weather_main_fc_row0_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->weather_main_fc_row0_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->weather_main_fc_row0_lbl, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row0_cond = lv_label_create(ui->weather_main_fc_row0);  /* 天气 */
    lv_obj_set_pos(ui->weather_main_fc_row0_cond, -10, 20);
    lv_obj_set_width(ui->weather_main_fc_row0_cond, 86);
    lv_label_set_text(ui->weather_main_fc_row0_cond, "晴/晴");
    lv_obj_set_style_text_font(ui->weather_main_fc_row0_cond, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row0_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row0_cond, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->weather_main_fc_row0_cond, LV_LABEL_LONG_WRAP);

    ui->weather_main_fc_row0_temp = lv_label_create(ui->weather_main_fc_row0);  /* 温度 */
    lv_obj_set_pos(ui->weather_main_fc_row0_temp, -10, 40);
    lv_label_set_text(ui->weather_main_fc_row0_temp, "1° ~ 16°");
    lv_obj_set_style_text_font(ui->weather_main_fc_row0_temp, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row0_temp, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row0_temp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row0_temp, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row0_temp, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row0_wind = lv_label_create(ui->weather_main_fc_row0);  /* 风向风力 */
    lv_obj_set_pos(ui->weather_main_fc_row0_wind, -10, 62);
    lv_label_set_text(ui->weather_main_fc_row0_wind, "风 西南/西南\n力 1-3");
    lv_obj_set_style_text_font(ui->weather_main_fc_row0_wind, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row0_wind, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row0_wind, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row0_wind, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row0_wind, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row1 = lv_obj_create(ui->weather_main_forecast);
    lv_obj_set_pos(ui->weather_main_fc_row1, 113, 22);
    lv_obj_set_size(ui->weather_main_fc_row1, 105, 115);
    lv_obj_set_style_radius(ui->weather_main_fc_row1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row1, lv_color_hex(0x182635), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(ui->weather_main_fc_row1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_fc_row1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_fc_row1, LV_DIR_NONE);
    lv_obj_set_style_border_width(ui->weather_main_fc_row1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row1, lv_color_hex(0x274d6b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->weather_main_fc_row1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->weather_main_fc_row1, lv_color_hex(0x1b64b1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui->weather_main_fc_row1, 40, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_main_fc_row1_lbl = lv_label_create(ui->weather_main_fc_row1);
    lv_obj_set_pos(ui->weather_main_fc_row1_lbl, -15, -8);
    lv_obj_set_width(ui->weather_main_fc_row1_lbl, 100);
    lv_label_set_text(ui->weather_main_fc_row1_lbl, "2025-11-15·周6");
    lv_obj_set_style_text_font(ui->weather_main_fc_row1_lbl, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row1_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row1_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row1_lbl, lv_color_hex(0x253244), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_fc_row1_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row1_lbl, lv_color_hex(0xD6E7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_fc_row1_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->weather_main_fc_row1_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->weather_main_fc_row1_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->weather_main_fc_row1_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->weather_main_fc_row1_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->weather_main_fc_row1_lbl, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row1_cond = lv_label_create(ui->weather_main_fc_row1);
    lv_obj_set_pos(ui->weather_main_fc_row1_cond, -10, 20);
    lv_obj_set_width(ui->weather_main_fc_row1_cond, 86);
    lv_label_set_text(ui->weather_main_fc_row1_cond, "晴/多云");
    lv_obj_set_style_text_font(ui->weather_main_fc_row1_cond, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row1_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row1_cond, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_main_fc_row1_temp = lv_label_create(ui->weather_main_fc_row1);
    lv_obj_set_pos(ui->weather_main_fc_row1_temp, -10, 40);
    lv_label_set_text(ui->weather_main_fc_row1_temp, "3° ~ 14°");
    lv_obj_set_style_text_font(ui->weather_main_fc_row1_temp, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row1_temp, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row1_temp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row1_temp, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row1_temp, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row1_wind = lv_label_create(ui->weather_main_fc_row1);
    lv_obj_set_pos(ui->weather_main_fc_row1_wind, -10, 62);
    lv_label_set_text(ui->weather_main_fc_row1_wind, "风 西南/西南\n力 1-3");
    lv_obj_set_style_text_font(ui->weather_main_fc_row1_wind, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row1_wind, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row1_wind, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row1_wind, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row1_wind, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row2 = lv_obj_create(ui->weather_main_forecast);
    lv_obj_set_pos(ui->weather_main_fc_row2, 221, 22);
    lv_obj_set_size(ui->weather_main_fc_row2, 105, 115);
    lv_obj_set_style_radius(ui->weather_main_fc_row2, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row2, lv_color_hex(0x182635), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(ui->weather_main_fc_row2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_fc_row2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_fc_row2, LV_DIR_NONE);
    lv_obj_set_style_border_width(ui->weather_main_fc_row2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row2, lv_color_hex(0x274d6b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->weather_main_fc_row2, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->weather_main_fc_row2, lv_color_hex(0x1b64b1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui->weather_main_fc_row2, 40, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_main_fc_row2_lbl = lv_label_create(ui->weather_main_fc_row2);
    lv_obj_set_pos(ui->weather_main_fc_row2_lbl, -15, -8);
    lv_obj_set_width(ui->weather_main_fc_row2_lbl, 100);
    lv_label_set_text(ui->weather_main_fc_row2_lbl, "2025-11-16·周7");
    lv_obj_set_style_text_font(ui->weather_main_fc_row2_lbl, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row2_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row2_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row2_lbl, lv_color_hex(0x253244), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_fc_row2_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row2_lbl, lv_color_hex(0xD6E7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_fc_row2_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->weather_main_fc_row2_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->weather_main_fc_row2_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->weather_main_fc_row2_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->weather_main_fc_row2_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->weather_main_fc_row2_lbl, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row2_cond = lv_label_create(ui->weather_main_fc_row2);
    lv_obj_set_pos(ui->weather_main_fc_row2_cond, -10, 20);
    lv_obj_set_width(ui->weather_main_fc_row2_cond, 86);
    lv_label_set_text(ui->weather_main_fc_row2_cond, "晴/晴");
    lv_obj_set_style_text_font(ui->weather_main_fc_row2_cond, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row2_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row2_cond, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_main_fc_row2_temp = lv_label_create(ui->weather_main_fc_row2);
    lv_obj_set_pos(ui->weather_main_fc_row2_temp, -10, 40);
    lv_label_set_text(ui->weather_main_fc_row2_temp, "-3° ~ 7°");
    lv_obj_set_style_text_font(ui->weather_main_fc_row2_temp, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row2_temp, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row2_temp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row2_temp, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row2_temp, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row2_wind = lv_label_create(ui->weather_main_fc_row2);
    lv_obj_set_pos(ui->weather_main_fc_row2_wind, -10, 62);
    lv_label_set_text(ui->weather_main_fc_row2_wind, "风 西北/西北\n力 1-3");
    lv_obj_set_style_text_font(ui->weather_main_fc_row2_wind, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row2_wind, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row2_wind, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row2_wind, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row2_wind, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row3 = lv_obj_create(ui->weather_main_forecast);
    lv_obj_set_pos(ui->weather_main_fc_row3, 329, 22);
    lv_obj_set_size(ui->weather_main_fc_row3, 105, 115);
    lv_obj_set_style_radius(ui->weather_main_fc_row3, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row3, lv_color_hex(0x182635), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(ui->weather_main_fc_row3, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->weather_main_fc_row3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->weather_main_fc_row3, LV_DIR_NONE);
    lv_obj_set_style_border_width(ui->weather_main_fc_row3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row3, lv_color_hex(0x274d6b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->weather_main_fc_row3, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->weather_main_fc_row3, lv_color_hex(0x1b64b1), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui->weather_main_fc_row3, 40, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_main_fc_row3_lbl = lv_label_create(ui->weather_main_fc_row3);
    lv_obj_set_pos(ui->weather_main_fc_row3_lbl, -15, -8);
    lv_obj_set_width(ui->weather_main_fc_row3_lbl, 100);
    lv_label_set_text(ui->weather_main_fc_row3_lbl, "2025-11-17·周1");
    lv_obj_set_style_text_font(ui->weather_main_fc_row3_lbl, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row3_lbl, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_main_fc_row3_lbl, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_main_fc_row3_lbl, lv_color_hex(0x253244), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_main_fc_row3_lbl, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->weather_main_fc_row3_lbl, lv_color_hex(0xD6E7F5), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->weather_main_fc_row3_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->weather_main_fc_row3_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->weather_main_fc_row3_lbl, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->weather_main_fc_row3_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->weather_main_fc_row3_lbl, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->weather_main_fc_row3_lbl, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row3_cond = lv_label_create(ui->weather_main_fc_row3);
    lv_obj_set_pos(ui->weather_main_fc_row3_cond, -10, 20);
    lv_obj_set_width(ui->weather_main_fc_row3_cond, 86);
    lv_label_set_text(ui->weather_main_fc_row3_cond, "晴/晴");
    lv_obj_set_style_text_font(ui->weather_main_fc_row3_cond, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row3_cond, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row3_cond, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_main_fc_row3_temp = lv_label_create(ui->weather_main_fc_row3);
    lv_obj_set_pos(ui->weather_main_fc_row3_temp, -10, 40);
    lv_label_set_text(ui->weather_main_fc_row3_temp, "-2° ~ 4°");
    lv_obj_set_style_text_font(ui->weather_main_fc_row3_temp, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row3_temp, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row3_temp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row3_temp, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row3_temp, LV_LABEL_LONG_CLIP);

    ui->weather_main_fc_row3_wind = lv_label_create(ui->weather_main_fc_row3);
    lv_obj_set_pos(ui->weather_main_fc_row3_wind, -10, 62);
    lv_label_set_text(ui->weather_main_fc_row3_wind, "风 北/北\n力 1-3");
    lv_obj_set_style_text_font(ui->weather_main_fc_row3_wind, &lv_font_cn_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->weather_main_fc_row3_wind, lv_color_hex( 0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->weather_main_fc_row3_wind, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_width(ui->weather_main_fc_row3_wind, 86);
    lv_label_set_long_mode(ui->weather_main_fc_row3_wind, LV_LABEL_LONG_CLIP);

    // City picker modal (hidden by default)
    ui->weather_city_modal = lv_obj_create(ui->weather_main);
    lv_obj_set_pos(ui->weather_city_modal, 0, 0);
    lv_obj_set_size(ui->weather_city_modal, 480, 320);
    lv_obj_add_flag(ui->weather_city_modal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(ui->weather_city_modal, 128, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_city_modal, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_city_modal, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->weather_city_panel = lv_obj_create(ui->weather_city_modal);
    lv_obj_set_size(ui->weather_city_panel, 440, 220);
    lv_obj_set_pos(ui->weather_city_panel, 20, 50);
    lv_obj_set_style_radius(ui->weather_city_panel, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->weather_city_panel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_city_panel, lv_color_hex(0x0c141c), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->weather_city_panel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_t *lbl_prov = lv_label_create(ui->weather_city_panel);
    lv_obj_set_pos(lbl_prov, 16, 16);
    lv_label_set_text(lbl_prov, "省");
    lv_obj_set_style_text_font(lbl_prov, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_city_dd_prov = lv_dropdown_create(ui->weather_city_panel);
    lv_obj_set_pos(ui->weather_city_dd_prov, 50, 10);
    lv_obj_set_size(ui->weather_city_dd_prov, 150, 38);
    lv_dropdown_set_options(ui->weather_city_dd_prov, "北京市\n上海市");
    lv_obj_set_style_text_font(ui->weather_city_dd_prov, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_city_dd_prov, &lv_font_cn_18, LV_PART_ITEMS|LV_STATE_DEFAULT);

    lv_obj_t *lbl_pref = lv_label_create(ui->weather_city_panel);
    lv_obj_set_pos(lbl_pref, 16, 70);
    lv_label_set_text(lbl_pref, "市");
    lv_obj_set_style_text_font(lbl_pref, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_city_dd_pref = lv_dropdown_create(ui->weather_city_panel);
    lv_obj_set_pos(ui->weather_city_dd_pref, 50, 64);
    lv_obj_set_size(ui->weather_city_dd_pref, 150, 38);
    lv_dropdown_set_options(ui->weather_city_dd_pref, "北京市");
    lv_obj_set_style_text_font(ui->weather_city_dd_pref, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_city_dd_pref, &lv_font_cn_18, LV_PART_ITEMS|LV_STATE_DEFAULT);

    lv_obj_t *lbl_county = lv_label_create(ui->weather_city_panel);
    lv_obj_set_pos(lbl_county, 220, 16);
    lv_label_set_text(lbl_county, "县/区");
    lv_obj_set_style_text_font(lbl_county, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->weather_city_dd_county = lv_dropdown_create(ui->weather_city_panel);
    lv_obj_set_pos(ui->weather_city_dd_county, 270, 10);
    lv_obj_set_size(ui->weather_city_dd_county, 150, 38);
    lv_dropdown_set_options(ui->weather_city_dd_county, "东城区\n西城区");
    lv_obj_set_style_text_font(ui->weather_city_dd_county, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->weather_city_dd_county, &lv_font_cn_18, LV_PART_ITEMS|LV_STATE_DEFAULT);

    ui->weather_city_btn_cancel = lv_button_create(ui->weather_city_panel);
    lv_obj_set_pos(ui->weather_city_btn_cancel, 220, 160);
    lv_obj_set_size(ui->weather_city_btn_cancel, 96, 40);
    lv_obj_set_style_radius(ui->weather_city_btn_cancel, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_city_btn_cancel, lv_color_hex(0x253244), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_t *lbl_cancel = lv_label_create(ui->weather_city_btn_cancel);
    lv_label_set_text(lbl_cancel, "取消");
    lv_obj_set_style_text_font(lbl_cancel, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_center(lbl_cancel);

    ui->weather_city_btn_ok = lv_button_create(ui->weather_city_panel);
    lv_obj_set_pos(ui->weather_city_btn_ok, 324, 160);
    lv_obj_set_size(ui->weather_city_btn_ok, 96, 40);
    lv_obj_set_style_radius(ui->weather_city_btn_ok, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->weather_city_btn_ok, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_t *lbl_ok = lv_label_create(ui->weather_city_btn_ok);
    lv_label_set_text(lbl_ok, "确定");
    lv_obj_set_style_text_font(lbl_ok, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_center(lbl_ok);

    // Update layout and init events (dark variant)
    lv_obj_update_layout(ui->weather_main);
    events_init_weather_main_dark(ui);
}
