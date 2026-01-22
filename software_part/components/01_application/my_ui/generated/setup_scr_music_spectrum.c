#include "lvgl.h"
#include "src/misc/lv_timer.h"
#include "widgets/canvas/lv_canvas.h"
#include "src/draw/lv_draw_buf.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "music_spectrum.h"
/* 不使用动态堆分配以减少依赖与潜在编译问题 */

/* 为稳健起见，默认使用单平面 RGB565 画布，避免直接手动写入 A8 Alpha 面导致的潜在越界。
 * 如需启用双平面 RGB565A8，请将 USE_RGB565A8 置为 1（启用前请确认 LVGL 版本的 stride/平面布局）。
 */
#ifndef USE_RGB565A8
#define USE_RGB565A8 0
#endif

/* （移动至像素写入函数之后定义） */

#define HYBRID_CANVAS_WIDTH    416
#define HYBRID_CANVAS_HEIGHT   240

#define SPECTRUM_MIN_HEIGHT    40

static lv_timer_t *s_spectrum_timer = NULL;
/* 画布缓冲放到 PSRAM，避免占用内部 DRAM。 */
#if defined(EXT_RAM_BSS_ATTR)
static EXT_RAM_BSS_ATTR uint8_t s_canvas_mem[
#if USE_RGB565A8
    LV_DRAW_BUF_SIZE(HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565A8)
#else
    LV_DRAW_BUF_SIZE(HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565)
#endif
];
#else
static EXT_RAM_ATTR uint8_t s_canvas_mem[
#if USE_RGB565A8
    LV_DRAW_BUF_SIZE(HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565A8)
#else
    LV_DRAW_BUF_SIZE(HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565)
#endif
];
#endif
static uint32_t s_frame_counter = 0;
static uint32_t s_fps_counter = 0;
static uint32_t s_last_fps = 0;
static int64_t s_fps_last_ts = 0;
#if MUSIC_SPECTRUM_UI_FPS_LOG
static const char *TAG = "MusicSpectrumUI";
#endif

/* --------- 可视化参数（与 HTML 版一致的视觉/动态逻辑） --------- */
#define WAVEFORM_MIN_GAIN           0.65f
#define WAVEFORM_MAX_GAIN           6.0f
#define WAVEFORM_TARGET_RATIO       0.72f
#define WAVEFORM_SMOOTH_MIN         0.05f
#define WAVEFORM_SMOOTH_MAX         0.18f
#define WAVEFORM_ENERGY_DECAY       0.12f

/* --------- 可视化参数（与 HTML 版保持一致） --------- */
#define WAVEFORM_MIN_GAIN           0.65f
#define WAVEFORM_MAX_GAIN           6.0f
#define WAVEFORM_TARGET_RATIO       0.72f
#define WAVEFORM_SMOOTH_MIN         0.05f
#define WAVEFORM_SMOOTH_MAX         0.18f
#define WAVEFORM_ENERGY_DECAY       0.12f

#define ENABLE_DC_BLOCK             1       /* 去直流（仅绘图数据） */
#define DRAW_GATE_DB                (-72.0f)/* 噪声门限 dBFS（仅绘图）*/
#define WAVEFORM_DRAW_GAMMA         (1.0f)  /* 1.0 关闭；<1 提升小信号 */
#define DRAW_CURVE_PATH             1       /* 曲线描边（中点二次贝塞尔近似） */
/* 是否启用波形面积填充（0=仅描边，1=描边+半透明填充） */
#ifndef WAVEFORM_ENABLE_FILL
#define WAVEFORM_ENABLE_FILL        0
#endif
#ifndef WAVEFORM_FILL_ALPHA
#define WAVEFORM_FILL_ALPHA         0.12f   /* 半透明填充的不透明度（0.0~1.0） */
#endif
#ifndef WAVEFORM_MIN_FILL_GAP
#define WAVEFORM_MIN_FILL_GAP       2       /* 小于该像素距离时不填充，避免 X 轴附近出现“浅色带” */
#endif

#define WAVE_LEFT_PAD               28      /* 与 HTML 外边距保持比例 */
#define WAVE_RIGHT_PAD              28
#define WAVE_TOP_PAD                4
#define WAVE_HEIGHT_RATIO           0.60f   /* 波形占整画布的比例（保留） */

/* --------- 频谱（上部）绘制参数，参考 tools/音乐频谱/index.html --------- */
#define SPECTRUM_TOP_RATIO          0.42f   /* 上部频谱区域占整个画布高度比例 */
#define SPECTRUM_BAR_SPACING        2       /* 条间距（像素） */
#define SPECTRUM_PAD_TOP            12      /* 波形与频谱之间留白 */
#define SPECTRUM_PAD_BOTTOM         0       /* 频谱到底部留白，0 表示贴底 */
#define SPECTRUM_USE_BLOCKS         1       /* 1=分块堆叠显示；0=整柱 */
#define SPECTRUM_BLOCK_HEIGHT       4       /* 每块高度（像素） */
#define SPECTRUM_BLOCK_GAP          1       /* 块间距（像素） */
#define SPECTRUM_BLOCK_SHADE        0.15f   /* 从下到上微弱明暗变化 0..1 */
#ifndef SPECTRUM_VISUAL_GAIN
#define SPECTRUM_VISUAL_GAIN        1.6f    /* 视觉增益（>1 更高）*/
#endif
#ifndef MUSIC_SPECTRUM_UI_FPS_LOG
#define MUSIC_SPECTRUM_UI_FPS_LOG   0       /* 关闭 fps 打印 */
#endif
#ifndef SPECTRUM_DRAW_BACKBOARD
#define SPECTRUM_DRAW_BACKBOARD     0       /* 频谱区域是否绘制背板（0=关闭，1=开启） */
#endif
#ifndef SPECTRUM_DRAW_COUNT
#define SPECTRUM_DRAW_COUNT         56      /* 固定绘制的频谱条数 */
#endif

