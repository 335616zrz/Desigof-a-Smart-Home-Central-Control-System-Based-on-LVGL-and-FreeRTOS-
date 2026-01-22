#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Measurement precision level. */
typedef enum {
    SHT40_PRECISION_HIGH = 0,
    SHT40_PRECISION_MEDIUM,
    SHT40_PRECISION_LOW,
} sht40_precision_t;

/** Optional on-chip heater configuration. */
typedef enum {
    SHT40_HEATER_OFF = 0,
    SHT40_HEATER_HIGH_1S,
    SHT40_HEATER_HIGH_100MS,
    SHT40_HEATER_MED_1S,
    SHT40_HEATER_MED_100MS,
    SHT40_HEATER_LOW_1S,
    SHT40_HEATER_LOW_100MS,
} sht40_heater_t;

typedef struct {
    int      i2c_port;     /**< I2C_NUM_0 / I2C_NUM_1 */
    int      sda_io;       /**< SDA GPIO number       */
    int      scl_io;       /**< SCL GPIO number       */
    uint32_t i2c_hz;       /**< Bus clock, e.g. 100k/400k/1M; 0 = default */
    uint8_t  i2c_addr;     /**< 7-bit address, default 0x44 (SHT40-AD1B)   */

    sht40_precision_t precision; /**< Measurement precision (repeatability) */
    sht40_heater_t    heater;    /**< On-chip heater mode                   */
} sht40_config_t;

/**
 * @brief Initialize SHT40 on a dedicated I2C master bus.
 *
 * This function creates a new I2C master bus on the specified port and pins,
 * attaches the SHT40 device, and issues a soft reset. Calling it again will
 * re-create the bus/device handles.
 */
esp_err_t sht40_init(const sht40_config_t *cfg);

/** @brief Release I2C resources previously allocated by sht40_init(). */
void      sht40_deinit(void);

/** @brief Change precision level used for subsequent measurements. */
void              sht40_set_precision(sht40_precision_t precision);
/** @brief Get current precision level. */
sht40_precision_t sht40_get_precision(void);

/** @brief Change heater mode used for subsequent measurements. */
void           sht40_set_heater(sht40_heater_t heater);
/** @brief Get current heater mode. */
sht40_heater_t sht40_get_heater(void);

/**
 * @brief Perform a single blocking measurement.
 *
 * The function selects the appropriate command based on precision and heater
 * settings, waits until conversion is finished, checks the CRC for both
 * temperature and humidity words, and converts them to °C and %RH.
 *
 * @param[out] temperature_c  Temperature in degree Celsius (nullable).
 * @param[out] humidity_rh    Relative humidity in %RH (0..100, nullable).
 *
 * @retval ESP_OK           Measurement completed and CRC passed.
 * @retval ESP_ERR_INVALID_STATE Driver not initialized.
 * @retval ESP_FAIL         I2C transfer or CRC error.
 * @retval other            I2C driver error codes.
 */
esp_err_t sht40_read(float *temperature_c, float *humidity_rh);

/**
 * @brief Read unique 32-bit serial number from the sensor.
 *
 * @param[out] serial  Pointer to store 32-bit serial number.
 *
 * @retval ESP_OK on success; ESP_ERR_INVALID_STATE or I2C error codes otherwise.
 */
esp_err_t sht40_read_serial(uint32_t *serial);

/**
 * @brief Send a soft reset command.
 *
 * @note Normally called from sht40_init(); exposed for completeness.
 */
esp_err_t sht40_reset(void);

#ifdef __cplusplus
}
#endif

