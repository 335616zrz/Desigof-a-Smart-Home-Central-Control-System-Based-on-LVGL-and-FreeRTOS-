#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/** 扫描一次并以 CSV 打印结果（阻塞等待扫描完成） */
void net_wifi_scan_dump_csv(void);

/** 若你想在连接后自动扫一次，可在 app_main 里调用；或结合事件回调。 */
void net_wifi_scan_dump_csv_if_enabled(void);

#ifdef __cplusplus
}
#endif

