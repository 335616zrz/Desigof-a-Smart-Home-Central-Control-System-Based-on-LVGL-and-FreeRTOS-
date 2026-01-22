cmake_minimum_required(VERSION 3.16)
include_guard(GLOBAL)

# ================== 基本前置 ==================
if(NOT DEFINED ENV{IDF_PATH})
  message(FATAL_ERROR "未检测到 IDF_PATH 环境变量，请在 ESP-IDF 环境下运行（使用 idf.py 或先执行 export）")
endif()
if(NOT EXISTS "$ENV{IDF_PATH}/tools/cmake/project.cmake")
  message(FATAL_ERROR "无效的 IDF_PATH：未找到 $ENV{IDF_PATH}/tools/cmake/project.cmake")
endif()

# ================== ADF 开关与路径 ==================
# 仅支持三块：audio_pipeline、audio_sal、esp-adf-libs
option(USE_ADF "启用 ESP-ADF 相关组件（仅三块）" ON)
set(ADF_PATH "" CACHE PATH "ESP-ADF 根目录；留空则尝试 ENV 或常见候选路径")

if(USE_ADF)
  # 路径发现：Cache > ENV > 常见候选
  if(NOT ADF_PATH)
    if(DEFINED ENV{ADF_PATH})
      set(ADF_PATH $ENV{ADF_PATH})
    else()
      set(_ADF_CANDIDATES
        "${CMAKE_SOURCE_DIR}/../esp-adf"
        "$ENV{HOME}/esp/esp-adf"
        "$ENV{USERPROFILE}/esp/esp-adf"
        "/home/zrz/esp/esp-adf"
      )
      foreach(p IN LISTS _ADF_CANDIDATES)
        if(EXISTS "${p}/components/audio_pipeline/CMakeLists.txt")
          set(ADF_PATH "${p}")
          break()
        endif()
      endforeach()
    endif()
  endif()

  if(NOT ADF_PATH)
    message(FATAL_ERROR "未能定位 ESP-ADF，请通过 -DADF_PATH=/abs/path 指定或设置环境变量 ADF_PATH")
  endif()

  # 归一化为绝对路径并校验仅需的三个组件存在
  get_filename_component(ADF_PATH "${ADF_PATH}" ABSOLUTE)
  foreach(req IN ITEMS audio_pipeline audio_sal esp-adf-libs)
    if(NOT EXISTS "${ADF_PATH}/components/${req}/CMakeLists.txt")
      message(FATAL_ERROR "缺少 ${ADF_PATH}/components/${req}/CMakeLists.txt ，请确认 ADF_PATH")
    endif()
  endforeach()
endif()

# ================== 仅加入需要的组件目录 ==================
# 说明：为提升健壮性，对不存在的目录进行过滤并提示 WARNING（不终止）
set(_USER_COMPONENT_DIRS
  # common (shared helpers)
  ${CMAKE_SOURCE_DIR}/components/common/mem_utils

  # 01_application
  ${CMAKE_SOURCE_DIR}/components/01_application/app_music
  ${CMAKE_SOURCE_DIR}/components/01_application/app_ui
  ${CMAKE_SOURCE_DIR}/components/01_application/app_env_sht40
  ${CMAKE_SOURCE_DIR}/components/01_application/app_inmp441
  ${CMAKE_SOURCE_DIR}/components/01_application/app_ws2812
  ${CMAKE_SOURCE_DIR}/components/01_application/my_ui
  ${CMAKE_SOURCE_DIR}/components/01_application/voice_commands

  # 02_middleware
  ${CMAKE_SOURCE_DIR}/components/02_middleware/audio_aec
  ${CMAKE_SOURCE_DIR}/components/02_middleware/audio_ns
  ${CMAKE_SOURCE_DIR}/components/02_middleware/audio_player
  ${CMAKE_SOURCE_DIR}/components/02_middleware/asr_client
  ${CMAKE_SOURCE_DIR}/components/02_middleware/http_stream
  ${CMAKE_SOURCE_DIR}/components/02_middleware/hal_display_touch
  ${CMAKE_SOURCE_DIR}/components/02_middleware/iot_mqtt
  ${CMAKE_SOURCE_DIR}/components/02_middleware/lvgl_port
  ${CMAKE_SOURCE_DIR}/components/02_middleware/net_rtt_ping
  ${CMAKE_SOURCE_DIR}/components/02_middleware/net_wifi_scan
  ${CMAKE_SOURCE_DIR}/components/02_middleware/net_wifi_sntp
  ${CMAKE_SOURCE_DIR}/components/02_middleware/ota_update
  ${CMAKE_SOURCE_DIR}/components/02_middleware/voice_afe

  # 03_drivers
  ${CMAKE_SOURCE_DIR}/components/03_drivers/audio_i2s
  ${CMAKE_SOURCE_DIR}/components/03_drivers/drivers_ft6336
  ${CMAKE_SOURCE_DIR}/components/03_drivers/drivers_st7796
  ${CMAKE_SOURCE_DIR}/components/03_drivers/i2c_touch_bus
  ${CMAKE_SOURCE_DIR}/components/03_drivers/drivers_sht40
  ${CMAKE_SOURCE_DIR}/components/03_drivers/drivers_inmp441
  ${CMAKE_SOURCE_DIR}/components/03_drivers/drivers_ws2812b
  ${CMAKE_SOURCE_DIR}/components/03_drivers/sdcard_spi

  # 04_hardware
  ${CMAKE_SOURCE_DIR}/components/04_hardware/board
)

