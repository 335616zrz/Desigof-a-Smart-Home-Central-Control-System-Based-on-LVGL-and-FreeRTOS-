#include "chat_ui.h"

#include <string.h>
#include <stdlib.h>

#include "esp_timer.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "gui_guider.h"

/* Active UI context (updated when GPT screen is created) */
static lv_ui *s_ui = NULL;
static bool  s_dark = false;

/* Assistant streaming state */
static lv_obj_t *s_assistant_label = NULL;
static bool      s_assistant_has_delta = false;

/* Forward declarations */
static lv_obj_t *get_chat_list(void);
static void scroll_list_to_bottom(lv_obj_t *list);

/* Coalesce llm.delta updates to reduce LVGL redraw pressure (helps TTS smoothness). */
static char    *s_delta_pending = NULL;
static size_t   s_delta_pending_len = 0;
static size_t   s_delta_pending_cap = 0;
static int64_t  s_delta_last_flush_us = 0;

static void delta_pending_reset(void)
{
    if (s_delta_pending) {
        free(s_delta_pending);
        s_delta_pending = NULL;
    }
    s_delta_pending_len = 0;
    s_delta_pending_cap = 0;
    s_delta_last_flush_us = 0;
}

static bool delta_pending_append(const char *utf8)
{
    if (!utf8 || !utf8[0]) return true;
    size_t add = strlen(utf8);
    if (add == 0) return true;

    size_t need = s_delta_pending_len + add + 1;
    if (need > s_delta_pending_cap) {
        size_t new_cap = s_delta_pending_cap ? s_delta_pending_cap : 256;
        while (new_cap < need) new_cap *= 2;
        char *p = (char *)realloc(s_delta_pending, new_cap);
        if (!p) return false;
        s_delta_pending = p;
        s_delta_pending_cap = new_cap;
    }

    memcpy(s_delta_pending + s_delta_pending_len, utf8, add);
    s_delta_pending_len += add;
    s_delta_pending[s_delta_pending_len] = '\0';
    return true;
}

static bool delta_should_flush(const char *just_appended)
{
    const int64_t now = esp_timer_get_time();
    const int64_t interval_us = 80 * 1000; /* ~12.5 fps */

    if (s_delta_pending_len == 0) return false;
    if (s_delta_last_flush_us == 0) return true; /* first paint */
    if ((now - s_delta_last_flush_us) >= interval_us) return true;

    /* Flush earlier at obvious text boundaries to feel responsive. */
    if (just_appended && strpbrk(just_appended, "\n。！？!?")) return true;

    /* Safety: avoid unbounded pending growth if lock is contended. */
    if (s_delta_pending_len >= 512) return true;

    return false;
}

static void delta_flush_to_label_locked(void)
{
    if (!s_delta_pending || s_delta_pending_len == 0) return;
    if (!s_assistant_label || !lv_obj_is_valid(s_assistant_label)) return;

    lv_label_ins_text(s_assistant_label, LV_LABEL_POS_LAST, s_delta_pending);
    s_assistant_has_delta = true;
    s_delta_pending_len = 0;
    s_delta_pending[0] = '\0';
    s_delta_last_flush_us = esp_timer_get_time();

    lv_obj_t *list = get_chat_list();
    scroll_list_to_bottom(list);
}

static lv_obj_t *get_chat_list(void)
{
    if (!s_ui) return NULL;
    /* screen_7_chat_list is created in setup_scr_GPT_screen*.c */
    if (!s_ui->screen_7_chat_list || !lv_obj_is_valid(s_ui->screen_7_chat_list)) return NULL;
    return s_ui->screen_7_chat_list;
}

static void scroll_list_to_bottom(lv_obj_t *list)
{
    if (!list || !lv_obj_is_valid(list)) return;
    lv_obj_update_layout(list);
    int32_t y = lv_obj_get_scroll_y(list) + lv_obj_get_scroll_bottom(list);
    lv_obj_scroll_to_y(list, y, LV_ANIM_OFF);
}

static void style_row_container(lv_obj_t *row)
{
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
}

static lv_obj_t *create_bubble_row(lv_obj_t *list, bool right_aligned, lv_obj_t **out_bubble)
{
    lv_obj_t *row = lv_obj_create(list);
    if (!row) return NULL;
    style_row_container(row);

    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
                          right_aligned ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    lv_obj_t *bubble = lv_obj_create(row);
    if (!bubble) return row;
    lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(bubble, 14, 0);
    lv_obj_set_style_pad_left(bubble, 10, 0);
    lv_obj_set_style_pad_right(bubble, 10, 0);
    lv_obj_set_style_pad_top(bubble, 8, 0);
    lv_obj_set_style_pad_bottom(bubble, 8, 0);
    lv_obj_set_style_border_width(bubble, 0, 0);

    if (out_bubble) *out_bubble = bubble;
    return row;
}

