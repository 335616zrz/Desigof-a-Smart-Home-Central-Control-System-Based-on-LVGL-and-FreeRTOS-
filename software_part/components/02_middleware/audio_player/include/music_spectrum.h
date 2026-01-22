#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUSIC_SPECTRUM_POINT_COUNT 32
#define MUSIC_SPECTRUM_BAND_COUNT 24
#define MUSIC_SPECTRUM_WAVE_SAMPLES 256

typedef struct {
    int16_t  bars[MUSIC_SPECTRUM_POINT_COUNT];
    uint16_t bands[MUSIC_SPECTRUM_BAND_COUNT];
    int16_t  waveform[MUSIC_SPECTRUM_WAVE_SAMPLES];
    uint16_t total_energy;
    uint8_t  dominant_band;
    bool     has_energy;
} music_spectrum_frame_t;

void music_spectrum_set_format(uint32_t sample_rate, uint32_t channels);
void music_spectrum_push_pcm(const int16_t *samples, size_t frames);
void music_spectrum_set_active(bool active);
bool music_spectrum_snapshot(music_spectrum_frame_t *frame);

#ifdef __cplusplus
}
#endif
