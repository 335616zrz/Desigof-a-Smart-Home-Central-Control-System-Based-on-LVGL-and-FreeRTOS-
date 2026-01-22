#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 与主控同步的事件位（Wi-Fi 就绪） */
#define APP_MUSIC_EVT_WIFI_READY  (1u << 0)

/* 音乐任务入口：
 * - 参数：传入 EventGroupHandle_t（g_sys_evt），用于等待 Wi-Fi 就绪；
 *         若传入 NULL，则任务内部退化为轮询检测 IP。
 * - 任务在启动播放调用后自删除（vTaskDelete(NULL)）。
 */
void app_music_task(void *sys_evt_arg);

#ifdef __cplusplus
}
#endif

