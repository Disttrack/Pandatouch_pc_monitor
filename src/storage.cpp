#include "storage.h"
#include <Preferences.h>
#include <LittleFS.h>
#include <WiFi.h>

char g_wifi_ssid[32] = "";
char g_wifi_pass[64] = "";
uint8_t g_brightness = 80;
uint32_t g_bg_color = 0x0A0A0A;
String g_ip_addr = "0.0.0.0";

void init_storage() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS init failed, formatting...");
    }
    load_settings();
}

void load_settings() {
    Preferences preferences;
    preferences.begin("monitor", true);
    preferences.getString("wifi_ssid", g_wifi_ssid, 31);
    preferences.getString("wifi_pass", g_wifi_pass, 63);
    g_brightness = preferences.getUChar("brightness", 80);
    g_bg_color = preferences.getUInt("bg_color", 0x0A0A0A);
    preferences.end();

    if (strlen(g_wifi_ssid) > 0) {
        WiFi.begin(g_wifi_ssid, g_wifi_pass);
        Serial.printf("WiFi: Connecting to %s...\n", g_wifi_ssid);
    }
}

void save_settings() {
    Preferences preferences;
    preferences.begin("monitor", false);
    preferences.putString("wifi_ssid", g_wifi_ssid);
    preferences.putString("wifi_pass", g_wifi_pass);
    preferences.putUChar("brightness", g_brightness);
    preferences.putUInt("bg_color", g_bg_color);
    preferences.end();
}