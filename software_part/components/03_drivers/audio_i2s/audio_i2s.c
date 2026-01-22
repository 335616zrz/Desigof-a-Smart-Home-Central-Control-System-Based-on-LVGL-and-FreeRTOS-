#include <string.h>
#include <stdbool.h>

#include "audio_i2s.h"
#include "board_config.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "soc/soc_caps.h"            // SOC_I2S_SUPPORTS_APLL
#include "esp_heap_caps.h"           // esp_ptr_dma_capable

/* ================= 可配选项（仅本组件） ================= */
#ifndef AUDIO_I2S_USE_APLL
#define AUDIO_I2S_USE_APLL 1         /* 若 SoC 不支持将自动回退 */
#endif
#ifndef AUDIO_I2S_CHUNK_FRAMES
#define AUDIO_I2S_CHUNK_FRAMES 1024
#endif
#ifndef AUDIO_I2S_SLOT_BIT_WIDTH
#define AUDIO_I2S_SLOT_BIT_WIDTH I2S_SLOT_BIT_WIDTH_16BIT
#endif
#ifndef AUDIO_I2S_DMA_DESC_NUM
#define AUDIO_I2S_DMA_DESC_NUM BOARD_I2S_OUT_DMA_DESC_NUM
#endif
#ifndef AUDIO_I2S_DMA_FRAME_NUM
#define AUDIO_I2S_DMA_FRAME_NUM BOARD_I2S_OUT_DMA_FRAME_NUM
#endif
/* ================= 可选日志（默认关闭） ================= */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  static const char *DRV_LOG_TAG = "AUDIO_I2S";
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif
/* ====================================================== */

static i2s_chan_handle_t s_tx          = NULL;
static int               s_sample_rate = 0;
static int               s_vol_q15     = 32767;   // 100% -> Q15
static int               s_vol_percent = 100;     // 0..100
static bool              s_muted       = false;
/* Cross-task flag (audio pipeline tasks + control tasks). Keep volatile to avoid stale reads. */
static volatile bool     s_enabled     = false;

/* 静态 DMA 安全缓冲（内部 RAM） */
static int16_t s_tmp_scaled[AUDIO_I2S_CHUNK_FRAMES * 2];
static int16_t s_zero_stereo[AUDIO_I2S_CHUNK_FRAMES * 2] = {0};

/*==================== AEC 参考信号缓冲区 ====================*/
static RingbufHandle_t s_aec_ref_ringbuf = NULL;
static bool s_aec_ref_enabled = false;
static int s_aec_ref_sample_rate = 16000;  /* AEC 工作在 16kHz */
/* 临时缓冲区：用于采样率转换（立体声 -> 单声道 16kHz） */
static int16_t s_aec_ref_tmp[AUDIO_I2S_CHUNK_FRAMES];
static portMUX_TYPE s_aec_ref_lock = portMUX_INITIALIZER_UNLOCKED;
/* 重采样累加器（Q16 定点数，16位整数+16位小数），跨调用保持相位连续 */
static uint32_t s_aec_resample_pos_q16 = 0;

/* 16-bit×Q15 缩放（饱和） */
static inline int16_t mul_q15_sat(int16_t x, int q15)
{
    int32_t t = (int32_t)x * (int32_t)q15;
    t >>= 15;
    if (t > 32767)  return 32767;
    if (t < -32768) return -32768;
    return (int16_t)t;
}

