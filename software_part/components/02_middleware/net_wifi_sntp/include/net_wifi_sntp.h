#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 上电初始化 + 启动 Wi-Fi（STA）+ 自动重连 + SNTP。
 *  非阻塞：内部事件组与回调自动推进流程。
 */
esp_err_t net_start(void);

/** 阻塞等待拿到 IP（返回 true=成功） */
bool net_wait_ip(TickType_t ticks_to_wait);

/** 阻塞等待完成首次校时（返回 true=成功） */
bool net_wait_time_synced(TickType_t ticks_to_wait);

/** 当前是否已连接（Got IP） */
bool net_is_connected(void);

/** 主动断开（应用希望停止联网时使用，停止自动重连） */
void net_disconnect(void);

/** 主动触发一次重连（例如 UI 上“重试”按钮） */
esp_err_t net_force_reconnect(void);

/** 切换/连接到指定 AP（STA）
 *
 * - 由本模块统一调用 esp_wifi_set_config/esp_wifi_connect，避免与 UI/其它模块抢占；
 * - 若 Wi-Fi 未启动会自动启动（esp_wifi_start）；
 * - 会取消当前退避重连定时器，并清零退避计数；
 * - 当需要从“已保存网络”或“扫描列表”手动连接时使用。
 *
 * @param ssid      目标 SSID（不能为空）
 * @param password  密码；开放网络可传 NULL 或空串
 */
esp_err_t net_connect_to(const char *ssid, const char *password);

// 运行时切换低时延（true=低时延/WIFI_PS_NONE，false=省电/WIFI_PS_MIN_MODEM）
esp_err_t net_set_low_latency(bool enable);

// 查询是否处于低时延模式（true=WIFI_PS_NONE）
bool net_is_low_latency(void);

#ifdef __cplusplus
}
#endif
