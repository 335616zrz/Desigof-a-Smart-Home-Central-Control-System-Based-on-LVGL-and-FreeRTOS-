#pragma once
/*
 * Board configuration (ESP32-S3-N16R8)
 * - LCD: ST7796 over SPI (RGB565)
 * - Touch: FT6336 over I2C
 * - Audio: MAX98357A over I2S (no MCLK)
 *
 * 用法原则（工程化）：
 * 1) 仅定义“板级常量与开关”，**不做驱动逻辑**；
 * 2) 默认值可被上层通过编译参数或更高优先级头文件覆盖（#ifndef 保护）；
 * 3) -1 表示未连接/不用；
 * 4) 编译期做必要约束校验，尽早暴露接线/配置问题。
 */

#include "driver/spi_common.h"   // SPIx_HOST
#include "driver/ledc.h"         // LEDC_* 常量
#include "driver/gpio.h"         // GPIO_NUM_*（如需）
#include "driver/i2s_types.h"    // I2S_NUM_*

/* ============================== LCD (SPI) ============================== */

#ifndef BOARD_LCD_HRES
#define BOARD_LCD_HRES        320      /* 逻辑宽 */
#endif
#ifndef BOARD_LCD_VRES
#define BOARD_LCD_VRES        480      /* 逻辑高 */
#endif

/* 0=竖屏, 1=顺时针90°横屏, 2=180°, 3=270° */
#ifndef BOARD_LCD_ROTATION
#define BOARD_LCD_ROTATION    1
#endif

#ifndef BOARD_LCD_MIRROR_X
#define BOARD_LCD_MIRROR_X    0        /* 1=左右镜像 */
#endif
#ifndef BOARD_LCD_MIRROR_Y
#define BOARD_LCD_MIRROR_Y    1        /* 1=上下镜像（现板卡需求） */
#endif

#ifndef BOARD_LCD_COLOR_ORDER_BGR
#define BOARD_LCD_COLOR_ORDER_BGR  1    /* 多数 ST7796 为 BGR */
#endif
#ifndef BOARD_LCD_INVERT_COLOR
#define BOARD_LCD_INVERT_COLOR     0
#endif

#ifndef BOARD_LCD_SPI_HOST
#define BOARD_LCD_SPI_HOST    SPI2_HOST /* ESP32-S3: SPI2/3 可做通用外设总线 */
#endif

#ifndef BOARD_LCD_SPI_HZ
#define BOARD_LCD_SPI_HZ      (80*1000*1000)  /* 常用 80 MHz；必要时降至 40 MHz 提升 SI 余量 */
#endif

#ifndef BOARD_LCD_PIN_SCLK
#define BOARD_LCD_PIN_SCLK    12   /* FSPICLK */
#endif
#ifndef BOARD_LCD_PIN_MOSI
#define BOARD_LCD_PIN_MOSI    11   /* FSPID */
#endif
#ifndef BOARD_LCD_PIN_MISO
#define BOARD_LCD_PIN_MISO    13   /* FSPIQ；未用可设为 -1 */
#endif
#ifndef BOARD_LCD_PIN_CS
#define BOARD_LCD_PIN_CS      10   /* FSPICS0 */
#endif
#ifndef BOARD_LCD_PIN_DC
#define BOARD_LCD_PIN_DC      4
#endif
#ifndef BOARD_LCD_PIN_RST
#define BOARD_LCD_PIN_RST     5
#endif
#ifndef BOARD_LCD_PIN_BL
#define BOARD_LCD_PIN_BL      16    /* 背光控制脚（可接 PWM） */
#endif

/* 背光 PWM（LEDC） */
#ifndef BOARD_BL_USE_PWM
#define BOARD_BL_USE_PWM          1
#endif
#ifndef BOARD_BL_LEDC_TIMER
#define BOARD_BL_LEDC_TIMER       LEDC_TIMER_0
#endif
#ifndef BOARD_BL_LEDC_MODE
#define BOARD_BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#endif
#ifndef BOARD_BL_LEDC_CHANNEL
#define BOARD_BL_LEDC_CHANNEL     LEDC_CHANNEL_0
#endif
#ifndef BOARD_BL_PWM_FREQ_HZ
#define BOARD_BL_PWM_FREQ_HZ      20000       /* 20 kHz，避开可闻噪声 */
#endif
#ifndef BOARD_BL_PWM_RES_BITS
#define BOARD_BL_PWM_RES_BITS     LEDC_TIMER_13_BIT
#endif
#ifndef BOARD_BL_PWM_DUTY_MAX
#define BOARD_BL_PWM_DUTY_MAX     ((1U<<13)-1)
#endif
#ifndef BOARD_BL_ACTIVE_HIGH
#define BOARD_BL_ACTIVE_HIGH      1           /* 背光高电平点亮 */
#endif

/* ============================ Touch (FT6336) ============================ */

