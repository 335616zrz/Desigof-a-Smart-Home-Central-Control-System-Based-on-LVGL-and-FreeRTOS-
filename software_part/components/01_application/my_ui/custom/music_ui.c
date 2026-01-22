// components/01_application/my_ui/custom/music_ui.c

#include "music_ui.h"
#include "custom.h"
#include "gui_guider.h"
#include "sdkconfig.h"
#include "server_root_cert.h"
#include "sdcard_spi.h"
#include "music_screen_common.h"
#include "board_config.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "cJSON.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "startup_ready.h"
#include "ota_update.h"
#include "audio_thread.h"

/* ====== mbedTLS 内存策略 ======
 * 原先这里通过 mbedtls_platform_set_calloc_free() 把 mbedTLS 的动态内存
 * 重定向到 heap_caps_*（优先 PSRAM），在大规模 TLS 并发时容易与底层
 * AES DMA/TLSF 分配器交互产生复杂问题（可疑堆破坏）。
 *
 * 为提高整体稳定性，这里暂时回退为使用 ESP-IDF 默认的 mbedTLS 分配器，
 * 如后续确有需要再按官方文档重新评估自定义分配策略。
 * ============================================== */

#ifndef MUSIC_INDEX_URL
#define MUSIC_INDEX_URL                                                        \
  CONFIG_APP_MUSIC_INDEX_URL /* 默认读取 menuconfig 配置 */
#endif

#if CONFIG_APP_MUSIC_SKIP_CN_CHECK
#define MUSIC_SKIP_CN_CHECK_DEV_MODE 1
#else
#define MUSIC_SKIP_CN_CHECK_DEV_MODE 0
#endif

#ifndef BUILD_BATCH
#define BUILD_BATCH 4 /* 更稳：每批 4 条，降低瞬时刷新压力 */
#endif

/* 单次在 LVGL 列表中实际创建的最大条目数。
 * 例如 150 表示：即便 SD 卡有 291 首歌，列表中也只保留前 150 条，
 * 减少 LVGL heap 与动画/滚动压力。*/
#ifndef MUSIC_LIST_MAX_VISIBLE_ITEMS
#define MUSIC_LIST_MAX_VISIBLE_ITEMS 200
#endif

/* LVGL 列表构建时的“保底剩余堆”阈值（单位 KB，可根据实际内存情况调整）。
 * 默认保留 2KB 总空闲、1KB 最大连续块，尽量避免堆被完全耗尽。
 * 如需关闭提前终止行为，可在编译时将这两个宏置 0。 */
#ifndef MUSIC_LIST_GUARD_FREE_KB
#define MUSIC_LIST_GUARD_FREE_KB 20
#endif
#ifndef MUSIC_LIST_GUARD_BIGGEST_KB
#define MUSIC_LIST_GUARD_BIGGEST_KB 6
#endif

/* —— Label 循环滚动模式的稳妥选择（适配不同版本命名） —— */
#if defined(LV_LABEL_LONG_SCROLL_CIRCULAR)
#define LONG_SCROLL_CIRC LV_LABEL_LONG_SCROLL_CIRCULAR
#elif defined(LV_LABEL_LONG_SCROLL_CIRC)
#define LONG_SCROLL_CIRC LV_LABEL_LONG_SCROLL_CIRC
#else
#define LONG_SCROLL_CIRC LV_LABEL_LONG_SCROLL /* 保底退化为非循环滚动 */
#endif

static const char *TAG = "music_ui";

/* ----------------- 业务数据 ----------------- */
typedef struct {
  char *title;
  char *url;
  char *duration;
  uint32_t duration_ms;
  bool liked;
} track_t;

typedef enum {
  MUSIC_SOURCE_HTTP = 0,
  MUSIC_SOURCE_SDCARD = 1,
} music_source_t;

/* 全局状态 */
static lv_ui *s_ui = NULL;
static lv_obj_t *s_list = NULL;
static track_t *s_tracks = NULL;
static int s_track_cnt = 0;
static bool s_fetching = false;
static bool s_fetched = false;
static TaskHandle_t s_wait_ota_task = NULL;
static uint32_t s_list_epoch = 1u; /* 每次列表失效或替换时递增 */
static int s_current_index = -1;
static int s_prev_index = -1;  /* 上一次高亮的条目，用于去除选中态 */
static int64_t s_last_click_us = 0;
static portMUX_TYPE s_click_mux = portMUX_INITIALIZER_UNLOCKED;
static lv_timer_t *s_visible_timer = NULL;
static uint32_t s_visible_timer_epoch = 0;
static QueueHandle_t s_evt_queue = NULL;
static lv_timer_t *s_evt_timer = NULL;
static int64_t s_last_auto_step_us = 0;
/* 当前使用的数据源：false=HTTP 索引，true=SD 卡文件列表 */
static bool s_use_sdcard = false;

#if CONFIG_FREERTOS_UNICORE
#define MUSIC_CLICK_ENTER_CRITICAL() portENTER_CRITICAL(&s_click_mux)
#define MUSIC_CLICK_EXIT_CRITICAL() portEXIT_CRITICAL(&s_click_mux)
#else
#define MUSIC_CLICK_ENTER_CRITICAL() portENTER_CRITICAL(&s_click_mux)
#define MUSIC_CLICK_EXIT_CRITICAL() portEXIT_CRITICAL(&s_click_mux)
#endif

static inline int64_t last_click_get(void) {
  MUSIC_CLICK_ENTER_CRITICAL();
  int64_t val = s_last_click_us;
  MUSIC_CLICK_EXIT_CRITICAL();
  return val;
}

static inline void last_click_set(int64_t val) {
  MUSIC_CLICK_ENTER_CRITICAL();
  s_last_click_us = val;
  MUSIC_CLICK_EXIT_CRITICAL();
}

#define MUSIC_CLICK_DEBOUNCE_US 350000LL

typedef struct {
  lv_obj_t *list;
  track_t *tracks;
  size_t total;
  size_t next;
  uint32_t epoch;
} build_list_state_t;

/* SD 卡目录构建上下文：用于从 sdcard_music_enum 回调中累积 track_t */
typedef struct {
  track_t *arr;
  int capacity;
  int count;
} sd_build_ctx_t;

/* SD 卡曲目：按标题（文件名）做升序排序。
 * 由于文件名以 000/001/... 数字前缀零填充，strcmp 即可保证数值顺序一致。*/
static int sd_track_title_cmp(const void *pa, const void *pb)
{
  const track_t *a = (const track_t *)pa;
  const track_t *b = (const track_t *)pb;
  const char *ta = a->title ? a->title : "";
  const char *tb = b->title ? b->title : "";
  int r = strcmp(ta, tb);
  if (r < 0) return -1;
  if (r > 0) return 1;
  return 0;
}

static lv_timer_t *s_build_timer = NULL;
static build_list_state_t *s_build_state = NULL;

static music_ui_play_cb_t s_play_cb = NULL;

typedef enum { MUSIC_UI_EVT_STATE, MUSIC_UI_EVT_FINISHED } music_ui_evt_type_t;

typedef struct {
  music_ui_evt_type_t type;
  union {
    ap_state_t state;
    bool forward;
  } data;
} music_ui_evt_t;

/* init 幂等 & 事件实例句柄，避免重复注册 */
static bool s_inited = false;
static esp_event_handler_instance_t s_got_ip_ins = NULL;
static esp_event_handler_instance_t s_wifi_disc_ins = NULL;

