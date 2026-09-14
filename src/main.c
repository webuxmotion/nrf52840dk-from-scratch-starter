#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Hello World from nRF52840 DK!");

    while (1) {
        k_msleep(5000);
    }

    return 0;
}
