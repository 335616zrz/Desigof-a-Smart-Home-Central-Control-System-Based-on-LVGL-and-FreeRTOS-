/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "lvgl.h"
#include "custom.h"
#include "lvgl_port.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#if CONFIG_MY_UI_CHATBOT_ENABLE
#include "tts_player.h"
#endif

#include "app_inmp441.h"       // ASR 启用/禁用控制
#include "weather_persist.h"
#include "chatbot_client.h"
#include "chat_ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/* 串行化 TLS 握手 / 会话创建，避免音乐索引与 Chatbot 同时抢占堆/DMA 资源 */
static SemaphoreHandle_t s_tls_mutex = NULL;

/* 室内/室外模式标志：false=室外(默认，使用天气服务数据)，true=室内(使用 SHT40) */
static bool s_env_indoor = false;

/* 室内环境显示定时器：定期从 app_env_sht40 读取温湿度并刷新首页标签 */
static lv_timer_t *s_env_timer = NULL;

/* 定时器回调：在“室内”模式下从 app_env_sht40 读取温湿度并刷新首页标签 */
static void env_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (!env_mode_is_indoor()) {
        return;
    }

    float t_c = 0.0f, rh = 0.0f;
    if (!app_env_sht40_get_latest(&t_c, &rh)) {
        return;
    }

    extern lv_ui guider_ui;
    if (!lvgl_port_lock(10)) {
        return;
    }

    /* 同步更新浅色/深色首页上的温湿度数字（若对象仍然有效） */
    char tbuf[8];
    char hbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%.0f", (double)t_c);
    snprintf(hbuf, sizeof(hbuf), "%.0f", (double)rh);

    if (guider_ui.screen_label_5 && guider_ui.screen_label_9 &&
        lv_obj_is_valid(guider_ui.screen_label_5) &&
        lv_obj_is_valid(guider_ui.screen_label_9)) {
        lv_label_set_text(guider_ui.screen_label_5, tbuf);
        lv_label_set_text(guider_ui.screen_label_9, hbuf);
    }

    if (guider_ui.screen_1_label_5 && guider_ui.screen_1_label_9 &&
        lv_obj_is_valid(guider_ui.screen_1_label_5) &&
        lv_obj_is_valid(guider_ui.screen_1_label_9)) {
        lv_label_set_text(guider_ui.screen_1_label_5, tbuf);
        lv_label_set_text(guider_ui.screen_1_label_9, hbuf);
    }

    lvgl_port_unlock();
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a demo application
 */
void custom_init(lv_ui *ui)
{
    /* 根据上次保存的模式恢复“室内/室外”状态，并刷新首页标签文字 */
    bool indoor = false;
    (void)weather_persist_load_env_mode(&indoor);
    env_mode_set_indoor(indoor);

    if (ui && ui->screen_label_12 && lv_obj_is_valid(ui->screen_label_12)) {
        const char *txt = indoor ? "室\n内" : "室\n外";
        lv_label_set_text(ui->screen_label_12, txt);
    }
    /* 创建一个 LVGL 定时器：在“室内”模式下从 app_env_sht40 读取并刷新首页温湿度 */
    if (!s_env_timer) {
        s_env_timer = lv_timer_create(env_timer_cb, 1000, NULL);
    }
}

void env_mode_set_indoor(bool indoor)
{
    s_env_indoor = indoor;
}

bool env_mode_is_indoor(void)
{
    return s_env_indoor;
}

/* 进入音乐页：如有正在播报的 TTS，优先停止，避免与音乐播放争用 I2S */
void AppAudio_OnEnterMusicScreen(void)
{
#if CONFIG_MY_UI_CHATBOT_ENABLE
    if (tts_player_is_active()) {
        tts_player_stop_now();
    }
#endif
}

/* 进入 GPT 聊天页：如音乐正在播放或处于加载态，则先暂停/停止音乐，并启用 ASR */
void AppAudio_OnEnterGPTScreen(void)
{
#if CONFIG_APP_ENABLE_MUSIC
    ap_state_t st = ap_get_state();
    if (st == AP_STATE_PLAYING ||
        st == AP_STATE_PAUSED  ||
        st == AP_STATE_LOADING) {
        (void)ap_stop();
    }
#endif
}

/* 离开 GPT 聊天页：禁用 ASR，释放 WebSocket 资源 */
void AppAudio_OnLeaveGPTScreen(void)
{
    /* 保险：退出 GPT 页时确保 PTT 已松开 */
    app_inmp441_asr_ptt_set_pressed(false);
    /* 禁用 ASR 语音识别 */
    app_inmp441_asr_set_enabled(false);
}

void AppASR_SetEnabled(bool enabled)
{
    app_inmp441_asr_set_enabled(enabled);
}

void AppASR_SetPTTMode(bool enabled)
{
    app_inmp441_asr_set_ptt_mode(enabled);
}

void AppASR_PTT_SetPressed(bool pressed)
{
    app_inmp441_asr_ptt_set_pressed(pressed);
}

void AppTLS_Lock(void)
{
    if (!s_tls_mutex) {
        s_tls_mutex = xSemaphoreCreateMutex();
    }
    if (s_tls_mutex) {
        (void)xSemaphoreTake(s_tls_mutex, portMAX_DELAY);
    }
}

void AppTLS_Unlock(void)
{
    if (s_tls_mutex) {
        (void)xSemaphoreGive(s_tls_mutex);
    }
}

/* ASR final -> GPT chat UI + trigger LLM reply.
 * This overrides the weak hook in app_inmp441. */
void app_inmp441_on_asr_final(const char *utf8_text)
{
    if (!utf8_text || !utf8_text[0]) {
        return;
    }

    extern lv_ui guider_ui;

    /* 1) Append to chat UI as user message (right bubble) */
    chat_ui_add_user(utf8_text);

    /* 2) Keep input box clean (even if hidden in voice mode) */
    if (lvgl_port_lock(2000)) {
        if (guider_ui.screen_7_input_ta && lv_obj_is_valid(guider_ui.screen_7_input_ta)) {
            lv_textarea_set_text(guider_ui.screen_7_input_ta, "");
        }
        lvgl_port_unlock();
    }

    /* 3) Trigger LLM reply */
#if CONFIG_MY_UI_CHATBOT_ENABLE
    chatbot_send_text(utf8_text);
#else
    (void)chatbot_send_text;
#endif
}
