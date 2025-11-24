#include "ui.h"
#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *label_gnss;
static lv_obj_t *label_imu;
static lv_obj_t *label_env;
static lv_obj_t *label_input;

void ui_init_display(void) {
    lv_obj_t *scr = lv_scr_act();

    label_gnss = lv_label_create(scr);
    lv_label_set_text(label_gnss, "GNSS: Wait...");
    lv_obj_align(label_gnss, LV_ALIGN_TOP_LEFT, 10, 10);

    label_imu = lv_label_create(scr);
    lv_label_set_text(label_imu, "IMU: Wait...");
    lv_obj_align(label_imu, LV_ALIGN_LEFT_MID, 10, -40);

    label_env = lv_label_create(scr);
    lv_label_set_text(label_env, "ENV: Wait...");
    lv_obj_align(label_env, LV_ALIGN_LEFT_MID, 10, 20);

    label_input = lv_label_create(scr);
    lv_label_set_text(label_input, "Input: Idle");
    lv_obj_align(label_input, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}

void ui_update_sensors(float speed, float alt, float heading, float press, float bat) {
    if (label_gnss) {
        lv_label_set_text_fmt(label_gnss, "Spd: %.1f km/h\nHdg: %.1f", speed, heading);
    }
    if (label_env) {
        lv_label_set_text_fmt(label_env, "Alt: %.1fm\nPre: %.1fhPa\nBat: %.2fV", alt, press, bat/1000.0f);
    }
}

void ui_update_input_status(const char *msg) {
    if (label_input) {
        lv_label_set_text_fmt(label_input, "Input: %s", msg);
    }
}
