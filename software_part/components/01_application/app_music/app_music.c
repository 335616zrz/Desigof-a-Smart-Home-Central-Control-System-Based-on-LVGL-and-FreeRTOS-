#include "app_music.h"

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "audio_mem.h"
#include "audio_player.h"
#include "music_ui.h"
#include "music_spectrum.h"
#include "music_volume.h"
#include "music_progress.h"
#include "url_encode.h"
#include "lvgl.h"  /* 用于把进度总时长更新投递到 LVGL 线程 */
#include "lvgl_port.h"

#include <string.h>
#include <time.h>

/* 日志 TAG 提前定义，供下方回调使用 */
static const char *const TAG = "app_music";

/*===============================
 * 本组件可调参数（工程化）
 *===============================*/
#define APP_WIFI_WAIT_MS (5 * 60 * 1000)
#define APP_TIME_VALID_YEAR_MIN 2022
#define APP_URL_MAX 256

/*===============================
 * Helpers
 *===============================*/

/* 关闭 STA 省电以降低网络/音频时延抖动；若未 init 则短暂重试后退出干预。 */
static inline void wifi_ps_disable_try(void) {
  for (int i = 0; i < 30; ++i) {
    esp_err_t r = esp_wifi_set_ps(WIFI_PS_NONE);
    if (r == ESP_OK) {
      (void)esp_wifi_set_inactive_time(WIFI_IF_STA, 0);
      return;
    }
    if (r != ESP_ERR_WIFI_NOT_INIT)
      return;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/* 阻塞至系统时间有效（year>=APP_TIME_VALID_YEAR_MIN）。 */
static inline void time_wait_synced(void) {
  TickType_t last = xTaskGetTickCount();
  time_t now = 0;
  struct tm tm_info = {0};
  do {
    vTaskDelayUntil(&last, pdMS_TO_TICKS(500));
    time(&now);
    localtime_r(&now, &tm_info);
  } while (tm_info.tm_year < (APP_TIME_VALID_YEAR_MIN - 1900));
}

/* 轮询等待获取 IPv4（仅在无事件组参数时使用）。 */
static bool wait_sta_ip_poll(TickType_t timeout_ticks) {
  const TickType_t start = xTaskGetTickCount();
  esp_netif_t *sta = NULL;

  while ((xTaskGetTickCount() - start) < timeout_ticks) {
    wifi_ps_disable_try();

    if (!sta) {
      sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      if (!sta) {
        vTaskDelay(pdMS_TO_TICKS(200));
        continue;
      }
    }
    esp_netif_ip_info_t info;
    if (esp_netif_get_ip_info(sta, &info) == ESP_OK && info.ip.addr != 0) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  return false;
}

/* 当停止事件发生且距离最近一次 UI
 * 操作非常接近时，认定为手动切歌，避免自动下一曲 */
#define MUSIC_STOP_IGNORE_WINDOW_US 400000LL

/* 在 LVGL 线程设置进度条总时长 */
static void _mp_set_total_async(void *p)
{
  uint32_t v = (uint32_t)(uintptr_t)p;
  music_progress_set_total(v);
}

static void ap_evt_cb(ap_event_t evt, const void *payload, void *user) {
  (void)user;

  switch (evt) {
  case AP_EVT_INFO:
    if (payload) {
      const ap_music_info_t *info = (const ap_music_info_t *)payload;
      music_spectrum_set_format((uint32_t)info->sample_rate,
                                (uint32_t)info->channels);
    }
    /* 若 UI 目录未给出时长（或为 0），在解码器上报 INFO 后，用播放器侧
       估算时长刷新一次总时长。必须在 LVGL 线程调用 UI API，这里用
       lv_async_call 投递。*/
    do {
      extern uint32_t ap_get_track_duration(void);
      uint32_t tot = ap_get_track_duration();
      ESP_LOGI(TAG, "AP_EVT_INFO: track duration=%u ms", (unsigned)tot);
      if (tot > 0) {
        if (lvgl_port_lock(20)) {
          (void)lv_async_call(_mp_set_total_async, (void *)(uintptr_t)tot);
          lvgl_port_unlock();
        }
      }
    } while (0);
    break;
  case AP_EVT_STATE:
    ESP_LOGI(TAG, "evt: AP_STATE=%d", (int)*(const ap_state_t *)payload);
    if (!payload) {
      break;
    }
    {
      ap_state_t state = *(const ap_state_t *)payload;
      if (state == AP_STATE_PLAYING) {
        extern const char *ap_get_current_url(void);
        music_ui_notify_url_resolved(ap_get_current_url());
      }
      /* 若是 STOPPED，且刚发生过用户操作（切歌/播放）在很近的时间窗口内，
       * 认为是“过渡性”停止（用于切换曲目），不把 UI 拉回▶，避免闪烁。*/
      if (state == AP_STATE_STOPPED && music_ui_recent_action_within_us(MUSIC_STOP_IGNORE_WINDOW_US)) {
        ESP_LOGI(TAG, "evt: AP_STATE=STOPPED ignored (recent user action)");
        break;
      }
      /* 其它情况一律同步给 UI（包含 PLAYING/PAUSED/ERROR/真正的 STOPPED） */
      music_ui_dispatch_player_state(state);
    }
    break;
  case AP_EVT_ERROR:
    ESP_LOGW(TAG, "evt: AP_ERROR: %s", (const char*)payload);
    music_ui_dispatch_player_state(AP_STATE_ERROR);
    break;
  case AP_EVT_FINISHED:
    ESP_LOGI(TAG, "evt: AP_FINISHED -> auto-next");
    /* 先把 UI 切到“非播放”态，再请求下一首，避免界面残留两条竖杠 */
    music_ui_dispatch_player_state(AP_STATE_STOPPED);
    music_ui_dispatch_playback_finished(true);
    break;
  default:
    break;
  }
}

/*===============================
 * Task
 *===============================*/
void app_music_task(void *sys_evt_arg) {
  EventGroupHandle_t sys_evt = (EventGroupHandle_t)sys_evt_arg;

  wifi_ps_disable_try();

  /* 优先使用事件组等待 Wi-Fi；若无事件组则退化为轮询 */
  if (sys_evt) {
    EventBits_t bits =
        xEventGroupWaitBits(sys_evt, APP_MUSIC_EVT_WIFI_READY, pdFALSE, pdTRUE,
                            pdMS_TO_TICKS(APP_WIFI_WAIT_MS));
    if ((bits & APP_MUSIC_EVT_WIFI_READY) == 0) {
      vTaskDelete(NULL);
      return;
    }
  } else {
    if (!wait_sta_ip_poll(pdMS_TO_TICKS(APP_WIFI_WAIT_MS))) {
      vTaskDelete(NULL);
      return;
    }
  }

  wifi_ps_disable_try();
  time_wait_synced();

  music_volume_init();

  if (!ap_init(ap_evt_cb, NULL)) {
    vTaskDelete(NULL);
    return;
  }

  music_volume_on_audio_ready();

  ESP_LOGI(TAG, "music task ready, wait for UI selection");

  vTaskDelete(NULL);
}
