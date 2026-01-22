#pragma once

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create (or recreate) an OTA upgrade dialog on the given parent screen.
 *
 * The dialog is created hidden by default. Use ota_upgrade_dialog_show() to show it.
 *
 * @param parent    Screen object (e.g. ui->screen / ui->screen_1)
 * @param dark_mode true for dark theme, false for light theme
 */
void ota_upgrade_dialog_create(lv_obj_t *parent, bool dark_mode);

/**
 * @brief Bind an "open OTA dialog" button.
 *
 * Clicking the button will open the dialog (version defaults to "--" until OTA reads image desc).
 *
 * @param btn       LVGL button object
 * @param dark_mode true for dark theme, false for light theme
 */
void ota_upgrade_dialog_bind_open_button(lv_obj_t *btn, bool dark_mode);

/**
 * @brief Show the dialog and reset UI state.
 *
 * @param dark_mode true for dark theme, false for light theme
 * @param target_version Optional. If NULL/empty, shows "--" and waits for OTA to report.
 */
void ota_upgrade_dialog_show(bool dark_mode, const char *target_version);

/**
 * @brief Hide the dialog (and stop progress timer if running).
 */
void ota_upgrade_dialog_hide(bool dark_mode);

/**
 * @brief Arm an auto-popup: when OTA check finds a new version, show the dialog once.
 *
 * Call this after Wi-Fi gets IP (or when network becomes available).
 */
void ota_upgrade_dialog_arm_auto_popup(void);

#ifdef __cplusplus
}
#endif
