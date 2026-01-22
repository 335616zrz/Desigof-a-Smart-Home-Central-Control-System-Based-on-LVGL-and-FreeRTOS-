#include "tts_player.h"
#include "audio_i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_heap_caps.h"
#include "audio_thread.h"
#include <string.h>

static const char *TAG = "tts_player";

typedef struct {
  uint8_t *data; // mono PCM16 LE bytes
  size_t   len;  // bytes
} tts_chunk_t;

static TaskHandle_t   s_task   = NULL;
static QueueHandle_t  s_queue  = NULL;
static volatile bool  s_finish = false;  // 请求优雅结束（播放完队列再退出）
static volatile uint32_t s_pending_bytes_mono = 0; // 尚未播放完的“单声道PCM字节数”
static int            s_fs     = 0;

static void tts_task(void *arg)
{
  (void)arg;
  const size_t tmp_cap = 4096; // samples per channel，增大块以提高吞吐
  int16_t *st = (int16_t *)heap_caps_malloc(tmp_cap * 2 * sizeof(int16_t), MALLOC_CAP_DEFAULT);
  if (!st) {
    ESP_LOGE(TAG, "alloc stereo buf failed");
  }

  while (1) {
    tts_chunk_t chunk;
    if (xQueueReceive(s_queue, &chunk, pdMS_TO_TICKS(200)) != pdTRUE) {
      // 队列暂时为空；若请求结束，则退出；否则继续等待
      if (s_finish) break;
      continue;
    }
    if (!chunk.data || chunk.len == 0) {
      free(chunk.data);
      continue;
    }
    // Convert mono PCM16 (LE) to stereo interleaved
    size_t samples = chunk.len / 2; // int16 samples（单声道）
    const int16_t *mono = (const int16_t *)chunk.data;
    size_t off = 0;
    while (off < samples) {
      size_t blk = samples - off;
      if (blk > tmp_cap) blk = tmp_cap;
      for (size_t i = 0; i < blk; ++i) {
        int16_t s = mono[off + i];
        st[(i << 1) + 0] = s;
        st[(i << 1) + 1] = s;
      }
      size_t w = audio_i2s_write(st, blk); // 实际写入的帧数
      if (w == 0) {
        // 极端情况下避免忙等
        vTaskDelay(pdMS_TO_TICKS(2));
      } else {
        // 每写入 w 帧（立体声），消费 w 个单声道采样，即 2*w 字节单声道数据的时长
        uint32_t sub = (uint32_t)w * 2;
        uint32_t cur = s_pending_bytes_mono;
        if (cur >= sub) s_pending_bytes_mono = cur - sub; else s_pending_bytes_mono = 0;
      }
      off += w; // 按实际写入推进
      // 主动让出一次调度，避免本任务长时间占用 Core1 导致 IDLE1 喂狗不及时
      taskYIELD();
    }
    free(chunk.data);
  }

  if (st) free(st);
  s_task = NULL; // 标记退出完成
  vTaskDelete(NULL);
}

bool tts_player_begin(int sample_rate_hz)
{
  if (s_task) return true;
  s_finish = false;
  s_pending_bytes_mono = 0;
  bool ready = audio_i2s_is_ready();
  ESP_LOGI(TAG, "begin: req_fs=%d i2s_ready=%d", sample_rate_hz, (int)ready);
  if (!ready) {
    bool inited = audio_i2s_init(sample_rate_hz);
    ESP_LOGI(TAG, "i2s_init rc=%d", (int)inited);
    if (!inited) {
      ESP_LOGE(TAG, "i2s init failed");
      return false;
    }
  }
  bool p = audio_i2s_pause();
  ESP_LOGI(TAG, "i2s_pause rc=%d", (int)p);
  bool set = audio_i2s_set_sample_rate(sample_rate_hz);
  ESP_LOGI(TAG, "i2s_set_fs rc=%d", (int)set);
  bool r = audio_i2s_resume();
  ESP_LOGI(TAG, "i2s_resume rc=%d", (int)r);
  s_fs = sample_rate_hz;
  if (!s_queue) {
    // 队列增大，避免服务端突发分片导致丢帧；每项仅保存指针与长度
    s_queue = xQueueCreate(64, sizeof(tts_chunk_t));
    if (!s_queue) {
      ESP_LOGE(TAG, "queue create failed");
      return false;
    }
  }
  // 将 TTS 任务迁至 Core0，避免与解码/I2S（Core1）争用造成 IDLE1 长时间无空隙
  esp_err_t rc = audio_thread_create((audio_thread_t *)&s_task,
                                     "tts_task",
                                     tts_task,
                                     NULL,
                                     4096,
                                     11,  // keep audio ahead of heavy UI redraw (LVGL is prio=10)
                                     true,  /* stack in PSRAM when possible */
                                     0);
  if (rc != ESP_OK) {
    s_task = NULL;
    ESP_LOGE(TAG, "create task failed");
    return false;
  }
  ESP_LOGI(TAG, "begin fs=%d vol=%d%%", s_fs, audio_i2s_get_volume_percent());
  return true;
}

