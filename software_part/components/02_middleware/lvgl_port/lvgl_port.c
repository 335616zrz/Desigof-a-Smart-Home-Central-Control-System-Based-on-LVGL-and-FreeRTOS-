// lvgl_port.c
#include "lvgl_port.h"
#include "lvgl.h"
#include "sdkconfig.h"
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_timer.h"         // esp_timer for LVGL tick
#include "esp_task_wdt.h"
#include "esp_memory_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"   // 递归互斥
#include "esp_heap_caps.h"     // DMA 内存

#include "st7796_driver.h"
#include "ft6336_driver.h"

static const char *TAG = "LVGL_PORT";

#ifndef CONFIG_LVGL_PORT_EXTRA_MEM_KB
#define CONFIG_LVGL_PORT_EXTRA_MEM_KB 0
#endif
#ifndef CONFIG_LV_MEM_SIZE_KILOBYTES
#define CONFIG_LV_MEM_SIZE_KILOBYTES 0
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_TRACE_FACILITY == 1 )
  /* 统一“运行时统计时基”的当前值：
     - 若启用 CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER：返回微秒
     - 否则：返回 portGET_RUN_TIME_COUNTER_VALUE()（单位与 idle 计数一致） */
  #if defined(CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER) && CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER
    #define RT_NOW_UNITS()   ((uint64_t)esp_timer_get_time())
  #else
    #ifndef portGET_RUN_TIME_COUNTER_VALUE
      #error "Run-time stats enabled but portGET_RUN_TIME_COUNTER_VALUE undefined"
    #endif
    #define RT_NOW_UNITS()   ((uint64_t)portGET_RUN_TIME_COUNTER_VALUE())
  #endif
#endif

/* ========================= 可调参数 ========================= */
#ifndef LVGL_PORT_BUF_LINES
#define LVGL_PORT_BUF_LINES   12   /* 更稳：每次处理 12 行 */
#endif

#ifndef FLUSH_SLICE_LINES
#define FLUSH_SLICE_LINES      12   /* 更稳：切片高度 12 行 */
#endif

#ifndef LVGL_PORT_BE_DIRECT
#define LVGL_PORT_BE_DIRECT     0   /* 非直通：由驱动完成 BE 发送 */
#endif

/* 字节序需求判定 */
#if defined(LV_COLOR_16_SWAP) && LV_COLOR_16_SWAP
  #define LVGL_NEED_BYTESWAP (!LVGL_PORT_BE_DIRECT)
#else
  #define LVGL_NEED_BYTESWAP (LVGL_PORT_BE_DIRECT)
#endif

#ifndef LVGL_PORT_PREFER_DOUBLE
#define LVGL_PORT_PREFER_DOUBLE  1
#endif

#ifndef LVGL_PORT_STATS_LOG
#define LVGL_PORT_STATS_LOG      0
#endif

/* ========================= 内部状态 ========================= */
static volatile uint32_t s_flush_cnt = 0;            // 每次 flush 完成 +1（近似 FPS）
static volatile uint32_t s_flush_px_accum = 0;       // 累计刷新像素（算 FS-FPS）
static TaskHandle_t s_idle0 = NULL, s_idle1 = NULL;  // 空闲任务句柄

/* 注意：该变量使用“运行时统计的同一时基” */
static uint64_t   s_prev_rt_units   = 0;
static uint32_t   s_prev_idle0_cnt  = 0;
static uint32_t   s_prev_idle1_cnt  = 0;

static lv_display_t     *s_disp       = NULL;
static lv_indev_t       *s_indev      = NULL;
static TaskHandle_t      s_lvgl_task  = NULL;
static SemaphoreHandle_t s_lvgl_mutex = NULL;
static bool              s_inited     = false;

static int               s_scr_w      = 0;
static int               s_scr_h      = 0;
static uint32_t          s_scr_px     = 0;
static int               s_slice_lines = FLUSH_SLICE_LINES;  // 实际切片高度（可降级）

#if LVGL_NEED_BYTESWAP
static uint16_t *s_linebuf    = NULL;  // DMA 行缓冲（宽 * 切片行数）
static int       s_linecap_px = 0;
#endif

#if CONFIG_LVGL_PORT_EXTRA_MEM_KB > 0
static void *s_lvgl_extra_pool_mem = NULL;
#endif

/* ================= 常驻叠层：FPS + CPU 使用率 ================= */
#define LVGL_PORT_FPS_OVERLAY 1
#define LVGL_PORT_CPU_OVERLAY 1

