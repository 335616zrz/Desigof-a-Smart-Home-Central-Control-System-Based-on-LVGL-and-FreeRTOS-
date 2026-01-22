#pragma once
#include "lvgl.h"
#include "gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化并缓存通用样式（不改变视觉，仅把生成代码里的属性集中到共享样式）。*/
void music_styles_init(void);

/* 给进度条追加共享样式（MAIN/INDICATOR/KNOB 三个部分）。*/
void music_styles_apply_progress(lv_obj_t *slider);

/* 对整页通用控件追加样式（当前仅处理进度条）。*/
static inline void music_styles_apply_for_ui(lv_ui *ui)
{
    if (!ui) return;
    music_styles_apply_progress(ui->MusicScreen_music_slider_prog);
}

#ifdef __cplusplus
}
#endif

