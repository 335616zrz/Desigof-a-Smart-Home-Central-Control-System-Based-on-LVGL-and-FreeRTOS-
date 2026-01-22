/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include "gui_guider.h"
#include "widgets_init.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"


__attribute__((unused)) void kb_event_cb (lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

__attribute__((unused)) void ta_event_cb (lv_event_t *e) {
#if LV_USE_KEYBOARD
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);

    if(code == LV_EVENT_FOCUSED) {
        if(lv_indev_get_type(lv_indev_active()) != LV_INDEV_TYPE_KEYPAD) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    } else if(code == LV_EVENT_READY) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta);
    } else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
#endif
}

void clock_count(int *hour, int *min, int *sec)
{
    (*sec)++;
    if(*sec == 60)
    {
        *sec = 0;
        (*min)++;
    }
    if(*min == 60)
    {
        *min = 0;
        if(*hour < 12)
        {
            (*hour)++;
        } else {
            (*hour)++;
            *hour = *hour %12;
        }
    }
}

/* 中/En 切换：关闭/开启 IME 对键盘的接管 */
/* 核心切换实现：复用到屏内按钮和键盘内置按钮 */
static void ime_toggle_common(lv_ui *ui)
{
#if LV_USE_IME_PINYIN
    static bool s_chinese = false; /* 默认英文输入（直输），点击后切到中文拼音 */
    /* 优先使用 GPT 屏内的键盘/IME，若不存在则回退到顶层 */
    lv_obj_t *kb = ui->screen_7_kb ? ui->screen_7_kb : ui->g_kb_top_layer;
#if LV_USE_IME_PINYIN
    lv_obj_t *ime = ui->screen_7_ime ? ui->screen_7_ime : ui->screen_7_pinyin_ime;
#endif
    if(!kb || !ime) return;

    if(s_chinese) {
        /* 当前中文 -> 切到英文直输 */
        if(ui->g_kb_lang_btn_label && lv_obj_is_valid(ui->g_kb_lang_btn_label))
            lv_label_set_text(ui->g_kb_lang_btn_label, "En");
        if(ui->screen_7_kb_lang_btn_label && lv_obj_is_valid(ui->screen_7_kb_lang_btn_label))
            lv_label_set_text(ui->screen_7_kb_lang_btn_label, "En");
        if(ui->screen_7_lang_btn_label && lv_obj_is_valid(ui->screen_7_lang_btn_label))
            lv_label_set_text(ui->screen_7_lang_btn_label, "En");
        /* 不再尝试解绑回调，直接把 IME 切到一个不会处理字母的模式，并隐藏候选栏 */
        lv_ime_pinyin_set_mode(ime, LV_IME_PINYIN_MODE_K9_NUMBER);
        lv_obj_add_flag(lv_ime_pinyin_get_cand_panel(ime), LV_OBJ_FLAG_HIDDEN);
        /* 确保键盘仍然绑定在当前文本框上（英文直输） */
        lv_keyboard_set_textarea(kb, ui->screen_7_input_ta);
        ESP_LOGI("IME", "switch -> EN, kb=%p ta=%p", (void*)kb, (void*)ui->screen_7_input_ta);
        s_chinese = false;
    } else {
        /* 当前英文 -> 切回中文拼音 */
        if(ui->g_kb_lang_btn_label && lv_obj_is_valid(ui->g_kb_lang_btn_label))
            lv_label_set_text(ui->g_kb_lang_btn_label, "中");
        if(ui->screen_7_kb_lang_btn_label && lv_obj_is_valid(ui->screen_7_kb_lang_btn_label))
            lv_label_set_text(ui->screen_7_kb_lang_btn_label, "中");
        if(ui->screen_7_lang_btn_label && lv_obj_is_valid(ui->screen_7_lang_btn_label))
            lv_label_set_text(ui->screen_7_lang_btn_label, "中");
        /* 注意：lv_ime_pinyin_set_keyboard() 会给键盘追加 VALUE_CHANGED 回调。
         * 反复调用会导致回调重复注册，进而出现候选异常/删字错乱。
         * GPT 屏内键盘在 setup 时已绑定，这里只在键盘对象真的变化时才重新绑定。 */
        if(lv_ime_pinyin_get_kb(ime) != kb) {
            lv_ime_pinyin_set_keyboard(ime, kb);
        }
        lv_ime_pinyin_set_mode(ime, LV_IME_PINYIN_MODE_K26);  /* 26 键拼音模式 */
        lv_obj_clear_flag(lv_ime_pinyin_get_cand_panel(ime), LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(kb, ui->screen_7_input_ta);
        ESP_LOGI("IME", "switch -> CN, kb=%p ta=%p", (void*)kb, (void*)ui->screen_7_input_ta);
        s_chinese = true;
    }
#else
    LV_UNUSED(ui);
#endif
}

void screen_7_lang_btn_event_handler(lv_event_t * e)
{
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    if(!ui) return;
    ime_toggle_common(ui);
}

void kb_lang_btn_event_cb(lv_event_t * e)
{
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    if(!ui) return;
    ime_toggle_common(ui);
}

/* Keyboard key: "中/En" (as part of buttonmatrix map). */
void kb_lang_key_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    lv_ui * ui = (lv_ui *)lv_event_get_user_data(e);
    if(!ui) return;

    lv_obj_t * kb = lv_event_get_current_target(e);
    if(!kb) return;

    uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb);
    if(btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;

    const char * txt = lv_buttonmatrix_get_button_text(kb, btn_id);
    if(!txt) return;

    if(lv_strcmp(txt, "中/En") == 0) {
        ime_toggle_common(ui);
        /* Prevent lv_keyboard_def_event_cb from inserting the key text into textarea. */
        lv_event_stop_processing(e);
    }
}

void digital_clock_count(int * hour, int * minute, int * seconds, char * meridiem)
{

    (*seconds)++;
    if(*seconds == 60) {
        *seconds = 0;
        (*minute)++;
    }
    if(*minute == 60) {
        *minute = 0;
        if(*hour < 12) {
            (*hour)++;
        }
        else {
            (*hour)++;
            (*hour) = (*hour) % 12;
        }
    }
    if(*hour == 12 && *seconds == 0 && *minute == 0) {
        if((lv_strcmp(meridiem, "PM") == 0)) {
            lv_strcpy(meridiem, "AM");
        }
        else {
            lv_strcpy(meridiem, "PM");
        }
    }
}