/* 拉取失败的重试计数（在同一个 fetch_task 中用 vTaskDelay 实现重试，
 * 不再依赖 FreeRTOS 软件定时器，避免对 Tmr Svc 任务栈和堆的额外压力）。*/
static int s_fetch_fail_count = 0;

#define MUSIC_FETCH_RETRY_MAX 3
#define MUSIC_FETCH_RETRY_DELAY_MS 3000

/* ---------------- 内存工具 ---------------- */

static lv_style_t s_list_label_style;
static bool s_list_label_style_init = false;

static void ensure_list_label_style(void) {
  if (s_list_label_style_init)
    return;
  lv_style_init(&s_list_label_style);
  lv_style_set_text_font(&s_list_label_style, &lv_font_montserratMedium_24);
  lv_style_set_text_color(&s_list_label_style, lv_color_hex(0x0D3055));
  lv_style_set_text_letter_space(&s_list_label_style, 0);
  lv_style_set_text_line_space(&s_list_label_style, 4);
  s_list_label_style_init = true;
}

static void free_catalog(void) {
  if (!s_tracks)
    return;
  for (int i = 0; i < s_track_cnt; ++i) {
    free(s_tracks[i].title);
    free(s_tracks[i].url);
    free(s_tracks[i].duration);
  }
  free(s_tracks);
  s_tracks = NULL;
  s_track_cnt = 0;
  s_fetched = false;
}

static void free_tracks_array(track_t *arr, int cnt) {
  if (!arr)
    return;
  for (int i = 0; i < cnt; ++i) {
    free(arr[i].title);
    free(arr[i].url);
    free(arr[i].duration);
  }
  free(arr);
}

static char *dup_str(const char *s) {
  if (!s)
    return NULL;
  size_t n = strlen(s) + 1;
  char *p = (char *)malloc(n);
  if (p)
    memcpy(p, s, n);
  return p;
}

static uint32_t duration_to_ms(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned int mm = 0;
  unsigned int ss = 0;
  if (sscanf(s, "%u:%u", &mm, &ss) != 2)
    return 0;
  uint64_t total = ((uint64_t)mm * 60ULL + (uint64_t)ss) * 1000ULL;
  if (total > UINT32_MAX) {
    total = UINT32_MAX;
  }
  return (uint32_t)total;
}

static inline void list_epoch_bump(void) {
  if (++s_list_epoch == 0)
    ++s_list_epoch;
}

static void stop_builder(void) {
  if (s_build_timer) {
    lv_timer_del(s_build_timer);
    s_build_timer = NULL;
  }
  if (s_build_state) {
    free(s_build_state);
    s_build_state = NULL;
  }
}

static void stop_visible_update_timer(void) {
  if (s_visible_timer) {
    lv_timer_del(s_visible_timer);
    s_visible_timer = NULL;
    s_visible_timer_epoch = 0;
  }
}

static lv_result_t lv_async_call_locked(lv_async_cb_t cb, void *user_data,
                                       uint32_t lock_timeout_ms) {
  if (!cb) {
    return LV_RESULT_INVALID;
  }

  if (!lvgl_port_lock(lock_timeout_ms)) {
    return LV_RESULT_INVALID;
  }

  lv_result_t res = lv_async_call(cb, user_data);
  lvgl_port_unlock();
  return res;
}

static bool enqueue_music_evt(const music_ui_evt_t *evt);
static void music_evt_timer_cb(lv_timer_t *timer);
static void ensure_evt_dispatcher(void);
static void list_update_selection(void);
static bool sd_enum_cb(const sdcard_music_item_t *item, void *user);

/* ----------------- SD 卡条目时长估算（用于进度条初始值） ----------------- */
/* 尝试从 FLAC 头部的 STREAMINFO 中读取精确时长：
 * - 仅读取文件前几十字节，不解析完整流，开销较小；
 * - 若解析失败则回退到基于经验比特率的估算。*/
static uint32_t flac_duration_from_header(const char *path)
{
  if (!path || !path[0]) {
    return 0;
  }

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return 0;
  }

  uint8_t buf[64];
  size_t  n = fread(buf, 1, sizeof(buf), fp);
  fclose(fp);
  if (n < 42) {
    return 0;
  }

  /* 1) 魔数 "fLaC" */
  if (buf[0] != 'f' || buf[1] != 'L' || buf[2] != 'a' || buf[3] != 'C') {
    return 0;
  }

  /* 2) 第一个 metadata block 应为 STREAMINFO */
  const uint8_t *p = buf + 4;
  uint8_t block_type = (uint8_t)(p[0] & 0x7F);
  uint32_t length = ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
  if (block_type != 0 || length < 34u || (size_t)(4u + length) > n) {
    return 0;
  }
  const uint8_t *si = p + 4; /* STREAMINFO data 起始 */

  /* STREAMINFO[10..17] 共 8 字节：
   *  [0..19]  sample_rate (20 bits)
   *  [20..22] channels-1  (3 bits)
   *  [23..27] bits-1      (5 bits)
   *  [28..63] total_samples (36 bits)
   * 这里我们只关心 sample_rate 与 total_samples。*/
  uint64_t x =
      ((uint64_t)si[10] << 56) |
      ((uint64_t)si[11] << 48) |
      ((uint64_t)si[12] << 40) |
      ((uint64_t)si[13] << 32) |
      ((uint64_t)si[14] << 24) |
      ((uint64_t)si[15] << 16) |
      ((uint64_t)si[16] << 8)  |
      ((uint64_t)si[17]);

  uint32_t sample_rate = (uint32_t)((x >> 44) & 0xFFFFFULL);   /* 20 bits */
  uint64_t total_samples = (x & 0xFFFFFFFFFULL);               /* 36 bits */

  if (sample_rate == 0 || total_samples == 0) {
    return 0;
  }

  uint64_t ms = (total_samples * 1000ULL) / (uint64_t)sample_rate;
  return (ms > UINT32_MAX) ? UINT32_MAX : (uint32_t)ms;
}

