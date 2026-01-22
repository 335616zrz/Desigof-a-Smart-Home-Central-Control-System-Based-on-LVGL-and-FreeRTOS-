#include "ws2812b_driver.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "board_config.h"

#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "hal/gpio_types.h"

static const char *TAG = "WS2812B";

/* ------------------------------ RMT encoder (WS2812) ------------------------------ */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} ws2812b_rmt_encoder_t;

static size_t ws2812b_rmt_encode(rmt_encoder_t *encoder,
                                rmt_channel_handle_t channel,
                                const void *primary_data,
                                size_t data_size,
                                rmt_encode_state_t *ret_state)
{
    ws2812b_rmt_encoder_t *led_encoder = __containerof(encoder, ws2812b_rmt_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
    case 0: /* send pixel bytes */
        encoded_symbols += led_encoder->bytes_encoder->encode(led_encoder->bytes_encoder, channel,
                                                             primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        /* fall-through */
    case 1: /* send reset code */
        encoded_symbols += led_encoder->copy_encoder->encode(led_encoder->copy_encoder, channel,
                                                            &led_encoder->reset_code,
                                                            sizeof(led_encoder->reset_code),
                                                            &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        break;
    default:
        break;
    }

out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t ws2812b_rmt_del(rmt_encoder_t *encoder)
{
    ws2812b_rmt_encoder_t *led_encoder = __containerof(encoder, ws2812b_rmt_encoder_t, base);
    if (led_encoder->bytes_encoder) {
        rmt_del_encoder(led_encoder->bytes_encoder);
        led_encoder->bytes_encoder = NULL;
    }
    if (led_encoder->copy_encoder) {
        rmt_del_encoder(led_encoder->copy_encoder);
        led_encoder->copy_encoder = NULL;
    }
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t ws2812b_rmt_reset(rmt_encoder_t *encoder)
{
    ws2812b_rmt_encoder_t *led_encoder = __containerof(encoder, ws2812b_rmt_encoder_t, base);
    if (led_encoder->bytes_encoder) {
        rmt_encoder_reset(led_encoder->bytes_encoder);
    }
    if (led_encoder->copy_encoder) {
        rmt_encoder_reset(led_encoder->copy_encoder);
    }
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t ws2812b_new_rmt_encoder(uint32_t resolution_hz, rmt_encoder_handle_t *ret_encoder)
{
    if (!ret_encoder || resolution_hz == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ws2812b_rmt_encoder_t *enc = rmt_alloc_encoder_mem(sizeof(ws2812b_rmt_encoder_t));
    if (!enc) {
        return ESP_ERR_NO_MEM;
    }
    memset(enc, 0, sizeof(*enc));

    enc->base.encode = ws2812b_rmt_encode;
    enc->base.del = ws2812b_rmt_del;
    enc->base.reset = ws2812b_rmt_reset;

    /* WS2812 timing (800kHz). Using the official ESP-IDF example values:
     * - bit0: 0.3us high + 0.9us low
     * - bit1: 0.9us high + 0.3us low
     * and MSB first in GRB order. */
    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = {
            .level0 = 1,
            .duration0 = (uint32_t)(0.3f * (float)resolution_hz / 1000000.0f),
            .level1 = 0,
            .duration1 = (uint32_t)(0.9f * (float)resolution_hz / 1000000.0f),
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = (uint32_t)(0.9f * (float)resolution_hz / 1000000.0f),
            .level1 = 0,
            .duration1 = (uint32_t)(0.3f * (float)resolution_hz / 1000000.0f),
        },
        .flags.msb_first = 1,
    };

    esp_err_t ret = rmt_new_bytes_encoder(&bytes_cfg, &enc->bytes_encoder);
    if (ret != ESP_OK) {
        free(enc);
        return ret;
    }

    rmt_copy_encoder_config_t copy_cfg = {};
    ret = rmt_new_copy_encoder(&copy_cfg, &enc->copy_encoder);
    if (ret != ESP_OK) {
        rmt_del_encoder(enc->bytes_encoder);
        free(enc);
        return ret;
    }

    uint32_t reset_ticks = (resolution_hz / 1000000U) * 50U / 2U; /* 50us reset, split into two durations */
    if (reset_ticks == 0) {
        reset_ticks = 1;
    }
    enc->reset_code = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &enc->base;
    return ESP_OK;
}

/* ------------------------------ Driver instance ------------------------------ */

static bool s_inited = false;
static ws2812b_config_t s_cfg = {0};

static rmt_channel_handle_t s_rmt_chan = NULL;
static rmt_encoder_handle_t s_rmt_enc = NULL;

static uint8_t *s_grb = NULL; /* led_num * 3 */

esp_err_t ws2812b_init(const ws2812b_config_t *cfg)
{
    if (s_inited) {
        return ESP_OK;
    }

    ws2812b_config_t c = {
        .gpio_num = BOARD_WS2812_GPIO,
        .led_num = BOARD_WS2812_LED_NUM,
        .resolution_hz = 10 * 1000 * 1000, /* 10MHz */
    };
    if (cfg) {
        c = *cfg;
    }
    if (c.led_num <= 0 || c.gpio_num < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (c.resolution_hz == 0) {
        c.resolution_hz = 10 * 1000 * 1000;
    }

    s_grb = (uint8_t *)calloc((size_t)c.led_num * 3U, 1);
    if (!s_grb) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ws2812b_new_rmt_encoder(c.resolution_hz, &s_rmt_enc);
    if (ret != ESP_OK) {
        free(s_grb);
        s_grb = NULL;
        return ret;
    }

    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = (gpio_num_t)c.gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = c.resolution_hz,
        .mem_block_symbols = 64,       /* enough for 1 LED; bump if you chain more */
        .trans_queue_depth = 1,
        .intr_priority = 0,
        .flags = {
            .invert_out = 0,
            .with_dma = 0,
            .io_loop_back = 0,
            .io_od_mode = 0,
            .init_level = 0,
        },
    };

    ret = rmt_new_tx_channel(&tx_cfg, &s_rmt_chan);
    if (ret != ESP_OK) {
        rmt_del_encoder(s_rmt_enc);
        s_rmt_enc = NULL;
        free(s_grb);
        s_grb = NULL;
        return ret;
    }

    ret = rmt_enable(s_rmt_chan);
    if (ret != ESP_OK) {
        rmt_del_channel(s_rmt_chan);
        s_rmt_chan = NULL;
        rmt_del_encoder(s_rmt_enc);
        s_rmt_enc = NULL;
        free(s_grb);
        s_grb = NULL;
        return ret;
    }

    s_cfg = c;
    s_inited = true;

    ESP_LOGI(TAG, "WS2812B init ok: gpio=%d leds=%d res=%" PRIu32 "Hz",
             s_cfg.gpio_num, s_cfg.led_num, s_cfg.resolution_hz);

    return ws2812b_clear();
}

esp_err_t ws2812b_deinit(void)
{
    if (!s_inited) {
        return ESP_OK;
    }

    (void)ws2812b_clear();

    if (s_rmt_chan) {
        (void)rmt_disable(s_rmt_chan);
        (void)rmt_del_channel(s_rmt_chan);
        s_rmt_chan = NULL;
    }
    if (s_rmt_enc) {
        (void)rmt_del_encoder(s_rmt_enc);
        s_rmt_enc = NULL;
    }

    free(s_grb);
    s_grb = NULL;

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_inited = false;
    return ESP_OK;
}

static esp_err_t ws2812b_refresh(void)
{
    if (!s_inited || !s_rmt_chan || !s_rmt_enc || !s_grb) {
        return ESP_ERR_INVALID_STATE;
    }

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 0,
        },
    };

    esp_err_t ret = rmt_transmit(s_rmt_chan, s_rmt_enc, s_grb, (size_t)s_cfg.led_num * 3U, &tx_cfg);
    if (ret != ESP_OK) {
        return ret;
    }
    return rmt_tx_wait_all_done(s_rmt_chan, 100 /*ms*/);
}

esp_err_t ws2812b_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    /* WS2812 uses GRB order */
    s_grb[0] = g;
    s_grb[1] = r;
    s_grb[2] = b;
    return ws2812b_refresh();
}

esp_err_t ws2812b_clear(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    memset(s_grb, 0, (size_t)s_cfg.led_num * 3U);
    return ws2812b_refresh();
}
