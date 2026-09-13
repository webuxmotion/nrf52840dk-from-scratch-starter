#ifndef CONTROLS_H
#define CONTROLS_H

#include <zephyr/drivers/gpio.h>

/* Налаштування швидкостей */
#define FORWARD_THRUST_SPEED   -0.8f
#define TURN_SPEED              0.08f
#define VERTICAL_SPEED          9.0f

void init_controls(void);
void get_controls_snapshot(float *out_vr, float *out_thrust, float *out_vy);

#endif
