#include "bl_ui.h"
#include "st7796_driver.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static int  s_bl_percent = 70;          // 默认 70（若 NVS 无记录）
static bool s_nvs_ready  = false;
static const char *TAG   = "bl_ui";

static inline void bl_apply_now(void) {
    st7796_set_backlight(BL_UI_PERCENT_TO_HW(s_bl_percent));
}

static esp_err_t bl_nvs_init_once(void) {
    if (s_nvs_ready) return ESP_OK;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err == ESP_OK) s_nvs_ready = true;
    return err;
}

static void bl_save_nvs(void) {
    if (bl_nvs_init_once() != ESP_OK) return;
    nvs_handle_t h;
    if (nvs_open("ui", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, "bl", (uint8_t)s_bl_percent);
        nvs_commit(h);
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "open NVS(ns=ui) failed, backlight not saved");
    }
}

static void bl_load_nvs(void) {
    if (bl_nvs_init_once() != ESP_OK) return;
    nvs_handle_t h;
    uint8_t v;
    if (nvs_open("ui", NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_u8(h, "bl", &v) == ESP_OK) s_bl_percent = v;
        nvs_close(h);
    } else {
        ESP_LOGI(TAG, "no NVS(ns=ui), use default %d%%", s_bl_percent);
    }
}

void bl_ui_init(void) {
    bl_load_nvs();    // 从 NVS 读
    bl_apply_now();   // 应用到背光
}

void bl_ui_set_percent(int percent, bool apply_now) {
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    if (s_bl_percent == percent) return;
    s_bl_percent = percent;
    if (apply_now) bl_apply_now();
}

int bl_ui_get_percent(void) {
    return s_bl_percent;
}

static void arc_evt_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *arc = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int v = lv_arc_get_value(arc);
        bl_ui_set_percent(v, true);    // 拖动时实时改亮度（不写 NVS，减少擦写）
    } else if (code == LV_EVENT_RELEASED) {
        bl_save_nvs();                 // 松手再写入 NVS
    }
}

void bl_ui_bind_arc(lv_obj_t *arc) {
    lv_obj_add_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arc, arc_evt_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(arc, arc_evt_cb, LV_EVENT_RELEASED, NULL);

    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, s_bl_percent);  // 显示为“记忆值”
    bl_apply_now();                       // 保证硬件与界面一致
}

