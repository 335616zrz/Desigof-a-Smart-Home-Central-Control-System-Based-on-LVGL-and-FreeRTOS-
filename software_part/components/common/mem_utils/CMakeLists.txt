# components/common/mem_utils/CMakeLists.txt
# Header-only helpers: PSRAM/internal heap allocation helpers + diagnostics.

idf_component_register(
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_common
        esp_hw_support
        heap
        log
)

target_compile_options(${COMPONENT_LIB} INTERFACE
    -Wall -Wextra
    -Wno-unused-parameter
)

