#include "display/display.h"
#include "spi.h"

static hal_spi_t s_spi;
static uint8_t tabcolor = INITR_GREENTAB;
static uint8_t _colstart = 0, _rowstart = 0;
static uint16_t _width = 0;
static uint16_t _height = 0;
static uint8_t rotation = 0;

// Inline the small helper functions for reduced overhead
static inline void ST7735_Command(uint8_t cmd) {
    hal_gpio_write(DC_PIN, 0);
    hal_spi_send_byte(&s_spi, cmd);
}

static inline void ST7735_Data(uint8_t* buff, uint8_t buff_size) {
    hal_gpio_write(DC_PIN, 1);
    hal_spi_transmit(&s_spi, SPI_TXD, buff, NULL, buff_size, 0);
}

static inline void ST7735_WriteCommand(uint8_t cmd) {
    ST7735_Command(cmd);
}

static inline void ST7735_WriteData(uint8_t data) {
    ST7735_Data(&data, 1);
}

static inline void ST7735_WriteDataMultiple(uint8_t* data, uint8_t size) {
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

void display_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
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
    // Set the address window once
    display_set_addr_window(x0, y0, x1, y1);
    
    // Switch to data mode and transfer the color buffer
    hal_gpio_write(DC_PIN, 1);
    hal_spi_transmit(&s_spi, SPI_TXD, (uint8_t*)color_buffer, NULL, size * 2, 0);
}

static void ST7735_InitR(uint8_t options) {
    static const uint8_t Rcmd1[] = {
        15,                             // 15 commands in list:
        ST77XX_SWRESET,   0x80,         // Software reset, w/delay
        10,                             // 10 ms delay
        ST77XX_SLPOUT,    0x80,         // Out of sleep mode, w/delay
        10,                             // 10 ms delay
        ST7735_FRMCTR1, 3,              // Framerate ctrl - normal mode, 3 args:
        0x01, 0x2C, 0x2D,
        ST7735_FRMCTR2, 3,              // Framerate ctrl - idle mode, 3 args:
        0x01, 0x2C, 0x2D,
        ST7735_FRMCTR3, 6,              // Framerate - partial mode, 6 args:
        0x01, 0x2C, 0x2D,
        0x01, 0x2C, 0x2D,
        ST7735_INVCTR,  1,              // Display inversion ctrl, 1 arg:
        0x07,
        ST7735_PWCTR1,  3,              // Power control, 3 args:
        0xA2, 0x02, 0x84,
        ST7735_PWCTR2,  1,              // Power control, 1 arg:
        0xC5,
        ST7735_PWCTR3,  2,              // Power control, 2 args:
        0x0A, 0x00,
        ST7735_PWCTR4,  2,              // Power control, 2 args:
        0x8A, 0x2A,
        ST7735_PWCTR5,  2,              // Power control, 2 args:
        0x8A, 0xEE,
        ST7735_VMCTR1,  1,              // Power control, 1 arg:
        0x0E,
        ST77XX_INVOFF,  0,              // Inversion off, no args
        ST77XX_MADCTL,  1,              // Memory access control, 1 arg:
        0xC8,
        ST77XX_COLMOD,  1,              // Set color mode, 1 arg:
        0x05
    };

    static const uint8_t Rcmd2green[] = {
        2,                              // 2 commands:
        ST77XX_CASET,   4,              // Column address set, 4 args:
        0x00, 0x02,
        0x00, 0x7F + 0x02,
        ST77XX_RASET,   4,              // Row address set, 4 args:
        0x00, 0x01,
        0x00, 0x9F + 0x01
    };

    static const uint8_t Rcmd2red[] = {
        2,                              // 2 commands:
        ST77XX_CASET,   4,              // Column address set, 4 args:
        0x00, 0x00,
        0x00, 0x7F,
        ST77XX_RASET,   4,              // Row address set, 4 args:
        0x00, 0x00,
        0x00, 0x9F
    };

    static const uint8_t Rcmd3[] = {
        4,                              // 4 commands:
        ST7735_GMCTRP1, 16,             // Gamma Adjustments (pos. polarity), 16 args:
        0x02, 0x1c, 0x07, 0x12,
        0x37, 0x32, 0x29, 0x2d,
        0x29, 0x25, 0x2B, 0x39,
        0x00, 0x01, 0x03, 0x10,
        ST7735_GMCTRN1, 16,             // Gamma Adjustments (neg. polarity), 16 args:
        0x03, 0x1d, 0x07, 0x06,
        0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F,
        0x00, 0x00, 0x02, 0x10,
        ST77XX_NORON,     0x80,         // Normal display on, w/delay
        10,
        ST77XX_DISPON,    0x80,         // Main screen turn on, w/delay
        10
    };

    // Execute command lists:
    ST7735_ExecuteCommandList(Rcmd1);

    if (options == INITR_GREENTAB) {
        ST7735_ExecuteCommandList(Rcmd2green);
        _colstart = 2;
        _rowstart = 1;
    } else {
        ST7735_ExecuteCommandList(Rcmd2red);
        // Use default _colstart and _rowstart (0)
    }

    ST7735_ExecuteCommandList(Rcmd3);

    tabcolor = options;
    
    // Set default rotation (0)
    display_set_rotation(0);
}

