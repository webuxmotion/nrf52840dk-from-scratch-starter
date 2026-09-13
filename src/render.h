#ifndef RENDER_H
#define RENDER_H

#include <zephyr/device.h>

#define SEGMENTS_COUNT 10

typedef struct {
	float x;
	float y;
} Vector2D;

void init_geometry_points(uint16_t width, uint16_t height);
void move_geometry_point(int index, float dx, float dy);
void do_math_and_render(const struct device *display, float border_radius, int selected_idx, uint16_t width, uint16_t height);

#endif
