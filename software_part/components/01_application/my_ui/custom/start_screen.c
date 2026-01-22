#include "start_screen.h"

#include "cute_devil_img.h"
#include "bg_noise_tile.h"

static void obj_set_translate_y(void *var, int32_t y)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_translate_y(obj, y, 0);
}

static void obj_set_translate_x(void *var, int32_t x)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_translate_x(obj, x, 0);
}

void start_screen_create(start_screen_t *ui)
{
    if(ui == NULL) return;

    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xFDFEFF), 0);
    lv_obj_set_style_bg_grad_color(ui->screen, lv_color_hex(0xC9E0FF), 0);
    lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_VER, 0);

    lv_obj_set_style_bg_image_src(ui->screen, &bg_noise_tile, 0);
    lv_obj_set_style_bg_image_tiled(ui->screen, true, 0);
    lv_obj_set_style_bg_image_recolor(ui->screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_image_opa(ui->screen, 30, 0);

    /* Simple 2D devil image directly on screen */
    ui->devil_img = lv_image_create(ui->screen);
    lv_image_set_src(ui->devil_img, &cute_devil_img);
    lv_obj_align(ui->devil_img, LV_ALIGN_CENTER, 0, -10);
    lv_image_set_antialias(ui->devil_img, false);

    /* Vertical bob animation */
    lv_anim_t bob;
    lv_anim_init(&bob);
    lv_anim_set_var(&bob, ui->devil_img);
    lv_anim_set_exec_cb(&bob, obj_set_translate_y);
    lv_anim_set_values(&bob, -7, 7);
    lv_anim_set_time(&bob, 1200);
    lv_anim_set_playback_time(&bob, 1200);
    lv_anim_set_repeat_count(&bob, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&bob, lv_anim_path_ease_in_out);
    lv_anim_start(&bob);

    /* Horizontal jump animation */
    lv_anim_t jump;
    lv_anim_init(&jump);
    lv_anim_set_var(&jump, ui->devil_img);
    lv_anim_set_exec_cb(&jump, obj_set_translate_x);
    lv_anim_set_values(&jump, -50, 50);
    lv_anim_set_time(&jump, 1500);
    lv_anim_set_playback_time(&jump, 1500);
    lv_anim_set_repeat_count(&jump, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&jump, lv_anim_path_ease_in_out);
    lv_anim_start(&jump);

    ui->bar = lv_bar_create(ui->screen);
    lv_obj_set_size(ui->bar, 240, 8);
    lv_obj_align(ui->bar, LV_ALIGN_BOTTOM_MID, 0, -26);
    lv_bar_set_range(ui->bar, 0, 100);
    lv_bar_set_value(ui->bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(ui->bar, 999, 0);
    lv_obj_set_style_bg_color(ui->bar, lv_color_hex(0xDDE7F6), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ui->bar, lv_color_hex(0xC7D4EA), LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui->bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->bar, lv_color_hex(0x3D7BFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(ui->bar, lv_color_hex(0x00D1C1), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(ui->bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
}

void start_screen_set_progress(start_screen_t *ui, int32_t progress_0_100)
{
    if(ui == NULL || ui->bar == NULL) return;
    if(progress_0_100 < 0) progress_0_100 = 0;
    if(progress_0_100 > 100) progress_0_100 = 100;
    lv_bar_set_value(ui->bar, progress_0_100, LV_ANIM_OFF);
}

void start_screen_set_status(start_screen_t *ui, const char *text)
{
    (void)ui;
    (void)text;
}
