#include "music_screen_common.h"
#include "music_styles.h"
#include "music_ui.h"
#include "music_progress.h"

/* 由生成文件提供 */
extern lv_ui *g_music_screen_ui;
extern void update_mode_icons(void);
extern void update_heart_icon(void);

void MusicScreen_UpdateSourceLabel(bool use_sdcard)
{
    if (!g_music_screen_ui) {
        return;
    }
    lv_obj_t *lbl = g_music_screen_ui->MusicScreen_source_label;
    if (!lbl || !lv_obj_is_valid(lbl)) {
        return;
    }
    lv_label_set_text(lbl, use_sdcard ? "当前来源：SD 卡" : "当前来源：网络");
}

void MusicScreen_CommonPostSetup(lv_ui *ui)
{
    if (!ui) return;

    /* 统一全局 UI 指针，供通用更新函数使用（与主题无关）。*/
    g_music_screen_ui = ui;

    /* 附加共享样式（与生成代码的属性一致，不改变外观）。*/
    music_styles_apply_for_ui(ui);

    /* 初始图标刷新（喜欢/模式）。*/
    update_mode_icons();
    update_heart_icon();

    /* 再次确保列表绑定，幂等安全。*/
    music_ui_attach_list(ui->MusicScreen_music_list);

    /* 刷新来源标签：根据当前 music_ui 模式（HTTP 或 SD 卡）显示 */
    MusicScreen_UpdateSourceLabel(music_ui_is_using_sdcard());
}
