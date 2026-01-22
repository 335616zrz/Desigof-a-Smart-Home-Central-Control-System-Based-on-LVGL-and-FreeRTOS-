/**
 * 简单的"启动就绪"事件组：
 * - 两个 bit：Wi-Fi、Music
 * - 各模块在内部初始化完成时置位
 * - 启动动画 / splash 屏可以一次性等待所有 bit 就绪（或超时）再进入首页
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 两个就绪标志位 */
#define STARTUP_READY_WIFI      (1u << 0)
#define STARTUP_READY_MUSIC     (1u << 1)

/** 获取（惰性创建）全局事件组句柄 */
EventGroupHandle_t startup_ready_group(void);

/** 由各模块在就绪时调用：置位一个或多个 READY bit */
static inline void startup_ready_set(EventBits_t bits)
{
    EventGroupHandle_t g = startup_ready_group();
    if (g) {
        (void)xEventGroupSetBits(g, bits);
    }
}

/** 读取当前 ready bits，用于调试/查询 */
static inline EventBits_t startup_ready_get(void)
{
    EventGroupHandle_t g = startup_ready_group();
    return g ? xEventGroupGetBits(g) : 0;
}

#ifdef __cplusplus
}
#endif

