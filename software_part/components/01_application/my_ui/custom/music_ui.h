#pragma once
#include "audio_player.h"
#include "sdkconfig.h"
#include "gui_guider.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================== 基于 Kconfig 的默认值 ==================
 * 若外部已定义 MUSIC_HOST_IP / MUSIC_BASE_URL / MUSIC_INDEX_URL，保留外部定义。
 * 否则，优先采用 sdkconfig 中的 CONFIG_APP_* 值；再次兜底到编译期常量。 */

#ifndef MUSIC_HOST_IP
  #ifdef CONFIG_APP_MUSIC_HOST_IP
    #define MUSIC_HOST_IP CONFIG_APP_MUSIC_HOST_IP
  #else
    #define MUSIC_HOST_IP "music-server.local"
  #endif
#endif

#ifndef MUSIC_BASE_URL
  #ifdef CONFIG_APP_MUSIC_BASE_URL
    #define MUSIC_BASE_URL CONFIG_APP_MUSIC_BASE_URL
  #else
    #define MUSIC_BASE_URL "https://" MUSIC_HOST_IP "/"
  #endif
#endif

#ifndef MUSIC_INDEX_URL
  #ifdef CONFIG_APP_MUSIC_INDEX_URL
    #define MUSIC_INDEX_URL CONFIG_APP_MUSIC_INDEX_URL
  #else
    #define MUSIC_INDEX_URL MUSIC_BASE_URL "index.json"
  #endif
#endif
/* ===================================================================== */

/* 开发期若证书 SAN 不含 IP，可在 menuconfig 中开启
 * CONFIG_APP_MUSIC_SKIP_CN_CHECK 临时跳过主机名校验（上线务必关闭）。 */

/* 初始化（幂等）：注册 “拿到IP就拉取索引” 的系统事件；保存 ui 指针（可
 * NULL）。*/
void music_ui_init(lv_ui *ui);

/* 把列表对象绑定给本模块；通常在 setup_scr_music_screen.c 里创建完列表后调用。
 */
void music_ui_attach_list(lv_obj_t *list_obj);

/* 当屏幕销毁/切走时可调用，安全移除滚动事件并中止正在进行的构建。 */
void music_ui_detach_list(lv_obj_t *list_obj);

/* 手动刷新一次（无论是否已有 IP 都会尝试；失败会静默）。*/
void music_ui_refetch(void);

typedef struct {
  int index;
  const char *title;
  const char *url;
  const char *duration;
  uint32_t duration_ms;
} music_ui_track_info_t;

/* 设置一个“播放回调”，点击列表项时会把选中条目的信息交给你（可不设）。*/
typedef void (*music_ui_play_cb_t)(const music_ui_track_info_t *info);
void music_ui_set_play_cb(music_ui_play_cb_t cb);

/* 取当前条目数量（已加载后返回 >0）。*/
int music_ui_count(void);
/* 取某条目的 url/title 指针（内部持有，不要 free；失败返回 NULL）。*/
const char *music_ui_title_at(int idx);
const char *music_ui_url_at(int idx);
const char *music_ui_duration_at(int idx);
uint32_t music_ui_duration_ms_at(int idx);
int music_ui_current_index(void);
bool music_ui_play_index(int idx);
bool music_ui_toggle_favorite(int idx);
bool music_ui_is_favorite(int idx);
int64_t music_ui_last_action_time_us(void);
bool music_ui_recent_action_within_us(int64_t window_us);

/* 列表数据源切换：在 HTTPS 索引 与 SD 卡文件列表之间切换。
 * - 默认使用 HTTPS 索引（基于 MUSIC_INDEX_URL）；
 * - 切换到 SD 卡时会读取已扫描好的 SD 卡音乐文件名填充列表；
 * - 再次切换回 HTTPS 时会清空列表并触发一次重新拉取。 */
void music_ui_toggle_source(void);

/* 查询当前是否使用 SD 卡作为音乐目录数据源 */
bool music_ui_is_using_sdcard(void);

/* 音频线程事件转发：避免直接触碰 LVGL，全部排队到 UI 线程里处理。*/
void music_ui_dispatch_player_state(ap_state_t state);
void music_ui_dispatch_playback_finished(bool forward);

/* 播放器确定最终 URL（含 HTTPS->HTTP 回退）后回调通知 UI（可用于显示/调试）。*/
void music_ui_notify_url_resolved(const char *url);

#ifdef __cplusplus
}
#endif
