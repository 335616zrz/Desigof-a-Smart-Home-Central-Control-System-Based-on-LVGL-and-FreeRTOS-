#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "audio_common.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_pipeline.h"
#include "http_stream.h"
#include "esp_http_client.h"

#include "audio_i2s.h"
#include "audio_player.h"
#include "board_config.h"
#include "server_root_cert.h"
#include "esp_heap_caps.h"

static const char *TAG = "audio_player";
extern void i2s_sink_request_abort(void);
extern void i2s_sink_clear_abort(void);

/* 简易 HTTP 事件日志与“Origin”补充，便于兼容某些服务器策略 */
static int http_evt_logger(http_stream_event_msg_t *m)
{
  if (!m) return ESP_OK;
  esp_http_client_handle_t c = (esp_http_client_handle_t)m->http_client;
  const char *uri = audio_element_get_uri(m->el);
  switch (m->event_id) {
  case HTTP_STREAM_PRE_REQUEST:
    if (c) {
      /* 一些反代会基于 Origin 做简单校验，这里与 WS 对齐 */
      esp_http_client_set_header(c, "Origin", "https://servers.local");
      esp_http_client_set_header(c, "Accept", "*/*");
      esp_http_client_set_header(c, "Accept-Encoding", "identity");
      /* 少数服务端会校验 Referer，没有害处，顺带加上 */
      esp_http_client_set_header(c, "Referer", "https://servers.local/");
    }
    ESP_LOGD(TAG, "HTTP PRE uri=%s", uri ? uri : "(null)");
    return ESP_OK;
  case HTTP_STREAM_POST_REQUEST:
    if (c) {
      int sc = esp_http_client_get_status_code(c);
      ESP_LOGD(TAG, "HTTP open status=%d uri=%s", sc, uri ? uri : "(null)");
    }
    return ESP_OK;
  case HTTP_STREAM_ON_RESPONSE:
    if (c) {
      int sc = esp_http_client_get_status_code(c);
      long long cl = esp_http_client_get_content_length(c);
      ESP_LOGD(TAG, "HTTP resp status=%d content_len=%lld", sc, cl);
    }
    return 0; /* 继续读取 */
  case HTTP_STREAM_FINISH_TRACK:
    ESP_LOGD(TAG, "HTTP finish track");
    return ESP_OK;
  default:
    return ESP_OK;
  }
}

/* ========= 可配置参数（来自 Kconfig） ========= */
#ifndef CONFIG_AUDIO_PLAYER_HTTP_BUFSZ
#define CONFIG_AUDIO_PLAYER_HTTP_BUFSZ 2048
#endif
#ifndef CONFIG_AUDIO_PLAYER_RINGBUF_SZ
#define CONFIG_AUDIO_PLAYER_RINGBUF_SZ 16384
#endif
#ifndef CONFIG_AUDIO_PLAYER_HTTP_STACK_IN_EXT
#define CONFIG_AUDIO_PLAYER_HTTP_STACK_IN_EXT 0
#endif
#ifndef CONFIG_AUDIO_PLAYER_HTTP_TASK_PRIO
#define CONFIG_AUDIO_PLAYER_HTTP_TASK_PRIO 8
#endif
#ifndef CONFIG_AUDIO_PLAYER_DEC_TASK_PRIO
#define CONFIG_AUDIO_PLAYER_DEC_TASK_PRIO 7
#endif
#ifndef CONFIG_AUDIO_PLAYER_DEFAULT_VOL
#define CONFIG_AUDIO_PLAYER_DEFAULT_VOL 100
#endif

/* 由 CMake 注入的 HAVE_xxx 宏控制是否包含对应头文件 */
#ifdef HAVE_MP3_DEC
#include "mp3_decoder.h"
#endif
#ifdef HAVE_AAC_DEC
#include "aac_decoder.h"
#endif
#ifdef HAVE_FLAC_DEC
#include "flac_decoder.h"
#endif
#ifdef HAVE_WAV_DEC
#include "wav_decoder.h"
#endif
#ifdef HAVE_OPUS_DEC
#include "opus_decoder.h"
#endif
#ifdef HAVE_AMRNB_DEC
#include "amr_decoder.h"
#endif
#ifdef HAVE_AMRWB_DEC
#include "amr_decoder.h"
#endif

extern audio_element_handle_t i2s_sink_element_create(void);

/* ========= 内部上下文 ========= */
typedef struct {
  audio_pipeline_handle_t pipeline;
  audio_element_handle_t http;
  audio_element_handle_t dec;
  audio_element_handle_t sink;
  audio_event_iface_handle_t evt;
  ap_event_cb_t cb;
  void *user;
  ap_state_t state;
} ap_ctx_t;

static ap_ctx_t g = {0};
static TaskHandle_t g_evt_task = NULL;
static volatile bool g_stop_requested = false;
/* 保护管线 stop/cleanup 等关键区，避免并发停止导致崩溃 */
static SemaphoreHandle_t g_pl_mtx = NULL;
static bool g_http_registered = false;
static bool g_sink_registered = false;
static bool g_dec_registered = false;
/* 标记当前源元素是否为“本地文件源”（SD 卡），避免依赖 tag 名判别类型，
 * 因为 audio_pipeline_register 会修改元素的 tag。*/
static bool g_src_is_file = false;
static bool g_pipeline_active = false;
static char *g_current_url = NULL;
static uint32_t g_current_duration_ms = 0;
static int64_t g_next_start_byte = -1;
static int64_t g_last_total_bytes = 0;
static int g_initial_volume = CONFIG_AUDIO_PLAYER_DEFAULT_VOL;
static uint32_t g_bytes_per_ms = 0;
static int64_t g_data_start_byte = 0;
static uint32_t g_sample_bytes_per_ms = 0;
static uint32_t g_pending_seek_ms = UINT32_MAX;
static volatile bool g_last_stop_was_eof = false;
static bool g_http_fallback_tried = false;
static bool g_auto_retry_tried = false;

/* 事件：使用 element callback -> 本地队列，避免 ADF 的 queue_set listener
 * 在反复切歌/重建时出现“add/remove queue set”失败与无限循环。 */
typedef struct {
  audio_event_iface_msg_t msg;
} ap_evt_msg_t;

static QueueHandle_t g_evt_q = NULL;

static inline void evtq_drain(void)
{
  if (!g_evt_q) return;
  ap_evt_msg_t m;
  while (xQueueReceive(g_evt_q, &m, 0) == pdTRUE) {
  }
}

static esp_err_t element_evt_cb(audio_element_handle_t el, audio_event_iface_msg_t *msg, void *ctx)
{
  (void)el;
  (void)ctx;
  if (!g_evt_q || !msg) return ESP_OK;
  ap_evt_msg_t m = {.msg = *msg};
  /* 绝不阻塞 element 线程；队列满时丢弃（避免卡住解码/HTTP/i2s）。 */
  (void)xQueueSend(g_evt_q, &m, 0);
  return ESP_OK;
}

static inline void attach_element_event_cb(audio_element_handle_t el)
{
  if (!el) return;
  (void)audio_element_set_event_callback(el, element_evt_cb, NULL);
}

/* 常驻事件线程：TCB 固定在内部 RAM，栈放在 PSRAM */
static StaticTask_t s_evt_tcb_static;           /* internal RAM */
static StackType_t *s_evt_stack_static = NULL;  /* PSRAM */
static size_t       s_evt_stack_words  = 0;

static bool ensure_src_element(const char *url);
static bool ensure_sink_element(void);
static void reset_element_safely(audio_element_handle_t el);
// 常驻事件线程，已不再需要等待其退出的逻辑
static inline void pl_lock(void) { if (g_pl_mtx) xSemaphoreTake(g_pl_mtx, portMAX_DELAY); }
static inline void pl_unlock(void) { if (g_pl_mtx) xSemaphoreGive(g_pl_mtx); }

