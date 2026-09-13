#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

#include "controls.h"
#include "render.h"

static int current_fps = 0;
static int frame_count = 0;
static int32_t last_fps_time = 0;

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

	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
	cfb_framebuffer_invert(display);

	init_controls();

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	srand(k_cycle_get_32());
	generate_points(width, height);

	/* Початковий базовий радіус плями (адаптований під невеликий екран) */
	float blob_radius = (float)height * 0.3f; 
	if (blob_radius < 10.0f) blob_radius = 24.0f;

	bool radius_minus = false, radius_plus = false, btn_regen = false, show_nodes = true;

	/* Стабільний період кадру */
	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(20)); 

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

		get_controls_snapshot(&radius_minus, &radius_plus, &btn_regen, &show_nodes);

		/* Інтерактивна зміна базового радіуса з кнопок */
		if (radius_minus) { blob_radius -= 0.8f; if (blob_radius < 5.0f) blob_radius = 5.0f; }
		if (radius_plus)  { blob_radius += 0.8f; if (blob_radius > (width * 0.45f)) blob_radius = width * 0.45f; }
		
		if (btn_regen) { generate_points(width, height); }

		cfb_framebuffer_clear(display, false);

		/* Постійна дельта часу для плавності (~0.025 сек) */
		update_and_render_blob(display, 0.025f, blob_radius, show_nodes, width, height);

		/* FPS */
		frame_count++;
		int32_t current_time = k_uptime_get_32();
		if (current_time - last_fps_time >= 1000) {
			current_fps = frame_count;
			frame_count = 0;
			last_fps_time = current_time;
		}

		char fps_buf[16];
		snprintf(fps_buf, sizeof(fps_buf), "FPS:%d", current_fps);
		cfb_framebuffer_set_font(display, 0);
		cfb_print(display, fps_buf, 0, 0);

		cfb_framebuffer_finalize(display);
		k_msleep(2);
	}
	return 0;
}
