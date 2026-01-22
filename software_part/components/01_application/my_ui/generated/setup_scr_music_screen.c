/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

#include "music_ui.h"
#include "music_volume.h"
#include "music_progress.h"
#include "audio_player.h"
#if CONFIG_MY_UI_CHATBOT_ENABLE
#include "tts_player.h"
#endif
#include "music_spectrum.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_timer.h"

/* ---------- MusicScreen 行为层：音量条交互 + 残月列表 + 进度显示 ---------- */
lv_ui *g_music_screen_ui = NULL;
static const char *TAG_MUSIC_SCR = "MusicScreen";
/* 在用户点击暂停/停止后，短时间内忽略底层的短暂 PLAYING 抖动，避免按钮又回到“暂停(Ⅱ)” */
static int64_t s_pause_guard_until_us = 0;

typedef struct {
    bool visible;
    bool from_dark;
} spectrum_state_t;

static spectrum_state_t s_spectrum_state = {0};

bool MusicScreen_IsSpectrumVisible(void)
{
    return s_spectrum_state.visible;
}

bool MusicScreen_OpenSpectrum(bool dark_variant)
{
    int cur_idx = music_ui_current_index();
    if (s_spectrum_state.visible) {
        ESP_LOGI(TAG_MUSIC_SCR, "OpenSpectrum while visible (dark=%d, idx=%d)", (int)dark_variant, cur_idx);
        s_spectrum_state.from_dark = dark_variant;
        return true;
    }
    if (cur_idx < 0) {
        ESP_LOGW(TAG_MUSIC_SCR, "OpenSpectrum rejected: no active track");
        return false;
    }

    s_spectrum_state.visible = true;
    s_spectrum_state.from_dark = dark_variant;
    ESP_LOGI(TAG_MUSIC_SCR, "OpenSpectrum start (dark=%d, idx=%d)", (int)dark_variant, cur_idx);

    ui_load_scr_animation(&guider_ui,
                          &guider_ui.MusicSpectrum,
                          guider_ui.MusicSpectrum_del,
                          dark_variant ? &guider_ui.MusicScreen_dark_del : &guider_ui.MusicScreen_del,
                          setup_scr_music_spectrum,
                          LV_SCR_LOAD_ANIM_MOVE_LEFT,
                          220, 0, true, true);
    return true;
}

void MusicScreen_CloseSpectrum(void)
{
    if (!s_spectrum_state.visible) {
        return;
    }
    bool from_dark = s_spectrum_state.from_dark;
    s_spectrum_state.visible = false;

    /* 主动停止频谱定时器，避免在删除子对象（画布）与屏幕动画之间插入一帧访问 */
    extern void music_spectrum_ui_stop(void);
    music_spectrum_ui_stop();

    ui_load_scr_animation(&guider_ui,
                          from_dark ? &guider_ui.MusicScreen_dark : &guider_ui.MusicScreen,
                          from_dark ? guider_ui.MusicScreen_dark_del : guider_ui.MusicScreen_del,
                          &guider_ui.MusicSpectrum_del,
                          from_dark ? setup_scr_music_screen_dark : setup_scr_music_screen,
                          LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                          220, 0, true, true);

#if CONFIG_APP_ENABLE_MUSIC
    ap_state_t state = ap_get_state();
    /* 仅在真正 PLAYING 时点亮暂停图标；LOADING 不再视为播放，避免误启动进度 */
    bool playing = (state == AP_STATE_PLAYING);
#else
    bool playing = false;
#endif
    MusicScreen_UpdatePlayState(playing);
    music_progress_set_playing(playing);
    music_spectrum_set_active(false);
}

typedef enum {
    MUSIC_MODE_SEQUENTIAL = 0,
    MUSIC_MODE_REPEAT_ONE,
    MUSIC_MODE_REPEAT_ALL,
    MUSIC_MODE_SHUFFLE,
} music_play_mode_t;

static music_play_mode_t s_play_mode = MUSIC_MODE_SEQUENTIAL;

static void set_image_active(lv_obj_t *img, bool active, lv_color_t color)
{
    if (!img || !lv_obj_is_valid(img)) {
        return;
    }
    lv_part_t part = LV_PART_MAIN | LV_STATE_DEFAULT;
    if (active) {
        lv_obj_set_style_image_recolor(img, color, part);
        lv_obj_set_style_image_recolor_opa(img, 255, part);
    } else {
        lv_obj_set_style_image_recolor_opa(img, 0, part);
    }
}

void update_heart_icon(void)
{
    if (!g_music_screen_ui || !g_music_screen_ui->MusicScreen_img_heart ||
        !lv_obj_is_valid(g_music_screen_ui->MusicScreen_img_heart)) {
        return;
    }
    int idx = music_ui_current_index();
    bool liked = (idx >= 0) && music_ui_is_favorite(idx);
    set_image_active(g_music_screen_ui->MusicScreen_img_heart, liked, lv_color_hex(0xE91E63));
}

void update_mode_icons(void)
{
    if (!g_music_screen_ui) {
        return;
    }
    set_image_active(g_music_screen_ui->MusicScreen_img_repeat_one,
                     s_play_mode == MUSIC_MODE_REPEAT_ONE,
                     lv_color_hex(0x2196F3));
    set_image_active(g_music_screen_ui->MusicScreen_img_repeat_all,
                     s_play_mode == MUSIC_MODE_REPEAT_ALL,
                     lv_color_hex(0x2196F3));
    set_image_active(g_music_screen_ui->MusicScreen_img_shuffle,
                     s_play_mode == MUSIC_MODE_SHUFFLE,
                     lv_color_hex(0xFF9800));
}

