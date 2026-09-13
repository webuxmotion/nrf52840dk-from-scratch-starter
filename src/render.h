#ifndef RENDER_H
#define RENDER_H

#include <zephyr/device.h>

#define TUNNEL_SECTIONS 8
#define POINTS_COUNT (TUNNEL_SECTIONS * 4)

typedef struct { float x; float y; float z; } Point3D;

void generate_points(void);
void render_frame(const struct device *display, Point3D *camera, float camAngle, uint16_t width, uint16_t height);

#endif