static uint32_t sd_estimate_duration_ms(const sdcard_music_item_t *item)
{
  if (!item) {
    return 0;
  }

  /* WAV：按板卡默认采样参数估算：size / (bytes_per_sample * ch) / sr */
  if (item->fmt == SDCARD_MUSIC_FMT_WAV) {
    const uint32_t sr   = BOARD_AUDIO_SAMPLE_RATE_DEFAULT;
    const uint32_t bits = BOARD_AUDIO_BITS_PER_SAMPLE;
    const uint32_t ch   = BOARD_AUDIO_NUM_CHANNELS;
    if (sr == 0 || bits == 0 || ch == 0) {
      return 0;
    }
    uint32_t bytes_per_frame = (bits / 8U) * ch;
    if (bytes_per_frame == 0) {
      return 0;
    }
    uint64_t data_bytes = item->size_bytes;
    /* 粗略跳过 WAV 头部几十字节，误差可以接受 */
    if (data_bytes > 64U) {
      data_bytes -= 64U;
    }
    uint64_t total_frames = data_bytes / bytes_per_frame;
    if (total_frames == 0) {
      return 0;
    }
    uint64_t ms = (total_frames * 1000ULL) / sr;
    return (ms > UINT32_MAX) ? UINT32_MAX : (uint32_t)ms;
  }

  /* MP3：大多数曲目为 128kbps，这里按 128kbps 估算。 */
  if (item->fmt == SDCARD_MUSIC_FMT_MP3) {
    const uint32_t kbps = 128U;
    if (kbps == 0) {
      return 0;
    }
    uint64_t bits = (uint64_t)item->size_bytes * 8ULL;
    /* duration_ms ≈ bits / (kbps*1000) * 1000 = bits / kbps */
    uint64_t ms = bits / kbps;
    return (ms > UINT32_MAX) ? UINT32_MAX : (uint32_t)ms;
  }

  /* FLAC：优先从头部 STREAMINFO 读取精确时长，失败再按经验比特率估算。*/
  if (item->fmt == SDCARD_MUSIC_FMT_FLAC) {
    uint32_t ms = flac_duration_from_header(item->path);
    if (ms > 0) {
      return ms;
    }
    /* 回退：使用 ~1000kbps 估算，兼容大多数 44.1k/16bit 曲目。*/
    const uint32_t kbps = 1000U;
    uint64_t bits = (uint64_t)item->size_bytes * 8ULL;
    uint64_t ms_fallback = bits / kbps;
    return (ms_fallback > UINT32_MAX) ? UINT32_MAX : (uint32_t)ms_fallback;
  }

  /* 其它格式当前不做估算；由底层播放器在解析后推算。*/
  return 0;
}

/* 安全把毫秒转成 "MM:SS"（out 至少 6 字节） */
static void ms_to_mmss_local(uint32_t ms, char out[6])
{
  uint32_t s = ms / 1000U;
  uint32_t m = s / 60U;
  s %= 60U;
  if (m > 99U) { m = 99U; s = 59U; }
  (void)snprintf(out, 6, "%02u:%02u", (unsigned)m, (unsigned)s);
}

static bool enqueue_music_evt(const music_ui_evt_t *evt) {
  if (!evt)
    return false;
  if (!s_evt_queue)
    return false;

  if (xQueueSend(s_evt_queue, evt, 0) == pdTRUE)
    return true;

  music_ui_evt_t drop;
  if (xQueueReceive(s_evt_queue, &drop, 0) == pdTRUE) {
    return xQueueSend(s_evt_queue, evt, 0) == pdTRUE;
  }
  return false;
}

static ap_state_t s_coalesced_state = AP_STATE_IDLE;
static bool       s_has_coalesced   = false;

static void music_evt_timer_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (!s_evt_queue)
    return;

  music_ui_evt_t evt;
  while (xQueueReceive(s_evt_queue, &evt, 0) == pdTRUE) {
    switch (evt.type) {
    case MUSIC_UI_EVT_STATE:
      /* 不立即调用 UI，合并同一周期内多次状态，仅提交“最后一个”。
       * 这样可以避免 LOADING 与 PLAYING 近距到达时发生倒序执行导致
       * 按钮先变成Ⅱ又被拉回▶的现象。*/
      s_coalesced_state = evt.data.state;
      s_has_coalesced = true;
      /* 仅在 FINISHED 里触发自动下一首。STOPPED 不再自动跳曲，避免加载/切歌过程中的
         临时停止导致“UI 自己快速切换”。*/
      if (evt.data.state == AP_STATE_PLAYING) {
        s_last_auto_step_us = 0;
      }
      break;
    case MUSIC_UI_EVT_FINISHED: {
      int64_t now = esp_timer_get_time();
      /* 若刚在很短时间内已经触发过自动切歌（例如播放器 FINISHED 与进度兜底同时到达），
       * 则丢弃重复请求，避免“跨两首”。*/
      if (s_last_auto_step_us != 0 && (now - s_last_auto_step_us) < 600000LL) {
        ESP_LOGI(TAG, "auto-step: suppress duplicate (diff=%lldus)", (long long)(now - s_last_auto_step_us));
        break;
      }
      if (MusicScreen_IsSpectrumVisible()) {
        MusicScreen_CloseSpectrum();
      }
      MusicScreen_RequestStep(evt.data.forward);
      s_last_auto_step_us = now;
      break;
    }
    default:
      break;
    }
  }

  if (s_has_coalesced) {
    MusicScreen_HandlePlaybackState(s_coalesced_state);
    s_has_coalesced = false;
  }
}

static void ensure_evt_dispatcher(void) {
  if (!s_evt_queue) {
    s_evt_queue = xQueueCreate(32, sizeof(music_ui_evt_t));
    if (!s_evt_queue) {
      ESP_LOGE(TAG, "create evt queue failed");
    }
  }

  if (!s_evt_timer && s_evt_queue) {
    if (lvgl_port_lock(10)) {
      s_evt_timer = lv_timer_create(music_evt_timer_cb, 30, NULL);
      if (!s_evt_timer) {
        ESP_LOGE(TAG, "create evt timer failed");
      } else {
        lv_timer_set_repeat_count(s_evt_timer, -1);
      }
      lvgl_port_unlock();
    } else {
      ESP_LOGE(TAG, "evt timer lock failed");
    }
  }
}

static void list_visible_timer_cb(lv_timer_t *timer);
static void schedule_visible_update(lv_obj_t *list);

/* ---------------- HTTP 拉取 ---------------- */
typedef struct {
  char *data;
  size_t len;
  size_t cap;
} dynbuf_t;

static bool dbuf_init(dynbuf_t *b, size_t cap) {
  b->data = (char *)malloc(cap ? cap : 1);
  if (!b->data)
    return false;
  b->len = 0;
  b->cap = cap ? cap : 1;
  return true;
}

static bool dbuf_ensure(dynbuf_t *b, size_t add) {
  if (b->len + add <= b->cap)
    return true;
  size_t ncap = b->cap ? b->cap : 1;
  while (ncap < b->len + add)
    ncap <<= 1;
  char *p = (char *)realloc(b->data, ncap);
  if (!p)
    return false;
  b->data = p;
  b->cap = ncap;
  return true;
}

static void dbuf_free(dynbuf_t *b) {
  free(b->data);
  b->data = NULL;
  b->len = b->cap = 0;
}