/* 生成 I2S 标准配置（按 SoC 能力选择时钟源） */
static inline void make_std_cfg(i2s_std_config_t *std_cfg, int fs_hz)
{
    *std_cfg = (i2s_std_config_t){
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(fs_hz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,        // MAX98357A 无需 MCLK
            .bclk = BOARD_I2S_BCLK,
            .ws   = BOARD_I2S_LRCLK,
            .dout = BOARD_I2S_DOUT,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    std_cfg->slot_cfg.slot_bit_width = AUDIO_I2S_SLOT_BIT_WIDTH;

#if AUDIO_I2S_USE_APLL && defined(SOC_I2S_SUPPORTS_APLL) && SOC_I2S_SUPPORTS_APLL
    std_cfg->clk_cfg.clk_src       = I2S_CLK_SRC_APLL;
    std_cfg->clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256; // 256fs
#endif
}

/* ================ 对外 API ===================== */
bool audio_i2s_init(int sample_rate_hz)
{
    if (s_tx) {
        /* 幂等重建 */
        audio_i2s_deinit();
    }

    /* 可选：上电功放/解除关断（MAX98357A 等） */
    /* 若未连接，请在 board_config.h 中将对应宏设为 -1 */
#ifdef BOARD_AMP_SD_MODE
#if (BOARD_AMP_SD_MODE >= 0)
    {
        gpio_config_t io = {0};
        io.pin_bit_mask = (1ULL << (BOARD_AMP_SD_MODE));
        io.mode = GPIO_MODE_OUTPUT;
        io.intr_type = GPIO_INTR_DISABLE;
        (void)gpio_config(&io);
        gpio_set_level(BOARD_AMP_SD_MODE, 1); /* 退出关断 */
    }
#endif
#endif
#ifdef BOARD_AMP_ENABLE
#if (BOARD_AMP_ENABLE >= 0)
    {
        gpio_config_t io = {0};
        io.pin_bit_mask = (1ULL << (BOARD_AMP_ENABLE));
        io.mode = GPIO_MODE_OUTPUT;
        io.intr_type = GPIO_INTR_DISABLE;
        (void)gpio_config(&io);
        gpio_set_level(BOARD_AMP_ENABLE, 1); /* 使能功放 */
    }
#endif
#endif
    /* 给功放一个稳定时间，避免上电瞬态噪声/首包丢失 */
    vTaskDelay(pdMS_TO_TICKS(5));

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BOARD_I2S_OUT_NUM, I2S_ROLE_MASTER);

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
    chan_cfg.dma_desc_num  = AUDIO_I2S_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = AUDIO_I2S_DMA_FRAME_NUM;
#endif
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0))
    chan_cfg.auto_clear = true;
#endif

    if (i2s_new_channel(&chan_cfg, &s_tx, NULL) != ESP_OK) {
        s_tx = NULL;
        return false;
    }

    i2s_std_config_t std_cfg;
    make_std_cfg(&std_cfg, sample_rate_hz);

    if (i2s_channel_init_std_mode(s_tx, &std_cfg) != ESP_OK) {
        i2s_del_channel(s_tx);
        s_tx = NULL;
        return false;
    }
    if (i2s_channel_enable(s_tx) != ESP_OK) {
        i2s_del_channel(s_tx);
        s_tx = NULL;
        return false;
    }

    s_sample_rate = sample_rate_hz;
    s_enabled     = true;
    s_muted       = false;

    DRV_LOGI("I2S ready: fs=%d, pins(BCLK=%d,LRCLK=%d,DOUT=%d), DMA(desc=%d,frame=%d)",
             sample_rate_hz, BOARD_I2S_BCLK, BOARD_I2S_LRCLK, BOARD_I2S_DOUT,
             AUDIO_I2S_DMA_DESC_NUM, AUDIO_I2S_DMA_FRAME_NUM);
    return true;
}

void audio_i2s_deinit(void)
{
    if (!s_tx) return;
    if (s_enabled) {
        /* Close the write gate first to minimize races with audio_i2s_write(). */
        s_enabled = false;
        (void)i2s_channel_disable(s_tx);
    }
    (void)i2s_del_channel(s_tx);
    s_tx          = NULL;
    s_sample_rate = 0;
}

size_t audio_i2s_write(const int16_t *st_lr, size_t n_frames)
{
    if (!s_tx || !n_frames) return 0;

    size_t frames_left = n_frames;
    size_t total_bytes = 0;

    while (frames_left) {
        if (!s_enabled) {
            break;
        }
        size_t blk = (frames_left > AUDIO_I2S_CHUNK_FRAMES) ? AUDIO_I2S_CHUNK_FRAMES : frames_left;

        /* 源选择：
           - 静音：零缓冲
           - 音量 100% 且源 DMA 能力 OK：零拷贝
           - 其余：缩放/拷贝到内部 DMA 缓冲 */
        const void *src_ptr = NULL;
        const int16_t *audio_data = st_lr;  /* 保存原始指针用于 AEC */
        bool src_dma_ok = esp_ptr_dma_capable(st_lr);
        if (s_muted) {
            src_ptr = s_zero_stereo;
            audio_data = s_zero_stereo;
        } else if (s_vol_q15 == 32767 && src_dma_ok) {
            src_ptr = st_lr;
        } else {
            const int16_t *in = st_lr;
            int16_t *out = s_tmp_scaled;
            if (s_vol_q15 == 32767) {
                memcpy(out, in, blk * 2 * sizeof(int16_t));
            } else {
                for (size_t i = 0; i < blk * 2; ++i) out[i] = mul_q15_sat(in[i], s_vol_q15);
            }
            src_ptr = out;
            audio_data = out;
        }

        size_t bytes = 0;
        const size_t want = blk * 2 * sizeof(int16_t);
        size_t done = 0;
        const uint8_t *p = (const uint8_t *)src_ptr;
        /* Bounded write loop:
         * - Prevents indefinite blocking (previously portMAX_DELAY).
         * - Allows controller to interrupt by disabling the channel (audio_i2s_pause -> s_enabled=false). */
        int timeout_cnt = 0;
        const int k_max_timeouts = 20; /* ~600ms worst-case for one chunk */
        while (s_enabled && done < want) {
            size_t w = 0;
            esp_err_t ret = i2s_channel_write(s_tx, p + done, want - done, &w, 30 /* ms */);
            done += w;
            if (ret == ESP_OK) {
                timeout_cnt = 0;
                if (w == 0) {
                    /* Defensive: avoid a tight loop if driver reports OK but writes nothing. */
                    break;
                }
                continue;
            }
            if (ret == ESP_ERR_TIMEOUT) {
                if (++timeout_cnt >= k_max_timeouts) {
                    break;
                }
                continue;
            }
            /* Invalid state/arg or other driver errors: stop writing this chunk. */
            break;
        }
        bytes = done;
        size_t written_frames = bytes / (2 * sizeof(int16_t));

        /* AEC 参考信号捕获：将播放数据保存到环形缓冲区 */
        if (s_aec_ref_enabled && s_aec_ref_ringbuf && bytes > 0) {
            /* written_frames already computed above */

            /* 采样率转换：使用 Q16 定点分数累加器 + 线性插值
             * 支持任意采样率组合（44.1k、48k、32k...→16k）
             * Q16 = 16位整数 + 16位小数，避免浮点和整数截断误差 */
            size_t out_idx = 0;
            if (s_sample_rate > 0 && s_sample_rate != s_aec_ref_sample_rate) {
                /* 非同采样率：分数重采样 + 线性插值 */
                uint32_t step_q16 = ((uint32_t)s_sample_rate << 16) / (uint32_t)s_aec_ref_sample_rate;
                uint32_t end_pos_q16 = (uint32_t)written_frames << 16;

                while (s_aec_resample_pos_q16 < end_pos_q16 && out_idx < AUDIO_I2S_CHUNK_FRAMES) {
                    uint32_t idx0 = s_aec_resample_pos_q16 >> 16;       /* 整数部分：当前样本索引 */
                    uint32_t frac = s_aec_resample_pos_q16 & 0xFFFF;    /* 小数部分：插值权重 */
                    uint32_t idx1 = idx0 + 1;

                    /* 边界保护：最后一个样本用零阶保持 */
                    if (idx1 >= written_frames) idx1 = idx0;

                    /* 立体声转单声道（两个相邻采样点） */
                    int32_t mono0 = ((int32_t)audio_data[idx0 * 2] + (int32_t)audio_data[idx0 * 2 + 1]) >> 1;
                    int32_t mono1 = ((int32_t)audio_data[idx1 * 2] + (int32_t)audio_data[idx1 * 2 + 1]) >> 1;

                    /* 线性插值：out = mono0 + (mono1 - mono0) * frac / 65536
                     * 注意：中间乘法可能超过 int32，需用 int64 避免溢出。 */
                    int32_t interp = mono0 + (int32_t)(((int64_t)(mono1 - mono0) * (int64_t)frac) >> 16);
                    s_aec_ref_tmp[out_idx++] = (int16_t)interp;

                    s_aec_resample_pos_q16 += step_q16;
                }

                /* 消费完当前块后，保留小数部分（相位连续） */
                if (s_aec_resample_pos_q16 >= end_pos_q16) {
                    s_aec_resample_pos_q16 -= end_pos_q16;
                }
            } else {
                /* 同采样率（16k→16k）：直接立体声转单声道，无需插值 */
                size_t mono_samples = written_frames;
                if (mono_samples > AUDIO_I2S_CHUNK_FRAMES) mono_samples = AUDIO_I2S_CHUNK_FRAMES;

                for (size_t i = 0; i < mono_samples; i++) {
                    int32_t left = audio_data[i * 2];
                    int32_t right = audio_data[i * 2 + 1];
                    s_aec_ref_tmp[i] = (int16_t)((left + right) >> 1);
                }
                out_idx = mono_samples;
            }

            /* 非阻塞写入环形缓冲区 */
            if (out_idx > 0) {
                portENTER_CRITICAL(&s_aec_ref_lock);
                xRingbufferSend(s_aec_ref_ringbuf, s_aec_ref_tmp,
                                out_idx * sizeof(int16_t), 0);
                portEXIT_CRITICAL(&s_aec_ref_lock);
            }
        }

        total_bytes += bytes;
        frames_left -= written_frames;
        st_lr       += written_frames * 2;
        /* Stop early if paused or the driver couldn't consume the whole chunk. */
        if (!s_enabled || written_frames < blk) {
            break;
        }
    }
    return total_bytes / (2 * sizeof(int16_t));
}

