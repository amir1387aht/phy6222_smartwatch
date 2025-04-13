#include "app/app.h"
#include "log.h"
#include "gpio.h"
#include "config.h"

void posedge_callback_wakeup(GPIO_Pin_e pin, IO_Wakeup_Pol_e type)
{
    LOG("PosEdge");
    hal_gpio_write(GPIO_VIBRATOR, 1);
}

void negedge_callback_wakeup(GPIO_Pin_e pin, IO_Wakeup_Pol_e type)
{
    LOG("NegEdge");
    hal_gpio_write(GPIO_VIBRATOR, 0);
}

void app_init()
{
    hal_gpioin_register(GPIO_P11, posedge_callback_wakeup, negedge_callback_wakeup);
}

void app_update()
{
    // Use proper RGB565 format for blue (0x001F)
    display_fill_screen(0x001F);
    WaitMs(1000);
        
    // Test with red (0xF800)
    display_fill_screen(0xF800);
    WaitMs(1000);
        
    // Test with green (0x07E0)
    display_fill_screen(0x07E0);
    WaitMs(1000);
}