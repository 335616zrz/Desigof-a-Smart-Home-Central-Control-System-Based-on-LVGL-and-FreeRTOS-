/**
 * SHT40 温湿度传感器驱动（ESP-IDF v5+ I2C master 总线/设备模型）
 *
 * - 仅提供阻塞式单次测量接口 sht40_read()
 * - 精度与加热模式接口仿照 Adafruit_SHT4x 库（基于官方 Datasheet）
 * - 默认零日志，如需排障可在 CMake 中添加 -DDRV_LOG_ENABLE=1
 */

#include "sht40_driver.h"
#include "board_config.h"
#include "i2c_touch_bus.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---- 可选日志（默认关闭） ---- */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  static const char *DRV_LOG_TAG = "DRV_SHT40";
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif

/* ---- 常量命令（参考 SHT4x Datasheet & Adafruit 库） ---- */
#ifndef SHT40_I2C_ADDR_DEFAULT
#define SHT40_I2C_ADDR_DEFAULT     0x44
#endif

/* 无加热测量命令（不同精度） */
#define SHT40_CMD_NOHEAT_HIGHPREC  0xFD  /* High precision, no heater        */
#define SHT40_CMD_NOHEAT_MEDPREC   0xF6  /* Medium precision, no heater      */
#define SHT40_CMD_NOHEAT_LOWPREC   0xE0  /* Low precision, no heater         */

/* 带加热测量命令（不同功率/时间组合） */
#define SHT40_CMD_HIGHHEAT_1S      0x39  /* High heat, 1 s                   */
#define SHT40_CMD_HIGHHEAT_100MS   0x32  /* High heat, 0.1 s                 */
#define SHT40_CMD_MEDHEAT_1S       0x2F  /* Medium heat, 1 s                 */
#define SHT40_CMD_MEDHEAT_100MS    0x24  /* Medium heat, 0.1 s               */
#define SHT40_CMD_LOWHEAT_1S       0x1E  /* Low heat, 1 s                    */
#define SHT40_CMD_LOWHEAT_100MS    0x15  /* Low heat, 0.1 s                  */

/* 其它命令 */
#define SHT40_CMD_READ_SERIAL      0x89
#define SHT40_CMD_SOFT_RESET       0x94

/* ---- 上下文 ---- */
typedef struct {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;

    int      port;
    int      sda_io;
    int      scl_io;
    uint32_t hz;
    uint8_t  addr;

    sht40_precision_t precision;
    sht40_heater_t    heater;
} sht40_ctx_t;

static sht40_ctx_t s_ctx = {0};

/* ---------------- I2C 基础 ---------------- */
static inline esp_err_t sht40_i2c_write(const uint8_t *buf, size_t len)
{
    if (!s_ctx.dev) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit(s_ctx.dev, buf, len, 1000);
}

static inline esp_err_t sht40_i2c_read(uint8_t *buf, size_t len)
{
    if (!s_ctx.dev) return ESP_ERR_INVALID_STATE;
    return i2c_master_receive(s_ctx.dev, buf, len, 1000);
}

