#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"   /* PSRAM 优先分配 */
#include "net_wifi_scan.h"

static const char *TAG = "net_wifi_scan";

static inline const char *auth_to_str(wifi_auth_mode_t a)
{
    switch (a) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-SAE";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "UNKNOWN";
    }
}

static inline const char *cipher_to_str(wifi_cipher_type_t c)
{
    switch (c) {
    case WIFI_CIPHER_TYPE_NONE:   return "NONE";
    case WIFI_CIPHER_TYPE_WEP40:  return "WEP40";
    case WIFI_CIPHER_TYPE_WEP104: return "WEP104";
    case WIFI_CIPHER_TYPE_TKIP:   return "TKIP";
    case WIFI_CIPHER_TYPE_CCMP:   return "CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP: return "TKIP/CCMP";
    case WIFI_CIPHER_TYPE_AES_CMAC128: return "AES-CMAC";
    default:                      return "UNK";
    }
}

static inline const char *secchan_to_str(wifi_second_chan_t s)
{
    switch (s) {
    case WIFI_SECOND_CHAN_NONE:  return "NONE";
    case WIFI_SECOND_CHAN_ABOVE: return "ABOVE";
    case WIFI_SECOND_CHAN_BELOW: return "BELOW";
    default:                     return "UNK";
    }
}

static inline void phy_flags_to_str(const wifi_ap_record_t *ap, char *buf, size_t n)
{
    size_t off = 0;
    off += snprintf(buf+off, n-off, "%s", ap->phy_11b ? "b" : "");
    off += snprintf(buf+off, n-off, "%s%s", (off && ap->phy_11g) ? "/" : "", ap->phy_11g ? "g" : "");
    off += snprintf(buf+off, n-off, "%s%s", (off && ap->phy_11n) ? "/" : "", ap->phy_11n ? "n" : "");
    off += snprintf(buf+off, n-off, "%s%s", (off && ap->phy_11ax)? "/" : "", ap->phy_11ax? "ax" : "");
    off += snprintf(buf+off, n-off, "%s%s", (off && ap->phy_lr) ? "/" : "", ap->phy_lr ? "lr" : "");
    if (off == 0) snprintf(buf, n, "na");
}

static inline void* scan_recs_alloc(uint16_t num)
{
#ifdef CONFIG_NET_WIFI_SCAN_USE_PSRAM
    void *p = heap_caps_calloc(num, sizeof(wifi_ap_record_t), MALLOC_CAP_SPIRAM);
    if (p) return p;
#endif
    return calloc(num, sizeof(wifi_ap_record_t));
}

void net_wifi_scan_dump_csv(void)
{
    const wifi_scan_config_t cfg = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
#if defined(CONFIG_NET_WIFI_SCAN_SHOW_HIDDEN) && CONFIG_NET_WIFI_SCAN_SHOW_HIDDEN
        .show_hidden = true,
#else
        .show_hidden = false,
#endif
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min =
#if defined(CONFIG_NET_WIFI_SCAN_ACTIVE_MIN_MS)
            CONFIG_NET_WIFI_SCAN_ACTIVE_MIN_MS,
#else
            100,
#endif
        .scan_time.active.max =
#if defined(CONFIG_NET_WIFI_SCAN_ACTIVE_MAX_MS)
            CONFIG_NET_WIFI_SCAN_ACTIVE_MAX_MS,
#else
            300,
#endif
    };

    const int64_t t0 = esp_timer_get_time() / 1000; // ms
    ESP_ERROR_CHECK(esp_wifi_scan_start(&cfg, true)); // 阻塞等待完成

    uint16_t num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&num));
    if (num == 0) {
        printf("SCAN_SUM,aps=0,ts_ms=%" PRId64 ",note=none_found\n", t0);
        return;
    }

    wifi_ap_record_t *recs = (wifi_ap_record_t *)scan_recs_alloc(num);
    if (!recs) {
        ESP_LOGE(TAG, "no mem for %u recs", (unsigned)num);
        return;
    }
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&num, recs));

    int min_rssi = 127, max_rssi = -127;
    double sum = 0.0;

    /* 统一 CSV 表头（一次） */
    printf("SCAN_HDR,idx,ssid,bssid,rssi,primary,second,auth,pair,group,phy,wps,ftm\n");

    for (uint16_t i = 0; i < num; ++i) {
        wifi_ap_record_t *ap = &recs[i];
        if (ap->rssi < min_rssi) min_rssi = ap->rssi;
        if (ap->rssi > max_rssi) max_rssi = ap->rssi;
        sum += ap->rssi;

        char bssid[18];
        snprintf(bssid, sizeof(bssid), "%02x:%02x:%02x:%02x:%02x:%02x",
                 ap->bssid[0], ap->bssid[1], ap->bssid[2],
                 ap->bssid[3], ap->bssid[4], ap->bssid[5]);

        char phy[24];
        phy_flags_to_str(ap, phy, sizeof(phy));

        printf("SCAN_CSV,%u,%s,%s,%d,%u,%s,%s,%s,%s,%s,%d,%u\n",
               (unsigned)i,
               (const char*)ap->ssid,
               bssid,
               ap->rssi,
               (unsigned)ap->primary,
               secchan_to_str(ap->second),
               auth_to_str(ap->authmode),
               cipher_to_str(ap->pairwise_cipher),
               cipher_to_str(ap->group_cipher),
               phy,
               (int)ap->wps,
               (unsigned)ap->ftm_responder);
    }

    const double avg = sum / (double)num;
    const int64_t t1 = esp_timer_get_time() / 1000; // ms
    printf("SCAN_SUM,aps=%u,min_rssi=%d,avg_rssi=%.1f,max_rssi=%d,ts_ms=%" PRId64 ",elapsed_ms=%" PRId64 "\n",
           (unsigned)num, min_rssi, avg, max_rssi, t0, (t1 - t0));

    free(recs);
}

void net_wifi_scan_dump_csv_if_enabled(void)
{
#ifdef CONFIG_NET_WIFI_SCAN_ON_BOOT
    net_wifi_scan_dump_csv();
#endif
}

