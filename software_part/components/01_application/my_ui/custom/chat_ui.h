#pragma once
/*
 * Simple chat UI helpers for GPT screen:
 * - user messages align right
 * - assistant messages align left (streaming supported)
 *
 * Implemented on top of LVGL flex + a scrollable container created in setup_scr_GPT_screen*.c
 */

#include <stdbool.h>
#include "gui_guider.h" /* lv_ui is an anonymous typedef; must include full definition */

#ifdef __cplusplus
extern "C" {
#endif

/* Called when GPT screen (light/dark) is created/entered. */
void chat_ui_set_active(lv_ui *ui, bool dark_theme);

/* Add a user message (right aligned). */
void chat_ui_add_user(const char *utf8_text);

/* Assistant streaming lifecycle (left aligned). */
void chat_ui_assistant_start(void);
void chat_ui_assistant_append(const char *utf8_delta);
void chat_ui_assistant_finish(const char *utf8_final_full_text);

#ifdef __cplusplus
}
#endif