/* CRC-8 校验：多项式 0x31，初值 0xFF（Datasheet 4.3） */
static uint8_t sht40_crc8(const uint8_t *data, int len)
{
    const uint8_t POLY = 0x31;
    uint8_t crc = 0xFF;

    for (int j = 0; j < len; ++j) {
        crc ^= data[j];
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ POLY);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* 选取测量命令与对应等待时间（毫秒） */
static void sht40_select_cmd(uint8_t *cmd, uint16_t *delay_ms)
{
    if (!cmd || !delay_ms) return;

    if (s_ctx.heater == SHT40_HEATER_OFF) {
        switch (s_ctx.precision) {
        case SHT40_PRECISION_LOW:
            *cmd = SHT40_CMD_NOHEAT_LOWPREC;
            *delay_ms = 2;   /* Datasheet：低精度典型 1.6 ms */
            break;
        case SHT40_PRECISION_MEDIUM:
            *cmd = SHT40_CMD_NOHEAT_MEDPREC;
            *delay_ms = 5;   /* Datasheet：中精度典型 4.5 ms */
            break;
        case SHT40_PRECISION_HIGH:
        default:
            *cmd = SHT40_CMD_NOHEAT_HIGHPREC;
            *delay_ms = 10;  /* Datasheet：高精度典型 8.2~9.2 ms */
            break;
        }
    } else {
        switch (s_ctx.heater) {
        case SHT40_HEATER_HIGH_1S:
            *cmd = SHT40_CMD_HIGHHEAT_1S;
            *delay_ms = 1100;
            break;
        case SHT40_HEATER_HIGH_100MS:
            *cmd = SHT40_CMD_HIGHHEAT_100MS;
            *delay_ms = 110;
            break;
        case SHT40_HEATER_MED_1S:
            *cmd = SHT40_CMD_MEDHEAT_1S;
            *delay_ms = 1100;
            break;
        case SHT40_HEATER_MED_100MS:
            *cmd = SHT40_CMD_MEDHEAT_100MS;
            *delay_ms = 110;
            break;
        case SHT40_HEATER_LOW_1S:
            *cmd = SHT40_CMD_LOWHEAT_1S;
            *delay_ms = 1100;
            break;
        case SHT40_HEATER_LOW_100MS:
            *cmd = SHT40_CMD_LOWHEAT_100MS;
            *delay_ms = 110;
            break;
        case SHT40_HEATER_OFF:
        default:
            *cmd = SHT40_CMD_NOHEAT_HIGHPREC;
            *delay_ms = 10;
            break;
        }
    }
}

/* ---------------- 对外接口 ---------------- */
esp_err_t sht40_init(const sht40_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    /* 若重复 init，释放旧设备；I2C 总线由 i2c_touch_bus 统一管理，不在此删除 */
    if (s_ctx.dev) {
        (void)i2c_master_bus_rm_device(s_ctx.dev);
        s_ctx.dev = NULL;
    }
    s_ctx.bus = NULL;

    s_ctx.port      = cfg->i2c_port;
    s_ctx.sda_io    = cfg->sda_io;
    s_ctx.scl_io    = cfg->scl_io;
    /* 默认频率从板级宏继承（400 kHz），必要时可通过 cfg->i2c_hz 覆写 */
    s_ctx.hz        = cfg->i2c_hz ? cfg->i2c_hz : BOARD_SHT4X_I2C_HZ;
    s_ctx.addr      = cfg->i2c_addr ? cfg->i2c_addr : SHT40_I2C_ADDR_DEFAULT;
    s_ctx.precision = cfg->precision;
    s_ctx.heater    = cfg->heater;

    if (s_ctx.hz > 1000000) s_ctx.hz = 1000000;  /* Datasheet：Fast Mode Plus up to 1 MHz */

    /* 1) 获取与 FT6336 共用的 I2C 总线（BOARD_TOUCH_I2C_PORT） */
    esp_err_t er = i2c_touch_bus_get(&s_ctx.bus);
    if (er != ESP_OK) {
        DRV_LOGE("i2c_touch_bus_get failed: %d", (int)er);
        return er;
    }

    /* 2) 在共享总线上挂载 SHT40 设备 */
    i2c_device_config_t devcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = s_ctx.addr,
        .scl_speed_hz    = s_ctx.hz,
    };
    er = i2c_master_bus_add_device(s_ctx.bus, &devcfg, &s_ctx.dev);
    if (er != ESP_OK) {
        DRV_LOGE("i2c_master_bus_add_device failed: %d", (int)er);
        s_ctx.dev = NULL;
        return er;
    }

    /* 软复位，使芯片进入已知状态 */
    er = sht40_reset();
    if (er != ESP_OK) {
        DRV_LOGE("sht40_reset failed: %d", (int)er);
        sht40_deinit();
        return er;
    }

    DRV_LOGI("SHT40 init ok: port=%d addr=0x%02X hz=%lu",
             s_ctx.port, (unsigned)s_ctx.addr, (unsigned long)s_ctx.hz);
    return ESP_OK;
}

void sht40_deinit(void)
{
    if (s_ctx.dev) {
        (void)i2c_master_bus_rm_device(s_ctx.dev);
        s_ctx.dev = NULL;
    }
    /* I2C 总线由 i2c_touch_bus 统一管理，这里不删除 */
    s_ctx.bus = NULL;
}

