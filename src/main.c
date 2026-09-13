#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

#include "controls.h"
#include "render.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
	generate_points();

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	float camAngle = M_PI + (M_PI / 2.0f);
	float vr = 0.0f, vx = 0.0f, vy = 0.0f, vz = 0.0f, thrust = 0.0f;
	Point3D camera = { .x = 0.0f, .y = 0.0f, .z = -900.0f };

	/* Налаштовуємо таймер на K_NO_WAIT. Період 15мс розблокує потенціал до 60+ FPS */
	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(15)); 

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

		/* 1. Миттєво зчитуємо безпечну копію стану кнопок */
		get_controls_snapshot(&vr, &thrust, &vy);

		cfb_framebuffer_clear(display, false);

		/* 2. Малюємо 3D сцену (вже оптимізовану) */
		render_frame(display, &camera, camAngle, width, height);

		/* 3. Фізика руху */
		camAngle += vr;
		vx += cosf(camAngle) * thrust;
		vz += sinf(camAngle) * thrust;

		if (fabsf(vx) > 0.01f) { vx *= 0.995f; camera.x += vx; }
		if (fabsf(vz) > 0.01f) { vz *= 0.995f; camera.z += vz; }
		camera.y += vy;

		/* 4. Обчислення FPS */
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
	}
	return 0;
}
