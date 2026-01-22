#include "sdcard_spi.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <inttypes.h>

#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"

/* ---- 可选日志（默认关闭） ---- */
#ifndef DRV_LOG_ENABLE
#define DRV_LOG_ENABLE 0
#endif
#if DRV_LOG_ENABLE
  #include "esp_log.h"
  static const char *DRV_LOG_TAG = "SDCARD_SPI";
  #define DRV_LOGI(...) ESP_LOGI(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGW(...) ESP_LOGW(DRV_LOG_TAG, __VA_ARGS__)
  #define DRV_LOGE(...) ESP_LOGE(DRV_LOG_TAG, __VA_ARGS__)
#else
  #define DRV_LOGI(...) (void)0
  #define DRV_LOGW(...) (void)0
  #define DRV_LOGE(...) (void)0
#endif

/* ---------------- 可调工程参数（可在 sdkconfig 或编译参数覆盖） ---------------- */
#ifndef SDCARD_SPI_WORKER_STACK
#define SDCARD_SPI_WORKER_STACK   4096
#endif
#ifndef SDCARD_SPI_WORKER_PRIO
#define SDCARD_SPI_WORKER_PRIO    5
#endif
#ifndef SDCARD_SPI_WORKER_CORE
#define SDCARD_SPI_WORKER_CORE    0      /* CPU0: 与LVGL同核，因为SD卡和LCD共用SPI2，避免跨核SPI竞争 */
#endif
#ifndef SDCARD_SPI_SCAN_MAX_DEPTH
#define SDCARD_SPI_SCAN_MAX_DEPTH 4      /* 目录递归最大深度 */
#endif
#ifndef SDCARD_SPI_MAX_FILES_HINT
#define SDCARD_SPI_MAX_FILES_HINT 64     /* FATFS max_files 提示值 */
#endif
/* 默认将 SDSPI 主频限制在 10MHz，兼顾可靠性；
 * 如硬件质量较好，可在编译时通过 -DSDCARD_SPI_MAX_FREQ_KHZ 提升。 */
#ifndef SDCARD_SPI_MAX_FREQ_KHZ
#define SDCARD_SPI_MAX_FREQ_KHZ 10000
#endif

/* ---------------- 内部状态 ---------------- */
typedef enum {
    REQ_SCAN = 1,
    REQ_STOP = 2,
} sdcard_req_type_t;

typedef struct {
    sdcard_req_type_t type;
} sdcard_req_t;

static sdmmc_card_t      *s_card       = NULL;
static bool               s_mounted    = false;

static TaskHandle_t       s_worker_task = NULL;
static QueueHandle_t      s_req_queue   = NULL;
static SemaphoreHandle_t  s_list_lock   = NULL;

static sdcard_music_item_t *s_music_head  = NULL;
static size_t               s_music_count = 0;
static bool                 s_list_ready  = false;

/* ---------------- 小工具：链表管理 ---------------- */
static void free_music_list(sdcard_music_item_t *head)
{
    while (head) {
        sdcard_music_item_t *n = head->next;
        free(head->path);
        free(head->name);
        free(head);
        head = n;
    }
}

static sdcard_music_format_t detect_format(const char *name)
{
    if (!name) return SDCARD_MUSIC_FMT_UNKNOWN;
    const char *dot = strrchr(name, '.');
    if (!dot || dot[1] == '\0') return SDCARD_MUSIC_FMT_UNKNOWN;

    char ext[8] = {0};
    size_t len = 0;
    for (const char *p = dot + 1; *p && len + 1 < sizeof(ext); ++p) {
        ext[len++] = (char)tolower((unsigned char)*p);
    }
    ext[len] = '\0';

    if (strcmp(ext, "mp3") == 0)  return SDCARD_MUSIC_FMT_MP3;
    if (strcmp(ext, "wav") == 0)  return SDCARD_MUSIC_FMT_WAV;
    if (strcmp(ext, "flac") == 0) return SDCARD_MUSIC_FMT_FLAC;
    return SDCARD_MUSIC_FMT_UNKNOWN;
}

