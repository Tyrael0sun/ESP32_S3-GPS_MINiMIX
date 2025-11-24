#ifndef UI_H
#define UI_H

#include <stdbool.h>

void ui_init_display(void);

void ui_update_header(float bat_v);
void ui_update_gps(bool fix, int sats, float lat, float lon, float spd, float alt);
void ui_update_imu(float gx, float gy, float gz, float ax, float ay, float az, float lx, float ly, float lz, float temp);
void ui_update_mag(float x, float y, float z, float head, float temp);
void ui_update_baro(float press, float alt, float temp);
void ui_update_input(const char *evt);

#endif // UI_H
