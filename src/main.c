#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

  const int sides = 20;
	const float radius = 30.0;
	const float angleStep = (M_PI * 2.0) / sides;

  const int center_x = width / 2;
	const int center_y = height / 2;

  struct cfb_position points[sides];

  for (int i = 0; i < sides; i++) {
    float x_calc = (cosf(angleStep * (float)i) * radius) + (float)center_x;
    float y_calc = (sinf(angleStep * (float)i) * radius) + (float)center_y;

    points[i].x = (uint16_t)lroundf(x_calc);
    points[i].y = (uint16_t)lroundf(y_calc);
  }

  for (int i = 0; i < sides; i++) {
		if (i < sides - 1) {
			cfb_draw_line(display, &points[i], &points[i + 1]);
		} else {
			cfb_draw_line(display, &points[i], &points[0]);
		}
	}

	cfb_framebuffer_finalize(display);
	LOG_INF("Lines and multiple fonts sent to display. Sleeping.");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
