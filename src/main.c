#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static const struct gpio_dt_spec button_plus = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec button_minus = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

static struct gpio_callback button_plus_cb_data;
static struct gpio_callback button_minus_cb_data;

static volatile int current_sides = 10;
static struct k_work display_work;

void update_display_handler(struct k_work *work)
{
	int sides = current_sides;

	if (!device_is_ready(display)) {
		return;
	}

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	cfb_framebuffer_clear(display, false);

	const float radius = 22.0f;
	const float angleStep = (M_PI * 2.0f) / (float)sides;
	const int center_x = width - radius - 1;
	const int center_y = height / 2; 

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

  for (int i = 0; i < sides; i++) {
		float x1_f = (cosf(angleStep * (float)i) * radius) + (float)center_x / 2;
		float y1_f = (sinf(angleStep * (float)i) * radius) + (float)center_y;

		int next_i = i + 1;
		if (next_i >= sides) {
			next_i = 0;
		}

		float x2_f = (cosf(angleStep * (float)next_i) * radius) + (float)center_x / 2;
		float y2_f = (sinf(angleStep * (float)next_i) * radius) + (float)center_y;

		struct cfb_position p1 = { (uint16_t)lroundf(x1_f), (uint16_t)lroundf(y1_f) };
		struct cfb_position p2 = { (uint16_t)lroundf(x2_f), (uint16_t)lroundf(y2_f) };

		cfb_draw_line(display, &p1, &p2);
	}

	char text_buf[16];
	snprintf(text_buf, sizeof(text_buf), "Sides: %d", sides);

	cfb_framebuffer_set_font(display, 0);
	cfb_draw_text(display, text_buf, 0, 0);
  cfb_draw_text(display, text_buf, 0, 12);
  cfb_draw_text(display, text_buf, 0, 24);
  cfb_draw_text(display, text_buf, 0, 36);
  cfb_draw_text(display, text_buf, 0, 48);

	cfb_framebuffer_finalize(display);
}

void button_plus_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (current_sides < 50) {
		current_sides++;
		k_work_submit(&display_work);
	}
}

void button_minus_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (current_sides > 3) {
		current_sides--;
		k_work_submit(&display_work);
	}
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

	k_work_init(&display_work, update_display_handler);

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

	k_work_submit(&display_work);

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