static void push_music_item(sdcard_music_item_t **head,
                            const char *full_path,
                            const char *name,
                            sdcard_music_format_t fmt,
                            uint32_t size_bytes)
{
    if (!full_path || !name || fmt == SDCARD_MUSIC_FMT_UNKNOWN) return;

    sdcard_music_item_t *node = (sdcard_music_item_t *)calloc(1, sizeof(sdcard_music_item_t));
    if (!node) return;

    size_t plen = strlen(full_path);
    size_t nlen = strlen(name);

    node->path = (char *)malloc(plen + 1);
    node->name = (char *)malloc(nlen + 1);
    if (!node->path || !node->name) {
        free(node->path);
        free(node->name);
        free(node);
        return;
    }
    memcpy(node->path, full_path, plen + 1);
    memcpy(node->name, name, nlen + 1);
    node->fmt        = fmt;
    node->size_bytes = size_bytes;

    /* 头插法：简单且 O(1)，顺序为“扫描逆序” */
    node->next = *head;
    *head = node;
}

/* ---------------- 目录扫描 ---------------- */
static void scan_dir_recursive(const char *dir_path,
                               sdcard_music_item_t **out_head,
                               size_t *out_count,
                               int depth)
{
    if (!dir_path || depth > SDCARD_SPI_SCAN_MAX_DEPTH) return;

    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *ent = NULL;
    char path_buf[256];

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (!name || name[0] == '.') continue;  /* 跳过隐藏/当前/父目录 */

        int n = snprintf(path_buf, sizeof(path_buf), "%s/%s", dir_path, name);
        if (n <= 0 || n >= (int)sizeof(path_buf)) {
            continue;  /* 路径过长，跳过 */
        }

        struct stat st;
        if (stat(path_buf, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            scan_dir_recursive(path_buf, out_head, out_count, depth + 1);
        } else if (S_ISREG(st.st_mode)) {
            sdcard_music_format_t fmt = detect_format(name);
            if (fmt != SDCARD_MUSIC_FMT_UNKNOWN) {
                push_music_item(out_head, path_buf, name, fmt, (uint32_t)st.st_size);
                if (out_count) (*out_count)++;
            }
        }
    }

    closedir(dir);
}

static void do_rescan(void)
{
    if (!s_mounted) {
        DRV_LOGW("rescan: card not mounted");
        return;
    }

    sdcard_music_item_t *new_head = NULL;
    size_t new_count = 0;

    scan_dir_recursive(SDCARD_SPI_SCAN_DIR, &new_head, &new_count, 0);

    /* 日志打印：仅输出总数，避免刷屏 */
    if (new_count == 0 || !new_head) {
        DRV_LOGI("rescan done: no audio files under \"%s\"", SDCARD_SPI_SCAN_DIR);
    } else {
        DRV_LOGI("rescan done: %u audio files under \"%s\"", (unsigned)new_count, SDCARD_SPI_SCAN_DIR);
    }

    if (s_list_lock && xSemaphoreTake(s_list_lock, portMAX_DELAY) == pdTRUE) {
        free_music_list(s_music_head);
        s_music_head  = new_head;
        s_music_count = new_count;
        s_list_ready  = true;
        xSemaphoreGive(s_list_lock);
    } else {
        /* 互斥锁不可用时直接丢弃新列表，避免内存泄漏 */
        free_music_list(new_head);
    }
}

