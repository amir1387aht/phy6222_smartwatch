#include "display/display.h"
#include "spi.h"

static hal_spi_t s_spi;
static uint8_t tabcolor = INITR_GREENTAB;
static uint8_t _colstart = 0, _rowstart = 0;
static uint16_t _width = ST7735_TFTWIDTH_128;
static uint16_t _height = ST7735_TFTHEIGHT_128;
static uint8_t rotation = 0;

static void ST7735_Command(uint8_t cmd) {
    hal_gpio_write(DC_PIN, 0);
    hal_spi_send_byte(&s_spi, cmd);
}

static void ST7735_Data(uint8_t* buff, uint8_t buff_size) {
    hal_gpio_write(DC_PIN, 1);
    hal_spi_transmit(&s_spi, SPI_TXD, buff, NULL, buff_size, 0);
}

static void ST7735_WriteCommand(uint8_t cmd) {
    ST7735_Command(cmd);
}

static void ST7735_WriteData(uint8_t data) {
    ST7735_Data(&data, 1);
}

static void ST7735_WriteDataMultiple(uint8_t* data, uint8_t size) {
    ST7735_Data(data, size);
}

static void ST7735_ExecuteCommandList(const uint8_t *addr) {
    uint8_t numCommands, numArgs;
    uint16_t ms;
    
    numCommands = *addr++;
    while(numCommands--) {
        uint8_t cmd = *addr++;
        ST7735_WriteCommand(cmd);
        
        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & 0x80 ? *addr++ : 0;
        numArgs &= 0x7F;
        
        if(numArgs) {
            ST7735_WriteDataMultiple((uint8_t*)addr, numArgs);
            addr += numArgs;
        }
        
        if(ms) {
            if(ms == 255) ms = 500;
            WaitMs(ms);
        }
    }
}

static void display_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t data[4];
    
    // Column address set
    ST7735_WriteCommand(ST77XX_CASET);
    data[0] = 0x00;
    data[1] = x0 + _colstart;
    data[2] = 0x00;
    data[3] = x1 + _colstart;
    ST7735_WriteDataMultiple(data, 4);
    
    // Row address set
    ST7735_WriteCommand(ST77XX_RASET);
    data[0] = 0x00;
    data[1] = y0 + _rowstart;
    data[2] = 0x00;
    data[3] = y1 + _rowstart;
    ST7735_WriteDataMultiple(data, 4);
    
    // Memory write
    ST7735_WriteCommand(ST77XX_RAMWR);
}

void display_fill_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t* color_buffer, uint32_t size) {
    // Set the address window
    display_set_addr_window(x0, y0, x1, y1);
    
    // Switch to data mode
    hal_gpio_write(DC_PIN, 1);
    
    // Send the color buffer to fill the window
    hal_spi_transmit(&s_spi, SPI_TXD, (uint8_t*)color_buffer, NULL, size * 2, 0);
}