static int pick_random_index(int exclude, int count)
{
    if (count <= 0) {
        return -1;
    }
    if (count == 1) {
        return 0;
    }
    int idx;
    do {
        idx = (int)(esp_random() % (uint32_t)count);
    } while (idx == exclude);
    return idx;
}

static int compute_step_index(bool forward)
{
    int count = music_ui_count();
    if (count <= 0) {
        return -1;
    }
    int cur = music_ui_current_index();
    if (cur < 0) {
        cur = forward ? 0 : (count - 1);
    }

    switch (s_play_mode) {
    case MUSIC_MODE_REPEAT_ONE:
        if (cur < 0) {
            return (count > 0) ? 0 : -1;
        }
        return cur;
    case MUSIC_MODE_SHUFFLE:
        if (count == 1) {
            return (cur < 0) ? 0 : cur;
        }
        return pick_random_index(cur, count);
    case MUSIC_MODE_REPEAT_ALL:
        if (forward) {
            int next = (cur < 0) ? 0 : (cur + 1);
            if (next >= count) {
                next = 0;
            }
            return next;
        } else {
            int prev = (cur < 0) ? (count - 1) : (cur - 1);
            if (prev < 0) {
                prev = count - 1;
            }
            return prev;
        }
    case MUSIC_MODE_SEQUENTIAL:
    default:
        if (forward) {
            int next = (cur < 0) ? 0 : (cur + 1);
            if (next >= count) {
                next = 0;
            }
            return next;
        } else {
            if (cur < 0) {
                return count - 1;
            }
            int prev = cur - 1;
            if (prev < 0) {
                prev = count - 1;
            }
            return prev;
        }
    }
}

void MusicScreen_UpdatePlayState(bool playing)
{
    if (!g_music_screen_ui || !g_music_screen_ui->MusicScreen_imgbtn_play ||
        !lv_obj_is_valid(g_music_screen_ui->MusicScreen_imgbtn_play)) {
        return;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "ui: UpdatePlayState playing=%d", (int)playing);
    if (playing) {
        lv_obj_add_state(g_music_screen_ui->MusicScreen_imgbtn_play, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(g_music_screen_ui->MusicScreen_imgbtn_play, LV_STATE_CHECKED);
    }
}

void MusicScreen_OnTrackChanged(void)
{
    update_heart_icon();
    /* 切歌时不要抢先显示“播放中”，等待底层真正进入 PLAYING 再更新。*/
    MusicScreen_UpdatePlayState(false);
    music_progress_set_playing(false);
}

/* 安全把毫秒转成 "MM:SS"（out 至少 6 字节） */
static void ms_to_mmss(uint32_t ms, char out[6])
{
    uint32_t s = ms / 1000U;
    uint32_t m = s / 60U;
    s %= 60U;
    if (m > 99U) { m = 99U; s = 59U; } /* 防止 100:xx 导致格式截断告警 */
    (void)snprintf(out, 6, "%02u:%02u", (unsigned)m, (unsigned)s);
}

static void music_ctrl_step_track(bool forward)
{
    int cur = music_ui_current_index();
    int target = compute_step_index(forward);
    ESP_LOGI(TAG_MUSIC_SCR, "click: %s -> target=%d (cur=%d) mode=%d", forward?"next":"prev", target, cur, (int)s_play_mode);
    if (target < 0) {
        MusicScreen_UpdatePlayState(false);
        music_progress_set_playing(false);
        return;
    }
    if (music_ui_play_index(target)) {
        /* 切换曲目后等待底层进入 PLAYING 再点亮按钮与进度 */
        MusicScreen_UpdatePlayState(false);
        music_progress_set_playing(false);
    } else {
        MusicScreen_UpdatePlayState(false);
        music_progress_set_playing(false);
    }
}

static void music_ctrl_step_async_cb(void *param)
{
    bool forward = ((intptr_t)param) != 0;
    music_ctrl_step_track(forward);
}

void MusicScreen_RequestStep(bool forward)
{
    ESP_LOGI(TAG_MUSIC_SCR, "auto-step: request %s", forward?"next":"prev");
    (void)lv_async_call(music_ctrl_step_async_cb, (void *)(intptr_t)(forward ? 1 : 0));
}

void music_ctrl_heart_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    int idx = music_ui_current_index();
    if (idx < 0) {
        return;
    }
    (void)music_ui_toggle_favorite(idx);
    update_heart_icon();
}

void music_ctrl_repeat_one_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_play_mode == MUSIC_MODE_REPEAT_ONE) {
        s_play_mode = MUSIC_MODE_SEQUENTIAL;
    } else {
        s_play_mode = MUSIC_MODE_REPEAT_ONE;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "mode: set REPEAT_ONE=%d", s_play_mode == MUSIC_MODE_REPEAT_ONE);
    update_mode_icons();
}

void music_ctrl_repeat_all_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_play_mode == MUSIC_MODE_REPEAT_ALL) {
        s_play_mode = MUSIC_MODE_SEQUENTIAL;
    } else {
        s_play_mode = MUSIC_MODE_REPEAT_ALL;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "mode: set REPEAT_ALL=%d", s_play_mode == MUSIC_MODE_REPEAT_ALL);
    update_mode_icons();
}

