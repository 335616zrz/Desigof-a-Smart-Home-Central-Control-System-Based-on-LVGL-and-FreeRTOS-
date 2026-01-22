// 最小字体声明集合
// 仅保留项目实际使用并存在定义的字体。
// 对应实现位于：my_ui/generated/guider_customer_fonts/

#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 必要字体（存在 .c 定义） */
extern const lv_font_t lv_font_cn_12;
extern const lv_font_t lv_font_cn_18;
extern const lv_font_t lv_font_cn_24;
extern const lv_font_t lv_font_cn_30;
extern const lv_font_t lv_font_cn_num_30; /* 数字专用字体 */

/* 可选 UI 便捷宏：如有需要，可统一改动到这里 */
#define FONT_UI_12  lv_font_cn_12
#define FONT_UI_18  lv_font_cn_18
#define FONT_UI_24  lv_font_cn_24
#define FONT_UI_30  lv_font_cn_30

#ifdef __cplusplus
}
#endif

/* 说明：
 * - 旧的声明（lv_font_cn_15/22/25/28）已移除，因为没有对应实现且未被使用；
 * - 若后续新增字体，请在 guider_customer_fonts 中生成 .c 并在此处补充 extern。*/