static void apply_bubble_style(lv_obj_t *bubble, bool is_user)
{
    if (!bubble || !lv_obj_is_valid(bubble)) return;

    /* Keep colors simple and readable on both light/dark backgrounds. */
    if (is_user) {
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x1583FF), 0);
    } else {
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bubble, s_dark ? lv_color_hex(0x152c3a) : lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(bubble, 1, 0);
        lv_obj_set_style_border_color(bubble, s_dark ? lv_color_hex(0x253244) : lv_color_hex(0xDCE7F5), 0);
    }
}

static void apply_label_style(lv_obj_t *label, bool is_user)
{
    if (!label || !lv_obj_is_valid(label)) return;
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, 340); /* wrap width; bubble stays aligned by row */
    lv_obj_set_style_text_font(label, &lv_font_cn_18, 0);
    if (is_user) {
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    } else {
        lv_obj_set_style_text_color(label, s_dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x0D3055), 0);
    }
}

void chat_ui_set_active(lv_ui *ui, bool dark_theme)
{
    s_ui = ui;
    s_dark = dark_theme;
    s_assistant_label = NULL;
    s_assistant_has_delta = false;
    delta_pending_reset();
}

void chat_ui_add_user(const char *utf8_text)
{
    if (!utf8_text || !utf8_text[0]) return;

    lv_obj_t *list = get_chat_list();
    if (!list) return;

    if (!lvgl_port_lock(2000)) {
        return;
    }

    /* Re-check after lock: screen might have been deleted while waiting. */
    list = get_chat_list();
    if (!list) {
        lvgl_port_unlock();
        return;
    }

    lv_obj_t *bubble = NULL;
    lv_obj_t *row = create_bubble_row(list, true /* right */, &bubble);
    if (bubble) {
        apply_bubble_style(bubble, true);
        lv_obj_t *label = lv_label_create(bubble);
        if (label) {
            apply_label_style(label, true);
            lv_label_set_text(label, utf8_text);
        }
    }

    (void)row;
    scroll_list_to_bottom(list);
    lvgl_port_unlock();
}

void chat_ui_assistant_start(void)
{
    lv_obj_t *list = get_chat_list();
    if (!list) return;

    delta_pending_reset();

    if (!lvgl_port_lock(2000)) {
        return;
    }

    list = get_chat_list();
    if (!list) {
        lvgl_port_unlock();
        return;
    }

    s_assistant_has_delta = false;
    s_assistant_label = NULL;

    lv_obj_t *bubble = NULL;
    lv_obj_t *row = create_bubble_row(list, false /* left */, &bubble);
    if (bubble) {
        apply_bubble_style(bubble, false);
        lv_obj_t *label = lv_label_create(bubble);
        if (label) {
            apply_label_style(label, false);
            lv_label_set_text(label, "");
            s_assistant_label = label;
        }
    }

    (void)row;
    scroll_list_to_bottom(list);
    lvgl_port_unlock();
}

void chat_ui_assistant_append(const char *utf8_delta)
{
    if (!utf8_delta || !utf8_delta[0]) return;

    /* If we missed llm.start, create a bubble now. */
    if (!s_assistant_label || !lv_obj_is_valid(s_assistant_label)) {
        chat_ui_assistant_start();
    }

    if (!s_assistant_label || !lv_obj_is_valid(s_assistant_label)) {
        return;
    }

    /* Always buffer first; UI flush is throttled to reduce redraw load. */
    (void)delta_pending_append(utf8_delta);

    if (!delta_should_flush(utf8_delta)) {
        return;
    }

    if (!lvgl_port_lock(2000)) {
        return;
    }

    delta_flush_to_label_locked();
    lvgl_port_unlock();
}

void chat_ui_assistant_finish(const char *utf8_final_full_text)
{
    /* If we missed llm.start (or the screen was re-created), ensure there is a bubble
     * to hold the final text. */
    if ((!s_assistant_label || !lv_obj_is_valid(s_assistant_label)) &&
        utf8_final_full_text && utf8_final_full_text[0]) {
        chat_ui_assistant_start();
    }

    if (!lvgl_port_lock(2000)) {
        /* Still close the state to avoid mixing next round into this bubble */
        s_assistant_label = NULL;
        s_assistant_has_delta = false;
        delta_pending_reset();
        return;
    }

    if (s_assistant_label && lv_obj_is_valid(s_assistant_label)) {
        /* Flush remaining buffered deltas first (for the case final is NULL). */
        delta_flush_to_label_locked();

        /* Final text is authoritative: even if some deltas were dropped due to lock
         * contention, replacing here makes the UI consistent with the server output. */
        if (utf8_final_full_text && utf8_final_full_text[0]) {
            lv_label_set_text(s_assistant_label, utf8_final_full_text);
        }
        lv_obj_t *list = get_chat_list();
        scroll_list_to_bottom(list);
    }

    /* Close assistant message */
    s_assistant_label = NULL;
    s_assistant_has_delta = false;
    delta_pending_reset();
    lvgl_port_unlock();
}