void music_ctrl_shuffle_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_play_mode == MUSIC_MODE_SHUFFLE) {
        s_play_mode = MUSIC_MODE_SEQUENTIAL;
    } else {
        s_play_mode = MUSIC_MODE_SHUFFLE;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "mode: set SHUFFLE=%d", s_play_mode == MUSIC_MODE_SHUFFLE);
    update_mode_icons();
}

void music_ctrl_prev_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "click: prev");
    music_ctrl_step_track(false);
}

void music_ctrl_next_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ESP_LOGI(TAG_MUSIC_SCR, "click: next");
    music_ctrl_step_track(true);
}

#if CONFIG_APP_ENABLE_MUSIC
void music_ctrl_play_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (!g_music_screen_ui || !g_music_screen_ui->MusicScreen_imgbtn_play) {
        return;
    }

#if CONFIG_MY_UI_CHATBOT_ENABLE
    /* 若此时 TTS 正在播报，先停止 TTS，避免与音乐播放争用 I2S */
    if (tts_player_is_active()) {
        tts_player_stop_now();
    }
#endif

    bool want_play = lv_obj_has_state(g_music_screen_ui->MusicScreen_imgbtn_play, LV_STATE_CHECKED);
    ap_state_t state = ap_get_state();
    ESP_LOGI(TAG_MUSIC_SCR, "click: play_btn want_play=%d ap_state_before=%d", (int)want_play, (int)state);

    if (want_play) {
        bool ok = false;
        if (state == AP_STATE_PLAYING) {
            ok = true;
        } else if (state == AP_STATE_PAUSED) {
            ok = ap_resume();
        } else {
            int idx = music_ui_current_index();
            if (idx < 0 && music_ui_count() > 0) {
                idx = 0;
            }
            if (idx >= 0) {
                ok = music_ui_play_index(idx);
            }
        }
        if (!ok) {
            MusicScreen_UpdatePlayState(ap_get_state() == AP_STATE_PLAYING);
            music_progress_set_playing(ap_get_state() == AP_STATE_PLAYING);
        } else {
            MusicScreen_UpdatePlayState(true);
            music_progress_set_playing(true);
        }
    } else {
        bool ok = true;
        if (state == AP_STATE_PLAYING) {
            ok = ap_pause();
        } else if (state == AP_STATE_LOADING) {
            /* 若仍在缓冲/加载阶段点了暂停，主动停止当前管线，避免随后进入播放 */
            ok = ap_stop();
        }
        /* 设置 500ms 保护窗，忽略底层可能晚到的 PLAYING 抖动 */
        s_pause_guard_until_us = (int64_t)esp_timer_get_time() + 500000LL;
        if (!ok) {
            MusicScreen_UpdatePlayState(true);
            music_progress_set_playing(true);
        } else {
            MusicScreen_UpdatePlayState(false);
            music_progress_set_playing(false);
        }
    }
    ESP_LOGI(TAG_MUSIC_SCR, "click: play_btn done ap_state_after=%d", (int)ap_get_state());
}
#else
void music_ctrl_play_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (!g_music_screen_ui || !g_music_screen_ui->MusicScreen_imgbtn_play) {
        return;
    }
    bool want_play = lv_obj_has_state(g_music_screen_ui->MusicScreen_imgbtn_play, LV_STATE_CHECKED);
    MusicScreen_UpdatePlayState(want_play);
    music_progress_set_playing(want_play);
}
#endif

/* ------------ 1) 让 lv_bar 可拖：把触点映射为 0~100 ------------ */
static void vol_bar_set_from_indev(lv_obj_t *bar)
{
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) { return; }

    lv_point_t p; 
    lv_indev_get_point(indev, &p);

    lv_area_t a; 
    lv_obj_get_coords(bar, &a);

    int32_t h = (a.y2 - a.y1 + 1);
    if (h <= 0) { return; }

    int32_t y = p.y - a.y1;                /* 相对条顶边的 y */
    if (y < 0)  { y = 0;  }
    if (y > h)  { y = h;  }

    /* 竖直条：越上值越大 */
    int32_t v = (int32_t)((100LL * (h - y) + h/2) / h);
    if (v < 0)   { v = 0;   }
    if (v > 100) { v = 100; }

    lv_bar_set_value(bar, v, LV_ANIM_OFF);

    /* TODO: 在此调用你的音量接口，例如：
       app_audio_svc_set_volume(v);
    */
}
void vol_evt_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        vol_bar_set_from_indev(lv_event_get_target(e));
    }
}

/* ------------ 2) 残月效果：滚动时对每个 item 做横向位移+透明度 ------------ */
#ifndef RING_MAX_SHIFT_PX
#define RING_MAX_SHIFT_PX 32 /* 距离中线越远，向右缩进最大像素，可按需调大 */
#endif

static void ring_list_refresh(lv_obj_t *list)
{
    lv_area_t la; 
    lv_obj_get_coords(list, &la);
    int32_t mid_y  = (la.y1 + la.y2) / 2;
    int32_t radius = (la.y2 - la.y1) / 2;
    if (radius <= 0) { radius = 1; }

    uint32_t n = lv_obj_get_child_count(list);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *item = lv_obj_get_child(list, i);
        lv_area_t ia; 
        lv_obj_get_coords(item, &ia);
        int32_t cy = (ia.y1 + ia.y2) / 2;
        int32_t dy = LV_ABS(cy - mid_y);

        int32_t shift = (RING_MAX_SHIFT_PX * dy) / radius;
        if (shift > RING_MAX_SHIFT_PX) { shift = RING_MAX_SHIFT_PX; }
        lv_obj_set_style_translate_x(item, shift, LV_PART_MAIN);

        int32_t opa = 255 - (dy * 140) / radius;   /* 140：衰减强度 */
        if (opa < 110) { opa = 110; }
        lv_obj_set_style_opa(item, (lv_opa_t)opa, LV_PART_MAIN);
    }
}
void list_scroll_cb(lv_event_t *e)
{
    ring_list_refresh(lv_event_get_target(e));
}

