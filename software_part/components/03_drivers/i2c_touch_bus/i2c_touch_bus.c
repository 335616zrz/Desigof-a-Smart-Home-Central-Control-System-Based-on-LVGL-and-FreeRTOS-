#include "i2c_touch_bus.h"

#include "board_config.h"
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t s_touch_bus = NULL;

esp_err_t i2c_touch_bus_get(i2c_master_bus_handle_t *out_bus)
{
    if (!out_bus) return ESP_ERR_INVALID_ARG;

    if (s_touch_bus) {
        *out_bus = s_touch_bus;
        return ESP_OK;
    }

    i2c_master_bus_config_t buscfg = {
        .i2c_port = BOARD_TOUCH_I2C_PORT,
        .sda_io_num = BOARD_TOUCH_SDA,
        .scl_io_num = BOARD_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t er = i2c_new_master_bus(&buscfg, &s_touch_bus);
    if (er != ESP_OK) {
        s_touch_bus = NULL;
        return er;
    }

    *out_bus = s_touch_bus;
    return ESP_OK;
}

