#ifndef __APP_H__
#define __APP_H__

#include "log.h"
#include "gpio.h"
#include "config.h"
#include "utils/utils.h"
#include "display/display.h"
#include "display/gfx.h"
#include "fonts/FreeMono9pt7b.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void app_init();
    void app_update();
#ifdef __cplusplus
}
#endif

#endif