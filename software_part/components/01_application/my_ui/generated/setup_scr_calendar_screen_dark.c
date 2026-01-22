/*
 * 日历屏（light，只有月视图）
 *
 * - 顶部：返回按钮 + “日历”标题 + 当前年月
 * - 中部：7×6 月视图网格，只显示公历日期
 * - 不包含年视图、事件列表、农历等复杂逻辑
 *
 * 切屏关系：
 * - 从 App Screen 的 screen_3_img_5 / screen_3_btn_5 进入本屏
 * - 左上角返回图标或点击区域，经 events_init_calendar_main() 回到 app_screen
 */

#include "lvgl.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

/* ---------- 简单月视图状态 ---------- */

typedef struct {
    lv_obj_t *btn;
    lv_obj_t *label;
    int year;   /* 公历年 */
    int month;  /* 公历月 */
    int day;    /* 公历日 */
    bool other_month;
} cal_cell_t;

/* 6 行 × 7 列 = 42 个格子 */
static cal_cell_t s_cells[42];

static int s_cur_year  = 1970;
static int s_cur_month = 1;    /* 1-12 */
static int s_today_y   = 1970;
static int s_today_m   = 1;
static int s_today_d   = 1;
static int s_sel_y     = 1970;
static int s_sel_m     = 1;
static int s_sel_d     = 1;

/* 公历 / 农历显示模式：false = 公历，true = 农历（仅影响文字显示） */
static bool      s_calendar_use_lunar          = false;
/* 顶部“公历 / 农历”切换按钮（仅在本文件内部使用） */
static lv_obj_t *s_calendar_mode_toggle_btn    = NULL;
static lv_obj_t *s_calendar_mode_toggle_label  = NULL;

/* “今天是 ...” 弹窗相关（仅在本文件内部使用） */
static lv_obj_t *s_calendar_jump_modal      = NULL;
static lv_obj_t *s_calendar_jump_btn_month  = NULL;
static lv_obj_t *s_calendar_jump_btn_date   = NULL;
static lv_obj_t *s_calendar_jump_btn_cancel = NULL;
static lv_obj_t *s_calendar_jump_tip_label  = NULL;

/* 日期选择滚轮 */
#define CAL_YEAR_MIN   1970
#define CAL_YEAR_MAX   2099
#define CAL_YEAR_COUNT (CAL_YEAR_MAX - CAL_YEAR_MIN + 1)

static lv_obj_t *s_calendar_roller_year  = NULL;
static lv_obj_t *s_calendar_roller_month = NULL;
static lv_obj_t *s_calendar_roller_day   = NULL;

/* 选项字符串缓冲区，生命周期为整个程序，适配 lv_roller_set_options 的需求 */
static char s_calendar_year_opts[CAL_YEAR_COUNT * 16];
static char s_calendar_month_opts[12 * 32];  /* 12 个月，每月预留更宽文本，便于显示农历+闰月 */
static char s_calendar_day_opts[31 * 16];

/* ---------- 农历文字格式化（仅做显示用，不改变内部日期算法） ---------- */

typedef struct {
    int  year;      /* 农历年（公历对应的农历年） */
    int  month;     /* 农历月 1-12 */
    int  day;       /* 农历日 1-30 */
    bool is_leap;   /* 是否闰月 */
} lunar_date_t;

/* 天干地支 */
static const char *s_gan_str[10] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
static const char *s_zhi_str[12] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};

static void format_lunar_year_ganzhi(int year, char *buf, size_t size)
{
    int gan_index = (year - 4) % 10;
    int zhi_index = (year - 4) % 12;
    if (gan_index < 0) gan_index += 10;
    if (zhi_index < 0) zhi_index += 12;
    lv_snprintf(buf, size, "%s%s年", s_gan_str[gan_index], s_zhi_str[zhi_index]);
}

/* 农历月份显示名称：这里简单将公历 1~12 月映射为“正月~腊月”，不处理闰月 */
static const char *s_lunar_month_names[12] = {
    "正月","二月","三月","四月","五月","六月",
    "七月","八月","九月","十月","冬月","腊月"
};

static void format_lunar_month_simple(int month, char *buf, size_t size)
{
    if (month < 1) month = 1;
    if (month > 12) month = 12;
    lv_snprintf(buf, size, "%s", s_lunar_month_names[month - 1]);
}

/* 带闰月前缀的月份名称，例如：闰三月、闰五月 */
static void format_lunar_month_with_leap(int month, bool is_leap, char *buf, size_t size)
{
    char month_buf[32];
    format_lunar_month_simple(month, month_buf, sizeof(month_buf));
    if (is_leap) {
        lv_snprintf(buf, size, "闰%s", month_buf);
    } else {
        lv_snprintf(buf, size, "%s", month_buf);
    }
}

/* 农历日名称：初一、初二…二十、廿一…三十；31 日用“卅一”作占位 */
static const char *s_lunar_day_names[31] = {
    "初一","初二","初三","初四","初五","初六","初七","初八","初九","初十",
    "十一","十二","十三","十四","十五","十六","十七","十八","十九","二十",
    "廿一","廿二","廿三","廿四","廿五","廿六","廿七","廿八","廿九","三十",
    "卅一"
};

static void format_lunar_day_simple(int day, char *buf, size_t size)
{
    if (day < 1) day = 1;
    if (day > 31) day = 31;
    lv_snprintf(buf, size, "%s", s_lunar_day_names[day - 1]);
}

/* ---------- 公历 → 农历（1900~2099，小表+推算，Flash 占用较小） ---------- */

/* 1900~2099 农历年信息表：
 * - 低 4 位：闰月月份（0 表示无闰月）
 * - 第 16 位：闰月大小（1=30 天，0=29 天）
 * - 中间 12 位：从正月到腊月的大小月信息，高位在前（1=30 天，0=29 天）
 */