static bool http_get_all(const char *url, dynbuf_t *out) {
  if (!url || !url[0]) {
    ESP_LOGW(TAG, "http_get_all: empty url");
    return false;
  }
  if (strncasecmp(url, "http://", 7) && strncasecmp(url, "https://", 8)) {
    ESP_LOGE(TAG, "invalid scheme: %s", url);
    return false;
  }

  bool ok = false;
  esp_http_client_handle_t c = NULL;

  /* 串行化 TLS：与 Chatbot 等共享一把互斥锁，避免同时握手抢占 AES/DMA/堆。 */
  AppTLS_Lock();

  esp_http_client_config_t cfg = {
    .url = url,
    .timeout_ms = 8000,
    .buffer_size = 2048,
    .keep_alive_enable = true,
    .cert_pem = server_root_cert_pem,
#if CONFIG_APP_MUSIC_SKIP_CN_CHECK
    .skip_cert_common_name_check = true,
#endif
  };

  c = esp_http_client_init(&cfg);
  if (!c) {
    ESP_LOGE(TAG, "esp_http_client_init failed");
    goto out;
  }
  /* 一些服务器会默认返回 gzip 压缩，导致 JSON 解析失败，这里强制明示不接收压缩 */
  esp_http_client_set_header(c, "Accept-Encoding", "identity");
  esp_http_client_set_header(c, "Accept", "*/*");

  esp_err_t e = esp_http_client_open(c, 0);
  if (e != ESP_OK) {
    ESP_LOGE(TAG, "open failed: %s", esp_err_to_name(e));
    goto out;
  }

  int hdr = esp_http_client_fetch_headers(c);
  if (hdr < 0) {
    ESP_LOGE(TAG, "fetch_headers failed");
    goto out;
  }
  int code = esp_http_client_get_status_code(c);
  if (code != 200) {
    ESP_LOGE(TAG, "HTTP %d", code);
    goto out;
  }

  char tmp[2048];
  while (1) {
    int r = esp_http_client_read(c, tmp, sizeof(tmp));
    if (r < 0) {
      ESP_LOGE(TAG, "read error");
      goto out;
    }
    if (r == 0)
      break;
    if (!dbuf_ensure(out, (size_t)r + 1)) {
      ESP_LOGE(TAG, "OOM");
      goto out;
    }
    memcpy(out->data + out->len, tmp, (size_t)r);
    out->len += (size_t)r;
    out->data[out->len] = '\0';
  }

  ok = true;

out:
  if (c) {
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
  }
  AppTLS_Unlock();
  return ok;
}

/* ---------------- JSON 解析 ---------------- */
static bool parse_index_json(const char *json, size_t len, track_t **out_arr,
                             int *out_cnt) {
  *out_arr = NULL;
  *out_cnt = 0;
  cJSON *root = cJSON_ParseWithLength(json, len);
  if (!root || !cJSON_IsArray(root)) {
    cJSON_Delete(root);
    return false;
  }

  int n = cJSON_GetArraySize(root);
  if (n <= 0) {
    cJSON_Delete(root);
    return true;
  }

  track_t *arr = (track_t *)calloc((size_t)n, sizeof(track_t));
  if (!arr) {
    cJSON_Delete(root);
    return false;
  }

  int real = 0;
  for (int i = 0; i < n; ++i) {
    cJSON *obj = cJSON_GetArrayItem(root, i);
    if (!cJSON_IsObject(obj))
      continue;
    const cJSON *jt = cJSON_GetObjectItemCaseSensitive(obj, "title");
    const cJSON *ju = cJSON_GetObjectItemCaseSensitive(obj, "url");
    const cJSON *jd = cJSON_GetObjectItemCaseSensitive(obj, "duration");
    if (!cJSON_IsString(jt) || !cJSON_IsString(ju))
      continue;

    char *t = dup_str(jt->valuestring);
    char *u = dup_str(ju->valuestring);
    char *d = NULL;
    uint32_t d_ms = 0;
    if (cJSON_IsString(jd) && jd->valuestring) {
      d = dup_str(jd->valuestring);
      d_ms = duration_to_ms(jd->valuestring);
    }

    if (!t || !u) {
      free(t);
      free(u);
      free(d);
      continue;
    }

    arr[real].title = t;
    arr[real].url = u;
    arr[real].duration = d;
    arr[real].duration_ms = d_ms;
    arr[real].liked = false;
    real++;
  }
  cJSON_Delete(root);

  if (real == 0) {
    free(arr);
    return false;
  }
  *out_arr = arr;
  *out_cnt = real;
  return true;
}

/* ----------------- 工具：对象类型与有效性守卫 ----------------- */
static inline bool list_ok(const lv_obj_t *obj) {
  return obj && lv_obj_is_valid(obj) && lv_obj_has_class(obj, &lv_list_class);
}

static void focus_button_center(lv_obj_t *list, lv_obj_t *btn) {
  if (!list_ok(list) || !btn)
    return;
  /* 更稳妥：让目标项可见（LVGL 内部处理坐标/滚动容器差异）。
   * 若必须居中，可在后续做微调。但先保证“能看到正确项”。 */
  lv_obj_scroll_to_view(btn, LV_ANIM_ON);
}

static lv_obj_t *get_list_button_by_index(int idx) {
  if (!list_ok(s_list) || idx < 0) {
    return NULL;
  }
  uint32_t cnt = lv_obj_get_child_cnt(s_list);
  if ((uint32_t)idx >= cnt) {
    return NULL;
  }
  return lv_obj_get_child(s_list, (uint32_t)idx);
}

static void play_track_by_index(int idx, lv_obj_t *btn, bool focus_btn) {
  if (idx < 0 || idx >= s_track_cnt) {
    return;
  }

  track_t *track = &s_tracks[idx];
  const char *title = track->title ? track->title : "";
  const char *url = track->url ? track->url : "(null)";

  ESP_LOGI(TAG, "play idx=%d '%s' -> %s", idx, title, url);

  s_current_index = idx;

  if (!btn) {
    btn = get_list_button_by_index(idx);
  }
  if (focus_btn && btn && s_list) {
    focus_button_center(s_list, btn);
  }

  /* 同步列表高亮状态，避免用户感知错位 */
  list_update_selection();

  if (s_play_cb) {
    music_ui_track_info_t info = {
        .index = idx,
        .title = track->title,
        .url = track->url,
        .duration = track->duration,
        .duration_ms = track->duration_ms,
    };
    s_play_cb(&info);
  }
}

/* ----------------- 列表可见性与动画控制 ----------------- */

static lv_obj_t *find_btn_label(lv_obj_t *btn) {
  uint32_t cnt = lv_obj_get_child_cnt(btn);
  for (uint32_t i = 0; i < cnt; ++i) {
    lv_obj_t *ch = lv_obj_get_child(btn, i);
    if (lv_obj_has_class(ch, &lv_label_class))
      return ch;
  }
  return NULL;
}

/* v9 没有公开的 lv_area_intersect，手写一个相交判断即可 */
static bool rect_intersect(const lv_area_t *a, const lv_area_t *b) {
  if (!a || !b)
    return false;
  if (a->x2 < b->x1)
    return false;
  if (b->x2 < a->x1)
    return false;
  if (a->y2 < b->y1)
    return false;
  if (b->y2 < a->y1)
    return false;
  return true;
}

static void list_update_visible_animations(lv_obj_t *list) {
  if (!list_ok(list))
    return;

  lv_area_t area_list;
  lv_obj_get_coords(list, &area_list);

  uint32_t cnt = lv_obj_get_child_cnt(list);
  for (uint32_t i = 0; i < cnt; ++i) {
    if ((i & 0x0Fu) == 0u) {
      (void)esp_task_wdt_reset();
    }
    lv_obj_t *btn = lv_obj_get_child(list, i);
    if (!btn)
      continue;

    lv_area_t area_btn;
    lv_obj_get_coords(btn, &area_btn);

    lv_obj_t *label = find_btn_label(btn);
    if (!label)
      continue;

    bool visible = rect_intersect(&area_list, &area_btn);

    lv_label_long_mode_t cur = lv_label_get_long_mode(label);
    if (visible) {
      if (cur != LONG_SCROLL_CIRC)
        lv_label_set_long_mode(label, LONG_SCROLL_CIRC);
    } else {
      if (cur != LV_LABEL_LONG_CLIP)
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    }
  }
}

