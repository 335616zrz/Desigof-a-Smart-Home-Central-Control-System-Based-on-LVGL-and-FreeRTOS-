#include "music_spectrum.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define FFT_SIZE 256U
#define FFT_HALF (FFT_SIZE / 2U)

/* Doing FFT inside the I2S sink task is expensive. Throttle how often we run the
 * spectrum analysis to avoid starving audio playback when the UI is open. */
#ifndef MUSIC_SPECTRUM_PROCESS_INTERVAL_MS
#define MUSIC_SPECTRUM_PROCESS_INTERVAL_MS 40
#endif

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static float s_fifo[FFT_SIZE];
static size_t s_fifo_pos = 0;
static float s_window[FFT_SIZE];
static bool s_window_ready = false;
static uint16_t s_bitrev[FFT_SIZE];
static bool s_fft_ready = false;

static float s_levels[MUSIC_SPECTRUM_POINT_COUNT];
static float s_wave_samples[FFT_SIZE];
static uint32_t s_version = 0;
static TickType_t s_last_update_tick = 0;
static TickType_t s_last_process_tick = 0;
static uint32_t s_sample_rate = 44100;
static uint32_t s_channels = 2;
static bool s_active = false;
static float s_fft_real[FFT_SIZE];
static float s_fft_imag[FFT_SIZE];

static void init_fft_tables(void) {
    if (s_fft_ready) {
        return;
    }
    const uint32_t bits = 8; /* log2(256) */
    for (uint32_t i = 0; i < FFT_SIZE; ++i) {
        uint32_t x = i;
        uint32_t y = 0;
        for (uint32_t b = 0; b < bits; ++b) {
            y = (y << 1) | (x & 1U);
            x >>= 1U;
        }
        s_bitrev[i] = (uint16_t)y;
    }
    for (uint32_t i = 0; i < FFT_SIZE; ++i) {
        float w = 0.5f * (1.0f - cosf((2.0f * M_PI * (float)i) / (float)(FFT_SIZE - 1U)));
        s_window[i] = w;
    }
    s_window_ready = true;
    s_fft_ready = true;
}

static inline void fft_compute(float *real, float *imag) {
    for (uint32_t i = 0; i < FFT_SIZE; ++i) {
        uint32_t j = s_bitrev[i];
        if (j > i) {
            float tmp_re = real[i];
            float tmp_im = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tmp_re;
            imag[j] = tmp_im;
        }
    }

    for (uint32_t len = 2; len <= FFT_SIZE; len <<= 1U) {
        float ang = -2.0f * (float)M_PI / (float)len;
        float wlen_re = cosf(ang);
        float wlen_im = sinf(ang);
        for (uint32_t i = 0; i < FFT_SIZE; i += len) {
            float w_re = 1.0f;
            float w_im = 0.0f;
            uint32_t half = len >> 1U;
            for (uint32_t j = 0; j < half; ++j) {
                uint32_t even_idx = i + j;
                uint32_t odd_idx = even_idx + half;
                float u_re = real[even_idx];
                float u_im = imag[even_idx];
                float v_re = real[odd_idx];
                float v_im = imag[odd_idx];

                float t_re = w_re * v_re - w_im * v_im;
                float t_im = w_re * v_im + w_im * v_re;

                real[even_idx] = u_re + t_re;
                imag[even_idx] = u_im + t_im;
                real[odd_idx] = u_re - t_re;
                imag[odd_idx] = u_im - t_im;

                float next_re = w_re * wlen_re - w_im * wlen_im;
                float next_im = w_re * wlen_im + w_im * wlen_re;
                w_re = next_re;
                w_im = next_im;
            }
        }
    }
}

void music_spectrum_set_format(uint32_t sample_rate, uint32_t channels) {
    if (sample_rate > 0) {
        s_sample_rate = sample_rate;
    }
    if (channels > 0) {
        s_channels = channels;
    }
    portENTER_CRITICAL(&s_lock);
    memset(s_levels, 0, sizeof(s_levels));
    memset(s_wave_samples, 0, sizeof(s_wave_samples));
    s_version = 0;
    s_last_update_tick = xTaskGetTickCount();
    s_last_process_tick = 0;
    portEXIT_CRITICAL(&s_lock);
}

void music_spectrum_set_active(bool active) {
    s_active = active;
    portENTER_CRITICAL(&s_lock);
    s_fifo_pos = 0;
    memset(s_fifo, 0, sizeof(s_fifo));
    memset(s_levels, 0, sizeof(s_levels));
    memset(s_wave_samples, 0, sizeof(s_wave_samples));
    s_version = 0;
    s_last_update_tick = xTaskGetTickCount();
    s_last_process_tick = 0;
    portEXIT_CRITICAL(&s_lock);
}

static inline float clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}

