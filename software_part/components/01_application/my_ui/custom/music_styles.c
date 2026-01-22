#include "music_styles.h"

static bool s_inited = false;
static lv_style_t s_prog_main;
static lv_style_t s_prog_ind;
static lv_style_t s_prog_knob;

void music_styles_init(void)
{
    if (s_inited) return;
    s_inited = true;

    lv_style_init(&s_prog_main);
    /* 与生成代码一致：bg_opa=60, bg_color=#2195f6, radius=8, no outline/shadow */
    lv_style_set_bg_opa(&s_prog_main, 60);
    lv_style_set_bg_color(&s_prog_main, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&s_prog_main, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&s_prog_main, 8);
    lv_style_set_outline_width(&s_prog_main, 0);
    lv_style_set_shadow_width(&s_prog_main, 0);

    lv_style_init(&s_prog_ind);
    lv_style_set_bg_opa(&s_prog_ind, LV_OPA_COVER);
    lv_style_set_bg_color(&s_prog_ind, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&s_prog_ind, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&s_prog_ind, 8);

    lv_style_init(&s_prog_knob);
    lv_style_set_bg_opa(&s_prog_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&s_prog_knob, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&s_prog_knob, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&s_prog_knob, 8);
}

void music_styles_apply_progress(lv_obj_t *slider)
{
    if (!slider || !lv_obj_is_valid(slider)) return;
    music_styles_init();
    /* 追加共享样式（不会改变外观，因为属性与现有设置一致；后续若生成代码减少
       重复 set_style，也能保证视觉保持）。*/
    lv_obj_add_style(slider, &s_prog_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(slider, &s_prog_ind,  LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(slider, &s_prog_knob, LV_PART_KNOB | LV_STATE_DEFAULT);
}

