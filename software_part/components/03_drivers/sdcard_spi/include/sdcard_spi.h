#pragma once
/**
 * @file sdcard_spi.h
 * @brief 基于 SPI2 的 SD 卡 + 音乐文件索引驱动（FreeRTOS 任务 + 队列 + 链表）
 *
 * 功能概述：
 * - 通过 board_config.h 中的 BOARD_SD_* GPIO 宏，经 SPI Host（默认 SPI2）挂载 SD 卡；
 * - 使用 FATFS 注册到 VFS，默认挂载点为 "/sdcard"；
 * - 后台任务异步扫描 SD 卡上的音乐文件（mp3/wav/flac），使用单向链表缓存索引；
 * - 提供简单 API 供 UI/播放器枚举歌曲、按索引获取路径、触发重新扫描等。
 *
 * 使用约定：
 * 1) 在 LCD/触摸初始化完成后再调用 sdcard_spi_init()，避免 SPI 总线先被 SD 抢占；
 * 2) 只在一个任务中调用 init/deinit，其他任务只读接口；
 * 3) 调用方使用返回的“绝对路径”直接通过 fopen/fread 等标准 API 读取文件内容。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 支持的音乐文件格式 */
typedef enum {
    SDCARD_MUSIC_FMT_UNKNOWN = 0,
    SDCARD_MUSIC_FMT_MP3,
    SDCARD_MUSIC_FMT_WAV,
    SDCARD_MUSIC_FMT_FLAC,
} sdcard_music_format_t;

/**
 * 音乐文件索引节点（单向链表）
 * - 所有字符串由驱动内部动态分配；
 * - 对外只读，不要在回调中修改其内容。
 */
typedef struct sdcard_music_item {
    struct sdcard_music_item *next;
    char *path;                     /**< 绝对路径，例如 "/sdcard/foo/bar.mp3" */
    char *name;                     /**< 文件名（不含路径） */
    sdcard_music_format_t fmt;      /**< 文件格式 */
    uint32_t size_bytes;            /**< 文件大小（字节） */
} sdcard_music_item_t;

/** 枚举回调，返回 false 可提前停止枚举 */
typedef bool (*sdcard_music_enum_cb_t)(const sdcard_music_item_t *item, void *user);

/** 默认挂载点与扫描目录，可在编译期通过 -DSDCARD_SPI_* 覆盖 */
#ifndef SDCARD_SPI_MOUNT_POINT
#define SDCARD_SPI_MOUNT_POINT   "/sdcard"
#endif
#ifndef SDCARD_SPI_SCAN_DIR
#define SDCARD_SPI_SCAN_DIR      SDCARD_SPI_MOUNT_POINT
#endif

/**
 * @brief 初始化 SD 卡 SPI 驱动并挂载 FATFS
 *
 * - 使用 board_config.h 中 BOARD_SD_SPI_HOST / BOARD_SD_PIN_* 宏；
 * - 默认不格式化卡（format_if_mount_failed=false）；
 * - 成功后自动创建后台扫描任务，并触发一次异步扫描。
 *
 * @return ESP_OK 成功；其它值为失败原因。
 */
esp_err_t sdcard_spi_init(void);

/**
 * @brief 停止后台任务并卸载 SD 卡（若已挂载）
 *
 * - 通常在系统关机或需要安全拔卡前调用；
 * - 调用后 sdcard_spi_is_mounted() 将返回 false。
 */
void sdcard_spi_deinit(void);

/**
 * @brief SD 卡是否已挂载成功
 */
bool sdcard_spi_is_mounted(void);

/**
 * @brief 触发一次异步重新扫描（仅投递队列，不阻塞）
 *
 * - 若扫描队列已满则静默丢弃（返回 ESP_ERR_TIMEOUT）；
 * - 典型用法：UI 检测到“刷新”操作时调用。
 */
esp_err_t sdcard_music_scan_async(void);

/**
 * @brief 阻塞等待“第一次扫描完成”
 *
 * - 用于在 UI 进入音乐界面前确保至少有一次扫描结果；
 * - 若在超时时间内扫描完成返回 true，否则返回 false。
 *
 * @param ticks_to_wait  最大等待时间（FreeRTOS Tick）
 */
bool sdcard_music_wait_ready(TickType_t ticks_to_wait);

/**
 * @brief 获取当前缓存的音乐文件数量
 *
 * - 仅统计最近一次扫描得到的链表长度；
 * - 若尚未完成任何扫描则返回 0。
 */
size_t sdcard_music_count(void);

/**
 * @brief 枚举所有音乐条目（在内部持有互斥锁）
 *
 * @param cb    回调函数（不可为 NULL），在链表上逐个调用；
 * @param user  透传给回调的用户指针；
 *
 * 注意：
 * - 回调中尽量避免耗时操作或阻塞调用；
 * - 不要在回调中调用会再次获取内部锁的 API（避免死锁）。
 */
void sdcard_music_enum(sdcard_music_enum_cb_t cb, void *user);

/**
 * @brief 按索引获取音乐文件绝对路径
 *
 * @param index      0-based 索引（0..count-1）
 * @param out_path   输出缓冲区
 * @param out_len    缓冲区长度（包含结尾 '\0'）
 * @return true 成功写入完整路径；false 索引越界或缓冲区太小。
 */
bool sdcard_music_get_path_by_index(size_t index, char *out_path, size_t out_len);

/**
 * @brief 手动清空内部链表缓存
 *
 * - 通常不需要调用；在某些特殊场景下可以用于释放内存；
 * - 清空后再次使用前应调用 sdcard_music_scan_async() 重新扫描。
 */
void sdcard_music_clear_cache(void);

#ifdef __cplusplus
}
#endif

