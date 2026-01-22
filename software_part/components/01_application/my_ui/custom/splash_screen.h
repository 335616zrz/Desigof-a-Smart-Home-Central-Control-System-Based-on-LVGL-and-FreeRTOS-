#pragma once
#include "gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 显示启动屏（动画 + 进度条），并自动尝试一次按“已保存网络(MRU)”连接。
 * - 成功或失败都会在尝试完成后切换到首页（浅色）。
 * - 该函数不阻塞 LVGL 线程；内部创建短任务执行连接与路由。 */
void splash_show_and_autoconnect(lv_ui *ui);

#ifdef __cplusplus
}
#endif