static void ST7735_InitR(uint8_t options) {
    static const uint8_t Rcmd1[] = {
        15,                             // 15 commands in list:
        ST77XX_SWRESET,   0x80,         //  1: Software reset, 0 args, w/delay
        10,                            //     10 ms delay
        ST77XX_SLPOUT,    0x80,         //  2: Out of sleep mode, 0 args, w/delay
        10,                            //     10 ms delay
        ST7735_FRMCTR1, 3,              //  3: Framerate ctrl - normal mode, 3 arg:
        0x01, 0x2C, 0x2D,               //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR2, 3,              //  4: Framerate ctrl - idle mode, 3 args:
        0x01, 0x2C, 0x2D,               //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR3, 6,              //  5: Framerate - partial mode, 6 args:
        0x01, 0x2C, 0x2D,               //     Dot inversion mode
        0x01, 0x2C, 0x2D,               //     Line inversion mode
        ST7735_INVCTR,  1,              //  6: Display inversion ctrl, 1 arg:
        0x07,                           //     No inversion
        ST7735_PWCTR1,  3,              //  7: Power control, 3 args, no delay:
        0xA2,
        0x02,                           //     -4.6V
        0x84,                           //     AUTO mode
        ST7735_PWCTR2,  1,              //  8: Power control, 1 arg, no delay:
        0xC5,                           //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
        ST7735_PWCTR3,  2,              //  9: Power control, 2 args, no delay:
        0x0A,                           //     Opamp current small
        0x00,                           //     Boost frequency
        ST7735_PWCTR4,  2,              // 10: Power control, 2 args, no delay:
        0x8A,                           //     BCLK/2,
        0x2A,                           //     opamp current small & medium low
        ST7735_PWCTR5,  2,              // 11: Power control, 2 args, no delay:
        0x8A, 0xEE,
        ST7735_VMCTR1,  1,              // 12: Power control, 1 arg, no delay:
        0x0E,
        ST77XX_INVOFF,  0,              // 13: Don't invert display, no args
        ST77XX_MADCTL,  1,              // 14: Mem access ctl (directions), 1 arg:
        0xC8,                           //     row/col addr, bottom-top refresh
        ST77XX_COLMOD,  1,              // 15: set color mode, 1 arg, no delay:
        0x05                            //     16-bit color
    };

    static const uint8_t Rcmd2green[] = {
        2,                              //  2 commands in list:
        ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
        0x00, 0x02,                     //     XSTART = 0
        0x00, 0x7F+0x02,                //     XEND = 127
        ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
        0x00, 0x01,                     //     XSTART = 0
        0x00, 0x9F+0x01                 //     XEND = 159
    };

    static const uint8_t Rcmd2red[] = {
        2,                              //  2 commands in list:
        ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
        0x00, 0x00,                     //     XSTART = 0
        0x00, 0x7F,                     //     XEND = 127
        ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
        0x00, 0x00,                     //     XSTART = 0
        0x00, 0x9F                      //     XEND = 159
    };

    static const uint8_t Rcmd3[] = {
        4,                              //  4 commands in list:
        ST7735_GMCTRP1, 16,             //  1: Gamma Adjustments (pos. polarity), 16 args:
        0x02, 0x1c, 0x07, 0x12,        
        0x37, 0x32, 0x29, 0x2d,        
        0x29, 0x25, 0x2B, 0x39,
        0x00, 0x01, 0x03, 0x10,
        ST7735_GMCTRN1, 16,             //  2: Gamma Adjustments (neg. polarity), 16 args:
        0x03, 0x1d, 0x07, 0x06,        
        0x2E, 0x2C, 0x29, 0x2D,        
        0x2E, 0x2E, 0x37, 0x3F,
        0x00, 0x00, 0x02, 0x10,
        ST77XX_NORON,     0x80,         //  3: Normal display on, no args, w/delay
        10,                             //     10 ms delay
        ST77XX_DISPON,    0x80,         //  4: Main screen turn on, no args w/delay
        10                             //     10 ms delay
    };

    // First command list - common to all types
    ST7735_ExecuteCommandList(Rcmd1);

    // Second command list - based on tab color
    if (options == INITR_GREENTAB) {
        ST7735_ExecuteCommandList(Rcmd2green);
        _colstart = 2;
        _rowstart = 1;
    } else {
        // Default is red tab
        ST7735_ExecuteCommandList(Rcmd2red);
        // colstart, rowstart left at default '0' values
    }

    // Third command list - common to all types
    ST7735_ExecuteCommandList(Rcmd3);

    // Black tab has different color settings
    if (options == INITR_BLACKTAB || options == INITR_MINI160x80) {
        uint8_t data = 0xC0;
        ST7735_WriteCommand(ST77XX_MADCTL);
        ST7735_WriteData(data);
    }

    tabcolor = options;
    
    // Set rotation to default state
    display_set_rotation(0);
}