static void recalc_bytes_per_ms(void) {
  /* 1) 优先用 UI 提供的 duration 反推 bytes/ms（HTTP 总字节已知时） */
  if (g_current_duration_ms > 0 && g_last_total_bytes > g_data_start_byte) {
    uint64_t data_bytes = (uint64_t)g_last_total_bytes - (uint64_t)g_data_start_byte;
    uint32_t calc = (uint32_t)(data_bytes / g_current_duration_ms);
    if (calc > 0) {
      g_bytes_per_ms = calc;
    }
  }
  /* 2) 否则使用解码器上报的采样信息估算 bytes/ms */
  if (g_bytes_per_ms == 0 && g_sample_bytes_per_ms > 0) {
    g_bytes_per_ms = g_sample_bytes_per_ms;
  }
  /* 3) 若 UI 未提供 duration，则在能计算时用总字节反推出一个估算的 duration */
  if (g_current_duration_ms == 0 && g_bytes_per_ms > 0 &&
      g_last_total_bytes > g_data_start_byte) {
    uint64_t data_bytes = (uint64_t)g_last_total_bytes - (uint64_t)g_data_start_byte;
    uint32_t est = (uint32_t)(data_bytes / (uint64_t)g_bytes_per_ms);
    if (est > 0 && est != UINT32_MAX) {
      g_current_duration_ms = est;
      ESP_LOGI(TAG, "duration resolved via bytes: %u ms", (unsigned)g_current_duration_ms);
    }
  }
}

/*
 * 事件派发函数：在本文件内任何使用点之前就给出“定义”，
 * 彻底避免因为隐式声明导致的“non-static declaration”冲突。
 * 注意：必须放在 g 上下文定义之后（需要访问 g.cb/g.user）。
 */
static inline void _emit(ap_event_t e, const void *p)
{
  if (g.cb) {
    g.cb(e, p, g.user);
  }
}

/* 在个别解码器未及时上报 MUSIC_INFO 时，尝试轮询一次解码器的信息作为兜底 */
static bool try_poll_music_info(void)
{
  if (!g.dec || g.state != AP_STATE_LOADING)
    return false;

  audio_element_info_t info = {0};
  audio_element_getinfo(g.dec, &info);
  if (info.sample_rates <= 0 || info.channels <= 0 || info.bits <= 0) {
    /* 兜底：若解码器尚未上报 MUSIC_INFO，但 I2S sink 已经进入 RUNNING，
     * 说明音频实际上已经开始播放。此时先把状态切到 PLAYING，
     * 之后真正的 MUSIC_INFO 到来时再补齐采样信息与时长估算。*/
    audio_element_state_t sink_st = g.sink ? audio_element_get_state(g.sink)
                                           : AEL_STATE_NONE;
    if (sink_st == AEL_STATE_RUNNING) {
      g.state = AP_STATE_PLAYING;
      _emit(AP_EVT_STATE, &g.state);
      return true; /* 已经促发到 PLAYING */
    }
    return false;
  }

  ap_music_info_t mi = {.sample_rate = info.sample_rates,
                        .channels = info.channels,
                        .bits = info.bits};

  /* 与事件分支保持一致：切换 I2S 采样率并进入 PLAYING */
  audio_i2s_pause();
  audio_i2s_set_sample_rate(info.sample_rates);
  audio_i2s_resume();
  _emit(AP_EVT_INFO, &mi);
  g.state = AP_STATE_PLAYING;
  _emit(AP_EVT_STATE, &g.state);

  if (info.sample_rates && info.channels && info.bits) {
    uint64_t bytes_per_sec =
        (uint64_t)info.sample_rates * info.channels * info.bits / 8ULL;
    if (bytes_per_sec >= 1000ULL) {
      g_sample_bytes_per_ms = (uint32_t)(bytes_per_sec / 1000ULL);
      if (g_sample_bytes_per_ms == 0) g_sample_bytes_per_ms = 1;
      if (g_bytes_per_ms == 0) g_bytes_per_ms = g_sample_bytes_per_ms;
    }
  }

  if (g.http) {
    audio_element_info_t http_info = {0};
    audio_element_getinfo(g.http, &http_info);
    if (http_info.total_bytes > 0) {
      if (http_info.total_bytes > g_last_total_bytes) {
        g_last_total_bytes = http_info.total_bytes;
      }
    }
  }
  recalc_bytes_per_ms();
  return true;
}

/* 在事件循环的每一轮都尝试把 LOADING 提升为 PLAYING，
 * 避免仅在 200ms 超时分支里才触发，导致某些持续有事件的场景中
 * UI 长时间收不到 PLAYING。*/
static void maybe_promote_loading_to_playing(void)
{
  if (g.state != AP_STATE_LOADING)
    return;

  bool promoted = false;
  if (g.dec) {
    audio_element_info_t info = {0};
    audio_element_getinfo(g.dec, &info);
    if (info.sample_rates > 0 && info.channels > 0 && info.bits > 0) {
      ap_music_info_t mi = {.sample_rate = info.sample_rates,
                            .channels = info.channels,
                            .bits = info.bits};
      audio_i2s_pause();
      audio_i2s_set_sample_rate(info.sample_rates);
      audio_i2s_resume();
      _emit(AP_EVT_INFO, &mi);
      g.state = AP_STATE_PLAYING;
      ESP_LOGI(TAG, "promote PLAYING via decoder info: sr=%d ch=%d bits=%d",
               mi.sample_rate, mi.channels, mi.bits);
      _emit(AP_EVT_STATE, &g.state);

      uint64_t bytes_per_sec =
          (uint64_t)info.sample_rates * info.channels * info.bits / 8ULL;
      if (bytes_per_sec >= 1000ULL) {
        g_sample_bytes_per_ms = (uint32_t)(bytes_per_sec / 1000ULL);
        if (g_sample_bytes_per_ms == 0) g_sample_bytes_per_ms = 1;
        if (g_bytes_per_ms == 0) g_bytes_per_ms = g_sample_bytes_per_ms;
      }
      promoted = true;
    }
  }

  if (!promoted) {
    audio_element_state_t sink_st = g.sink ? audio_element_get_state(g.sink)
                                           : AEL_STATE_NONE;
    if (sink_st == AEL_STATE_RUNNING) {
      g.state = AP_STATE_PLAYING;
      ESP_LOGI(TAG, "promote PLAYING via sink RUNNING");
      _emit(AP_EVT_STATE, &g.state);
    }
  }
}
static bool restart_pipeline_at(int64_t target) {
  ESP_LOGI(TAG, "restart pipeline at byte=%" PRIi64, target);
  g_next_start_byte = target;
  if (!ap_stop()) {
    g_next_start_byte = -1;
    return false;
  }
  if (!ap_play_url(g_current_url)) {
    g_next_start_byte = -1;
    return false;
  }
  return true;
}
static char g_dec_ext[8] = {0};

/* _emit 已在文件更前处定义，避免重复定义 */

/* 统一配置解码器：Core1 / prio=7 / 栈位置按 Kconfig / 输出环形缓冲按 Kconfig */
#define DEC_COMMON_CFG(_cfg_)                                                  \
  do {                                                                         \
    (_cfg_).task_core = 1;                                                     \
    (_cfg_).task_prio = CONFIG_AUDIO_PLAYER_DEC_TASK_PRIO;                     \
    (_cfg_).task_stack = 4096;                                                 \
    (_cfg_).stack_in_ext = (CONFIG_AUDIO_PLAYER_DEC_STACK_IN_EXT != 0);        \
    (_cfg_).out_rb_size = CONFIG_AUDIO_PLAYER_RINGBUF_SZ;                      \
  } while (0)

static void reset_element_safely(audio_element_handle_t el);
// 常驻事件线程，不提供退出等待

/* 按扩展名创建解码器。
 *
 * 注意：
 * - 我们使用 element event callback -> 本地队列，不依赖 ADF 的 queue_set listener；
 *   因此这里可以安全 deinit 旧解码器，避免切歌造成泄漏。 */
