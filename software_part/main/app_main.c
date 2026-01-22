// main/app_main.c  —— 根据配置启动 UI 任务，并可选音乐任务

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h" /* xTaskCreate*WithCaps / vTaskDeleteWithCaps */

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"  // 堆诊断

#include "app_ui.h"
#include "app_music.h"      // 仍保留：里边的事件位宏会在回调里用到（没副作用）
#include "app_env_sht40.h"  // 环境温湿度任务
#include "app_inmp441.h"    // 兼容旧路径：INMP441 麦克风任务
#if CONFIG_APP_WS2812_ENABLE
#include "app_ws2812.h"     // WS2812 灯珠：告警闪烁 + 夜灯控制
#endif
#include "voice_commands.h" // 新增：本地唤醒/命令 + 远端 ASR 推流
#include "net_wifi_sntp.h"  // Wi-Fi + SNTP 管理（net_start / net_wait_ip）
#include "iot_mqtt.h"       // IoT MQTT uplink
#include "ota_update.h"     // HTTPS OTA
#include "audio_i2s.h"      // I2S 输出（预初始化以保留 DMA 内存）
#include "board_config.h"   // BOARD_AUDIO_SAMPLE_RATE_DEFAULT

/*===============================
 * 任务配置
 *===============================*/
enum {
    APP_PRIO_UI       = 8,     /* Core0: GUI */
    APP_PRIO_AUDIO_IO = 18     /* Core1: 网络/音频（不开启时不使用） */
};

/*===============================
 * 堆诊断宏（调试 DMA/内存问题时使用）
 * 打开 CONFIG_HEAP_DIAG_ENABLE 可在关键点打印内存状态
 *===============================*/
#ifndef CONFIG_HEAP_DIAG_ENABLE
#define CONFIG_HEAP_DIAG_ENABLE 1  /* 设为 1 启用详细堆诊断 */
#endif

#if CONFIG_HEAP_DIAG_ENABLE
#define HEAP_DIAG(tag) do { \
    ESP_LOGW("HEAP", "[%s] DMA free=%lu largest=%lu | Internal free=%lu largest=%lu | SPIRAM free=%lu largest=%lu", \
             (tag), \
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_DMA), \
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DMA), \
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), \
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), \
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), \
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)); \
} while(0)
#else
#define HEAP_DIAG(tag) ((void)0)
#endif
#define APP_CORE_GUI           0
#define APP_CORE_NET_AUDIO     1
#define APP_CORE_MUSIC         APP_CORE_NET_AUDIO  /* 恢复原始配置：Core1，与原设计保持一致 */
#define APP_STACK_UI_BYTES     6144
#define APP_STACK_MUSIC_BYTES  6144
#define APP_STACK_WS2812_BYTES 3072

#if (configSUPPORT_STATIC_ALLOCATION == 1)
  #define APP_STACK_WORDS(bytes) ((bytes) / sizeof(StackType_t))
  static StaticTask_t g_ui_tcb;
  static StackType_t  g_ui_stack[APP_STACK_WORDS(APP_STACK_UI_BYTES)];

  #if CONFIG_APP_ENABLE_MUSIC
  static StaticTask_t g_music_tcb;
    #if CONFIG_APP_MUSIC_USE_PSRAM_STACK
  EXT_RAM_BSS_ATTR static StackType_t  g_music_stack[APP_STACK_WORDS(APP_STACK_MUSIC_BYTES)];
    #else
  static StackType_t  g_music_stack[APP_STACK_WORDS(APP_STACK_MUSIC_BYTES)];
    #endif
  #endif

  #if CONFIG_APP_WS2812_ENABLE && CONFIG_APP_WS2812_USE_PSRAM_STACK
  static StaticTask_t g_ws2812_tcb;
  /* 使用 EXT_RAM_BSS_ATTR 将栈放入 PSRAM，避免占用内部堆 */
  EXT_RAM_BSS_ATTR static StackType_t g_ws2812_stack[APP_STACK_WORDS(APP_STACK_WS2812_BYTES)];
  #endif
#endif

/*===============================
 * 事件组：系统同步
 *===============================*/
static EventGroupHandle_t g_sys_evt = NULL;

