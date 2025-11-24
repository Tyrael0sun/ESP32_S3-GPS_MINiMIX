#include "ui.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "UI";

// Labels for sections
static lv_obj_t *lbl_header;
static lv_obj_t *lbl_gps_title;
static lv_obj_t *lbl_gps_data;
static lv_obj_t *lbl_imu_title;
static lv_obj_t *lbl_imu_data;
static lv_obj_t *lbl_mag_title;
static lv_obj_t *lbl_mag_data;
static lv_obj_t *lbl_baro_title;
static lv_obj_t *lbl_baro_data;
// static lv_obj_t *lbl_input_title; // Unused
static lv_obj_t *lbl_input_data;

static lv_style_t style_title;
static lv_style_t style_text;

void ui_init_display(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    // Styles
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_white());
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_make(0x00, 0xFF, 0xFF));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_14);

    // 1. Header
    lbl_header = lv_label_create(scr);
    lv_obj_add_style(lbl_header, &style_text, 0);
    lv_label_set_text(lbl_header, "[00:00:00]          [BAT: --%]");
    lv_obj_align(lbl_header, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sep1 = lv_label_create(scr);
    lv_obj_add_style(sep1, &style_text, 0);
    lv_label_set_text(sep1, "--------------------------------------");
    lv_obj_align(sep1, LV_ALIGN_TOP_LEFT, 0, 15);

    // 2. GPS DATA
    lbl_gps_title = lv_label_create(scr);
    lv_obj_add_style(lbl_gps_title, &style_title, 0);
    lv_label_set_text(lbl_gps_title, "> GPS DATA");
    lv_obj_align(lbl_gps_title, LV_ALIGN_TOP_LEFT, 0, 30);

    lbl_gps_data = lv_label_create(scr);
    lv_obj_add_style(lbl_gps_data, &style_text, 0);
    lv_label_set_text(lbl_gps_data, "Fix: -- (Sats:0)\nLat: -- N   Lon: -- E\nSpd: 0.0 km/h   Alt: 0.0 m");
    lv_obj_align(lbl_gps_data, LV_ALIGN_TOP_LEFT, 0, 45);

    lv_obj_t *sep2 = lv_label_create(scr);
    lv_obj_add_style(sep2, &style_text, 0);
    lv_label_set_text(sep2, "--------------------------------------");
    lv_obj_align(sep2, LV_ALIGN_TOP_LEFT, 0, 90);

    // 3. IMU DATA
    lbl_imu_title = lv_label_create(scr);
    lv_obj_add_style(lbl_imu_title, &style_title, 0);
    lv_label_set_text(lbl_imu_title, "> IMU DATA");
    lv_obj_align(lbl_imu_title, LV_ALIGN_TOP_LEFT, 0, 105);

    lbl_imu_data = lv_label_create(scr);
    lv_obj_add_style(lbl_imu_data, &style_text, 0);
    lv_label_set_text(lbl_imu_data, "Grav(g): X:-- Y:-- Z:--\nGyr(d): X:-- Y:-- Z:--\nAcc(g): X:-- Y:-- Z:--\nTemp: -- C");
    lv_obj_align(lbl_imu_data, LV_ALIGN_TOP_LEFT, 0, 120);

    lv_obj_t *sep3 = lv_label_create(scr);
    lv_obj_add_style(sep3, &style_text, 0);
    lv_label_set_text(sep3, "--------------------------------------");
    lv_obj_align(sep3, LV_ALIGN_TOP_LEFT, 0, 180);

    // 4. MAG DATA
    lbl_mag_title = lv_label_create(scr);
    lv_obj_add_style(lbl_mag_title, &style_title, 0);
    lv_label_set_text(lbl_mag_title, "> MAG DATA");
    lv_obj_align(lbl_mag_title, LV_ALIGN_TOP_LEFT, 0, 195);

    lbl_mag_data = lv_label_create(scr);
    lv_obj_add_style(lbl_mag_data, &style_text, 0);
    lv_label_set_text(lbl_mag_data, "Mag(uT):X:-- Y:-- Z:--\nHeading: -- deg\nTemp: -- C");
    lv_obj_align(lbl_mag_data, LV_ALIGN_TOP_LEFT, 0, 210);

    lv_obj_t *sep4 = lv_label_create(scr);
    lv_obj_add_style(sep4, &style_text, 0);
    lv_label_set_text(sep4, "--------------------------------------");
    lv_obj_align(sep4, LV_ALIGN_TOP_LEFT, 0, 255);

    // 5. BARO DATA
    lbl_baro_title = lv_label_create(scr);
    lv_obj_add_style(lbl_baro_title, &style_title, 0);
    lv_label_set_text(lbl_baro_title, "> BARO");
    lv_obj_align(lbl_baro_title, LV_ALIGN_TOP_LEFT, 0, 270);

    lbl_baro_data = lv_label_create(scr);
    lv_obj_add_style(lbl_baro_data, &style_text, 0);
    lv_label_set_text(lbl_baro_data, "P: -- hPa A: -- m T: -- C");
    lv_obj_align(lbl_baro_data, LV_ALIGN_TOP_LEFT, 60, 270);

    // Footer
    lv_obj_t *sep5 = lv_label_create(scr);
    lv_obj_add_style(sep5, &style_text, 0);
    lv_label_set_text(sep5, "======================================");
    lv_obj_align(sep5, LV_ALIGN_BOTTOM_LEFT, 0, -30);

    lbl_input_data = lv_label_create(scr);
    lv_obj_add_style(lbl_input_data, &style_text, 0);
    lv_label_set_text(lbl_input_data, "Btn: [ ]S [ ]M [ ]L [ ]D\nRot: [ ]L [ ]R");
    lv_obj_align(lbl_input_data, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    ESP_LOGI(TAG, "UI Initialized");
}

void ui_update_header(float bat_v) {
    int64_t now = esp_timer_get_time() / 1000000;
    int h = (now / 3600) % 24;
    int m = (now / 60) % 60;
    int s = now % 60;
    int bat_pct = (int)((bat_v - 3000) / 12.0f);
    if (bat_pct < 0) bat_pct = 0;
    if (bat_pct > 100) bat_pct = 100;

    lv_label_set_text_fmt(lbl_header, "[%02d:%02d:%02d]          [BAT: %d%%]", h, m, s, bat_pct);
}

void ui_update_gps(bool fix, int sats, float lat, float lon, float spd, float alt) {
    if (fix) {
        lv_label_set_text_fmt(lbl_gps_data,
            "Fix: 3D (Sats:%d)\nLat: %.4f N   Lon: %.4f E\nSpd: %.1f km/h   Alt: %.1f m",
            sats, lat, lon, spd, alt);
    } else {
        lv_label_set_text_fmt(lbl_gps_data,
            "Fix: NO (Sats:%d)\nLat: --   Lon: --\nSpd: 0.0 km/h   Alt: 0.0 m",
            sats);
    }
}

void ui_update_imu(float gx, float gy, float gz, float ax, float ay, float az, float lx, float ly, float lz, float temp) {
    lv_label_set_text_fmt(lbl_imu_data,
        "Grav(g): X:%.2f  Y:%.2f  Z:%.2f\nGyr(d): X:%.1f   Y:%.1f   Z:%.1f\nAcc(g): X:%.2f   Y:%.2f   Z:%.2f\nTemp: %.1f C",
        gx, gy, gz,
        ax, ay, az,
        lx, ly, lz,
        temp);
}

void ui_update_mag(float x, float y, float z, float head, float temp) {
    lv_label_set_text_fmt(lbl_mag_data,
        "Mag(uT):X:%.1f   Y:%.1f  Z:%.1f\nHeading: %.1f deg\nTemp: %.1f C",
        x, y, z, head, temp);
}

void ui_update_baro(float press, float alt, float temp) {
    lv_label_set_text_fmt(lbl_baro_data, "P: %.1f hPa A: %.1f m T: %.1f C", press, alt, temp);
}

void ui_update_input(const char *evt) {
    // Buffer must be large enough for strcpy
    char s_s[8] = "[ ]", s_m[8] = "[ ]", s_l[8] = "[ ]", s_d[8] = "[ ]";
    char r_l[8] = "[ ]", r_r[8] = "[ ]";

    if (strstr(evt, "SHORT")) strcpy(s_s, "[X]");
    else if (strstr(evt, "MEDIUM")) strcpy(s_m, "[X]");
    else if (strstr(evt, "LONG")) strcpy(s_l, "[X]");
    else if (strstr(evt, "DOUBLE")) strcpy(s_d, "[X]");

    if (strstr(evt, "CCW")) strcpy(r_l, "[<-]");
    else if (strstr(evt, "CW")) strcpy(r_r, "[->]");

    lv_label_set_text_fmt(lbl_input_data,
        "Btn: %sS %sM %sL %sD\nRot: %sL %sR",
        s_s, s_m, s_l, s_d, r_l, r_r);
}
