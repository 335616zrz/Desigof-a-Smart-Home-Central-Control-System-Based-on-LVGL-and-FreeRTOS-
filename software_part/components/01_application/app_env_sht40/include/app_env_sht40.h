#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FreeRTOS 任务入口：周期性读取 SHT40，并做滤波与日志输出 */
void app_env_sht40_task(void *arg);

/* 获取最近一次滤波后的温湿度值（室内传感器）
 * - 返回 true 表示 *t_c / *rh 已填充有效值；
 * - 若尚未读取到有效样本，返回 false。*/
bool app_env_sht40_get_latest(float *t_c, float *rh);

#ifdef __cplusplus
}
#endif

