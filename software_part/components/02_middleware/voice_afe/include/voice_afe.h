#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VOICE_AFE_STATE_IDLE = 0,       ///< 只检测唤醒词（或纯采集）
    VOICE_AFE_STATE_WAIT_CMD,       ///< 已唤醒，等待命令词
    VOICE_AFE_STATE_LOCAL_EXEC,     ///< 本地执行命令
    VOICE_AFE_STATE_REMOTE_ASR,     ///< 把 PCM 推给远端 ASR
} voice_afe_state_t;

typedef void (*voice_afe_cmd_handler_t)(int cmd_id, const char *phrase, void *user_ctx);
typedef void (*voice_afe_pcm_sink_t)(const int16_t *pcm, size_t samples, void *user_ctx);

typedef struct {
    int                  sample_rate_hz;     ///< 采样率（默认 16000）
    bool                 enable_remote;      ///< 初始化时是否允许推流到远端 ASR
    voice_afe_cmd_handler_t on_cmd;          ///< 命令回调（MultiNet）
    void                *on_cmd_ctx;         ///< 用户上下文
    voice_afe_pcm_sink_t  pcm_sink;          ///< PCM 输出（给 WS / 录音）
    void                *pcm_sink_ctx;       ///< 用户上下文
} voice_afe_config_t;

esp_err_t voice_afe_init(const voice_afe_config_t *cfg);

void voice_afe_set_remote_enabled(bool enable);
bool voice_afe_remote_enabled(void);

void voice_afe_set_state(voice_afe_state_t s);
voice_afe_state_t voice_afe_get_state(void);

/* 运行时追加/替换回调 */
void voice_afe_set_cmd_handler(voice_afe_cmd_handler_t cb, void *user_ctx);
void voice_afe_set_pcm_sink(voice_afe_pcm_sink_t cb, void *user_ctx);

#ifdef __cplusplus
}
#endif
