#include <stdio.h>
#include <string.h>
#include <strings.h>  /* strcasecmp */
#include <inttypes.h> /* PRId64 */

#include "audio_common.h"
#include "audio_element.h"
#include "audio_mem.h"
#include "esp_log.h"
#include "http_stream.h"  /* 复用 HTTP 源的任务栈配置 */

static const char *TAG = "file_src";

/* WAV 文件头最大缓存大小（标准 WAV 头约 44 字节，但可能有扩展块） */
#define WAV_HEADER_MAX_SIZE 256

/* 专用 seek 目标变量：绕过 audio_element_set_byte_pos 的时序问题
 * -1 表示无 seek 请求，>=0 表示目标字节偏移 */
static volatile int64_t s_pending_seek_byte = -1;

typedef struct {
    FILE *fp;
    /* WAV seek 支持：缓存头部数据，seek 后先返回头部再读 PCM */
    uint8_t wav_header[WAV_HEADER_MAX_SIZE];
    int     wav_header_size;       /* 实际头部大小（到 data chunk 开始） */
    int     wav_header_sent;       /* 已发送的头部字节数 */
    int64_t wav_seek_target;       /* PCM 数据中的目标字节偏移（相对于 data 起始） */
    bool    wav_seek_pending;      /* 是否有待执行的 seek */
} file_src_t;

/* 检查文件扩展名是否为 WAV */
static bool is_wav_file(const char *uri)
{
    if (!uri) return false;
    const char *ext = strrchr(uri, '.');
    if (!ext) return false;
    return (strcasecmp(ext, ".wav") == 0);
}

/* 设置下一次 open 时的 seek 目标字节位置
 * byte_pos >= 0 表示 seek 到指定位置，-1 表示从头开始 */
void file_src_set_seek_position(int64_t byte_pos)
{
    s_pending_seek_byte = byte_pos;
    ESP_LOGW(TAG, "[DEBUG] set_seek_position: %" PRId64, byte_pos);
}

/* 解析 WAV 头，返回 data chunk 的起始位置（相对于文件开头）
 * 成功返回 > 0，失败返回 -1 */
static int parse_wav_header(FILE *fp, uint8_t *header_buf, int *header_size)
{
    /* 读取 RIFF 头（12 字节） */
    if (fread(header_buf, 1, 12, fp) != 12) {
        return -1;
    }
    /* 验证 RIFF/WAVE 标识 */
    if (memcmp(header_buf, "RIFF", 4) != 0 ||
        memcmp(header_buf + 8, "WAVE", 4) != 0) {
        return -1;
    }

    int pos = 12;
    /* 遍历 chunk 找到 data chunk */
    while (pos < WAV_HEADER_MAX_SIZE - 8) {
        uint8_t chunk_hdr[8];
        if (fread(chunk_hdr, 1, 8, fp) != 8) {
            return -1;
        }
        memcpy(header_buf + pos, chunk_hdr, 8);
        pos += 8;

        uint32_t chunk_size = chunk_hdr[4] | (chunk_hdr[5] << 8) |
                              (chunk_hdr[6] << 16) | (chunk_hdr[7] << 24);

        /* 找到 data chunk */
        if (memcmp(chunk_hdr, "data", 4) == 0) {
            *header_size = pos;  /* 头部到 data chunk 数据起始 */
            return pos;          /* data chunk 数据在文件中的起始位置 */
        }

        /* 非 data chunk：读取 chunk 内容到头部缓冲（如 fmt chunk） */
        if (pos + (int)chunk_size > WAV_HEADER_MAX_SIZE) {
            /* 头部太大，截断处理 */
            size_t to_read = WAV_HEADER_MAX_SIZE - pos;
            if (fread(header_buf + pos, 1, to_read, fp) != to_read) {
                return -1;
            }
            /* 跳过剩余部分 */
            fseek(fp, (long)(chunk_size - to_read), SEEK_CUR);
            pos = WAV_HEADER_MAX_SIZE;
        } else {
            if (fread(header_buf + pos, 1, chunk_size, fp) != chunk_size) {
                return -1;
            }
            pos += (int)chunk_size;
        }
    }
    return -1;  /* 未找到 data chunk */
}