static const unsigned int s_lunar_year_table[] = {
    0x04bd8,0x04ae0,0x0a570,0x054d5,0x0d260,0x0d950,0x16554,0x056a0,0x09ad0,0x055d2,
    0x04ae0,0x0a5b6,0x0a4d0,0x0d250,0x1d255,0x0b540,0x0d6a0,0x0ada2,0x095b0,0x14977,
    0x04970,0x0a4b0,0x0b4b5,0x06a50,0x06d40,0x1ab54,0x02b60,0x09570,0x052f2,0x04970,
    0x06566,0x0d4a0,0x0ea50,0x06e95,0x05ad0,0x02b60,0x186e3,0x092e0,0x1c8d7,0x0c950,
    0x0d4a0,0x1d8a6,0x0b550,0x056a0,0x1a5b4,0x025d0,0x092d0,0x0d2b2,0x0a950,0x0b557,
    0x06ca0,0x0b550,0x15355,0x04da0,0x0a5b0,0x14573,0x052b0,0x0a9a8,0x0e950,0x06aa0,
    0x0aea6,0x0ab50,0x04b60,0x0aae4,0x0a570,0x05260,0x0f263,0x0d950,0x05b57,0x056a0,
    0x096d0,0x04dd5,0x04ad0,0x0a4d0,0x0d4d4,0x0d250,0x0d558,0x0b540,0x0b5a0,0x195a6,
    0x095b0,0x049b0,0x0a974,0x0a4b0,0x0b27a,0x06a50,0x06d40,0x0af46,0x0ab60,0x09570,
    0x04af5,0x04970,0x064b0,0x074a3,0x0ea50,0x06b58,0x05ac0,0x0ab60,0x096d5,0x092e0,
    0x0c960,0x0d954,0x0d4a0,0x0da50,0x07552,0x056a0,0x0abb7,0x025d0,0x092d0,0x0cab5,
    0x0a950,0x0b4a0,0x0baa4,0x0ad50,0x055d9,0x04ba0,0x0a5b0,0x15176,0x052b0,0x0a930,
    0x07954,0x06aa0,0x0ad50,0x05b52,0x04b60,0x0a6e6,0x0a4e0,0x0d260,0x0ea65,0x0d530,
    0x05aa0,0x076a3,0x096d0,0x04bd7,0x04ad0,0x0a4d0,0x1d0b6,0x0d250,0x0d520,0x0dd45,
    0x0b5a0,0x056d0,0x055b2,0x049b0,0x0a577,0x0a4b0,0x0aa50,0x1b255,0x06d20,0x0ada0,
    0x14b63,0x09370,0x049f8,0x04970,0x064b0,0x168a6,0x0ea50,0x06b20,0x1a6c4,0x0aae0,
    0x0a2e0,0x0d2e3,0x0c960,0x0d557,0x0d4a0,0x0da50,0x05d55,0x056a0,0x0a6d0,0x055d4,
    0x052d0,0x0a9b8,0x0a950,0x0b4a0,0x0b6a6,0x0ad50,0x055a0,0x0aba4,0x0a5b0,0x052b0,
    0x0b273,0x06930,0x07337,0x06aa0,0x0ad50,0x14b55,0x04b60,0x0a570,0x054e4,0x0d160,
    0x0e968,0x0d520,0x0daa0,0x16aa6,0x056d0,0x04ae0,0x0a9d4,0x0a2d0,0x0d150,0x0f252,
    0x0d520,0x0dd45 /* 2099，通常只用到 2099 年之前 */
};

/* 返回指定年的闰月月份（1-12），0 表示无闰月 */
static int lunar_leap_month(int year)
{
    if (year < 1900 || year > 2099) return 0;
    return (int)(s_lunar_year_table[year - 1900] & 0x0Fu);
}

/* 返回指定年闰月的天数（0、29 或 30） */
static int lunar_leap_days(int year)
{
    int lm = lunar_leap_month(year);
    if (lm == 0) {
        return 0;
    }
    return (s_lunar_year_table[year - 1900] & 0x10000u) ? 30 : 29;
}

/* 返回指定年某月的天数（29 或 30） */
static int lunar_month_days(int year, int month)
{
    if (year < 1900 || year > 2099 || month < 1 || month > 12) {
        return 29;
    }
    return (s_lunar_year_table[year - 1900] & (0x10000u >> month)) ? 30 : 29;
}

/* 指定农历年总天数（含闰月） */
static int lunar_year_days(int year)
{
    if (year < 1900 || year > 2099) {
        return 354;
    }
    unsigned int info = s_lunar_year_table[year - 1900];
    int sum = 348; /* 12 个月全为小月时的基数：29×12 */
    for (unsigned int mask = 0x8000; mask > 0x8; mask >>= 1) {
        if (info & mask) {
            sum += 1; /* 对应位为 1 的月为大月（+1 天） */
        }
    }
    return sum + lunar_leap_days(year);
}

/* 公历日期转为儒略日数（JDN），便于计算两日期差值 */
static long gregorian_to_jdn(int year, int month, int day)
{
    int a  = (14 - month) / 12;
    int yy = year + 4800 - a;
    int mm = month + 12 * a - 3;
    long jdn = day
               + (153L * mm + 2) / 5
               + 365L * yy
               + yy / 4
               - yy / 100
               + yy / 400
               - 32045L;
    return jdn;
}

