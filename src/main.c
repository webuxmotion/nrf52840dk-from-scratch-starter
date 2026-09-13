#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Реєструємо модуль логування з назвою "main" */
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    /* Виводимо інформаційне повідомлення в консоль */
    LOG_INF("Hello World from nRF52840 DK!");

    while (1) {
        /* Цикл залишається порожнім, або сюди можна додати періодичний вивід повідомлень.
           Засинаємо на 5 секунд, щоб процесор не працював у холосту. */
        k_msleep(5000);
    }

    return 0;
}
