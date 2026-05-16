#include <Arduino.h>
#include <WiFi.h>
#include "pt/pt_display.h"
#include "monitor_app.h"
#include "webserver.h"
#include "storage.h"

void setup() {
    Serial.begin(115200);
    delay(200);

    setCpuFrequencyMhz(240);

    Serial.println("\n\n=== PandaTouch PC Monitor Starting ===");
    pt_setup_display(PT_LVGL_RENDER_PARTIAL_1);

    WiFi.mode(WIFI_STA);
    MonitorApp::setup();

    pt_set_backlight(g_brightness, true);
}

void loop() {
    MonitorApp::loop();
}