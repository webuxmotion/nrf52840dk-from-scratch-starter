#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

#include "controls.h"
#include "render.h"

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
K_SEM_DEFINE(display_sem, 0, 1);

void animation_timer_handler(struct k_timer *dummy) {
	k_sem_give(&display_sem);
}
K_TIMER_DEFINE(anim_timer, animation_timer_handler, NULL);

int main(void) {
	if (!device_is_ready(display) || cfb_framebuffer_init(display)) {
		return -EIO;
	}

	cfb_framebuffer_invert(display);

	init_controls();

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

  float angle = 0.0f;
  float rotate_speed = 0.0f;

	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(20)); 

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

    get_controls_snapshot(&rotate_speed);

		cfb_framebuffer_clear(display, false);

		render_frame(display, angle);

    angle += rotate_speed;

		cfb_framebuffer_finalize(display);

		k_msleep(2);
	}
	return 0;
}
