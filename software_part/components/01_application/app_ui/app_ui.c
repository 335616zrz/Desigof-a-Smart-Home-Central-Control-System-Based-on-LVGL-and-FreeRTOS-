#include "app_ui.h"

#include "sdkconfig.h"
#include "hal_display_touch.h"
#include "lvgl_port.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if CONFIG_APP_ENABLE_MUSIC
#include "freertos/queue.h"
#include "audio_player.h"
#endif
#include "custom.h"
#if CONFIG_APP_ENABLE_MUSIC
#include "audio_thread.h"
#endif
#include "sdcard_spi.h"

static const char *TAG = "APP";

/* 由 GUI Guider 生成（声明 setup_ui / lv_ui） */
#include "gui_guider.h"

// ★ 新增：引入我们刚写的拉取与填充列表模块
#include "music_ui.h"
#include "music_volume.h"
#include "music_progress.h"
#include "chatbot_client.h"

/* 全局 UI 句柄（与 gui_guider.h 中的 extern 对应） */
lv_ui guider_ui;

/* 幂等控制：避免重复初始化/构建 */
static bool s_ui_started = false;


#if CONFIG_APP_ENABLE_MUSIC
typedef struct {
    char     *url;
    uint32_t  duration_ms;
} audio_cmd_msg_t;

static QueueHandle_t s_audio_cmd_q    = NULL;
static TaskHandle_t  s_audio_cmd_task = NULL;