#if LVGL_PORT_FPS_OVERLAY
static lv_obj_t   *s_fps_label = NULL;
static lv_timer_t *s_fps_timer = NULL;
static uint32_t    s_flush_cnt_prev = 0;

static void fps_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    uint32_t now = s_flush_cnt;
    uint32_t d   = now - s_flush_cnt_prev;   // 500ms 内 flush 次数
    s_flush_cnt_prev = now;

    float fps = (float)d * 2.0f;             // 半秒计数 -> *2
    if (s_fps_label) {
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "%4.1f FPS", fps);
        lv_label_set_text(s_fps_label, buf);
        lv_obj_align(s_fps_label, LV_ALIGN_BOTTOM_RIGHT, -3, -3);
    }
}
#endif

#if LVGL_PORT_CPU_OVERLAY
static lv_obj_t   *s_cpu_label = NULL;
static lv_timer_t *s_cpu_timer = NULL;

static inline uint32_t clamp100(uint32_t x){ return x>100?100:x; }

/* 第一次调用时做一次基线采样（统一同一时基） */
static bool cpu_stats_try_init_baseline(void)
{
#if ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_TRACE_FACILITY == 1 )
    if (!s_idle0 || !s_idle1) {
        s_idle0 = xTaskGetIdleTaskHandleForCore(0);
        s_idle1 = xTaskGetIdleTaskHandleForCore(1);
        if (!s_idle0 || !s_idle1) return false;

        TaskStatus_t st0_i, st1_i;
        vTaskGetInfo(s_idle0, &st0_i, pdTRUE, eInvalid);
        vTaskGetInfo(s_idle1, &st1_i, pdTRUE, eInvalid);
        s_prev_idle0_cnt = st0_i.ulRunTimeCounter;
        s_prev_idle1_cnt = st1_i.ulRunTimeCounter;
        s_prev_rt_units  = RT_NOW_UNITS();
    }
    return true;
#else
    return false;
#endif
}

static void cpu_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
#if ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_TRACE_FACILITY == 1 )
    if (!cpu_stats_try_init_baseline()) {
        if (s_cpu_label) {
            lv_label_set_text(s_cpu_label, "C0 --%  C1 --%");
            lv_obj_align(s_cpu_label, LV_ALIGN_BOTTOM_RIGHT, -3, -22);
        }
        return;
    }

    TaskStatus_t st0, st1;
    vTaskGetInfo(s_idle0, &st0, pdTRUE, eInvalid);
    vTaskGetInfo(s_idle1, &st1, pdTRUE, eInvalid);

    uint32_t idle0_now = st0.ulRunTimeCounter;
    uint32_t idle1_now = st1.ulRunTimeCounter;

    uint64_t now = RT_NOW_UNITS();            /* 与 idle 计数同一时基 */
    uint64_t dt  = now - s_prev_rt_units;
    if (dt == 0) dt = 1;                      /* 防止除零 */

    uint32_t d_idle0 = idle0_now - s_prev_idle0_cnt;
    uint32_t d_idle1 = idle1_now - s_prev_idle1_cnt;

    /* 每核占用率 = 1 - (idle_delta / dt) */
    uint32_t c0 = clamp100((uint32_t)(((dt - (uint64_t)d_idle0) * 100ULL + dt/2) / dt));
    uint32_t c1 = clamp100((uint32_t)(((dt - (uint64_t)d_idle1) * 100ULL + dt/2) / dt));

    if (s_cpu_label) {
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "C0 %2" PRIu32 "%%  C1 %2" PRIu32 "%%", c0, c1);
        lv_label_set_text(s_cpu_label, buf);
        lv_obj_align(s_cpu_label, LV_ALIGN_BOTTOM_RIGHT, -3, -22);
    }

    s_prev_rt_units  = now;
    s_prev_idle0_cnt = idle0_now;
    s_prev_idle1_cnt = idle1_now;
#else
    if (s_cpu_label) {
        lv_label_set_text(s_cpu_label, "C0 --%  C1 --%");
        lv_obj_align(s_cpu_label, LV_ALIGN_BOTTOM_RIGHT, -3, -22);
    }
#endif
}
#endif

