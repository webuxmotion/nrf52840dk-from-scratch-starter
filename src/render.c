#include "render.h"
#include <zephyr/display/cfb.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { float x; float y; } Point2D;
typedef struct { int16_t x; int16_t y; float radius; float z; bool visible; } RenderObject;

static Point3D points[POINTS_COUNT];
static RenderObject objects_to_draw[POINTS_COUNT];

void generate_points(void) {
	float half_w = 200.0f;
	float half_h = 150.0f;
	float z_spacing = 120.0f;

	for (int s = 0; s < TUNNEL_SECTIONS; s++) {
		int idx = s * 4;
		float current_z = (float)s * z_spacing - 300.0f;
		
		points[idx]   = (Point3D){-half_w, -half_h, current_z};
		points[idx+1] = (Point3D){ half_w, -half_h, current_z};
		points[idx+2] = (Point3D){ half_w,  half_h, current_z};
		points[idx+3] = (Point3D){-half_w,  half_h, current_z};
	}
}

static inline Point2D closest_point_on_line(float x1, float z1, float x2, float z2, float px, float pz, float *out_t) {
	float dx = x2 - x1, dz = z2 - z1;
	float t = ((px - x1) * dx + (pz - z1) * dz) / (dx * dx + dz * dz);
	*out_t = t;
	return (Point2D){ x1 + t * dx, z1 + t * dz };
}

static inline bool is_point_on_right_side(float x1, float z1, float x2, float z2, float px, float pz) {
	return ((x2 - x1) * (pz - z1) - (z2 - z1) * (px - x1)) <= 0;
}

static void draw_ball_optimized(const struct device *display, int center_x, int center_y, float radius, uint16_t max_w, uint16_t max_h) {
	int r = (int)lroundf(radius);
	if (r <= 2) {
		/* ОПТИМІЗАЦІЯ: Для маленьких/дальніх об'єктів малюємо швидку точку, без cosf/sinf */
		struct cfb_position p = { (uint16_t)center_x, (uint16_t)center_y };
		if (center_x >= 0 && center_x < max_w && center_y >= 0 && center_y < max_h) {
			cfb_draw_line(display, &p, &p);
		}
		return;
	}

	/* ОПТИМІЗАЦІЯ: Чим менше коло, тим менше сторін малюємо */
	int sides = r > 12 ? 10 : 6;
	float angle_step = (M_PI * 2.0f) / (float)sides;

	for (int i = 0; i < sides; i++) {
		float x1 = cosf(angle_step * i) * r + center_x;
		float y1 = sinf(angle_step * i) * r + center_y;
		float x2 = cosf(angle_step * (i + 1)) * r + center_x;
		float y2 = sinf(angle_step * (i + 1)) * r + center_y;

		struct cfb_position p1 = { (uint16_t)lroundf(x1), (uint16_t)lroundf(y1) };
		struct cfb_position p2 = { (uint16_t)lroundf(x2), (uint16_t)lroundf(y2) };
		cfb_draw_line(display, &p1, &p2);
	}
}

void render_frame(const struct device *display, Point3D *camera, float camAngle, uint16_t width, uint16_t height) {
	float fl = 150.0f;
	float vpX = (float)width / 2.0f;
	float vpY = (float)height / 2.0f;

	/* ОПТИМІЗАЦІЯ: Розраховуємо тригонометрію камери ОДИН раз на кадр, а не для кожної точки! */
	float cos_angle_p = cosf(camAngle + M_PI / 2.0f) * 250.0f;
	float sin_angle_p = sinf(camAngle + M_PI / 2.0f) * 250.0f;
	float cos_angle_m = cosf(camAngle - M_PI / 2.0f) * 250.0f;
	float sin_angle_m = sinf(camAngle - M_PI / 2.0f) * 250.0f;

	float camera_p1_x = camera->x + cos_angle_p;
	float camera_p1_z = camera->z + sin_angle_p;
	float camera_p2_x = camera->x + cos_angle_m;
	float camera_p2_z = camera->z + sin_angle_m;

	for (int i = 0; i < POINTS_COUNT; i++) {
		objects_to_draw[i].visible = false;
		
		if (is_point_on_right_side(camera_p1_x, camera_p1_z, camera_p2_x, camera_p2_z, points[i].x, points[i].z)) {
			float t = 0;
			Point2D closest = closest_point_on_line(camera_p1_x, camera_p1_z, camera_p2_x, camera_p2_z, points[i].x, points[i].z, &t);

			float dx = camera->x - closest.x;
			float dy = camera->z - closest.y;
			float distance = (t > 0.5f) ? sqrtf(dx * dx + dy * dy) : -sqrtf(dx * dx + dy * dy);

			float dx2 = points[i].x - closest.x;
			float dy2 = points[i].z - closest.y;
			float distanceForZ = sqrtf(dx2 * dx2 + dy2 * dy2);

			if (distanceForZ < 1.0f) distanceForZ = 1.0f;
			float scale = fl / (fl + distanceForZ);

			objects_to_draw[i].z = distanceForZ;
			objects_to_draw[i].radius = 80.0f * scale;
			objects_to_draw[i].x = (int16_t)lroundf(vpX + distance * scale);
			objects_to_draw[i].y = (int16_t)lroundf(vpY + (points[i].y + camera->y) * scale);
			objects_to_draw[i].visible = true;
		}
	}

	/* Рендеринг від далеких до близьких */
	for (int s = TUNNEL_SECTIONS - 1; s >= 0; s--) {
		int idx = s * 4;

		if (objects_to_draw[idx].visible && objects_to_draw[idx+1].visible &&
			objects_to_draw[idx+2].visible && objects_to_draw[idx+3].visible) {
			struct cfb_position p3 = { objects_to_draw[idx+2].x, objects_to_draw[idx+2].y };
			struct cfb_position p4 = { objects_to_draw[idx+3].x, objects_to_draw[idx+3].y };
			cfb_draw_line(display, &p3, &p4); /* Малюємо лише нижню рейку */
		}

		for (int c = 0; c < 4; c++) {
			if (objects_to_draw[idx + c].visible) {
				draw_ball_optimized(display, objects_to_draw[idx + c].x, objects_to_draw[idx + c].y, objects_to_draw[idx + c].radius, width, height);
			}
		}
	}
}
