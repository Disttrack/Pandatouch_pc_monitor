#include "monitor_app.h"
#include "ui_monitor.h"
#include "storage.h"
#include "metrics.h"
#include "pt/pt_display.h"
#include "webserver.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

PCMetrics g_metrics = {0};
volatile bool g_metrics_updated = false;

// ── PSRAM allocator for ArduinoJson ──
class PSRAMAlloc : public ArduinoJson::Allocator {
    void* allocate(size_t size) override { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
    void deallocate(void* ptr) override { heap_caps_free(ptr); }
    void* reallocate(void* ptr, size_t new_size) override { return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM); }
};
static PSRAMAlloc s_psram_alloc;

static float parse_lhm_val(const char* s) {
    if (!s || !*s) return 0.0f;
    char buf[32];
    size_t i = 0;
    for (; i < sizeof(buf) - 1 && s[i] && s[i] != ' '; i++) {
        buf[i] = (s[i] == ',') ? '.' : s[i];
    }
    buf[i] = '\0';
    float val = strtof(buf, nullptr);
    const char* unit = s + i;
    while (*unit == ' ') unit++;
    if (strncmp(unit, "MB", 2) == 0 || strncmp(unit, "MiB", 3) == 0)
        val /= 1024.0f;
    return val;
}

// scratch values computed per parse cycle
static float s_ram_available_gb = 0;

static void walk_lhm_tree(JsonObject obj, PCMetrics& m, const String& ctx = "") {
    const char* type = obj["Type"];
    const char* text = obj["Text"];
    const char* val_s = obj["Value"];

    String full_ctx = ctx;
    if (text) {
        if (full_ctx.length() > 0) full_ctx += "/";
        full_ctx += text;
    }
    String ctx_lower = full_ctx;
    ctx_lower.toLowerCase();

    if (type && text && val_s) {
        float val = parse_lhm_val(val_s);
        String t = text;
        t.toLowerCase();

        if (strcmp(type, "Temperature") == 0) {
            if ((t.indexOf("cpu") >= 0 || t.indexOf("package") >= 0) && t.indexOf("gpu") < 0)
                m.cpu_temp = max(m.cpu_temp, val);
            else if (t.indexOf("gpu core") >= 0 || t.indexOf("gpu temperature") >= 0)
                m.gpu_temp = max(m.gpu_temp, val);
            else if (t.indexOf("hot spot") >= 0 || t.indexOf("gpu hotspot") >= 0)
                m.gpu_hotspot_temp = max(m.gpu_hotspot_temp, val);
            else if (t.indexOf("gpu memory") >= 0 || t.indexOf("memory junction") >= 0)
                m.gpu_mem_temp = max(m.gpu_mem_temp, val);
            else if (t.indexOf("system") >= 0 || t.indexOf("motherboard") >= 0)
                m.mobo_temp = max(m.mobo_temp, val);
            else if (t.indexOf("pch") >= 0 || t.indexOf("chipset") >= 0)
                m.pch_temp = max(m.pch_temp, val);
            else if (t.indexOf("vrm") >= 0)
                m.vrm_temp = max(m.vrm_temp, val);
            else if ((t.indexOf("core") >= 0 || t.indexOf("ccd") >= 0 || t.indexOf("tdie") >= 0) && t.indexOf("gpu") < 0)
                m.cpu_temp = max(m.cpu_temp, val);
            else if (t.indexOf("dimm") >= 0 || t.indexOf("dram") >= 0) {
                if (m.dimm_temp_min == 0 || val < m.dimm_temp_min) m.dimm_temp_min = val;
                if (val > m.dimm_temp_max) m.dimm_temp_max = val;
            }
        } else if (strcmp(type, "Load") == 0) {
            if (t.indexOf("gpu core") >= 0 || t.indexOf("d3d 3d") >= 0)
                m.gpu_percent = max(m.gpu_percent, val);
            else if (t.indexOf("cpu total") >= 0)
                m.cpu_percent = max(m.cpu_percent, val);
            else if (t.indexOf("memory controller") >= 0)
                m.vram_percent = max(m.vram_percent, val);
            else if (t == "memory" && ctx_lower.indexOf("total memory") >= 0)
                m.ram_percent = max(m.ram_percent, val);
        } else if (strcmp(type, "Clock") == 0) {
            if (t.indexOf("gpu core") >= 0 && t.indexOf("memory") < 0)
                m.gpu_freq = max(m.gpu_freq, val);
            else if (t.indexOf("gpu memory") >= 0)
                m.gpu_mem_freq = max(m.gpu_mem_freq, val);
            else if (t.indexOf("average") >= 0 && t.indexOf("cores") >= 0 && t.indexOf("effective") < 0)
                m.cpu_freq = max(m.cpu_freq, val);
        } else if (strcmp(type, "Power") == 0) {
            if (t.indexOf("package") >= 0 && ctx_lower.indexOf("ryzen") >= 0)
                m.cpu_power = max(m.cpu_power, val);
            else if (t.indexOf("package") >= 0 && ctx_lower.indexOf("nvidia") >= 0)
                m.gpu_power = max(m.gpu_power, val);
        } else if (strcmp(type, "Fan") == 0) {
            if (t.indexOf("gpu") >= 0 || ctx_lower.indexOf("gpu") >= 0)
                m.gpu_fan = max(m.gpu_fan, val);
            else if (t.indexOf("cpu fan") >= 0)
                m.cpu_fan = max(m.cpu_fan, val);
            else if (t.indexOf("system fan") >= 0 && val > 0) {
                if (m.sys_fan_min == 0 || val < m.sys_fan_min) m.sys_fan_min = val;
                if (val > m.sys_fan_max) m.sys_fan_max = val;
            } else if (t.indexOf("pump") >= 0)
                m.pump_fan = max(m.pump_fan, val);
        } else if (strcmp(type, "Data") == 0) {
            if (ctx_lower.indexOf("total memory") >= 0) {
                if (t.indexOf("used") >= 0)
                    m.ram_used_gb = max(m.ram_used_gb, val);
                else if (t.indexOf("available") >= 0)
                    s_ram_available_gb = max(s_ram_available_gb, val);
            }
        } else if (strcmp(type, "SmallData") == 0) {
            if (t.indexOf("memory used") >= 0 && ctx_lower.indexOf("nvidia") >= 0)
                m.vram_used_gb = max(m.vram_used_gb, val);
            else if (t.indexOf("memory total") >= 0 && ctx_lower.indexOf("nvidia") >= 0)
                m.vram_total_gb = max(m.vram_total_gb, val);
        }
    }

    // Recurse into children, passing current text as context
    JsonArray children = obj["Children"].as<JsonArray>();
    if (children) {
        for (JsonVariant child : children) {
            JsonObject co = child.as<JsonObject>();
            if (co) walk_lhm_tree(co, m, full_ctx);
        }
    }
}

