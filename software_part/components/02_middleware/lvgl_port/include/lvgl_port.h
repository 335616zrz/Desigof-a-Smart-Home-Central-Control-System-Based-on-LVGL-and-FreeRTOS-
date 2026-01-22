#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool        lvgl_port_init(void);
lv_indev_t* lvgl_port_get_indev(void);

/* 新增：用于跨任务安全调用 LVGL 的互斥接口 */
bool lvgl_port_lock(uint32_t timeout_ms);   /* timeout_ms=0 表示不等待，建议 UI 初始化时用 0 或小超时 */
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif

