/**
 * INMP441 数字麦克风 I2S 驱动（ESP32-S3 / ESP-IDF v5+）
 *
 * 设计目标：
 * - 封装 i2s_std 接口，提供“初始化 + 阻塞读取”最小可用能力；
 * - 充分利用 board_config.h 中的 BOARD_I2S_MIC_* 管脚定义；
 * - 默认支持 APLL 时钟源（若 SoC 支持），以获得更稳定采样率。
 *
 * 关键信息（参考官方 Datasheet）：
 * - 接口：I2S，Philips 格式，24-bit 2's complement，MSB first；
 * - LR 脚：0 = 左通道，1 = 右通道；
 * - 标准 I2S 帧：64 × BCLK 周期（左右各 32bit 槽位），本驱动只采单声道。
 */

#include "inmp441_driver.h"

#include <string.h>

#include "board_config.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

/* ================= 可配选项（仅本组件） ================= */
#ifndef INMP441_USE_APLL_DEFAULT
#define INMP441_USE_APLL_DEFAULT   1   /* 若 SoC 支持，则默认使用 APLL */
#endif

#ifndef INMP441_DMA_DESC_NUM_DEFAULT
#define INMP441_DMA_DESC_NUM_DEFAULT  BOARD_I2S_MIC_DMA_DESC_NUM
#endif
#ifndef INMP441_DMA_FRAME_NUM_DEFAULT
#define INMP441_DMA_FRAME_NUM_DEFAULT BOARD_I2S_MIC_DMA_FRAME_NUM
#endif

#ifndef INMP441_CHUNK_SAMPLES
#define INMP441_CHUNK_SAMPLES   256   /* 单次 read 内部最大样本块 */
#endif

/* ================= 可选日志（默认关闭） ================= */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
static const char *DRV_LOG_TAG = "DRV_INMP441";
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif
/* ====================================================== */

static i2s_chan_handle_t  s_rx          = NULL;
static int                s_sample_rate = 0;
static inmp441_channel_t  s_channel     = INMP441_CHANNEL_LEFT;
static bool               s_enabled     = false;

/* 临时缓冲：内部 32-bit 样本读取缓冲（单声道） */
static int32_t s_tmp32[INMP441_CHUNK_SAMPLES];

/* 将逻辑声道转换为 I2S 槽掩码 */
static inline i2s_std_slot_mask_t channel_to_slot_mask(inmp441_channel_t ch)
{
    return (ch == INMP441_CHANNEL_RIGHT) ? I2S_STD_SLOT_RIGHT : I2S_STD_SLOT_LEFT;
}

/* 统一配置 LR_SEL 与 CHIPEN 引脚（如有） */
static void inmp441_apply_gpio_side_effects(inmp441_channel_t ch)
{
    /* 1) 可选：CHIP EN（上电使能） */
#if (BOARD_I2S_MIC_CHIPEN >= 0)
    {
        gpio_config_t io = {0};
        io.pin_bit_mask = (1ULL << BOARD_I2S_MIC_CHIPEN);
        io.mode         = GPIO_MODE_OUTPUT;
        io.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io);
        gpio_set_level(BOARD_I2S_MIC_CHIPEN, 1);   /* 上电使能麦克风 */
    }
#endif

    /* 2) 可选：LR_SEL（左右声道选择，0=左，1=右） */
#if (BOARD_I2S_MIC_LR_SEL >= 0)
    {
        gpio_config_t io = {0};
        io.pin_bit_mask = (1ULL << BOARD_I2S_MIC_LR_SEL);
        io.mode         = GPIO_MODE_OUTPUT;
        io.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io);
        int level = (ch == INMP441_CHANNEL_RIGHT) ? 1 : 0;
        gpio_set_level(BOARD_I2S_MIC_LR_SEL, level);
    }
#else
    (void)ch;
#endif
}

/* 生成 I2S 标准配置 */
static void make_std_cfg(i2s_std_config_t *std_cfg, int fs_hz, bool use_apll, inmp441_channel_t ch)
{
    if (!std_cfg) return;

    *std_cfg = (i2s_std_config_t){
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(fs_hz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = BOARD_I2S_MIC_BCLK,
            .ws   = BOARD_I2S_MIC_LRCLK,
            .dout = I2S_GPIO_UNUSED,
            .din  = BOARD_I2S_MIC_DATA,
        },
    };

    /* 槽位配置：32-bit 槽宽，单声道 & 槽掩码 */
    std_cfg->slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    std_cfg->slot_cfg.slot_mask      = channel_to_slot_mask(ch);

#if defined(SOC_I2S_SUPPORTS_APLL) && SOC_I2S_SUPPORTS_APLL
    if (use_apll) {
        std_cfg->clk_cfg.clk_src       = I2S_CLK_SRC_APLL;
        std_cfg->clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    }
#else
    (void)use_apll;
#endif
}

/* ================= 对外 API ================= */

esp_err_t inmp441_init(const inmp441_config_t *cfg)
{
    ESP_RETURN_ON_FALSE(cfg, ESP_ERR_INVALID_ARG, DRV_LOG_TAG, "cfg is NULL");
    ESP_RETURN_ON_FALSE(cfg->sample_rate_hz > 0, ESP_ERR_INVALID_ARG,
                        DRV_LOG_TAG, "sample_rate_hz must be >0");

    /* 幂等重建：若已存在旧通道则先释放 */
    if (s_rx) {
        inmp441_deinit();
    }

    bool use_apll = cfg->use_apll;
    if (!cfg->use_apll) {
        use_apll = INMP441_USE_APLL_DEFAULT;
    }
    s_channel = cfg->channel;

    /* 配置 LR_SEL / CHIPEN 等 GPIO，确保上电后处于已知状态 */
    inmp441_apply_gpio_side_effects(s_channel);
    /* 给麦克风一点时间上电稳定（Datasheet 上电时序要求通常在百微秒级，这里取 1ms 保险） */
    vTaskDelay(pdMS_TO_TICKS(1));

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BOARD_I2S_MIC_NUM, I2S_ROLE_MASTER);

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
    chan_cfg.dma_desc_num  = (cfg->dma_desc_num  > 0) ? cfg->dma_desc_num  : INMP441_DMA_DESC_NUM_DEFAULT;
    chan_cfg.dma_frame_num = (cfg->dma_frame_num > 0) ? cfg->dma_frame_num : INMP441_DMA_FRAME_NUM_DEFAULT;
#endif

    esp_err_t er = i2s_new_channel(&chan_cfg, NULL, &s_rx);
    ESP_RETURN_ON_ERROR(er, DRV_LOG_TAG, "i2s_new_channel failed: %d", (int)er);

    i2s_std_config_t std_cfg;
    make_std_cfg(&std_cfg, cfg->sample_rate_hz, use_apll, s_channel);

    er = i2s_channel_init_std_mode(s_rx, &std_cfg);
    if (er != ESP_OK) {
        DRV_LOGE("i2s_channel_init_std_mode failed: %d", (int)er);
        (void)i2s_del_channel(s_rx);
        s_rx = NULL;
        return er;
    }

    er = i2s_channel_enable(s_rx);
    if (er != ESP_OK) {
        DRV_LOGE("i2s_channel_enable failed: %d", (int)er);
        (void)i2s_del_channel(s_rx);
        s_rx = NULL;
        return er;
    }

    s_sample_rate = cfg->sample_rate_hz;
    s_enabled     = true;

    DRV_LOGI("INMP441 ready: fs=%d, pins(BCLK=%d,LRCLK=%d,DIN=%d)",
             s_sample_rate, BOARD_I2S_MIC_BCLK, BOARD_I2S_MIC_LRCLK, BOARD_I2S_MIC_DATA);
    return ESP_OK;
}

void inmp441_deinit(void)
{
    if (!s_rx) return;

    if (s_enabled) {
        (void)i2s_channel_disable(s_rx);
        s_enabled = false;
    }
    (void)i2s_del_channel(s_rx);
    s_rx          = NULL;
    s_sample_rate = 0;
}

esp_err_t inmp441_set_sample_rate(int fs_hz)
{
    if (!s_rx || fs_hz <= 0) return ESP_ERR_INVALID_STATE;

    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(fs_hz);

#if defined(SOC_I2S_SUPPORTS_APLL) && SOC_I2S_SUPPORTS_APLL
    clk.clk_src       = I2S_CLK_SRC_APLL;
    clk.mclk_multiple = I2S_MCLK_MULTIPLE_256;
#endif

    bool was_enabled = s_enabled;
    if (was_enabled) {
        (void)i2s_channel_disable(s_rx);
        s_enabled = false;
    }

    esp_err_t er = i2s_channel_reconfig_std_clock(s_rx, &clk);
    if (er == ESP_OK && was_enabled) {
        if (i2s_channel_enable(s_rx) == ESP_OK) {
            s_enabled = true;
        }
    }

    if (er == ESP_OK) {
        s_sample_rate = fs_hz;
        DRV_LOGI("fs -> %d", fs_hz);
    }
    return er;
}