void audio_i2s_set_volume_percent(int vol)
{
    if (vol < 0)   vol = 0;
    if (vol > 100) vol = 100;

    s_vol_percent = vol;
    s_vol_q15 = (vol >= 100) ? 32767 : (vol <= 0 ? 0 : (int)(32767.0f * (vol / 100.0f)));
    if (s_vol_q15 > 32767) s_vol_q15 = 32767;
    if (s_vol_q15 < 0)     s_vol_q15 = 0;
}

int audio_i2s_get_volume_percent(void) { return s_vol_percent; }

void audio_i2s_set_mute(bool mute) { s_muted = mute; }
bool audio_i2s_get_mute(void) { return s_muted; }

int audio_i2s_get_sample_rate(void) { return s_sample_rate; }

bool audio_i2s_set_sample_rate(int fs_hz)
{
    if (!s_tx) return false;

    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(fs_hz);
#if AUDIO_I2S_USE_APLL && defined(SOC_I2S_SUPPORTS_APLL) && SOC_I2S_SUPPORTS_APLL
    clk.clk_src       = I2S_CLK_SRC_APLL;
    clk.mclk_multiple = I2S_MCLK_MULTIPLE_256;
#endif

    /* 为避免驱动层打印“invalid state, should be disabled before reconfiguring”，
       我们统一采用：若已启用则先 disable，再 reconfig，然后按需 enable。 */
    bool was_enabled = s_enabled;
    if (was_enabled) {
        /* Close the write gate first to minimize races with audio_i2s_write(). */
        s_enabled = false;
        (void)i2s_channel_disable(s_tx);
    }
    esp_err_t err = i2s_channel_reconfig_std_clock(s_tx, &clk);
    if (err == ESP_OK && was_enabled) {
        if (i2s_channel_enable(s_tx) == ESP_OK) s_enabled = true;
    }
    if (err == ESP_OK) {
        s_sample_rate = fs_hz;
        s_aec_resample_pos_q16 = 0;  /* 采样率切换时重置重采样累加器 */
        DRV_LOGI("fs -> %d", fs_hz);
    }
    return err == ESP_OK;
}

bool audio_i2s_pause(void)
{
    if (!s_tx) return false;
    if (!s_enabled) {
        /* Already gated off at our layer; still try to disable the channel to keep driver state consistent. */
        esp_err_t err = i2s_channel_disable(s_tx);
        return (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
    }
    /* Close the write gate first to minimize races with audio_i2s_write(). */
    s_enabled = false;
    esp_err_t err = i2s_channel_disable(s_tx);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        /* Only rollback for real disable failures. INVALID_STATE means it was already disabled. */
        s_enabled = true;
        return false;
    }
    return true;
}
bool audio_i2s_resume(void)
{
    if (!s_tx) return false;
    if (s_enabled) return true; /* already enabled */
    esp_err_t err = i2s_channel_enable(s_tx);
    /* INVALID_STATE usually means the channel is already enabled; treat as success. */
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return false;
    s_enabled = true;
    return true;
}
bool audio_i2s_is_ready(void) { return s_tx && s_enabled; }

/*==================== AEC 参考信号 API ====================*/

