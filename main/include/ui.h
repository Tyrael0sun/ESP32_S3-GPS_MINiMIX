#ifndef UI_H
#define UI_H

#include "esp_err.h"

void ui_init_display(void);
void ui_update_sensors(float speed, float alt, float heading, float press, float bat);
void ui_update_input_status(const char *msg);

#endif // UI_H
