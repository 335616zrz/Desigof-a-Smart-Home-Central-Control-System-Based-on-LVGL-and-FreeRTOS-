/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef __CUSTOM_H_
#define __CUSTOM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"
#include "sdkconfig.h"
#if CONFIG_APP_ENABLE_MUSIC
#include "audio_player.h"
#endif
#include "app_env_sht40.h"    // 室内传感器读数

void custom_init(lv_ui *ui);

/* 音频场景切换辅助：在进入不同业务界面时做统一的音频互斥处理 */
void AppAudio_OnEnterMusicScreen(void);
void AppAudio_OnEnterGPTScreen(void);
void AppAudio_OnLeaveGPTScreen(void);

/* ASR 控制：供 UI 在“语音输入模式”下启用识别；文本模式下关闭以节省资源 */
void AppASR_SetEnabled(bool enabled);

/* ASR Push-To-Talk：按住说话/松开发送 */
void AppASR_SetPTTMode(bool enabled);
void AppASR_PTT_SetPressed(bool pressed);

/* 环境显示模式（室内/室外） */
void env_mode_set_indoor(bool indoor);
bool env_mode_is_indoor(void);

/* TLS 互斥：串行化音乐索引与 Chatbot 等 TLS 握手/会话创建，避免同时占用稀缺堆资源 */
void AppTLS_Lock(void);
void AppTLS_Unlock(void);

void MusicScreen_UpdatePlayState(bool playing);
void MusicScreen_OnTrackChanged(void);
void MusicScreen_RequestStep(bool forward);
bool MusicScreen_OpenSpectrum(bool dark_variant);
void MusicScreen_CloseSpectrum(void);
bool MusicScreen_IsSpectrumVisible(void);
/* 更新“当前来源：SD 卡 / 网络”标签 */
void MusicScreen_UpdateSourceLabel(bool use_sdcard);
#if CONFIG_APP_ENABLE_MUSIC
void MusicScreen_HandlePlaybackState(ap_state_t state);
#endif

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