static esp_err_t _file_open(audio_element_handle_t self)
{
    ESP_LOGW(TAG, "[DEBUG] _file_open called, pending_seek=%" PRId64, s_pending_seek_byte);

    file_src_t *src = (file_src_t *)audio_element_getdata(self);
    const char *uri = audio_element_get_uri(self);
    if (!uri || !uri[0]) {
        ESP_LOGE(TAG, "open: empty uri");
        return ESP_FAIL;
    }

    if (src->fp) {
        fclose(src->fp);
        src->fp = NULL;
    }

    /* 重置 WAV seek 状态 */
    src->wav_header_size = 0;
    src->wav_header_sent = 0;
    src->wav_seek_target = 0;
    src->wav_seek_pending = false;

    src->fp = fopen(uri, "rb");
    if (!src->fp) {
        ESP_LOGE(TAG, "open: fopen(%s) failed", uri);
        return ESP_FAIL;
    }

    /* 获取 seek 目标：优先使用专用变量，其次使用 element info */
    int64_t requested_pos = s_pending_seek_byte;
    if (requested_pos < 0) {
        audio_element_info_t info_tmp;
        audio_element_getinfo(self, &info_tmp);
        requested_pos = info_tmp.byte_pos;
    }
    s_pending_seek_byte = -1;  /* 消费掉 seek 请求 */

    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    ESP_LOGI(TAG, "open: uri=%s, byte_pos=%" PRId64 ", is_wav=%d",
             uri, requested_pos, (int)is_wav_file(uri));

    /* 计算总字节数 */
    if (fseek(src->fp, 0L, SEEK_END) == 0) {
        long end = ftell(src->fp);
        if (end > 0) {
            info.total_bytes = (int64_t)end;
        }
        (void)fseek(src->fp, 0L, SEEK_SET);
    }

    /* WAV 文件 seek 特殊处理 */
    if (is_wav_file(uri) && requested_pos > 0) {
        int header_size = 0;
        int data_start = parse_wav_header(src->fp, src->wav_header, &header_size);

        if (data_start > 0 && requested_pos >= data_start) {
            /* 有效的 WAV seek：保存头部，准备两阶段读取 */
            src->wav_header_size = header_size;
            src->wav_header_sent = 0;
            src->wav_seek_target = requested_pos;  /* 文件中的绝对位置 */
            src->wav_seek_pending = true;

            /* 文件指针保持在 data_start 位置，等 read 时再 seek */
            fseek(src->fp, data_start, SEEK_SET);

            info.byte_pos = 0;  /* 对外报告从 0 开始（头部会重新发送） */
            audio_element_setinfo(self, &info);

            ESP_LOGI(TAG, "WAV seek: header=%d, target=%" PRId64 ", total=%" PRId64,
                     header_size, requested_pos, info.total_bytes);
            return ESP_OK;
        } else {
            /* 解析失败或目标位置无效，从头开始 */
            ESP_LOGW(TAG, "WAV seek failed: data_start=%d, pos=%" PRId64 ", fallback to start",
                     data_start, requested_pos);
            fseek(src->fp, 0L, SEEK_SET);
        }
    } else if (requested_pos > 0) {
        /* 非 WAV 文件：直接 seek 到目标位置 */
        fseek(src->fp, (long)requested_pos, SEEK_SET);
        info.byte_pos = requested_pos;
    } else {
        info.byte_pos = 0;
    }

    audio_element_setinfo(self, &info);
    ESP_LOGI(TAG, "open file: %s, pos=%" PRId64 ", total=%" PRId64,
             uri, info.byte_pos, info.total_bytes);
    return ESP_OK;
}

