#pragma once
#include "lvgl.h"
#include "gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_ui_init(lv_ui *ui);

/* 供事件回调调用 */
void wifi_ui_toggle(bool on);
void wifi_ui_scan(void);
void wifi_ui_back(void);
void wifi_ui_connect_from_dialog(void);
void wifi_ui_deinit(void);   /* 新增：反注册事件并释放扫描缓存 */

/* 启动一次“按 MRU 已保存网络自动连接”，并在给定超时内等待是否获得 IP。
 * 返回值：true 表示在超时内已连上（拿到 IP）；false 表示无保存网络或未连上。 */
bool wifi_ui_connect_mru_saved_once(uint32_t wait_ms);

/* 查询当前是否已连接（已关联并获取 IP） */
bool wifi_ui_is_connected(void);

#ifdef __cplusplus
}
#endif