/* 公历 → 农历（支持 1900-01-31 之后、2099 年之前） */
static bool solar_to_lunar(int year, int month, int day, lunar_date_t *out)
{
    if (!out) return false;

    /* 超出表格范围时，退回到“同年同月同日”的简单映射 */
    if (year < 1900 || year > 2099) {
        out->year    = year;
        out->month   = month;
        out->day     = day;
        out->is_leap = false;
        return false;
    }

    long base_jdn   = gregorian_to_jdn(1900, 1, 31); /* 对应农历 1900-正月初一 */
    long cur_jdn    = gregorian_to_jdn(year, month, day);
    long offset     = cur_jdn - base_jdn;
    if (offset < 0) {
        out->year    = year;
        out->month   = month;
        out->day     = day;
        out->is_leap = false;
        return false;
    }

    /* 从 1900 年开始累减农历年天数，直到定位到具体农历年 */
    int lunar_year = 1900;
    while (lunar_year <= 2099) {
        int days_of_year = lunar_year_days(lunar_year);
        if (offset < days_of_year) {
            break;
        }
        offset -= days_of_year;
        ++lunar_year;
    }
    if (lunar_year > 2099) {
        out->year    = year;
        out->month   = month;
        out->day     = day;
        out->is_leap = false;
        return false;
    }

    /* 在该农历年内部，按月累减，处理闰月 */
    int  leap_month = lunar_leap_month(lunar_year);
    int  lunar_month = 1;
    bool is_leap = false;

    for (;;) {
        int days_of_month;
        if (is_leap && leap_month) {
            days_of_month = lunar_leap_days(lunar_year);
        } else {
            days_of_month = lunar_month_days(lunar_year, lunar_month);
        }

        if (offset < days_of_month) {
            break;
        }
        offset -= days_of_month;

        if (leap_month && lunar_month == leap_month && !is_leap) {
            /* 刚走完平月，下一轮先走同序号的闰月 */
            is_leap = true;
        } else {
            if (is_leap) {
                /* 闰月走完，进入下一正常月份 */
                is_leap = false;
                ++lunar_month;
            } else {
                ++lunar_month;
            }
        }
    }

    out->year    = lunar_year;
    out->month   = lunar_month;
    out->day     = (int)offset + 1;
    out->is_leap = is_leap;
    return true;
}

static int days_in_month(int y, int m)
{
    static const int d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 30;
    int res = d[m - 1];
    if (m == 2) {
        bool leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
        if (leap) res = 29;
    }
    return res;
}

static int weekday_of(int y, int m, int d)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = y - 1900;
    t.tm_mon  = m - 1;
    t.tm_mday = d;
    t.tm_isdst = -1;
    if (mktime(&t) == (time_t)-1) return 0;
    return t.tm_wday; /* 0=Sunday */
}

static bool same_date(int y1,int m1,int d1,int y2,int m2,int d2)
{
    return (y1==y2 && m1==m2 && d1==d2);
}

/* 前置声明：在构建 UI 后绑定 */
static void calendar_update_header_date(lv_ui *ui);
static void calendar_update_month_view(lv_ui *ui);
static void calendar_update_jump_rollers_text(void);

static void calendar_build_year_options(void)
{
    char *p = s_calendar_year_opts;
    size_t remain = sizeof(s_calendar_year_opts);
    for (int y = CAL_YEAR_MIN; y <= CAL_YEAR_MAX && remain > 1; ++y) {
        int last = (y == CAL_YEAR_MAX);
        int n;
        if (!s_calendar_use_lunar) {
            n = lv_snprintf(p, remain, last ? "%d年" : "%d年\n", y);
        } else {
            char year_buf[32];
            /* 按农历年分界：用当年年中某日（例如 6 月 15 日）推算所属农历年 */
            lunar_date_t ld;
            if (!solar_to_lunar(y, 6, 15, &ld)) {
                /* 兜底：退回按公历年计算的干支 */
                format_lunar_year_ganzhi(y, year_buf, sizeof(year_buf));
            } else {
                format_lunar_year_ganzhi(ld.year, year_buf, sizeof(year_buf));
            }
            n = lv_snprintf(p, remain, last ? "%s" : "%s\n", year_buf);
        }
        if (n < 0 || (size_t)n >= remain) {
            break;
        }
        p += n;
        remain -= (size_t)n;
    }
}

static void calendar_build_month_options(int year)
{
    char *p = s_calendar_month_opts;
    size_t remain = sizeof(s_calendar_month_opts);
    for (int m = 1; m <= 12 && remain > 1; ++m) {
        int last = (m == 12);
        int n;
        if (!s_calendar_use_lunar) {
            n = lv_snprintf(p, remain, last ? "%d月" : "%d月\n", m);
        } else {
            /* 取当月中旬某日，推算对应农历月份（含闰月信息），用于显示 */
            lunar_date_t ld;
            int sample_day = 15;
            if (!solar_to_lunar(year, m, sample_day, &ld)) {
                char month_buf[32];
                format_lunar_month_simple(m, month_buf, sizeof(month_buf));
                n = lv_snprintf(p, remain, last ? "%s" : "%s\n", month_buf);
            } else {
                char month_buf[32];
                format_lunar_month_with_leap(ld.month, ld.is_leap, month_buf, sizeof(month_buf));
                n = lv_snprintf(p, remain, last ? "%s" : "%s\n", month_buf);
            }
        }
        if (n < 0 || (size_t)n >= remain) {
            break;
        }
        p += n;
        remain -= (size_t)n;
    }
}

static void calendar_build_day_options(int year, int month)
{
    int max_day = days_in_month(year, month);
    if (max_day < 1) max_day = 1;
    if (max_day > 31) max_day = 31;
    char *p = s_calendar_day_opts;
    size_t remain = sizeof(s_calendar_day_opts);
    for (int d = 1; d <= max_day && remain > 1; ++d) {
        int last = (d == max_day);
        int n;
        if (!s_calendar_use_lunar) {
            n = lv_snprintf(p, remain, last ? "%d日" : "%d日\n", d);
        } else {
            /* 使用对应公历日期推算真实农历日，确保“农历日”与公历日对齐 */
            lunar_date_t ld;
            char day_buf[32];
            if (solar_to_lunar(year, month, d, &ld)) {
                format_lunar_day_simple(ld.day, day_buf, sizeof(day_buf));
            } else {
                /* 超出表格范围时，退回简单数字映射 */
                format_lunar_day_simple(d, day_buf, sizeof(day_buf));
            }
            n = lv_snprintf(p, remain, last ? "%s" : "%s\n", day_buf);
        }
        if (n < 0 || (size_t)n >= remain) {
            break;
        }
        p += n;
        remain -= (size_t)n;
    }
}

