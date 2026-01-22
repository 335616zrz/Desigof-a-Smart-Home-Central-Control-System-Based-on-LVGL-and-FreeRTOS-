// Persist and load last selected weather adcodes in NVS
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Load saved province/prefecture/county adcodes. Returns true if all three
// were found (any missing will be returned as 0 and function returns false).
bool weather_persist_load(uint32_t *prov, uint32_t *pref, uint32_t *county);

// Save province/prefecture/county adcodes. Missing layers can be 0.
bool weather_persist_save(uint32_t prov, uint32_t pref, uint32_t county);

// Additional helpers: persist last env display mode (0=室外,1=室内).
// If key not found, *indoor will be left as false and function returns false.
bool weather_persist_load_env_mode(bool *indoor);
bool weather_persist_save_env_mode(bool indoor);
