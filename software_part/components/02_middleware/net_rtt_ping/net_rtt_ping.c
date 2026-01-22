#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/param.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <math.h>

#include "esp_timer.h"
#include "ping/ping_sock.h"

#include "net_rtt_ping.h"

typedef struct {
    uint64_t t0_us;
    uint32_t tx;
    uint32_t rx;
    uint32_t min_ms;
    uint32_t max_ms;
    double   mean;   /* Welford */
    double   M2;
    bool     printed_header;
} stats_t;

static esp_ping_handle_t s_hdl = NULL;
static stats_t           s_stats;
static char              s_target_ip[IPADDR_STRLEN_MAX] = {0};  /* 目标 IP 文本，启动时一次解析 */

static inline uint32_t now_ms_from_boot(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static inline void stats_reset(stats_t *s) {
    memset(s, 0, sizeof(*s));
    s->min_ms = UINT32_MAX;
    s->t0_us  = esp_timer_get_time();
}

static inline void stats_update(stats_t *s, uint32_t rtt_ms, bool ok) {
    s->tx++;
    if (ok) {
        s->rx++;
        s->min_ms = MIN(s->min_ms, rtt_ms);
        s->max_ms = MAX(s->max_ms, rtt_ms);
        const double d  = (double)rtt_ms;
        const double d1 = d - s->mean;
        s->mean  += d1 / (double)s->rx;
        s->M2    += d1 * (d - s->mean);
    }
}

static void on_ping_success(esp_ping_handle_t hdl, void *args) {
    (void)args;
    uint8_t  ttl = 0;
    uint16_t seqno = 0;
    uint32_t elapsed_time = 0, recv_len = 0;

    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL,   &ttl,   sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE,  &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    stats_update(&s_stats, elapsed_time, true);

    if (!s_stats.printed_header) {
        printf("PING_CSV,ts_ms,seq,status,rtt_ms,bytes,ttl,ip\n");
        s_stats.printed_header = true;
    }
    printf("PING_CSV,%" PRIu32 ",%" PRIu16 ",ok,%" PRIu32 ",%" PRIu32 ",%" PRIu8 ",%s\n",
           now_ms_from_boot(), seqno, elapsed_time, recv_len, ttl, s_target_ip);
#if defined(CONFIG_NET_PING_CSV_FLUSH_EACH) && CONFIG_NET_PING_CSV_FLUSH_EACH
    fflush(stdout);
#endif
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    (void)hdl; (void)args;
    uint16_t seqno = 0;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));

    stats_update(&s_stats, 0, false);

    if (!s_stats.printed_header) {
        printf("PING_CSV,ts_ms,seq,status,rtt_ms,bytes,ttl,ip\n");
        s_stats.printed_header = true;
    }
    printf("PING_CSV,%" PRIu32 ",%" PRIu16 ",timeout,,,,%s\n",
           now_ms_from_boot(), seqno, s_target_ip);
#if defined(CONFIG_NET_PING_CSV_FLUSH_EACH) && CONFIG_NET_PING_CSV_FLUSH_EACH
    fflush(stdout);
#endif
}

static void on_ping_end(esp_ping_handle_t hdl, void *args) {
    (void)args;
    uint32_t transmitted=0, received=0, duration_ms=0;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST,  &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY,    &received,    sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &duration_ms, sizeof(duration_ms));

    const double avg = (s_stats.rx > 0) ? s_stats.mean : 0.0;
    const double std = (s_stats.rx > 1) ? sqrt(s_stats.M2 / (double)(s_stats.rx - 1)) : 0.0;
    const uint32_t minv = (s_stats.rx > 0) ? s_stats.min_ms : 0;
    const uint32_t maxv = (s_stats.rx > 0) ? s_stats.max_ms : 0;
    const double loss = (transmitted > 0) ? (100.0 * (transmitted - received) / (double)transmitted) : 0.0;

    printf("PING_SUM,tx,rx,loss_pct,min_ms,avg_ms,max_ms,std_ms,duration_ms\n");
    printf("PING_SUM,%" PRIu32 ",%" PRIu32 ",%.1f,%" PRIu32 ",%.2f,%" PRIu32 ",%.2f,%" PRIu32 "\n",
           transmitted, received, loss, minv, avg, maxv, std, duration_ms);
    fflush(stdout);

    esp_ping_stop(hdl);
    esp_ping_delete_session(hdl);
    s_hdl = NULL;
}

static bool resolve_target(const char *host, ip_addr_t *out) {
    memset(out, 0, sizeof(*out));
    struct addrinfo hint = {0}, *res = NULL;
    if (getaddrinfo(host, NULL, &hint, &res) == 0 && res) {
        if (res->ai_addr && res->ai_addr->sa_family == AF_INET) {
            struct in_addr a4 = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
            inet_addr_to_ip4addr(ip_2_ip4(out), &a4);
            freeaddrinfo(res);
            return true;
        }
        freeaddrinfo(res);
    }
    ip4_addr_t ip4;
    if (ip4addr_aton(host, &ip4)) {
        ip_addr_copy_from_ip4(*out, ip4);
        return true;
    }
    return false;
}

esp_err_t ping_csv_start(const net_ping_csv_cfg_t *cfg) {
    if (!cfg || !cfg->target || cfg->interval_ms == 0 || cfg->timeout_ms == 0)
        return ESP_ERR_INVALID_ARG;

    if (s_hdl) {
        esp_ping_stop(s_hdl);
        esp_ping_delete_session(s_hdl);
        s_hdl = NULL;
    }
    ip_addr_t target_addr;
    if (!resolve_target(cfg->target, &target_addr)) {
        return ESP_FAIL;
    }

    /* 预先文本化 IP，后续回调直接复用，避免重复转换 */
    ipaddr_ntoa_r(&target_addr, s_target_ip, sizeof(s_target_ip));

    stats_reset(&s_stats);
    s_stats.printed_header = cfg->print_header;

    esp_ping_config_t pcfg = ESP_PING_DEFAULT_CONFIG();
    pcfg.target_addr = target_addr;
    pcfg.count       = cfg->count;       /* 0=无限 */
    pcfg.interval_ms = cfg->interval_ms;
    pcfg.timeout_ms  = cfg->timeout_ms;
    pcfg.data_size   = cfg->data_size;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end     = on_ping_end,
        .cb_args         = NULL,
    };

    esp_err_t err = esp_ping_new_session(&pcfg, &cbs, &s_hdl);
    if (err != ESP_OK) return err;
    return esp_ping_start(s_hdl);
}

bool ping_csv_running(void) { return s_hdl != NULL; }

void ping_csv_stop(void) {
    if (s_hdl) {
        esp_ping_stop(s_hdl);
        esp_ping_delete_session(s_hdl);
        s_hdl = NULL;
    }
}