static void list_visible_timer_cb(lv_timer_t *timer) {
  lv_obj_t *list = (lv_obj_t *)lv_timer_get_user_data(timer);
  uint32_t expected_epoch = s_visible_timer_epoch;

  s_visible_timer = NULL;
  s_visible_timer_epoch = 0;

  if (list && list_ok(list) && expected_epoch == s_list_epoch) {
    list_update_visible_animations(list);
  }

  lv_timer_del(timer);
}

static void schedule_visible_update(lv_obj_t *list) {
  if (!list_ok(list)) {
    return;
  }

  if (s_visible_timer) {
    if (lv_timer_get_user_data(s_visible_timer) == list) {
      lv_timer_reset(s_visible_timer);
      s_visible_timer_epoch = s_list_epoch;
      return;
    }
    stop_visible_update_timer();
  }

  s_visible_timer = lv_timer_create(list_visible_timer_cb, 80, list);
  if (s_visible_timer) {
    lv_timer_set_repeat_count(s_visible_timer, 1);
    s_visible_timer_epoch = s_list_epoch;
  }
}

static void on_list_scrolled(lv_event_t *e) {
  lv_obj_t *list = lv_event_get_target(e);
  if (!list)
    return;
  schedule_visible_update(list);
}

static void on_list_deleted(lv_event_t *e);

static void on_item_clicked(lv_event_t *e) {
  int64_t now = esp_timer_get_time();
  int64_t last = last_click_get();
  if (last != 0 && (now - last) < MUSIC_CLICK_DEBOUNCE_US) {
    ESP_LOGI(TAG, "click ignored (debounce %lld us)", (long long)(now - last));
    return;
  }
  last_click_set(now);

  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  if (idx < 0 || idx >= s_track_cnt)
    return;

  lv_obj_t *btn = lv_event_get_target(e);
  play_track_by_index(idx, btn, true);
}

static lv_obj_t *create_list_item(lv_obj_t *list, const char *title, int idx) {
  if (!list_ok(list)) {
    ESP_LOGW(TAG, "create_item: list invalid (idx=%d)", idx);
    return NULL;
  }

  /* mute create_item noisy log */
  // ESP_LOGI(TAG, "create_item: list=%p idx=%d title=%s", (void *)list, idx,
  //          title ? title : "(null)");
  LV_ASSERT_OBJ(list, &lv_list_class);

  /* icon 传 NULL，避免创建 image child，占内存又影响性能 */
  const char *txt = title ? title : "(no title)";
  lv_obj_t *btn = lv_list_add_button(list, NULL, txt);
  if (!btn) {
    ESP_LOGE(TAG, "lv_list_add_button failed idx=%d", idx);
    return NULL;
  }

  /* 允许高亮显示 */
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

  /* 点击回调：v9 推荐的 event user_data 传参 */
  lv_obj_add_event_cb(btn, on_item_clicked, LV_EVENT_CLICKED,
                      (void *)(intptr_t)idx);

  /* 初始 label 为 CLIP，避免立即创建滚动动画 */
  lv_obj_t *label = find_btn_label(btn);
  if (label) {
    ensure_list_label_style();
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_add_style(label, &s_list_label_style, LV_PART_MAIN);
  }

  return btn;
}

/* 高亮当前播放的列表项 */
static void list_update_selection(void) {
  if (!list_ok(s_list)) {
    return;
  }
  /* 清除旧选中态 */
  if (s_prev_index >= 0) {
    lv_obj_t *old_btn = get_list_button_by_index(s_prev_index);
    if (old_btn) {
      lv_obj_clear_state(old_btn, LV_STATE_CHECKED);
    }
  }
  /* 设置新选中态 */
  if (s_current_index >= 0) {
    lv_obj_t *cur_btn = get_list_button_by_index(s_current_index);
    if (cur_btn) {
      lv_obj_add_state(cur_btn, LV_STATE_CHECKED);
      s_prev_index = s_current_index;
    } else {
      s_prev_index = -1;
    }
  } else {
    s_prev_index = -1;
  }
}

static void build_list_step(lv_timer_t *t) {
  (void)esp_task_wdt_reset();
  build_list_state_t *st = (build_list_state_t *)lv_timer_get_user_data(t);
  if (!st || st != s_build_state) {
    stop_builder();
    return;
  }

  if (st->epoch != s_list_epoch || st->list != s_list || !st->tracks ||
      !list_ok(st->list)) {
    ESP_LOGW(TAG,
             "build_step abort: epoch=%" PRIu32 "/%" PRIu32
             " list=%p/%p tracks=%p",
             st->epoch, s_list_epoch, (void *)st->list, (void *)s_list,
             (void *)st->tracks);
    stop_builder();
    return;
  }

  if (lv_mem_test() != LV_RESULT_OK) {
    ESP_LOGE(TAG, "build_step: lv_mem_test failed, stop builder");
    stop_builder();
    return;
  }

  lv_mem_monitor_t mem;
  lv_mem_monitor(&mem);
  /* 预留一部分 LVGL heap 给后续动画/事件使用，避免把堆压空。
   * 当阈值为 0 时，对应检查关闭，仅依赖 lv_list_add_button 失败来终止构建。*/
  const size_t guard_free =
      (size_t)MUSIC_LIST_GUARD_FREE_KB * 1024U;
  const size_t guard_biggest =
      (size_t)MUSIC_LIST_GUARD_BIGGEST_KB * 1024U;
  if ((guard_free > 0 && mem.free_size < guard_free) ||
      (guard_biggest > 0 && mem.free_biggest_size < guard_biggest)) {
    ESP_LOGW(TAG,
             "build_step: LVGL heap low (free=%zu, biggest=%zu), stop at %u/%u",
             (size_t)mem.free_size, (size_t)mem.free_biggest_size,
             (unsigned)st->next, (unsigned)st->total);
    stop_builder();
    return;
  }

  int added = 0;
  while (st->next < st->total && added < BUILD_BATCH) {
    (void)esp_task_wdt_reset();
    track_t *tr = &st->tracks[st->next];
    ESP_LOGD(TAG, "build item idx=%u title=%s", (unsigned)st->next,
             tr->title ? tr->title : "(null)");
    if (!create_list_item(st->list, tr->title, (int)st->next)) {
      ESP_LOGE(TAG, "build_step: create_list_item failed idx=%u",
               (unsigned)st->next);
      stop_builder();
      return;
    }
    st->next++;
    added++;
  }

  if (st->next >= st->total) {
    list_update_visible_animations(st->list);
    if (s_current_index >= 0 && s_current_index < (int)st->total &&
        list_ok(st->list)) {
      lv_obj_t *btn = lv_obj_get_child(st->list, (uint32_t)s_current_index);
      if (btn) {
        focus_button_center(st->list, btn);
        list_update_selection();
      }
    }
    stop_builder();
  }
}