/* Wi-Fi/IP 回调（保留，不影响 UI；music 关闭时这些位不会被消费） */
static bool s_mqtt_started = false;  // 标记MQTT是否已启动
#if CONFIG_APP_OTA_ENABLE && CONFIG_APP_OTA_AUTO_START
static bool s_ota_started = false;
#endif

static void ota_confirm_running_app_if_needed(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            /* 极简自检策略：能跑到 app_main 即认为启动成功，先确认可用。
             * 若新固件在此之前就崩溃/复位，bootloader 仍会按机制回滚。 */
            esp_err_t e = esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI("MAIN", "OTA pending verify -> mark valid: %s", esp_err_to_name(e));
        }
    }
#endif
}

#if CONFIG_APP_OTA_ENABLE && CONFIG_APP_OTA_AUTO_START
static void delayed_ota_check_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));  /* 延迟 10 秒 */
    ESP_LOGI("MAIN", "Starting delayed OTA check...");
    esp_err_t e = ota_update_check_start(CONFIG_APP_OTA_URL);
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("MAIN", "ota_update_check_start failed: %s", esp_err_to_name(e));
    } else {
        ESP_LOGI("MAIN", "✅ OTA check started: %s", CONFIG_APP_OTA_URL);
    }
    vTaskDelete(NULL);
}
#endif


static void on_got_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    if (g_sys_evt) xEventGroupSetBits(g_sys_evt, APP_MUSIC_EVT_WIFI_READY);

    /* 在获得IP后立即启动MQTT（仅启动一次） */
#if CONFIG_IOT_MQTT_ENABLE
    if (!s_mqtt_started) {
        s_mqtt_started = true;
        esp_err_t mqtt_ret = iot_mqtt_start();
        if (mqtt_ret != ESP_OK) {
            ESP_LOGE("MAIN", "iot_mqtt_start failed: %s", esp_err_to_name(mqtt_ret));
            s_mqtt_started = false;  // 失败则允许重试
        } else {
            ESP_LOGI("MAIN", "✅ MQTT started after IP acquired");
        }
    }
#endif

#if CONFIG_APP_OTA_ENABLE && CONFIG_APP_OTA_AUTO_START
    if (!s_ota_started) {
        s_ota_started = true;
        /* 延迟 OTA 检查 10 秒，避免与 Music 索引加载的 TLS 连接冲突 */
        ESP_LOGI("MAIN", "⏱️  OTA check will start in 10s (delayed to avoid TLS conflicts)");
        BaseType_t ret = xTaskCreatePinnedToCore(delayed_ota_check_task, "ota_delay", 3072, NULL, 5, NULL, 1);
        if (ret != pdPASS) {
            ESP_LOGE("MAIN", "Failed to create delayed OTA check task");
            s_ota_started = false;
        }
    }
#endif
}

static void on_sta_disconnected(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    if (g_sys_evt) xEventGroupClearBits(g_sys_evt, APP_MUSIC_EVT_WIFI_READY);
}

