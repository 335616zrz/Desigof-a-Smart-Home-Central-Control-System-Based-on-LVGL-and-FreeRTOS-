#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化音量管理（读取 NVS，设置播放器初始音量）。幂等。 */
void music_volume_init(void);

/* 播放器初始化完成后调用，一次即可，确保实际音量与记忆值一致。 */
void music_volume_on_audio_ready(void);

/* 绑定 UI 滑条，自动同步当前音量并接管事件。 */
void music_volume_bind_slider(lv_obj_t *slider);

/* 读取当前音量百分比（0-100）。 */
int  music_volume_get_percent(void);

/* 设置音量（不会写 NVS），from_ui 表示是否来自 UI 交互。 */
void music_volume_set_percent(int percent, bool from_ui);

/* 将当前音量写入 NVS（在用户松手时调用）。 */
void music_volume_commit(void);

#ifdef __cplusplus
}
#endif
