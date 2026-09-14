#ifndef CONTROLS_H
#define CONTROLS_H

#include <zephyr/drivers/gpio.h>

#define ROTATE_SPEED          0.1f

void init_controls(void);
void get_controls_snapshot(float *rotate_speed);

#endif
