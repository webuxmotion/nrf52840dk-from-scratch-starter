#include "render.h"
#include <zephyr/display/cfb.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Vector2D A, B, C;
static Vector2D arcPoints[SEGMENTS_COUNT + 1];

void init_geometry_points(uint16_t width, uint16_t height) {
	/* Зміщуємо всю композицію вліво (множники зменшено, щоб посунути до лівого краю) */
	A = (Vector2D){ (float)width * 0.45f, (float)height * 0.20f };
	B = (Vector2D){ (float)width * 0.12f, (float)height * 0.50f };
	C = (Vector2D){ (float)width * 0.45f, (float)height * 0.80f };
}

void move_geometry_point(int index, float dx, float dy) {
	if (index == 0) { A.x += dx; A.y += dy; }
	else if (index == 1) { B.x += dx; B.y += dy; }
	else if (index == 2) { C.x += dx; C.y += dy; }
}

/* Функція малювання штрихової/пунктирної лінії точками */
static void draw_dashed_line(const struct device *display, float x0, float y0, float x1, float y1) {
	float dx = x1 - x0;
	float dy = y1 - y0;
	float len = hypotf(dx, dy);
	if (len < 1.0f) return;

	float step_x = dx / len;
	float step_y = dy / len;

	/* Малюємо точки з кроком у 4 пікселі для ефекту штриху */
	for (float d = 0; d <= len; d += 4.0f) {
		struct cfb_position p = { 
			(uint16_t)lroundf(x0 + step_x * d), 
			(uint16_t)lroundf(y0 + step_y * d) 
		};
		cfb_draw_line(display, &p, &p);
	}
}

static void draw_point_node(const struct device *display, int cx, int cy, int r, bool is_selected) {
	struct cfb_position center = { (uint16_t)cx, (uint16_t)cy };
	/* Замість cfb_draw_circle використовуємо 4-точковий маркер для швидкості */
	struct cfb_position p1 = { (uint16_t)(cx - r), (uint16_t)cy };
	struct cfb_position p2 = { (uint16_t)(cx + r), (uint16_t)cy };
	struct cfb_position p3 = { (uint16_t)cx, (uint16_t)(cy - r) };
	struct cfb_position p4 = { (uint16_t)cx, (uint16_t)(cy + r) };
	cfb_draw_line(display, &p1, &p2);
	cfb_draw_line(display, &p3, &p4);

	if (is_selected) {
		/* Для вибраної точки малюємо додаткову рамку навколо */
		struct cfb_position hl1 = { (uint16_t)(cx - r - 2), (uint16_t)(cy - r - 2) };
		struct cfb_position hl2 = { (uint16_t)(cx + r + 2), (uint16_t)(cy - r - 2) };
		struct cfb_position vl1 = { (uint16_t)(cx - r - 2), (uint16_t)(cy + r + 2) };
		struct cfb_position vl2 = { (uint16_t)(cx + r + 2), (uint16_t)(cy + r + 2) };
		cfb_draw_line(display, &hl1, &hl2);
		cfb_draw_line(display, &vl1, &vl2);
	}
}

