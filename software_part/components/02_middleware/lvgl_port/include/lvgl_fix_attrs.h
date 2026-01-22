#pragma once
#include "sdkconfig.h"

// 这些 Kconfig 是字符串类型，sdkconfig.h 会定义成带引号的宏，直接参与语法会报错。
// 在这里把它们取消，令 LVGL 的 lv_conf_internal.h 走默认的“空定义”分支。
#ifdef CONFIG_LV_ATTRIBUTE_LARGE_CONST
#undef CONFIG_LV_ATTRIBUTE_LARGE_CONST
#endif
#ifdef CONFIG_LV_ATTRIBUTE_MEM_ALIGN
#undef CONFIG_LV_ATTRIBUTE_MEM_ALIGN
#endif
#ifdef CONFIG_LV_ATTRIBUTE_FAST_MEM
#undef CONFIG_LV_ATTRIBUTE_FAST_MEM
#endif
#ifdef CONFIG_LV_ATTRIBUTE_DMA
#undef CONFIG_LV_ATTRIBUTE_DMA
#endif