static void audio_cmd_task(void *arg)
{
    (void)arg;
    audio_cmd_msg_t msg = {0};

    for (;;) {
        if (xQueueReceive(s_audio_cmd_q, &msg, pdMS_TO_TICKS(200)) == pdTRUE) {
            if (msg.url && msg.url[0]) {
                ap_set_track_duration(msg.duration_ms);
                /* ap_play_url() 内部已处理正在播放/加载态的 stop，避免重复 stop 引发竞态 */
                if (!ap_play_url(msg.url)) {
                    ESP_LOGW(TAG, "playback start failed for %s", msg.url);
                }
                free(msg.url);
            } else if (msg.url) {
                free(msg.url);
            }
            msg.url = NULL;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif

/* 如果你想点一下只打印标题、先不播放，可以留空回调（不设置），
   或者像下面这样写个忽略 url 的回调： */
static void on_track_pick(const music_ui_track_info_t *info)
{
    if (!info) {
        return;
    }

    const char *title = (info->title && info->title[0]) ? info->title : "(no title)";
    ESP_LOGI(TAG, "pick: %s", title);

    /* 进度总时长：
     * - 若目录/索引提供了明确的毫秒值（HTTP 模式），直接使用；
     * - 若为 0（SD 卡估算场景），先重置为 0，等待底层解码器在
     *   上报 MUSIC_INFO 后，通过 ap_get_track_duration() 再刷新一次。*/
    if (info->duration_ms > 0) {
        music_progress_set_total(info->duration_ms);
    } else {
        music_progress_set_total(0);
    }

    if (guider_ui.MusicScreen_music_title &&
        lv_obj_is_valid(guider_ui.MusicScreen_music_title)) {
        lv_label_set_text(guider_ui.MusicScreen_music_title, title);
    }

    if (info->duration && info->duration[0] &&
        guider_ui.MusicScreen_music_time_total &&
        lv_obj_is_valid(guider_ui.MusicScreen_music_time_total)) {
        lv_label_set_text(guider_ui.MusicScreen_music_time_total, info->duration);
    }

    if (guider_ui.MusicScreen_music_time_elapsed &&
        lv_obj_is_valid(guider_ui.MusicScreen_music_time_elapsed)) {
        lv_label_set_text(guider_ui.MusicScreen_music_time_elapsed, "00:00");
    }

    /* 保持 GUI Guider 生成代码的同时，追加一次自定义同步，
       即使下次重新导出覆盖也能由此处确保“切曲初始为未播放，等待底层进入PLAYING”。 */
    MusicScreen_OnTrackChanged();
    extern void MusicScreen_UpdatePlayState(bool playing);
    MusicScreen_UpdatePlayState(false);
    music_progress_set_playing(false);
#if CONFIG_APP_ENABLE_MUSIC
    // 若 TTS 正在播报，先快速停止，避免与音乐争用 I2S 导致卡顿/无声/WDT
    extern void tts_player_stop_now(void);
    extern bool tts_player_is_active(void);
    if (tts_player_is_active()) {
        tts_player_stop_now();
    }
    /* 同样，仅在有“权威”的 duration_ms 时提前告知播放器；
     * 对于 SD 卡估算的曲目（duration_ms=0），让 audio_player
     * 自行结合解码器信息与 total_bytes 估算时长，避免双重估算。*/
    ap_set_track_duration(info->duration_ms);

    const char *url = info->url;
    if (url && url[0] && s_audio_cmd_q) {
        audio_cmd_msg_t msg = {
            .url = strdup(url),
            .duration_ms = info->duration_ms,
        };
        if (!msg.url) {
            ESP_LOGE(TAG, "OOM duplicating url");
            return;
        }
        if (uxQueueMessagesWaiting(s_audio_cmd_q) > 0) {
            audio_cmd_msg_t old = {0};
            if (xQueueReceive(s_audio_cmd_q, &old, 0) == pdTRUE) {
                if (old.url) {
                    free(old.url);
                }
            }
        }
        if (xQueueSend(s_audio_cmd_q, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "audio cmd queue full, drop %s", url);
            free(msg.url);
        }
    }
#endif
}

/* 在 LVGL 线程上下文中构建并加载 UI（由 lv_async_call 调度进入） */
static void ui_build_cb(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "ui_build_cb: start");
    music_volume_init();
    ESP_LOGI(TAG, "ui_build_cb: calling setup_ui");
    /* GUI Guider 入口；内部通常会调用 lv_screen_load() */
    setup_ui(&guider_ui);
    ESP_LOGI(TAG, "ui_build_cb: setup_ui done");
    /* 扩展点：此处可追加事件绑定/数据订阅 */
    
    // ★ 初始化音乐索引模块（注册“拿到 IP 就拉取”的系统事件）
    music_ui_init(&guider_ui);

    // ★ 把 GUI Guider 里创建的列表对象绑定给模块
    music_ui_attach_list(guider_ui.MusicScreen_music_list);

    // ★ 通用后置初始化：统一 g_music_screen_ui、追加共享样式、刷新图标、兜底绑定
    extern void MusicScreen_CommonPostSetup(lv_ui *ui);
    MusicScreen_CommonPostSetup(&guider_ui);

    // ★ 先不传入 url，不对接播放器：不设置回调即可
    music_ui_set_play_cb(on_track_pick); // 如果只想打印标题，可以解注这行

#if CONFIG_APP_ENABLE_MUSIC
    if (!s_audio_cmd_q) {
        s_audio_cmd_q = xQueueCreate(1, sizeof(audio_cmd_msg_t));
        if (!s_audio_cmd_q) {
            ESP_LOGE(TAG, "create audio cmd queue failed");
        }
    }
    if (s_audio_cmd_q && !s_audio_cmd_task) {
        esp_err_t rc = audio_thread_create((audio_thread_t *)&s_audio_cmd_task,
                                           "audio_cmd",
                                           audio_cmd_task,
                                           NULL,
                                           4096,
                                           6,
                                           true,  /* stack in PSRAM when possible */
                                           1);
        if (rc != ESP_OK) {
            ESP_LOGE(TAG, "create audio cmd task failed");
            s_audio_cmd_task = NULL;
        }
    }
#endif

    // ★ 可选：如果希望无论是否已联网，都主动尝试拉一次
    // music_ui_refetch();

    // 启动 Chatbot 客户端（可在菜单使能后生效）
#if CONFIG_MY_UI_CHATBOT_ENABLE
    chatbot_client_bind_ui(&guider_ui);
    chatbot_client_start();
#endif

    /* 通用自定义初始化：恢复“室内/室外”等 UI 状态 */
    custom_init(&guider_ui);
}

bool app_ui_init(void)
{
    if (s_ui_started) return true;

    ESP_LOGI(TAG, "app_ui_init: start");

    /* 1) 显示与触摸 HAL */
    if (!display_touch_init()) {
        return false;
    }
    ESP_LOGI(TAG, "app_ui_init: display_touch_init done");

    /* 2) LVGL 端口层（驱动绑定、时钟、互斥等） */
    if (!lvgl_port_init()) {
        return false;
    }
    ESP_LOGI(TAG, "app_ui_init: lvgl_port_init done");

    /* 3) 在 LVGL 线程里创建 UI，避免互斥争用（优先显示启动屏，避免上电白屏） */
    ESP_LOGI(TAG, "app_ui_init: calling lv_async_call(ui_build_cb)");
    if (!lvgl_port_lock(200)) {
        ESP_LOGE(TAG, "app_ui_init: lvgl lock failed, cannot schedule UI build");
        return false;
    }
    lv_result_t res = lv_async_call(ui_build_cb, NULL);
    lvgl_port_unlock();
    if (res != LV_RESULT_OK) {
        ESP_LOGE(TAG, "app_ui_init: lv_async_call failed %d", (int)res);
        return false;
    }
    ESP_LOGI(TAG, "app_ui_init: lv_async_call returned");

    /* 4) SD 卡挂载 + 后台扫描（不阻塞 UI 启动） */
    (void)sdcard_spi_init();

    s_ui_started = true;
    ESP_LOGI(TAG, "app_ui_init: done");
    return true;
}

/* FreeRTOS 任务入口：调用 app_ui_init 后自删除 */
void app_ui_task(void *arg)
{
    (void)arg;
    (void)app_ui_init();  /* 失败不阻塞其他路径 */
    vTaskDelete(NULL);
}