static void calendar_update_day_roller_from_ym(void)
{
    if (!s_calendar_roller_year || !s_calendar_roller_month || !s_calendar_roller_day) {
        return;
    }
    int y = CAL_YEAR_MIN + (int)lv_roller_get_selected(s_calendar_roller_year);
    int m = 1 + (int)lv_roller_get_selected(s_calendar_roller_month);
    int max_day = days_in_month(y, m);
    uint32_t cur_idx = lv_roller_get_selected(s_calendar_roller_day);
    if ((int)(cur_idx + 1) > max_day) {
        cur_idx = (uint32_t)(max_day - 1);
    }
    calendar_build_day_options(y, m);
    lv_roller_set_options(s_calendar_roller_day, s_calendar_day_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_calendar_roller_day, cur_idx, LV_ANIM_OFF);
}

static void calendar_set_rollers_from_date(int year, int month, int day)
{
    if (!s_calendar_roller_year || !s_calendar_roller_month || !s_calendar_roller_day) {
        return;
    }
    if (year < CAL_YEAR_MIN) year = CAL_YEAR_MIN;
    if (year > CAL_YEAR_MAX) year = CAL_YEAR_MAX;
    if (month < 1) month = 1;
    if (month > 12) month = 12;
    int max_day = days_in_month(year, month);
    if (day < 1) day = 1;
    if (day > max_day) day = max_day;

    /* 先根据年份构建月份滚轮文本（在农历模式下会使用真实农历月份/闰月名称） */
    calendar_build_month_options(year);
    lv_roller_set_options(s_calendar_roller_month, s_calendar_month_opts, LV_ROLLER_MODE_NORMAL);

    /* 再构建该年该月下的“日”滚轮文本（公历模式为数字，农历模式为真实农历日） */
    calendar_build_day_options(year, month);
    lv_roller_set_options(s_calendar_roller_day, s_calendar_day_opts, LV_ROLLER_MODE_NORMAL);

    lv_roller_set_selected(s_calendar_roller_year,  (uint16_t)(year  - CAL_YEAR_MIN), LV_ANIM_OFF);
    lv_roller_set_selected(s_calendar_roller_month, (uint16_t)(month - 1),            LV_ANIM_OFF);
    lv_roller_set_selected(s_calendar_roller_day,   (uint16_t)(day   - 1),            LV_ANIM_OFF);
}

/* 根据当前滚轮选中值和显示模式，重新生成“年 / 月 / 日”滚轮的文本 */
static void calendar_update_jump_rollers_text(void)
{
    if (!s_calendar_roller_year || !s_calendar_roller_month || !s_calendar_roller_day) {
        return;
    }
    /* 直接根据当前选中日期（s_sel_*) 重新构建滚轮文本，保证公历/农历两种模式下都与真实日期对齐 */
    calendar_build_year_options();
    lv_roller_set_options(s_calendar_roller_year, s_calendar_year_opts, LV_ROLLER_MODE_NORMAL);
    calendar_set_rollers_from_date(s_sel_y, s_sel_m, s_sel_d);
}

/* 年/月滚轮变更时，同步更新“日”滚轮 */
static void calendar_ym_roller_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    calendar_update_day_roller_from_ym();
}

/* 顶部“公历 / 农历”按钮点击：切换显示模式，仅影响文字显示 */
static void calendar_mode_toggle_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    s_calendar_use_lunar = !s_calendar_use_lunar;

    if (s_calendar_mode_toggle_label) {
        lv_label_set_text(s_calendar_mode_toggle_label,
                          s_calendar_use_lunar ? "农历" : "公历");
    }

    /* 更新“请选择年 / 月 / 日”提示文本 */
    if (s_calendar_jump_tip_label) {
        if (s_calendar_use_lunar) {
            lv_label_set_text(s_calendar_jump_tip_label, "请选择农历年 / 月 / 日");
        } else {
            lv_label_set_text(s_calendar_jump_tip_label, "请选择年 / 月 / 日");
        }
    }

    /* 根据模式刷新日历与弹窗滚轮文本 */
    calendar_update_month_view(&guider_ui);
    calendar_update_jump_rollers_text();
}

/* 头部“今天是 …”点击：弹出日期选择弹窗 */
static void calendar_today_label_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (!s_calendar_jump_modal) {
        return;
    }
    /* 显示遮罩层，并确保位于本屏最上层（覆盖 calendar_month_view_cont 等所有内容） */
    lv_obj_clear_flag(s_calendar_jump_modal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_calendar_jump_modal);
}

