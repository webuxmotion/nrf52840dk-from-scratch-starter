#ifndef CONTROLS_H
#define CONTROLS_H

#include <zephyr/drivers/gpio.h>

void init_controls(void);
void get_controls_snapshot(bool *out_radius_minus, bool *out_radius_plus, bool *out_regenerate, bool *out_toggle_ui);

#endif