void app_main(void)
{
    ota_confirm_running_app_if_needed();

    /* 降噪：统一把多数模块的 INFO 降到 WARN（可按需维持少数 TAG） */
    {
        esp_log_level_set("*", ESP_LOG_WARN);
        const char *mute_tags[] = {
            /* 我们已在内部关闭了某些日志，这里再兜底降级 */
            "APP", "music_ui", "MusicScreen", "MusicSpectrumUI", "MusicSpectrumSetup",
            "audio_player", "AUDIO_PIPELINE", "AUDIO_ELEMENT", "AUDIO_THREAD",
            /* 保留 HTTP_STREAM 的 INFO 以便排查音频拉流 */
            "MP3_DECODER", "CODEC_ELEMENT_HELPER", "HAL", "LVGL_PORT",
            "gpio", "boot", "esp_image"
        };
        for (size_t i = 0; i < (sizeof(mute_tags)/sizeof(mute_tags[0])); ++i) {
            esp_log_level_set(mute_tags[i], ESP_LOG_WARN);
        }
        /* 进一步收敛 ADF 噪声：仅保留 ERROR */
        esp_log_level_set("AUDIO_ELEMENT",  ESP_LOG_ERROR);
        esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
        /* 恢复到默认安静日志（WARN）。为 Chatbot 排障临时上调到 INFO。*/
        esp_log_level_set("transport_base",    ESP_LOG_WARN);
        /* 关键链路：音乐 HTTP 拉流，默认降到 WARN；需要排障时再临时升到 INFO/DEBUG */
        esp_log_level_set("HTTP_STREAM", ESP_LOG_WARN);

        /* 生产默认：UI 侧也降到 WARN；需要排障时再临时升到 INFO */
        esp_log_level_set("MusicScreen",    ESP_LOG_WARN);
        esp_log_level_set("music_progress", ESP_LOG_WARN);
        esp_log_level_set("app_music",      ESP_LOG_WARN);

#if CONFIG_MY_UI_CHATBOT_ENABLE
        esp_log_level_set("CHATBOT",           ESP_LOG_INFO);
        esp_log_level_set("websocket_client",  ESP_LOG_INFO);
        esp_log_level_set("esp-tls",           ESP_LOG_INFO);
        esp_log_level_set("transport_ws",      ESP_LOG_INFO);
#else
        esp_log_level_set("CHATBOT",           ESP_LOG_WARN);
        esp_log_level_set("websocket_client",  ESP_LOG_WARN);
        esp_log_level_set("esp-tls",           ESP_LOG_WARN);
        esp_log_level_set("transport_ws",      ESP_LOG_WARN);
#endif

        /* 环境传感器日志：保留 INFO，方便在 monitor 中查看温湿度 */
        esp_log_level_set("ENV_SHT40", ESP_LOG_INFO);
        /* IoT MQTT 日志：保留 INFO，方便查看连接和发布状态 */
        esp_log_level_set("IOT_MQTT", ESP_LOG_INFO);
        /* OTA 日志：保留 INFO，便于观察升级进度/耗时 */
#if CONFIG_APP_OTA_ENABLE
        esp_log_level_set("OTA_UPDATE", ESP_LOG_INFO);
#endif
        /* 麦克风任务日志：保留 INFO，便于查看音量等特征 */
        esp_log_level_set("INMP441_APP", ESP_LOG_INFO);
        /* 调试启动屏问题：临时开启 APP/GUI/splash_ui 日志 */
        esp_log_level_set("APP", ESP_LOG_INFO);
        esp_log_level_set("GUI", ESP_LOG_INFO);
        esp_log_level_set("splash_ui", ESP_LOG_INFO);
        esp_log_level_set("net_wifi_sntp", ESP_LOG_INFO);
        esp_log_level_set("wifi_ui", ESP_LOG_INFO);
    }

    /* ★ 预初始化 I2S（DMA 缓冲区分配）——必须在任何 TLS 连接之前完成
     *   TLS 握手会消耗大量 DMA/内部堆，若 I2S 在其后初始化，DMA 分配可能失败。
     *   此处仅分配资源，音频播放任务启动后会检测到已初始化并跳过重复初始化。 */
    HEAP_DIAG("before_i2s_preinit");
    if (!audio_i2s_init(BOARD_AUDIO_SAMPLE_RATE_DEFAULT)) {
        ESP_LOGE("MAIN", "audio_i2s_init pre-init failed (DMA alloc?)");
        /* 不 return：允许系统继续启动，音频功能可能受限 */
    } else {
        ESP_LOGI("MAIN", "✅ I2S pre-initialized @ %d Hz (DMA reserved)", BOARD_AUDIO_SAMPLE_RATE_DEFAULT);
    }
    HEAP_DIAG("after_i2s_preinit");

    /* 基础网络：由 net_wifi_sntp 统一管理 Wi-Fi + SNTP，便于 Chatbot/ASR 使用 */

    if (net_start() != ESP_OK) {
        ESP_LOGE("MAIN", "net_start failed");
        return;
    }

    g_sys_evt = xEventGroupCreate();
    if (!g_sys_evt) { return; }

    (void)esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL, NULL);
    (void)esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, on_sta_disconnected, NULL, NULL);

    /* MQTT 将在 on_got_ip 回调中启动，无需在此等待 */
    ESP_LOGI("MAIN", "⏳ Waiting for WiFi connection to start MQTT...");

    /* —— 语音链路：二选一 —— */
