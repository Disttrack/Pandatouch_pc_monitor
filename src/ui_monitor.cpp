#include "ui_monitor.h"
#include "ui_wifi_config.h"
#include "storage.h"
#include "pt/pt_display.h"
#include <LittleFS.h>
#include <esp_task_wdt.h>

lv_obj_t* g_monitor_screen = nullptr;
lv_obj_t* g_header_ip = nullptr;
volatile bool g_ota_screen_requested = false;
volatile int g_ota_progress = -1;

static lv_obj_t* g_card_titles[4] = {nullptr};
static lv_obj_t* g_card_subs[4] = {nullptr};
static lv_obj_t* g_card_temps[4] = {nullptr};
static lv_obj_t* g_card_bars[4] = {nullptr};
static lv_obj_t* g_card_bars2[4] = {nullptr};
static lv_obj_t* g_bar_labels[4] = {nullptr};
static lv_obj_t* g_vram_label = nullptr;
static lv_obj_t* g_main_cont = nullptr;

static lv_color_t bar_color(float pct) {
    if (pct < 50.0f) return lv_color_hex(0x4CAF50);
    if (pct < 80.0f) return lv_color_hex(0xFF9800);
    return lv_color_hex(0xF44336);
}

static lv_color_t temp_color(float t) {
    if (t < 50.0f) return lv_color_hex(0x4CAF50);
    if (t < 70.0f) return lv_color_hex(0xFF9800);
    return lv_color_hex(0xF44336);
}

static void settings_btn_cb(lv_event_t* e) {
    (void)e;
    create_wifi_ui();
}

static void ota_ui() {
    if (!g_monitor_screen) return;
    lv_obj_clean(g_monitor_screen);

    lv_obj_t* bar = lv_bar_create(g_monitor_screen);
    lv_obj_set_size(bar, 400, 30);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);

    lv_obj_t* lbl = lv_label_create(g_monitor_screen);
    lv_label_set_text(lbl, "Updating firmware...");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -60);

    g_ota_progress = 0;
    lv_scr_load(g_monitor_screen);
}