void do_math_and_render(const struct device *display, float border_radius, int selected_idx, uint16_t width, uint16_t height) {
	float vAx = A.x - B.x;
	float vAy = A.y - B.y;
	float vCx = C.x - B.x;
	float vCy = C.y - B.y;

	float lenA = hypotf(vAx, vAy);
	float lenC = hypotf(vCx, vCy);

	if (lenA < 0.1f || lenC < 0.1f) return;

	float nAx = vAx / lenA;
	float nAy = vAy / lenA;
	float nCx = vCx / lenC;
	float nCy = vCy / lenC;

	float bisX = nAx + nCx;
	float bisY = nAy + nCy;
	float lenBis = hypotf(bisX, bisY);

	if (lenBis < 0.001f) {
		bisX = -nAy;
		bisY = nAx;
		lenBis = 1.0f;
	}

	float nBisX = bisX / lenBis;
	float nBisY = bisY / lenBis;

	float dot = nAx * nCx + nAy * nCy;
	if (dot > 1.0f) dot = 1.0f;
	if (dot < -1.0f) dot = -1.0f;

	float halfAngleCos = sqrtf((1.0f + dot) / 2.0f);
	float halfAngleSin = sqrtf((1.0f - dot) / 2.0f);
	if (halfAngleCos < 0.001f) halfAngleCos = 0.001f;
	float halfAngleTan = halfAngleSin / halfAngleCos;

	float BF = border_radius / halfAngleTan;
	float BP = border_radius / halfAngleSin;

	Vector2D F = { B.x + nAx * BF, B.y + nAy * BF };
	Vector2D G = { B.x + nCx * BF, B.y + nCy * BF };
	Vector2D P = { B.x + nBisX * BP, B.y + nBisY * BP };

	float anglePG = atan2f(G.y - P.y, G.x - P.x);
	float anglePF = atan2f(F.y - P.y, F.x - P.x);

	float arcDiff = anglePF - anglePG;
	if (arcDiff < -M_PI) arcDiff += M_PI * 2.0f;
	if (arcDiff > M_PI)  arcDiff -= M_PI * 2.0f;

	float stepAngle = arcDiff / (float)SEGMENTS_COUNT;

	/* 1. Обчислюємо точки дуги за допомогою sinf/cosf (Тільки основна лінія заокруглення!) */
	for (int i = 0; i <= SEGMENTS_COUNT; i++) {
		float currentAngle = anglePG + stepAngle * (float)i;
		arcPoints[i].x = P.x + cosf(currentAngle) * border_radius;
		arcPoints[i].y = P.y + sinf(currentAngle) * border_radius;
	}

	/* === ВІДМАЛЬОВУВАННЯ СЦЕНИ === */

	// 2. Напрямні лінії каркаса малюємо точками (штрихом)
	draw_dashed_line(display, A.x, A.y, B.x, B.y);
	draw_dashed_line(display, B.x, B.y, C.x, C.y);

	// 3. Малюємо тільки основну лінію заокруглення через прораховані через sin/cos точки дуги
	for (int i = 0; i < SEGMENTS_COUNT; i++) {
		struct cfb_position p1 = { (uint16_t)lroundf(arcPoints[i].x),   (uint16_t)lroundf(arcPoints[i].y) };
		struct cfb_position p2 = { (uint16_t)lroundf(arcPoints[i+1].x), (uint16_t)lroundf(arcPoints[i+1].y) };
		cfb_draw_line(display, &p1, &p2);
	}

	// 4. Промені від точок дотику (F та G), що уходять далеко вперед за межі екрана (нескінченність)
	float infinity_factor = 500.0f; // Велика дистанція для гарантованого виходу за екран
	struct cfb_position pF_start = { (uint16_t)lroundf(F.x), (uint16_t)lroundf(F.y) };
	struct cfb_position pF_end   = { (uint16_t)lroundf(F.x + nAx * infinity_factor), (uint16_t)lroundf(F.y + nAy * infinity_factor) };
	cfb_draw_line(display, &pF_start, &pF_end);

	struct cfb_position pG_start = { (uint16_t)lroundf(G.x), (uint16_t)lroundf(G.y) };
	struct cfb_position pG_end   = { (uint16_t)lroundf(G.x + nCx * infinity_factor), (uint16_t)lroundf(G.y + nCy * infinity_factor) };
	cfb_draw_line(display, &pG_start, &pG_end);

	// 5. Малюємо опорні маркери точок дотику F, G та центру P
	draw_point_node(display, (int)F.x, (int)F.y, 2, false);
	draw_point_node(display, (int)G.x, (int)G.y, 2, false);
	draw_point_node(display, (int)P.x, (int)P.y, 2, false);

	// 6. Малюємо головні точки A, B, C (вибрана підсвічується)
	draw_point_node(display, (int)A.x, (int)A.y, 4, (selected_idx == 0));
	draw_point_node(display, (int)B.x, (int)B.y, 4, (selected_idx == 1));
	draw_point_node(display, (int)C.x, (int)C.y, 4, (selected_idx == 2));
}
