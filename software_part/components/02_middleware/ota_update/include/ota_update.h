#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_UPDATE_EVT_START = 1,
    OTA_UPDATE_EVT_PROGRESS,   /* payload: int percent (0..100), -1 if unknown */
    OTA_UPDATE_EVT_SKIPPED,    /* payload: 0 */
    OTA_UPDATE_EVT_DONE,       /* payload: 0 */
    OTA_UPDATE_EVT_ERROR,      /* payload: esp_err_t */
} ota_update_event_t;

typedef void (*ota_update_cb_t)(ota_update_event_t evt, int payload, void *user_ctx);

typedef enum {
    OTA_UPDATE_CHECK_UNKNOWN = 0,
    OTA_UPDATE_CHECK_NO_UPDATE,
    OTA_UPDATE_CHECK_HAS_UPDATE,
    OTA_UPDATE_CHECK_ERROR,
} ota_update_check_result_t;

/**
 * @brief Start an HTTPS OTA update in a background task.
 *
 * Notes:
 * - The task will wait for Wi-Fi Got IP (via net_wait_ip()).
 * - On success, it will call esp_restart().
 * - If rollback is enabled, the *next boot* should confirm the image by calling
 *   esp_ota_mark_app_valid_cancel_rollback() from application code.
 *
 * @param url Firmware image URL (e.g. https://servers.local/firmware)
 */
esp_err_t ota_update_start(const char *url);

bool ota_update_is_running(void);

/**
 * @brief Start an OTA "version check" task (does not write flash).
 *
 * After the check completes, use ota_update_get_check_result() and
 * ota_update_get_new_version() to read the outcome.
 *
 * @param url Firmware image URL (e.g. https://servers.local/firmware)
 */
esp_err_t ota_update_check_start(const char *url);

bool ota_update_check_is_running(void);

ota_update_check_result_t ota_update_get_check_result(void);

esp_err_t ota_update_get_check_error(void);

/**
 * @brief Optional: register a callback to observe OTA progress/state.
 *
 * Callback is invoked from the OTA task context. If you need to touch LVGL,
 * use lv_async_call() or another thread-safe mechanism.
 */
void ota_update_set_callback(ota_update_cb_t cb, void *user_ctx);

/**
 * @brief Last known progress percent.
 *
 * @return 0..100, or -1 if unknown/not started.
 */
int ota_update_get_progress_percent(void);

/**
 * @brief Last error from OTA task.
 *
 * @return ESP_OK if none (or success), otherwise the failure esp_err_t.
 */
esp_err_t ota_update_get_last_error(void);

/**
 * @brief New firmware version (from esp_app_desc_t) if available.
 *
 * - Available after esp_https_ota_get_img_desc() succeeds.
 * - Returns an empty string if unknown/not started.
 *
 * @return Zero-terminated version string buffer owned by ota_update module.
 */
const char *ota_update_get_new_version(void);

/**
 * @brief Mark that user cancelled the OTA upgrade dialog.
 *
 * This prevents auto-popup for the rest of this boot cycle.
 */
void ota_mark_user_cancelled(void);

/**
 * @brief Check if auto-popup should be shown.
 *
 * @return true if new version available and user hasn't cancelled, false otherwise.
 */
bool ota_should_auto_popup(void);

#ifdef __cplusplus
}
#endif