void create_monitor_ui() {
    if (!g_monitor_screen) {
        g_monitor_screen = lv_obj_create(NULL);
    }
    lv_obj_clean(g_monitor_screen);
    lv_obj_set_style_bg_color(g_monitor_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    g_main_cont = lv_obj_create(g_monitor_screen);
    lv_obj_set_size(g_main_cont, 800, 445);
    lv_obj_set_pos(g_main_cont, 0, 45);
    lv_obj_set_style_bg_opa(g_main_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_main_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_main_cont, 15, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_main_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g_main_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(g_main_cont, 12, LV_PART_MAIN);

    const char* titles[4] = {"CPU", "RAM", "GPU", "System"};
    const char* subs[4] = {
        "--%   -- MHz",
        "--.-- / --.-- GB",
        "--%   -- MHz\n-- C\n--.--/--.-- GB  --%",
        "No data"
    };
    int card_heights[4] = {195, 195, 195, 195};

    for (int i = 0; i < 4; i++) {
        lv_obj_t* card = lv_obj_create(g_main_cont);
        lv_obj_set_size(card, 375, card_heights[i]);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 18, LV_PART_MAIN);

        lv_obj_t* title = lv_label_create(card);
        lv_label_set_text(title, titles[i]);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
        g_card_titles[i] = title;

        g_card_temps[i] = lv_label_create(card);
        if (i == 1) {
            lv_label_set_text(g_card_temps[i], "");
        } else {
            const char* placeholder = (i == 3) ? "-- FPS" : "-- C";
            lv_label_set_text(g_card_temps[i], placeholder);
            lv_obj_set_style_text_font(g_card_temps[i], &lv_font_montserrat_28, LV_PART_MAIN);
            lv_obj_set_style_text_color(g_card_temps[i], lv_color_hex(0xAAAAAA), LV_PART_MAIN);
            lv_obj_align(g_card_temps[i], LV_ALIGN_TOP_RIGHT, -5, -3);
        }

        lv_obj_t* sub = lv_label_create(card);
        lv_label_set_text(sub, subs[i]);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(sub, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
        lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 0, 32);
        g_card_subs[i] = sub;

        lv_obj_t* bar = lv_bar_create(card);
        lv_obj_set_size(bar, 339, 16);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 0, -28);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 8, LV_PART_MAIN);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        g_card_bars[i] = bar;

        lv_obj_t* bl = lv_label_create(card);
        lv_label_set_text(bl, "0%");
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(bl, LV_ALIGN_BOTTOM_RIGHT, 0, -26);
        g_bar_labels[i] = bl;

        g_card_bars2[i] = nullptr;
        if (i == 2) {
            lv_obj_t* bar2 = lv_bar_create(card);
            lv_obj_set_size(bar2, 339, 12);
            lv_obj_align(bar2, LV_ALIGN_BOTTOM_LEFT, 0, -50);
            lv_obj_set_style_bg_color(bar2, lv_color_hex(0x333333), LV_PART_MAIN);
            lv_obj_set_style_radius(bar2, 6, LV_PART_MAIN);
            lv_bar_set_value(bar2, 0, LV_ANIM_OFF);
            g_card_bars2[i] = bar2;

            lv_obj_t* vpct = lv_label_create(card);
            lv_label_set_text(vpct, "VRAM 0%");
            lv_obj_set_style_text_font(vpct, &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_text_color(vpct, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_align(vpct, LV_ALIGN_BOTTOM_RIGHT, 0, -48);
            g_vram_label = vpct;
        }
    }

    g_header_ip = lv_label_create(g_monitor_screen);
    lv_label_set_text(g_header_ip, "IP: 0.0.0.0");
    lv_obj_set_style_text_font(g_header_ip, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_header_ip, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(g_header_ip, LV_ALIGN_TOP_LEFT, 20, 16);

    lv_obj_t* settings_btn = lv_btn_create(g_monitor_screen);
    lv_obj_set_size(settings_btn, 80, 30);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -20, 12);
    lv_obj_set_style_bg_color(settings_btn, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_radius(settings_btn, 8, LV_PART_MAIN);

    lv_obj_t* sbl = lv_label_create(settings_btn);
    lv_label_set_text(sbl, "\xEF\x80\x93 Config");
    lv_obj_set_style_text_font(sbl, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(sbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(sbl);

    lv_obj_add_event_cb(settings_btn, settings_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(g_monitor_screen);
}

void update_monitor_ui() {
    if (!g_monitor_screen) return;

    // ── Card 0: CPU ──
    lv_bar_set_value(g_card_bars[0], (int)g_metrics.cpu_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[0], bar_color(g_metrics.cpu_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[0], "%.0f%%", g_metrics.cpu_percent);
    {
        char buf[96];
        int pos = 0;
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%.0f%%   %.0f MHz",
                        g_metrics.cpu_percent, g_metrics.cpu_freq);
        if (g_metrics.cpu_power > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "   Pkg:%.0f W",
                            g_metrics.cpu_power);
        if (g_metrics.cpu_fan > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\nCPU Fan: %.0f RPM",
                            g_metrics.cpu_fan);
        lv_label_set_text(g_card_subs[0], buf);
    }
    lv_label_set_text_fmt(g_card_temps[0], "%.0f C", g_metrics.cpu_temp);
    lv_obj_set_style_text_color(g_card_temps[0], temp_color(g_metrics.cpu_temp), LV_PART_MAIN);

    // ── Card 1: RAM ──
    lv_bar_set_value(g_card_bars[1], (int)g_metrics.ram_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[1], bar_color(g_metrics.ram_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[1], "%.0f%%", g_metrics.ram_percent);
    {
        char buf[96];
        int pos = 0;
        if (g_metrics.ram_total_gb > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.1f / %.1f GB",
                            g_metrics.ram_used_gb, g_metrics.ram_total_gb);
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.0f%% used", g_metrics.ram_percent);
        if (g_metrics.dimm_temp_max > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\nDIMM: %.0f/%.0f C",
                            g_metrics.dimm_temp_min, g_metrics.dimm_temp_max);
        lv_label_set_text(g_card_subs[1], buf);
    }

    // ── Card 2: GPU ──
    lv_bar_set_value(g_card_bars[2], (int)g_metrics.gpu_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[2], bar_color(g_metrics.gpu_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[2], "%.0f%%", g_metrics.gpu_percent);
    lv_label_set_text_fmt(g_card_temps[2], "%.0f C", g_metrics.gpu_temp);
    lv_obj_set_style_text_color(g_card_temps[2], temp_color(g_metrics.gpu_temp), LV_PART_MAIN);

    {
        char buf[128];
        int pos = 0;
        if (g_metrics.gpu_mem_freq > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.0f%%   %.0f/%.0f MHz",
                            g_metrics.gpu_percent, g_metrics.gpu_freq, g_metrics.gpu_mem_freq);
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.0f%%   %.0f MHz",
                            g_metrics.gpu_percent, g_metrics.gpu_freq);
        if (g_metrics.gpu_power > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "  Pkg:%.0f W",
                            g_metrics.gpu_power);
        if (g_metrics.gpu_hotspot_temp > 0 || g_metrics.gpu_mem_temp > 0) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\nHS:%.0f  M:%.0f C",
                            g_metrics.gpu_hotspot_temp, g_metrics.gpu_mem_temp);
        } else {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n%.0f C",
                            g_metrics.gpu_temp);
        }
        if (g_metrics.vram_total_gb > 0) {
            int rem = (int)sizeof(buf) - pos;
            int n = snprintf(buf + pos, rem, "\n%.1f/%.1f GB  %.0f%%",
                             g_metrics.vram_used_gb, g_metrics.vram_total_gb,
                             g_metrics.vram_percent);
            pos += n; rem -= n;
            if (g_metrics.gpu_fan > 0 && rem > 20)
                pos += snprintf(buf + pos, rem, "   Fan:%.0f", g_metrics.gpu_fan);
        } else if (g_metrics.gpu_fan > 0) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\nFan: %.0f RPM", g_metrics.gpu_fan);
        }
        lv_label_set_text(g_card_subs[2], buf);
    }

    if (g_card_bars2[2]) {
        lv_bar_set_value(g_card_bars2[2], (int)g_metrics.vram_percent, LV_ANIM_ON);
        lv_obj_set_style_bg_color(g_card_bars2[2], bar_color(g_metrics.vram_percent), LV_PART_INDICATOR);
        if (g_vram_label) {
            lv_label_set_text_fmt(g_vram_label, "VRAM %.0f%%", g_metrics.vram_percent);
        }
    }

    // ── Card 3: System ──
    {
        if (g_metrics.fps > 0) {
            lv_label_set_text_fmt(g_card_temps[3], "%.0f FPS", g_metrics.fps);
            lv_obj_set_style_text_color(g_card_temps[3], lv_color_hex(0xAAAAAA), LV_PART_MAIN);
        } else {
            lv_label_set_text(g_card_temps[3], "");
        }
        float disk = g_metrics.disk_percent;
        bool has_disk = (disk > 0);
        if (has_disk) {
            lv_obj_remove_flag(g_card_bars[3], LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(g_bar_labels[3], LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(g_card_bars[3], (int)disk, LV_ANIM_ON);
            lv_obj_set_style_bg_color(g_card_bars[3], bar_color(disk), LV_PART_INDICATOR);
            lv_label_set_text_fmt(g_bar_labels[3], "%.0f%%", disk);
        } else {
            lv_obj_add_flag(g_card_bars[3], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_bar_labels[3], LV_OBJ_FLAG_HIDDEN);
        }
        char buf[128];
        int pos = 0;
        if (g_metrics.mobo_temp > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "MoBo: %.0f C\n", g_metrics.mobo_temp);
        if (g_metrics.pch_temp > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "PCH: %.0f C\n", g_metrics.pch_temp);
        if (g_metrics.vrm_temp > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "VRM: %.0f C\n", g_metrics.vrm_temp);
        if (g_metrics.ssd_temp > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "SSD: %.0f C\n", g_metrics.ssd_temp);
        if (g_metrics.sys_fan_max > 0) {
            if (g_metrics.sys_fan_min < g_metrics.sys_fan_max)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "Fans: %.0f~%.0f RPM", g_metrics.sys_fan_min, g_metrics.sys_fan_max);
            else
                pos += snprintf(buf + pos, sizeof(buf) - pos, "Fan: %.0f RPM", g_metrics.sys_fan_max);
            if (g_metrics.pump_fan > 0)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "  Pump:%.0f", g_metrics.pump_fan);
            pos += snprintf(buf + pos, sizeof(buf) - pos, "\n");
        }
        {
            float total_w = g_metrics.cpu_power + g_metrics.gpu_power;
            if (total_w > 0)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "Total: %.0f W\n", total_w);
        }
        if (disk > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, "Disk: %.0f%%", disk);
        if (pos == 0)
            snprintf(buf, sizeof(buf), "No data");
        lv_label_set_text(g_card_subs[3], buf);
    }

    if (g_ip_addr.length() > 0) {
        lv_label_set_text_fmt(g_header_ip, "IP: %s", g_ip_addr.c_str());
    }

    g_metrics_updated = false;
}

void show_ota_screen() {
    ota_ui();
}

void update_ota_progress(int pct, const char* msg) {
    (void)pct;
    (void)msg;
}