static void start_build_list(lv_obj_t *list, track_t *tracks, size_t total) {
  if (!list_ok(list) || !tracks || total == 0)
    return;

  /* 限制单屏最大可见条目数，避免一次性创建过多 LVGL 子对象导致内存紧张。
   * 超出上限的曲目仍保留在 s_tracks（供自动下一曲等逻辑使用），
   * 但不会在列表中全部展开。*/
  size_t capped_total = total;
  if (capped_total > (size_t)MUSIC_LIST_MAX_VISIBLE_ITEMS) {
    capped_total = (size_t)MUSIC_LIST_MAX_VISIBLE_ITEMS;
  }

  if (!lvgl_port_lock(50)) {
    ESP_LOGE(TAG, "start_build_list: lock failed");
    return;
  }

  stop_builder();
  stop_visible_update_timer();

  /* mute list building noisy log */
  // ESP_LOGI(TAG, "start_build_list: total=%u capped=%u",
  //          (unsigned)total, (unsigned)capped_total);

  lv_obj_clean(list);
  lv_obj_remove_event_cb(list, on_list_scrolled);
  lv_obj_remove_event_cb(list, on_list_deleted);
  lv_obj_add_event_cb(list, on_list_scrolled, LV_EVENT_SCROLL, NULL);
  lv_obj_add_event_cb(list, on_list_deleted, LV_EVENT_DELETE, NULL);

  s_build_state = (build_list_state_t *)calloc(1, sizeof(*s_build_state));
  if (!s_build_state) {
    lvgl_port_unlock();
    return;
  }

  s_build_state->list = list;
  s_build_state->tracks = tracks;
  s_build_state->total = capped_total;
  s_build_state->next = 0;
  s_build_state->epoch = s_list_epoch;

  s_build_timer = lv_timer_create(build_list_step, 15, s_build_state);
  lvgl_port_unlock();
}

typedef struct {
  lv_obj_t *list;
  track_t *tracks;
  size_t total;
  uint32_t epoch;
} build_async_arg_t;

static void build_list_async_cb(void *p) {
  build_async_arg_t *a = (build_async_arg_t *)p;
  if (a) {
    if (a->epoch == s_list_epoch && a->list == s_list && list_ok(a->list) &&
        a->tracks && a->total) {
      start_build_list(a->list, a->tracks, a->total);
    }
    free(a);
  }
}

static void post_start_build_list(lv_obj_t *list, track_t *tracks,
                                  size_t total) {
  if (!tracks || total == 0 || !list_ok(list))
    return;
  if (list != s_list)
    return;

  build_async_arg_t *a = (build_async_arg_t *)malloc(sizeof(*a));
  if (!a)
    return;
  a->list = list;
  a->tracks = tracks;
  a->total = total;
  a->epoch = s_list_epoch;
  lv_result_t res = lv_async_call_locked(build_list_async_cb, a, 50);
  if (res != LV_RESULT_OK) {
    free(a);
  }
}

static void clear_list_async_cb(void *arg) {
  LV_UNUSED(arg);
  stop_builder();
  stop_visible_update_timer();
  list_epoch_bump();
  if (list_ok(s_list)) {
    lv_obj_clean(s_list);
  }
}

typedef struct {
  track_t *tracks;
  int count;
  music_source_t source;
} catalog_update_arg_t;

static void catalog_update_cb(void *arg) {
  catalog_update_arg_t *msg = (catalog_update_arg_t *)arg;
  if (!msg)
    return;

  track_t *tracks = msg->tracks;
  int count = msg->count;
  music_source_t src = msg->source;

  stop_builder();
  stop_visible_update_timer();
  list_epoch_bump();
  free_catalog();

  /* 若当前 UI 模式与本次更新来源不一致，则丢弃这批目录，避免
   * HTTP 拉取结果覆盖 SD 卡模式，或反之。*/
  if ((src == MUSIC_SOURCE_HTTP && s_use_sdcard) ||
      (src == MUSIC_SOURCE_SDCARD && !s_use_sdcard)) {
    ESP_LOGI(TAG, "catalog_update: drop obsolete catalog (src=%d, use_sdcard=%d)",
             (int)src, (int)s_use_sdcard);
    free_tracks_array(tracks, count);
    if (src == MUSIC_SOURCE_HTTP) {
      s_fetching = false;
    }
    free(msg);
    return;
  }

  if (!tracks || count <= 0) {
    free_tracks_array(tracks, count);
    s_tracks = NULL;
    s_track_cnt = 0;
    s_fetched = false;
    s_current_index = -1;
    if (list_ok(s_list)) {
      lv_obj_clean(s_list);
    }
  } else {
    ESP_LOGI(TAG, "catalog_update: total tracks=%d", count);
    s_tracks = tracks;
    s_track_cnt = count;
    s_fetched = true;
    /* 注：startup_ready_set 已在 "loaded tracks" 日志处调用，此处不再重复 */
    if (s_current_index >= s_track_cnt) {
      s_current_index = (s_track_cnt > 0) ? 0 : -1;
    }
    if (list_ok(s_list)) {
      post_start_build_list(s_list, s_tracks, (size_t)s_track_cnt);
    }
  }

  if (src == MUSIC_SOURCE_HTTP) {
    s_fetching = false;
  }
  free(msg);
}

/* 拉取成功后在 LVGL 线程创建列表（当前称为 “经典流程”） */
static void catalog_apply_tracks(track_t *tracks, int count, music_source_t source) {
  catalog_update_arg_t *msg = (catalog_update_arg_t *)calloc(1, sizeof(*msg));
  if (!msg) {
    ESP_LOGE(TAG, "catalog_apply: alloc failed");
    free_tracks_array(tracks, count);
    if (source == MUSIC_SOURCE_HTTP) {
      s_fetching = false;
    }
    return;
  }
  msg->tracks = tracks;
  msg->count = count;
  msg->source = source;
  lv_result_t res = lv_async_call_locked(catalog_update_cb, msg, 200);
  if (res != LV_RESULT_OK) {
    ESP_LOGE(TAG, "catalog_apply: lv_async_call failed %d", (int)res);
    free_tracks_array(tracks, count);
    free(msg);
    if (source == MUSIC_SOURCE_HTTP) {
      s_fetching = false;
    }
  }
}

/* ---------------- 拉取任务与入口 ---------------- */
static void fetch_task(void *arg) {
  (void)arg;

  /* 为了错峰启动，避免在 UI / Wi‑Fi / 音频管线初始化的最早阶段立即占用较多
   * 栈与 TLS 资源，这里在开机时对首次索引拉取增加一个小延迟。后续重试仍然
   * 在同一任务内通过 vTaskDelay 实现。*/
  vTaskDelay(pdMS_TO_TICKS(1500));

  if (!(MUSIC_INDEX_URL[0])) {
    ESP_LOGW(TAG, "MUSIC_INDEX_URL is empty, skip fetching");
    s_fetching = false;
    vTaskDelete(NULL);
    return;
  }

  /* 在同一任务内做有限次重试，失败之间用 vTaskDelay 退避，
   * 避免额外使用软件定时器占用 Tmr Svc 任务栈。*/
  for (;;) {
    dynbuf_t buf;
    if (!dbuf_init(&buf, 4096)) {
      ESP_LOGE(TAG, "buf OOM");
      s_fetching = false;
      break;
    }

    bool ok = http_get_all(MUSIC_INDEX_URL, &buf);
    if (!ok) {
      dbuf_free(&buf);
      s_fetch_fail_count++;
      ESP_LOGW(TAG, "fetch failed: %s (fail=%d/%d)", MUSIC_INDEX_URL,
               s_fetch_fail_count, MUSIC_FETCH_RETRY_MAX);
      if (s_fetch_fail_count < MUSIC_FETCH_RETRY_MAX) {
        vTaskDelay(pdMS_TO_TICKS(MUSIC_FETCH_RETRY_DELAY_MS));
        continue;  /* 同一任务内重试下一轮 */
      } else {
        ESP_LOGW(TAG, "fetch: giving up after %d failures", s_fetch_fail_count);
        s_fetching = false;
        break;
      }
    }

    track_t *arr = NULL;
    int cnt = 0;
    ok = parse_index_json(buf.data, buf.len, &arr, &cnt);
    dbuf_free(&buf);

    if (!ok) {
      ESP_LOGE(TAG, "parse json failed");
      s_fetching = false;
      break;
    }

    s_fetch_fail_count = 0;
    ESP_LOGI(TAG, "loaded %d tracks", cnt);
    /* 日志打印后立即设置启动就绪标志，确保日志与状态同步 */
    startup_ready_set(STARTUP_READY_MUSIC);

    catalog_apply_tracks(arr, cnt, MUSIC_SOURCE_HTTP);
    break;
  }

  vTaskDelete(NULL);
}

