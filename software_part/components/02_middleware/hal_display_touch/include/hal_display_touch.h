#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISP_ROT_0   = 0,  // 竖屏(默认)
    DISP_ROT_90  = 1,  // 顺时针90° 横屏
    DISP_ROT_180 = 2,
    DISP_ROT_270 = 3
} display_rotation_t;

/* 初始化：LCD + 触摸 + INT→任务通知。成功返回 true */
bool display_touch_init(void);

/* 简易绘图封装（转发至 LCD 驱动） */
void lcd_fill_rect(int x,int y,int w,int h,uint16_t c);
void lcd_draw_bitmap(int x,int y,int w,int h,const uint16_t *p);

/* 获取“已校正后”的屏幕坐标（有按压则 true） */
bool touch_get_point(int *x,int *y);

/* 运行时旋转：
 * - 同步 LCD 与触摸的方向；LVGL 的同步建议在 lvgl_port 中实现（若需要）。
 */
void hal_screen_set_rotation(display_rotation_t rot);

/* 竖<->横 快速切换（0/180 视作竖，90/270 视作横） */
void hal_screen_toggle_orientation(void);

#ifdef __cplusplus
}
#endif
