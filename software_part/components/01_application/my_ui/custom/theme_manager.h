// theme_manager.h
#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise theme manager and apply last persisted theme.
 */
void theme_manager_init(void);

/**
 * @brief Switch UI theme and persist choice.
 * @param is_dark true to enable dark theme, false for light.
 */
void my_ui_set_theme(bool is_dark);

/**
 * @brief Get current theme flag.
 * @return true if dark theme is active.
 */
bool my_ui_get_current_theme(void);

#ifdef __cplusplus
}
#endif

#endif // THEME_MANAGER_H
