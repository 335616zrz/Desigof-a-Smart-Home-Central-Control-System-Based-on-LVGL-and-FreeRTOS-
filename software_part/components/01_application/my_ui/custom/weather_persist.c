#include "weather_persist.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "WEATHER_PERSIST";
static const char *NS  = "weather";

static nvs_handle_t open_ns(bool write)
{
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(NS, write ? NVS_READWRITE : NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open('%s') failed: %s", NS, esp_err_to_name(err));
        return 0;
    }
    return h;
}

bool weather_persist_load(uint32_t *prov, uint32_t *pref, uint32_t *county)
{
    // Initialize outputs explicitly on separate lines to avoid -Wmisleading-indentation
    if (prov)   *prov   = 0;
    if (pref)   *pref   = 0;
    if (county) *county = 0;
    nvs_handle_t h = open_ns(false);
    if (!h) return false;
    uint32_t p=0, c=0, r=0;
    esp_err_t e1 = nvs_get_u32(h, "prov", &p);
    esp_err_t e2 = nvs_get_u32(h, "pref", &r);
    esp_err_t e3 = nvs_get_u32(h, "county", &c);
    nvs_close(h);
    if (prov) *prov = (e1 == ESP_OK) ? p : 0;
    if (pref) *pref = (e2 == ESP_OK) ? r : 0;
    if (county) *county = (e3 == ESP_OK) ? c : 0;
    return (e1 == ESP_OK) && (e2 == ESP_OK) && (e3 == ESP_OK);
}

bool weather_persist_save(uint32_t prov, uint32_t pref, uint32_t county)
{
    nvs_handle_t h = open_ns(true);
    if (!h) return false;
    bool ok = true;
    if (nvs_set_u32(h, "prov", prov) != ESP_OK) ok = false;
    if (nvs_set_u32(h, "pref", pref) != ESP_OK) ok = false;
    if (nvs_set_u32(h, "county", county) != ESP_OK) ok = false;
    if (ok && nvs_commit(h) != ESP_OK) ok = false;
    nvs_close(h);
    return ok;
}

bool weather_persist_load_env_mode(bool *indoor)
{
    if (indoor) *indoor = false;
    nvs_handle_t h = open_ns(false);
    if (!h) return false;
    uint8_t v = 0;
    esp_err_t e = nvs_get_u8(h, "env_mode", &v);
    nvs_close(h);
    if (indoor && e == ESP_OK) {
        *indoor = (v != 0);
    }
    return (e == ESP_OK);
}

bool weather_persist_save_env_mode(bool indoor)
{
    nvs_handle_t h = open_ns(true);
    if (!h) return false;
    uint8_t v = indoor ? 1 : 0;
    bool ok = (nvs_set_u8(h, "env_mode", v) == ESP_OK);
    if (ok && nvs_commit(h) != ESP_OK) ok = false;
    nvs_close(h);
    return ok;
}