#if LVGL_PORT_STATS_LOG
/* -------- 每秒打印到 monitor：FPS + FS-FPS + CPU -------- */
static lv_timer_t *s_stats_timer = NULL;
static void stats_log_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    /* FPS（过去 1 秒 flush 次数） */
    static uint32_t s_prev_flush = 0;
    uint32_t now_flush = s_flush_cnt;
    uint32_t d_flush   = now_flush - s_prev_flush;
    s_prev_flush = now_flush;
    uint32_t fps10 = d_flush * 10;

    /* FS-FPS（过去 1 秒刷新像素 / 全屏像素） */
    static uint32_t s_prev_px = 0;
    uint32_t now_px = s_flush_px_accum;
    uint32_t d_px   = now_px - s_prev_px;
    s_prev_px = now_px;

    uint32_t fsfps10 = 0;
    if (s_scr_px) {
        fsfps10 = (uint32_t)(((uint64_t)d_px * 10ULL + s_scr_px/2ULL) / (uint64_t)s_scr_px);
    }

#if ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_TRACE_FACILITY == 1 )
    if (!cpu_stats_try_init_baseline()) {
        ESP_LOGI("LVGL_STATS",
                 "FPS=%" PRIu32 ".%" PRIu32 "  FS-FPS=%" PRIu32 ".%" PRIu32 "  (no CPU stats; baseline pending)",
                 (fps10/10), (fps10%10), (fsfps10/10), (fsfps10%10));
        return;
    }

    TaskStatus_t st0, st1;
    vTaskGetInfo(s_idle0, &st0, pdTRUE, eInvalid);
    vTaskGetInfo(s_idle1, &st1, pdTRUE, eInvalid);
    uint32_t idle0_now = st0.ulRunTimeCounter;
    uint32_t idle1_now = st1.ulRunTimeCounter;

    uint64_t t_now = RT_NOW_UNITS();          /* 与 idle 计数同一时基 */
    uint64_t dt    = t_now - s_prev_rt_units;
    if (dt == 0) dt = 1;

    uint32_t d_idle0 = idle0_now - s_prev_idle0_cnt;
    uint32_t d_idle1 = idle1_now - s_prev_idle1_cnt;

    uint32_t c0 = clamp100((uint32_t)(((dt - (uint64_t)d_idle0) * 100ULL + dt/2) / dt));
    uint32_t c1 = clamp100((uint32_t)(((dt - (uint64_t)d_idle1) * 100ULL + dt/2) / dt));

    ESP_LOGI("LVGL_STATS",
             "FPS=%" PRIu32 ".%" PRIu32 "  FS-FPS=%" PRIu32 ".%" PRIu32 "  CPU: C0=%" PRIu32 "%%  C1=%" PRIu32 "%%",
             (fps10/10), (fps10%10),
             (fsfps10/10), (fsfps10%10),
             c0, c1);

    s_prev_rt_units  = t_now;
    s_prev_idle0_cnt = idle0_now;
    s_prev_idle1_cnt = idle1_now;
#else
    ESP_LOGI("LVGL_STATS",
             "FPS=%" PRIu32 ".%" PRIu32 "  FS-FPS=%" PRIu32 ".%" PRIu32 "  (no CPU stats; enable run-time stats)",
             (fps10/10), (fps10%10),
             (fsfps10/10), (fsfps10%10));
#endif
}
#endif

/* ================= 工具/互斥 ================= */
static uint32_t lvgl_tick_get_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static inline TickType_t ms_to_ticks_min1(uint32_t ms)
{
    TickType_t t = pdMS_TO_TICKS(ms);
    return (t == 0) ? 1 : t;
}

/* === 对外导出的互斥接口（供其他任务安全调用 LVGL） === */
bool lvgl_port_lock(uint32_t timeout_ms)
{
    if (!s_lvgl_mutex) return false;
    TickType_t to = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mutex, to) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    if (s_lvgl_mutex) xSemaphoreGiveRecursive(s_lvgl_mutex);
}

