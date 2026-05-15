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
static lv_obj_t* g_card_bars[4] = {nullptr};
static lv_obj_t* g_bar_labels[4] = {nullptr};
static lv_obj_t* g_main_cont = nullptr;

static lv_color_t bar_color(float pct) {
    if (pct < 50.0f) return lv_color_hex(0x4CAF50);
    if (pct < 80.0f) return lv_color_hex(0xFF9800);
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
    lv_obj_set_size(g_main_cont, 800, 435);
    lv_obj_set_pos(g_main_cont, 0, 45);
    lv_obj_set_style_bg_opa(g_main_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_main_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_main_cont, 20, LV_PART_MAIN);
    lv_obj_set_flex_flow(g_main_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g_main_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(g_main_cont, 20, LV_PART_MAIN);

    const char* titles[4] = {"CPU", "RAM", "GPU", "Temps"};
    const char* subs[4] = {
        "-- %\n-- MHz | -- C",
        "-- %\n-- / -- GB",
        "-- %\n-- C",
        "CPU: -- C\nGPU: -- C\nSSD: -- C\nMoBo: -- C"
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t* card = lv_obj_create(g_main_cont);
        lv_obj_set_size(card, 370, 187);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);

        lv_obj_t* title = lv_label_create(card);
        lv_label_set_text(title, titles[i]);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
        g_card_titles[i] = title;

        lv_obj_t* sub = lv_label_create(card);
        lv_label_set_text(sub, subs[i]);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(sub, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
        lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 0, 28);
        g_card_subs[i] = sub;

        lv_obj_t* bar = lv_bar_create(card);
        lv_obj_set_size(bar, 338, 16);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 0, -24);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 8, LV_PART_MAIN);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        g_card_bars[i] = bar;

        lv_obj_t* bl = lv_label_create(card);
        lv_label_set_text(bl, "0%");
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(bl, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        g_bar_labels[i] = bl;
    }

    g_header_ip = lv_label_create(g_monitor_screen);
    lv_label_set_text(g_header_ip, "IP: 0.0.0.0");
    lv_obj_set_style_text_font(g_header_ip, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_header_ip, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(g_header_ip, LV_ALIGN_TOP_LEFT, 20, 16);

    lv_obj_t* settings_btn = lv_btn_create(g_monitor_screen);
    lv_obj_set_size(settings_btn, 80, 35);
    lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
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

    lv_bar_set_value(g_card_bars[0], (int)g_metrics.cpu_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[0], bar_color(g_metrics.cpu_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[0], "%.0f%%", g_metrics.cpu_percent);
    lv_label_set_text_fmt(g_card_subs[0], "%.0f%% | %.0f MHz\n%.0f C",
                          g_metrics.cpu_percent, g_metrics.cpu_freq, g_metrics.cpu_temp);

    lv_bar_set_value(g_card_bars[1], (int)g_metrics.ram_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[1], bar_color(g_metrics.ram_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[1], "%.0f%%", g_metrics.ram_percent);
    lv_label_set_text_fmt(g_card_subs[1], "%.0f%% used", g_metrics.ram_percent);

    lv_bar_set_value(g_card_bars[2], (int)g_metrics.gpu_percent, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[2], bar_color(g_metrics.gpu_percent), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[2], "%.0f%%", g_metrics.gpu_percent);
    lv_label_set_text_fmt(g_card_subs[2], "%.0f C", g_metrics.gpu_temp);

    float avg_temp = (g_metrics.cpu_temp + g_metrics.gpu_temp + g_metrics.ssd_temp + g_metrics.mobo_temp) / 4.0f;
    lv_bar_set_value(g_card_bars[3], (int)avg_temp, LV_ANIM_ON);
    lv_obj_set_style_bg_color(g_card_bars[3], bar_color(avg_temp), LV_PART_INDICATOR);
    lv_label_set_text_fmt(g_bar_labels[3], "%.0f C", avg_temp);
    lv_label_set_text_fmt(g_card_subs[3],
        "CPU ........... %.0f C\nGPU ........... %.0f C\nSSD ........... %.0f C\nMoBo .......... %.0f C",
        g_metrics.cpu_temp, g_metrics.gpu_temp, g_metrics.ssd_temp, g_metrics.mobo_temp);

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
