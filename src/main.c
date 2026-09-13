#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static uint32_t timer_counter = 0;

void my_timer_handler(struct k_timer *dummy)
{
    timer_counter++;
    LOG_INF("[ТАЙМЕР] Лічильник збільшено: %u", timer_counter);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

int main(void)
{
  uint32_t counter = 0;
  LOG_INF("Старт програми з k_timer!");

  k_timer_start(&my_timer, K_MSEC(0), K_MSEC(300));

  while (1) {

    LOG_INF("Andrii's counter: %u", counter);

    counter += 1;

    k_msleep(2000);
  }

  return 0;
}
