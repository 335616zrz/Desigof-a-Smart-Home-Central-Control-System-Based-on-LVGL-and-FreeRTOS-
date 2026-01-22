#pragma once
#include "gui_guider.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 通用后置初始化：
 * - 统一设置 g_music_screen_ui 指向传入的 ui（与主题无关）
 * - 附加通用样式（不改变外观，仅复用）
 * - 刷新模式/收藏图标；确保列表绑定
 */
void MusicScreen_CommonPostSetup(lv_ui *ui);

/* 更新“当前来源：SD 卡 / 网络”标签 */
void MusicScreen_UpdateSourceLabel(bool use_sdcard);

#ifdef __cplusplus
}
#endif