/* ---------------- 后台任务 ---------------- */
static void sdcard_worker(void *arg)
{
    (void)arg;
    sdcard_req_t msg;

    for (;;) {
        if (!s_req_queue) break;
        if (xQueueReceive(s_req_queue, &msg, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (msg.type == REQ_STOP) {
            break;
        } else if (msg.type == REQ_SCAN) {
            do_rescan();
        }
    }

    DRV_LOGI("worker exit");
    s_worker_task = NULL;
    vTaskDelete(NULL);
}

/* ---------------- 对外 API 实现 ---------------- */
esp_err_t sdcard_spi_init(void)
{
    if (s_mounted) {
        return ESP_OK;  /* 幂等 */
    }

    /* 1) FATFS 挂载配置 */
    const esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files              = SDCARD_SPI_MAX_FILES_HINT,
        .allocation_unit_size   = 0,
    };

    /* 2) SDSPI Host + 设备配置
     *
     * 注意：此处使用“新式” SDSPI_DEVICE_CONFIG_DEFAULT，
     * SPI 总线本身（MISO/MOSI/SCLK）应由其它驱动提前通过 spi_bus_initialize 配好，
     * 例如本工程中的 ST7796 LCD 驱动已经对 SPI2_HOST 做了初始化。
     */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    sdmmc_host_t host_template = SDSPI_HOST_DEFAULT();
    host_template.slot = BOARD_SD_SPI_HOST;      /* 这里的 slot 代表“底层 host 标识” */

    /* 仅需配置 CS 引脚；MISO/MOSI/SCLK 由 spi_bus_initialize 时的 buscfg 决定 */
    slot_config.host_id = host_template.slot;
    slot_config.gpio_cs = BOARD_SD_PIN_CS;

    /* 多档频率重试：硬件/走线较差时降低主频可提高成功率 */
    const uint32_t freqs_khz[] = {
        SDCARD_SPI_MAX_FREQ_KHZ,
        8000,
        4000,
        2000,
    };

    esp_err_t err = ESP_FAIL;
    for (size_t i = 0; i < sizeof(freqs_khz)/sizeof(freqs_khz[0]); ++i) {
        uint32_t f = freqs_khz[i];
        if (f == 0) continue;
        sdmmc_host_t host = host_template;
        host.max_freq_khz = f;
        err = esp_vfs_fat_sdspi_mount(SDCARD_SPI_MOUNT_POINT,
                                      &host, &slot_config,
                                      &mount_cfg, &s_card);
        if (err == ESP_OK) {
            DRV_LOGI("SD mounted at %s, freq=%" PRIu32 " kHz", SDCARD_SPI_MOUNT_POINT, f);
            break;
        }
        DRV_LOGW("sdspi mount failed at %" PRIu32 " kHz: %s", f, esp_err_to_name(err));
        /* 清理句柄，准备下一轮 */
        s_card = NULL;
        sdspi_host_deinit();  /* safe to call even if host not fully inited */
    }

    if (err != ESP_OK) {
        DRV_LOGE("esp_vfs_fat_sdspi_mount failed after retries: %s", esp_err_to_name(err));
        s_card    = NULL;
        s_mounted = false;
        return err;
    }

    s_mounted = true;
#if DRV_LOG_ENABLE
    /* 覆盖全局日志等级，确保 SDCARD_SPI 的 INFO 能输出 */
    esp_log_level_set(DRV_LOG_TAG, ESP_LOG_INFO);
#endif
    DRV_LOGI("SD card mounted at %s", SDCARD_SPI_MOUNT_POINT);

    /* 3) 创建链表互斥锁与请求队列 */
    if (!s_list_lock) {
        s_list_lock = xSemaphoreCreateMutex();
    }
    if (!s_req_queue) {
        s_req_queue = xQueueCreate(4, sizeof(sdcard_req_t));
    }

    /* 4) 创建后台任务，并投递一次首次扫描请求 */
    if (!s_worker_task && s_req_queue) {
        BaseType_t ok = xTaskCreatePinnedToCore(
            sdcard_worker, "sdcard_worker",
            SDCARD_SPI_WORKER_STACK, NULL,
            SDCARD_SPI_WORKER_PRIO,
            &s_worker_task, SDCARD_SPI_WORKER_CORE);
        if (ok != pdPASS) {
            DRV_LOGE("create worker task failed");
            s_worker_task = NULL;
        }
    }

    if (s_req_queue) {
        sdcard_req_t msg = { .type = REQ_SCAN };
        (void)xQueueSend(s_req_queue, &msg, 0);
    }

    return ESP_OK;
}

void sdcard_spi_deinit(void)
{
    /* 1) 停止后台任务 */
    if (s_req_queue && s_worker_task) {
        sdcard_req_t msg = { .type = REQ_STOP };
        (void)xQueueSend(s_req_queue, &msg, 0);
        /* 简单等待任务退出（非严格同步） */
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* 2) 释放链表与同步原语 */
    if (s_list_lock && xSemaphoreTake(s_list_lock, portMAX_DELAY) == pdTRUE) {
        free_music_list(s_music_head);
        s_music_head  = NULL;
        s_music_count = 0;
        s_list_ready  = false;
        xSemaphoreGive(s_list_lock);
    } else {
        free_music_list(s_music_head);
        s_music_head  = NULL;
        s_music_count = 0;
        s_list_ready  = false;
    }

    if (s_list_lock) {
        vSemaphoreDelete(s_list_lock);
        s_list_lock = NULL;
    }
    if (s_req_queue) {
        vQueueDelete(s_req_queue);
        s_req_queue = NULL;
    }

    /* 3) 卸载 FATFS + SD 卡 */
    if (s_mounted && s_card) {
        esp_vfs_fat_sdcard_unmount(SDCARD_SPI_MOUNT_POINT, s_card);
        s_card = NULL;
        s_mounted = false;
        DRV_LOGI("SD card unmounted");
    }
}

bool sdcard_spi_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sdcard_music_scan_async(void)
{
    if (!s_req_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    sdcard_req_t msg = { .type = REQ_SCAN };
    if (xQueueSend(s_req_queue, &msg, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

bool sdcard_music_wait_ready(TickType_t ticks_to_wait)
{
    TickType_t start = xTaskGetTickCount();
    for (;;) {
        if (s_list_ready) return true;
        if (ticks_to_wait == 0) return false;
        TickType_t now = xTaskGetTickCount();
        if ((now - start) >= ticks_to_wait) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

size_t sdcard_music_count(void)
{
    size_t n = 0;
    if (s_list_lock && xSemaphoreTake(s_list_lock, portMAX_DELAY) == pdTRUE) {
        n = s_music_count;
        xSemaphoreGive(s_list_lock);
    }
    return n;
}

void sdcard_music_enum(sdcard_music_enum_cb_t cb, void *user)
{
    if (!cb) return;
    if (!s_list_lock) return;

    if (xSemaphoreTake(s_list_lock, portMAX_DELAY) != pdTRUE) {
        return;
    }

    sdcard_music_item_t *p = s_music_head;
    while (p) {
        if (!cb(p, user)) break;
        p = p->next;
    }

    xSemaphoreGive(s_list_lock);
}

bool sdcard_music_get_path_by_index(size_t index, char *out_path, size_t out_len)
{
    if (!out_path || out_len == 0) return false;
    if (!s_list_lock) return false;

    bool ok = false;

    if (xSemaphoreTake(s_list_lock, portMAX_DELAY) == pdTRUE) {
        size_t i = 0;
        sdcard_music_item_t *p = s_music_head;
        while (p) {
            if (i == index) {
                size_t len = strlen(p->path);
                if (len + 1 <= out_len) {
                    memcpy(out_path, p->path, len + 1);
                    ok = true;
                }
                break;
            }
            ++i;
            p = p->next;
        }
        xSemaphoreGive(s_list_lock);
    }

    return ok;
}

void sdcard_music_clear_cache(void)
{
    if (!s_list_lock) {
        free_music_list(s_music_head);
        s_music_head  = NULL;
        s_music_count = 0;
        s_list_ready  = false;
        return;
    }

    if (xSemaphoreTake(s_list_lock, portMAX_DELAY) == pdTRUE) {
        free_music_list(s_music_head);
        s_music_head  = NULL;
        s_music_count = 0;
        s_list_ready  = false;
        xSemaphoreGive(s_list_lock);
    }
}
