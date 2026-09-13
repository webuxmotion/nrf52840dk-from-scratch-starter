#ifndef RENDER_H
#define RENDER_H

#include <zephyr/device.h>

#define POINTS_NUMBER 20
/* Замкнене коло: 20 базових точок + 20 проміжних серединних = 40 точок */
#define NEW_POINTS_COUNT ((POINTS_NUMBER) * 2)

void generate_points(uint16_t width, uint16_t height);
void update_and_render_blob(const struct device *display, float delta_time, float current_radius, bool show_nodes, uint16_t width, uint16_t height);

#endif
