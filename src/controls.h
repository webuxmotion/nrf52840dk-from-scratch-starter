#ifndef CONTROLS_H
#define CONTROLS_H

#include <zephyr/drivers/gpio.h>

void init_controls(void);
void get_controls_snapshot(bool *out_radius_click, bool *out_move_y, bool *out_next_point, bool *out_move_x, 
                           bool *out_dir_y, bool *out_dir_x);

#endif
