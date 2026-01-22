#pragma once

#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_memory_utils.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== PSRAM alloc ==================== */

#define PSRAM_MALLOC(size) \
    heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#define PSRAM_CALLOC(n, size) \
    heap_caps_calloc((n), (size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#define PSRAM_REALLOC(ptr, size) \
    heap_caps_realloc((ptr), (size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

static inline char *psram_strdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = (char *)PSRAM_MALLOC(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static inline void *psram_malloc_fallback(size_t size, const char *tag)
{
    void *ptr = PSRAM_MALLOC(size);
    if (!ptr) {
        ESP_LOGW(tag, "PSRAM alloc %u failed, fallback to internal", (unsigned)size);
        ptr = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return ptr;
}

#define PSRAM_MALLOC_FALLBACK(size, tag) psram_malloc_fallback((size), (tag))

#define INTERNAL_MALLOC(size) \
    heap_caps_malloc((size), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)

/* DMA compatible alloc: only for truly DMA-required buffers. */
#define DMA_MALLOC(size) \
    heap_caps_malloc((size), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)

/* ==================== Pointer helpers ==================== */

#define IS_PSRAM_PTR(ptr) esp_ptr_external_ram(ptr)

#define LOG_PTR_LOCATION(tag, name, ptr) \
    ESP_LOGI((tag), "%s @ %p (%s)", (name), (void *)(ptr), \
             esp_ptr_external_ram((ptr)) ? "PSRAM" : "Internal")

/* ==================== Safe free ==================== */

#define SAFE_FREE(ptr) do { \
    if (ptr) { free(ptr); (ptr) = NULL; } \
} while (0)

#define SAFE_HEAP_FREE(ptr) do { \
    if (ptr) { heap_caps_free(ptr); (ptr) = NULL; } \
} while (0)

/* ==================== Heap diagnostics ==================== */

#define MEM_DIAG(label) do { \
    ESP_LOGI("MEM", "[%s] Internal: free=%luKB largest=%luKB | PSRAM: free=%luKB largest=%luKB", \
             (label), \
             (unsigned long)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), \
             (unsigned long)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024), \
             (unsigned long)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024), \
             (unsigned long)(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024)); \
} while (0)

#ifdef __cplusplus
}
#endif

