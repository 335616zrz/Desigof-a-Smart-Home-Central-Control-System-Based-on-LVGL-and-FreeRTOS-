#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 幂等初始化：重复调用安全（内部自防重） */
bool app_ui_init(void);

/* UI 任务入口：
 * - 在独立任务上下文中完成 UI 初始化后自删除；
 * - 由 app_main 直接 xTaskCreatePinnedToCore 调度。
 */
void app_ui_task(void *arg);

#ifdef __cplusplus
}
#endif