void display_set_rotation(uint8_t m) {
    // Support 4 rotation modes (0 - 3)
    m %= 4;
    rotation = m;
    uint8_t madctl = 0;

    if(tabcolor == INITR_GREENTAB) {
        _colstart = (m == 0 || m == 2) ? 2 : 1;
        _rowstart = (m == 0 || m == 1) ? 1 : 2;
    }
    
    // Set madctl bits for 4 rotation options
    switch(m) {
        case 0:
            madctl = ST7735_MADCTL_BGR;
            _colstart = (tabcolor == INITR_GREENTAB) ? 2 : 0;
            break;
        case 1:
            madctl = ST77XX_MADCTL_MV | ST7735_MADCTL_BGR;
            _colstart = (tabcolor == INITR_GREENTAB) ? 1 : 0;
            break;
        case 2:
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MX | ST7735_MADCTL_BGR;
            _colstart = (tabcolor == INITR_GREENTAB) ? 2 : 0;
            break;
        case 3:
            madctl = ST77XX_MADCTL_MV | ST77XX_MADCTL_MY | ST7735_MADCTL_BGR;
            _colstart = (tabcolor == INITR_GREENTAB) ? 1 : 0;
            break;
    }
    
    ST7735_WriteCommand(ST77XX_MADCTL);
    ST7735_WriteData(madctl);
}

// Fill the screen with a specific color using chunked DMA-friendly transfers.
void display_fill_screen(uint16_t color) {
    #define BUFFER_ROWS 128
    static uint16_t buffer[ST7735_TFTWIDTH_128 * BUFFER_ROWS];
    uint16_t i, rows_remaining = _height;
    
    // Convert color to the proper format (swap bytes if necessary)
    uint16_t swapped_color = ((color & 0xFF) << 8) | ((color >> 8) & 0xFF);
    
    // Fill buffer with the desired color
    for (i = 0; i < ST7735_TFTWIDTH_128 * BUFFER_ROWS; i++) {
        buffer[i] = swapped_color;
    }
    
    display_set_addr_window(0, 0, _width - 1, _height - 1);
    hal_gpio_write(DC_PIN, 1); // Data mode
    
    while (rows_remaining > 0) {
        uint16_t rows_to_send = (rows_remaining > BUFFER_ROWS) ? BUFFER_ROWS : rows_remaining;
        hal_spi_transmit(&s_spi, SPI_TXD, (uint8_t*)buffer, NULL, rows_to_send * _width * 2, 0);
        rows_remaining -= rows_to_send;
    }
}

uint16_t display_color(uint8_t r, uint8_t g, uint8_t b) {
    // Convert to RGB565 format: 5 bits for R, 6 for G, 5 for B
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= _width || y >= _height)
        return;
        
    uint8_t color_data[2] = { color >> 8, color & 0xFF };
    
    display_set_addr_window(x, y, x, y);
    hal_gpio_write(DC_PIN, 1); // Data mode
    hal_spi_transmit(&s_spi, SPI_TXD, color_data, NULL, 2, 0);
}

void display_init(uint16_t width, uint16_t height, uint8_t _rotation) {
    _width = width;
    _height = height;
    rotation = _rotation;

    // GPIO initialization
    hal_gpio_pin_init(DC_PIN, GPIO_OUTPUT);
    hal_gpio_pin_init(RST_PIN, GPIO_OUTPUT);
    hal_gpio_pin_init(CS_PIN, GPIO_OUTPUT);
    
    // Reset Display
    hal_gpio_write(RST_PIN, 0);
    WaitMs(10);
    hal_gpio_write(RST_PIN, 1);
    WaitMs(10);
    
    // SPI configuration with an increased baudrate (adjust as necessary)
    spi_Cfg_t spi_cfg = {
        .sclk_pin   = SCLK_PIN,
        .ssn_pin    = CS_PIN,
        .MOSI       = MOSI_PIN,
        .MISO       = 0, // not used
        .baudrate   = 15000000,
        .spi_tmod   = SPI_TXD,
        .spi_scmod  = SPI_MODE0,
        .spi_dfsmod = SPI_8BIT,
        .int_mode   = false,
        .force_cs   = false,
        .evt_handler= NULL,
    };
    
    s_spi.spi_index = SPI0;
    hal_spi_bus_init(&s_spi, spi_cfg);

    // Initialize the display with chosen tab color; you may want to allow other options
    ST7735_InitR(INITR_GREENTAB);
    
    // Set rotation based on the caller’s parameter (now supports values 0-3)
    display_set_rotation(rotation);
    
    // Clear screen (black)
    display_fill_screen(0x0000);

    // Backlight control initialization
    hal_gpio_pin_init(BKL_PIN, GPIO_OUTPUT);
    backlight_turn_on();
}

void backlight_turn_off()
{
    hal_gpio_write(BKL_PIN, 1);
}

void backlight_turn_on()
{
    hal_gpio_write(BKL_PIN, 0);
}