static void try_start_fetch(void);

static void wait_ota_then_fetch_task(void *arg) {
  (void)arg;
  while (ota_update_is_running()) {
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  s_wait_ota_task = NULL;
  try_start_fetch();
  vTaskDelete(NULL);
}

static void schedule_fetch_after_ota(void) {
  if (s_wait_ota_task) {
    return;
  }
  /* 放到 Core0，尽量不抢 LVGL（Core1）；栈尽量放 PSRAM */
  const uint32_t stack = 3072;
  const UBaseType_t prio = 3;
  esp_err_t rc = audio_thread_create((audio_thread_t *)&s_wait_ota_task,
                                     "music_ota_wait",
                                     wait_ota_then_fetch_task,
                                     NULL,
                                     stack,
                                     prio,
                                     true,  /* stack in PSRAM when possible */
                                     0);
  if (rc != ESP_OK) {
    s_wait_ota_task = NULL;
    ESP_LOGW(TAG, "create ota wait task failed");
  }
}

static void try_start_fetch(void) {
  if (s_fetching)
    return;
  if (!(MUSIC_INDEX_URL[0])) {
    ESP_LOGW(TAG, "skip fetch: empty MUSIC_INDEX_URL");
    return;
  }
  if (ota_update_is_running()) {
    ESP_LOGW(TAG, "OTA running, delay music index fetch");
    schedule_fetch_after_ota();
    return;
  }
  /* 在创建任务前即置位，避免 delay 窗口内被重复触发导致并发拉取。*/
  s_fetching = true;
  const UBaseType_t prio = 4;
  /* 注意：在使用 HTTPS（mbedTLS）且启用 Heap Poisoning 时，
   * HTTP 客户端 + TLS 握手 + 本地 2KB 缓冲会占用较多栈空间。
   * 4096 已证明不够用，这里使用 8192 以保证稳定。*/
  const uint32_t stack = 8192;
  /* CPU1: 网络任务恢复原始配置，与音频管线同核 */
  esp_err_t rc = audio_thread_create(NULL,
                                     "music_idx",
                                     fetch_task,
                                     NULL,
                                     stack,
                                     prio,
                                     true,  /* stack in PSRAM when possible */
                                     1);
  if (rc != ESP_OK) {
    s_fetching = false;
    ESP_LOGE(TAG, "create fetch_task failed");
  }
}

static void on_got_ip(void *arg, esp_event_base_t base, int32_t id,
                      void *data) {
  (void)arg;
  (void)base;
  (void)id;
  (void)data;
  ESP_LOGI(TAG, "IP ready, try fetch index");
  /* 仅在使用 HTTP 索引模式时自动拉取；若当前处于 SD 卡模式，则忽略 IP 事件。*/
  if (!s_use_sdcard) {
    /* 直接启动拉取任务：HTTP/JSON 解析在独立任务中完成，不阻塞 IP 事件回调。
     * 如需额外延迟，可在 fetch_task 内部首行 vTaskDelay。*/
    try_start_fetch();
  }
}

static void on_wifi_disconnected(void *arg, esp_event_base_t base, int32_t id,
                                 void *data) {
  (void)arg;
  (void)base;
  (void)data;
  if (id != WIFI_EVENT_STA_DISCONNECTED)
    return;

  /* 若当前是 SD 卡模式，则 Wi-Fi 断开不影响本地音乐目录，忽略清空逻辑。*/
  if (s_use_sdcard) {
    ESP_LOGW(TAG, "Wi-Fi disconnected, but using SD card catalog, keep list");
    return;
  }

  ESP_LOGW(TAG, "Wi-Fi disconnected, clear HTTP music catalog");
  catalog_update_arg_t *msg = (catalog_update_arg_t *)calloc(1, sizeof(*msg));
  if (msg) {
    msg->tracks = NULL;
    msg->count = 0;
    msg->source = MUSIC_SOURCE_HTTP;
    lv_result_t res = lv_async_call_locked(catalog_update_cb, msg, 200);
    if (res != LV_RESULT_OK) {
      free(msg);
    }
  } else {
    /* 回退：至少清空 UI，标记状态 */
    s_fetching = false;
    s_fetched = false;
    free_catalog();
    (void)lv_async_call_locked(clear_list_async_cb, NULL, 200);
    ESP_LOGE(TAG, "on_wifi_disconnected: OOM dispatching reset");
  }
}

static void on_list_deleted(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  if (!obj)
    return;

  if (obj == s_list) {
    stop_visible_update_timer();
    stop_builder();
    s_list = NULL;
    list_epoch_bump();
  }
}

/* ---------------------------- API ----------------------------- */
void music_ui_init(lv_ui *ui) {
  if (s_inited) {
    s_ui = ui;
    return;
  } /* 幂等 */
  s_inited = true;
  s_ui = ui;
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
#if MUSIC_SKIP_CN_CHECK_DEV_MODE
  ESP_LOGW(TAG, "CONFIG_APP_MUSIC_SKIP_CN_CHECK=1，仅建议开发阶段使用");
#endif
  ensure_evt_dispatcher();
  /* 使用 instance 版本，可在需要时安全注销；并避免 “handler already registered,
   * overwriting” */
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL, &s_got_ip_ins));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, on_wifi_disconnected, NULL,
      &s_wifi_disc_ins));
}

void music_ui_attach_list(lv_obj_t *list_obj) {
  if (!list_obj)
    return;
  if (s_list && s_list != list_obj) {
    music_ui_detach_list(s_list);
  }

  s_list = list_obj;
  if (!list_ok(s_list)) {
    s_list = NULL;
    return;
  }

  stop_visible_update_timer();
  list_epoch_bump();
  lv_obj_remove_event_cb(s_list, on_list_scrolled);
  lv_obj_remove_event_cb(s_list, on_list_deleted);
  lv_obj_add_event_cb(s_list, on_list_scrolled, LV_EVENT_SCROLL, NULL);
  lv_obj_add_event_cb(s_list, on_list_deleted, LV_EVENT_DELETE, NULL);

  if (s_fetched && s_track_cnt > 0) {
    post_start_build_list(s_list, s_tracks, (size_t)s_track_cnt);
  } else if (s_current_index >= 0 && list_ok(s_list)) {
    lv_obj_t *btn = lv_obj_get_child(s_list, (uint32_t)s_current_index);
    if (btn) {
      focus_button_center(s_list, btn);
      list_update_selection();
    }
  }
}