bool audio_i2s_aec_ref_enable(bool enable, int buffer_ms)
{
    if (enable) {
        if (s_aec_ref_ringbuf) {
            /* 已启用，直接返回成功 */
            s_aec_ref_enabled = true;
            return true;
        }

        /* 计算缓冲区大小：16kHz 单声道，每样本 2 字节 */
        size_t buf_samples = (s_aec_ref_sample_rate * buffer_ms) / 1000;
        size_t buf_bytes = buf_samples * sizeof(int16_t);
        if (buf_bytes < 1024) buf_bytes = 1024;

        /* 使用 PSRAM 分配环形缓冲区，减少内部 RAM 压力 */
        s_aec_ref_ringbuf = xRingbufferCreateWithCaps(buf_bytes, RINGBUF_TYPE_BYTEBUF,
                                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_aec_ref_ringbuf) {
            /* 回退到内部 RAM */
            DRV_LOGW("PSRAM ringbuf failed, fallback to internal RAM");
            s_aec_ref_ringbuf = xRingbufferCreate(buf_bytes, RINGBUF_TYPE_BYTEBUF);
        }
        if (!s_aec_ref_ringbuf) {
            DRV_LOGE("Failed to create AEC ref ringbuf (%u bytes)", (unsigned)buf_bytes);
            return false;
        }

        s_aec_ref_enabled = true;
        s_aec_resample_pos_q16 = 0;  /* 新启用时重置重采样累加器 */
        DRV_LOGI("AEC ref enabled: %d ms, %u bytes", buffer_ms, (unsigned)buf_bytes);
        return true;
    } else {
        /* 禁用 */
        s_aec_ref_enabled = false;
        s_aec_resample_pos_q16 = 0;  /* 禁用时重置 */
        if (s_aec_ref_ringbuf) {
            vRingbufferDelete(s_aec_ref_ringbuf);
            s_aec_ref_ringbuf = NULL;
        }
        return true;
    }
}

size_t audio_i2s_get_ref_signal(int16_t *out_buf, size_t samples)
{
    if (!s_aec_ref_ringbuf || !out_buf || samples == 0) {
        return 0;
    }

    size_t requested_bytes = samples * sizeof(int16_t);
    size_t total_read = 0;

    /* 尝试从环形缓冲区读取数据 */
    while (total_read < requested_bytes) {
        size_t item_size = 0;
        void *item = xRingbufferReceiveUpTo(s_aec_ref_ringbuf,
                                            &item_size,
                                            0,  /* 非阻塞 */
                                            requested_bytes - total_read);
        if (!item) {
            break;
        }

        memcpy((uint8_t *)out_buf + total_read, item, item_size);
        vRingbufferReturnItem(s_aec_ref_ringbuf, item);
        total_read += item_size;
    }

    /* 如果读取不足，用零填充剩余部分 */
    if (total_read < requested_bytes) {
        memset((uint8_t *)out_buf + total_read, 0, requested_bytes - total_read);
    }

    return total_read / sizeof(int16_t);
}

void audio_i2s_aec_ref_clear(void)
{
    if (!s_aec_ref_ringbuf) return;

    /* 清空环形缓冲区 */
    portENTER_CRITICAL(&s_aec_ref_lock);
    size_t item_size;
    void *item;
    while ((item = xRingbufferReceive(s_aec_ref_ringbuf, &item_size, 0)) != NULL) {
        vRingbufferReturnItem(s_aec_ref_ringbuf, item);
    }
    portEXIT_CRITICAL(&s_aec_ref_lock);
}

size_t audio_i2s_aec_ref_available(void)
{
    if (!s_aec_ref_ringbuf) return 0;

    UBaseType_t free_size = xRingbufferGetCurFreeSize(s_aec_ref_ringbuf);
    size_t total_size = 0;

    /* 获取环形缓冲区总大小（通过 API 无法直接获取，需要计算） */
    /* 这里简化处理：返回已用字节数估算的样本数 */
    /* 注意：FreeRTOS ringbuf 没有直接获取已用大小的 API */
    /* 我们用 (total - free) 估算，但需要知道 total */
    /* 暂时返回一个保守估计 */
    (void)free_size;
    (void)total_size;

    /* 使用 peek 方式检查可用数据量 */
    size_t item_size = 0;
    void *item = xRingbufferReceiveUpTo(s_aec_ref_ringbuf, &item_size, 0, 0xFFFFFFFF);
    if (item) {
        /* 放回数据 - 注意：这不是真正的 peek，数据已被取出 */
        /* FreeRTOS ringbuf 不支持 peek，需要重新写入 */
        xRingbufferSend(s_aec_ref_ringbuf, item, item_size, 0);
        vRingbufferReturnItem(s_aec_ref_ringbuf, item);
        return item_size / sizeof(int16_t);
    }

    return 0;
}