/* ------------ 3) 进度条：拖动时即时显示左侧时间；释放时可 seek ------------ */
void prog_evt_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) {
        return;
    }

    if (!lv_event_get_indev(e)) {
        return;
    }

    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    int32_t max = lv_slider_get_max_value(slider);
    if (max <= 0) { max = 100; }

    uint32_t total_ms = music_progress_total_ms();

    if (code == LV_EVENT_VALUE_CHANGED) {
        music_progress_set_dragging(true);
        ESP_LOGD(TAG_MUSIC_SCR, "progress.drag value_changed slider=%d/%d", (int)val, (int)max);
    } else if (code == LV_EVENT_RELEASED) {
        music_progress_set_dragging(false);
        ESP_LOGI(TAG_MUSIC_SCR, "progress.release slider=%d/%d", (int)val, (int)max);
    }

    if (total_ms == 0) {
        return;
    }

    uint32_t cur_ms = (uint32_t)((int64_t)total_ms * val / max);

    if (code == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGD(TAG_MUSIC_SCR, "progress.preview cur=%u total=%u", (unsigned)cur_ms, (unsigned)total_ms);
        music_progress_preview(cur_ms);
    } else if (code == LV_EVENT_RELEASED) {
        uint32_t cur_pos = music_progress_elapsed_ms();
        uint32_t diff = (cur_ms > cur_pos) ? (cur_ms - cur_pos) : (cur_pos - cur_ms);
        if (diff < 500U) {
            ESP_LOGI(TAG_MUSIC_SCR, "progress.commit small-diff -> preview cur=%u pos=%u", (unsigned)cur_ms, (unsigned)cur_pos);
            music_progress_preview(cur_ms);
        } else {
            ESP_LOGI(TAG_MUSIC_SCR, "progress.commit seek cur=%u pos=%u", (unsigned)cur_ms, (unsigned)cur_pos);
            music_progress_commit(cur_ms);
        }
    }
}

/* ------------ 4) 可给播放线程调用的 UI 更新接口（可选） ------------ */
#if CONFIG_APP_ENABLE_MUSIC
static void music_ctrl_state_async_cb(void *param)
{
    ap_state_t state = (ap_state_t)(intptr_t)param;
    /* 仅在真正播放时才点亮暂停图标，LOADING 不算播放。*/
    bool playing = (state == AP_STATE_PLAYING);
    /* 若处于“暂停保护窗”，忽略临时性的 PLAYING 抖动。*/
    if (playing && s_pause_guard_until_us > 0) {
        int64_t now = (int64_t)esp_timer_get_time();
        if (now < s_pause_guard_until_us) {
            ESP_LOGI(TAG_MUSIC_SCR, "ui: guard PLAYING ignored (pause_guard %lld us left)",
                     (long long)(s_pause_guard_until_us - now));
            playing = false;
        } else {
            s_pause_guard_until_us = 0;
        }
    }
    MusicScreen_UpdatePlayState(playing);
    music_progress_set_playing(playing);
}

void MusicScreen_HandlePlaybackState(ap_state_t state)
{
    (void)lv_async_call(music_ctrl_state_async_cb, (void *)(intptr_t)state);
}
#endif

void MusicScreen_UpdateProgress(uint32_t cur_ms, uint32_t total_ms)
{
    if (!g_music_screen_ui) { return; }
    if (!g_music_screen_ui->MusicScreen_music_slider_prog ||
        !lv_obj_is_valid(g_music_screen_ui->MusicScreen_music_slider_prog)) {
        return;
    }
    if (!g_music_screen_ui->MusicScreen_music_time_elapsed ||
        !lv_obj_is_valid(g_music_screen_ui->MusicScreen_music_time_elapsed) ||
        !g_music_screen_ui->MusicScreen_music_time_total ||
        !lv_obj_is_valid(g_music_screen_ui->MusicScreen_music_time_total)) {
        return;
    }
    if (total_ms == 0U) { total_ms = 1U; }

    uint32_t ratio = (cur_ms >= total_ms) ? 100U
                     : (uint32_t)((cur_ms * 100ULL) / total_ms);

    lv_slider_set_value(g_music_screen_ui->MusicScreen_music_slider_prog, (int32_t)ratio, LV_ANIM_OFF);

    char l[6], r[6];
    ms_to_mmss(cur_ms,  l);
    ms_to_mmss(total_ms, r);
    lv_label_set_text(g_music_screen_ui->MusicScreen_music_time_elapsed, l);
    lv_label_set_text(g_music_screen_ui->MusicScreen_music_time_total,  r);
}