void sht40_set_precision(sht40_precision_t precision) { s_ctx.precision = precision; }
sht40_precision_t sht40_get_precision(void) { return s_ctx.precision; }

void sht40_set_heater(sht40_heater_t heater) { s_ctx.heater = heater; }
sht40_heater_t sht40_get_heater(void) { return s_ctx.heater; }

esp_err_t sht40_reset(void)
{
    uint8_t cmd = SHT40_CMD_SOFT_RESET;
    esp_err_t er = sht40_i2c_write(&cmd, 1);
    if (er != ESP_OK) return er;
    vTaskDelay(pdMS_TO_TICKS(1));  /* Datasheet：t_reset 最大 1 ms */
    return ESP_OK;
}

esp_err_t sht40_read_serial(uint32_t *serial)
{
    if (!s_ctx.dev) return ESP_ERR_INVALID_STATE;
    if (!serial) return ESP_ERR_INVALID_ARG;

    uint8_t cmd = SHT40_CMD_READ_SERIAL;
    uint8_t buf[6] = {0};

    esp_err_t er = sht40_i2c_write(&cmd, 1);
    if (er != ESP_OK) return er;

    vTaskDelay(pdMS_TO_TICKS(10));

    er = sht40_i2c_read(buf, sizeof(buf));
    if (er != ESP_OK) return er;

    if (sht40_crc8(buf, 2) != buf[2] || sht40_crc8(buf + 3, 2) != buf[5]) {
        DRV_LOGW("serial CRC mismatch");
        return ESP_FAIL;
    }

    uint32_t id = 0;
    id |= (uint32_t)buf[0] << 24;
    id |= (uint32_t)buf[1] << 16;
    id |= (uint32_t)buf[3] << 8;
    id |= (uint32_t)buf[4];

    *serial = id;
    return ESP_OK;
}

/* 单次测量（无重试）：供上层带重试包装调用 */
static esp_err_t sht40_do_single_measure(float *temperature_c, float *humidity_rh)
{
    if (!s_ctx.dev) return ESP_ERR_INVALID_STATE;

    uint8_t  cmd = 0;
    uint16_t wait_ms = 0;
    sht40_select_cmd(&cmd, &wait_ms);

    esp_err_t er = sht40_i2c_write(&cmd, 1);
    if (er != ESP_OK) return er;

    vTaskDelay(pdMS_TO_TICKS(wait_ms));

    uint8_t buf[6] = {0};
    er = sht40_i2c_read(buf, sizeof(buf));
    if (er != ESP_OK) return er;

    if (sht40_crc8(buf, 2) != buf[2] || sht40_crc8(buf + 3, 2) != buf[5]) {
        DRV_LOGW("measure CRC mismatch");
        return ESP_FAIL;
    }

    /* 转换为物理量：Datasheet 4.5 */
    uint16_t t_ticks  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t rh_ticks = ((uint16_t)buf[3] << 8) | buf[4];

    float t  = -45.0f + 175.0f * ((float)t_ticks  / 65535.0f);
    float rh =  -6.0f + 125.0f * ((float)rh_ticks / 65535.0f);

    if (rh > 100.0f) rh = 100.0f;
    else if (rh < 0.0f) rh = 0.0f;

    if (temperature_c) *temperature_c = t;
    if (humidity_rh)   *humidity_rh   = rh;

    return ESP_OK;
}

/* 带重试的对外读取接口：对偶发 I2C NACK 做兜底 */
#define SHT40_MAX_I2C_RETRIES 3

esp_err_t sht40_read(float *temperature_c, float *humidity_rh)
{
    esp_err_t last = ESP_FAIL;

    for (int attempt = 0; attempt < SHT40_MAX_I2C_RETRIES; ++attempt) {
        last = sht40_do_single_measure(temperature_c, humidity_rh);
        if (last == ESP_OK) {
            return ESP_OK;
        }
        /* 对连续 NACK 做一点间隔，避免总线瞬时抖动导致多次失败 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return last;
}
