#ifndef DISPLAY_H
#define DISPLAY_H

#include "log.h"
#include "gpio.h"
#include "config.h"
#include "mcu.h"

#define ST77XX_SWRESET    0x01
#define ST77XX_SLPOUT     0x11
#define ST77XX_NORON      0x13
#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_MADCTL     0x36
#define ST77XX_COLMOD     0x3A

#define ST7735_FRMCTR1    0xB1
#define ST7735_FRMCTR2    0xB2
#define ST7735_FRMCTR3    0xB3
#define ST7735_INVCTR     0xB4
#define ST7735_DISSET5    0xB6
#define ST7735_PWCTR1     0xC0
#define ST7735_PWCTR2     0xC1
#define ST7735_PWCTR3     0xC2
#define ST7735_PWCTR4     0xC3
#define ST7735_PWCTR5     0xC4
#define ST7735_VMCTR1     0xC5
#define ST7735_PWCTR6     0xFC
#define ST7735_GMCTRP1    0xE0
#define ST7735_GMCTRN1    0xE1

// MADCTL bit definitions
#define ST77XX_MADCTL_MX  0x40  // Row address order
#define ST77XX_MADCTL_MY  0x80  // Column address order
#define ST77XX_MADCTL_MV  0x20  // Row/Column exchange
#define ST7735_MADCTL_RGB 0x00  // RGB order
#define ST7735_MADCTL_BGR 0x08  // BGR order

// Display size definitions
#define ST7735_TFTWIDTH_128   128
#define ST7735_TFTHEIGHT_128  128

#define ST7735_TFTHEIGHT_160  160

// Display type options
#define INITR_GREENTAB        0x0
#define INITR_REDTAB          0x1
#define INITR_BLACKTAB        0x2
#define INITR_144GREENTAB     0x3
#define INITR_MINI160x80      0x4
#define INITR_MINI160x80_PLUGIN 0x5
#define INITR_HALLOWING       0x6

#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0x07E0
#define ST77XX_GREEN 0x001F
#define ST77XX_BLUE 0xF800

void display_init(uint16_t width, uint16_t height, uint8_t _rotation);
void display_fill_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t* color_buffer, uint32_t size);
void display_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_fill_screen(uint16_t color);
void display_set_rotation(uint8_t m);
uint16_t display_get_color(uint8_t r, uint8_t g, uint8_t b);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void backlight_turn_off();
void backlight_turn_on();

#endif // DISPLAY_H