static int _file_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    (void)ticks_to_wait;
    (void)context;

    file_src_t *src = (file_src_t *)audio_element_getdata(self);
    if (!src || !src->fp) {
        return AEL_IO_FAIL;
    }

    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    /* WAV seek 两阶段读取：先返回缓存的头部 */
    if (src->wav_header_size > 0 && src->wav_header_sent < src->wav_header_size) {
        int remaining = src->wav_header_size - src->wav_header_sent;
        int to_copy = (len < remaining) ? len : remaining;
        memcpy(buffer, src->wav_header + src->wav_header_sent, to_copy);
        src->wav_header_sent += to_copy;

        info.byte_pos += to_copy;
        audio_element_setinfo(self, &info);

        /* 头部全部发送完毕后，执行 seek 到目标位置 */
        if (src->wav_header_sent >= src->wav_header_size && src->wav_seek_pending) {
            ESP_LOGI(TAG, "WAV header sent, seeking to %" PRId64, src->wav_seek_target);
            fseek(src->fp, (long)src->wav_seek_target, SEEK_SET);
            src->wav_seek_pending = false;
            /* 更新 byte_pos 为实际的 seek 位置 */
            info.byte_pos = src->wav_seek_target;
            audio_element_setinfo(self, &info);
        }

        return to_copy;
    }

    /* 正常文件读取 */
    size_t r = fread(buffer, 1, (size_t)len, src->fp);
    if (r > 0) {
        info.byte_pos += (int64_t)r;
        audio_element_setinfo(self, &info);
        return (int)r;
    }

    if (feof(src->fp)) {
        ESP_LOGI(TAG, "EOF reached, pos=%" PRId64, info.byte_pos);
        audio_element_report_status(self, AEL_STATUS_INPUT_DONE);
        return AEL_IO_DONE;
    }

    ESP_LOGE(TAG, "read error on file source");
    return AEL_IO_FAIL;
}

/* 简单的“源”处理函数：
 * - 通过 audio_element_input() 调用上面的 _file_read 从磁盘读数据；
 * - 再通过 audio_element_output() 写入下游解码器的输入 ringbuffer。*/
static int _file_process(audio_element_handle_t self, char *buffer, int len)
{
    int r_size = audio_element_input(self, buffer, len);
    if (audio_element_is_stopping(self)) {
        ESP_LOGW(TAG, "No output due to stopping");
        return AEL_IO_ABORT;
    }

    if (r_size > 0) {
        int w_size = audio_element_output(self, buffer, r_size);
        audio_element_multi_output(self, buffer, r_size, 0);
        return w_size;
    }

    /* 直接把 AEL_IO_DONE / AEL_IO_FAIL 等返回给框架，由上层处理 */
    return r_size;
}

static esp_err_t _file_close(audio_element_handle_t self)
{
    file_src_t *src = (file_src_t *)audio_element_getdata(self);
    if (src && src->fp) {
        fclose(src->fp);
        src->fp = NULL;
    }
    return ESP_OK;
}

audio_element_handle_t file_src_element_create(void)
{
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.tag         = "file_src";
    cfg.open        = _file_open;
    cfg.close       = _file_close;
    cfg.read        = _file_read;
    cfg.process     = _file_process;
    cfg.task_core   = 0;  /* CPU0: SD卡文件读取必须与LVGL/LCD同核，避免跨核SPI2竞争 */
    cfg.task_prio   = 8;
    /* 与 http_stream 保持一致的任务栈配置，避免在本地文件模式下
     * 因默认 2KB 栈过小导致的 “task http” 栈溢出。*/
    cfg.task_stack  = HTTP_STREAM_TASK_STACK;
#ifdef CONFIG_AUDIO_PLAYER_HTTP_STACK_IN_EXT
    cfg.stack_in_ext = (CONFIG_AUDIO_PLAYER_HTTP_STACK_IN_EXT != 0);
#else
    cfg.stack_in_ext = false;
#endif

    /* 与 http_stream 类似的缓冲与 ringbuffer 配置 */
#ifndef CONFIG_AUDIO_PLAYER_HTTP_BUFSZ
#define CONFIG_AUDIO_PLAYER_HTTP_BUFSZ 2048
#endif
#ifndef CONFIG_AUDIO_PLAYER_RINGBUF_SZ
#define CONFIG_AUDIO_PLAYER_RINGBUF_SZ (8 * 1024)
#endif
    cfg.buffer_len = CONFIG_AUDIO_PLAYER_HTTP_BUFSZ;
    cfg.out_rb_size = CONFIG_AUDIO_PLAYER_RINGBUF_SZ;

    file_src_t *src = (file_src_t *)audio_calloc(1, sizeof(file_src_t));
    if (!src) {
        ESP_LOGE(TAG, "no mem for file_src");
        return NULL;
    }

    audio_element_handle_t el = audio_element_init(&cfg);
    if (!el) {
        ESP_LOGE(TAG, "audio_element_init failed for file_src");
        audio_free(src);
        return NULL;
    }

    audio_element_setdata(el, src);
    return el;
}
