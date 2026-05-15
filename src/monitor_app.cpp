#include "monitor_app.h"
#include "ui_monitor.h"
#include "storage.h"
#include "metrics.h"
#include "pt/pt_display.h"
#include "webserver.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

PCMetrics g_metrics = {0};
volatile bool g_metrics_updated = false;

void MonitorApp::setup() {
    init_storage();

    create_monitor_ui();

    ArduinoOTA.setHostname("pandatouch-monitor");
    ArduinoOTA.begin();
}

void MonitorApp::loop() {
    pt_loop_display();

    if (g_ota_screen_requested) {
        g_ota_screen_requested = false;
        pt_enter_ota_mode();
    }

    if (g_metrics_updated) {
        update_monitor_ui();
    }

    check_wifi_status();
    ArduinoOTA.handle();

    yield();
    esp_task_wdt_reset();
}