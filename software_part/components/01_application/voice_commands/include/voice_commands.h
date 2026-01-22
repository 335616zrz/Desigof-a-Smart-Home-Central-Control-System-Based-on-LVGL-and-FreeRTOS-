#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化并注册默认命令映射
 *
 * - 依赖 voice_afe 已启用
 * - 默认命令：
 *   0: 连接网络
 *   1: 深色界面
 *   2: 浅色界面
 *   3: 播放网络歌曲
 *   4: 播放本地音乐
 *   5: 室内多少度
 *   6: 外面什么天气
 */
esp_err_t voice_commands_init(void);

#ifdef __cplusplus
}
#endif
