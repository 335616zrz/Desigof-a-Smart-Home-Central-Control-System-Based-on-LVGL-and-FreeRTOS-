/* 启动屏：使用 tools/UI/start_screen_ui 的动画风格（移植到固件），并基于
 * STARTUP_READY(Wi‑Fi/Music) 进度条显示启动进度；后台自动尝试一次
 * "按 MRU 已保存网络连接"，完成（或超时）后切回首页。 */

#include "splash_screen.h"

#include "sdkconfig.h"

#include "gui_guider.h"
#include "bl_ui.h"
#include "start_screen.h"
#include "startup_ready.h"
#include "wifi_ui.h"
#include "weather_service_client.h"
#include "weather_persist.h"
#include "weather_city.h"

#include "lvgl.h"
#include "lvgl_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h" /* xTaskCreate*WithCaps / vTaskDeleteWithCaps */

#include "esp_log.h"

#include <stdbool.h>

static const char *TAG = "splash_ui";

static start_screen_t s_boot_ui = {0};
static lv_timer_t *s_boot_progress_timer = NULL;
static bool s_backlight_inited = false;

static int bits_to_progress(EventBits_t bits)
{
    int ready = 0;
    if (bits & STARTUP_READY_WIFI) {
        ready++;
    }
    if (bits & STARTUP_READY_MUSIC) {
        ready++;
    }
    return (ready * 100) / 2;
}

/* 在 LVGL 线程中定期刷新启动进度条 */
static void boot_progress_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (!s_boot_ui.bar || !lv_obj_is_valid(s_boot_ui.bar)) {
        return;
    }

    const EventBits_t bits = startup_ready_get();
    start_screen_set_progress(&s_boot_ui, bits_to_progress(bits));

    /* 若两项都 ready，则可以停止定时器 */
    const EventBits_t ALL = STARTUP_READY_WIFI | STARTUP_READY_MUSIC;
    if ((bits & ALL) == ALL && s_boot_progress_timer) {
        lv_timer_del(s_boot_progress_timer);
        s_boot_progress_timer = NULL;
    }
}

static void route_to_home(void *arg)
{
    ESP_LOGI(TAG, "route_to_home called, arg=%p", arg);
    lv_ui *ui = (lv_ui*)arg;
    if (!ui) {
        ESP_LOGE(TAG, "route_to_home: ui is NULL!");
        return;
    }

    /* 在切换屏幕前，必须先清理定时器和动画，避免访问已删除的对象 */
    if (s_boot_progress_timer) {
        lv_timer_del(s_boot_progress_timer);
        s_boot_progress_timer = NULL;
    }
    /* 停止启动屏上的所有动画 */
    if (s_boot_ui.devil_img && lv_obj_is_valid(s_boot_ui.devil_img)) {
        lv_anim_delete(s_boot_ui.devil_img, NULL);
    }
    s_boot_ui = (start_screen_t){0};

    ESP_LOGI(TAG, "route_to_home: screen_del=%d, screen=%p before load",
             ui->screen_del, (void*)ui->screen);
    /* 切回浅色首页；不做动画，快速进入 */
    ui_load_scr_animation(ui, &ui->screen, ui->screen_del, &ui->screen_del,
                          setup_scr_home_screen, LV_SCR_LOAD_ANIM_NONE,
                          100, 0, false, true);
    ESP_LOGI(TAG, "route_to_home: screen_del=%d, screen=%p after load",
             ui->screen_del, (void*)ui->screen);

    /* 强制立即刷新一帧，确保屏幕切换生效 */
    lv_refr_now(NULL);
    ESP_LOGI(TAG, "route_to_home: done after lv_refr_now");

    /* 绑定天气服务到 UI 并触发初始天气更新 */
    weather_bind_ui(ui);
    ESP_LOGI(TAG, "route_to_home: weather_bind_ui called");

    /* 加载保存的 adcode 并触发天气更新 */
    uint32_t prov = 0, pref = 0, county = 0;
    (void)weather_persist_load(&prov, &pref, &county);
    uint32_t adcode = county ? county : (pref ? pref : prov);
    if (!adcode) {
        adcode = weather_city_get_selected_county_code();
        if (!adcode) adcode = weather_city_get_selected_pref_code();
        if (!adcode) adcode = weather_city_get_selected_province_code();
    }
    if (adcode) {
        weather_set_last_adcode(adcode);
        (void)weather_update_for_adcode(adcode);
        ESP_LOGI(TAG, "route_to_home: triggered weather update for adcode=%lu", (unsigned long)adcode);
    } else {
        ESP_LOGI(TAG, "route_to_home: no adcode available, skipping weather update");
    }
}