set(EXTRA_COMPONENT_DIRS "")
foreach(dir IN LISTS _USER_COMPONENT_DIRS)
  if(EXISTS "${dir}/CMakeLists.txt")
    list(APPEND EXTRA_COMPONENT_DIRS "${dir}")
  else()
    message(WARNING "跳过不存在的组件目录：${dir}")
  endif()
endforeach()

# —— ADF：严格只加入三块 —— #
if(USE_ADF)
  foreach(adf_dir IN ITEMS
    "${ADF_PATH}/components/audio_pipeline"
    "${ADF_PATH}/components/audio_sal"
    "${ADF_PATH}/components/esp-adf-libs")
    if(EXISTS "${adf_dir}/CMakeLists.txt")
      list(APPEND EXTRA_COMPONENT_DIRS "${adf_dir}")
    else()
      message(FATAL_ERROR "预期的 ADF 组件缺失：${adf_dir}")
    endif()
  endforeach()
endif()

# ================== 引入 IDF 构建系统并声明工程 ==================
# 注意：EXTRA_COMPONENT_DIRS 必须在 include(project.cmake) 之前设置
include("$ENV{IDF_PATH}/tools/cmake/project.cmake")
project(undergraduate_project)

# ================== LVGL 预定义（保持原配置） ==================
idf_build_set_property(COMPILE_DEFINITIONS "LV_CONF_INCLUDE_SIMPLE" APPEND)

# LVGL Kconfig "Compiler Settings" uses string values (default ""), which become
# C string literals in sdkconfig.h. LVGL expects these macros to be raw tokens
# (e.g. __attribute__((...))). Define them empty here to avoid `""` breaking C.
idf_build_set_property(COMPILE_DEFINITIONS "LV_ATTRIBUTE_LARGE_CONST=" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_ATTRIBUTE_MEM_ALIGN="  APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_ATTRIBUTE_FAST_MEM="   APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_ATTRIBUTE_DMA="        APPEND)

# B1: Put LVGL's large RAM arrays (incl. LVGL internal heap pool) into PSRAM to
# free internal DMA-capable RAM for SDMMC/I2S/etc.
idf_build_set_property(COMPILE_DEFINITIONS "LV_ATTRIBUTE_LARGE_RAM_ARRAY=EXT_RAM_BSS_ATTR" APPEND)

# ================== 状态输出（便于排查） ==================
message(STATUS "USE_ADF = ${USE_ADF}")
if(USE_ADF)
  message(STATUS "ADF_PATH = ${ADF_PATH}")
endif()
message(STATUS "EXTRA_COMPONENT_DIRS:")
foreach(dir IN LISTS EXTRA_COMPONENT_DIRS)
  message(STATUS "  - ${dir}")
endforeach()

# ================== （可选）白名单构建 ==================
# 如需进一步限制仅构建明确列出的组件，可启用下方白名单（需补齐实际子组件名）：
# set(IDF_COMPONENTS
#   main
#   app_ui app_music
#   audio_player http_stream
#   audio_pipeline audio_sal
#   # 如需 esp-adf-libs 内具体子组件（如 audio_stream/audio_hal 等），在此补充
# )
