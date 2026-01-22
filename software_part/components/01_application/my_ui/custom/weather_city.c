#include "weather_city.h"
#include <string.h>
#include <stdlib.h>

// 嵌入的 CSV 资源符号（由 EMBED_TXTFILES 生成）
/* 符号名取自生成的 admin_divisions_2023.csv.S：_binary_admin_divisions_2023_csv_start/end */
extern const unsigned char admin_divisions_csv_start[] asm("_binary_admin_divisions_2023_csv_start");
extern const unsigned char admin_divisions_csv_end[]   asm("_binary_admin_divisions_2023_csv_end");

typedef struct {
    uint32_t count;
    uint32_t cap;
    uint32_t *codes; // 与 options 中的行一一对应
} code_list_t;

static lv_ui *s_ui = NULL;
static const char *s_csv;        // 指向嵌入 CSV 的起始
static size_t      s_csv_len;    // 长度

// 当前三联选择的代码映射（与 UI 下拉的 options 同步）
static code_list_t s_codes_prov = {0};
static code_list_t s_codes_pref = {0};
static code_list_t s_codes_county = {0};

// 前向声明：为避免隐式声明导致的构建错误
static void dd_apply_font(lv_obj_t *dd);

static void free_codes(code_list_t *cl)
{
    if(!cl) return;
    if(cl->codes) lv_free(cl->codes);
    cl->codes = NULL;
    cl->count = cl->cap = 0;
}

static bool push_code(code_list_t *cl, uint32_t code)
{
    if(cl->count >= cl->cap) {
        uint32_t ncap = cl->cap ? (cl->cap * 2) : 16;
        uint32_t *ncodes = (uint32_t*)lv_realloc(cl->codes, ncap * sizeof(uint32_t));
        if(!ncodes) return false;
        cl->codes = ncodes;
        cl->cap = ncap;
    }
    cl->codes[cl->count++] = code;
    return true;
}

// 简单 CSV 行迭代：返回下一行的起始指针，并把当前行 [out_start, out_end) 输出
static const char* next_line(const char *cur, const char *end, const char **out_start, const char **out_end)
{
    if(cur >= end) return NULL;
    const char *s = cur;
    const char *e = s;
    while(e < end && *e != '\n') e++;
    // 去掉结尾的 \r
    const char *line_end = (e > s && *(e-1) == '\r') ? (e-1) : e;
    *out_start = s;
    *out_end = line_end;
    return (e < end) ? (e + 1) : e; // 下一个起点
}

// 从一行中取逗号分隔的字段，简单实现（不处理带引号的逗号）
static void split_fields(const char *s, const char *e, const char **fields, int max_fields)
{
    int idx = 0;
    const char *p = s;
    while(p < e && idx < max_fields) {
        const char *q = p;
        while(q < e && *q != ',') q++;
        fields[idx++] = p;
        if(q >= e) break;      // 本字段已到行尾
        p = q + 1;             // 跳过逗号到下一个字段
    }
    // 填充缺失字段为行尾指针（调用侧据此判断缺失）
    for(; idx < max_fields; idx++) fields[idx] = e;
}

// 将 [s,e) 的无符号整数字符串转为整数（失败返回 0）
static uint32_t to_u32(const char *s, const char *e)
{
    uint32_t v = 0;
    for(const char *p = s; p < e; p++) {
        if(*p < '0' || *p > '9') return 0;
        v = v * 10u + (uint32_t)(*p - '0');
    }
    return v;
}

// 动态拼接带换行的 options 字符串
typedef struct {
    char *buf; size_t len; size_t cap;
} strb_t;