#if CONFIG_VOICE_AFE_ENABLE
    /* 新路径：GMF AFE + 本地命令 + 远端 ASR 推流 */
    if (voice_commands_init() != ESP_OK) {
        ESP_LOGE("MAIN", "voice_commands_init failed");
    }
#else
    /* 兼容旧路径：仅推流到 Chatbot ASR */
    /* WebSocket + TLS + Base64 编码会占用较多栈，适当放大 */
    (void)xTaskCreatePinnedToCore(
        app_inmp441_task, "inmp441_task",
        8192, NULL, 7, NULL, APP_CORE_NET_AUDIO);
#endif

    /* 环境监测任务 */
  #if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_APP_ENV_SHT40_USE_PSRAM_STACK
    (void)xTaskCreatePinnedToCoreWithCaps(
        app_env_sht40_task, "env_sht40",
        3072, NULL, 5, NULL, APP_CORE_NET_AUDIO,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_LOGI("MAIN", "env_sht40 task with PSRAM stack (3KB)");
  #else
    (void)xTaskCreatePinnedToCore(
        app_env_sht40_task, "env_sht40",
        3072, NULL, 5, NULL, APP_CORE_NET_AUDIO);
  #endif

    HEAP_DIAG("before_ws2812");

    /* WS2812：告警指示 + 夜灯
     * 移到 CPU0 (GUI核心)，避免告警闪烁时与 CPU1 的音频 I2S DMA 竞争资源 */
#if CONFIG_APP_WS2812_ENABLE
  #if (configSUPPORT_STATIC_ALLOCATION == 1) && CONFIG_APP_WS2812_USE_PSRAM_STACK
    (void)xTaskCreateStaticPinnedToCore(
        app_ws2812_task, "ws2812",
        APP_STACK_WORDS(APP_STACK_WS2812_BYTES), NULL, 4,
        g_ws2812_stack, &g_ws2812_tcb, APP_CORE_GUI);
    ESP_LOGI("MAIN", "WS2812 task created on CPU0 with PSRAM stack (%d bytes)", APP_STACK_WS2812_BYTES);
  #else
    (void)xTaskCreatePinnedToCore(
        app_ws2812_task, "ws2812",
        APP_STACK_WS2812_BYTES, NULL, 4, NULL, APP_CORE_GUI);
    ESP_LOGI("MAIN", "WS2812 task created on CPU0 with internal heap stack (%d bytes)", APP_STACK_WS2812_BYTES);
  #endif
#else
    ESP_LOGW("MAIN", "WS2812 task disabled (CONFIG_APP_WS2812_ENABLE=n) for A/B testing");
#endif

    HEAP_DIAG("after_ws2812");

    /* UI 任务 */
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    (void)xTaskCreateStaticPinnedToCore(
        app_ui_task, "ui_task",
        APP_STACK_WORDS(APP_STACK_UI_BYTES), NULL, APP_PRIO_UI,
        g_ui_stack, &g_ui_tcb, APP_CORE_GUI);
#else
    (void)xTaskCreatePinnedToCore(
        app_ui_task, "ui_task",
        APP_STACK_UI_BYTES, NULL, APP_PRIO_UI, NULL, APP_CORE_GUI);
#endif

    /* music_task（受 CONFIG_APP_ENABLE_MUSIC 控制） */
#if CONFIG_APP_ENABLE_MUSIC
  #if (configSUPPORT_STATIC_ALLOCATION == 1)
    (void)xTaskCreateStaticPinnedToCore(
        app_music_task, "music_task",
        APP_STACK_WORDS(APP_STACK_MUSIC_BYTES), (void*)g_sys_evt, APP_PRIO_AUDIO_IO,
        g_music_stack, &g_music_tcb, APP_CORE_MUSIC);
    #if CONFIG_APP_MUSIC_USE_PSRAM_STACK
    ESP_LOGI("MAIN", "music_task with PSRAM stack (6KB)");
    #endif
  #else
    (void)xTaskCreatePinnedToCore(
        app_music_task, "music_task",
        APP_STACK_MUSIC_BYTES, (void*)g_sys_evt, APP_PRIO_AUDIO_IO, NULL, APP_CORE_MUSIC);
  #endif
#endif
}
