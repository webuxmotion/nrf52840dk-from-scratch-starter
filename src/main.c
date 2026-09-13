#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int main(void)
{
	uint16_t width;
	uint16_t height;

	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready");
		return -ENODEV;
	}

	if (cfb_framebuffer_init(display)) {
		LOG_ERR("Framebuffer initialization failed");
		return -EIO;
	}

	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);

	width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);
	LOG_INF("Display ready: %dx%d. Drawing...", width, height);

	cfb_framebuffer_clear(display, false);
	cfb_framebuffer_invert(display);

	cfb_draw_line(display, &(struct cfb_position){0, 0},           &(struct cfb_position){width - 1, 0});
	cfb_draw_line(display, &(struct cfb_position){width - 1, 0},   &(struct cfb_position){width - 1, height - 1});
	cfb_draw_line(display, &(struct cfb_position){width - 1, height - 1}, &(struct cfb_position){0, height - 1});
	cfb_draw_line(display, &(struct cfb_position){0, height - 1},   &(struct cfb_position){0, 0});

	cfb_framebuffer_finalize(display);
	LOG_INF("Lines sent to display. Sleeping.");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
