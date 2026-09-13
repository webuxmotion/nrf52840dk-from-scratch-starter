#include "controls.h"

static const struct gpio_dt_spec btn_sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios); // Радіус кута
static const struct gpio_dt_spec btn_sw1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios); // Рух Y
static const struct gpio_dt_spec btn_sw2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios); // Перемикання точок A, B, C
static const struct gpio_dt_spec btn_sw3 = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios); // Рух X

static struct gpio_callback cb_sw0, cb_sw1, cb_sw2, cb_sw3;

static volatile bool is_sw0_pressed = false;
static volatile bool is_sw1_pressed = false;
static volatile bool is_sw2_pressed = false;
static volatile bool is_sw3_pressed = false;

static volatile bool dir_y_positive = true;
static volatile bool dir_x_positive = true;
static volatile bool next_point_trigger = false;
static volatile bool radius_trigger = false;

static void handler_sw0(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	if (gpio_pin_get_dt(&btn_sw0)) { radius_trigger = true; }
}
static void handler_sw1(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	bool current_state = gpio_pin_get_dt(&btn_sw1);
	/* Після кожного завершення натискання-утримання (відпускання) інвертуємо напрямок Y */
	if (!current_state && is_sw1_pressed) {
		dir_y_positive = !dir_y_positive;
	}
	is_sw1_pressed = current_state;
}
static void handler_sw2(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	if (gpio_pin_get_dt(&btn_sw2)) { next_point_trigger = true; }
}
static void handler_sw3(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	bool current_state = gpio_pin_get_dt(&btn_sw3);
	/* Після кожного завершення натискання-утримання (відпускання) інвертуємо напрямок X */
	if (!current_state && is_sw3_pressed) {
		dir_x_positive = !dir_x_positive;
	}
	is_sw3_pressed = current_state;
}

void init_controls(void) {
	gpio_pin_configure_dt(&btn_sw0, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&btn_sw0, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&cb_sw0, handler_sw0, BIT(btn_sw0.pin));
	gpio_add_callback(btn_sw0.port, &cb_sw0);

	gpio_pin_configure_dt(&btn_sw1, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&btn_sw1, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&cb_sw1, handler_sw1, BIT(btn_sw1.pin));
	gpio_add_callback(btn_sw1.port, &cb_sw1);

	gpio_pin_configure_dt(&btn_sw2, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&btn_sw2, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&cb_sw2, handler_sw2, BIT(btn_sw2.pin));
	gpio_add_callback(btn_sw2.port, &cb_sw2);

	gpio_pin_configure_dt(&btn_sw3, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&btn_sw3, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&cb_sw3, handler_sw3, BIT(btn_sw3.pin));
	gpio_add_callback(btn_sw3.port, &cb_sw3);
}

void get_controls_snapshot(bool *out_radius_click, bool *out_move_y, bool *out_next_point, bool *out_move_x, 
                           bool *out_dir_y, bool *out_dir_x) {
	*out_radius_click = radius_trigger; radius_trigger = false;
	*out_move_y = is_sw1_pressed;
	*out_next_point = next_point_trigger; next_point_trigger = false;
	*out_move_x = is_sw3_pressed;
	*out_dir_y = dir_y_positive;
	*out_dir_x = dir_x_positive;
}