/* 弹窗按钮：按当前滚轮值跳转到某年某月 / 某年某月某日，或关闭 */
static void calendar_jump_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    lv_obj_t *target = lv_event_get_target(e);

    if (target == s_calendar_jump_btn_month || target == s_calendar_jump_btn_date) {
        int year  = s_cur_year;
        int month = s_cur_month;
        int day   = s_sel_d;

        if (s_calendar_roller_year && s_calendar_roller_month) {
            int y_idx = (int)lv_roller_get_selected(s_calendar_roller_year);
            int m_idx = (int)lv_roller_get_selected(s_calendar_roller_month);
            year  = CAL_YEAR_MIN + y_idx;
            month = 1 + m_idx;

            int max_day = days_in_month(year, month);
            if (s_calendar_roller_day) {
                int d_idx = (int)lv_roller_get_selected(s_calendar_roller_day);
                day = d_idx + 1;
                if (day > max_day) {
                    day = max_day;
                }
            } else {
                day = 1;
            }
        }

        if (target == s_calendar_jump_btn_month) {
            /* 跳转到该月：月份视图跳转到所选年/月，顶部日期显示该月1日 */
            s_cur_year  = year;
            s_cur_month = month;
            s_sel_y     = year;
            s_sel_m     = month;
            s_sel_d     = 1;
            calendar_update_month_view(&guider_ui);
        } else {
            /* 跳转到该日：同时更新选中日期，高亮该日 */
            s_cur_year  = year;
            s_cur_month = month;
            s_sel_y     = year;
            s_sel_m     = month;
            s_sel_d     = day;
            calendar_update_month_view(&guider_ui);
        }
    } else if (target == s_calendar_jump_btn_cancel) {
        /* 仅关闭，不做任何跳转 */
    }

    if (s_calendar_jump_modal) {
        lv_obj_add_flag(s_calendar_jump_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 日历主体容器点击：左右空白区域切换上一月/下一月 */
static void calendar_body_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) {
        return;
    }

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_obj_t *body  = lv_event_get_target(e);
    lv_obj_t *month = guider_ui.calendar_month_view_cont;
    if (!body || !month) {
        return;
    }

    lv_area_t a_body;
    lv_area_t a_month;
    lv_obj_get_coords(body, &a_body);
    lv_obj_get_coords(month, &a_month);

    /* 仅当点击位置在月视图垂直范围内，且位于其左右空白区域时才触发 */
    if (p.y < a_month.y1 || p.y > a_month.y2) {
        return;
    }

    if (p.x < a_month.x1) {
        /* 左侧空白：上一月 */
        s_cur_month--;
        if (s_cur_month == 0) {
            s_cur_month = 12;
            s_cur_year--;
        }
        calendar_update_month_view(&guider_ui);
    } else if (p.x > a_month.x2) {
        /* 右侧空白：下一月 */
        s_cur_month++;
        if (s_cur_month == 13) {
            s_cur_month = 1;
            s_cur_year++;
        }
        calendar_update_month_view(&guider_ui);
    }
}

/* 顶部日期文本：
 * - 公历模式：YYYY年MM月DD日
 * - 农历模式：天干地支年 + 农历月日（示例：甲辰年正月初一）
 */
static void calendar_update_header_date(lv_ui *ui)
{
    if (!ui || !ui->calendar_today_label) return;
    char buf[64];
    if (!s_calendar_use_lunar) {
        lv_snprintf(buf, sizeof(buf), "%04d年%02d月%02d日", s_sel_y, s_sel_m, s_sel_d);
    } else {
        lunar_date_t ld;
        solar_to_lunar(s_sel_y, s_sel_m, s_sel_d, &ld);
        char year_buf[32];
        char month_buf[32];
        char day_buf[32];
        format_lunar_year_ganzhi(ld.year, year_buf, sizeof(year_buf));
        format_lunar_month_with_leap(ld.month, ld.is_leap, month_buf, sizeof(month_buf));
        format_lunar_day_simple(ld.day, day_buf, sizeof(day_buf));
        lv_snprintf(buf, sizeof(buf), "%s%s%s", year_buf, month_buf, day_buf);
    }
    lv_label_set_text(ui->calendar_today_label, buf);
}

/* 点击某个日期格子：仅更新选中高亮（使用全局 guider_ui） */
static void cell_event_cb_fixed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    cal_cell_t *cell = (cal_cell_t *)lv_event_get_user_data(e);
    if (!cell || cell->day <= 0) return;
    s_sel_y = cell->year;
    s_sel_m = cell->month;
    s_sel_d = cell->day;
    calendar_update_month_view(&guider_ui);
}

