#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

extern char g_wifi_ssid[32];
extern char g_wifi_pass[64];
extern uint8_t g_brightness;
extern uint32_t g_bg_color;
extern String g_ip_addr;
extern String g_pc_ip;

void init_storage();
void load_settings();
void save_settings();

#endif