static bool sb_init(strb_t *sb, size_t cap)
{ sb->buf = (char*)lv_malloc(cap); if(!sb->buf) return false; sb->len=0; sb->cap=cap; sb->buf[0]='\0'; return true; }
static bool sb_append_n(strb_t *sb, const char *s, size_t n)
{ if(sb->len + n + 2 > sb->cap){ size_t ncap = (sb->cap*2 > sb->len+n+2)? sb->cap*2 : (sb->len+n+2); char *nb=(char*)lv_realloc(sb->buf,ncap); if(!nb) return false; sb->buf=nb; sb->cap=ncap;} memcpy(sb->buf+sb->len,s,n); sb->len+=n; sb->buf[sb->len]='\0'; return true; }
static bool sb_append_c(strb_t *sb, char c)
{ if(sb->len + 2 > sb->cap){ size_t ncap = sb->cap? sb->cap*2 : 64; char *nb=(char*)lv_realloc(sb->buf,ncap); if(!nb) return false; sb->buf=nb; sb->cap=ncap;} sb->buf[sb->len++]=c; sb->buf[sb->len]='\0'; return true; }
static void sb_free(strb_t *sb)
{ if(sb->buf){ lv_free(sb->buf); sb->buf=NULL; } sb->len=sb->cap=0; }

// 过滤出省级，写入 options 和 codes
static bool build_provinces(strb_t *opts, code_list_t *codes)
{
    free_codes(codes);
    const char *p = s_csv; const char *end = s_csv + s_csv_len;
    const char *ls, *le; // line start/end
    // 跳过首行表头
    p = next_line(p, end, &ls, &le);
    for( ; (p = next_line(p, end, &ls, &le)); ) {
        const char *f[7] = {0};
        split_fields(ls, le, f, 7);
        // f[0]=code, f[1]=name, f[2]=level
        if(f[2] >= le) continue;
        if((le - f[2]) >= 8 && strncmp(f[2], "province", 8) == 0) {
            uint32_t code = to_u32(f[0], (const char*)memchr(f[0], ',', le - f[0]) ?: le);
            if(!push_code(codes, code)) return false;
            size_t nlen = (size_t)((const char*)memchr(f[1], ',', le - f[1]) ?: le) - (size_t)(f[1]);
            if(opts->len > 0) sb_append_c(opts, '\n');
            if(!sb_append_n(opts, f[1], nlen)) return false;
        }
    }
    return true;
}

// 过滤出指定省下的“市级/直辖市本级”
static bool build_prefectures(uint32_t prov_code, strb_t *opts, code_list_t *codes)
{
    free_codes(codes);
    const char *p = s_csv; const char *end = s_csv + s_csv_len; const char *ls,*le;
    // 跳过表头
    p = next_line(p, end, &ls, &le);
    for( ; (p = next_line(p, end, &ls, &le)); ) {
        const char *f[7] = {0}; split_fields(ls, le, f, 7);
        uint32_t prov = to_u32(f[3], (const char*)memchr(f[3], ',', le - f[3]) ?: le);
        if(prov != prov_code) continue;
        // 以 prefecture_name 分组。prefecture_code 可能为空（直辖市用 province_code）
        uint32_t pref_code = to_u32(f[5], (const char*)memchr(f[5], ',', le - f[5]) ?: le);
        if(pref_code == 0) pref_code = prov_code;
        // 仅采集一条 pref_code 的代表行
        // 为简化，这里依据“第一个出现的 pref_code”追加一项；避免重复需 O(N^2) 检查
        bool seen = false;
        for(uint32_t i=0;i<codes->count;i++){ if(codes->codes[i]==pref_code){ seen=true; break; } }
        if(seen) continue;
        if(!push_code(codes, pref_code)) return false;
        const char *nm = f[6];
        size_t nlen = (size_t)((const char*)memchr(nm, ',', le - nm) ?: le) - (size_t)nm;
        if(opts->len > 0) sb_append_c(opts, '\n');
        if(nlen==0){ // 兜底：pref 名为空时用 province_name
            nm = f[4]; nlen = (size_t)((const char*)memchr(nm, ',', le - nm) ?: le) - (size_t)nm;
        }
        if(!sb_append_n(opts, nm, nlen)) return false;
    }
    return true;
}