void setup_scr_music_screen(lv_ui *ui)
{
    /* 进入音乐页时，统一做一次音频场景切换，确保不存在残留的 GPT 语音播放 */
    AppAudio_OnEnterMusicScreen();

    //Write codes MusicScreen
    ui->MusicScreen = lv_obj_create(NULL);
    lv_obj_set_size(ui->MusicScreen, 480, 320);
    lv_obj_set_scrollbar_mode(ui->MusicScreen, LV_SCROLLBAR_MODE_OFF);

    // 记住本屏的 ui 指针（用于进度/时间实时更新）
    g_music_screen_ui = ui;

    //Write style for MusicScreen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_root
    ui->MusicScreen_music_root = lv_obj_create(ui->MusicScreen);
    lv_obj_set_pos(ui->MusicScreen_music_root, 0, 0);
    lv_obj_set_size(ui->MusicScreen_music_root, 480, 320);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_root, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_root, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_root, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_root, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_root, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_root, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_top
    ui->MusicScreen_music_top = lv_obj_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_music_top, 75, 0);
    lv_obj_set_size(ui->MusicScreen_music_top, 405, 60);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_top, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->MusicScreen_music_top, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->MusicScreen_music_top, LV_OBJ_FLAG_GESTURE_BUBBLE);

    //Write style for MusicScreen_music_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_top, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_top, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_title
    ui->MusicScreen_music_title = lv_label_create(ui->MusicScreen_music_top);
    lv_obj_set_pos(ui->MusicScreen_music_title, 5, 15);
    lv_obj_set_size(ui->MusicScreen_music_title, 260, 30);
    lv_label_set_text(ui->MusicScreen_music_title, "Track 01 - Artist");
    lv_label_set_long_mode(ui->MusicScreen_music_title, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(ui->MusicScreen_music_title, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(ui->MusicScreen_music_title, LV_OBJ_FLAG_CLICKABLE);

    //Write style for MusicScreen_music_title, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_music_title, lv_color_hex(0xe999e0), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_music_title, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_music_title, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_music_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_source_label
    ui->MusicScreen_source_label = lv_label_create(ui->MusicScreen_music_top);
    lv_obj_set_pos(ui->MusicScreen_source_label, 275, 20);
    lv_obj_set_size(ui->MusicScreen_source_label, 125, 30);
    lv_label_set_text(ui->MusicScreen_source_label, "当前来源：网络");
    lv_label_set_long_mode(ui->MusicScreen_source_label, LV_LABEL_LONG_CLIP);

    //Write style for MusicScreen_source_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_source_label, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_source_label, &lv_font_HarmonyOSHans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_source_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_source_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_source_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 根据当前模式（HTTP / SD 卡）刷新一次来源标签文本 */
    MusicScreen_UpdateSourceLabel(music_ui_is_using_sdcard());

    //Write codes MusicScreen_music_mid
    ui->MusicScreen_music_mid = lv_obj_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_music_mid, 0, 65);
    lv_obj_set_size(ui->MusicScreen_music_mid, 480, 165);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_mid, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_mid, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_mid, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_mid, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_mid, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_mid, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_list
    ui->MusicScreen_music_list = lv_list_create(ui->MusicScreen_music_mid);
    lv_obj_set_pos(ui->MusicScreen_music_list, 80, 5);
    lv_obj_set_size(ui->MusicScreen_music_list, 395, 150);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_list, LV_SCROLLBAR_MODE_OFF);
    ui->MusicScreen_music_list_item0 = lv_list_add_button(ui->MusicScreen_music_list, LV_SYMBOL_AUDIO, "Track 01");
    ui->MusicScreen_music_list_item1 = lv_list_add_button(ui->MusicScreen_music_list, LV_SYMBOL_AUDIO, "Track 02");
    ui->MusicScreen_music_list_item2 = lv_list_add_button(ui->MusicScreen_music_list, LV_SYMBOL_AUDIO, "Track 03");
    ui->MusicScreen_music_list_item3 = lv_list_add_button(ui->MusicScreen_music_list, LV_SYMBOL_AUDIO, "Track 04");
    ui->MusicScreen_music_list_item4 = lv_list_add_button(ui->MusicScreen_music_list, LV_SYMBOL_AUDIO, "Track 05");

    // 残月效果：监听滚动
    lv_obj_add_event_cb(ui->MusicScreen_music_list, list_scroll_cb, LV_EVENT_SCROLL, NULL);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_main_main_default
    static lv_style_t style_MusicScreen_music_list_main_main_default;
    ui_init_style(&style_MusicScreen_music_list_main_main_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_main_main_default, 111);
    lv_style_set_bg_color(&style_MusicScreen_music_list_main_main_default, lv_color_hex(0xf3c2ed));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_radius(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_shadow_width(&style_MusicScreen_music_list_main_main_default, 0);
    lv_style_set_pad_top(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_main_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_main_main_default, 5);
    lv_obj_add_style(ui->MusicScreen_music_list, &style_MusicScreen_music_list_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_main_scrollbar_default
    static lv_style_t style_MusicScreen_music_list_main_scrollbar_default;
    ui_init_style(&style_MusicScreen_music_list_main_scrollbar_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&style_MusicScreen_music_list_main_scrollbar_default, 3);
    lv_obj_add_style(ui->MusicScreen_music_list, &style_MusicScreen_music_list_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_extra_btns_main_default
    static lv_style_t style_MusicScreen_music_list_extra_btns_main_default;
    ui_init_style(&style_MusicScreen_music_list_extra_btns_main_default);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_MusicScreen_music_list_extra_btns_main_default, 0);
    lv_style_set_radius(&style_MusicScreen_music_list_extra_btns_main_default, 3);
    lv_style_set_text_color(&style_MusicScreen_music_list_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_MusicScreen_music_list_extra_btns_main_default, &lv_font_montserratMedium_24);
    lv_style_set_text_opa(&style_MusicScreen_music_list_extra_btns_main_default, 255);
    lv_style_set_pad_top(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_extra_btns_main_default, 5);
    lv_obj_add_style(ui->MusicScreen_music_list_item4, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_music_list_item3, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_music_list_item2, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_music_list_item1, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->MusicScreen_music_list_item0, &style_MusicScreen_music_list_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_MusicScreen_music_list_extra_texts_main_default
    static lv_style_t style_MusicScreen_music_list_extra_texts_main_default;
    ui_init_style(&style_MusicScreen_music_list_extra_texts_main_default);
    lv_style_set_pad_top(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_MusicScreen_music_list_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_MusicScreen_music_list_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_MusicScreen_music_list_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_MusicScreen_music_list_extra_texts_main_default, &lv_font_montserratMedium_24);
    lv_style_set_text_opa(&style_MusicScreen_music_list_extra_texts_main_default, 255);
    lv_style_set_radius(&style_MusicScreen_music_list_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_MusicScreen_music_list_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_MusicScreen_music_list_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_MusicScreen_music_list_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_MusicScreen_music_list_extra_texts_main_default, LV_GRAD_DIR_NONE);

    /* 在创建完 ui->MusicScreen_music_list 之后放这段（放前/后都行） */
    static lv_style_t style_list_items_txt24;
    static bool style_list_items_inited = false;
    if (!style_list_items_inited) {
    	ui_init_style(&style_list_items_txt24);
    	lv_style_set_text_font(&style_list_items_txt24, &lv_font_montserratMedium_24); // 24号
    	lv_style_set_text_color(&style_list_items_txt24, lv_color_hex(0x0D3055));   // 你的配色
    	style_list_items_inited = true;
    }

    /* 关键：把样式挂到 items 部分 */
    lv_obj_add_style(ui->MusicScreen_music_list,
                 	&style_list_items_txt24,
                 	LV_PART_ITEMS | LV_STATE_DEFAULT);

    /* 为选中项添加简单高亮（淡色底） */
    static lv_style_t style_list_item_checked;
    static bool style_item_checked_inited = false;
    if (!style_item_checked_inited) {
        ui_init_style(&style_list_item_checked);
        lv_style_set_bg_opa(&style_list_item_checked, 96);
        lv_style_set_bg_color(&style_list_item_checked, lv_color_hex(0xEAF4FF));
        lv_style_set_radius(&style_list_item_checked, 4);
        style_item_checked_inited = true;
    }
    lv_obj_add_style(ui->MusicScreen_music_list,
                     &style_list_item_checked,
                     LV_PART_ITEMS | LV_STATE_CHECKED);

    music_ui_attach_list(ui->MusicScreen_music_list);

    //Write codes MusicScreen_music_bar_vol
    ui->MusicScreen_music_bar_vol = lv_bar_create(ui->MusicScreen_music_mid);
    lv_obj_set_pos(ui->MusicScreen_music_bar_vol, 30, 5);
    lv_obj_set_size(ui->MusicScreen_music_bar_vol, 20, 150);
    lv_obj_set_style_anim_duration(ui->MusicScreen_music_bar_vol, 1000, 0);
    lv_bar_set_mode(ui->MusicScreen_music_bar_vol, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->MusicScreen_music_bar_vol, 0, 100);
    lv_bar_set_value(ui->MusicScreen_music_bar_vol, music_volume_get_percent(), LV_ANIM_OFF);

    // 让音量条可拖
    lv_obj_add_flag(ui->MusicScreen_music_bar_vol, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->MusicScreen_music_bar_vol, vol_evt_cb, LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(ui->MusicScreen_music_bar_vol, vol_evt_cb, LV_EVENT_PRESSING, NULL);

    music_volume_bind_slider(ui->MusicScreen_music_bar_vol);

    //Write style for MusicScreen_music_bar_vol, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_bar_vol, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_bar_vol, lv_color_hex(0x2FCADA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_bar_vol, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_bar_vol, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_bar_vol, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_bar_vol, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_bar_vol, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_bar_vol, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_bar_vol, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_bar_vol, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_bottom_a
    ui->MusicScreen_music_bottom_a = lv_obj_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_music_bottom_a, 80, 240);
    lv_obj_set_size(ui->MusicScreen_music_bottom_a, 400, 30);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_bottom_a, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_bottom_a, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_bottom_a, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_bottom_a, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_bottom_a, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_bottom_a, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_time_total
    ui->MusicScreen_music_time_total = lv_label_create(ui->MusicScreen_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_music_time_total, 325, 0);
    lv_obj_set_size(ui->MusicScreen_music_time_total, 65, 25);
    lv_label_set_text(ui->MusicScreen_music_time_total, "00:00");
    lv_label_set_long_mode(ui->MusicScreen_music_time_total, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_time_total, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_music_time_total, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_music_time_total, &lv_font_HarmonyOSHans_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_music_time_total, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_music_time_total, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_time_total, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_slider_prog
    ui->MusicScreen_music_slider_prog = lv_slider_create(ui->MusicScreen_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_music_slider_prog, 70, 5);
    lv_obj_set_size(ui->MusicScreen_music_slider_prog, 250, 10);
    lv_slider_set_range(ui->MusicScreen_music_slider_prog, 0, 100);
    lv_slider_set_mode(ui->MusicScreen_music_slider_prog, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->MusicScreen_music_slider_prog, 0, LV_ANIM_OFF);

    // 进度条拖动/释放事件（VALUE_CHANGED 实时显示；RELEASED 可做 seek）
    lv_obj_add_event_cb(ui->MusicScreen_music_slider_prog, prog_evt_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_music_slider_prog, prog_evt_cb, LV_EVENT_RELEASED,      NULL);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_slider_prog, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_slider_prog, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->MusicScreen_music_slider_prog, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_slider_prog, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_slider_prog, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_slider_prog, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for MusicScreen_music_slider_prog, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_slider_prog, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_slider_prog, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_slider_prog, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_slider_prog, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_time_elapsed
    ui->MusicScreen_music_time_elapsed = lv_label_create(ui->MusicScreen_music_bottom_a);
    lv_obj_set_pos(ui->MusicScreen_music_time_elapsed, 0, 0);
    lv_obj_set_size(ui->MusicScreen_music_time_elapsed, 65, 25);
    lv_label_set_text(ui->MusicScreen_music_time_elapsed, "00:00");
    lv_label_set_long_mode(ui->MusicScreen_music_time_elapsed, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_time_elapsed, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_music_time_elapsed, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_music_time_elapsed, &lv_font_HarmonyOSHans_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_music_time_elapsed, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_music_time_elapsed, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_time_elapsed, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_music_ctrl_row
    ui->MusicScreen_music_ctrl_row = lv_obj_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_music_ctrl_row, 0, 280);
    lv_obj_set_size(ui->MusicScreen_music_ctrl_row, 480, 40);
    lv_obj_set_scrollbar_mode(ui->MusicScreen_music_ctrl_row, LV_SCROLLBAR_MODE_OFF);

    //Write style for MusicScreen_music_ctrl_row, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_ctrl_row, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicScreen_music_ctrl_row, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicScreen_music_ctrl_row, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_ctrl_row, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_heart
    ui->MusicScreen_img_heart = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_heart, 30, 0);
    lv_obj_set_size(ui->MusicScreen_img_heart, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_heart, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_heart, &_ui_img_heart_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_heart, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_heart, 0);

    //Write style for MusicScreen_img_heart, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_heart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_heart, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_repeat_one
    ui->MusicScreen_img_repeat_one = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_repeat_one, 90, 0);
    lv_obj_set_size(ui->MusicScreen_img_repeat_one, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_repeat_one, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_repeat_one, &_ui_img_repeat_one_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_repeat_one, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_repeat_one, 0);

    //Write style for MusicScreen_img_repeat_one, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_repeat_one, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_repeat_one, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_prev
    ui->MusicScreen_img_prev = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_prev, 150, 0);
    lv_obj_set_size(ui->MusicScreen_img_prev, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_prev, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_prev, &_ui_img_prev_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_prev, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_prev, 0);

    //Write style for MusicScreen_img_prev, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_prev, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_prev, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_imgbtn_play
    ui->MusicScreen_imgbtn_play = lv_imagebutton_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_imgbtn_play, 210, 0);
    lv_obj_set_size(ui->MusicScreen_imgbtn_play, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_imgbtn_play, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->MusicScreen_imgbtn_play, LV_OBJ_FLAG_CHECKABLE);
    lv_imagebutton_set_src(ui->MusicScreen_imgbtn_play, LV_IMAGEBUTTON_STATE_RELEASED, &_ui_img_play_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_imgbtn_play, LV_IMAGEBUTTON_STATE_PRESSED, &_ui_img_play_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_imgbtn_play, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, &_ui_img_pause_png_RGB565A8_40x40, NULL, NULL);
    lv_imagebutton_set_src(ui->MusicScreen_imgbtn_play, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, &_ui_img_pause_png_RGB565A8_40x40, NULL, NULL);
    ui->MusicScreen_imgbtn_play_label = lv_label_create(ui->MusicScreen_imgbtn_play);
    lv_label_set_text(ui->MusicScreen_imgbtn_play_label, "");
    lv_label_set_long_mode(ui->MusicScreen_imgbtn_play_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->MusicScreen_imgbtn_play_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->MusicScreen_imgbtn_play, 0, LV_STATE_DEFAULT);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->MusicScreen_imgbtn_play, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_imgbtn_play, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_image_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->MusicScreen_imgbtn_play, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->MusicScreen_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_PRESSED);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_image_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_color(ui->MusicScreen_imgbtn_play, lv_color_hex(0xFF33FF), LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_font(ui->MusicScreen_imgbtn_play, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_text_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style for MusicScreen_imgbtn_play, Part: LV_PART_MAIN, State: LV_IMAGEBUTTON_STATE_RELEASED.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_imgbtn_play, 0, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);
    lv_obj_set_style_image_opa(ui->MusicScreen_imgbtn_play, 255, LV_PART_MAIN|LV_IMAGEBUTTON_STATE_RELEASED);

    //Write codes MusicScreen_img_next
    ui->MusicScreen_img_next = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_next, 270, 0);
    lv_obj_set_size(ui->MusicScreen_img_next, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_next, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_next, &_ui_img_next_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_next, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_next, 0);

    //Write style for MusicScreen_img_next, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_next, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_next, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_shuffle
    ui->MusicScreen_img_shuffle = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_shuffle, 330, 0);
    lv_obj_set_size(ui->MusicScreen_img_shuffle, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_shuffle, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_shuffle, &_ui_img_shuffle_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_shuffle, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_shuffle, 0);

    //Write style for MusicScreen_img_shuffle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_shuffle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_shuffle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_repeat_all
    ui->MusicScreen_img_repeat_all = lv_image_create(ui->MusicScreen_music_ctrl_row);
    lv_obj_set_pos(ui->MusicScreen_img_repeat_all, 390, 0);
    lv_obj_set_size(ui->MusicScreen_img_repeat_all, 40, 40);
    lv_obj_add_flag(ui->MusicScreen_img_repeat_all, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_repeat_all, &_ui_img_repeat_all_png_RGB565A8_40x40);
    lv_image_set_pivot(ui->MusicScreen_img_repeat_all, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_repeat_all, 0);

    //Write style for MusicScreen_img_repeat_all, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_repeat_all, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_repeat_all, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui->MusicScreen_img_heart, music_ctrl_heart_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_img_repeat_one, music_ctrl_repeat_one_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_img_repeat_all, music_ctrl_repeat_all_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_img_shuffle, music_ctrl_shuffle_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_img_prev, music_ctrl_prev_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_img_next, music_ctrl_next_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->MusicScreen_imgbtn_play, music_ctrl_play_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    update_mode_icons();
    update_heart_icon();
