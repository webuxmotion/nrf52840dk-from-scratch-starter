#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Змінні для розрахунку FPS */
static int current_fps = 0;
static int frame_count = 0;
static int32_t last_fps_time = 0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Кількість секцій (прямокутників) у тунелі */
#define TUNNEL_SECTIONS 8
/* Разом точок: 15 секцій * 4 кути = 60 точок */
#define POINTS_COUNT (TUNNEL_SECTIONS * 4)

/* --- НАЛАШТУВАННЯ ДИНАМІКИ ПОЛЬОТУ --- */
#define FORWARD_THRUST_SPEED   -0.8f  /* Швидкість розгону вперед (було -0.2f) */
#define TURN_SPEED              0.08f /* Швидкість повороту ліворуч/праворуч (було 0.03f) */
#define VERTICAL_SPEED          9.0f  /* Швидкість руху вгору/вниз (було 4.0f) */

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static const struct gpio_dt_spec btn_left   = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_right  = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_thrust = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_toggle = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

static struct gpio_callback cb_left;
static struct gpio_callback cb_right;
static struct gpio_callback cb_thrust;
static struct gpio_callback cb_toggle;

static volatile bool is_left_pressed = false;
static volatile bool is_right_pressed = false;
static volatile bool is_thrust_pressed = false;
static volatile bool is_toggle_pressed = false;

static volatile bool next_vertical_dir_is_top = true; 

K_SEM_DEFINE(display_sem, 0, 1);

void animation_timer_handler(struct k_timer *dummy)
{
	k_sem_give(&display_sem);
}
K_TIMER_DEFINE(anim_timer, animation_timer_handler, NULL);

void handler_left(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	is_left_pressed = gpio_pin_get_dt(&btn_left);
}

void handler_right(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	is_right_pressed = gpio_pin_get_dt(&btn_right);
}

void handler_thrust(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	is_thrust_pressed = gpio_pin_get_dt(&btn_thrust);
}

void handler_toggle(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	bool current_state = gpio_pin_get_dt(&btn_toggle);
	
	if (current_state && !is_toggle_pressed) {
		next_vertical_dir_is_top = !next_vertical_dir_is_top;
	}
	
	is_toggle_pressed = current_state;
}

typedef struct { float x; float y; float z; } Point3D;
typedef struct { float x; float y; } Point2D;

/* Зберігаємо розраховані 2D екранні координати для ліній прямокутника */
typedef struct { 
	int16_t x; 
	int16_t y; 
	float radius; 
	float z; 
	bool visible; 
} RenderObject;

static Point3D points[POINTS_COUNT];
static RenderObject objects_to_draw[POINTS_COUNT];

/* Створення тунелю з послідовних прямокутників */
void generate_points(void)
{
	float half_w = 200.0f; /* Напівширина тунелю */
	float half_h = 150.0f; /* Напіввисота тунелю */
	float z_spacing = 120.0f; /* Відстань між секціями по Z */

	for (int s = 0; s < TUNNEL_SECTIONS; s++) {
		int idx = s * 4;
		float current_z = (float)s * z_spacing - 300.0f;

		// Кут 1: Верхній лівий
		points[idx].x = -half_w;
		points[idx].y = -half_h;
		points[idx].z = current_z;

		// Кут 2: Верхній правий
		points[idx + 1].x = half_w;
		points[idx + 1].y = -half_h;
		points[idx + 1].z = current_z;

		// Кут 3: Нижній правий
		points[idx + 2].x = half_w;
		points[idx + 2].y = half_h;
		points[idx + 2].z = current_z;

		// Кут 4: Нижній лівий
		points[idx + 3].x = -half_w;
		points[idx + 3].y = half_h;
		points[idx + 3].z = current_z;
	}
}

Point2D closest_point_on_line(float x1, float z1, float x2, float z2, float px, float pz, float *out_t)
{
	float dx = x2 - x1;
	float dz = z2 - z1;
	float t = ((px - x1) * dx + (pz - z1) * dz) / (dx * dx + dz * dz);
	*out_t = t;
	Point2D res = { .x = x1 + t * dx, .y = z1 + t * dz };
	return res;
}

