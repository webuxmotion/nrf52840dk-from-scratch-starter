#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

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

	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
	cfb_framebuffer_invert(display);

	init_controls();

	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	init_geometry_points(width, height);

	int selected_idx = 0; // 0=A, 1=B, 2=C
	float current_border_radius = 20.0f;

	bool r_click = false, move_y = false, next_point = false, move_x = false;
	bool dir_y = true, dir_x = true;

	/* Стабільний період оновлення кадру */
	k_timer_start(&anim_timer, K_NO_WAIT, K_MSEC(25)); 

	while (1) {
		k_sem_take(&display_sem, K_FOREVER);

		get_controls_snapshot(&r_click, &move_y, &next_point, &move_x, &dir_y, &dir_x);

		/* Button 3: Наступна точка (A -> B -> C) */
		if (next_point) {
			selected_idx = (selected_idx + 1) % 3;
		}

		/* Button 1: Зміна радіуса кута при кліку */
		if (r_click) {
			current_border_radius += 10.0f;
			if (current_border_radius > 60.0f) {
				current_border_radius = 10.0f;
			}
		}

		/* Швидкість перетягування */
		float step_y = dir_y ? 1.0f : -1.0f;
		float step_x = dir_x ? 1.0f : -1.0f;

		/* Button 2: Перетягуємо Y */
		if (move_y) {
			move_geometry_point(selected_idx, 0.0f, step_y);
		}

		/* Button 4: Перетягуємо X */
		if (move_x) {
			move_geometry_point(selected_idx, step_x, 0.0f);
		}

		cfb_framebuffer_clear(display, false);

		/* Розрахунок та відмальовування геометричного кута */
		do_math_and_render(display, current_border_radius, selected_idx, width, height);

		cfb_framebuffer_finalize(display);
		k_msleep(2); /* Захисна пауза шини */
	}
	return 0;
}