static void splash_autoconn_task(void *arg)
{
    lv_ui *ui = (lv_ui*)arg;
    /* 1) 先尝试一次 MRU 保存网络，等待至多 8 秒拿到 IP */
    ESP_LOGI(TAG, "splash_conn: try MRU Wi-Fi once (8s)");
    (void)wifi_ui_connect_mru_saved_once(8000);

    /* 2) 在此基础上，额外等待一段时间，让 Music 完成首轮初始化
     *    条件：Wi‑Fi+IP、Music 索引 两者全部 READY
     *    为防止异常阻塞，设置最大等待时间（例如 30 秒）。*/
    EventGroupHandle_t g = startup_ready_group();
    const EventBits_t ALL = STARTUP_READY_WIFI | STARTUP_READY_MUSIC;
    const TickType_t max_wait = pdMS_TO_TICKS(30000);  /* 30s 上限 */

    EventBits_t bits = 0;
    if (g) {
        ESP_LOGI(TAG, "splash_conn: wait STARTUP_READY (WiFi/Music), max 30s");
        const EventBits_t before = xEventGroupGetBits(g);
        ESP_LOGI(TAG, "splash_conn: current bits before wait: WiFi=%d Music=%d",
                 (before & STARTUP_READY_WIFI) ? 1 : 0,
                 (before & STARTUP_READY_MUSIC) ? 1 : 0);
        bits = xEventGroupWaitBits(g, ALL,
                                   pdFALSE, /* clearOnExit */
                                   pdTRUE,  /* waitForAllBits */
                                   max_wait);
        if ((bits & ALL) == ALL) {
            ESP_LOGI(TAG, "splash_conn: all ready, bits=0x%02x (WiFi=%d Music=%d)",
                     (unsigned)bits,
                     (bits & STARTUP_READY_WIFI) ? 1 : 0,
                     (bits & STARTUP_READY_MUSIC) ? 1 : 0);
        } else {
            ESP_LOGW(TAG, "splash_conn: timeout after 30s, bits=0x%02x (WiFi=%d Music=%d)",
                     (unsigned)bits,
                     (bits & STARTUP_READY_WIFI) ? 1 : 0,
                     (bits & STARTUP_READY_MUSIC) ? 1 : 0);
        }
    } else {
        ESP_LOGW(TAG, "splash_conn: startup_ready_group() returned NULL");
    }

    /* 3) 无论是否全部就绪，都切回首页（日志中可看到实际就绪情况） */
    if (lvgl_port_lock(200)) {
        (void)lv_async_call(route_to_home, ui);
        lvgl_port_unlock();
    }
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
    vTaskDeleteWithCaps(NULL);
#else
    vTaskDelete(NULL);
#endif
}

void splash_show_and_autoconnect(lv_ui *ui)
{
    ESP_LOGI(TAG, "splash_show_and_autoconnect called, ui=%p", (void*)ui);
    if (!ui) {
        ESP_LOGE(TAG, "ui is NULL, returning");
        return;
    }

    /* 若重复进入（例如热重启 UI），先清理旧的 timer/screen 引用 */
    if (s_boot_progress_timer) {
        lv_timer_del(s_boot_progress_timer);
        s_boot_progress_timer = NULL;
    }
    if (s_boot_ui.screen && lv_obj_is_valid(s_boot_ui.screen)) {
        lv_obj_del_async(s_boot_ui.screen);
    }
    s_boot_ui = (start_screen_t){0};

    start_screen_create(&s_boot_ui);

    /* 初始化进度条值并启动刷新 */
    boot_progress_timer_cb(NULL);
    if (!s_boot_progress_timer) {
        s_boot_progress_timer = lv_timer_create(boot_progress_timer_cb, 300, NULL);
    }

    /* 立即切换到启动屏 */
    lv_screen_load(s_boot_ui.screen);
    /* 强制立即刷新一帧：确保背光打开时看到的是启动屏，而不是上电清屏内容 */
    lv_refr_now(NULL);

    if (!s_backlight_inited) {
        bl_ui_init();
        s_backlight_inited = true;
    }

    // 在后台任务中执行一次自动连接 + 路由
    ESP_LOGI(TAG, "creating splash_conn task...");
    BaseType_t ret = pdFAIL;
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
    ret = xTaskCreatePinnedToCoreWithCaps(
        splash_autoconn_task, "splash_conn",
        4096, ui, 4, NULL, 1,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    ret = xTaskCreatePinnedToCore(splash_autoconn_task, "splash_conn", 4096, ui, 4, NULL, 1);
#endif
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "splash_conn task create failed! ret=%d, falling back to home", (int)ret);
        if (lvgl_port_lock(200)) {
            (void)lv_async_call(route_to_home, ui);
            lvgl_port_unlock();
        }
    } else {
        ESP_LOGI(TAG, "splash_conn task created successfully");
#if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
        ESP_LOGI(TAG, "splash_conn: PSRAM stack (4KB)");
#endif
    }
}