/* 顶端指示块（peak hold）物理参数 */
#define TOPPER_GRAVITY              0.9f
#define TOPPER_MAX_VEL              18.0f
#define TOPPER_RESTITUTION_TOP      0.15f
#define TOPPER_RESTITUTION_BAR      0.25f
#define TOPPER_MIN_KICK             2.0f

/* 内部工作缓冲（重现 HTML 的 2048 点窗口与抽点算法） */
#define TD_BUFFER_LENGTH            2048

static void draw_hybrid_canvas(lv_obj_t *canvas, const music_spectrum_frame_t *frame);

/* 采样渐变（RGB） */
static inline void sample_wave_gradient_rgb(int y, int wave_top, int wave_h, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (wave_h <= 0) { *r = 0x66; *g = 0xD9; *b = 0xFF; return; }
    float t = (float)(y - wave_top) / (float)wave_h; if (t < 0) t = 0; if (t > 1) t = 1;
    float rf, gf, bf;
    if (t <= 0.5f) {
        float k = t / 0.5f; rf = 102.0f + (124.0f - 102.0f) * k; gf = 217.0f + (255.0f - 217.0f) * k; bf = 255.0f + (183.0f - 255.0f) * k;
    } else {
        float k = (t - 0.5f) / 0.5f; rf = 124.0f + (255.0f - 124.0f) * k; gf = 255.0f + (155.0f - 255.0f) * k; bf = 183.0f + (248.0f - 183.0f) * k;
    }
    *r = (uint8_t)rf; *g = (uint8_t)gf; *b = (uint8_t)bf;
}

static inline uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b)
{ return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }

#if !USE_RGB565A8
/* Brightness-scale a RGB565 color against a black background (used for "alpha" effects). */
static inline uint16_t scale_rgb565(uint16_t c565, uint8_t a)
{
    if (a >= 255) return c565;
    if (a == 0) return 0x0000;
    uint8_t r5 = (uint8_t)((c565 >> 11) & 0x1F);
    uint8_t g6 = (uint8_t)((c565 >> 5)  & 0x3F);
    uint8_t b5 = (uint8_t)( c565        & 0x1F);
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g6 << 2) | (g6 >> 4);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);
    uint32_t k = a; /* 0..255 */
    r8 = (uint8_t)((r8 * k) / 255u);
    g8 = (uint8_t)((g8 * k) / 255u);
    b8 = (uint8_t)((b8 * k) / 255u);
    return pack_rgb565(r8, g8, b8);
}
#endif