static unsigned long s_last_lhm = 0;
static const unsigned long LHM_INTERVAL = 1000;

void fetch_lhm_data() {
    if (g_pc_ip.length() == 0) return;
    unsigned long now = millis();
    if (now - s_last_lhm < LHM_INTERVAL) return;
    s_last_lhm = now;
    if (WiFi.status() != WL_CONNECTED) return;

    String url = "http://" + g_pc_ip + ":8085/data.json";
    Serial.printf("LHM: fetching %s\n", url.c_str());
    HTTPClient http;
    http.begin(url);
    http.setTimeout(5000);
    int code = http.GET();
    if (code <= 0) {
        Serial.printf("LHM: HTTP error %d\n", code);
        http.end();
        return;
    }
    Serial.printf("LHM: HTTP %d\n", code);
    if (code != 200) { http.end(); return; }

    int len = http.getSize();
    if (len <= 0) len = 262144;

    WiFiClient* stream = http.getStreamPtr();
    if (!stream) { http.end(); return; }

    char* buf = (char*)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
    if (!buf) { http.end(); return; }

    size_t total = 0;
    unsigned long timeout = millis() + 5000;
    stream->setTimeout(5000);
    while (stream->connected() && total < (size_t)len && millis() < timeout) {
        size_t avail = stream->available();
        if (avail) {
            int r = stream->readBytes(buf + total, min((size_t)avail, (size_t)(len - total)));
            total += r;
        } else {
            delay(1);
        }
    }
    buf[total] = '\0';
    http.end();

    if (total > 0) {
        Serial.printf("LHM: read %u bytes\n", total);
        PCMetrics m = {0};
        m.disk_percent = g_metrics.disk_percent;
        s_ram_available_gb = 0;

        JsonDocument doc(&s_psram_alloc);
        DeserializationError err = deserializeJson(doc, buf, DeserializationOption::NestingLimit(64));
        if (!err) {
            JsonArray arr = doc.as<JsonArray>();
            if (arr) {
                for (JsonVariant item : arr) {
                    JsonObject obj = item.as<JsonObject>();
                    if (obj) walk_lhm_tree(obj, m);
                }
            } else {
                JsonObject obj = doc.as<JsonObject>();
                if (obj) walk_lhm_tree(obj, m);
            }
            if (s_ram_available_gb > 0 && m.ram_used_gb > 0)
                m.ram_total_gb = m.ram_used_gb + s_ram_available_gb;
            if (m.vram_total_gb > 0)
                m.vram_percent = (m.vram_used_gb / m.vram_total_gb) * 100.0f;
            m.fps = g_metrics.fps;
            g_metrics = m;
            g_metrics_updated = true;
            Serial.printf("LHM: updated cpu=%.0f gpu=%.0f cpu_t=%.0f gpu_t=%.0f\n",
                          m.cpu_percent, m.gpu_percent, m.cpu_temp, m.gpu_temp);
        } else {
            Serial.printf("LHM: JSON parse error: %s\n", err.c_str());
        }
    } else {
        Serial.println("LHM: no data read");
    }

    heap_caps_free(buf);
}

// ── Setup & Loop ──

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

    check_wifi_status();

    fetch_lhm_data();

    if (g_metrics_updated) {
        update_monitor_ui();
    }
    ArduinoOTA.handle();

    yield();
    esp_task_wdt_reset();
}
