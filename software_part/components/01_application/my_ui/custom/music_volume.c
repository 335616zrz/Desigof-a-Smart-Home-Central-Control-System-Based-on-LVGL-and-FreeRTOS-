#include "music_volume.h"

#include "sdkconfig.h"

#if CONFIG_APP_ENABLE_MUSIC
#include "audio_player.h"
#endif

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include <stdlib.h>

// 默认音量保持 10%，由用户根据环境调节。
// 增加防呆：若 NVS 里存的是 0（历史误触/异常写入），
// 将在加载阶段钳到该默认值，避免“开机即静音”误判为无声。
#define MUSIC_VOLUME_DEFAULT 10

static const char *TAG = "music_volume";

static int       s_volume_percent   = MUSIC_VOLUME_DEFAULT;
static bool      s_nvs_ready        = false;
static bool      s_init_done        = false;
static bool      s_audio_ready      = false;
static lv_obj_t *s_slider           = NULL;
static bool      s_slider_updating  = false;

static esp_err_t volume_nvs_init_once(void)
{
    if (s_nvs_ready) return ESP_OK;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err == ESP_OK) {
        s_nvs_ready = true;
    } else {
        ESP_LOGE(TAG, "nvs init failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void volume_load_nvs(void)
{
    if (volume_nvs_init_once() != ESP_OK) return;
    nvs_handle_t h;
    uint8_t v;
    if (nvs_open("ui", NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_u8(h, "music_vol", &v) == ESP_OK) {
            s_volume_percent = v;
            if (s_volume_percent == 0) {
                // 防止“静音陷阱”：将 0 钳到默认值并回写
                ESP_LOGW(TAG, "saved volume=0 -> clamp to default %d%%", MUSIC_VOLUME_DEFAULT);
                s_volume_percent = MUSIC_VOLUME_DEFAULT;
                // 回写到 NVS
                nvs_close(h);
                if (nvs_open("ui", NVS_READWRITE, &h) == ESP_OK) {
                    nvs_set_u8(h, "music_vol", (uint8_t)s_volume_percent);
                    nvs_commit(h);
                }
            }
        }
        nvs_close(h);
    } else {
        ESP_LOGI(TAG, "no saved volume, use default %d%%", MUSIC_VOLUME_DEFAULT);
    }
}

void music_volume_commit(void)
{
    if (volume_nvs_init_once() != ESP_OK) return;
    nvs_handle_t h;
    if (nvs_open("ui", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, "music_vol", (uint8_t)s_volume_percent);
        nvs_commit(h);
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "open NVS(ns=ui) failed, volume not saved");
    }
}

static void apply_volume_now(void)
{
#if CONFIG_APP_ENABLE_MUSIC
    if (s_audio_ready) {
        ap_set_volume(s_volume_percent);
        ESP_LOGI(TAG, "apply volume=%d%%", s_volume_percent);
    }
#endif
}

void music_volume_init(void)
{
    if (s_init_done) return;
    s_init_done = true;

    volume_load_nvs();
#if CONFIG_APP_ENABLE_MUSIC
    ap_set_initial_volume(s_volume_percent);
#endif
}

void music_volume_on_audio_ready(void)
{
    music_volume_init();
    s_audio_ready = true;
    apply_volume_now();
}

int music_volume_get_percent(void)
{
    music_volume_init();
    return s_volume_percent;
}

void music_volume_set_percent(int percent, bool from_ui)
{
    if (!s_init_done) {
        music_volume_init();
    }
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (s_volume_percent == percent) {
        return;
    }
    s_volume_percent = percent;

    if (s_slider && !from_ui) {
        s_slider_updating = true;
        lv_slider_set_value(s_slider, percent, LV_ANIM_OFF);
        s_slider_updating = false;
    }

    apply_volume_now();
}

static void slider_evt_cb(lv_event_t *e)
{
    if (s_slider_updating) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_PRESSING) {
        int v = lv_slider_get_value(slider);
        music_volume_set_percent(v, true);
    } else if (code == LV_EVENT_RELEASED) {
        music_volume_commit();
    }
}

void music_volume_bind_slider(lv_obj_t *slider)
{
    if (!slider) return;
    music_volume_init();
    s_slider = slider;

    lv_slider_set_range(slider, 0, 100);
    s_slider_updating = true;
    lv_slider_set_value(slider, s_volume_percent, LV_ANIM_OFF);
    s_slider_updating = false;

    lv_obj_add_event_cb(slider, slider_evt_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(slider, slider_evt_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(slider, slider_evt_cb, LV_EVENT_RELEASED, NULL);

    apply_volume_now();
}
