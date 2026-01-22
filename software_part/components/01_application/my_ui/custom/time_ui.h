#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void time_ui_init(void);
/* 绑定两个已有的标签：左为时间(HH:MM:SS)，右为日期(YYYY-MM-DD) */
void time_ui_attach(lv_obj_t *label_time, lv_obj_t *label_date);
/* 拿到 IP 后启动 SNTP；tz 例如 "CST-8"，传 NULL 用默认 */
void time_ui_start_sntp(const char *tz_str);

void time_ui_detach(void);

/* 兼容 GUI Guider 生成代码的旧接口名 */
void time_ui_bind_labels(lv_obj_t *label_time, lv_obj_t *label_date);
void time_ui_bind_week_label(lv_obj_t *lbl_week);   // 绑定星期标签（可选）
void time_ui_refresh_now(void);
void time_ui_detach_if_screen(lv_obj_t *root);

/* 额外扩展：绑定一个“日期+时间”合并显示的标签（格式：YYYY-MM-DD HH:MM:SS）。
 * 可多次绑定多个标签，模块会在每秒 tick 时统一更新。*/
void time_ui_bind_datetime_label(lv_obj_t *lbl_datetime);

#ifdef __cplusplus
}
#endif
