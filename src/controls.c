#include "controls.h"

static const struct gpio_dt_spec btn_left   = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_right  = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_thrust = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_toggle = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

static struct gpio_callback cb_left, cb_right, cb_thrust, cb_toggle;

static volatile bool is_left_pressed = false;
static volatile bool is_right_pressed = false;
static volatile bool is_thrust_pressed = false;
static volatile bool is_toggle_pressed = false;
static volatile bool next_vertical_dir_is_top = true;

static void handler_left(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	is_left_pressed = gpio_pin_get_dt(&btn_left);
}
static void handler_right(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	is_right_pressed = gpio_pin_get_dt(&btn_right);
}
static void handler_thrust(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	is_thrust_pressed = gpio_pin_get_dt(&btn_thrust);
}
static void handler_toggle(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	bool current_state = gpio_pin_get_dt(&btn_toggle);
	if (current_state && !is_toggle_pressed) {
		next_vertical_dir_is_top = !next_vertical_dir_is_top;
	}
	is_toggle_pressed = current_state;
}

void init_controls(void) {
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
}

void get_controls_snapshot(float *rotate_speed) {

  *rotate_speed = is_thrust_pressed ? ROTATE_SPEED : 0.0f;
}
