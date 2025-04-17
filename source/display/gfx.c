// gfx.c - By Another(@Do3p3iri)

#include "display/gfx.h"
#include <string.h>

// Configuration
static uint16_t _width;
static uint16_t _height;
static uint8_t _rotation;
static int16_t WIDTH, HEIGHT; // Original dimensions

static const GFXfont *_font = NULL;
static uint16_t _text_color = 0xFFFF;       // Default: white
static uint16_t _text_bg_color = 0x0000;    // Default: black
static uint8_t _text_size = 1;
static bool _wrap = true;
static int16_t _cursor_x = 0;
static int16_t _cursor_y = 0;

// Set the font to use for text rendering
void gfx_set_font(const GFXfont *f) {
    _font = f;
}

// Set the text size (scaling factor)
void gfx_set_text_size(uint8_t s) {
    if (s > 0)
        _text_size = s;
}

// Set the text color
void gfx_set_text_color(uint16_t c) {
    _text_color = c;
    _text_bg_color = c; // Same as foreground for transparent background
}

// Set the text color with background
void gfx_set_text_color_bg(uint16_t c, uint16_t bg) {
    _text_color = c;
    _text_bg_color = bg;
}

// Set text wrapping mode
void gfx_set_text_wrap(bool w) {
    _wrap = w;
}

// Set cursor position
void gfx_set_cursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

// Draw a single character
int16_t gfx_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (!_font) {
        // Handle case for built-in font (not implemented here)
        return 0;
    }
    
    // Check if character is valid
    if (c < _font->first || c > _font->last)
        c = '?'; // Replace with question mark if out of range
    
    // Get glyph information
    c -= _font->first;
    GFXglyph *glyph = &(((GFXglyph *)_font->glyph)[c]);
    uint8_t *bitmap = (uint8_t *)_font->bitmap;
    
    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    uint8_t xx, yy, bits = 0, bit = 0;
    int16_t xo16 = xo, yo16 = yo;
    
    // Handle size scaling
    if (size > 1) {
        xo16 *= size;
        yo16 *= size;
    }
    
    // Draw character
    for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
            if (!(bit++ & 7)) {
                bits = bitmap[bo++];
            }
            if (bits & 0x80) {
                if (size == 1) {
                    gfx_draw_pixel(x + xo16 + xx, y + yo16 + yy, color);
                } else {
                    gfx_fill_rect(x + (xo16 + xx) * size, y + (yo16 + yy) * size, size, size, color);
                }
            } else if (bg != color) {
                if (size == 1) {
                    gfx_draw_pixel(x + xo16 + xx, y + yo16 + yy, bg);
                } else {
                    gfx_fill_rect(x + (xo16 + xx) * size, y + (yo16 + yy) * size, size, size, bg);
                }
            }
            bits <<= 1;
        }
    }
    
    // Return width of character
    return glyph->xAdvance * size;
}

// Calculate text bounds
void gfx_get_text_bounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    if (!_font) {
        // Handle case for built-in font (not implemented here)
        *x1 = x;
        *y1 = y;
        *w = 0;
        *h = 0;
        return;
    }
    
    *x1 = x;
    *y1 = y;
    *w = 0;
    *h = 0;
    
    int16_t minx = x;
    int16_t miny = y;
    int16_t maxx = x;
    int16_t maxy = y;
    
    uint16_t gw, gh;
    int16_t gx1, gy1;
    uint16_t gx2, gy2;
    
    while (*str) {
        unsigned char c = *str++;
        
        if (c < _font->first || c > _font->last) {
            c = '?';
        }
        
        c -= _font->first;
        GFXglyph *glyph = &(((GFXglyph *)_font->glyph)[c]);
        
        gw = glyph->width * _text_size;
        gh = glyph->height * _text_size;
        
        gx1 = x + glyph->xOffset * _text_size;
        gy1 = y + glyph->yOffset * _text_size;
        gx2 = gx1 + gw - 1;
        gy2 = gy1 + gh - 1;
        
        if (gx1 < minx) minx = gx1;
        if (gy1 < miny) miny = gy1;
        if (gx2 > maxx) maxx = gx2;
        if (gy2 > maxy) maxy = gy2;
        
        x += glyph->xAdvance * _text_size;
    }
    
    *x1 = minx;
    *y1 = miny;
    *w = maxx - minx + 1;
    *h = maxy - miny + 1;
}