// 过滤出指定省/市下的区县（level=county）
static bool build_counties(uint32_t prov_code, uint32_t pref_code, strb_t *opts, code_list_t *codes)
{
    free_codes(codes);
    const char *p = s_csv; const char *end = s_csv + s_csv_len; const char *ls,*le;
    p = next_line(p, end, &ls, &le);
    for( ; (p = next_line(p, end, &ls, &le)); ) {
        const char *f[7] = {0}; split_fields(ls, le, f, 7);
        if((le - f[2]) >= 6 && strncmp(f[2], "county", 6) == 0) {
            uint32_t prov = to_u32(f[3], (const char*)memchr(f[3], ',', le - f[3]) ?: le);
            if(prov != prov_code) continue;
            uint32_t pref = to_u32(f[5], (const char*)memchr(f[5], ',', le - f[5]) ?: le);
            if(pref == 0) pref = prov;
            if(pref != pref_code) continue;
            uint32_t code = to_u32(f[0], (const char*)memchr(f[0], ',', le - f[0]) ?: le);
            if(!push_code(codes, code)) return false;
            const char *nm = f[1];
            size_t nlen = (size_t)((const char*)memchr(nm, ',', le - nm) ?: le) - (size_t)nm;
            if(opts->len > 0) sb_append_c(opts, '\n');
            if(!sb_append_n(opts, nm, nlen)) return false;
        }
    }
    return true;
}

static void dd_prov_changed(lv_event_t *e)
{
    if(!s_ui || !s_ui->weather_city_dd_prov || !s_ui->weather_city_dd_pref || !s_ui->weather_city_dd_county) return;
    uint32_t sel = lv_dropdown_get_selected(s_ui->weather_city_dd_prov);
    if(sel >= s_codes_prov.count) return;
    uint32_t prov_code = s_codes_prov.codes[sel];
    // 重建 prefecture
    strb_t sb; if(!sb_init(&sb, 128)) return;
    if(!build_prefectures(prov_code, &sb, &s_codes_pref)) { sb_free(&sb); return; }
    lv_dropdown_set_options(s_ui->weather_city_dd_pref, sb.buf && sb.len? sb.buf : "-");
    lv_dropdown_set_selected(s_ui->weather_city_dd_pref, 0);
    dd_apply_font(s_ui->weather_city_dd_pref);
    sb_free(&sb);
    // 同步 county：以首个市为默认构建
    uint32_t pref_code = (s_codes_pref.count>0)? s_codes_pref.codes[0] : prov_code;
    strb_t sb2; if(!sb_init(&sb2, 128)) return;
    if(!build_counties(prov_code, pref_code, &sb2, &s_codes_county)) { sb_free(&sb2); return; }
    lv_dropdown_set_options(s_ui->weather_city_dd_county, sb2.buf && sb2.len? sb2.buf : "-");
    lv_dropdown_set_selected(s_ui->weather_city_dd_county, 0);
    dd_apply_font(s_ui->weather_city_dd_county);
    sb_free(&sb2);
}

static void dd_pref_changed(lv_event_t *e)
{
    if(!s_ui || !s_ui->weather_city_dd_prov || !s_ui->weather_city_dd_pref || !s_ui->weather_city_dd_county) return;
    uint32_t selp = lv_dropdown_get_selected(s_ui->weather_city_dd_prov);
    if(selp >= s_codes_prov.count) return;
    uint32_t prov_code = s_codes_prov.codes[selp];
    uint32_t sel = lv_dropdown_get_selected(s_ui->weather_city_dd_pref);
    uint32_t pref_code = (sel < s_codes_pref.count) ? s_codes_pref.codes[sel] : prov_code;
    strb_t sb; if(!sb_init(&sb, 128)) return;
    if(!build_counties(prov_code, pref_code, &sb, &s_codes_county)) { sb_free(&sb); return; }
    lv_dropdown_set_options(s_ui->weather_city_dd_county, sb.buf && sb.len? sb.buf : "-");
    lv_dropdown_set_selected(s_ui->weather_city_dd_county, 0);
    dd_apply_font(s_ui->weather_city_dd_county);
    sb_free(&sb);
}

