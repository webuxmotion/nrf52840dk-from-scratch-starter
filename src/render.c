#include "render.h"
#include <zephyr/display/cfb.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
	float base_x;
	float base_y;
} BasePoint;

typedef struct {
	float x;
	float y;
} RuntimePoint;

typedef struct {
	float range;
	float angle;
	float speed;
} AnimData;

static BasePoint points[POINTS_NUMBER];
static RuntimePoint new_points[NEW_POINTS_COUNT];
static AnimData points_animation[POINTS_NUMBER];

void generate_points(uint16_t width, uint16_t height) {
	float center_x = (float)width / 2.0f;
	float center_y = (float)height / 2.0f;
	float step_angle = (M_PI * 2.0f) / POINTS_NUMBER;

	for (int i = 0; i < POINTS_NUMBER; i++) {
		/* Генеруємо базові напрямні вектори по колу (радіус буде додано динамічно) */
		float angle = (float)i * step_angle + (M_PI / 4.0f);
		points[i].base_x = cosf(angle);
		points[i].base_y = sinf(angle);

		/* Налаштування індивідуальної пульсації кутів */
		points_animation[i].range = ((float)rand() / (float)RAND_MAX) * 12.0f + 3.0f; // амплітуда
		points_animation[i].angle = ((float)rand() / (float)RAND_MAX) * 6.28f;
		points_animation[i].speed = ((float)rand() / (float)RAND_MAX) * 2.5f + 1.0f; // швидкість
	}
}

/* Малювання квадратичного Безьє на основі кроків */
static void draw_quadratic_bezier(const struct device *display, float x0, float y0, float x1, float y1, float x2, float y2) {
	const int steps = 6; /* Оптимізовано для малих екранів */
	float prev_x = x0;
	float prev_y = y0;

	for (int i = 1; i <= steps; i++) {
		float t = (float)i / (float)steps;
		float mt = 1.0f - t;

		float f_x = (mt * mt * x0) + (2.0f * mt * t * x1) + (t * t * x2);
		float f_y = (mt * mt * y0) + (2.0f * mt * t * y1) + (t * t * y2);

		struct cfb_position p1 = { (uint16_t)lroundf(prev_x), (uint16_t)lroundf(prev_y) };
		struct cfb_position p2 = { (uint16_t)lroundf(f_x), (uint16_t)lroundf(f_y) };
		cfb_draw_line(display, &p1, &p2);

		prev_x = f_x;
		prev_y = f_y;
	}
}

void update_and_render_blob(const struct device *display, float delta_time, float current_radius, bool show_nodes, uint16_t width, uint16_t height) {
	float center_x = (float)width / 2.0f;
	float center_y = (float)height / 2.0f;

	/* 1. Оновлюємо фази кутів анімації */
	for (int i = 0; i < POINTS_NUMBER; i++) {
		points_animation[i].angle += points_animation[i].speed * delta_time;
	}

	/* 2. Розраховуємо поточні координати головних точок (парні індекси) */
	for (int i = 0; i < POINTS_NUMBER; i++) {
		int runtime_idx = i * 2;
		
		/* Динамічний радіус з урахуванням коливання */
		float wave = sinf(points_animation[i].angle) * points_animation[i].range;
		float final_radius = current_radius + wave;

		new_points[runtime_idx].x = center_x + points[i].base_x * final_radius;
		new_points[runtime_idx].y = center_y + points[i].base_y * final_radius;
	}

	/* 3. Розраховуємо проміжні серединні точки (Midpoints на непарних індексах) */
	for (int i = 0; i < NEW_POINTS_COUNT; i += 2) {
		int next_idx = (i + 2) % NEW_POINTS_COUNT; // Кільцеве замикання
		int mid_idx = i + 1;

		new_points[mid_idx].x = (new_points[i].x + new_points[next_idx].x) * 0.5f;
		new_points[mid_idx].y = (new_points[i].y + new_points[next_idx].y) * 0.5f;
	}

	/* 4. Малюємо плавний замкнений контур плями Безьє */
	/* Трійки: Midpoint[i-1] -> AnchorPoint[i] -> Midpoint[i+1] */
	for (int i = 0; i < NEW_POINTS_COUNT; i += 2) {
		int prev_mid_idx = (i == 0) ? (NEW_POINTS_COUNT - 1) : (i - 1);
		int next_mid_idx = i + 1;

		draw_quadratic_bezier(display,
			new_points[prev_mid_idx].x, new_points[prev_mid_idx].y,
			new_points[i].x,            new_points[i].y,
			new_points[next_mid_idx].x, new_points[next_mid_idx].y
		);
	}

	/* 5. Якщо увімкнено режим вузлів, малюємо точки */
	if (show_nodes) {
		for (int i = 0; i < NEW_POINTS_COUNT; i++) {
			struct cfb_position p = { (uint16_t)lroundf(new_points[i].x), (uint16_t)lroundf(new_points[i].y) };
			// Перевірка меж екрана перед малюванням точки
			if (p.x < width && p.y < height) {
				cfb_draw_line(display, &p, &p);
			}
		}
	}
}
