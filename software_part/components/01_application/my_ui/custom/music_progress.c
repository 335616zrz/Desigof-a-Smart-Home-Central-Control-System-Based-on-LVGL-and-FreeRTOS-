#include "music_progress.h"

#include "lvgl.h"
#include "esp_log.h"

#include "sdkconfig.h"

#if CONFIG_APP_ENABLE_MUSIC
#include "audio_player.h"
#endif

static const char *TAG = "music_progress";

/* 由 GUI Guider 生成的函数，负责刷新标签/滑条。 */
extern void MusicScreen_UpdateProgress(uint32_t cur_ms, uint32_t total_ms);

static lv_timer_t *s_timer        = NULL;
static uint32_t    s_total_ms     = 0;
static uint32_t    s_elapsed_ms   = 0;
static uint32_t    s_last_tick    = 0;
static uint32_t    s_committed_ms = 0;
static bool        s_dragging     = false;
static bool        s_is_playing   = false;

static void timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    /* 和底层播放器状态做一次轻量同步，防止偶发事件丢失导致
     * 音乐已播放而 UI 仍停在▶/进度不走的状态。*/
#if CONFIG_APP_ENABLE_MUSIC
    {
        ap_state_t st = ap_get_state();
        bool want_play = (st == AP_STATE_PLAYING);
        if (want_play != s_is_playing) {
            ESP_LOGI(TAG, "progress.sync: ap_state=%d -> playing=%d",
                     (int)st, (int)want_play);
            s_is_playing = want_play;
            s_last_tick  = lv_tick_get();
            /* 更新按钮图标（不调用 set_playing 以免递归），
             * 进度推进交给本回调后续逻辑。*/
            extern void MusicScreen_UpdatePlayState(bool playing);
            MusicScreen_UpdatePlayState(s_is_playing);
        }
    }
#endif

    if (!s_timer) {
        return;
    }

    uint32_t now = lv_tick_get();
    if (s_last_tick == 0) {
        s_last_tick = now;
        return;
    }

    if (s_dragging) {
        s_last_tick = now;
        return;
    }

    /* 若当前未处于“播放”状态，或者总时长未知，则不推进进度，
     * 但仍保持定时器常驻运行，以便及时感知 ap_state 变化。*/
    if (!s_is_playing || s_total_ms == 0) {
        MusicScreen_UpdateProgress(s_elapsed_ms, s_total_ms);
        s_last_tick = now;
        return;
    }

    uint32_t delta = now - s_last_tick;
    if (delta > 2000U) {
        delta = 2000U;
    }
    s_last_tick = now;

    if (s_elapsed_ms + delta >= s_total_ms) {
        s_elapsed_ms = s_total_ms;
        MusicScreen_UpdateProgress(s_elapsed_ms, s_total_ms);
#if CONFIG_APP_ENABLE_MUSIC
        /* 兜底：如果 UI 进度已到终点而底层没有及时上报 FINISHED，
         * 发一个“播放完成”事件让 UI 端按循环模式切到下一首。
         * music_ui 内部带去重保护，避免与真正的 FINISHED 同时到达时二次切歌。 */
        extern void music_ui_dispatch_playback_finished(bool forward);
        ESP_LOGI(TAG, "progress.end: fallback request auto-next");
        music_ui_dispatch_playback_finished(true);
#endif
        return;
    }

    s_elapsed_ms += delta;
    MusicScreen_UpdateProgress(s_elapsed_ms, s_total_ms);
}

static void ensure_timer_created(void)
{
    if (!s_timer) {
        s_timer = lv_timer_create(timer_cb, 500, NULL);
    }
}

static void update_progress(uint32_t position_ms, bool trigger_seek)
{
    if (position_ms > s_total_ms) {
        position_ms = s_total_ms;
    }

    s_elapsed_ms = position_ms;
    s_last_tick  = lv_tick_get();
    MusicScreen_UpdateProgress(s_elapsed_ms, s_total_ms);

#if CONFIG_APP_ENABLE_MUSIC
    if (trigger_seek) {
        ap_state_t st = ap_get_state();
        if (st == AP_STATE_PLAYING || st == AP_STATE_PAUSED ||
            st == AP_STATE_LOADING) {
            if (!ap_seek_to_ms(position_ms)) {
                ESP_LOGW(TAG, "seek rejected @%u ms",
                         (unsigned)position_ms);
            }
        }
    }
#else
    LV_UNUSED(trigger_seek);
#endif

    if (trigger_seek) {
        s_committed_ms = s_elapsed_ms;
    }
}

void music_progress_set_total(uint32_t total_ms)
{
    ESP_LOGI(TAG, "progress.set_total total_ms=%u (cur=%u)", (unsigned)total_ms, (unsigned)s_total_ms);

    /* If total_ms is the same as current, don't reset elapsed.
     * This prevents resume from resetting progress to 0 when MUSIC_INFO re-fires. */
    if (total_ms == s_total_ms && s_total_ms > 0) {
        ESP_LOGI(TAG, "progress.set_total: same total, skip reset");
        ensure_timer_created();
        return;
    }

    s_total_ms   = total_ms;
    s_elapsed_ms = 0;
    s_last_tick  = lv_tick_get();
    s_dragging   = false;
    s_committed_ms = 0;

    ensure_timer_created();
    if (!s_timer) {
        ESP_LOGE(TAG, "create timer failed");
        return;
    }

    MusicScreen_UpdateProgress(0, s_total_ms);
    lv_timer_set_period(s_timer, 500);
}

void music_progress_set_dragging(bool dragging)
{
    s_dragging  = dragging;
    s_last_tick = lv_tick_get();
}

void music_progress_preview(uint32_t position_ms)
{
    if (s_total_ms == 0) return;
    ESP_LOGD(TAG, "progress.preview ms=%u/%u", (unsigned)position_ms,
             (unsigned)s_total_ms);
    update_progress(position_ms, false);
}

void music_progress_commit(uint32_t position_ms)
{
    if (s_total_ms == 0) return;
    ESP_LOGI(TAG, "progress.commit seek_to=%u (from=%u) total=%u",
             (unsigned)position_ms, (unsigned)s_elapsed_ms,
             (unsigned)s_total_ms);
    update_progress(position_ms, true);
}

void music_progress_set_playing(bool playing)
{
    ESP_LOGI(TAG, "progress.set_playing playing=%d (total_ms=%u)",
             (int)playing, (unsigned)s_total_ms);
    ensure_timer_created();
    if (!s_timer) {
        ESP_LOGE(TAG, "timer missing, cannot toggle play state");
        return;
    }

    s_is_playing = playing;
    s_last_tick  = lv_tick_get();
}

uint32_t music_progress_total_ms(void)
{
    return s_total_ms;
}

uint32_t music_progress_elapsed_ms(void)
{
    return s_elapsed_ms;
}