void weather_city_setup(lv_ui *ui)
{
    s_ui = ui;
    s_csv = (const char*)admin_divisions_csv_start;
    s_csv_len = (size_t)(admin_divisions_csv_end - admin_divisions_csv_start);
    if(!ui || !ui->weather_city_dd_prov || !ui->weather_city_dd_pref || !ui->weather_city_dd_county) return;

    // 构建省份
    strb_t sb; if(!sb_init(&sb, 256)) return;
    if(!build_provinces(&sb, &s_codes_prov)) { sb_free(&sb); return; }
    lv_dropdown_set_options(ui->weather_city_dd_prov, sb.buf && sb.len? sb.buf : "-");
    lv_dropdown_set_selected(ui->weather_city_dd_prov, 0);
    dd_apply_font(ui->weather_city_dd_prov);
    sb_free(&sb);

    // 首个省的市与区县
    dd_prov_changed(NULL);

    // 绑定事件（值变更联动）
    lv_obj_add_event_cb(ui->weather_city_dd_prov, dd_prov_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui->weather_city_dd_pref, dd_pref_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

uint32_t weather_city_get_selected_province_code(void)
{
    if(!s_ui || !s_ui->weather_city_dd_prov) return 0;
    uint32_t sel = lv_dropdown_get_selected(s_ui->weather_city_dd_prov);
    if(sel >= s_codes_prov.count) return 0;
    return s_codes_prov.codes[sel];
}

uint32_t weather_city_get_selected_pref_code(void)
{
    if(!s_ui || !s_ui->weather_city_dd_pref) return 0;
    uint32_t sel = lv_dropdown_get_selected(s_ui->weather_city_dd_pref);
    if(sel >= s_codes_pref.count) return 0;
    return s_codes_pref.codes[sel];
}

uint32_t weather_city_get_selected_county_code(void)
{
    if(!s_ui || !s_ui->weather_city_dd_county) return 0;
    uint32_t sel = lv_dropdown_get_selected(s_ui->weather_city_dd_county);
    if(sel >= s_codes_county.count) return 0;
    return s_codes_county.codes[sel];
}

void weather_city_get_selected_codes(uint32_t *prov_code, uint32_t *pref_code, uint32_t *county_code)
{
    if (prov_code)   *prov_code   = weather_city_get_selected_province_code();
    if (pref_code)   *pref_code   = weather_city_get_selected_pref_code();
    if (county_code) *county_code = weather_city_get_selected_county_code();
}

// 根据 code 回设下拉框，若找不到则忽略该层；会触发联动重建下一层 options
void weather_city_select_codes(lv_ui *ui, uint32_t prov_code, uint32_t pref_code, uint32_t county_code)
{
    (void)ui; // 与 s_ui 相同
    if (!s_ui || !s_ui->weather_city_dd_prov || !s_ui->weather_city_dd_pref || !s_ui->weather_city_dd_county) return;

    // 省
    if (prov_code) {
        uint32_t idx = 0, found = 0;
        for (; idx < s_codes_prov.count; ++idx) if (s_codes_prov.codes[idx] == prov_code) { found = 1; break; }
        if (found) {
            lv_dropdown_set_selected(s_ui->weather_city_dd_prov, idx);
            dd_prov_changed(NULL); // 重建市+县
        }
    }

    // 市
    if (pref_code) {
        uint32_t idx = 0, found = 0;
        for (; idx < s_codes_pref.count; ++idx) if (s_codes_pref.codes[idx] == pref_code) { found = 1; break; }
        if (found) {
            lv_dropdown_set_selected(s_ui->weather_city_dd_pref, idx);
            dd_pref_changed(NULL); // 重建县
        }
    }

    // 县
    if (county_code) {
        uint32_t idx = 0;
        for (; idx < s_codes_county.count; ++idx) if (s_codes_county.codes[idx] == county_code) { lv_dropdown_set_selected(s_ui->weather_city_dd_county, idx); break; }
    }
}
// 统一为下拉与其列表项应用中文字体（避免显示方框）
static void dd_apply_font(lv_obj_t *dd)
{
    if(!dd) return;
    // 下拉本体文字
    lv_obj_set_style_text_font(dd, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    // 下拉项列表（list 是 dropdown 的内部对象，子 0 为 label）
    lv_obj_t *list = lv_dropdown_get_list(dd);
    if(list) {
        lv_obj_set_style_text_font(list, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
        if(lv_obj_get_child_cnt(list) > 0) {
            lv_obj_t *lbl = lv_obj_get_child(list, 0);
            if(lbl) lv_obj_set_style_text_font(lbl, &lv_font_cn_18, LV_PART_MAIN|LV_STATE_DEFAULT);
        }
    }
}