// Write a single character at the current cursor position
void gfx_write_char(char c) {
    if (!_font) {
        // Handle case for built-in font (not implemented here)
        return;
    }
    
    if (c == '\n') {
        // Handle newline
        _cursor_y += _font->yAdvance * _text_size;
        _cursor_x = 0;
    } else if (c == '\r') {
        // Carriage return
        _cursor_x = 0;
    } else {
        // Draw the character
        int16_t xAdvance = gfx_draw_char(_cursor_x, _cursor_y, c, _text_color, _text_bg_color, _text_size);
        _cursor_x += xAdvance;
        
        // Handle text wrapping
        if (_wrap && (_cursor_x > _width - (_font->yAdvance * _text_size))) {
            _cursor_y += _font->yAdvance * _text_size;
            _cursor_x = 0;
        }
    }
}

// Print a string of text
void gfx_print(const char *str) {
    while (*str) {
        gfx_write_char(*str++);
    }
}

// Swap function for integers
static void _swap_int16(int16_t *a, int16_t *b) {
    int16_t t = *a;
    *a = *b;
    *b = t;
}

// Initialize the GFX library
void gfx_init(uint16_t width, uint16_t height, uint8_t rotation) {
    _width = width;
    _height = height;
    WIDTH = width;
    HEIGHT = height;
    _rotation = rotation;
    display_init(width, height, rotation);
}

// Draw a single pixel
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= _width || y >= _height) return;
    display_draw_pixel(x, y, color);
}

// Draw a 1-bit bitmap (each bit represents a pixel) with specified color for '1' bits
void gfx_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    int16_t byteWidth = (w + 7) / 8; // Bytes per row
    uint8_t byte = 0;

    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte >>= 1;
            } else {
                byte = bitmap[j * byteWidth + i / 8];
            }
            if (byte & 0x01) {
                gfx_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

// Draw a 1-bit bitmap with background color (for '0' bits)
void gfx_draw_bitmap_bg(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    int16_t byteWidth = (w + 7) / 8; // Bytes per row
    uint8_t byte = 0;

    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte >>= 1;
            } else {
                byte = bitmap[j * byteWidth + i / 8];
            }
            if (byte & 0x01) {
                gfx_draw_pixel(x + i, y + j, color);
            } else {
                gfx_draw_pixel(x + i, y + j, bg);
            }
        }
    }
}

// Draw a 16-bit (RGB565) color bitmap
void gfx_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
    // Bounds checking
    if (x >= _width || y >= _height || (x + w - 1) < 0 || (y + h - 1) < 0) return;

    // Crop bitmap if it extends beyond display boundaries
    int16_t x1 = x;
    int16_t y1 = y;
    int16_t x2 = x + w - 1;
    int16_t y2 = y + h - 1;
    
    // Adjust coordinates if partially off-screen
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= _width) x2 = _width - 1;
    if (y2 >= _height) y2 = _height - 1;
    
    int16_t bw = x2 - x1 + 1; // Adjusted width
    int16_t bh = y2 - y1 + 1; // Adjusted height
    
    // Calculate offsets into the bitmap
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    
    // Draw visible portion of bitmap
    for (int16_t j = 0; j < bh; j++) {
        for (int16_t i = 0; i < bw; i++) {
            gfx_draw_pixel(x1 + i, y1 + j, bitmap[(dy + j) * w + (dx + i)]);
        }
    }
}

// Draw a 16-bit color bitmap with 1-bit mask
// Where mask bit is 0, don't draw pixel
void gfx_draw_rgb_bitmap_with_mask(int16_t x, int16_t y, const uint16_t *bitmap, const uint8_t *mask, int16_t w, int16_t h) {
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;

    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte >>= 1;
            } else {
                byte = mask[j * byteWidth + i / 8];
            }
            
            // Draw pixel only if corresponding mask bit is 1
            if (byte & 0x01) {
                gfx_draw_pixel(x + i, y + j, bitmap[j * w + i]);
            }
        }
    }
}

void gfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Special cases for vertical and horizontal lines
    if (x0 == x1) {
        if (y0 > y1) _swap_int16(&y0, &y1);
        gfx_draw_fast_v_line(x0, y0, y1 - y0 + 1, color);
    } else if (y0 == y1) {
        if (x0 > x1) _swap_int16(&x0, &x1);
        gfx_draw_fast_h_line(x0, y0, x1 - x0 + 1, color);
    } else {
        // Handle general case
        int16_t steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep) {
            _swap_int16(&x0, &y0);
            _swap_int16(&x1, &y1);
        }
        if (x0 > x1) {
            _swap_int16(&x0, &x1);
            _swap_int16(&y0, &y1);
        }

        int16_t dx, dy;
        dx = x1 - x0;
        dy = abs(y1 - y0);

        int16_t err = dx / 2;
        int16_t ystep;

        if (y0 < y1) {
            ystep = 1;
        } else {
            ystep = -1;
        }

        for (; x0 <= x1; x0++) {
            if (steep) {
                gfx_draw_pixel(y0, x0, color);
            } else {
                gfx_draw_pixel(x0, y0, color);
            }
            err -= dy;
            if (err < 0) {
                y0 += ystep;
                err += dx;
            }
        }
    }
}

// Draw a vertical line (optimized)
void gfx_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color) {
    // Bounds check
    if (x < 0 || x >= _width || y >= _height) return;
    if (y < 0) {
        h += y;
        y = 0;
    }
    if ((y + h - 1) >= _height) h = _height - y;
    if (h <= 0) return;
    
    // Prepare a buffer with the color
    uint16_t *buffer = (uint16_t *)osal_mem_alloc(h * sizeof(uint16_t));
    if (buffer) {
        for (int i = 0; i < h; i++) {
            buffer[i] = color;
        }
        display_fill_window(x, y, x, y + h - 1, buffer, h);
        osal_mem_free(buffer);
    } else {
        // Fallback to pixel-by-pixel if malloc fails
        for (int16_t i = 0; i < h; i++) {
            gfx_draw_pixel(x, y + i, color);
        }
    }
}

// Draw a horizontal line (optimized)
void gfx_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color) {
    // Bounds check
    if (y < 0 || y >= _height || x >= _width) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if ((x + w - 1) >= _width) w = _width - x;
    if (w <= 0) return;
    
    // Prepare a buffer with the color
    uint16_t *buffer = (uint16_t *)osal_mem_alloc(w * sizeof(uint16_t));
    if (buffer) {
        for (int i = 0; i < w; i++) {
            buffer[i] = color;
        }
        display_fill_window(x, y, x + w - 1, y, buffer, w);
        osal_mem_free(buffer);
    } else {
        // Fallback to pixel-by-pixel if malloc fails
        for (int16_t i = 0; i < w; i++) {
            gfx_draw_pixel(x + i, y, color);
        }
    }
}

// Draw a rectangle outline
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    gfx_draw_fast_h_line(x, y, w, color);         // Top
    gfx_draw_fast_h_line(x, y + h - 1, w, color); // Bottom
    gfx_draw_fast_v_line(x, y, h, color);         // Left
    gfx_draw_fast_v_line(x + w - 1, y, h, color); // Right
}

// Fill a rectangle
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    // Bounds check
    if (x >= _width || y >= _height || w <= 0 || h <= 0) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if ((x + w - 1) >= _width) w = _width - x;
    if ((y + h - 1) >= _height) h = _height - y;
    
    // Prepare a buffer with the color
    uint32_t total_pixels = (uint32_t)w * h;
    uint16_t *buffer = (uint16_t *)osal_mem_alloc(total_pixels * sizeof(uint16_t));
    if (buffer) {
        for (uint32_t i = 0; i < total_pixels; i++) {
            buffer[i] = color;
        }
        display_fill_window(x, y, x + w - 1, y + h - 1, buffer, total_pixels);
        osal_mem_free(buffer);
    } else {
        // Fallback to line-by-line if malloc fails
        for (int16_t i = 0; i < h; i++) {
            gfx_draw_fast_h_line(x, y + i, w, color);
        }
    }
}

// Draw a circle helper (for rounded rectangles and other partial circles)
void gfx_draw_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if (cornername & 0x4) {
            gfx_draw_pixel(x0 + x, y0 + y, color);
            gfx_draw_pixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            gfx_draw_pixel(x0 + x, y0 - y, color);
            gfx_draw_pixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            gfx_draw_pixel(x0 - y, y0 + x, color);
            gfx_draw_pixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            gfx_draw_pixel(x0 - y, y0 - x, color);
            gfx_draw_pixel(x0 - x, y0 - y, color);
        }
    }
}

