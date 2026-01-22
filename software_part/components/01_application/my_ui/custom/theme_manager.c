#include "theme_manager.h"

#include "esp_log.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "THEME_MGR";
static nvs_handle_t s_nvs = 0;
static bool current_theme_dark = false;

static void apply_theme(bool is_dark)
{
    /* Re-init default theme with desired flag; uses global display by passing NULL */
    lv_theme_default_init(NULL,
                          lv_palette_main(LV_PALETTE_BLUE),
                          lv_palette_main(LV_PALETTE_RED),
                          is_dark,
                          LV_FONT_DEFAULT);

    lv_obj_invalidate(lv_scr_act());
    current_theme_dark = is_dark;
}

void theme_manager_init(void)
{
    /* NVS is typically inited by app_main; here we only open the namespace */
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    uint8_t saved = 0;
    err = nvs_get_u8(s_nvs, "theme", &saved);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Restoring theme from NVS: %s", saved ? "dark" : "light");
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No stored theme, default to light");
        saved = 0;
    } else {
        ESP_LOGW(TAG, "Read theme failed: %s", esp_err_to_name(err));
        saved = 0;
    }

    apply_theme(saved == 1);
}

void my_ui_set_theme(bool is_dark)
{
    ESP_LOGI(TAG, "Switching to %s theme", is_dark ? "dark" : "light");
    apply_theme(is_dark);

    if (s_nvs != 0) {
        esp_err_t err = nvs_set_u8(s_nvs, "theme", is_dark ? 1 : 0);
        if (err == ESP_OK) {
            nvs_commit(s_nvs);
        } else {
            ESP_LOGW(TAG, "Persist theme failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "NVS handle not ready, skip persist");
    }
}

bool my_ui_get_current_theme(void)
{
    return current_theme_dark;
}
