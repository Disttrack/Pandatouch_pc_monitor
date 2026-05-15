#ifndef UI_MONITOR_H
#define UI_MONITOR_H

#include <lvgl.h>
#include "metrics.h"

extern lv_obj_t* g_monitor_screen;
extern lv_obj_t* g_header_ip;
extern volatile bool g_ota_screen_requested;
extern volatile int g_ota_progress;

void create_monitor_ui();
void update_monitor_ui();
void show_ota_screen();
void update_ota_progress(int pct, const char* msg);

#endif
