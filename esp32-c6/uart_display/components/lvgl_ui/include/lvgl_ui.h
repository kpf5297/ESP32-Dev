#pragma once

#include "lvgl.h"
#include "ui.h"
#include "ui_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize LVGL UI system */
void lvgl_ui_init(void);

/* UI screen functions */
void ui_init(void);
void ui_show_splash(void);
void ui_show_wifi(void);
void ui_show_debug(void);
void ui_show_clock(void);

/* UI helper functions */
void ui_update_label(lv_obj_t *label, const char *text);

#ifdef __cplusplus
}
#endif
