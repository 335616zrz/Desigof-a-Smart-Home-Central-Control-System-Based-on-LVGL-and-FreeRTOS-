#include "audio_common.h"
#include "audio_element.h"
#include "audio_i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include <stdbool.h>

#include "music_spectrum.h"

static const char *TAG = "i2s_sink";
static volatile bool s_abort_requested = false;

void i2s_sink_request_abort(void) { s_abort_requested = true; }

void i2s_sink_clear_abort(void) { s_abort_requested = false; }

/* 把上游解码出来的 16-bit 立体声 PCM 写到 I2S */
static esp_err_t _process(audio_element_handle_t self, char *in, int len) {
  if (len <= 0)
    return len;

  if (s_abort_requested) {
    ESP_LOGW(TAG, "abort requested by controller");
    vTaskDelay(pdMS_TO_TICKS(10));
    return AEL_IO_ABORT;
  }

  if (audio_element_is_stopping(self)) {
    ESP_LOGW(TAG, "element stopping");
    vTaskDelay(pdMS_TO_TICKS(10));
    return AEL_IO_ABORT;
  }

  /* 在 RUNNING 前不要向上游发 ABORT，避免解码器提前退出。*/
  audio_element_state_t st = audio_element_get_state(self);
  if (st != AEL_STATE_RUNNING) {
    vTaskDelay(pdMS_TO_TICKS(20)); /* 增加 delay 减少 CPU 空转，给 IDLE 任务更多机会 */
    /* 注意：ADF 将返回值 0 视为 AEL_IO_OK，可能触发元素 FINISHED。
     * 这里用 TIMEOUT 表示"暂时不处理"，既不消费数据也不结束链路。 */
    return AEL_IO_TIMEOUT;
  }

  ringbuf_handle_t input_rb = audio_element_get_input_ringbuf(self);
  if (!input_rb) {
    vTaskDelay(pdMS_TO_TICKS(5));
    return AEL_IO_TIMEOUT; /* 等待上游就绪 */
  }

  if (!audio_i2s_is_ready()) {
    /* I2S 未就绪：不要消耗输入，给上游施加背压（否则会“读走但返回0”，导致数据丢失/无声） */
    vTaskDelay(pdMS_TO_TICKS(5));
    return AEL_IO_TIMEOUT;
  }

  const int rbytes = audio_element_input(self, in, len);
  if (rbytes <= 0)
    return rbytes;

  /* 一帧 = L(2B) + R(2B) = 4 字节 */
  const size_t frames = (size_t)rbytes / 4;
  music_spectrum_push_pcm((const int16_t *)in, frames);
  const size_t wframes = audio_i2s_write((const int16_t *)in, frames);
  const size_t wbytes = wframes * 4;

  /* 降噪：调低到 DEBUG，避免大量 UART 输出卡住 Core1 */
  if (wbytes + (rbytes >> 2) < (size_t)rbytes) {
    ESP_LOGD(TAG, "short write: in=%dB, wrote=%uB", rbytes, (unsigned)wbytes);
  }
  return rbytes; // ADF 约定：返回“已消费的输入字节”
}

static esp_err_t _open(audio_element_handle_t self) {
  (void)self;
  /* 采样率跟随 MUSIC_INFO 在上层设置；此处无需额外动作 */
  return ESP_OK;
}

static esp_err_t _close(audio_element_handle_t self) {
  (void)self;
  return ESP_OK;
}

audio_element_handle_t i2s_sink_element_create(void) {
  audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
  cfg.task_stack = 3 * 1024; /* 适中，避免堆栈报警 */
  cfg.stack_in_ext = false;  /* 内部内存：更稳的实时性 */
  cfg.task_prio = 8;         /* 提高到 8，确保在解码计算密集时优先喂 I2S，减少 WDT 风险 */
  cfg.task_core = 1;         /* 音频链路统一放在 Core1 */
  cfg.tag = "i2s_sink";
  cfg.open = _open;
  cfg.close = _close;
  cfg.process = _process;
  cfg.out_rb_size = 0;   /* sink 没有下游 */
  /* 桥接缓冲：可通过 Kconfig 调整，默认 4096B */
#ifndef CONFIG_AUDIO_PLAYER_I2S_SINK_BUF
#define CONFIG_AUDIO_PLAYER_I2S_SINK_BUF 4096
#endif
  cfg.buffer_len = CONFIG_AUDIO_PLAYER_I2S_SINK_BUF;

  audio_element_handle_t el = audio_element_init(&cfg);
  if (!el) {
    ESP_LOGE(TAG, "no mem for i2s_sink");
    return NULL;
  }
  /* 使用有限超时，避免快速切歌时无限阻塞导致 watchdog
   * 500ms 足够长以减少 CPU 空转，又足够短以响应 stop 请求 */
  audio_element_set_input_timeout(el, pdMS_TO_TICKS(500));
  return el;
}