#if CONFIG_APP_ENABLE_MUSIC
    MusicScreen_UpdatePlayState(ap_get_state() == AP_STATE_PLAYING);
#else
    MusicScreen_UpdatePlayState(false);
#endif

    /* 补救：屏幕被删除后重建会把标题重置为默认文本。
       这里在创建完成时，用当前播放条目恢复标题与时间显示。*/
    do {
        int idx = music_ui_current_index();
        if (idx < 0) break;

        const char *title = music_ui_title_at(idx);
        if (ui->MusicScreen_music_title && lv_obj_is_valid(ui->MusicScreen_music_title) && title && title[0]) {
            lv_label_set_text(ui->MusicScreen_music_title, title);
        }

        /* 优先使用目录中的“MM:SS”，否则用毫秒换算 */
        if (ui->MusicScreen_music_time_total && lv_obj_is_valid(ui->MusicScreen_music_time_total)) {
            const char *dur = music_ui_duration_at(idx);
            if (dur && dur[0]) {
                lv_label_set_text(ui->MusicScreen_music_time_total, dur);
            } else {
                char mmss[6];
                ms_to_mmss(music_ui_duration_ms_at(idx), mmss);
                lv_label_set_text(ui->MusicScreen_music_time_total, mmss);
            }
        }

        /* 同步一次进度到新建的控件（不改变实际播放位置） */
        MusicScreen_UpdateProgress(music_progress_elapsed_ms(), music_progress_total_ms());
    } while (0);

    //Write codes MusicScreen_music_vlo_label
    ui->MusicScreen_music_vlo_label = lv_label_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_music_vlo_label, 15, 235);
    lv_obj_set_size(ui->MusicScreen_music_vlo_label, 50, 25);
    lv_label_set_text(ui->MusicScreen_music_vlo_label, "音量");
    lv_label_set_long_mode(ui->MusicScreen_music_vlo_label, LV_LABEL_LONG_WRAP);

    //Write style for MusicScreen_music_vlo_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_music_vlo_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_music_vlo_label, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_music_vlo_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_music_vlo_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_music_vlo_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_btn_1
    ui->MusicScreen_btn_1 = lv_button_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_btn_1, 15, 10);
    lv_obj_set_size(ui->MusicScreen_btn_1, 30, 30);
    ui->MusicScreen_btn_1_label = lv_label_create(ui->MusicScreen_btn_1);
    lv_label_set_text(ui->MusicScreen_btn_1_label, "");
    lv_label_set_long_mode(ui->MusicScreen_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->MusicScreen_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->MusicScreen_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->MusicScreen_btn_1_label, LV_PCT(100));

    //Write style for MusicScreen_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->MusicScreen_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicScreen_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->MusicScreen_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->MusicScreen_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->MusicScreen_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicScreen_btn_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicScreen_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->MusicScreen_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->MusicScreen_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->MusicScreen_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->MusicScreen_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes MusicScreen_img_1
    ui->MusicScreen_img_1 = lv_image_create(ui->MusicScreen_music_root);
    lv_obj_set_pos(ui->MusicScreen_img_1, 13, 10);
    lv_obj_set_size(ui->MusicScreen_img_1, 30, 30);
    lv_obj_add_flag(ui->MusicScreen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->MusicScreen_img_1, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->MusicScreen_img_1, 50,50);
    lv_image_set_rotation(ui->MusicScreen_img_1, 0);

    //Write style for MusicScreen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->MusicScreen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->MusicScreen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of MusicScreen.

    //Update current screen layout.
    lv_obj_update_layout(ui->MusicScreen);

    // 刷新一次“残月”初始效果（坐标已计算完成）
    ring_list_refresh(ui->MusicScreen_music_list);

    //Init events for screen.
    events_init_MusicScreen(ui);

    ui->MusicScreen_del = false;
}
/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

#include "music_ui.h"
#include "music_volume.h"
#include "music_progress.h"
#include "audio_player.h"
#include "music_spectrum.h"
#include "esp_random.h"
#include "esp_log.h"

/* ---------- MusicScreen 行为层：音量条交互 + 残月列表 + 进度显示 ---------- */