#if USE_RGB565A8
static inline void put_px_rgb565a8(uint8_t *base, int stride, int width, int height, int x, int y, uint16_t c565, uint8_t a)
{
    if((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) return;
    uint8_t *color = base + (size_t)y * (size_t)stride + (size_t)x * 2u;
    /* 注意：这里假设 Alpha 面的 stride == (color_stride/2) */
    uint8_t *alpha = base + (size_t)stride * (size_t)height + (size_t)y * ((size_t)stride / 2u) + (size_t)x;
    color[0] = (uint8_t)(c565 & 0xFF); color[1] = (uint8_t)(c565 >> 8); *alpha = a;
}
#else
static inline void put_px_rgb565(uint8_t *base, int stride, int width, int height, int x, int y, uint16_t c565, uint8_t a)
{
    if((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) return;
    uint8_t *color = base + (size_t)y * (size_t)stride + (size_t)x * 2u;
    /* 解包 RGB565 */
    uint8_t r5 = (uint8_t)((c565 >> 11) & 0x1F);
    uint8_t g6 = (uint8_t)((c565 >> 5)  & 0x3F);
    uint8_t b5 = (uint8_t)( c565        & 0x1F);
    /* 扩展到 8bit 便于缩放 */
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g6 << 2) | (g6 >> 4);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);
    /* 以 alpha 作为亮度比例（背景视为黑） */
    uint32_t k = a; /* 0..255 */
    r8 = (uint8_t)((r8 * k) / 255u);
    g8 = (uint8_t)((g8 * k) / 255u);
    b8 = (uint8_t)((b8 * k) / 255u);
    /* 重新打包为 RGB565 */
    uint16_t out = (uint16_t)(((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3));
    color[0] = (uint8_t)(out & 0xFF);
    color[1] = (uint8_t)(out >> 8);
}
#endif

static void canvas_draw_line565a(uint8_t *buf, int stride, int width, int height,
                               int x0, int y0, int x1, int y1,
                               int wave_top, int wave_h,
                               uint8_t alpha, uint8_t thickness)
{
    if (!buf) return;
    int dx = abs(x1 - x0), dy = abs(y1 - y0); int sx = (x0 < x1) ? 1 : -1; int sy = (y0 < y1) ? 1 : -1; int err = dx - dy; int radius = (thickness <= 1) ? 0 : (thickness / 2);
    while (true) {
        for (int oy = -radius; oy <= radius; ++oy) {
            int py = y0 + oy; if ((unsigned)py >= (unsigned)height) continue;
            for (int ox = -radius; ox <= radius; ++ox) {
                int px = x0 + ox; if ((unsigned)px >= (unsigned)width) continue;
                uint8_t r, g, b; sample_wave_gradient_rgb(py, wave_top, wave_h, &r, &g, &b);
#if USE_RGB565A8
                put_px_rgb565a8(buf, stride, width, height, px, py, pack_rgb565(r,g,b), alpha);
#else
                put_px_rgb565(buf, stride, width, height, px, py, pack_rgb565(r,g,b), alpha);
#endif
            }
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/* 简单 RGB565 画直线与矩形工具在上方已实现 */

/* 简易矩形填充（RGB565 或 RGB565A8） */
static inline void fill_rect565(uint8_t *buf, int stride, int width, int height,
                                int x, int y, int w, int h, uint16_t c565, uint8_t a)
{
    if (!buf || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= width || y >= height) return;
    if (x + w > width)  w = width - x;
    if (y + h > height) h = height - y;
#if USE_RGB565A8
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            put_px_rgb565a8(buf, stride, width, height, x + xx, yy, c565, a);
        }
    }
#else
    /* Pre-scale once per rectangle (huge speedup vs per-pixel scaling). */
    const uint16_t out = scale_rgb565(c565, a);
    for (int yy = y; yy < y + h; ++yy) {
        uint8_t *row = buf + (size_t)yy * (size_t)stride + (size_t)x * 2u;
        for (int xx = 0; xx < w; ++xx) {
            row[(size_t)xx * 2u + 0u] = (uint8_t)(out & 0xFF);
            row[(size_t)xx * 2u + 1u] = (uint8_t)(out >> 8);
        }
    }
#endif
}

/* 按索引生成渐变颜色（蓝->青->粉），与 HTML 渐变接近 */
static inline uint16_t bar_color_by_index(int idx, int count, float light_scale)
{
    if (count <= 1) count = 2;
    float t = (float)idx / (float)(count - 1);
    uint8_t r,g,b;
    if (t <= 0.5f) {
        float k = t / 0.5f;
        float r1 = 0x66, g1 = 0xD9, b1 = 0xFF;
        float r2 = 0x7C, g2 = 0xFF, b2 = 0xB7;
        r = (uint8_t)(r1 + (r2 - r1) * k);
        g = (uint8_t)(g1 + (g2 - g1) * k);
        b = (uint8_t)(b1 + (b2 - b1) * k);
    } else {
        float k = (t - 0.5f) / 0.5f;
        float r1 = 0x7C, g1 = 0xFF, b1 = 0xB7;
        float r2 = 0xFF, g2 = 0x9B, b2 = 0xF8;
        r = (uint8_t)(r1 + (r2 - r1) * k);
        g = (uint8_t)(g1 + (g2 - g1) * k);
        b = (uint8_t)(b1 + (b2 - b1) * k);
    }
    if (light_scale < 0) {
        light_scale = 0;
    }
    if (light_scale > 1) {
        light_scale = 1;
    }
    r = (uint8_t)((float)r * light_scale);
    g = (uint8_t)((float)g * light_scale);
    b = (uint8_t)((float)b * light_scale);
    return pack_rgb565(r,g,b);
}

static inline int16_t clamp_s16(int v) {
    if (v < INT16_MIN) return INT16_MIN;
    if (v > INT16_MAX) return INT16_MAX;
    return (int16_t)v;
}

static float s_wave_gain = 1.0f;
static float s_wave_smooth = WAVEFORM_SMOOTH_MIN;
static float s_wave_energy_avg = 0.0f;

/* 频谱顶端指示块状态 */
static float s_topper_pos[SPECTRUM_DRAW_COUNT];
static float s_topper_vel[SPECTRUM_DRAW_COUNT];
static float s_last_bar_h[SPECTRUM_DRAW_COUNT];
static void spectrum_reset_topper(void)
{
    for (int i = 0; i < SPECTRUM_DRAW_COUNT; ++i) {
        s_topper_pos[i] = 0.0f;
        s_topper_vel[i] = 0.0f;
        s_last_bar_h[i] = 0.0f;
    }
}

/* --------- HTML 等效的时域渲染管线（2048 点窗口 + 抽点 + 曲线 + 渐变描边 + 半透明填充） --------- */

/* UI 侧 2048 点环形缓冲，拼接 audio 中间件的 256 点快照 */
static float s_ring[TD_BUFFER_LENGTH * 2];
static int   s_ring_idx = 0;
static int   s_ring_count = 0;

static void ring_reset(void)
{
    memset(s_ring, 0, sizeof(s_ring));
    s_ring_idx = 0;
    s_ring_count = 0;
}

static void ring_push_block(const int16_t *src, int len)
{
    if (!src || len <= 0) return;
    const int R = (int)(sizeof(s_ring)/sizeof(s_ring[0]));
    for (int i = 0; i < len; ++i) {
        float v = (float)src[i] / 16384.0f; /* 归一化 */
        s_ring[s_ring_idx] = v;
        s_ring_idx = (s_ring_idx + 1) % R;
        if (s_ring_count < R) s_ring_count++;
    }
}

static void copy_latest_2048(float *dst)
{
    const int R = (int)(sizeof(s_ring)/sizeof(s_ring[0]));
    const int need = TD_BUFFER_LENGTH;
    if (s_ring_count <= 0) {
        memset(dst, 0, need * sizeof(float));
        return;
    }
    int filled = s_ring_count < need ? s_ring_count : need;
    int start = s_ring_idx - filled;
    while (start < 0) start += R;
    int offset = need - filled;
    if (offset > 0) memset(dst, 0, offset * sizeof(float));
    int src = start;
    for (int i = 0; i < filled; ++i) {
        dst[offset + i] = s_ring[src];
        src++; if (src >= R) src = 0;
    }
}

/* 抽点缓冲（索引与数值） */
/* 预分配足够容量（最大约 2*TD + 4） */
static uint16_t s_idx_buf[TD_BUFFER_LENGTH * 2];
static float    s_val_buf[TD_BUFFER_LENGTH * 2];
static inline void ensure_point_capacity(uint32_t need) { (void)need; /* 静态缓冲已足够 */ }

static inline float draw_gamma(float x)
{
    float ax = x < 0 ? -x : x;
    #if 1
    if (WAVEFORM_DRAW_GAMMA == 1.0f) return x;
    float mx = powf(ax, WAVEFORM_DRAW_GAMMA);
    return x < 0 ? -mx : mx;
    #else
    return x;
    #endif
}

static uint32_t reduce_waveform_points(const float *data, uint32_t length, int wave_width)
{
    if (length <= 2) return 0;
    float budget_f = (float)wave_width / 1.4f; /* 参考 HTML: waveWidth/(1.4*DPR) */
    int pixel_budget = (int)floorf(budget_f);
    if (pixel_budget < 16) pixel_budget = 16;
    if (pixel_budget >= (int)length) return 0;
    float bucket_size = (float)length / (float)pixel_budget;
    uint32_t max_points = (uint32_t)pixel_budget * 2u + 4u;
    ensure_point_capacity(max_points);
    uint32_t count = 0;
    int last_index = -1;
    for (int b = 0; b < pixel_budget; ++b) {
        int start = (int)floorf((float)b * bucket_size);
        int end   = (b == pixel_budget - 1) ? (int)length : (int)floorf(((float)(b + 1) * bucket_size));
        if (end <= start) continue;
        float minVal =  1e9f, maxVal = -1e9f;
        int   minIdx = start, maxIdx = start;
        for (int i = start; i < end; ++i) {
            float v = data[i];
            if (v < minVal) { minVal = v; minIdx = i; }
            if (v > maxVal) { maxVal = v; maxIdx = i; }
        }
        if (minIdx == maxIdx) {
            if (minIdx != last_index) {
                s_idx_buf[count] = (uint16_t)minIdx;
                s_val_buf[count] = data[minIdx];
                count++; last_index = minIdx;
            }
        } else {
            int firstIdx = (minIdx < maxIdx) ? minIdx : maxIdx;
            float firstVal = (minIdx < maxIdx) ? minVal : maxVal;
            int secondIdx = (minIdx < maxIdx) ? maxIdx : minIdx;
            float secondVal = (minIdx < maxIdx) ? maxVal : minVal;
            if (firstIdx != last_index) {
                s_idx_buf[count] = (uint16_t)firstIdx;
                s_val_buf[count] = firstVal; count++; last_index = firstIdx;
            }
            if (secondIdx != last_index) {
                s_idx_buf[count] = (uint16_t)secondIdx;
                s_val_buf[count] = secondVal; count++; last_index = secondIdx;
            }
        }
    }
    if (count == 0 || s_idx_buf[0] != 0) {
        ensure_point_capacity(count + 1);
        for (int i = (int)count; i > 0; --i) {
            s_idx_buf[i] = s_idx_buf[i - 1];
            s_val_buf[i] = s_val_buf[i - 1];
        }
        s_idx_buf[0] = 0; s_val_buf[0] = data[0]; count++;
    }
    if (s_idx_buf[count - 1] != length - 1) {
        ensure_point_capacity(count + 1);
        s_idx_buf[count] = (uint16_t)(length - 1);
        s_val_buf[count] = data[length - 1]; count++;
    }
    return count;
}

static void draw_hybrid_canvas(lv_obj_t *canvas, const music_spectrum_frame_t *frame)
{
    if (!canvas) {
        return;
    }
    uint8_t *buf = (uint8_t *)lv_canvas_get_buf(canvas);
    if (!buf) {
        return;
    }
    /* 背景 */
    
    const int width = HYBRID_CANVAS_WIDTH;
    const int height = HYBRID_CANVAS_HEIGHT;
    /* 取 stride，用于 RGB565A8 双面布局 */
    lv_image_dsc_t *img = lv_canvas_get_image(canvas);
    int stride = img ? (int)img->header.stride : (width * 2); /* Fallback */
    /* 背景全黑：用 memset 清空比逐像素写入快很多 */
#if USE_RGB565A8
    memset(buf, 0, (size_t)stride * (size_t)height);
    /* 注意：这里沿用上方 put_px_rgb565a8 的假设：alpha_stride == (color_stride/2) */
    memset(buf + (size_t)stride * (size_t)height, 0xFF,
           ((size_t)stride / 2u) * (size_t)height);
#else
    memset(buf, 0, (size_t)stride * (size_t)height);
#endif

    /* 开始在画布上部绘制时域波形 */

    if (!frame) {
        lv_obj_invalidate(canvas);
        return;
    }

    /* （下面继续原来的时域波形绘制逻辑） */

    /* 时域波形区域计算（与 HTML 版一致：顶部少许内边距 + 60% 高度） */
    const int wave_left = WAVE_LEFT_PAD;
    const int wave_right = width - WAVE_RIGHT_PAD;
    const int wave_width = LV_MAX(1, wave_right - wave_left);
    const int wave_top = WAVE_TOP_PAD;
    const int wave_h = (int)(height * WAVE_HEIGHT_RATIO) - (WAVE_TOP_PAD * 2);
    const int wave_mid = wave_top + (wave_h / 2);

    /* 构造 2048 点窗口，按 HTML 一致的环形缓冲取得 */
    ring_push_block(frame->waveform, MUSIC_SPECTRUM_WAVE_SAMPLES);
    static float td_data[TD_BUFFER_LENGTH];
    copy_latest_2048(td_data);

    /* 绘图前处理：去 DC + 简易门限（仅绘图层） */
    #if ENABLE_DC_BLOCK
    {
        double sum = 0.0;
        for (int i = 0; i < TD_BUFFER_LENGTH; ++i) sum += td_data[i];
        float mean = (float)(sum / (double)TD_BUFFER_LENGTH);
        for (int i = 0; i < TD_BUFFER_LENGTH; ++i) td_data[i] -= mean;
    }
    #endif
    /* 噪声门阈值（常量开启/关闭），与 HTML 的 Number.isFinite(DRAW_GATE_DB) 类似 */
    {
        static float s_gate_amp = -1.0f;
        if (s_gate_amp < 0.0f) {
            s_gate_amp = powf(10.0f, DRAW_GATE_DB / 20.0f);
        }
        const float gate_amp = s_gate_amp;
        for (int i = 0; i < TD_BUFFER_LENGTH; ++i) {
            float v = td_data[i];
            if (v < gate_amp && v > -gate_amp) td_data[i] = 0.0f;
        }
    }

    /* 峰值与 RMS（基于 2048 点） */
    float peak = 0.0f, sumsq = 0.0f;
    for (int i = 0; i < TD_BUFFER_LENGTH; ++i) {
        float x = td_data[i];
        float a = x < 0 ? -x : x;
        if (a > peak) {
            peak = a;
        }
        sumsq += x * x;
    }
    float rms = sqrtf(sumsq / (float)TD_BUFFER_LENGTH);
    float amp_ref = LV_MAX(peak * 0.6f, rms * 1.41421356f); /* 与 HTML 版相同思想 */

    /* 自适应平滑参数（能量越高，响应越快） */
    float energy_norm = LV_MIN(1.0f, rms * 2.2f);
    s_wave_energy_avg += (energy_norm - s_wave_energy_avg) * WAVEFORM_ENERGY_DECAY;
    float smooth_target = WAVEFORM_SMOOTH_MIN + (WAVEFORM_SMOOTH_MAX - WAVEFORM_SMOOTH_MIN) * s_wave_energy_avg;
    s_wave_smooth += (smooth_target - s_wave_smooth) * 0.28f;
    if (s_wave_smooth < WAVEFORM_SMOOTH_MIN) s_wave_smooth = WAVEFORM_SMOOTH_MIN;
    if (s_wave_smooth > WAVEFORM_SMOOTH_MAX) s_wave_smooth = WAVEFORM_SMOOTH_MAX;

    /* 目标增益：希望主振幅 ~ 0.72 * wave_h */
    float target_gain = WAVEFORM_MIN_GAIN;
    if (amp_ref > 1e-6f) {
        float g = WAVEFORM_TARGET_RATIO / amp_ref;
        if (g < WAVEFORM_MIN_GAIN) g = WAVEFORM_MIN_GAIN;
        if (g > WAVEFORM_MAX_GAIN) g = WAVEFORM_MAX_GAIN;
        target_gain = g;
    }
    s_wave_gain += (target_gain - s_wave_gain) * s_wave_smooth;

    /* 抽点：对 2048 点进行“保峰值”下采样，得到最多 ~2*pixel_budget 个关键点 */
    uint32_t reduced = reduce_waveform_points(td_data, TD_BUFFER_LENGTH, wave_width);

    /* 将关键点转为坐标数组，并做非线性映射（如开启） */
    if (reduced > 0) {
        /* 填充区域：按曲线 y(x) 扫描，记录每列的 y 值，再向中线填充半透明渐变颜色 */
        static int16_t y_by_x[HYBRID_CANVAS_WIDTH];
        for (int i = 0; i < wave_width; ++i) y_by_x[i] = INT16_MIN;

        /* 生成曲线近似的高密度折线并写入 y_by_x */
        int idx0 = s_idx_buf[0];
        float val0 = draw_gamma(s_val_buf[0]);
        int prev_x = wave_left + (int)lrintf(((float)idx0 / (float)(TD_BUFFER_LENGTH - 1)) * (float)(wave_width - 1));
        int prev_y = wave_mid - (int)lrintf(val0 * (float)(wave_h/2) * s_wave_gain);

        #define CURVE_STEPS 6
        /* 初始点 */
        for (uint32_t i = 1; i + 1 < reduced; ++i) {
            /* 控制点为当前关键点，终点是与下一关键点的中点 */
            int idx_c = s_idx_buf[i];
            float val_c = draw_gamma(s_val_buf[i]);
            int x_c = wave_left + (int)lrintf(((float)idx_c / (float)(TD_BUFFER_LENGTH - 1)) * (float)(wave_width - 1));
            int y_c = wave_mid - (int)lrintf(val_c * (float)(wave_h/2) * s_wave_gain);

            int idx_n = s_idx_buf[i + 1];
            float val_n = draw_gamma(s_val_buf[i + 1]);
            int x_n = wave_left + (int)lrintf(((float)idx_n / (float)(TD_BUFFER_LENGTH - 1)) * (float)(wave_width - 1));
            int y_n = wave_mid - (int)lrintf(val_n * (float)(wave_h/2) * s_wave_gain);

            int x_e = (x_c + x_n) / 2;
            int y_e = (y_c + y_n) / 2;

            /* 采样 quadraticCurve(prev -> (x_c,y_c) -> (x_e,y_e)) */
            for (int s = 1; s <= CURVE_STEPS; ++s) {
                float t = (float)s / (float)CURVE_STEPS;
                float omt = 1.0f - t;
                float xf = omt*omt*prev_x + 2*omt*t*x_c + t*t*x_e;
                float yf = omt*omt*prev_y + 2*omt*t*y_c + t*t*y_e;
                int xi = (int)lrintf(xf);
                int yi = (int)lrintf(yf);
                if (xi < wave_left) xi = wave_left;
                if (xi >= wave_right) xi = wave_right - 1;
                y_by_x[xi - wave_left] = yi;
            }
            prev_x = x_e; prev_y = y_e;
        }
        /* 最后一段：控制点=最后一个关键点，终点=最后一个关键点坐标 */
        {
            int idx_c = s_idx_buf[reduced - 1];
            float val_c = draw_gamma(s_val_buf[reduced - 1]);
            int x_c = wave_left + (int)lrintf(((float)idx_c / (float)(TD_BUFFER_LENGTH - 1)) * (float)(wave_width - 1));
            int y_c = wave_mid - (int)lrintf(val_c * (float)(wave_h/2) * s_wave_gain);
            int x_e = x_c, y_e = y_c;
            for (int s = 1; s <= CURVE_STEPS; ++s) {
                float t = (float)s / (float)CURVE_STEPS;
                float omt = 1.0f - t;
                float xf = omt*omt*prev_x + 2*omt*t*x_c + t*t*x_e;
                float yf = omt*omt*prev_y + 2*omt*t*y_c + t*t*y_e;
                int xi = (int)lrintf(xf);
                int yi = (int)lrintf(yf);
                if (xi < wave_left) xi = wave_left;
                if (xi >= wave_right) xi = wave_right - 1;
                y_by_x[xi - wave_left] = yi;
            }
        }

        /* 将未覆盖的列用最近一次已覆盖的 y 插值，避免稀疏采样造成竖向缝隙 */
        {
            int last_y = INT16_MIN;
            for (int col = 0; col < wave_width; ++col) {
                if (y_by_x[col] == INT16_MIN) {
                    y_by_x[col] = last_y;
                } else {
                    last_y = y_by_x[col];
                }
            }
            /* 反向再扫一遍，填补前缀的空洞 */
            last_y = INT16_MIN;
            for (int col = wave_width - 1; col >= 0; --col) {
                if (y_by_x[col] == INT16_MIN) {
                    y_by_x[col] = last_y;
                } else {
                    last_y = y_by_x[col];
                }
            }
        }

        /* 半透明填充可选：避免 X 轴附近出现浅色带，默认关闭 */
#if WAVEFORM_ENABLE_FILL
        {
            uint8_t fill_alpha = (uint8_t)(WAVEFORM_FILL_ALPHA * 255.0f);
            for (int col = 0; col < wave_width; ++col) {
                int x = wave_left + col;
                int y = y_by_x[col];
                if (y == INT16_MIN) continue;
                int gap = y - wave_mid; if (gap < 0) gap = -gap;
                if (gap < WAVEFORM_MIN_FILL_GAP) continue; /* 距离中线太近时不填充，消除浅色带 */
                if (y <= wave_mid) {
                    int y0 = y; if (y0 < 0) y0 = 0; if (y0 > height - 1) y0 = height - 1;
                    int y1 = wave_mid - 1; if (y1 < 0) y1 = 0; if (y1 > height - 1) y1 = height - 1;
                    for (int yy = y0; yy <= y1; ++yy) {
                        uint8_t r,g,b; sample_wave_gradient_rgb(yy, wave_top, wave_h, &r,&g,&b);
#if USE_RGB565A8
                        put_px_rgb565a8(buf, stride, width, height, x, yy, pack_rgb565(r,g,b), fill_alpha);
#else
                        put_px_rgb565(buf, stride, width, height, x, yy, pack_rgb565(r,g,b), fill_alpha);
#endif
                    }
                } else {
                    int y0 = wave_mid + 1; if (y0 < 0) y0 = 0; if (y0 > height - 1) y0 = height - 1;
                    int y1 = y; if (y1 < 0) y1 = 0; if (y1 > height - 1) y1 = height - 1;
                    for (int yy = y0; yy <= y1; ++yy) {
                        uint8_t r,g,b; sample_wave_gradient_rgb(yy, wave_top, wave_h, &r,&g,&b);
#if USE_RGB565A8
                        put_px_rgb565a8(buf, stride, width, height, x, yy, pack_rgb565(r,g,b), fill_alpha);
#else
                        put_px_rgb565(buf, stride, width, height, x, yy, pack_rgb565(r,g,b), fill_alpha);
#endif
                    }
                }
            }
        }
#endif

        /* 渐变描边：用实线连接 y_by_x 的相邻列像素 */
        uint8_t stroke_alpha = 255; /* 近似 HTML 0.95 的不透明度 */
        int prev_set = 0, px = 0, py = 0;
        for (int col = 0; col < wave_width; ++col) {
            int x = wave_left + col;
            int y = y_by_x[col];
            if (y == INT16_MIN) continue;
            if (!prev_set) { px = x; py = y; prev_set = 1; continue; }
            canvas_draw_line565a(buf, stride, width, height, px, py, x, y, wave_top, wave_h, stroke_alpha, 2);
            px = x; py = y;
        }
    }

    /* 在下部绘制频谱柱（基于 frame->bars），与 HTML 版一致的视觉演化 */
    {
        const int spec_left = wave_left;
        const int spec_width = wave_width;
        int spec_top = wave_top + wave_h + SPECTRUM_PAD_TOP;
        int spec_h = height - spec_top - SPECTRUM_PAD_BOTTOM;
        if (spec_h < SPECTRUM_MIN_HEIGHT) spec_h = SPECTRUM_MIN_HEIGHT;
        const int spec_base_y = spec_top + spec_h;

        if (spec_h > 0) {
#if SPECTRUM_DRAW_BACKBOARD
            /* 背板微暗处理（如需关闭，置 SPECTRUM_DRAW_BACKBOARD=0） */
            {
                uint16_t bgc = pack_rgb565(10,14,28);
                fill_rect565(buf, stride, width, height,
                             spec_left - 12, spec_top - 8,
                             spec_width + 24, spec_h + 16,
                             bgc, 220);
            }
#endif

            /* 固定为 46 条（按需求），将 32 源条线性插值到 46 条 */
            const int src_count = MUSIC_SPECTRUM_POINT_COUNT;
            const int draw_count = SPECTRUM_DRAW_COUNT;
            const int spacing = SPECTRUM_BAR_SPACING;
            int bar_w = (spec_width - spacing * (draw_count - 1)) / draw_count;
            if (bar_w < 1) bar_w = 1;
            int leftover = spec_width - (bar_w * draw_count + spacing * (draw_count - 1));
            if (leftover < 0) leftover = 0;
            int x = spec_left; /* 逐条增量推进，均匀把剩余像素分配到前面的若干个间隙 */

            /* Hot path optimization: avoid powf() per bar per frame. */
            static bool s_eased_lut_ready = false;
            static float s_eased_lut[121];
            if (!s_eased_lut_ready) {
                for (int ii = 0; ii <= 120; ++ii) {
                    float norm = (float)ii / 120.0f;
                    float eased = powf(norm, 0.78f) * SPECTRUM_VISUAL_GAIN;
                    if (eased > 1.0f) eased = 1.0f;
                    s_eased_lut[ii] = eased;
                }
                s_eased_lut_ready = true;
            }

            for (int i = 0; i < draw_count; ++i) {
                /* 线性插值将 32 源条映射到 draw_count 显示条 */
                float pos = (draw_count <= 1) ? 0.0f : ((float)i * (float)(src_count - 1) / (float)(draw_count - 1));
                int i0 = (int)pos;
                int i1 = i0 + 1; if (i1 >= src_count) i1 = src_count - 1;
                float t = pos - (float)i0;
                float v0 = (float)frame->bars[i0];
                float v1 = (float)frame->bars[i1];
                int16_t v = (int16_t)lrintf(v0 * (1.0f - t) + v1 * t);
                if (v < 0) {
                    v = 0;
                }
                if (v > 120) {
                    v = 120;
                }
                float eased = s_eased_lut[(int)v];
                int bar_h = (int)lrintf(eased * (float)spec_h);
                int cur_w = bar_w; /* 所有柱宽一致 */

                float light = 0.78f + eased * 0.18f;
                if (light > 1.0f) light = 1.0f;
                uint16_t col = bar_color_by_index(i, draw_count, light);

                int visible_blocks_for_topper = 0;
                if (SPECTRUM_USE_BLOCKS) {
                    int bh = SPECTRUM_BLOCK_HEIGHT;
                    int bg = SPECTRUM_BLOCK_GAP;
                    int step = bh + bg; if (step <= 0) step = 1;
                    int blocks = (bar_h + bg) / step;
                    if (blocks <= 0) {
                        /* 无论是否暂停/静音，保证底部至少一块可见 */
                        blocks = 1;
                    }
                    visible_blocks_for_topper = blocks;
                    for (int k = 0; k < blocks; ++k) {
                        int by = spec_base_y - (k + 1) * bh - k * bg;
                        float shade = SPECTRUM_BLOCK_SHADE * ((blocks <= 1) ? 0.0f : ((float)k / (float)(blocks - 1) - 0.5f));
                        float ls = light + shade; if (ls < 0.0f) ls = 0.0f; if (ls > 1.0f) ls = 1.0f;
                        uint16_t cc = bar_color_by_index(i, draw_count, ls);
                        fill_rect565(buf, stride, width, height, x, by, cur_w, bh, cc, 255);
                    }
                } else {
                    int y = spec_base_y - bar_h;
                    fill_rect565(buf, stride, width, height, x, y, cur_w, bar_h, col, 255);
                    visible_blocks_for_topper = (bar_h > 0) ? 1 : 0;
                }

                /* 顶端指示块（peak hold） */
                float last_h = s_last_bar_h[i];
                float delta = (float)bar_h - last_h;
                if (delta > 0 && bar_h >= s_topper_pos[i] - 0.5f) {
                    float impulse = TOPPER_MIN_KICK + delta * 0.22f;
                    if (impulse > s_topper_vel[i]) s_topper_vel[i] = impulse;
                }
                s_topper_vel[i] -= TOPPER_GRAVITY;
                if (s_topper_vel[i] > TOPPER_MAX_VEL) s_topper_vel[i] = TOPPER_MAX_VEL;
                if (s_topper_vel[i] < -TOPPER_MAX_VEL) s_topper_vel[i] = -TOPPER_MAX_VEL;
                s_topper_pos[i] += s_topper_vel[i];
                float pos_max = (float)spec_h;
                if (s_topper_pos[i] > pos_max) { s_topper_pos[i] = pos_max; s_topper_vel[i] = -fabsf(s_topper_vel[i]) * TOPPER_RESTITUTION_TOP; }
                if (s_topper_pos[i] < (float)bar_h) {
                    s_topper_pos[i] = (float)bar_h;
                    if (s_topper_vel[i] < 0) s_topper_vel[i] = fabsf(s_topper_vel[i]) * TOPPER_RESTITUTION_BAR + TOPPER_MIN_KICK;
                    else if (s_topper_vel[i] < TOPPER_MIN_KICK) s_topper_vel[i] = TOPPER_MIN_KICK;
                }
                s_last_bar_h[i] = (float)bar_h;
                /* 恢复顶部矩形块的跳动：当可见块>=1 时绘制顶端指示块（暂停时也有底部一排，故不会出现“全是点”） */
                if (visible_blocks_for_topper > 0) {
                    int tp = (int)lrintf(s_topper_pos[i]);
                    /* 对齐到块栅格，使顶端块与条块对齐更像 HTML */
                    int bh = SPECTRUM_BLOCK_HEIGHT;
                    int bg = SPECTRUM_BLOCK_GAP;
                    int step = bh + bg; if (step <= 0) step = 1;
                    tp = (tp + bg) / step * step; /* grid snap */
                    int tsize = LV_MAX(2, (int)lrintf(cur_w * 0.65f));
                    int ty = spec_base_y - tp - tsize;
                    uint16_t tc = bar_color_by_index(i, draw_count, LV_MIN(1.0f, light + 0.12f));
                    fill_rect565(buf, stride, width, height, x + (cur_w - tsize)/2, ty, tsize, tsize, tc, 230);
                }

                /* 前 leftover 个间隙增加 1px 间距，保证最后一条也到右边界，且不拉伸任何柱宽 */
                if (i < draw_count - 1) {
                    int extra = (i < leftover) ? 1 : 0;
                    x += cur_w + spacing + extra;
                }
            }
        }
    }

    lv_obj_invalidate(canvas);
}

static void spectrum_timer_cb(lv_timer_t *timer)
{
    if (!timer) {
        return;
    }
    lv_ui *ui = (lv_ui *)lv_timer_get_user_data(timer);
    /* 若画布已被删除（在 LV_EVENT_DELETE 中会把指针置空），让当前 timer 自行删除（避免二次删除） */
    if (!ui || ui->MusicSpectrum_chart_line == NULL) {
        lv_timer_del(timer);
        s_spectrum_timer = NULL;
        music_spectrum_set_active(false);
        ring_reset();
        spectrum_reset_topper();
        return;
    }

    music_spectrum_frame_t frame;
    bool has_new = music_spectrum_snapshot(&frame);
#if MUSIC_SPECTRUM_UI_FPS_LOG
    uint32_t frame_id = s_frame_counter++;
#else
    s_frame_counter++;
#endif
    int64_t now_us = esp_timer_get_time();
    if (s_fps_last_ts == 0) {
        s_fps_last_ts = now_us;
    }
    s_fps_counter++;
    int64_t elapsed = now_us - s_fps_last_ts;
    if (elapsed >= 1000000) {
        s_last_fps = (uint32_t)((s_fps_counter * 1000000LL) / elapsed);
#if MUSIC_SPECTRUM_UI_FPS_LOG
        ESP_LOGI(TAG, "fps=%" PRIu32 " frame=%lu new=%d",
                 s_last_fps,
                 (unsigned long)frame_id,
                 has_new ? 1 : 0);
#endif
        s_fps_counter = 0;
        s_fps_last_ts = now_us;
    }

    if (has_new) {
        draw_hybrid_canvas(ui->MusicSpectrum_chart_line, &frame);
    }
}

static void spectrum_delete_cb(lv_event_t *e)
{
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    /* 不在这里删除定时器，避免与其它路径产生二次删除；交给 timer 自删 */
    music_spectrum_set_active(false);
    ring_reset();
    spectrum_reset_topper();
    /* 清空 UI 指针，避免后续定时器使用悬空指针 */
    if (ui) {
        ui->MusicSpectrum_chart_line = NULL;
        ui->MusicSpectrum_panel = NULL;
        ui->MusicSpectrum_root = NULL;
        ui->MusicSpectrum = NULL;
        ui->MusicSpectrum_del = true;
    }
}

/* 画布对象的删除回调：优先于父屏幕触发，确保第一时间撤销定时器并清空指针 */
static void canvas_delete_cb(lv_event_t *e)
{
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
    /* 不在这里删除定时器，避免与其它路径产生二次删除；交给 timer 自删 */
    music_spectrum_set_active(false);
    ring_reset();
    spectrum_reset_topper();
    if (ui) {
        ui->MusicSpectrum_chart_line = NULL;
    }
}

/* 提供给外部在关闭动画前主动停止定时器，避免在删除序列中插入一帧访问 */
void music_spectrum_ui_stop(void)
{
    /* 不直接删除定时器，避免与删除回调/自删路径冲突；定时器在下一次回调中自删 */
    music_spectrum_set_active(false);
    ring_reset();
    spectrum_reset_topper();
}

void setup_scr_music_spectrum(lv_ui *ui)
{
    ESP_LOGI("MusicSpectrumSetup", "reverted canvas layout active");

    ui->MusicSpectrum = lv_obj_create(NULL);
    lv_obj_set_size(ui->MusicSpectrum, 480, 320);
    lv_obj_set_scrollbar_mode(ui->MusicSpectrum, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->MusicSpectrum, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->MusicSpectrum, lv_color_hex(0x050915), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->MusicSpectrum, lv_color_hex(0x111c39), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->MusicSpectrum, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->MusicSpectrum, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui->MusicSpectrum_root = lv_obj_create(ui->MusicSpectrum);
    lv_obj_set_size(ui->MusicSpectrum_root, 480, 320);
    lv_obj_center(ui->MusicSpectrum_root);
    lv_obj_set_scrollbar_mode(ui->MusicSpectrum_root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->MusicSpectrum_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui->MusicSpectrum_root, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicSpectrum_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->MusicSpectrum_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui->MusicSpectrum_panel = lv_obj_create(ui->MusicSpectrum_root);
    lv_obj_set_size(ui->MusicSpectrum_panel, 456, 284);
    lv_obj_center(ui->MusicSpectrum_panel);
    lv_obj_set_scrollbar_mode(ui->MusicSpectrum_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->MusicSpectrum_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->MusicSpectrum_panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->MusicSpectrum_panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicSpectrum_panel, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicSpectrum_panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->MusicSpectrum_panel, lv_color_hex(0x1f3a66), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->MusicSpectrum_panel, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui->MusicSpectrum_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui->MusicSpectrum_panel, 45, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui->MusicSpectrum_panel, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui->MusicSpectrum_chart_line = lv_canvas_create(ui->MusicSpectrum_panel);
    memset(s_canvas_mem, 0, sizeof(s_canvas_mem));
    lv_canvas_set_buffer(ui->MusicSpectrum_chart_line, s_canvas_mem,
                         HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT,
#if USE_RGB565A8
                         LV_COLOR_FORMAT_RGB565A8
#else
                         LV_COLOR_FORMAT_RGB565
#endif
                         );
    lv_obj_set_size(ui->MusicSpectrum_chart_line, HYBRID_CANVAS_WIDTH, HYBRID_CANVAS_HEIGHT);
    lv_obj_center(ui->MusicSpectrum_chart_line);
    lv_obj_set_scrollbar_mode(ui->MusicSpectrum_chart_line, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->MusicSpectrum_chart_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->MusicSpectrum_chart_line, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->MusicSpectrum_chart_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->MusicSpectrum_chart_line, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->MusicSpectrum_chart_line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ring_reset();
    spectrum_reset_topper();
    music_spectrum_frame_t blank_frame = {0};
    draw_hybrid_canvas(ui->MusicSpectrum_chart_line, &blank_frame);

    music_spectrum_set_active(true);
    /* 避免在打开页面时对一个可能已被 LVGL 回收的旧指针调用 lv_timer_del，
     * 这里不再尝试删除旧定时器（关闭页面时已主动删除且置 NULL）。
     * 直接创建新定时器并记录指针。 */
    s_spectrum_timer = lv_timer_create(spectrum_timer_cb, 40, ui);

    events_init_MusicSpectrum(ui);
    /* 画布删除优先：先绑定画布的删除事件 */
    lv_obj_add_event_cb(ui->MusicSpectrum_chart_line, canvas_delete_cb, LV_EVENT_DELETE, ui);
    /* 屏幕删除事件：兜底清理 */
    lv_obj_add_event_cb(ui->MusicSpectrum, spectrum_delete_cb, LV_EVENT_DELETE, ui);

    ui->MusicSpectrum_del = false;
}