/* ================= 刷屏回调：按“切片”多行发送 ================= */
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)disp;
    if (!area || !px_map) { lv_display_flush_ready(disp); return; }

    const int32_t x  = area->x1;
    const int32_t y0 = area->y1;
    const int32_t w  = area->x2 - area->x1 + 1;
    const int32_t h  = area->y2 - area->y1 + 1;

    const uint16_t *src = (const uint16_t *)px_map;

    for (int32_t y = 0; y < h; ) {
        (void)esp_task_wdt_reset();
        int32_t slice = s_slice_lines;
        if (slice < 1) slice = 1;
        if (y + slice > h) slice = h - y;

#if LVGL_NEED_BYTESWAP
        uint16_t *d = s_linebuf;
        if (!d || s_linecap_px < w * slice) {
            ESP_LOGE(TAG, "line buffer too small (%d < %ld)", s_linecap_px, (long)(w * slice));
            lv_display_flush_ready(disp);
            return;
        }
        for (int r = 0; r < slice; r++) {
            const uint16_t *s = src + (y + r) * w;
            uint16_t *dd = d + r * w;
            for (int c = 0; c < w; c++) {
                uint16_t cc = s[c];
                dd[c] = (uint16_t)((cc << 8) | (cc >> 8));
            }
        }
        st7796_draw_bitmap((int)x, (int)(y0 + y), (int)w, (int)slice, d);
#else
        const uint16_t *p = src + y * w;
        st7796_draw_bitmap((int)x, (int)(y0 + y), (int)w, (int)slice, p);
#endif
        y += slice;
        (void)esp_task_wdt_reset();
    }

    s_flush_px_accum += (uint32_t)((uint32_t)w * (uint32_t)h);
    s_flush_cnt++;
    lv_display_flush_ready(disp);
}

/* ================= 触摸回调 ================= */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int tx, ty;
    if (ft6336_read_point(&tx, &ty)) {
        data->point.x = (lv_coord_t)tx;
        data->point.y = (lv_coord_t)ty;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
    }
}

/* ================= LVGL 后台任务（自适应等待） ================= */
#define LVGL_WDT_ADD()    do{ (void)esp_task_wdt_add(NULL); }while(0)
#define LVGL_WDT_RESET()  do{ (void)esp_task_wdt_reset(); }while(0)

static void lvgl_task(void *arg)
{
    (void)arg;
    LVGL_WDT_ADD();

    while (1) {
        LVGL_WDT_RESET();
        uint32_t wait_ms = 10;

        if (lvgl_port_lock(2)) {                // 2ms 内抢锁
            LVGL_WDT_RESET();                   // 进重活前先喂一次
            uint32_t next = lv_timer_handler(); // 可能较重
            lvgl_port_unlock();

            wait_ms = next;                     // 用 LVGL 给的下次调度间隔
        } else {
            wait_ms = 2;                        // 抢锁失败→快速重试，降低输入/动画延迟
        }

        // 夹在 [1, 20] ms，既不忙转也不拖延
        if (wait_ms > 20) wait_ms = 20;
        if (wait_ms < 1)  wait_ms = 1;

        vTaskDelay(ms_to_ticks_min1(wait_ms));
        LVGL_WDT_RESET();                       // 跑完一轮再喂一次
    }
}

/* ================= 初始化 ================= */
static void* try_malloc(size_t bytes)
{
    void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        p = heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    }
    return p;
}

