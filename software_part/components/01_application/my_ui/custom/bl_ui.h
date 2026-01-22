#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 若你的背光驱动接收 0~255 或 PWM 占空，重定义这个宏就能全局生效 */
#ifndef BL_UI_PERCENT_TO_HW
#define BL_UI_PERCENT_TO_HW(p)  ((uint8_t)(p))   /* 默认：驱动接受 0~100 百分比 */
#endif

/* 初始化：加载 NVS 保存的亮度并立刻应用到背光 */
void bl_ui_init(void);

/* 把 Arc 绑定为背光控制器（显示为保存值；拖动实时调亮度；松手写入 NVS） */
void bl_ui_bind_arc(lv_obj_t *arc);

/* 若需要从代码里直接设置/读取亮度（0~100） */
void bl_ui_set_percent(int percent, bool apply_now);
int  bl_ui_get_percent(void);

#ifdef __cplusplus
}
#endif

