#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void     music_progress_set_total(uint32_t total_ms);
void     music_progress_set_dragging(bool dragging);
void     music_progress_preview(uint32_t position_ms);
void     music_progress_commit(uint32_t position_ms);
void     music_progress_set_playing(bool playing);
uint32_t music_progress_total_ms(void);
uint32_t music_progress_elapsed_ms(void);

#ifdef __cplusplus
}
#endif
