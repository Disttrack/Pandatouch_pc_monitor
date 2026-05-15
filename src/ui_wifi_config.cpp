#include "ui_wifi_config.h"
#include "ui_monitor.h"
#include "storage.h"
#include <WiFi.h>

static lv_obj_t* g_wifi_screen = nullptr;
static lv_obj_t* g_wifi_kb = nullptr;
static lv_obj_t* g_wifi_ssid_ta = nullptr;
static lv_obj_t* g_wifi_pass_ta = nullptr;
static lv_obj_t* g_wifi_pcip_ta = nullptr;

static void back_btn_cb(lv_event_t* e) {
    (void)e;
    create_monitor_ui();
}

static void ta_event_cb(lv_event_t* e) {
    lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
    if (g_wifi_kb) lv_keyboard_set_textarea(g_wifi_kb, ta);
}

static void save_wifi_cb(lv_event_t* e) {
    (void)e;

    strncpy(g_wifi_ssid, lv_textarea_get_text(g_wifi_ssid_ta), 31);
    strncpy(g_wifi_pass, lv_textarea_get_text(g_wifi_pass_ta), 63);
    g_pc_ip = lv_textarea_get_text(g_wifi_pcip_ta);

    save_settings();

    Serial.printf("WiFi: SSID=\"%s\" PASS=\"%s\"\n", g_wifi_ssid, g_wifi_pass);
    WiFi.disconnect();
    WiFi.begin(g_wifi_ssid, g_wifi_pass);
    Serial.printf("WiFi: Connecting to %s...\n", g_wifi_ssid);

    create_monitor_ui();
}

void create_wifi_ui() {
    if (!g_wifi_screen) {
        g_wifi_screen = lv_obj_create(NULL);
    }
    lv_obj_clean(g_wifi_screen);

    lv_obj_set_style_bg_color(g_wifi_screen, lv_color_hex(0x0A0A0A), LV_PART_MAIN);

    g_wifi_kb = lv_keyboard_create(g_wifi_screen);
    lv_obj_set_size(g_wifi_kb, 800, 200);
    lv_obj_align(g_wifi_kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    g_wifi_ssid_ta = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_ssid_ta, true);
    lv_textarea_set_placeholder_text(g_wifi_ssid_ta, "SSID");
    lv_obj_set_size(g_wifi_ssid_ta, 360, 50);
    lv_obj_align(g_wifi_ssid_ta, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_text_font(g_wifi_ssid_ta, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_keyboard_set_textarea(g_wifi_kb, g_wifi_ssid_ta);
    lv_obj_add_event_cb(g_wifi_ssid_ta, ta_event_cb, LV_EVENT_FOCUSED, NULL);

    g_wifi_pass_ta = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_pass_ta, true);
    lv_textarea_set_placeholder_text(g_wifi_pass_ta, "Password");
    lv_textarea_set_password_mode(g_wifi_pass_ta, true);
    lv_obj_set_size(g_wifi_pass_ta, 360, 50);
    lv_obj_align(g_wifi_pass_ta, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_text_font(g_wifi_pass_ta, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_add_event_cb(g_wifi_pass_ta, ta_event_cb, LV_EVENT_FOCUSED, NULL);

    g_wifi_pcip_ta = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_pcip_ta, true);
    lv_textarea_set_placeholder_text(g_wifi_pcip_ta, "PC IP (LHM)");
    if (g_pc_ip.length() > 0) lv_textarea_set_text(g_wifi_pcip_ta, g_pc_ip.c_str());
    lv_obj_set_size(g_wifi_pcip_ta, 760, 50);
    lv_obj_align(g_wifi_pcip_ta, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_text_font(g_wifi_pcip_ta, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_add_event_cb(g_wifi_pcip_ta, ta_event_cb, LV_EVENT_FOCUSED, NULL);

    lv_obj_t* save_btn = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(save_btn, 160, 50);
    lv_obj_align(save_btn, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_set_style_radius(save_btn, 8, LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(save_btn);
    lv_label_set_text(label, "Save & Connect");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(label);

    lv_obj_add_event_cb(save_btn, save_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_btn = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 160);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 6, LV_PART_MAIN);

    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(g_wifi_screen);
}