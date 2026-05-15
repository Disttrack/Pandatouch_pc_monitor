#include "webserver.h"
#include "metrics.h"
#include "ui_monitor.h"
#include "storage.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <Update.h>
#include <ArduinoOTA.h>
#define WEBSERVER_H
#define HTTP_GET     0b00000001
#define HTTP_POST    0b00000010
#define HTTP_DELETE  0b00000100
#define HTTP_PUT     0b00001000
#define HTTP_PATCH   0b00010000
#define HTTP_HEAD    0b00100000
#define HTTP_OPTIONS 0b01000000
#define HTTP_ANY     0b01111111
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

AsyncWebServer server(80);
static bool webserver_started = false;

void check_wifi_status() {
    static bool was_connected = false;
    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected) {
        g_ip_addr = WiFi.localIP().toString();
        if (g_header_ip) {
            lv_label_set_text_fmt(g_header_ip, "IP: %s", g_ip_addr.c_str());
        }
        if (!was_connected) {
            init_webserver();
        }
    } else {
        g_ip_addr = "Disconnected";
    }
    was_connected = connected;
}

void init_webserver() {
    if (webserver_started) return;
    webserver_started = true;

    server.on("/api/metrics", HTTP_POST, [](AsyncWebServerRequest* request) {},
    NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        static String buffer;
        if (index == 0) buffer = "";
        for (size_t i = 0; i < len; i++) buffer += (char)data[i];

        if (index + len == total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, buffer);
            buffer = "";

            if (!error) {
                g_metrics.fps = doc["fps"] | g_metrics.fps;
                g_metrics_updated = true;
            }
            request->send(200, "text/plain", "OK");
        }
    });

    server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Update started");
        g_ota_screen_requested = true;
    }, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            Serial.printf("OTA: Update start, total=%u\n", total);
            if (!Update.begin(total)) {
                Update.printError(Serial);
                return;
            }
            g_ota_progress = 0;
        }
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
            return;
        }
        if (total > 0) {
            g_ota_progress = (int)((index + len) * 100 / total);
        }
        if (index + len == total) {
            if (Update.end(true)) {
                Serial.println("OTA: Update success, rebooting...");
                request->send(200, "text/plain", "Update OK, rebooting");
                delay(500);
                ESP.restart();
            } else {
                Update.printError(Serial);
                request->send(500, "text/plain", "Update failed");
            }
        }
    });

    server.begin();
    Serial.println("WEB: HTTP server started");
}
