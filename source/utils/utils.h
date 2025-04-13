#ifndef UTILS_H
#define UTILS_H

#include "log.h"
#include "gpio.h"
#include "config.h"

void device_init(void);
void device_vibrate(int count);
void device_led_blank(int count);

#endif // UTILS_H