#ifndef GFX_H
#define GFX_H

#include "display/display.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "osal.h"

// First, the GFXglyph structure should be defined to match your font data:
typedef struct {
    uint16_t bitmapOffset;  // Offset into bitmap array
    uint8_t  width;         // Width of glyph
    uint8_t  height;        // Height of glyph
    uint8_t  xAdvance;      // Distance to advance cursor
    int8_t   xOffset;       // X offset from cursor position
    int8_t   yOffset;       // Y offset from cursor position
} GFXglyph;

// Then the GFXfont structure should be:
typedef struct {
    const uint8_t  *bitmap;  // Bitmap data
    const GFXglyph *glyph;   // Glyph array
    uint8_t         first;   // ASCII index of first character
    uint8_t         last;    // ASCII index of last character
    uint8_t         yAdvance; // Newline distance (typically height)
} GFXfont;

// Init
void gfx_init(uint16_t width, uint16_t height, uint8_t rotation);

// Display rotation and orientation functions
void gfx_set_rotation(uint8_t rotation);
uint8_t gfx_get_rotation(void);
int16_t gfx_get_width(void);
int16_t gfx_get_height(void);

// Bitmap functions
void gfx_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
void gfx_draw_bitmap_bg(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
void gfx_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
void gfx_draw_rgb_bitmap_with_mask(int16_t x, int16_t y, const uint16_t *bitmap, const uint8_t *mask, int16_t w, int16_t h);
void gfx_draw_scaled_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, int16_t scale, uint16_t color);

// Basic drawing functions
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color);
void gfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void gfx_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color);
void gfx_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color);
void gfx_draw_dashed_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, uint8_t dash_len, uint8_t gap_len);

// Rectangle functions
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_draw_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
void gfx_fill_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

// Circle functions
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_draw_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
void gfx_fill_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);

// Arc function
void gfx_draw_arc(int16_t x0, int16_t y0, int16_t r, int16_t start_angle, int16_t end_angle, 
                uint8_t thickness, uint16_t color);

// Ellipse functions
void gfx_draw_ellipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color);
void gfx_fill_ellipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color);

// Triangle functions
void gfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void gfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

// Polygon functions
void gfx_draw_polygon(int16_t *points, uint8_t num_points, uint16_t color);
void gfx_fill_polygon(int16_t *points, uint8_t num_points, uint16_t color);

// Fill the screen
void gfx_fill_screen(uint16_t color);
void gfx_clear(void);

// Text functions
void gfx_set_font(const GFXfont *f);
const GFXfont* gfx_get_current_font(void);
void gfx_set_text_size(uint8_t s);
uint8_t gfx_get_text_size(void);
void gfx_set_text_color(uint16_t c);
void gfx_set_text_color_bg(uint16_t c, uint16_t bg);
void gfx_set_text_wrap(bool w);
bool gfx_get_text_wrap(void);
void gfx_set_cursor(int16_t x, int16_t y);
void gfx_set_cursor_no_height(int16_t x, int16_t y);
int16_t gfx_get_cursor_x(void);
int16_t gfx_get_cursor_y(void);
void gfx_get_text_bounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
void gfx_print(const char *str);
void gfx_println(const char *str);
void gfx_draw_text(int16_t x, int16_t y, const char *str, uint16_t color);
void gfx_write_char(char c);
int16_t gfx_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);

#endif // GFX_H