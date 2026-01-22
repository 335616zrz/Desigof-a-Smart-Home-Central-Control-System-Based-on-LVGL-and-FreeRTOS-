#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** WS2812B 应用层任务：处理 MQTT 控制/告警指示，并驱动板载单颗灯珠。 */
void app_ws2812_task(void *arg);

#ifdef __cplusplus
}
#endif

