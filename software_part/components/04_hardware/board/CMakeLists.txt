# components/04_hardware/board/CMakeLists.txt
# Header-only 组件：仅暴露板级常量与驱动头引用；不参与链接

idf_component_register(
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver          # 提供 driver/gpio.h、driver/spi_master.h、driver/ledc.h 等
)

# 仅对本组件生效（即使是 header-only），保持编译规范且不污染全局
target_compile_options(${COMPONENT_LIB} INTERFACE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