static audio_element_handle_t _create_decoder_by_ext(const char *url) {
  const char *ext = strrchr(url, '.');
  if (!ext)
    return NULL;
  ext++;

  char ebuf[16] = {0};
  for (int i = 0; i < (int)sizeof(ebuf) - 1 && ext[i]; ++i) {
    const char c = ext[i];
    ebuf[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
  }

  /* 若已有旧解码器且扩展名一致，则复用之，仅做状态重置。*/
  if (g.dec && g_dec_ext[0] != '\0' && strcmp(g_dec_ext, ebuf) == 0) {
    ESP_LOGD(TAG, "reuse decoder for .%s", ebuf);
    reset_element_safely(g.dec);
    return g.dec;
  }
  /* 扩展名不同：释放旧解码器，避免内存泄漏（切换 mp3<->wav 会不断 new decoder）。 */
  if (g.dec) {
    if (g_dec_registered && g.pipeline) {
      (void)audio_pipeline_unregister(g.pipeline, g.dec);
      g_dec_registered = false;
    }
    (void)audio_element_deinit(g.dec);
    g.dec = NULL;
    g_dec_ext[0] = '\0';
  }

  audio_element_handle_t dec = NULL;

#ifdef HAVE_MP3_DEC
  if (!strcmp(ebuf, "mp3")) {
    mp3_decoder_cfg_t cfg = DEFAULT_MP3_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = mp3_decoder_init(&cfg);
  }
#endif
#ifdef HAVE_AAC_DEC
  if (!strcmp(ebuf, "aac") || !strcmp(ebuf, "adts")) {
    aac_decoder_cfg_t cfg = DEFAULT_AAC_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = aac_decoder_init(&cfg);
  }
#endif
#ifdef HAVE_FLAC_DEC
  if (!strcmp(ebuf, "flac")) {
    flac_decoder_cfg_t cfg = DEFAULT_FLAC_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = flac_decoder_init(&cfg);
  }
#endif
#ifdef HAVE_WAV_DEC
  if (!strcmp(ebuf, "wav")) {
    wav_decoder_cfg_t cfg = DEFAULT_WAV_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = wav_decoder_init(&cfg);
  }
#endif
#ifdef HAVE_OPUS_DEC
  if (!strcmp(ebuf, "opus")) {
    opus_decoder_cfg_t cfg = DEFAULT_OPUS_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = opus_decoder_init(&cfg);
  }
#endif
#if defined(HAVE_AMRNB_DEC) || defined(HAVE_AMRWB_DEC)
  if (!strcmp(ebuf, "amr")) {
#ifdef HAVE_AMRNB_DEC
    amrnb_decoder_cfg_t cfg = DEFAULT_AMRNB_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = amrnb_decoder_init(&cfg);
#elif defined(HAVE_AMRWB_DEC)
    amrwb_decoder_cfg_t cfg = DEFAULT_AMRWB_DECODER_CONFIG();
    DEC_COMMON_CFG(cfg);
    dec = amrwb_decoder_init(&cfg);
#endif
  }
#endif

  if (!dec) {
    ESP_LOGW(TAG, "No decoder for .%s (url=%s)", ebuf, url);
    return NULL;
  }

  strncpy(g_dec_ext, ebuf, sizeof(g_dec_ext) - 1);
  g_dec_ext[sizeof(g_dec_ext) - 1] = '\0';
  g.dec = dec;
  attach_element_event_cb(g.dec);
  ESP_LOGI(TAG, "decoder selected: .%s", g_dec_ext);
  return dec;
}

static audio_element_handle_t create_http_element(void) {
  http_stream_cfg_t hcfg = HTTP_STREAM_CFG_DEFAULT();
  hcfg.type = AUDIO_STREAM_READER;
  // 回到 Core1（默认与解码同核更稳），避免跨核切换导致事件节拍与缓冲抖动
  hcfg.task_core = 1;
  hcfg.task_prio = CONFIG_AUDIO_PLAYER_HTTP_TASK_PRIO;
  hcfg.stack_in_ext = (CONFIG_AUDIO_PLAYER_HTTP_STACK_IN_EXT != 0);
  hcfg.request_size = CONFIG_AUDIO_PLAYER_HTTP_BUFSZ;
  hcfg.out_rb_size = CONFIG_AUDIO_PLAYER_RINGBUF_SZ;
  hcfg.request_range_size = 0;
  /* 生产模式：使用服务器 CA/自签根证书做链校验 */
  hcfg.cert_pem = server_root_cert_pem;
  hcfg.event_handle = http_evt_logger;
  hcfg.user_agent = "ESP32S3-MusicPlayer/1.0";
  return http_stream_init(&hcfg);
}

/* 根据 URL 选择合适的“源”元素：
 * - HTTP/HTTPS：使用 http_stream 作为源；
 * - 以 /sdcard/ 或 file:// 开头：使用文件流源（从 SD 卡读取）。*/
static bool is_sdcard_url(const char *url)
{
  if (!url) return false;
  if (strncmp(url, "/sdcard/", 8) == 0) return true;
  if (strncasecmp(url, "file://", 7) == 0) return true;
  return false;
}

extern audio_element_handle_t file_src_element_create(void);
extern void file_src_set_seek_position(int64_t byte_pos);

static bool ensure_src_element(const char *url)
{
  const bool want_sd = is_sdcard_url(url);
  const char *cur_tag = g.http ? audio_element_get_tag(g.http) : NULL;
  ESP_LOGI(TAG, "ensure_src_element: url=%s want_sd=%d cur_tag=%s src_is_file=%d",
           url ? url : "(null)", (int)want_sd,
           cur_tag ? cur_tag : "(null)", (int)g_src_is_file);

  if (g.http) {
    /* 若当前源类型与本次请求一致，复用之 */
    if (want_sd == g_src_is_file) {
      return true;
    }
    /* 类型不符：停止并销毁旧源，避免泄漏与状态残留 */
    ESP_LOGI(TAG, "recreate src element: old=%s new=%s",
             g_src_is_file ? "file" : "http", want_sd ? "file" : "http");
    if (g_http_registered && g.pipeline) {
      (void)audio_pipeline_unregister(g.pipeline, g.http);
      g_http_registered = false;
    }
    (void)audio_element_deinit(g.http);
    g.http = NULL;
    g_src_is_file = false;
  }

  if (want_sd) {
    g.http = file_src_element_create();
    g_src_is_file = (g.http != NULL);
  } else {
    g.http = create_http_element();
    g_src_is_file = false;
  }

  if (!g.http) {
    ESP_LOGE(TAG, "create %s source failed", want_sd ? "file" : "http");
    g_src_is_file = false;
    return false;
  }
  attach_element_event_cb(g.http);
  return true;
}

static bool ensure_sink_element(void) {
  if (g.sink) {
    return true;
  }
  g.sink = i2s_sink_element_create();
  if (!g.sink) {
    ESP_LOGE(TAG, "create i2s sink failed");
    return false;
  }
  attach_element_event_cb(g.sink);
  return true;
}

static void reset_element_safely(audio_element_handle_t el) {
  if (!el) {
    return;
  }
  (void)audio_element_reset_state(el);
  (void)audio_element_reset_input_ringbuf(el);
  (void)audio_element_reset_output_ringbuf(el);
  (void)audio_element_set_uri(el, NULL);
}

/* 统一停止 & 清理当前 pipeline（可重复调用，幂等） */
static bool _pipeline_stop_running(void) {
  if (!g.pipeline)
    return true;

  g_last_stop_was_eof = false;
  i2s_sink_request_abort();
  /* Ensure I2S write can't block indefinitely (audio_i2s_write uses bounded waits but pause helps exit faster). */
  (void)audio_i2s_pause();

  /* === 1) 无条件 abort 所有 ringbuf：先唤醒所有可能卡住的线程 ===
   *
   * 说明：
   * - audio_element_terminate 依赖 element task 能处理 DESTROY cmd；
   * - 若任务阻塞在 rb_read/rb_write，则需要先 abort ringbuf 才能尽快退出。 */
  if (g.http) {
    (void)audio_element_abort_input_ringbuf(g.http);
    (void)audio_element_abort_output_ringbuf(g.http);
  }
  if (g.dec) {
    (void)audio_element_abort_input_ringbuf(g.dec);
    (void)audio_element_abort_output_ringbuf(g.dec);
  }
  if (g.sink) {
    (void)audio_element_abort_input_ringbuf(g.sink);
    (void)audio_element_abort_output_ringbuf(g.sink);
  }

  /* ADF 的 audio_pipeline_stop/wait_for_stop 仅在 pipeline->state==RUNNING 时返回 ESP_OK，
   * 否则会返回 ESP_FAIL（例如已经 stop/cleanup 过，或错误状态）。这并不意味着 element
   * 真的“坏了”。这里采用 best-effort：总是请求各 element stop 并等待，避免因 ESP_FAIL
   * 误判而丢弃句柄导致内存泄漏。 */
  if (g.http) (void)audio_element_stop(g.http);
  if (g.dec)  (void)audio_element_stop(g.dec);
  if (g.sink) (void)audio_element_stop(g.sink);

  esp_err_t stop_ret = audio_pipeline_stop(g.pipeline);
  if (stop_ret != ESP_OK) {
    ESP_LOGW(TAG, "audio_pipeline_stop ignored: %s (app_state=%d)",
             esp_err_to_name(stop_ret), (int)g.state);
  }

  esp_err_t wait_ret = audio_pipeline_wait_for_stop_with_ticks(g.pipeline, pdMS_TO_TICKS(800));
  if (wait_ret != ESP_OK) {
    ESP_LOGW(TAG, "audio_pipeline_wait_for_stop ignored: %s",
             esp_err_to_name(wait_ret));
  }
  /* 某些异常路径下 pipeline->state 可能不是 RUNNING，wait_for_stop 会直接返回 ESP_FAIL
   * 而不复位 state；这里强制回到 INIT，避免后续 audio_pipeline_run() 误判“已启动”。 */
  (void)audio_pipeline_change_state(g.pipeline, AEL_STATE_INIT);

  g_pipeline_active = false;
  g_pending_seek_ms = UINT32_MAX;

  /* === 3) 硬保证：terminate 三个元素，等 TASK_DESTROYED_BIT ===
   *
   * ADF 的 is_running/STOPPED 边缘语义在极端快速切歌场景下不可靠；我们直接等待
   * TASK_DESTROYED_BIT，确保 task 真的退出后再 cleanup/unlink，彻底规避 ringbuf UAF。 */
  const TickType_t term_ticks = pdMS_TO_TICKS(500);
  if (g.http && audio_element_terminate_with_ticks(g.http, term_ticks) != ESP_OK) {
    ESP_LOGE(TAG, "http terminate timeout");
    evtq_drain();
    return false;
  }
  if (g.dec && audio_element_terminate_with_ticks(g.dec, term_ticks) != ESP_OK) {
    ESP_LOGE(TAG, "dec terminate timeout");
    evtq_drain();
    return false;
  }
  if (g.sink && audio_element_terminate_with_ticks(g.sink, term_ticks) != ESP_OK) {
    ESP_LOGE(TAG, "sink terminate timeout");
    evtq_drain();
    return false;
  }

  /* All elements stopped: safe to clear abort and optionally re-enable I2S for next playback. */
  i2s_sink_clear_abort();
  (void)audio_i2s_resume();

  /* 防止在 STOP/TERMINATE 过程中各元素仍尝试上报事件导致队列/队列集爆满，
     这里显式清空一次事件接口的队列。*/
  evtq_drain();
  return true;
}

static void _pipeline_cleanup(void) {
  if (!g.pipeline)
    return;

  /* 重要：先 unlink，释放 rb_list 并断开 ringbuf。
   *
   * 否则在“切歌 mp3<->wav”场景中，我们可能先 deinit 旧 decoder，
   * 但 pipeline->rb_list 里仍保存着旧 decoder 的 host_el 指针；
   * 下一次 audio_pipeline_link() 触发内部 auto-unlink 时会对悬空指针
   * 调用 audio_element_set_*_ringbuf，导致内存损坏，进而出现
   * AUDIO_EVT / queue_set 相关的连环报错与无声播放。 */
  (void)audio_pipeline_unlink(g.pipeline);

  if (g.http && g_http_registered) {
    audio_pipeline_unregister(g.pipeline, g.http);
    g_http_registered = false;
  }
  if (g.dec && g_dec_registered) {
    audio_pipeline_unregister(g.pipeline, g.dec);
    g_dec_registered = false;
  }
  if (g.sink && g_sink_registered) {
    audio_pipeline_unregister(g.pipeline, g.sink);
    g_sink_registered = false;
  }

  audio_pipeline_reset_ringbuffer(g.pipeline);
  audio_pipeline_reset_items_state(g.pipeline);

  /* 保留实例，重置状态等待下次播放复用 */
  reset_element_safely(g.http);
  reset_element_safely(g.dec);
  reset_element_safely(g.sink);

  g_pipeline_active = false;

  /* 清理阶段再次丢弃遗留事件，避免在下一次播放前残留的 STOP/STATUS 挤占队列。*/
  evtq_drain();
}

/* 硬重置当前 pipeline：
 * - 停止并清理现有管线；
 * - 调用 audio_pipeline_deinit 销毁管线及其中元素；
 * - 将所有与元素相关的全局句柄/标志复位。
 *
 * 注意：不触碰事件线程与 I2S，本函数仅保证“从 source/decoder 到 sink”完全互斥。*/
static void ap_hard_reset_pipeline(void)
{
  if (!g.pipeline) {
    /* 确保元素相关全局状态也回到“空” */
    g.http = NULL;
    g.sink = NULL;
    g.dec  = NULL;
    g_http_registered = false;
    g_sink_registered = false;
    g_dec_registered  = false;
    g_dec_ext[0]      = '\0';
    g_pipeline_active = false;
    g_last_total_bytes = 0;
    g_data_start_byte  = 0;
    g_bytes_per_ms     = 0;
    g_sample_bytes_per_ms = 0;
    g_pending_seek_ms  = UINT32_MAX;
    g_next_start_byte  = -1;
    g_src_is_file      = false;
    return;
  }

  g_stop_requested = true;
  pl_lock();
  bool stopped = _pipeline_stop_running();
  if (!stopped) {
    pl_unlock();
    ESP_LOGE(TAG, "FATAL: stop failed in hard reset, restarting to avoid ringbuf UAF");
    esp_restart();
    return; /* should not reach */
  }
  _pipeline_cleanup();
  pl_unlock();
  g_stop_requested = false;

  /* _pipeline_cleanup 已把元素从 pipeline 里 unregister 掉，
   * 这里需要手动 deinit 元素，避免 ap_deinit() 路径泄漏。 */
  if (g.http) {
    (void)audio_element_deinit(g.http);
    g.http = NULL;
  }
  if (g.dec) {
    (void)audio_element_deinit(g.dec);
    g.dec = NULL;
  }
  if (g.sink) {
    (void)audio_element_deinit(g.sink);
    g.sink = NULL;
  }

  audio_pipeline_deinit(g.pipeline);
  g.pipeline = NULL;

  g_http_registered = false;
  g_sink_registered = false;
  g_dec_registered  = false;
  g_dec_ext[0]      = '\0';
  g_pipeline_active = false;
  g_last_total_bytes = 0;
  g_data_start_byte  = 0;
  g_bytes_per_ms     = 0;
  g_sample_bytes_per_ms = 0;
  g_pending_seek_ms  = UINT32_MAX;
  g_next_start_byte  = -1;
  g_src_is_file      = false;
}

static void audio_evt_task(void *arg) {
  (void)arg;
  for (;;) {
    /* 避免长时间霸占 Core1，让 IDLE 任务喂狗 */
    vTaskDelay(pdMS_TO_TICKS(1));

    /* 若收到外部停止请求：主动停止并清理当前管线，但线程常驻继续监听 */
    if (g_stop_requested) {
      /* stop 正在进行中（由 ap_stop/ap_hard_reset_pipeline 同步执行），
       * 这里只清空残留事件并短暂等待，不执行 stop/cleanup，
       * 避免“延迟 stop”误伤新启动的 pipeline。 */
      evtq_drain();
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    if (!g_evt_q) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    ap_evt_msg_t m = {0};
    if (xQueueReceive(g_evt_q, &m, pdMS_TO_TICKS(200)) != pdTRUE) {
      /* 无事件：轮询一次解码器信息 + 尝试将 LOADING 提升为 PLAYING。 */
      (void)try_poll_music_info();
      (void)maybe_promote_loading_to_playing();
      continue;
    }
    const audio_event_iface_msg_t msg = m.msg;

    if (msg.source_type != AUDIO_ELEMENT_TYPE_ELEMENT) {
      (void)maybe_promote_loading_to_playing();
      continue;
    }

    /* 丢弃“旧元素”残留事件（例如切歌时旧 decoder 还来得及上报的一帧）。 */
    if (msg.source && msg.source != (void *)g.http && msg.source != (void *)g.dec &&
        msg.source != (void *)g.sink) {
      continue;
    }

    if (msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
      audio_element_info_t info = {0};
      audio_element_getinfo((audio_element_handle_t)msg.source, &info);

      ap_music_info_t mi = {.sample_rate = info.sample_rates,
                            .channels = info.channels,
                            .bits = info.bits};

      ESP_LOGI(TAG, "music info: sr=%d ch=%d bits=%d byte_pos=%" PRIi64,
               info.sample_rates, info.channels, info.bits,
               (int64_t)info.byte_pos);

      /* Only pause/set/resume I2S if sample rate actually changed.
       * Avoid unnecessary operations that can interfere with ap_resume() verify. */
      int cur_fs = audio_i2s_get_sample_rate();
      if (info.sample_rates > 0 && info.sample_rates != cur_fs) {
        audio_i2s_pause();
        audio_i2s_set_sample_rate(info.sample_rates);
      }
      /* Important: always ensure I2S is enabled.
       * Otherwise, if I2S was paused earlier (e.g. after TTS/pause) and the sample
       * rate stays the same, we may never call resume and music becomes silent. */
      if (!audio_i2s_resume()) {
        ESP_LOGW(TAG, "i2s_resume failed (cur_fs=%d sr=%d)", cur_fs, info.sample_rates);
      }
      _emit(AP_EVT_INFO, &mi);

      /* Only promote to PLAYING if not currently PAUSED.
       * If user paused, we should respect that state. */
      if (g.state != AP_STATE_PAUSED) {
        g.state = AP_STATE_PLAYING;
        _emit(AP_EVT_STATE, &g.state);
      } else {
        ESP_LOGI(TAG, "promote PLAYING via decoder info: sr=%d ch=%d bits=%d",
                 info.sample_rates, info.channels, info.bits);
      }

      if (info.byte_pos > 0 && g_data_start_byte == 0) {
        g_data_start_byte = info.byte_pos;
      }

      if (info.sample_rates && info.channels && info.bits) {
        uint64_t bytes_per_sec =
            (uint64_t)info.sample_rates * info.channels * info.bits / 8ULL;
        if (bytes_per_sec >= 1000ULL) {
          g_sample_bytes_per_ms = (uint32_t)(bytes_per_sec / 1000ULL);
          if (g_sample_bytes_per_ms == 0)
            g_sample_bytes_per_ms = 1;
          if (g_bytes_per_ms == 0) {
            g_bytes_per_ms = g_sample_bytes_per_ms;
          }
        }
      }

      if (g.http) {
        audio_element_info_t http_info = {0};
        audio_element_getinfo(g.http, &http_info);
        if (http_info.total_bytes > 0) {
          if (http_info.total_bytes > g_last_total_bytes) {
            g_last_total_bytes = http_info.total_bytes;
            ESP_LOGD(TAG, "http total bytes=%" PRIi64,
                     (int64_t)g_last_total_bytes);
          }
        }
      }

      /* 对于本地文件源或其它非 HTTP 场景，部分解码器会在 MUSIC_INFO 中通过
       * total_bytes 提供整段大小，这里也利用该字段参与后续时长估算，
       * 以便 SD 卡等场景下能够得到非零的 duration。*/
      if (info.total_bytes > 0 && info.total_bytes > g_last_total_bytes) {
        g_last_total_bytes = info.total_bytes;
        ESP_LOGD(TAG, "decoder total bytes=%" PRIi64,
                 (int64_t)g_last_total_bytes);
      }

      /* 若解码器提供了 duration（秒），优先采用为“权威时长”，
       * 避免完全依赖总字节数 + 采样率的粗略估算。*/
      if (info.duration > 0 && g_current_duration_ms == 0) {
        uint64_t ms = (uint64_t)info.duration * 1000ULL;
        if (ms > 0 && ms < UINT32_MAX) {
          g_current_duration_ms = (uint32_t)ms;
        }
      }

      /* 计算 bytes/ms 与必要时的 duration 估算（当解码器未提供 duration 时） */
      uint32_t old_dur = g_current_duration_ms;
      recalc_bytes_per_ms();
      if (old_dur == 0 && g_current_duration_ms > 0) {
        /* UI 若未提供时，告知一次以刷新总时长/进度条 */
        _emit(AP_EVT_INFO, &mi);
      }

      if (g_pending_seek_ms != UINT32_MAX && g_bytes_per_ms > 0) {
        uint32_t pend = g_pending_seek_ms;
        g_pending_seek_ms = UINT32_MAX;
        (void)ap_seek_to_ms(pend);
      }

    } else if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
      const int st = (int)msg.data;
      if (st == AEL_STATUS_INPUT_DONE) {
        ESP_LOGI(TAG, "stream finished");
        /* INPUT_DONE 是源端 EOF；此时下游(dec/sink)可能仍在 drain ringbuf。
         * 由于 _pipeline_cleanup() 现在会进行 unlink(释放 ringbuf)，这里必须先 stop/wait，
         * 确保所有元素线程退出 RUNNING 再做 cleanup。 */
        pl_lock();
        bool stopped = _pipeline_stop_running();
        if (!stopped) {
          pl_unlock();
          ESP_LOGE(TAG, "FATAL: stop failed on EOF, restarting to avoid ringbuf UAF");
          esp_restart();
          return; /* should not reach */
        }
        _pipeline_cleanup();
        pl_unlock();
        g_last_stop_was_eof = true;
        g.state = AP_STATE_STOPPED;
        _emit(AP_EVT_STATE, &g.state);
        _emit(AP_EVT_FINISHED, NULL);
        continue;
      } else if (st == AEL_STATUS_ERROR_OPEN || st == AEL_STATUS_ERROR_INPUT) {
        /* 若首次打开 HTTPS 失败，自动回退到 HTTP 再试一次 */
        if (!g_http_fallback_tried && g_current_url && strncasecmp(g_current_url, "https://", 8) == 0) {
          size_t cap = strlen(g_current_url) + 1;
          char *fallback = (char *)malloc(cap);
          if (fallback) {
            strcpy(fallback, "http");
            strcat(fallback, g_current_url + 5);
            ESP_LOGW(TAG, "open failed -> fallback HTTPS->HTTP: %s", fallback);
            g_http_fallback_tried = true;
            (void)ap_play_url(fallback);
            free(fallback);
            continue; /* 交由新的播放流程 */
          }
        }
        /* 如果不是打开失败而是读流出错，或已回退过，重试当前 URL 一次（防抖）。*/
        if (!g_auto_retry_tried && g_current_url) {
          g_auto_retry_tried = true;
          ESP_LOGW(TAG, "stream error -> auto retry once: %s", g_current_url);
          (void)ap_play_url(g_current_url);
          continue;
        }
        _emit(AP_EVT_ERROR, "Stream error");
        g_last_stop_was_eof = false;
        pl_lock();
        bool stopped = _pipeline_stop_running();
        if (!stopped) {
          pl_unlock();
          ESP_LOGE(TAG, "FATAL: stop failed on stream error, restarting to avoid ringbuf UAF");
          esp_restart();
          return; /* should not reach */
        }
        _pipeline_cleanup();
        pl_unlock();
        g.state = AP_STATE_STOPPED;
        _emit(AP_EVT_STATE, &g.state);
        continue;
      }
    }

    /* 处理完一帧消息后主动让出 CPU，避免 audio_evt 长时间占用导致 IDLE 喂狗超时
     */
    (void)maybe_promote_loading_to_playing();
    taskYIELD();
  }
}

/* 将静态缓冲的释放实现隐藏在同一编译单元，供 free_evt_task_mem() 调用 */
/* 不再需要在任务退出时释放音频事件线程的 TCB/栈（常驻内存） */

bool ap_init(ap_event_cb_t cb, void *user) {
  /* 若已经完成初始化（回调/事件线程就绪），则幂等返回。*/
  if (g.cb || g_evt_task)
    return true;

  /* 创建管线互斥，仅保护后续播放阶段的 pipeline 关键区。 */
  if (!g_pl_mtx) {
    g_pl_mtx = xSemaphoreCreateMutex();
    if (!g_pl_mtx) {
      ESP_LOGE(TAG, "create pipeline mutex failed");
      return false;
    }
  }

  /* 初始化 pipeline（仅一次），并预创建 I2S sink，保持与 ADF 典型用法一致。 */
  if (!g.pipeline) {
    audio_pipeline_cfg_t pcfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    g.pipeline = audio_pipeline_init(&pcfg);
    AUDIO_NULL_CHECK(TAG, g.pipeline, return false);
  }

#if CONFIG_APP_MUSIC_SKIP_CN_CHECK
  ESP_LOGW(TAG, "music HTTPS common-name check skipped "
                "(CONFIG_APP_MUSIC_SKIP_CN_CHECK=y)");
#endif
  if (!ensure_sink_element()) {
    return false;
  }

  /* 创建本地事件队列：element callback -> queue -> audio_evt_task */
  if (!g_evt_q) {
    g_evt_q = xQueueCreate(64, sizeof(ap_evt_msg_t));
    if (!g_evt_q) {
      ESP_LOGE(TAG, "create evt queue failed");
      return false;
    }
  } else {
    evtq_drain();
  }

  /* 保存回调与初始状态。*/
  g.cb = cb;
  g.user = user;
  g.state = AP_STATE_IDLE;

  /* 初始化 I2S 输出（若未就绪），为后续播放做好准备。*/
  if (!audio_i2s_is_ready()) {
    audio_i2s_init(BOARD_AUDIO_SAMPLE_RATE_DEFAULT);
  }

#if CONFIG_APP_MUSIC_SKIP_CN_CHECK
  ESP_LOGW(TAG, "music HTTPS common-name check skipped "
                "(CONFIG_APP_MUSIC_SKIP_CN_CHECK=y)");
#endif

  // 为避免 I2S 短写日志在默认 INFO 级别刷屏导致 UART 堵塞（进而触发 WDT），
  // 将 i2s_sink 的日志级别钳到 ERROR（关键问题仍可见）。
  esp_log_level_set("i2s_sink",     ESP_LOG_ERROR);
  esp_log_level_set("tts_player",   ESP_LOG_WARN);
  // 调试阶段：保留 audio_player 的 INFO 级别，便于查看来源类型与 URL。
  esp_log_level_set("audio_player", ESP_LOG_INFO);
  esp_log_level_set("HTTP_STREAM",  ESP_LOG_WARN);
  esp_log_level_set("MP3_DECODER",  ESP_LOG_WARN);
  esp_log_level_set("WAV_DECODER",  ESP_LOG_WARN);

  /* 默认音量使用记忆值或 Kconfig */
  ap_set_volume(g_initial_volume);

  // 打印实际生效的音量，避免被 g_initial_volume 的历史值误导（降到 DEBUG）
  ESP_LOGD(TAG, "audio_player init OK (http_buf=%d, rb=%d, vol=%d%%)",
           CONFIG_AUDIO_PLAYER_HTTP_BUFSZ, CONFIG_AUDIO_PLAYER_RINGBUF_SZ,
           ap_get_volume());
  /* 常驻创建事件线程（Core1），监听来自各 element 的事件。 */
  if (!g_evt_task) {
    const size_t stack_bytes = 6144;
    if (!s_evt_stack_static) {
      s_evt_stack_words = stack_bytes / sizeof(StackType_t);
      s_evt_stack_static = (StackType_t *)heap_caps_malloc(stack_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!s_evt_stack_static) {
        ESP_LOGE(TAG, "alloc audio_evt stack failed");
        return false;
      }
    }
    g_evt_task = xTaskCreateStaticPinnedToCore(
        audio_evt_task, "audio_evt",
        s_evt_stack_words, NULL, 6,
        s_evt_stack_static, &s_evt_tcb_static, 1);
    if (!g_evt_task) {
      ESP_LOGE(TAG, "create audio_evt task failed");
      return false;
    }
  }
  return true;
}

/* 删除：事件线程常驻，无需等待退出 */

bool ap_deinit(void) {
  if (!g.pipeline)
    return true;

  /* 常驻事件线程不退出；仅停止并销毁当前管线 */
  ap_hard_reset_pipeline();

  if (g_current_url) {
    free(g_current_url);
    g_current_url = NULL;
  }
  g_current_duration_ms = 0;
  g_next_start_byte = -1;
  g_last_total_bytes = 0;
  g_bytes_per_ms = 0;
  g_data_start_byte = 0;
  g_sample_bytes_per_ms = 0;
  g_pending_seek_ms = UINT32_MAX;

  audio_i2s_deinit();
  g.state = AP_STATE_IDLE;
  g.cb = NULL;
  g.user = NULL;
  return true;
}

bool ap_play_url(const char *url) {
  if (!url || !url[0])
    return false;

  char *new_url = strdup(url);
  if (!new_url) {
    ESP_LOGE(TAG, "strdup url failed");
    return false;
  }

  if (g.state == AP_STATE_PLAYING || g.state == AP_STATE_PAUSED ||
      g.state == AP_STATE_LOADING) {
    /* 播放过程中切歌：采用温和的 stop+cleanup，保持 pipeline 与 listener 常驻，
     * 避免在播放线程仍监听事件队列时销毁 pipeline/队列集导致死锁。 */
    (void)ap_stop();
  }

  /* 若尚未创建管线（异常情况下的兜底），新建一条全新的 pipeline。
   * 正常情况下 g.pipeline 已在 ap_init 中完成初始化。*/
  if (!g.pipeline) {
    audio_pipeline_cfg_t pcfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    g.pipeline = audio_pipeline_init(&pcfg);
    AUDIO_NULL_CHECK(TAG, g.pipeline, {
      free(new_url);
      return false;
    });
  }

  /* 确保 I2S 端口就绪：若未 ready，则重新初始化到板卡默认采样率。
   * 这样在切换 HTTP <-> SD 卡后也能保证 I2S 始终处于可写状态。*/
  if (!audio_i2s_is_ready()) {
    if (!audio_i2s_init(BOARD_AUDIO_SAMPLE_RATE_DEFAULT)) {
      ESP_LOGE(TAG, "audio_i2s_init failed");
      free(new_url);
      return false;
    }
  }

  /* 在可能经历了一次 stop/重建之后，再确保源元素 / sink 元素存在且状态正常。*/
  if (!ensure_src_element(url)) {
    free(new_url);
    return false;
  }
  if (!ensure_sink_element()) {
    free(new_url);
    return false;
  }

  g_last_total_bytes = 0;
  /*
   * 从此处开始，进入“管线配置关键区”：注册/链接元素、设置 listener、
   * 设置 URI 以及启动 pipeline。这样可避免事件线程在窗口期抢占并发调用
   * _pipeline_cleanup()/unregister 导致的竞态（之前曾出现 RUN 过程中被
   * 清理，触发 audio_element_get_state 对悬空指针访问）。
   */
  pl_lock();
  const char *src_tag = g.http ? audio_element_get_tag(g.http) : "(null)";
  ESP_LOGI(TAG, "ap_play_url: src_tag=%s url=%s",
           src_tag ? src_tag : "(null)", url ? url : "(null)");
  g.dec = _create_decoder_by_ext(url);
  if (!g.dec) {
    _emit(AP_EVT_ERROR, "No decoder for this URL");
    g.state = AP_STATE_ERROR;
    free(new_url);
    pl_unlock();
    return false;
  }
  attach_element_event_cb(g.dec);

  esp_err_t reg_ret = audio_pipeline_register(g.pipeline, g.http, "http");
  if (reg_ret != ESP_OK) {
    ESP_LOGE(TAG, "register http failed: %s", esp_err_to_name(reg_ret));
    audio_element_deinit(g.dec);
    g.dec = NULL;
    g_dec_ext[0] = '\0';
    free(new_url);
    return false;
  }
  g_http_registered = true;

  reg_ret = audio_pipeline_register(g.pipeline, g.dec, "dec");
  if (reg_ret != ESP_OK) {
    ESP_LOGE(TAG, "register decoder failed: %s", esp_err_to_name(reg_ret));
    audio_pipeline_unregister(g.pipeline, g.http);
    g_http_registered = false;
    audio_element_deinit(g.dec);
    g.dec = NULL;
    g_dec_ext[0] = '\0';
    free(new_url);
    return false;
  }
  g_dec_registered = true;

  reg_ret = audio_pipeline_register(g.pipeline, g.sink, "sink");
  if (reg_ret != ESP_OK) {
    ESP_LOGE(TAG, "register sink failed: %s", esp_err_to_name(reg_ret));
    audio_pipeline_unregister(g.pipeline, g.dec);
    g_dec_registered = false;
    audio_pipeline_unregister(g.pipeline, g.http);
    g_http_registered = false;
    audio_element_deinit(g.dec);
    g.dec = NULL;
    g_dec_ext[0] = '\0';
    free(new_url);
    return false;
  }
  g_sink_registered = true;

  const char *link[3] = {"http", "dec", "sink"};
  esp_err_t link_ret = audio_pipeline_link(g.pipeline, link, 3);
  if (link_ret != ESP_OK) {
    ESP_LOGE(TAG, "pipeline link failed: %s", esp_err_to_name(link_ret));
    _pipeline_cleanup();
    free(new_url);
    pl_unlock();
    return false;
  }
  /* 在开始新一轮运行前，丢弃上一轮残留事件（切歌时旧元素可能还来得及上报一帧）。 */
  evtq_drain();

  /* HTTPS 优先：默认直接使用传入的 URL，不再开发期强制改为 HTTP */
  g_http_fallback_tried = false;
  g_auto_retry_tried = false;
  audio_element_set_uri(g.http, url);

  /* 提前记录当前 URL，确保后续立即上报 PLAYING 时，外部通过 ap_get_current_url()
     能拿到本次的最终 URL（避免出现 url_resolved:(null) 或上一曲 URL）。*/
  if (g_current_url) {
    free(g_current_url);
    g_current_url = NULL;
  }
  g_current_url = new_url;
  new_url = NULL; /* 移交所有权给全局 g_current_url */

  /* 在收到 MUSIC_INFO 前，预先把 I2S 恢复到板卡默认采样率；
     仅做时钟重配，不强制 pause/resume，避免驱动状态机被打断导致 resume 失败。 */
  (void)audio_i2s_set_sample_rate(BOARD_AUDIO_SAMPLE_RATE_DEFAULT);
  /* Even if reconfig fails and leaves the channel disabled, keep the output enabled
   * so the sink won't silently drop samples. */
  (void)audio_i2s_resume();

  /* 重要：在 pipeline_run 之前设置 byte_pos！
   * _file_open() / http_stream_open() 会在 pipeline_run 期间被调用，
   * 需要此时就能读取到正确的 byte_pos 以执行 seek。
   *
   * 对于文件源，额外使用专用 API 传递 seek 目标，
   * 因为 audio_element_set_byte_pos 可能存在时序问题。 */
  if (g_next_start_byte > 0) {
    int target =
        (g_next_start_byte > INT_MAX) ? INT_MAX : (int)g_next_start_byte;
    /* 本地 WAV seek：file_src_element 会“先回放 WAV 头，再 seek 到目标 PCM 位置”，
     * 因此解码器必须从 byte_pos=0 打开，否则 WAV_DECODER 会走“中途恢复”分支并报
     * "The reserve data is null"（缺少保留头部数据）。 */
    const bool is_wav = (strcasecmp(g_dec_ext, "wav") == 0);
    const bool local_wav_seek = (is_wav && g_src_is_file);
    audio_element_set_byte_pos(g.http, target);
    if (g.dec) {
      audio_element_set_byte_pos(g.dec, local_wav_seek ? 0 : target);
    }
    /* 文件源专用 seek API */
    if (g_src_is_file) {
      file_src_set_seek_position(g_next_start_byte);
    }
  } else {
    audio_element_set_byte_pos(g.http, 0);
    if (g.dec) {
      audio_element_set_byte_pos(g.dec, 0);
    }
    /* 清除文件源的 seek 请求 */
    if (g_src_is_file) {
      file_src_set_seek_position(-1);
    }
  }
  g_next_start_byte = -1;

  g.state = AP_STATE_LOADING;
  _emit(AP_EVT_STATE, &g.state);

  esp_err_t run_ret = audio_pipeline_run(g.pipeline);
  if (run_ret != ESP_OK) {
    ESP_LOGE(TAG, "pipeline run failed: %s", esp_err_to_name(run_ret));
    bool stopped = _pipeline_stop_running();
    if (!stopped) {
      pl_unlock();
      ESP_LOGE(TAG, "FATAL: stop failed after run failure, restarting to avoid ringbuf UAF");
      esp_restart();
      return false; /* should not reach */
    }
    _pipeline_cleanup();
    g.state = AP_STATE_ERROR;
    _emit(AP_EVT_STATE, &g.state);
    free(new_url);
    pl_unlock();
    return false;
  }

  g_pipeline_active = true;

  g_stop_requested = false;

  /* 提前把状态切为 PLAYING：
   * - 真实播放很快就会开始（特别是本地/WAV），UI应立即进入"暂停(Ⅱ)+进度走动"。
   * - 若随后发生错误/停止，事件线程会发 STOPPED/ERROR 覆盖 UI。*/
  if (g.state == AP_STATE_LOADING) {
    g.state = AP_STATE_PLAYING;
    _emit(AP_EVT_STATE, &g.state);
    ESP_LOGI(TAG, "promote PLAYING immediately after pipeline_run");
  }

  pl_unlock();
  return true;
}

bool ap_stop(void) {
  if (!g.pipeline)
    return false;

  g_stop_requested = true;
  pl_lock();
  bool stopped = _pipeline_stop_running();
  if (!stopped) {
    pl_unlock();
    ESP_LOGE(TAG, "FATAL: stop failed, restarting to avoid ringbuf UAF");
    esp_restart();
    return false; /* should not reach */
  }
  _pipeline_cleanup();
  pl_unlock();
  g_stop_requested = false;
  g.state = AP_STATE_STOPPED;
  _emit(AP_EVT_STATE, &g.state);
  return stopped;
}

/* 静态任务的 TCB/栈为常驻内存（TCB 内部 RAM、栈 PSRAM），不在任务退出时释放，避免反复分配 */

bool ap_pause(void) {
  if (!g.pipeline || g.state != AP_STATE_PLAYING)
    return false;

  audio_element_pause(g.http);
  if (g.dec)
    audio_element_pause(g.dec);
  audio_element_pause(g.sink);

  if (!audio_i2s_pause()) {
    ESP_LOGW(TAG, "i2s pause failed");
  }

  g.state = AP_STATE_PAUSED;
  _emit(AP_EVT_STATE, &g.state);
  return true;
}

bool ap_resume(void) {
  if (!g.pipeline || g.state != AP_STATE_PAUSED)
    return false;

  /* Debug: capture resume behavior (return codes + actual element states).
   * Keep logs during investigation; trim once stable. */
  audio_element_state_t pre_http = g.http ? audio_element_get_state(g.http) : AEL_STATE_NONE;
  audio_element_state_t pre_dec  = g.dec ? audio_element_get_state(g.dec) : AEL_STATE_NONE;
  audio_element_state_t pre_sink = g.sink ? audio_element_get_state(g.sink) : AEL_STATE_NONE;
  ESP_LOGI(TAG, "[resume] pre: ap_state=%d http=%d dec=%d sink=%d i2s_ready=%d",
           (int)g.state, (int)pre_http, (int)pre_dec, (int)pre_sink,
           (int)audio_i2s_is_ready());

  /* 1) Resume I2S first (un-gate writes). */
  bool i2s_ok = audio_i2s_resume();
  if (!i2s_ok) {
    ESP_LOGW(TAG, "[resume] i2s_resume failed");
  }

  /* 2) Ask ADF elements to resume with a bounded timeout.
   * Note: audio_element_resume() waits for RESUMED_BIT, which is set before open() completes,
   * so ESP_OK does not guarantee the element is already RUNNING. We must verify states below. */
  const TickType_t tmo = pdMS_TO_TICKS(200);
  esp_err_t r_http = g.http ? audio_element_resume(g.http, 0, tmo) : ESP_FAIL;
  esp_err_t r_dec  = g.dec ? audio_element_resume(g.dec, 0, tmo) : ESP_OK;
  esp_err_t r_sink = g.sink ? audio_element_resume(g.sink, 0, tmo) : ESP_FAIL;
  ESP_LOGI(TAG, "[resume] rc: i2s=%d http=%s dec=%s sink=%s", (int)i2s_ok,
           esp_err_to_name(r_http), esp_err_to_name(r_dec), esp_err_to_name(r_sink));

  /* 3) Verify: wait a short window for all elements to reach RUNNING and I2S be ready.
   * If verification fails, fallback to a controlled restart. */
  bool verified = false;
  audio_element_state_t st_http = AEL_STATE_NONE;
  audio_element_state_t st_dec  = AEL_STATE_NONE;
  audio_element_state_t st_sink = AEL_STATE_NONE;
  for (int i = 0; i < 10; i++) { /* ~300ms */
    vTaskDelay(pdMS_TO_TICKS(30));
    st_http = g.http ? audio_element_get_state(g.http) : AEL_STATE_NONE;
    st_dec  = g.dec ? audio_element_get_state(g.dec) : AEL_STATE_NONE;
    st_sink = g.sink ? audio_element_get_state(g.sink) : AEL_STATE_NONE;

    /* Any terminal state -> stop early. */
    if (st_http == AEL_STATE_ERROR || st_http == AEL_STATE_FINISHED ||
        st_dec  == AEL_STATE_ERROR || st_dec  == AEL_STATE_FINISHED ||
        st_sink == AEL_STATE_ERROR || st_sink == AEL_STATE_FINISHED) {
      break;
    }

    /* Normal running check. Allow source to be INITIALIZING briefly. */
    bool http_ok = (st_http == AEL_STATE_RUNNING || st_http == AEL_STATE_INITIALIZING);
    bool dec_ok  = (!g.dec) || (st_dec == AEL_STATE_RUNNING);
    bool sink_ok = (st_sink == AEL_STATE_RUNNING);

    /* Retry I2S resume if not ready — concurrent MUSIC_INFO may have toggled it */
    if (!audio_i2s_is_ready()) {
      audio_i2s_resume();
    }

    if (http_ok && dec_ok && sink_ok && audio_i2s_is_ready()) {
      verified = true;
      break;
    }
  }

  ESP_LOGI(TAG, "[resume] verify=%d states: http=%d dec=%d sink=%d i2s_ready=%d",
           (int)verified, (int)st_http, (int)st_dec, (int)st_sink,
           (int)audio_i2s_is_ready());

  if (verified) {
    g.state = AP_STATE_PLAYING;
    _emit(AP_EVT_STATE, &g.state);
    return true;
  }

  ESP_LOGW(TAG, "[resume] verify failed -> fallback restart (http=%d dec=%d sink=%d)",
           (int)st_http, (int)st_dec, (int)st_sink);

  /* Best-effort: restart from current byte_pos if available.
   * WAV 文件 seek 处理：
   * - SD 卡本地文件：支持 seek（file_src_element 实现两阶段读取）
   * - HTTP 流：不支持，从头开始 */
  int64_t byte_pos = -1;
  bool is_wav = (strcasecmp(g_dec_ext, "wav") == 0);
  bool is_local = is_sdcard_url(g_current_url);
  bool can_seek = !is_wav || is_local;  /* 非 WAV 或本地 WAV 可以 seek */

  if (can_seek) {
    audio_element_info_t info = {0};
    if (is_wav && is_local) {
      /* 本地 WAV：优先用 file_src 的 byte_pos（它跟踪的是“文件绝对位置”，seek 后更可靠）。 */
      if (g.http) {
        audio_element_getinfo(g.http, &info);
        byte_pos = info.byte_pos;
      }
      if (byte_pos <= 0 && g.dec) {
        memset(&info, 0, sizeof(info));
        audio_element_getinfo(g.dec, &info);
        byte_pos = info.byte_pos;
      }
    } else {
      if (g.dec) {
        audio_element_getinfo(g.dec, &info);
        byte_pos = info.byte_pos;
      }
      if (byte_pos <= 0 && g.http) {
        memset(&info, 0, sizeof(info));
        audio_element_getinfo(g.http, &info);
        byte_pos = info.byte_pos;
      }
    }
  }

  if (g_current_url && g_current_url[0]) {
    if (byte_pos > 0 && can_seek) {
      return restart_pipeline_at(byte_pos);
    }
    /* HTTP WAV or no valid byte_pos: restart from beginning */
    if (is_wav && !is_local) {
      ESP_LOGI(TAG, "[resume] HTTP WAV cannot seek, restart from beginning");
    }
    return ap_play_url(g_current_url);
  }

  return false;
}

void ap_set_initial_volume(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  g_initial_volume = percent;
  if (g.pipeline) {
    ap_set_volume(g_initial_volume);
  }
}

void ap_set_track_duration(uint32_t duration_ms) {
  ESP_LOGD(TAG, "set_track_duration=%" PRIu32, duration_ms);
  g_current_duration_ms = duration_ms;
  g_bytes_per_ms = 0;
  g_data_start_byte = 0;
  g_sample_bytes_per_ms = 0;
  g_pending_seek_ms = UINT32_MAX;
  g_last_total_bytes = 0;
}

uint32_t ap_get_track_duration(void) { return g_current_duration_ms; }

bool ap_seek_to_ms(uint32_t position_ms) {
  if (!g_current_url || g_current_duration_ms == 0) {
    ESP_LOGW(TAG, "seek ignored: no active track");
    return false;
  }

  /* WAV 文件 seek 处理：
   * - SD 卡本地文件：支持 seek（file_src_element 实现两阶段读取）
   * - HTTP 流：不支持（需要两次 HTTP 请求，复杂度高）
   */
  bool is_wav = (strcasecmp(g_dec_ext, "wav") == 0);
  bool is_local = is_sdcard_url(g_current_url);
  if (is_wav && !is_local) {
    ESP_LOGW(TAG, "seek rejected: HTTP WAV files do not support seeking");
    return false;
  }

  if (position_ms >= g_current_duration_ms) {
    position_ms = (g_current_duration_ms > 0) ? (g_current_duration_ms - 1) : 0;
  }

  if (g_bytes_per_ms == 0) {
    ESP_LOGW(TAG, "seek deferred: bytes_per_ms unknown (%u ms)",
             (unsigned)position_ms);
    g_pending_seek_ms = position_ms;
    return true;
  }

  if (g_last_total_bytes <= 0 && g.http) {
    audio_element_info_t info = {0};
    audio_element_getinfo(g.http, &info);
    if (info.total_bytes > 0) {
      g_last_total_bytes = info.total_bytes;
      ESP_LOGI(TAG, "update total bytes via http info: %" PRIi64,
               (int64_t)g_last_total_bytes);
    }
  }

  int64_t target = g_data_start_byte + (int64_t)g_bytes_per_ms * position_ms;
  if (target < g_data_start_byte)
    target = g_data_start_byte;
  if (g_last_total_bytes > 0 && target >= g_last_total_bytes) {
    target = g_last_total_bytes - 1;
  }

  ESP_LOGI(TAG, "seek to %u ms -> byte %" PRIi64 " (wav=%d local=%d)",
           (unsigned)position_ms, target, (int)is_wav, (int)is_local);

  g_pending_seek_ms = UINT32_MAX;
  return restart_pipeline_at(target);
}

void ap_set_volume(int vol) {
  if (vol < 0)
    vol = 0;
  if (vol > 100)
    vol = 100;
  audio_i2s_set_volume_percent(vol);
}

int ap_get_volume(void) { return audio_i2s_get_volume_percent(); }

ap_state_t ap_get_state(void) { return g.state; }

const char *ap_get_current_url(void) { return g_current_url; }
