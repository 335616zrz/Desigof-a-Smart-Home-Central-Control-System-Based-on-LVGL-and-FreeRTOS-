#pragma once

#include <lvgl.h>

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *devil_img;
    lv_obj_t *bar;
} start_screen_t;

void start_screen_create(start_screen_t *ui);
void start_screen_set_progress(start_screen_t *ui, int32_t progress_0_100);
void start_screen_set_status(start_screen_t *ui, const char *text);