#if CONFIG_LVGL_PORT_EXTRA_MEM_KB > 0
static bool extend_lvgl_heap(void)
{
    if (s_lvgl_extra_pool_mem) {
        return true;
    }

    size_t request = (size_t)CONFIG_LVGL_PORT_EXTRA_MEM_KB * 1024U;
    if (request == 0) {
        return true;
    }

    const size_t min_block = 16U * 1024U;
    size_t attempt = request;

    while (attempt >= min_block) {
        void *pool = heap_caps_malloc(attempt, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        const char *region = "SPIRAM";
        if (!pool) {
            pool = heap_caps_malloc(attempt, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            region = "internal";
        }

        if (!pool) {
            attempt -= 16U * 1024U;
            continue;
        }

        lv_mem_pool_t pool_handle = lv_mem_add_pool(pool, attempt);
        if (pool_handle) {
            s_lvgl_extra_pool_mem = pool;
            if (attempt < request) {
                ESP_LOGI(TAG, "extend LVGL heap: requested %u KB, granted %u KB (%s)",
                         (unsigned)(request / 1024U), (unsigned)(attempt / 1024U), region);
            } else {
                ESP_LOGI(TAG, "extend LVGL heap: +%u KB (%s)",
                         (unsigned)(attempt / 1024U), region);
            }
            return true;
        }

        ESP_LOGD(TAG, "lv_mem_add_pool failed for %u bytes, retry smaller chunk", (unsigned)attempt);
        heap_caps_free(pool);
        attempt -= 16U * 1024U;
    }

    ESP_LOGE(TAG, "extend LVGL heap failed (requested %u KB)", (unsigned)(request / 1024U));
    return false;
}
#else
static inline bool extend_lvgl_heap(void)
{
    return true;
}
#endif

static bool alloc_display_buffers(uint32_t bpp,
                                  int *out_lines,
                                  void **out_b1, void **out_b2,
                                  bool *out_double)
{
    static const int plan[] = { LVGL_PORT_BUF_LINES, 96, 88, 80, 72, 64, 56, 48, 40, 32, 24, 16, 8 };
    void *b1 = NULL, *b2 = NULL;

#if LVGL_PORT_PREFER_DOUBLE
    /* 先尝试双缓冲 */
    for (size_t i = 0; i < sizeof(plan)/sizeof(plan[0]); i++) {
        int lines = plan[i];
        if (lines <= 0) continue;

        size_t bytes = (size_t)s_scr_w * (size_t)lines * (size_t)bpp;
        b1 = try_malloc(bytes);
        b2 = try_malloc(bytes);
        if (b1 && b2) {
            *out_lines  = lines;
            *out_b1     = b1;
            *out_b2     = b2;
            *out_double = true;
            ESP_LOGI(TAG, "alloc OK: double buf, lines=%d, %u bytes each", lines, (unsigned)bytes);
            return true;
        }
        if (b1) { heap_caps_free(b1); b1 = NULL; }
        if (b2) { heap_caps_free(b2); b2 = NULL; }
    }
#endif

    /* 再尝试单缓冲 */
    for (size_t i = 0; i < sizeof(plan)/sizeof(plan[0]); i++) {
        int lines = plan[i];
        if (lines <= 0) continue;

        size_t bytes = (size_t)s_scr_w * (size_t)lines * (size_t)bpp;
        b1 = try_malloc(bytes);
        if (b1) {
            *out_lines  = lines;
            *out_b1     = b1;
            *out_b2     = NULL;
            *out_double = false;
            ESP_LOGW(TAG, "alloc OK: single buf, lines=%d, %u bytes", lines, (unsigned)bytes);
            return true;
        }
    }

    return false;
}

bool lvgl_port_init(void)
{
    if (s_inited) {
        ESP_LOGI(TAG, "already initialized");
        return true;
    }

    /* 显示/触摸硬件应由上层先完成初始化 */

    /* 1) 初始化 LVGL + tick 源 */
    lv_init();
#if LVGL_VERSION_MAJOR >= 9
    lv_tick_set_cb(lvgl_tick_get_ms);
#endif
    if (!extend_lvgl_heap()) {
        ESP_LOGW(TAG, "extra LVGL heap unavailable, only %u KB base pool active",
                 (unsigned)CONFIG_LV_MEM_SIZE_KILOBYTES);
    }

    /* 2) 递归互斥 */
    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (!s_lvgl_mutex) {
        ESP_LOGE(TAG, "create lvgl mutex failed");
        return false;
    }

    /* 3) 获取面板尺寸 */
    s_scr_w = st7796_width();
    s_scr_h = st7796_height();
    if (s_scr_w <= 0 || s_scr_h <= 0) {
        ESP_LOGE(TAG, "invalid panel size (%d x %d). Did you init display first?", s_scr_w, s_scr_h);
        return false;
    }
    s_scr_px = (uint32_t)s_scr_w * (uint32_t)s_scr_h;
    ESP_LOGI(TAG, "panel hw=%dx%d (from driver), LVGL use the same", s_scr_w, s_scr_h);

    s_disp = lv_display_create(s_scr_w, s_scr_h);
    if (!s_disp) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return false;
    }

    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_0);
    lv_display_set_flush_cb(s_disp, disp_flush_cb);

    /* 4) 分配显示缓冲 */
    const uint32_t bpp = (uint32_t)lv_color_format_get_size(LV_COLOR_FORMAT_RGB565); // 应为2
    int buf_lines_used = 0;
    void *buf1 = NULL, *buf2 = NULL;
    bool use_double = false;

    if (!alloc_display_buffers(bpp, &buf_lines_used, &buf1, &buf2, &use_double)) {
        ESP_LOGE(TAG, "malloc lvgl buffers failed (RGB565, %ux%u*lines)", (unsigned)s_scr_w, (unsigned)bpp);
        return false;
    }

    const size_t buf_bytes = (size_t)s_scr_w * (size_t)buf_lines_used * (size_t)bpp;
    lv_display_set_buffers(s_disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(s_disp);
    ESP_LOGI(TAG, "LVGL buffers: %zu bytes %s%s",
             buf_bytes,
             esp_ptr_external_ram(buf1) ? "PSRAM" : "internal",
             (buf2 ? (esp_ptr_external_ram(buf2) ? "+PSRAM" : "+internal") : ""));

    /* 5) 若需要字节序对调，分配 DMA 行缓冲；否则直接用 LVGL 缓冲的行数做切片大小 */
#if LVGL_NEED_BYTESWAP
    for (int try_lines = FLUSH_SLICE_LINES; try_lines >= 1; try_lines = (try_lines > 32 ? try_lines - 8 : try_lines - 4)) {
        s_linecap_px = s_scr_w * try_lines;
        s_linebuf = (uint16_t *)heap_caps_malloc((size_t)s_linecap_px * sizeof(uint16_t),
                                                 MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (s_linebuf) { s_slice_lines = try_lines; break; }
    }
    if (!s_linebuf) {
        ESP_LOGE(TAG, "malloc line buffer failed");
        return false;
    }
#else
    s_slice_lines = buf_lines_used;
#endif

    /* 6) 触摸输入设备 */
    s_indev = lv_indev_create();
    if (!s_indev) {
        ESP_LOGE(TAG, "lv_indev_create failed");
        return false;
    }
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);

    /* 6.1) 刷新/触摸周期 */
#if LVGL_VERSION_MAJOR >= 9
    lv_timer_t *refr = lv_disp_get_refr_timer(s_disp);
    if (refr) lv_timer_set_period(refr, 18);    // 微调：18ms ≈ 55fps
    lv_timer_t *read_tmr = lv_indev_get_read_timer(s_indev);
    if (read_tmr) lv_timer_set_period(read_tmr, 18);
#endif

    /* 7) 右下角叠层（FPS/CPU） */
#if LVGL_PORT_CPU_OVERLAY
    s_cpu_label = lv_label_create(lv_layer_top());
    lv_label_set_text(s_cpu_label, "C0 --%  C1 --%");
    lv_obj_set_style_text_color(s_cpu_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color  (s_cpu_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa    (s_cpu_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all   (s_cpu_label, 3, 0);
    lv_obj_set_style_radius    (s_cpu_label, 4, 0);
    lv_obj_align(s_cpu_label, LV_ALIGN_BOTTOM_RIGHT, -3, -22);

    /* 初始化 CPU 统计的基线（若失败，会在定时器里重试） */
    (void)cpu_stats_try_init_baseline();

    s_cpu_timer = lv_timer_create(cpu_timer_cb, 500, NULL);
#endif

#if LVGL_PORT_FPS_OVERLAY
    s_fps_label = lv_label_create(lv_layer_top());
    lv_label_set_text(s_fps_label, "---- FPS");
    lv_obj_set_style_text_color(s_fps_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color  (s_fps_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa    (s_fps_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all   (s_fps_label, 3, 0);
    lv_obj_set_style_radius    (s_fps_label, 4, 0);
    lv_obj_align(s_fps_label, LV_ALIGN_BOTTOM_RIGHT, -3, -3);

    s_fps_timer = lv_timer_create(fps_timer_cb, 500, NULL);
#endif

    /* 8) 启动 LVGL 后台任务 */
    const UBaseType_t prio  = 10;
    const uint32_t    stack = 16 * 1024;
    const BaseType_t  core  = 0;   // 更稳：将 LVGL 任务放到 Core0
    BaseType_t ok = xTaskCreatePinnedToCore(lvgl_task, "lvgl", stack, NULL, prio, &s_lvgl_task, core);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "create lvgl task failed");
        return false;
    }

    s_inited = true;
    ESP_LOGI(TAG, "LVGL task created: stack=%" PRIu32 ", prio=%" PRIu32 ", core=%ld",
             (uint32_t)stack, (uint32_t)prio, (long)core);

    ESP_LOGI(TAG, "LVGL ready: %dx%d, buf_lines=%d, %s-buffer, slice=%d, bpp=%" PRIu32,
             s_scr_w, s_scr_h, buf_lines_used, use_double ? "double" : "single", s_slice_lines, bpp);

    /* 9) 如启用，周期性打印性能统计到 log monitor */
#if LVGL_PORT_STATS_LOG
    s_stats_timer = lv_timer_create(stats_log_timer_cb, 1000, NULL);
#endif

    return true;
}

/* ---------- 句柄导出 ---------- */
lv_indev_t* lvgl_port_get_indev(void)
{
    return s_indev;
}

/* 如需导出显示句柄可启用
lv_display_t* lvgl_port_get_display(void)
{
    return s_disp;
}
*/
