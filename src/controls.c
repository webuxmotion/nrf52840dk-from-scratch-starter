#include "controls.h"

static const struct gpio_dt_spec btn_sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_sw1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_sw2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_sw3 = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

static struct gpio_callback cb_sw0, cb_sw1, cb_sw2, cb_sw3;

static volatile bool is_sw0_pressed = false;
static volatile bool is_sw1_pressed = false;
static volatile bool is_sw2_pressed = false;
static volatile bool is_sw3_pressed = false;

static volatile bool ui_show_nodes = true;
static volatile bool regen_trigger = false;

static void handler_sw0(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	is_sw0_pressed = gpio_pin_get_dt(&btn_sw0);
}
static void handler_sw1(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	is_sw1_pressed = gpio_pin_get_dt(&btn_sw1);
}
static void handler_sw2(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	if (gpio_pin_get_dt(&btn_sw2)) { regen_trigger = true; }
}
static void handler_sw3(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	if (gpio_pin_get_dt(&btn_sw3) && !is_sw3_pressed) {
		ui_show_nodes = !ui_show_nodes;
	}
	is_sw3_pressed = gpio_pin_get_dt(&btn_sw3);
}

void init_controls(void) {
	if (device_is_ready(btn_sw0.port)) {
		gpio_pin_configure_dt(&btn_sw0, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_sw0, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_sw0, handler_sw0, BIT(btn_sw0.pin));
		gpio_add_callback(btn_sw0.port, &cb_sw0);
	}
	if (device_is_ready(btn_sw1.port)) {
		gpio_pin_configure_dt(&btn_sw1, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_sw1, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_sw1, handler_sw1, BIT(btn_sw1.pin));
		gpio_add_callback(btn_sw1.port, &cb_sw1);
	}
	if (device_is_ready(btn_sw2.port)) {
		gpio_pin_configure_dt(&btn_sw2, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_sw2, GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&cb_sw2, handler_sw2, BIT(btn_sw2.pin));
		gpio_add_callback(btn_sw2.port, &cb_sw2);
	}
	if (device_is_ready(btn_sw3.port)) {
		gpio_pin_configure_dt(&btn_sw3, GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&btn_sw3, GPIO_INT_EDGE_BOTH);
		gpio_init_callback(&cb_sw3, handler_sw3, BIT(btn_sw3.pin));
		gpio_add_callback(btn_sw3.port, &cb_sw3);
	}
}

void get_controls_snapshot(bool *out_radius_minus, bool *out_radius_plus, bool *out_regenerate, bool *out_toggle_ui) {
	*out_radius_minus = is_sw0_pressed;
	*out_radius_plus = is_sw1_pressed;
	*out_regenerate = regen_trigger;
	regen_trigger = false;
	*out_toggle_ui = ui_show_nodes;
}