#ifndef BOARD_TOUCH_I2C_PORT
#define BOARD_TOUCH_I2C_PORT   0
#endif
#ifndef BOARD_TOUCH_I2C_HZ
#define BOARD_TOUCH_I2C_HZ     (400*1000)
#endif
#ifndef BOARD_TOUCH_SCL
#define BOARD_TOUCH_SCL        17
#endif
#ifndef BOARD_TOUCH_SDA
#define BOARD_TOUCH_SDA        18
#endif
#ifndef BOARD_TOUCH_INT
#define BOARD_TOUCH_INT        6     /* 下降沿中断 */
#endif
#ifndef BOARD_TOUCH_RST
#define BOARD_TOUCH_RST        21
#endif
#ifndef BOARD_TOUCH_ADDR
#define BOARD_TOUCH_ADDR       0x38
#endif

/* 触摸坐标纠正：相对 LCD 额外旋转（单位 90°）；镜像开关 */
#ifndef BOARD_TOUCH_ROT_OFFSET
#define BOARD_TOUCH_ROT_OFFSET 0
#endif
#ifndef BOARD_TOUCH_MIRROR_X
#define BOARD_TOUCH_MIRROR_X   1
#endif
#ifndef BOARD_TOUCH_MIRROR_Y
#define BOARD_TOUCH_MIRROR_Y   1
#endif

/* ============================ SHT4x (Temp & RH) ============================ */
/* 与触摸共用 I2C0 总线（GPIO17/18），避免重复维护引脚宏 */

#ifndef BOARD_SHT4X_I2C_PORT
#define BOARD_SHT4X_I2C_PORT   BOARD_TOUCH_I2C_PORT
#endif

#ifndef BOARD_SHT4X_I2C_HZ
#define BOARD_SHT4X_I2C_HZ     BOARD_TOUCH_I2C_HZ   /* 默认 400 kHz，SHT4x 最高 1 MHz */
#endif

#ifndef BOARD_SHT4X_ADDR
#define BOARD_SHT4X_ADDR       0x44                 /* 常见 SHT40-AD1B 默认地址 */
#endif

/* ======================= SD Card over SPI2 ======================= */

#ifndef BOARD_SD_SPI_HOST
#define BOARD_SD_SPI_HOST      BOARD_LCD_SPI_HOST   /* 复用 SPI2_HOST */
#endif

#ifndef BOARD_SD_PIN_MOSI
#define BOARD_SD_PIN_MOSI      BOARD_LCD_PIN_MOSI   /* 11 */
#endif
#ifndef BOARD_SD_PIN_MISO
#define BOARD_SD_PIN_MISO      BOARD_LCD_PIN_MISO   /* 13 */
#endif
#ifndef BOARD_SD_PIN_SCLK
#define BOARD_SD_PIN_SCLK      BOARD_LCD_PIN_SCLK   /* 12 */
#endif
#ifndef BOARD_SD_PIN_CS
#define BOARD_SD_PIN_CS        15   /* ★ 推荐：GPIO15 作为 SD_CS */
#endif

/* ======================= Audio (MAX98357A via I2S) ====================== */
/* 仅需 BCLK、LRCLK(WS)、DOUT；MCLK 未用 */

#ifndef BOARD_I2S_OUT_NUM
#define BOARD_I2S_OUT_NUM      I2S_NUM_0   /* 音频输出使用 I2S 控制器 0 */
#endif

/* I2S DMA 配置（需在内部 RAM 分配，注意总量控制） */
#ifndef BOARD_I2S_OUT_DMA_DESC_NUM
#define BOARD_I2S_OUT_DMA_DESC_NUM   6     /* 音频输出 DMA 描述符数量 */
#endif
#ifndef BOARD_I2S_OUT_DMA_FRAME_NUM
#define BOARD_I2S_OUT_DMA_FRAME_NUM  256   /* 音频输出每帧样本数 */
#endif

#ifndef BOARD_I2S_BCLK
#define BOARD_I2S_BCLK         1
#endif
#ifndef BOARD_I2S_LRCLK
#define BOARD_I2S_LRCLK        2
#endif
#ifndef BOARD_I2S_DOUT
#define BOARD_I2S_DOUT         3
#endif
#ifndef BOARD_I2S_MCLK
#define BOARD_I2S_MCLK         (-1)   /* 未使用 */
#endif

#ifndef BOARD_AUDIO_SAMPLE_RATE_DEFAULT
#define BOARD_AUDIO_SAMPLE_RATE_DEFAULT  48000
#endif
#ifndef BOARD_AUDIO_BITS_PER_SAMPLE
#define BOARD_AUDIO_BITS_PER_SAMPLE      16
#endif
#ifndef BOARD_AUDIO_NUM_CHANNELS
#define BOARD_AUDIO_NUM_CHANNELS         2
#endif

/* 可选功放控制脚（-1 表示未接） */
#ifndef BOARD_AMP_SD_MODE
#define BOARD_AMP_SD_MODE      8       /* 若未接，改为 -1 */
#endif
#ifndef BOARD_AMP_GAIN_SLOT
#define BOARD_AMP_GAIN_SLOT    (-1)
#endif
#ifndef BOARD_AMP_ENABLE
#define BOARD_AMP_ENABLE       (-1)
#endif