void music_ui_detach_list(lv_obj_t *list_obj) {
  if (s_list == list_obj) {
    stop_builder();
    stop_visible_update_timer();
    lv_obj_remove_event_cb(s_list, on_list_scrolled);
    lv_obj_remove_event_cb(s_list, on_list_deleted);
    s_list = NULL;
    list_epoch_bump();
  }
}

void music_ui_refetch(void) { try_start_fetch(); }
void music_ui_set_play_cb(music_ui_play_cb_t cb) { s_play_cb = cb; }

int music_ui_count(void) { return s_track_cnt; }
const char *music_ui_title_at(int idx) {
  return (idx < 0 || idx >= s_track_cnt) ? NULL : s_tracks[idx].title;
}
const char *music_ui_url_at(int idx) {
  return (idx < 0 || idx >= s_track_cnt) ? NULL : s_tracks[idx].url;
}
const char *music_ui_duration_at(int idx) {
  return (idx < 0 || idx >= s_track_cnt) ? NULL : s_tracks[idx].duration;
}
uint32_t music_ui_duration_ms_at(int idx) {
  return (idx < 0 || idx >= s_track_cnt) ? 0u : s_tracks[idx].duration_ms;
}
int music_ui_current_index(void) { return s_current_index; }

bool music_ui_play_index(int idx) {
  if (idx < 0 || idx >= s_track_cnt) {
    return false;
  }
  last_click_set(esp_timer_get_time());
  play_track_by_index(idx, NULL, true);
  return true;
}

bool music_ui_toggle_favorite(int idx) {
  if (idx < 0 || idx >= s_track_cnt) {
    return false;
  }
  s_tracks[idx].liked = !s_tracks[idx].liked;
  return s_tracks[idx].liked;
}

bool music_ui_is_favorite(int idx) {
  if (idx < 0 || idx >= s_track_cnt) {
    return false;
  }
  return s_tracks[idx].liked;
}

int64_t music_ui_last_action_time_us(void) { return last_click_get(); }

bool music_ui_recent_action_within_us(int64_t window_us) {
  if (window_us <= 0) {
    return false;
  }
  int64_t last = last_click_get();
  if (last == 0) {
    return false;
  }
  int64_t now = esp_timer_get_time();
  return (now - last) < window_us;
}

void music_ui_dispatch_player_state(ap_state_t state) {
  ensure_evt_dispatcher();
  if (!s_evt_queue) {
    ESP_LOGW(TAG, "state queue not ready");
    return;
  }
  music_ui_evt_t evt = {.type = MUSIC_UI_EVT_STATE};
  evt.data.state = state;
  if (!enqueue_music_evt(&evt)) {
    ESP_LOGW(TAG, "dispatch state drop %d", (int)state);
  }
}

void music_ui_dispatch_playback_finished(bool forward) {
  ensure_evt_dispatcher();
  if (!s_evt_queue) {
    ESP_LOGW(TAG, "finished queue not ready");
    return;
  }
  music_ui_evt_t evt = {.type = MUSIC_UI_EVT_FINISHED};
  evt.data.forward = forward;
  if (!enqueue_music_evt(&evt)) {
    ESP_LOGW(TAG, "dispatch finished drop");
  }
}

void music_ui_notify_url_resolved(const char *url)
{
  if (!url) url = "(null)";
  ESP_LOGI(TAG, "url_resolved: %s", url);
  /* 如需在 UI 显示，可在此更新标签文本。当前仅打印日志避免阻塞。*/
}

/* ----------------- HTTP / SD 卡 数据源切换 ----------------- */

static bool sd_enum_cb(const sdcard_music_item_t *item, void *user)
{
  sd_build_ctx_t *c = (sd_build_ctx_t *)user;
  if (!c || !item) return false;
  if (c->count >= c->capacity) {
    return false;
  }

  track_t *t = &c->arr[c->count];
  memset(t, 0, sizeof(*t));
  t->title = dup_str(item->name);
  t->url   = dup_str(item->path);
  t->duration = NULL;
  /* 对于 SD 卡，本地估算一个 duration_ms 作为“初始总时长”，
   * 既用于列表右侧展示，也用于进度条与自动切歌的 fallback；
   * 若解码器后续上报更精确的 duration，可由底层播放器覆盖。*/
  t->duration_ms = 0;
  t->liked = false;

  /* 既用于列表右侧“总时长”初始显示，也作为进度条的初始总时长；
   * 若解码器后续上报更精确的时长，播放器会在 MUSIC_INFO 中进行修正。*/
  uint32_t est_ms = sd_estimate_duration_ms(item);
  if (est_ms > 0) {
    char mmss[6];
    ms_to_mmss_local(est_ms, mmss);
    t->duration = dup_str(mmss);
    t->duration_ms = est_ms;
  }

  if (!t->title || !t->url) {
    free(t->title);
    free(t->url);
    free(t->duration);
    memset(t, 0, sizeof(*t));
    return false;
  }

  c->count++;
  return true;
}

/* 构建基于 SD 卡扫描结果的 track_t 数组，并提交给 UI。
 * - 若 SD 卡无文件或尚未就绪，则清空列表。*/
static void music_ui_build_sdcard_catalog(void)
{
  size_t n = sdcard_music_count();
  if (n == 0) {
    ESP_LOGW(TAG, "SD card catalog: no audio files, clear list");
    catalog_apply_tracks(NULL, 0, MUSIC_SOURCE_SDCARD);
    return;
  }

  track_t *arr = (track_t *)calloc(n, sizeof(track_t));
  if (!arr) {
    ESP_LOGE(TAG, "SD card catalog: OOM for %u tracks", (unsigned)n);
    catalog_apply_tracks(NULL, 0, MUSIC_SOURCE_SDCARD);
    return;
  }

  sd_build_ctx_t ctx = {
      .arr = arr,
      .capacity = (int)n,
      .count = 0,
  };

  sdcard_music_enum(sd_enum_cb, &ctx);

  if (ctx.count <= 0) {
    ESP_LOGW(TAG, "SD card catalog: enumeration produced 0 entries");
    free_tracks_array(arr, (int)n);
    catalog_apply_tracks(NULL, 0, MUSIC_SOURCE_SDCARD);
    return;
  }

  /* 根据标题排序，确保 000/001/.../290 等按自然顺序展示 */
  if (ctx.count > 1) {
    qsort(arr, (size_t)ctx.count, sizeof(track_t), sd_track_title_cmp);
  }

  ESP_LOGI(TAG, "SD card catalog: loaded %d tracks", ctx.count);
  catalog_apply_tracks(arr, ctx.count, MUSIC_SOURCE_SDCARD);
}

void music_ui_toggle_source(void)
{
  s_use_sdcard = !s_use_sdcard;
  ESP_LOGI(TAG, "music_ui_toggle_source: now %s",
           s_use_sdcard ? "SDCARD" : "HTTP");

  /* 同步更新 MusicScreen 顶部“当前来源”标签 */
  MusicScreen_UpdateSourceLabel(s_use_sdcard);

  if (s_use_sdcard) {
    /* 进入 SD 卡模式：基于当前 SD 扫描结果重建目录 */
    music_ui_build_sdcard_catalog();
  } else {
    /* 切回 HTTP 模式：先清空目录，再触发一次拉取 */
    catalog_apply_tracks(NULL, 0, MUSIC_SOURCE_HTTP);
    try_start_fetch();
  }
}

bool music_ui_is_using_sdcard(void)
{
  return s_use_sdcard;
}
