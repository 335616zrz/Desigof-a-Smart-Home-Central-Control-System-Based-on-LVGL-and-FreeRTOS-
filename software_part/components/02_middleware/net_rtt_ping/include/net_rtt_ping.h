#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *target;        // 目标，可为域名或IPv4字面量，如 "192.168.0.1"/"www.espressif.com"
    uint32_t    count;         // 探测次数（如 300）；=0 表示无限（不建议论文采样）
    uint32_t    interval_ms;   // 间隔（如 200）
    uint32_t    timeout_ms;    // 单次超时（如 1000）
    uint32_t    data_size;     // 负载大小（如 56）
    bool        print_header;  // 是否打印CSV表头
} net_ping_csv_cfg_t;

// 启动一次采样，会在串口打印以 "PING_CSV," 开头的CSV行；若已有会话在跑，先停止再启动。
esp_err_t ping_csv_start(const net_ping_csv_cfg_t *cfg);

// 若会话在运行
bool ping_csv_running(void);

// 停止会话（若 count=0 无穷采样，用此结束）
void ping_csv_stop(void);

#ifdef __cplusplus
}
#endif

