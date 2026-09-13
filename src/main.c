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

	/* 1. Малюємо рамку по контуру */
	cfb_draw_rect(display, &(struct cfb_position){0, 0}, &(struct cfb_position){width - 1, height - 1});

	/* 2. Малюємо горизонтальну лінію */
	cfb_draw_line(display, &(struct cfb_position){0, 20}, &(struct cfb_position){width - 1, 20});

	/* 3. Маленький шрифт (Індекс 0) для заголовка */
	cfb_framebuffer_set_font(display, 0);
	cfb_draw_text(display, "Font 0: SH1106 Demo", 6, 4);

	/* 4. Середній шрифт (Індекс 1) */
	cfb_framebuffer_set_font(display, 1);
	cfb_draw_text(display, "Font 1", 6, 24);

	/* 5. Повертаємося на маленький шрифт (Індекс 0) для решта елементів */
	cfb_framebuffer_set_font(display, 0);

	/* 6. Малюємо коло зліва внизу */
	cfb_draw_circle(display, &(struct cfb_position){24, 48}, 12);

	/* 7. Малюємо точки поруч із колом */
	cfb_draw_point(display, &(struct cfb_position){50, 48});
	cfb_draw_point(display, &(struct cfb_position){54, 48});
	cfb_draw_point(display, &(struct cfb_position){58, 48});

	/* 8. Виводимо текст меню та інвертуємо плашку */
	cfb_draw_text(display, "MENU ITEM", 66, 44);
	cfb_invert_area(display, 64, 42, 60, 18);

	/* Відправляємо кадр на OLED */
	cfb_framebuffer_finalize(display);
	LOG_INF("Lines and multiple fonts sent to display. Sleeping.");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
