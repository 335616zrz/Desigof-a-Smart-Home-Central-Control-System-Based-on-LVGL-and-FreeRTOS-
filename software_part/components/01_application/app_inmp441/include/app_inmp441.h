#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FreeRTOS 任务入口：周期性从 INMP441 读取音频样本并打印简单特征 */
void app_inmp441_task(void *arg);

/**
 * @brief 启用/禁用 ASR 功能
 *
 * 当 enabled=true 时，音频采集任务会连接 ASR WebSocket 并发送音频数据；
 * 当 enabled=false 时，会断开 ASR WebSocket，但音频采集任务继续运行（不发送数据）。
 *
 * @param enabled true=启用 ASR，false=禁用 ASR
 */
void app_inmp441_asr_set_enabled(bool enabled);

/**
 * @brief 获取 ASR 是否启用
 * @return true=已启用，false=已禁用
 */
bool app_inmp441_asr_is_enabled(void);

/**
 * @brief 设置 ASR 是否使用 PTT（按住说话/松开发送）模式
 *
 * - enabled=false：保持原有逻辑（持续推流 + VAD 静音收口发送 stop）
 * - enabled=true ：仅在 PTT 按下期间推流；松开时立即发送 stop
 *
 * @param enabled true=PTT 模式，false=非 PTT 模式
 */
void app_inmp441_asr_set_ptt_mode(bool enabled);

/**
 * @brief PTT 按键状态（由 UI 在按下/松开事件中设置）
 *
 * 仅在 app_inmp441_asr_set_ptt_mode(true) 时生效。
 *
 * @param pressed true=按下（开始说话），false=松开（结束并发送）
 */
void app_inmp441_asr_ptt_set_pressed(bool pressed);

/**
 * @brief 获取 PTT 是否按下
 * @return true=按下，false=松开
 */
bool app_inmp441_asr_ptt_is_pressed(void);

/**
 * @brief 开始录制音频到 SD 卡（用于诊断音频质量）
 *
 * 录制 AEC 处理后的音频（与发送给 ASR 的相同），保存为 WAV 格式。
 * 文件路径：/sdcard/mic_debug_XXXXXX.wav（XXXXXX 为时间戳）
 *
 * @param duration_sec 录制时长（秒），最大 30 秒
 * @return true=开始录制，false=已在录制或参数错误
 */
bool app_inmp441_record_start(int duration_sec);

/**
 * @brief 停止录制音频
 */
void app_inmp441_record_stop(void);

/**
 * @brief 获取录制状态
 * @return true=正在录制，false=未录制
 */
bool app_inmp441_record_is_active(void);

#ifdef __cplusplus
}
#endif