static void process_buffer(void) {
    if (!s_window_ready) {
        init_fft_tables();
    }

    for (uint32_t i = 0; i < FFT_SIZE; ++i) {
        float sample = s_fifo[i];
        if (s_window_ready) {
            sample *= s_window[i];
        }
        s_fft_real[i] = sample;
        s_fft_imag[i] = 0.0f;
    }

    fft_compute(s_fft_real, s_fft_imag);

    float mags[FFT_HALF];
    float total_energy = 0.0f;
    for (uint32_t i = 0; i < FFT_HALF; ++i) {
        float re = s_fft_real[i];
        float im = s_fft_imag[i];
        float mag = sqrtf(re * re + im * im) / (float)FFT_HALF;
        mags[i] = mag;
        total_energy += mag;
    }

    float new_levels[MUSIC_SPECTRUM_POINT_COUNT] = {0};
    const uint32_t bins_per_bar = FFT_HALF / MUSIC_SPECTRUM_POINT_COUNT;
    for (uint32_t band = 0; band < MUSIC_SPECTRUM_POINT_COUNT; ++band) {
        uint32_t start = band * bins_per_bar;
        uint32_t end = start + bins_per_bar;
        if (end <= start) {
            end = start + 1U;
        }
        if (end > FFT_HALF) {
            end = FFT_HALF;
        }

        float accum = 0.0f;
        for (uint32_t k = start; k < end; ++k) {
            accum += mags[k];
        }
        float avg = accum / (float)(end - start);
        float scaled = avg * 6.0f;
        if (scaled > 1.2f) {
            scaled = 1.2f;
        }
        new_levels[band] = clamp01(scaled);
    }

    TickType_t now = xTaskGetTickCount();
    portENTER_CRITICAL(&s_lock);
    for (uint32_t i = 0; i < MUSIC_SPECTRUM_POINT_COUNT; ++i) {
        float prev = s_levels[i];
        float target = new_levels[i];
        if (target > prev) {
            prev += (target - prev) * 0.5f;
        } else {
            prev = prev * 0.85f + target * 0.15f;
        }
        s_levels[i] = clamp01(prev);
    }
    memcpy(s_wave_samples, s_fifo, sizeof(float) * FFT_SIZE);
    s_version++;
    s_last_update_tick = now;
    portEXIT_CRITICAL(&s_lock);
}

void music_spectrum_push_pcm(const int16_t *samples, size_t frames) {
    if (!samples || frames == 0) {
        return;
    }
    if (!s_active) {
        return;
    }
    if (s_channels == 0) {
        s_channels = 2;
    }

    const uint32_t channels = s_channels;
    const float inv_norm = 1.0f / (32768.0f * (float)channels);

    for (size_t i = 0; i < frames; ++i) {
        int32_t sum = 0;
        size_t base = i * channels;
        for (uint32_t ch = 0; ch < channels; ++ch) {
            sum += samples[base + ch];
        }
        float norm = (float)sum * inv_norm;
        if (norm > 1.2f) {
            norm = 1.2f;
        } else if (norm < -1.2f) {
            norm = -1.2f;
        }

        s_fifo[s_fifo_pos++] = norm;
        if (s_fifo_pos >= FFT_SIZE) {
            const TickType_t now = xTaskGetTickCount();
            const TickType_t min_interval = pdMS_TO_TICKS(MUSIC_SPECTRUM_PROCESS_INTERVAL_MS);
            const bool do_process = (s_last_process_tick == 0) || (min_interval == 0) ||
                                    ((now - s_last_process_tick) >= min_interval);
            if (do_process) {
                process_buffer();
                s_last_process_tick = now;
            }
            const size_t overlap = FFT_SIZE / 2U;
            memmove(s_fifo, &s_fifo[overlap], overlap * sizeof(float));
            s_fifo_pos = overlap;
        }
    }
}

bool music_spectrum_snapshot(music_spectrum_frame_t *frame) {
    if (!frame) {
        return false;
    }

    static uint32_t s_prev_version = 0;
    TickType_t now = xTaskGetTickCount();
    float bars_tmp[MUSIC_SPECTRUM_POINT_COUNT];
    float wave_tmp[FFT_SIZE];
    uint32_t version_snapshot;

    portENTER_CRITICAL(&s_lock);
    if (s_version == 0) {
        memset(s_levels, 0, sizeof(s_levels));
        memset(s_wave_samples, 0, sizeof(s_wave_samples));
        s_last_update_tick = now;
    }
    if (s_version == s_prev_version) {
        TickType_t delta = now - s_last_update_tick;
        if (delta > pdMS_TO_TICKS(60)) {
            for (uint32_t i = 0; i < MUSIC_SPECTRUM_POINT_COUNT; ++i) {
                s_levels[i] *= 0.92f;
                if (s_levels[i] < 0.001f) {
                    s_levels[i] = 0.0f;
                }
            }
            s_last_update_tick = now;
        }
    }
    memcpy(bars_tmp, s_levels, sizeof(bars_tmp));
    memcpy(wave_tmp, s_wave_samples, sizeof(wave_tmp));
    version_snapshot = s_version;
    portEXIT_CRITICAL(&s_lock);

    for (uint32_t i = 0; i < MUSIC_SPECTRUM_POINT_COUNT; ++i) {
        float v = clamp01(bars_tmp[i]);
        frame->bars[i] = (int16_t)lrintf(v * 120.0f);
    }

    for (uint32_t i = 0; i < MUSIC_SPECTRUM_BAND_COUNT; ++i) {
        frame->bands[i] = 0;
    }

    for (uint32_t i = 0; i < MUSIC_SPECTRUM_WAVE_SAMPLES; ++i) {
        float sample = wave_tmp[i];
        if (sample > 1.2f) {
            sample = 1.2f;
        } else if (sample < -1.2f) {
            sample = -1.2f;
        }
        frame->waveform[i] = (int16_t)lrintf(sample * 16384.0f);
    }

    frame->total_energy = 0;
    frame->dominant_band = 0;
    frame->has_energy = false;

    bool has_new = (version_snapshot != s_prev_version);
    s_prev_version = version_snapshot;
    return has_new;
}