/* 刷新顶部年月文字和所有格子 */
static void calendar_update_month_view(lv_ui *ui)
{
    if (!ui) return;

    /* 顶部当前年月标签（使用导航中间的 label，如果存在的话） */
    if (ui->calendar_nav_month_label) {
        char buf[32];
        if (!s_calendar_use_lunar) {
            lv_snprintf(buf, sizeof(buf), "%04d 年 %02d 月", s_cur_year, s_cur_month);
        } else {
            /* 以当前页面所示的公历年月的第一天来估算对应的农历年/月 */
            lunar_date_t ld;
            solar_to_lunar(s_cur_year, s_cur_month, 1, &ld);
            char year_buf[32];
            char month_buf[32];
            format_lunar_year_ganzhi(ld.year, year_buf, sizeof(year_buf));
            format_lunar_month_with_leap(ld.month, ld.is_leap, month_buf, sizeof(month_buf));
            lv_snprintf(buf, sizeof(buf), "%s%s", year_buf, month_buf);
        }
        lv_label_set_text(ui->calendar_nav_month_label, buf);
    }

    /* 同步更新顶部“当前日期”文本 */
    calendar_update_header_date(ui);

    /* 填充 42 个格子：前导上一月、当前月、尾部下一月 */
    int year  = s_cur_year;
    int month = s_cur_month;
    int days  = days_in_month(year, month);
    int w0    = weekday_of(year, month, 1);  /* 0=周日 */

    int prev_m = month - 1;
    int prev_y = year;
    if (prev_m == 0) { prev_m = 12; prev_y--; }
    int prev_days = days_in_month(prev_y, prev_m);

    int next_m = month + 1;
    int next_y = year;
    if (next_m == 13) { next_m = 1; next_y++; }

    int idx = 0;
    /* 前导：上一月尾巴 */
    for (int i = 0; i < w0 && idx < 42; ++i) {
        int d = prev_days - w0 + 1 + i;
        cal_cell_t *cell = &s_cells[idx++];
        cell->year  = prev_y;
        cell->month = prev_m;
        cell->day   = d;
        cell->other_month = true;
    }
    /* 当前月 */
    for (int d = 1; d <= days && idx < 42; ++d) {
        cal_cell_t *cell = &s_cells[idx++];
        cell->year  = year;
        cell->month = month;
        cell->day   = d;
        cell->other_month = false;
    }
    /* 尾部：下一月开头 */
    int nd = 1;
    while (idx < 42) {
        cal_cell_t *cell = &s_cells[idx++];
        cell->year  = next_y;
        cell->month = next_m;
        cell->day   = nd++;
        cell->other_month = true;
    }

    /* 根据状态刷新所有格子显示与样式（深色主题） */
    for (int i = 0; i < 42; ++i) {
        cal_cell_t *cell = &s_cells[i];
        if (!cell->btn || !cell->label) continue;

        char txt[32];
        if (!s_calendar_use_lunar) {
            lv_snprintf(txt, sizeof(txt), "%d", cell->day);
        } else {
            lunar_date_t ld;
            if (solar_to_lunar(cell->year, cell->month, cell->day, &ld)) {
                format_lunar_day_simple(ld.day, txt, sizeof(txt));
            } else {
                /* 超出表格范围时退回简单映射，保证界面稳定 */
                format_lunar_day_simple(cell->day, txt, sizeof(txt));
            }
        }
        lv_label_set_text(cell->label, txt);

        /* 深色基础样式：深色底、细边框 */
        lv_obj_set_style_bg_opa(cell->btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(cell->btn, lv_color_hex(0x151515), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(cell->btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(cell->btn, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);

        /* 深色主题下，日历数字统一使用浅色文字 */
        lv_color_t txt_color = lv_color_hex(0xffffff);

        /* 今日加蓝色边框 */
        bool is_today = same_date(cell->year, cell->month, cell->day,
                                  s_today_y, s_today_m, s_today_d);
        if (is_today) {
            lv_obj_set_style_border_color(cell->btn, lv_color_hex(0x1677ff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        /* 选中日期高亮为蓝底白字 */
        bool is_sel = same_date(cell->year, cell->month, cell->day,
                                s_sel_y, s_sel_m, s_sel_d);
        if (is_sel) {
            lv_obj_set_style_bg_color(cell->btn, lv_color_hex(0x1677ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            txt_color = lv_color_hex(0xffffff);
        }

        lv_obj_set_style_text_color(cell->label, txt_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void setup_scr_calendar_screen_dark(lv_ui *ui)
{
    if (!ui) return;

    /* 初始化今天/当前月/选中日期 */
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    s_today_y = t.tm_year + 1900;
    s_today_m = t.tm_mon + 1;
    s_today_d = t.tm_mday;
    s_cur_year  = s_today_y;
    s_cur_month = s_today_m;
    s_sel_y     = s_today_y;
    s_sel_m     = s_today_m;
    s_sel_d     = s_today_d;

    /* 根屏幕 */
    ui->calendar_main = lv_obj_create(NULL);
    lv_obj_set_size(ui->calendar_main, 480, 320);
    lv_obj_set_scrollbar_mode(ui->calendar_main, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->calendar_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->calendar_main, LV_DIR_NONE);
    lv_obj_set_style_bg_opa(ui->calendar_main, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->calendar_main, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 顶部栏容器 */
    ui->calendar_header_cont = lv_obj_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_header_cont, 0, 0);
    lv_obj_set_size(ui->calendar_header_cont, 480, 50);
    lv_obj_set_scrollbar_mode(ui->calendar_header_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->calendar_header_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->calendar_header_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->calendar_header_cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->calendar_header_cont, lv_color_hex(0x152c3a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->calendar_header_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 返回图标 */
    /*
    ui->calendar_img_back = lv_image_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_img_back, 6, 10);
    lv_obj_set_size(ui->calendar_img_back, 30, 30);
    lv_obj_add_flag(ui->calendar_img_back, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->calendar_img_back, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->calendar_img_back, 50, 50);
    lv_image_set_rotation(ui->calendar_img_back, 0);
    lv_obj_set_style_image_recolor_opa(ui->calendar_img_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->calendar_img_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    */
    
    //Write codes calendar_img_back
    ui->calendar_img_back = lv_image_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_img_back, 2, 10);
    lv_obj_set_size(ui->calendar_img_back, 30, 30);
    lv_obj_add_flag(ui->calendar_img_back, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->calendar_img_back, &_return_RGB565A8_30x30);
    lv_image_set_pivot(ui->calendar_img_back, 50,50);
    lv_image_set_rotation(ui->calendar_img_back, 0);

    //Write style for calendar_img_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->calendar_img_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->calendar_img_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 返回按钮（点击区域） */
    /*
    ui->calendar_btn_back = lv_button_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_btn_back, 4, 8);
    lv_obj_set_size(ui->calendar_btn_back, 36, 34);
    ui->calendar_btn_back_label = lv_label_create(ui->calendar_btn_back);
    lv_label_set_text(ui->calendar_btn_back_label, "");
    lv_label_set_long_mode(ui->calendar_btn_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->calendar_btn_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->calendar_btn_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->calendar_btn_back_label, LV_PCT(100));
    lv_obj_set_style_bg_opa(ui->calendar_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->calendar_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    */
    
    //Write codes calendar_btn_back
    ui->calendar_btn_back = lv_button_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_btn_back, 4, 10);
    lv_obj_set_size(ui->calendar_btn_back, 30, 30);
    ui->calendar_btn_back_label = lv_label_create(ui->calendar_btn_back);
    lv_label_set_text(ui->calendar_btn_back_label, "");
    lv_label_set_long_mode(ui->calendar_btn_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->calendar_btn_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->calendar_btn_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->calendar_btn_back_label, LV_PCT(100));

    //Write style for calendar_btn_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->calendar_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->calendar_btn_back, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->calendar_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->calendar_btn_back, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->calendar_btn_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->calendar_btn_back, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->calendar_btn_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->calendar_btn_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->calendar_btn_back, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->calendar_btn_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->calendar_btn_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 标题（左上角） */
    ui->calendar_title = lv_label_create(ui->calendar_header_cont);
    lv_obj_set_pos(ui->calendar_title, 30, 0);
    lv_label_set_text(ui->calendar_title, "日历Calendar");
    lv_label_set_long_mode(ui->calendar_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(ui->calendar_title, &lv_font_cn_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->calendar_title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 头部右侧：公历 / 农历切换按钮 */
    s_calendar_mode_toggle_btn = lv_button_create(ui->calendar_header_cont);
    lv_obj_set_pos(s_calendar_mode_toggle_btn, 390, -5);
    lv_obj_set_size(s_calendar_mode_toggle_btn, 60, 32);
    lv_obj_set_style_radius(s_calendar_mode_toggle_btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_calendar_mode_toggle_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_mode_toggle_btn, lv_color_hex(0x2195f6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_calendar_mode_toggle_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_calendar_mode_toggle_btn, calendar_mode_toggle_event_cb, LV_EVENT_CLICKED, ui);

    s_calendar_mode_toggle_label = lv_label_create(s_calendar_mode_toggle_btn);
    lv_label_set_text(s_calendar_mode_toggle_label, "公历");
    lv_obj_center(s_calendar_mode_toggle_label);
    lv_obj_set_style_text_font(s_calendar_mode_toggle_label, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_calendar_mode_toggle_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 头部右侧：“当前日期”标签（可点击，文本为“YYYY年MM月DD日” / 农历） */
    ui->calendar_today_label = lv_label_create(ui->calendar_header_cont);
    lv_obj_set_pos(ui->calendar_today_label, 200, 0);
    lv_obj_set_width(ui->calendar_today_label, 220);
    lv_label_set_long_mode(ui->calendar_today_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(ui->calendar_today_label, &lv_font_cn_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->calendar_today_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(ui->calendar_today_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->calendar_today_label, calendar_today_label_event_cb, LV_EVENT_CLICKED, ui);
    /* 初始化为“今天”的年月日 */
    calendar_update_header_date(ui);

    /* “今天是 …” 弹窗：默认隐藏，通过点击标签显示 */
    s_calendar_jump_modal = lv_obj_create(ui->calendar_main);
    lv_obj_set_pos(s_calendar_jump_modal, 0, 0);
    lv_obj_set_size(s_calendar_jump_modal, 480, 320);
    lv_obj_add_flag(s_calendar_jump_modal, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_calendar_jump_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_calendar_jump_modal, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_jump_modal, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_calendar_jump_modal, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *jump_panel = lv_obj_create(s_calendar_jump_modal);
    lv_obj_set_size(jump_panel, 420, 220);
    lv_obj_align(jump_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(jump_panel, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(jump_panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(jump_panel, lv_color_hex(0x0c141c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(jump_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(jump_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *jump_title = lv_label_create(jump_panel);
    lv_obj_set_pos(jump_title, 16, 0);
    lv_obj_set_width(jump_title, 328);
    lv_label_set_long_mode(jump_title, LV_LABEL_LONG_CLIP);
    lv_label_set_text(jump_title, "选择跳转日期");
    lv_obj_set_style_text_font(jump_title, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(jump_title, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_calendar_jump_tip_label = lv_label_create(jump_panel);
    lv_obj_set_pos(s_calendar_jump_tip_label, 16, 24);
    lv_label_set_text(s_calendar_jump_tip_label, "请选择年 / 月 / 日");
    lv_obj_set_style_text_font(s_calendar_jump_tip_label, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_calendar_jump_tip_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_calendar_roller_year = lv_roller_create(jump_panel);
    lv_obj_set_pos(s_calendar_roller_year, 16, 50);
    lv_obj_set_size(s_calendar_roller_year, 120, 70);
    /* 年 / 月 / 日滚轮 */
    calendar_build_year_options();
    lv_roller_set_options(s_calendar_roller_year, s_calendar_year_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_calendar_roller_year, 3);
    lv_obj_set_style_text_font(s_calendar_roller_year, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_calendar_roller_year, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_calendar_roller_year, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_roller_year, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_calendar_roller_month = lv_roller_create(jump_panel);
    lv_obj_set_pos(s_calendar_roller_month, 140, 50);
    lv_obj_set_size(s_calendar_roller_month, 90, 70);
    lv_roller_set_options(s_calendar_roller_month, s_calendar_month_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_calendar_roller_month, 3);
    lv_obj_set_style_text_font(s_calendar_roller_month, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_calendar_roller_month, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_calendar_roller_month, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_roller_month, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    s_calendar_roller_day = lv_roller_create(jump_panel);
    lv_obj_set_pos(s_calendar_roller_day, 234, 50);
    lv_obj_set_size(s_calendar_roller_day, 90, 70);
    lv_roller_set_visible_row_count(s_calendar_roller_day, 3);
    lv_obj_set_style_text_font(s_calendar_roller_day, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_calendar_roller_day, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_calendar_roller_day, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_roller_day, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(s_calendar_roller_year,  calendar_ym_roller_event_cb, LV_EVENT_VALUE_CHANGED, ui);
    lv_obj_add_event_cb(s_calendar_roller_month, calendar_ym_roller_event_cb, LV_EVENT_VALUE_CHANGED, ui);

    /* 初始滚轮值：使用当前月/选中日 */
    calendar_set_rollers_from_date(s_cur_year, s_cur_month, s_sel_d);

    s_calendar_jump_btn_month = lv_button_create(jump_panel);
    lv_obj_set_pos(s_calendar_jump_btn_month, 10, 160);
    lv_obj_set_size(s_calendar_jump_btn_month, 120, 40);
    lv_obj_set_style_radius(s_calendar_jump_btn_month, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* 浅蓝色背景，区分为“跳转到该月” */
    lv_obj_set_style_bg_color(s_calendar_jump_btn_month, lv_color_hex(0x253244), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *lbl_month = lv_label_create(s_calendar_jump_btn_month);
    lv_label_set_text(lbl_month, "跳转到该月");
    lv_obj_set_style_text_font(lbl_month, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_month, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl_month);

    s_calendar_jump_btn_date = lv_button_create(jump_panel);
    lv_obj_set_pos(s_calendar_jump_btn_date, 135, 160);
    lv_obj_set_size(s_calendar_jump_btn_date, 120, 40);
    lv_obj_set_style_radius(s_calendar_jump_btn_date, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_jump_btn_date, lv_color_hex(0x2195f6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *lbl_date = lv_label_create(s_calendar_jump_btn_date);
    lv_label_set_text(lbl_date, "跳转到该日");
    lv_obj_set_style_text_font(lbl_date, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl_date);

    s_calendar_jump_btn_cancel = lv_button_create(jump_panel);
    lv_obj_set_pos(s_calendar_jump_btn_cancel, 265, 160);
    lv_obj_set_size(s_calendar_jump_btn_cancel, 96, 40);
    lv_obj_set_style_radius(s_calendar_jump_btn_cancel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_calendar_jump_btn_cancel, lv_color_hex(0x253244), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *lbl_cancel = lv_label_create(s_calendar_jump_btn_cancel);
    lv_label_set_text(lbl_cancel, "取消");
    lv_obj_set_style_text_font(lbl_cancel, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl_cancel);

    lv_obj_add_event_cb(s_calendar_jump_btn_month,  calendar_jump_btn_event_cb, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(s_calendar_jump_btn_date,   calendar_jump_btn_event_cb, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(s_calendar_jump_btn_cancel, calendar_jump_btn_event_cb, LV_EVENT_CLICKED, ui);

    /* 在 (0,55) 开始、大小 480×265 的区域作为日历主体区域 */
    ui->calendar_body_cont = lv_obj_create(ui->calendar_main);
    lv_obj_set_pos(ui->calendar_body_cont, 0, 55);
    lv_obj_set_size(ui->calendar_body_cont, 480, 265);
    lv_obj_set_scrollbar_mode(ui->calendar_body_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->calendar_body_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui->calendar_body_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->calendar_body_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui->calendar_body_cont, calendar_body_event_cb, LV_EVENT_CLICKED, ui);

    /* 在主体区域中间放置一个空白圆角矩形容器（无填充，仅边框） */
    ui->calendar_month_view_cont = lv_obj_create(ui->calendar_body_cont);
    lv_obj_set_size(ui->calendar_month_view_cont, 440, 230);
    lv_obj_align(ui->calendar_month_view_cont, LV_ALIGN_CENTER, 0, 0);
    //lv_obj_add_flag(ui->calendar_month_view_cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_scrollbar_mode(ui->calendar_month_view_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->calendar_month_view_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->calendar_month_view_cont, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->calendar_month_view_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->calendar_month_view_cont, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->calendar_month_view_cont, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 在圆角矩形容器顶部显示“日 一 二 三 四 五 六” */
    const char *WK[7] = {"日","一","二","三","四","五","六"};
    int total_w = 400;
    int item_w  = total_w / 7;   /* 平分 7 份，自动留出合理间隔 */
    int weekday_y = 0;           /* 距离容器顶部 0 像素 */
    int weekday_h = 24;          /* 星期标题行高 */

    for (int i = 0; i < 7; ++i) {
        lv_obj_t *lbl = lv_label_create(ui->calendar_month_view_cont);
        lv_label_set_text(lbl, WK[i]); 
        lv_obj_set_size(lbl, item_w, weekday_h);
        /* 整体左移 5 像素 */
        lv_obj_set_pos(lbl, i * item_w - 5, weekday_y);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl, &lv_font_cn_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /* 在星期行下方创建 7×6 月日历网格（使用 12 号字体） */
    {
        int row_h    = 28;                  /* 每行高度 */
        int grid_y   = weekday_y + weekday_h - 15;  /* 原来+6，整体上移21像素 => -15 */

        ui->calendar_month_days = lv_obj_create(ui->calendar_month_view_cont);
        lv_obj_set_pos(ui->calendar_month_days, 0, grid_y);
        lv_obj_set_size(ui->calendar_month_days, 440, 230);
        //lv_obj_add_flag(ui->calendar_month_days, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_scrollbar_mode(ui->calendar_month_days, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(ui->calendar_month_days, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(ui->calendar_month_days, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui->calendar_month_days, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        for (int i = 0; i < 42; ++i) {
            cal_cell_t *cell = &s_cells[i];
            int row = i / 7;
            int col = i % 7;
            /* 与顶部“日 一 二 三 四 五 六”列对齐：整体左移 20 像素 */
            int x = col * item_w - 20;
            int y = row * row_h;

            cell->btn = lv_button_create(ui->calendar_month_days);
            lv_obj_set_pos(cell->btn, x + 6, y + 4);
            lv_obj_set_size(cell->btn, item_w - 12, row_h - 8);
            lv_obj_set_style_radius(cell->btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(cell->btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(cell->btn, lv_color_hex(0x151515), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(cell->btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(cell->btn, lv_color_hex(0xe0e0e0), LV_PART_MAIN | LV_STATE_DEFAULT);

            cell->label = lv_label_create(cell->btn);
            lv_label_set_text(cell->label, "--");
            lv_obj_center(cell->label);
            lv_obj_set_style_text_font(cell->label, &lv_font_cn_12, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_add_event_cb(cell->btn, cell_event_cb_fixed, LV_EVENT_CLICKED, cell);
        }
    }

    /* 刷新一次视图（填充日期） */
    calendar_update_month_view(ui);

    /* 更新布局并初始化本屏事件（主要是返回切换动画，深色主题） */
    lv_obj_update_layout(ui->calendar_main);
    events_init_calendar_main_dark(ui);
}
