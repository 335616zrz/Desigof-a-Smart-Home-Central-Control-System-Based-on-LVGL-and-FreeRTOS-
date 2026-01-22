#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 事件类型（给 UI / 上层用） */
typedef enum {
  AP_EVT_STATE = 1,   /* 负载：ap_state_t* */
  AP_EVT_INFO = 2,    /* 负载：ap_music_info_t* */
  AP_EVT_ERROR = 3,   /* 负载：const char*（错误信息） */
  AP_EVT_FINISHED = 4 /* 负载：NULL，曲目自然结束 */
} ap_event_t;

/* 播放器状态 */
typedef enum {
  AP_STATE_IDLE = 0,
  AP_STATE_LOADING,
  AP_STATE_PLAYING,
  AP_STATE_PAUSED,
  AP_STATE_STOPPED,
  AP_STATE_ERROR,
} ap_state_t;

/* 曲目信息（来自解码器 MUSIC_INFO） */
typedef struct {
  int sample_rate;
  int channels;
  int bits;
} ap_music_info_t;

/* 事件回调 */
typedef void (*ap_event_cb_t)(ap_event_t evt, const void *payload, void *user);

/* 对外 API */
bool ap_init(ap_event_cb_t cb, void *user);
bool ap_deinit(void);

bool ap_play_url(const char *url);
bool ap_stop(void);
bool ap_pause(void);
bool ap_resume(void);

void ap_set_initial_volume(int percent);
ap_state_t ap_get_state(void);
void ap_set_volume(int percent);
int ap_get_volume(void);
void ap_set_track_duration(uint32_t duration_ms);
uint32_t ap_get_track_duration(void);
bool ap_seek_to_ms(uint32_t position_ms);
/* 获取当前实际播放 URL（可能已发生 HTTPS->HTTP 回退）。返回内部指针，请勿修改/释放。*/
const char *ap_get_current_url(void);

#ifdef __cplusplus
}
#endif