/* ======================= 麦克风 (INMP441 via I2S) ======================= */
#ifndef BOARD_I2S_MIC_NUM
#define BOARD_I2S_MIC_NUM        I2S_NUM_1   /* 麦克风使用 I2S 控制器 1 */
#endif

#ifndef BOARD_I2S_MIC_DMA_DESC_NUM
#define BOARD_I2S_MIC_DMA_DESC_NUM   6       /* 麦克风 DMA 描述符数量 */
#endif
#ifndef BOARD_I2S_MIC_DMA_FRAME_NUM
#define BOARD_I2S_MIC_DMA_FRAME_NUM  256     /* 麦克风每帧样本数 */
#endif

#ifndef BOARD_I2S_MIC_BCLK
#define BOARD_I2S_MIC_BCLK       19
#endif

#ifndef BOARD_I2S_MIC_LRCLK
#define BOARD_I2S_MIC_LRCLK      45
#endif

#ifndef BOARD_I2S_MIC_DATA
#define BOARD_I2S_MIC_DATA       47
#endif

#ifndef BOARD_I2S_MIC_LR_SEL
#define BOARD_I2S_MIC_LR_SEL     (-1)
#endif

#ifndef BOARD_I2S_MIC_CHIPEN
#define BOARD_I2S_MIC_CHIPEN     (-1)
#endif

/* ======================== WS2812B (Neopixel RGB LED) ======================= */
/* 原理图：GPIO48 -> WS2812B DIN，单颗 5050 RGB（GRB 时序，800kHz） */

#ifndef BOARD_WS2812_GPIO
#define BOARD_WS2812_GPIO        48
#endif

#ifndef BOARD_WS2812_LED_NUM
#define BOARD_WS2812_LED_NUM     1
#endif


/* ========================= Build-time Validations ======================= */
/* 尽早发现配置错误：简单范围/关系校验，必要时 #error 中断编译 */

/* SPI 时钟上限（大多数 ST7796 在 80 MHz 左右稳定；如不稳定请降频） */
#if (BOARD_LCD_SPI_HZ > 80000000)
  #error "BOARD_LCD_SPI_HZ 超过 80MHz，建议 ≤ 80MHz（必要时 40MHz 提升 SI 余量）"
#endif

/* GPIO 合法性：允许 -1（未接）或 0..48（S3 范围）；如项目需要更严谨可在此扩展黑名单 */
#define __BOARD_GPIO_VALID(g)  ((g) >= -1 && (g) <= 48)

#if !__BOARD_GPIO_VALID(BOARD_LCD_PIN_SCLK) || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_MOSI) || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_MISO) || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_CS)   || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_DC)   || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_RST)  || \
    !__BOARD_GPIO_VALID(BOARD_LCD_PIN_BL)
  #error "LCD GPIO 配置越界，请检查 board_config.h"
#endif

#if !__BOARD_GPIO_VALID(BOARD_TOUCH_SCL) || \
    !__BOARD_GPIO_VALID(BOARD_TOUCH_SDA) || \
    !__BOARD_GPIO_VALID(BOARD_TOUCH_INT) || \
    !__BOARD_GPIO_VALID(BOARD_TOUCH_RST)
  #error "Touch GPIO 配置越界，请检查 board_config.h"
#endif

#if !__BOARD_GPIO_VALID(BOARD_I2S_BCLK)  || \
    !__BOARD_GPIO_VALID(BOARD_I2S_LRCLK) || \
    !__BOARD_GPIO_VALID(BOARD_I2S_DOUT)  || \
    !__BOARD_GPIO_VALID(BOARD_I2S_MCLK)
  #error "I2S GPIO 配置越界，请检查 board_config.h"
#endif

#if !__BOARD_GPIO_VALID(BOARD_WS2812_GPIO)
  #error "WS2812 GPIO 配置越界，请检查 board_config.h"
#endif

#if (BOARD_LCD_ROTATION < 0) || (BOARD_LCD_ROTATION > 3)
  #error "BOARD_LCD_ROTATION 取值应为 0..3"
#endif

/* 触摸 I2C 端口：仅允许 0 或 1（ESP32-S3） */
#if (BOARD_TOUCH_I2C_PORT != 0) && (BOARD_TOUCH_I2C_PORT != 1)
  #error "BOARD_TOUCH_I2C_PORT 仅支持 0 或 1"
#endif

/* ============================ 集成提示 ============================ */
/* 若 SPI/I2S/I2C 与 PSRAM/Flash/下载口冲突，请在硬件上避开或在此头文件重映射。
 * 显示颠倒优先调 BOARD_LCD_MIRROR_X/Y 或 ROTATION；触摸错位先调 BOARD_TOUCH_MIRROR_X/Y，
 * 再调 BOARD_TOUCH_ROT_OFFSET。*/
