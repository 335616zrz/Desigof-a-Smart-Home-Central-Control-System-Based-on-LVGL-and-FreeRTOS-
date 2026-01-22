// Weather service HTTP client: fetch live and forecast via services/weather-service
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "gui_guider.h"

// 绑定 UI 以便在请求完成后更新控件
void weather_bind_ui(lv_ui *ui);

// 根据 adcode 拉取天气（会在独立任务中执行 HTTP 请求并更新 UI）
bool weather_update_for_adcode(uint32_t adcode);

// 记录“上次使用”的 adcode，供自动刷新使用
void weather_set_last_adcode(uint32_t adcode);

// 启动后台定时刷新（毫秒），周期到达时按最后一次设置的 adcode 拉取
bool weather_periodic_start(uint32_t period_ms);

// 停止后台定时刷新（如不需要可不调用）
void weather_periodic_stop(void);

/* 启动“开机短重试”序列：
 * - 在拿到 IP 的最初 ~60 秒内，按 2s→5s→10s→20s 的阶梯计划，
 *   每次触发调用 weather_update_for_adcode(s_last_adcode)。
 * - 任何一次拉取成功则自动停止，不影响 5 分钟长周期刷新。
 */
void weather_short_retry_bootstrap(void);

/* 主动停止开机短重试（例如在首次成功后调用） */
void weather_short_retry_stop(void);

/* 在已有一帧成功天气数据缓存的前提下，将缓存中的“实时温度/湿度”
 * 重新写回首页（浅色/深色）的温湿度数字标签。
 * - 仅在当前为“室外模式”时生效（室内模式下由 SHT40 驱动负责），
 * - 若尚未有成功数据或 UI 尚未绑定，则为 no-op。*/
void weather_home_apply_from_cache(void);