const char* point_side(float x1, float z1, float x2, float z2, float px, float pz)
{
	float val = (x2 - x1) * (pz - z1) - (z2 - z1) * (px - x1);
	return (val > 0) ? "left" : "right";
}

void draw_ball(int center_x, int center_y, float radius, uint16_t max_w, uint16_t max_h)
{
	int r = (int)lroundf(radius);
	if (r < 1) {
		struct cfb_position p = { (uint16_t)center_x, (uint16_t)center_y };
		if (center_x >= 0 && center_x < max_w && center_y >= 0 && center_y < max_h) {
			cfb_draw_line(display, &p, &p); 
		}
		return;
	}

	int sides = r > 10 ? 10 : 6; 
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

int main(void)
{
	if (!device_is_ready(display) || cfb_framebuffer_init(display)) {
		return -EIO;
	}

	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
	cfb_framebuffer_invert(display);

	if (device_is_ready(btn_left.port)) {
		gpio_pin_configure_dt(&btn_left, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_left, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_left, handler_left, BIT(btn_left.pin));
		gpio_add_callback(btn_left.port, &cb_left);
	}
	if (device_is_ready(btn_right.port)) {
		gpio_pin_configure_dt(&btn_right, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_right, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_right, handler_right, BIT(btn_right.pin));
		gpio_add_callback(btn_right.port, &cb_right);
	}
	if (device_is_ready(btn_thrust.port)) {
		gpio_pin_configure_dt(&btn_thrust, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_thrust, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_thrust, handler_thrust, BIT(btn_thrust.pin));
		gpio_add_callback(btn_thrust.port, &cb_thrust);
	}
	if (device_is_ready(btn_toggle.port)) {
		gpio_pin_configure_dt(&btn_toggle, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_toggle, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_toggle, handler_toggle, BIT(btn_toggle.pin));
		gpio_add_callback(btn_toggle.port, &cb_toggle);
	}

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	generate_points();

	float fl = 150.0f;
	float vpX = (float)width / 2.0f;
	float vpY = (float)height / 2.0f;

	float camAngle = M_PI + (M_PI / 2.0f);
	float vr = 0.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float vz = 0.0f;
	float thrust = 0.0f;

	Point3D camera = { .x = 0.0f, .y = 0.0f, .z = -900.0f };

	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(30));

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

    /* Button 1 & Button 2: Повороти з новою швидкістю TURN_SPEED */
    if ((!is_left_pressed && !is_right_pressed) || (is_left_pressed && is_right_pressed)) {
      vr = 0.0f;
    } else if (is_left_pressed) {
      vr = -TURN_SPEED;
    } else if (is_right_pressed) {
      vr = TURN_SPEED;
    }

    /* Button 3: Прискорення вперед */
    thrust = is_thrust_pressed ? FORWARD_THRUST_SPEED : 0.0f;

    /* Button 4: Рух вгору або вниз із новою швидкістю VERTICAL_SPEED */
    if (is_toggle_pressed) {
      vy = next_vertical_dir_is_top ? VERTICAL_SPEED : -VERTICAL_SPEED;
    } else {
      vy = 0.0f;
    }

    cfb_framebuffer_clear(display, false);

		Point2D camera_p1 = {
			.x = camera.x + cosf(camAngle + M_PI / 2.0f) * 250.0f,
			.y = camera.z + sinf(camAngle + M_PI / 2.0f) * 250.0f
		};
		Point2D camera_p2 = {
			.x = camera.x + cosf(camAngle - M_PI / 2.0f) * 250.0f,
			.y = camera.z + sinf(camAngle - M_PI / 2.0f) * 250.0f
		};

		/* Очищуємо стан видимості об'єктів для нового кадру */
		for (int i = 0; i < POINTS_COUNT; i++) {
			objects_to_draw[i].visible = false;
		}

		/* 1. Розраховуємо 2D позицію для кожної 3D точки */
		for (int i = 0; i < POINTS_COUNT; i++) {
			float t = 0;
			Point2D closest = closest_point_on_line(camera_p1.x, camera_p1.y, camera_p2.x, camera_p2.y, points[i].x, points[i].z, &t);
			const char* side = point_side(camera_p1.x, camera_p1.y, camera_p2.x, camera_p2.y, points[i].x, points[i].z);

			if (side != NULL && strcmp(side, "right") == 0) {
				float dx = camera.x - closest.x;
				float dy = camera.z - closest.y;

				float distance = sqrtf(dx * dx + dy * dy);
				distance = (t > 0.5f) ? distance : -distance;

				float dx2 = points[i].x - closest.x;
				float dy2 = points[i].z - closest.y;
				float distanceForZ = sqrtf(dx2 * dx2 + dy2 * dy2);

				if (distanceForZ < 1.0f) distanceForZ = 1.0f; 

				float scale = fl / (fl + distanceForZ);
				
				objects_to_draw[i].z = distanceForZ;
				objects_to_draw[i].radius = 80.0f * scale; /* Розмір кіл на кутах */
				objects_to_draw[i].x = (int16_t)lroundf(vpX + distance * scale);
				objects_to_draw[i].y = (int16_t)lroundf(vpY + (points[i].y + camera.y) * scale);
				objects_to_draw[i].visible = true;
			}
		}

		/* 2. Малюємо лінії прямокутників та кола на кутах секцій (від дальніх до ближніх) */
		for (int s = TUNNEL_SECTIONS - 1; s >= 0; s--) {
			int idx = s * 4;

			/* Перевіряємо, чи видимі всі 4 кути поточної секції прямокутника */
			if (objects_to_draw[idx].visible && objects_to_draw[idx+1].visible &&
				objects_to_draw[idx+2].visible && objects_to_draw[idx+3].visible) {

				struct cfb_position p1 = { objects_to_draw[idx].x,   objects_to_draw[idx].y };
				struct cfb_position p2 = { objects_to_draw[idx+1].x, objects_to_draw[idx+1].y };
				struct cfb_position p3 = { objects_to_draw[idx+2].x, objects_to_draw[idx+2].y };
				struct cfb_position p4 = { objects_to_draw[idx+3].x, objects_to_draw[idx+3].y };

				/* З'єднуємо кути лініями, створюючи прямокутник секції */
				 //cfb_draw_line(display, &p1, &p2);
				 //cfb_draw_line(display, &p2, &p3);
				 cfb_draw_line(display, &p3, &p4);
				//cfb_draw_line(display, &p4, &p1);
			}

			/* Окремо малюємо кола на кутах, якщо вони в полі зору */
			for (int c = 0; c < 4; c++) {
				if (objects_to_draw[idx + c].visible) {
					draw_ball(objects_to_draw[idx + c].x, objects_to_draw[idx + c].y, 
							  objects_to_draw[idx + c].radius, width, height);
				}
			}
		}

		/* Фізика руху */
		camAngle += vr;
		float ax = cosf(camAngle) * thrust;
		float ay = sinf(camAngle) * thrust;
		vx += ax;
		vz += ay;

if (fabsf(vx) > 0.01f) {vx *= 0.995f;camera.x += vx;}
if (fabsf(vz) > 0.01f) {vz *= 0.995f;camera.z += vz;}
camera.y += vy;


  /* --- БЛОК ОБЧИСЛЕННЯ ТА ВИВЕДЕННЯ FPS --- */
		frame_count++;
		int32_t current_time = k_uptime_get_32();
		if (current_time - last_fps_time >= 1000) {
			current_fps = frame_count;
			frame_count = 0;
			last_fps_time = current_time;
		}

		/* Виводимо FPS поверх всієї графіки у верхньому лівому кутку */
		char fps_buf[16];
		snprintf(fps_buf, sizeof(fps_buf), "FPS:%d", current_fps);
		cfb_framebuffer_set_font(display, 0);
		cfb_print(display, fps_buf, 0, 0);

cfb_framebuffer_finalize(display);
} return 0;
}