// Fill a circle helper (for filled rounded rectangles)
void gfx_fill_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++; // Avoid some +1's in the loop

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        if (x < (y + 1)) {
            if (cornername & 0x1)
                gfx_draw_fast_v_line(x0 + x, y0 - y, 2 * y + delta, color);
            if (cornername & 0x2)
                gfx_draw_fast_v_line(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (cornername & 0x1)
                gfx_draw_fast_v_line(x0 + py, y0 - px, 2 * px + delta, color);
            if (cornername & 0x2)
                gfx_draw_fast_v_line(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

// Draw a circle
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    gfx_draw_pixel(x0, y0 + r, color);
    gfx_draw_pixel(x0, y0 - r, color);
    gfx_draw_pixel(x0 + r, y0, color);
    gfx_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        gfx_draw_pixel(x0 + y, y0 + x, color);
        gfx_draw_pixel(x0 - y, y0 + x, color);
        gfx_draw_pixel(x0 + y, y0 - x, color);
        gfx_draw_pixel(x0 - y, y0 - x, color);
    }
}

// Fill a circle
void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    gfx_draw_fast_v_line(x0, y0 - r, 2 * r + 1, color);
    gfx_fill_circle_helper(x0, y0, r, 3, 0, color);
}

// Draw a rounded rectangle
void gfx_draw_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    // Check and constrain the radius size
    int16_t max_radius = ((w < h) ? w : h) / 2;
    if (radius > max_radius) radius = max_radius;
    
    // Draw the straight edges
    gfx_draw_fast_h_line(x0 + radius, y0, w - 2 * radius, color);         // Top
    gfx_draw_fast_h_line(x0 + radius, y0 + h - 1, w - 2 * radius, color); // Bottom
    gfx_draw_fast_v_line(x0, y0 + radius, h - 2 * radius, color);         // Left
    gfx_draw_fast_v_line(x0 + w - 1, y0 + radius, h - 2 * radius, color); // Right
    
    // Draw the four corners
    gfx_draw_circle_helper(x0 + radius, y0 + radius, radius, 1, color);
    gfx_draw_circle_helper(x0 + w - radius - 1, y0 + radius, radius, 2, color);
    gfx_draw_circle_helper(x0 + w - radius - 1, y0 + h - radius - 1, radius, 4, color);
    gfx_draw_circle_helper(x0 + radius, y0 + h - radius - 1, radius, 8, color);
}

// Fill a rounded rectangle
void gfx_fill_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    // Check and constrain the radius size
    int16_t max_radius = ((w < h) ? w : h) / 2;
    if (radius > max_radius) radius = max_radius;
    
    // Fill the center
    gfx_fill_rect(x0 + radius, y0, w - 2 * radius, h, color);
    
    // Fill the rounded corners
    gfx_fill_circle_helper(x0 + w - radius - 1, y0 + radius, radius, 1, h - 2 * radius - 1, color);
    gfx_fill_circle_helper(x0 + radius, y0 + radius, radius, 2, h - 2 * radius - 1, color);
}

// Draw a triangle
void gfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    gfx_draw_line(x0, y0, x1, y1, color);
    gfx_draw_line(x1, y1, x2, y2, color);
    gfx_draw_line(x2, y2, x0, y0, color);
}

// Fill a triangle
void gfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16(&y0, &y1);
        _swap_int16(&x0, &x1);
    }
    if (y1 > y2) {
        _swap_int16(&y2, &y1);
        _swap_int16(&x2, &x1);
    }
    if (y0 > y1) {
        _swap_int16(&y0, &y1);
        _swap_int16(&x0, &x1);
    }

    // Special case for flat-bottomed triangle
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        gfx_draw_fast_h_line(a, y0, b - a + 1, color);
        return;
    }

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2. If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0 error)
    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // Include y1 scanline or skip it
    last = (y1 == y2) ? y1 : y1 - 1;

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        
        if (a > b) _swap_int16(&a, &b);
        gfx_draw_fast_h_line(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 1-2 and 0-2. This loop is skipped if y1=y2
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        
        if (a > b) _swap_int16(&a, &b);
        gfx_draw_fast_h_line(a, y, b - a + 1, color);
    }
}

// Set the _rotation of the display
void gfx_set_rotation(uint8_t r) {
    _rotation = r % 4;
    switch (_rotation) {
    case 0:
    case 2:
        _width = WIDTH;
        _height = HEIGHT;
        break;
    case 1:
    case 3:
        _width = HEIGHT;
        _height = WIDTH;
        break;
    }
}

// Fill the entire screen with a color
void gfx_fill_screen(uint16_t color) {
    display_fill_screen(color);
}