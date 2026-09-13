#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_CIRCLES 30

int fps = 0;
int frame_count = 0;
int32_t last_fps_time = 0;

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static const struct gpio_dt_spec button_plus = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec button_minus = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

static struct gpio_callback button_plus_cb_data;
static struct gpio_callback button_minus_cb_data;

static volatile int current_sides = 10;

K_SEM_DEFINE(display_sem, 0, 1);

static float anim_angle = 0.0f;

struct CircleData {
  float base_center_x;
};

static struct CircleData circles_data[MAX_CIRCLES];

static volatile int current_circles_count = 10;

void animation_timer_handler(struct k_timer *dummy)
{
	k_sem_give(&display_sem);
}

K_TIMER_DEFINE(anim_timer, animation_timer_handler, NULL);

void button_plus_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (current_sides < 50) {
		current_sides++;
	}
}

void button_minus_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (current_sides > 3) {
		current_sides--;
	}
}

void drawCircle(int sides, float angleStep, float radius, int center_x, int center_y) {
  for (int i = 0; i < sides; i++) {
    float x1_f = (cosf(angleStep * (float)i) * radius) + (float)center_x;
    float y1_f = (sinf(angleStep * (float)i) * radius) + (float)center_y;

    int next_i = i + 1;
    if (next_i >= sides) {
      next_i = 0;
    }

    float x2_f = (cosf(angleStep * (float)next_i) * radius) + (float)center_x;
    float y2_f = (sinf(angleStep * (float)next_i) * radius) + (float)center_y;

    struct cfb_position p1 = { (uint16_t)lroundf(x1_f), (uint16_t)lroundf(y1_f) };
    struct cfb_position p2 = { (uint16_t)lroundf(x2_f), (uint16_t)lroundf(y2_f) };

    cfb_draw_line(display, &p1, &p2);
  }
}

void drawText(int sides, int current_fps) {
  char text_buf[16];
  
  /* Виводимо кількість сторін */
  snprintf(text_buf, sizeof(text_buf), "Sides: %d", sides);
  cfb_framebuffer_set_font(display, 0);
  cfb_print(display, text_buf, 0, 0);

  /* Виводимо FPS в іншому місці (наприклад, y = 12) */
  snprintf(text_buf, sizeof(text_buf), "FPS: %d", current_fps);
  cfb_print(display, text_buf, 0, 12);
  
  // char text_buf[16];
	// snprintf(text_buf, sizeof(text_buf), "Sides: %d", sides);

	// cfb_framebuffer_set_font(display, 0);
	// cfb_draw_text(display, text_buf, 0, 0);
  // cfb_draw_text(display, text_buf, 0, 12);
  // cfb_draw_text(display, text_buf, 0, 24);
  // cfb_draw_text(display, text_buf, 0, 36);
  // cfb_draw_text(display, text_buf, 0, 48);

	// cfb_framebuffer_finalize(display);
}

int main(void)
{
	if (!device_is_ready(display)) {
		LOG_ERR("Display not ready");
		return -ENODEV;
	}

	if (cfb_framebuffer_init(display)) {
		LOG_ERR("Framebuffer init failed");
		return -EIO;
	}

	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
	cfb_framebuffer_invert(display);

	if (device_is_ready(button_plus.port)) {
		gpio_pin_configure_dt(&button_plus, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&button_plus, GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&button_plus_cb_data, button_plus_pressed, BIT(button_plus.pin));
		gpio_add_callback(button_plus.port, &button_plus_cb_data);
	}

	if (device_is_ready(button_minus.port)) {
		gpio_pin_configure_dt(&button_minus, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&button_minus, GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&button_minus_cb_data, button_minus_pressed, BIT(button_minus.pin));
		gpio_add_callback(button_minus.port, &button_minus_cb_data);
	}

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

  srand(k_cycle_get_32());
	for (int i = 0; i < current_circles_count; i++) {
		int idx = i;
    circles_data[idx].base_center_x = (float)width - (2.0f + ((float)rand() / (float)RAND_MAX) * (100.0f - 2.0f));
	}

	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(20));

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

		anim_angle += 0.08f; 
		if (anim_angle > M_PI * 2.0f) {
			anim_angle -= M_PI * 2.0f;
		}

		int sides = current_sides;
    int active_circles = current_circles_count;
		cfb_framebuffer_clear(display, false);

		const float radius = 24.0f;
		const float angleStep = (M_PI * 2.0f) / (float)sides;
		float base_center_y = (float)height / 2.0f;

    for (int c = 0; c < active_circles; c++) {
      int c_idx = c;

      float base_center_x = circles_data[c_idx].base_center_x;

			int center_x = (int)lroundf(base_center_x + (cosf(anim_angle) * 20.0f));
			int center_y = (int)lroundf(base_center_y + (sinf(anim_angle) * 12.0f));

      drawCircle(sides, angleStep, radius, center_x, center_y);
    }

    /* --- [Розрахунок FPS] --- */
    frame_count++;
    int32_t current_time = k_uptime_get_32();
    
    /* Якщо минула 1 секунда (1000 мілісекунд) */
    if (current_time - last_fps_time >= 1000) {
        fps = frame_count;          /* Кількість кадрів за секунду */
        frame_count = 0;            /* Скидаємо лічильник для наступної секунди */
        last_fps_time = current_time;
    }

    drawText(sides, fps);

		cfb_framebuffer_finalize(display);
	}

	return 0;
}
