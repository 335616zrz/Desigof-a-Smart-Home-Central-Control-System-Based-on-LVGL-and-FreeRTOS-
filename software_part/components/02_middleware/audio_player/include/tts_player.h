// Simple PCM TTS streaming player over I2S (mono PCM16 -> stereo)
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start a TTS playback session. Will set I2S sample rate and spawn a worker task.
// If music pipeline is active, caller should stop/pause it before begin.
bool tts_player_begin(int sample_rate_hz);

// Feed mono PCM16 bytes. Non-blocking: returns false if queue is full.
bool tts_player_feed(const uint8_t *pcm_mono_le, size_t nbytes);

// Finish playback and release resources.
void tts_player_end(void);

// 查询 TTS 是否活跃（是否存在播放任务）
bool tts_player_is_active(void);

// 立刻停止 TTS：丢弃队列、尽快让出 I2S（用于用户主动播放音乐时的音频焦点切换）
void tts_player_stop_now(void);

#ifdef __cplusplus
}
#endif
