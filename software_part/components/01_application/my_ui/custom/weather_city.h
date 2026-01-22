// Simple province / city / county linkage based on embedded CSV
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
#include "gui_guider.h"

// 初始化并绑定 UI，下拉项会被填充，事件会被挂上
void weather_city_setup(lv_ui *ui);

// 获取当前选择的代码（若无则返回 0）
uint32_t weather_city_get_selected_province_code(void);
uint32_t weather_city_get_selected_pref_code(void);
uint32_t weather_city_get_selected_county_code(void);

// 根据 code 选择省/市/县（若找不到则忽略该层）
void weather_city_select_codes(lv_ui *ui, uint32_t prov_code, uint32_t pref_code, uint32_t county_code);
// 读取当前三联动所选 codes
void weather_city_get_selected_codes(uint32_t *prov_code, uint32_t *pref_code, uint32_t *county_code);