void display_set_rotation(uint8_t m) {
    uint8_t madctl = 0;

    rotation = m & 3; // can't be higher than 3

    // For ST7735 with GREEN TAB
    if (tabcolor == INITR_144GREENTAB) {
        // ..._rowstart is 3 for rotations 0&1, 1 for rotations 2&3
        _rowstart = (rotation < 2) ? 3 : 1;
    }

    switch (rotation) {
    case 0:
        if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
        } else {
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;  // Changed from BGR to RGB
        }

        if (tabcolor == INITR_144GREENTAB) {
            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
        } else if (tabcolor == INITR_MINI160x80) {
            _height = ST7735_TFTHEIGHT_160;
            _width = 80;
        } else {
            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
        }
        break;
    case 1:
        if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        } else {
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        }

        if (tabcolor == INITR_144GREENTAB) {
            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
        } else if (tabcolor == INITR_MINI160x80) {
            _width = ST7735_TFTHEIGHT_160;
            _height = 80;
        } else {
            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
        }
        break;
    case 2:
        if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
            madctl = ST77XX_MADCTL_RGB;
        } else {
            madctl = ST77XX_MADCTL_RGB;
        }

        if (tabcolor == INITR_144GREENTAB) {
            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
        } else if (tabcolor == INITR_MINI160x80) {
            _height = ST7735_TFTHEIGHT_160;
            _width = 80;
        } else {
            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
        }
        break;
    case 3:
        if ((tabcolor == INITR_BLACKTAB) || (tabcolor == INITR_MINI160x80)) {
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        } else {
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        }

        if (tabcolor == INITR_144GREENTAB) {
            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
        } else if (tabcolor == INITR_MINI160x80) {
            _width = ST7735_TFTHEIGHT_160;
            _height = 80;
        } else {
            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
        }
        break;
    }

    ST7735_WriteCommand(ST77XX_MADCTL);
    ST7735_WriteData(madctl);
}

// Fill the screen with a specified color
void display_fill_screen(uint16_t color) {
    #define BUFFER_ROWS 128
    static uint16_t buffer[ST7735_TFTWIDTH_128 * BUFFER_ROWS];
    uint16_t i, rows_remaining = _height;
    
    // Fill buffer with color
    for (i = 0; i < ST7735_TFTWIDTH_128 * BUFFER_ROWS; i++) {
        buffer[i] = color;
    }
    
    // Set display window for full screen
    display_set_addr_window(0, 0, _width - 1, _height - 1);
    hal_gpio_write(DC_PIN, 1); // Data mode
    
    // Send screen in chunks using the buffer
    while (rows_remaining > 0) {
        uint16_t rows_to_send = (rows_remaining > BUFFER_ROWS) ? BUFFER_ROWS : rows_remaining;
        hal_spi_transmit(&s_spi, SPI_TXD, (uint8_t*)buffer, NULL, rows_to_send * _width * 2, 0);
        rows_remaining -= rows_to_send;
    }
}

// Convert RGB color to display format
uint16_t display_color(uint8_t r, uint8_t g, uint8_t b) {
    // RGB565 format: 5 bits for R, 6 bits for G, 5 bits for B
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Draw a pixel at specified coordinates with specified color
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= _width || y >= _height) return;
    
    uint8_t color_data[2] = {color >> 8, color & 0xFF};
    
    display_set_addr_window(x, y, x, y);
    hal_gpio_write(DC_PIN, 1); // Data mode
    hal_spi_transmit(&s_spi, SPI_TXD, color_data, NULL, 2, 0);
}

// Initialize the display
void display_init(void) {
    // GPIO Init
    hal_gpio_pin_init(DC_PIN, GPIO_OUTPUT);
    hal_gpio_pin_init(RST_PIN, GPIO_OUTPUT);
    hal_gpio_pin_init(CS_PIN, GPIO_OUTPUT);
    
    // Reset Display
    hal_gpio_write(RST_PIN, 0);
    WaitMs(10);
    hal_gpio_write(RST_PIN, 1);
    WaitMs(10);
    
    // SPI Config
    spi_Cfg_t spi_cfg = {
        .sclk_pin  = SCLK_PIN,
        .ssn_pin   = CS_PIN,
        .MOSI      = MOSI_PIN,
        .MISO      = 0, // unused
        .baudrate  = 80000000,
        .spi_tmod  = SPI_TXD,
        .spi_scmod = SPI_MODE0,
        .spi_dfsmod = SPI_8BIT,
        .int_mode  = false,
        .force_cs  = false,
        .evt_handler = NULL,
    };
    
    s_spi.spi_index = SPI0;
    hal_spi_bus_init(&s_spi, spi_cfg);

    ST7735_InitR(INITR_GREENTAB);
    display_set_rotation(2);
    display_fill_screen(0x000);

    hal_gpio_pin_init(BKL_PIN, GPIO_OUTPUT);
    hal_gpio_write(BKL_PIN, 0); // Turn on backlight
}