int inmp441_get_sample_rate(void)
{
    return s_sample_rate;
}

esp_err_t inmp441_set_channel(inmp441_channel_t ch)
{
    if (!s_rx) {
        s_channel = ch;
        inmp441_apply_gpio_side_effects(ch);
        return ESP_ERR_INVALID_STATE;
    }

    /* 更新 GPIO 电平（如有） */
    inmp441_apply_gpio_side_effects(ch);

    /* 更新 I2S 槽掩码 */
    i2s_std_slot_config_t slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO);
    slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    slot_cfg.slot_mask      = channel_to_slot_mask(ch);

    bool was_enabled = s_enabled;
    if (was_enabled) {
        (void)i2s_channel_disable(s_rx);
        s_enabled = false;
    }

    esp_err_t er = i2s_channel_reconfig_std_slot(s_rx, &slot_cfg);
    if (er == ESP_OK && was_enabled) {
        if (i2s_channel_enable(s_rx) == ESP_OK) {
            s_enabled = true;
        }
    }

    if (er == ESP_OK) {
        s_channel = ch;
        DRV_LOGI("channel -> %s", (ch == INMP441_CHANNEL_RIGHT) ? "RIGHT" : "LEFT");
    }
    return er;
}

inmp441_channel_t inmp441_get_channel(void)
{
    return s_channel;
}

bool inmp441_is_ready(void)
{
    return s_rx && s_enabled;
}

size_t inmp441_read_raw(int32_t *buf, size_t n_samples, TickType_t timeout)
{
    if (!s_rx || !buf || n_samples == 0) return 0;

    size_t samples_left = n_samples;
    size_t total_read   = 0;

    while (samples_left > 0) {
        size_t blk = (samples_left > INMP441_CHUNK_SAMPLES) ? INMP441_CHUNK_SAMPLES : samples_left;
        size_t bytes_expected = blk * sizeof(int32_t);
        size_t bytes_read = 0;

        esp_err_t er = i2s_channel_read(s_rx, (void *)(buf + total_read),
                                        bytes_expected, &bytes_read, timeout);
        if (er != ESP_OK || bytes_read == 0) {
            DRV_LOGW("i2s_channel_read failed/timeout: er=%d bytes=%u",
                     (int)er, (unsigned)bytes_read);
            break;
        }

        size_t got = bytes_read / sizeof(int32_t);
        total_read   += got;
        samples_left -= got;

        if (got < blk) {
            /* 一次 read 未满足预期数量，视为到此为止 */
            break;
        }
    }
    return total_read;
}

size_t inmp441_read_pcm16(int16_t *buf, size_t n_samples, TickType_t timeout)
{
    if (!s_rx || !buf || n_samples == 0) return 0;

    size_t samples_left = n_samples;
    size_t total_read   = 0;

    while (samples_left > 0) {
        size_t blk = (samples_left > INMP441_CHUNK_SAMPLES) ? INMP441_CHUNK_SAMPLES : samples_left;

        size_t bytes_expected = blk * sizeof(int32_t);
        size_t bytes_read = 0;

        esp_err_t er = i2s_channel_read(s_rx, (void *)s_tmp32,
                                        bytes_expected, &bytes_read, timeout);
        if (er != ESP_OK || bytes_read == 0) {
            DRV_LOGW("i2s_channel_read failed/timeout: er=%d bytes=%u",
                     (int)er, (unsigned)bytes_read);
            break;
        }

        size_t got = bytes_read / sizeof(int32_t);
        if (got == 0) break;

        /* 将 32-bit I2S 样本转换为 16-bit PCM：
         * - INMP441 在 32-bit 槽内输出 24-bit 有效数据，左对齐（[31:8]）。
         * - 直接左移再右移会在高幅度时溢出符号位，导致音量异常偏小。
         * - 这里使用两次算术右移：先丢弃无效的低 8 位，再将 24-bit 缩放到 16-bit，避免溢出并保持幅度一致。
         */
        for (size_t i = 0; i < got; ++i) {
            int32_t v = s_tmp32[i];
            v >>= 8;       /* 去掉填充的低 8bit，得到有符号 24-bit */
            v >>= 8;       /* 24-bit => 16-bit 缩放（除以 256），保持符号扩展 */
            buf[total_read + i] = (int16_t)v;
        }

        total_read   += got;
        samples_left -= got;

        if (got < blk) {
            break;
        }
    }

    return total_read;
}
