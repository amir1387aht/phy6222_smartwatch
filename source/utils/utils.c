#include "utils/utils.h"

void device_init(void)
{
    // LED
    hal_gpioretention_register(GPIO_LED);
    hal_gpio_write(GPIO_LED, 0);

    // Vibrator
    hal_gpioretention_register(GPIO_VIBRATOR);
    hal_gpio_write(GPIO_VIBRATOR, 0);
}

void device_vibrate(int count)
{
    for(int i = 0; i < count; i++)
    {
        if(i != 0) WaitMs(300);

        hal_gpio_write(GPIO_VIBRATOR, 1);
        WaitMs(300);
        hal_gpio_write(GPIO_VIBRATOR, 0);
    }
}

void device_led_blank(int count)
{
    for(int i = 0; i < count; i++)
    {
        if(i != 0) WaitMs(300);

        hal_gpio_write(GPIO_LED, 1);
        WaitMs(300);
        hal_gpio_write(GPIO_LED, 0);
    }
}