bool tts_player_feed(const uint8_t *pcm_mono_le, size_t nbytes)
{
  if (!s_task || !s_queue || !pcm_mono_le || nbytes == 0) return false;
  tts_chunk_t m = {0};
  m.data = (uint8_t *)heap_caps_malloc(nbytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!m.data) return false;
  memcpy(m.data, pcm_mono_le, nbytes);
  m.len = nbytes;
  // 喂入前先累加待播“单声道字节数”
  s_pending_bytes_mono += (uint32_t)nbytes;
  // 尽量等待队列有空位，避免丢帧；上限约 5000ms
  if (xQueueSend(s_queue, &m, pdMS_TO_TICKS(5000)) != pdTRUE) {
    free(m.data);
    // 回退累加
    if (s_pending_bytes_mono >= nbytes) s_pending_bytes_mono -= (uint32_t)nbytes; else s_pending_bytes_mono = 0;
    ESP_LOGW(TAG, "feed queue full (drop %uB)", (unsigned)nbytes);
    return false;
  }
  return true;
}

void tts_player_end(void)
{
  if (!s_task) return;
  // 请求优雅结束：让播放线程把队列数据播完再退出
  s_finish = true;
  const uint32_t hard_cap_ms = 30000; // 30s 上限
  uint32_t waited = 0;
  bool drained_clean = false;
  while (s_task) {
    // 估算剩余播放时长（ms）：pending_mono_bytes / (2 bytes/sample) / fs * 1000
    uint32_t pending = s_pending_bytes_mono;
    uint32_t est_ms = (s_fs > 0) ? (uint32_t)((pending * 500U) / (uint32_t)s_fs) : 0; // (pending/2)*1000/fs == pending*500/fs
    uint32_t step = (est_ms > 50) ? 20 : 10; // 短时收敛得更快
    vTaskDelay(pdMS_TO_TICKS(step));
    waited += step;
    if (pending == 0 && uxQueueMessagesWaiting(s_queue) == 0) { drained_clean = true; break; } // 已经播空
    if (waited >= hard_cap_ms) break; // 安全上限
  }
  if (s_task) {
    /* 若队列已空且 pending==0，很可能播放线程正处于 xQueueReceive(200ms) 的等待窗口。
       给予一个短暂宽限等待其自然退出，避免出现“pending=0 仍打印 force drain”的噪声日志。 */
    if (drained_clean) {
      const uint32_t grace_ms = 300;
      uint32_t left = grace_ms;
      while (s_task && left > 0) { vTaskDelay(pdMS_TO_TICKS(10)); left -= 10; }
    }
    if (s_task) {
      // 仍未退出，强制排空队列以避免卡住
      ESP_LOGW(TAG, "end timeout, force drain (pending=%uB)", (unsigned)s_pending_bytes_mono);
      uint32_t dropped = 0;
      while (uxQueueMessagesWaiting(s_queue) > 0) {
        tts_chunk_t c;
        if (xQueueReceive(s_queue, &c, 0) == pdTRUE) { free(c.data); dropped++; }
        else break;
      }
      ESP_LOGI(TAG, "end fs=%d dropped=%u", s_fs, (unsigned)dropped);
    } else {
      ESP_LOGI(TAG, "end fs=%d flushed", s_fs);
    }
  } else {
    ESP_LOGI(TAG, "end fs=%d flushed", s_fs);
  }
}

bool tts_player_is_active(void) { return s_task != NULL; }

void tts_player_stop_now(void)
{
  if (!s_task) return;
  // 请求结束并清空待播队列
  s_finish = true;
  s_pending_bytes_mono = 0;
  if (s_queue) {
    while (uxQueueMessagesWaiting(s_queue) > 0) {
      tts_chunk_t c;
      if (xQueueReceive(s_queue, &c, 0) == pdTRUE) {
        free(c.data);
      } else {
        break;
      }
    }
  }
  // 给工作线程一个短暂窗口自然退出
  uint32_t left = 300;
  while (s_task && left > 0) { vTaskDelay(pdMS_TO_TICKS(10)); left -= 10; }
}
