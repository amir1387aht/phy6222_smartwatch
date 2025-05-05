#include "display/gfx.h"

// Configuration
static uint16_t _width;
static uint16_t _height;
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
    _cursor_y = y + _font->yAdvance * _text_size;
}

void gfx_draw_text(int16_t x, int16_t y, const char *str, uint16_t color) {
    uint16_t orig_color = _text_color;
    int16_t orig_x = _cursor_x;
    int16_t orig_y = _cursor_y;
    
    _text_color = color;
    gfx_set_cursor(x, y);
    gfx_print(str);
    
    // Restore original state
    _text_color = orig_color;
    _cursor_x = orig_x;
    _cursor_y = orig_y;
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
    
    // Calculate actual pixel positions
    int16_t x_pixel, y_pixel;
    
    // Draw character
    for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
            if (!(bit++ & 7)) {
                bits = bitmap[bo++];
            }
            
            // Calculate actual pixel position with scaling
            x_pixel = x + xo16 + (xx * size);
            y_pixel = y + yo16 + (yy * size);
            
            // Only draw if pixel is within display bounds
            if (bits & 0x80) {
                if (size == 1) {
                    if (x_pixel >= 0 && y_pixel >= 0 && x_pixel < _width && y_pixel < _height) {
                        gfx_draw_pixel(x_pixel, y_pixel, color);
                    }
                } else {
                    // For scaled text, check boundaries for the entire scaled pixel
                    if (x_pixel >= 0 && y_pixel >= 0 && x_pixel + size - 1 < _width && y_pixel + size - 1 < _height) {
                        gfx_fill_rect(x_pixel, y_pixel, size, size, color);
                    } else {
                        // Partial drawing for pixels at the edge of the display
                        for (uint8_t sx = 0; sx < size; sx++) {
                            for (uint8_t sy = 0; sy < size; sy++) {
                                if ((x_pixel + sx) >= 0 && (y_pixel + sy) >= 0 && 
                                    (x_pixel + sx) < _width && (y_pixel + sy) < _height) {
                                    gfx_draw_pixel(x_pixel + sx, y_pixel + sy, color);
                                }
                            }
                        }
                    }
                }
            } else if (bg != color) {
                if (size == 1) {
                    if (x_pixel >= 0 && y_pixel >= 0 && x_pixel < _width && y_pixel < _height) {
                        gfx_draw_pixel(x_pixel, y_pixel, bg);
                    }
                } else {
                    // For scaled text, check boundaries for the entire scaled pixel
                    if (x_pixel >= 0 && y_pixel >= 0 && x_pixel + size - 1 < _width && y_pixel + size - 1 < _height) {
                        gfx_fill_rect(x_pixel, y_pixel, size, size, bg);
                    } else {
                        // Partial drawing for pixels at the edge of the display
                        for (uint8_t sx = 0; sx < size; sx++) {
                            for (uint8_t sy = 0; sy < size; sy++) {
                                if ((x_pixel + sx) >= 0 && (y_pixel + sy) >= 0 && 
                                    (x_pixel + sx) < _width && (y_pixel + sy) < _height) {
                                    gfx_draw_pixel(x_pixel + sx, y_pixel + sy, bg);
                                }
                            }
                        }
                    }
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
        return;
    }
    
    if (c == '\n') {
        // Handle newline - move down by the correct amount based on font size
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
        if (_wrap && (_cursor_x > (_width - (_font->yAdvance * _text_size)))) {
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

// Draw a scaled bitmap
void gfx_draw_scaled_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, int16_t scale, uint16_t color) {
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;

    if (scale < 1) scale = 1;

    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte >>= 1;
            } else {
                byte = bitmap[j * byteWidth + i / 8];
            }
            
            if (byte & 0x01) {
                gfx_fill_rect(x + i * scale, y + j * scale, scale, scale, color);
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

// Fill the entire screen with a color
void gfx_fill_screen(uint16_t color) {
    display_fill_screen(color);
}

int16_t gfx_get_width(void) {
    return _width;
}

int16_t gfx_get_height(void) {
    return _height;
}

// Additional text functions
const GFXfont* gfx_get_current_font(void) {
    return _font;
}

uint8_t gfx_get_text_size(void) {
    return _text_size;
}

bool gfx_get_text_wrap(void) {
    return _wrap;
}

int16_t gfx_get_cursor_x(void) {
    return _cursor_x;
}

int16_t gfx_get_cursor_y(void) {
    return _cursor_y;
}

void gfx_println(const char *str) {
    gfx_print(str);
    _cursor_x = 0;
    _cursor_y += _font ? (_font->yAdvance * _text_size) : (8 * _text_size);
}

void gfx_clear(void) {
    gfx_fill_screen(0); // Default to black
    gfx_set_cursor(0, 0);
}

// Draw a dashed line
void gfx_draw_dashed_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, uint8_t dash_len, uint8_t gap_len) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16(&x0, &y0);
        _swap_int16(&x1, &y1);
    }

    if (x0 > x1) {
        _swap_int16(&x0, &x1);
        _swap_int16(&y0, &y1);
    }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    int16_t segment_length = 0;
    bool draw = true;

    for (int16_t x = x0, y = y0; x <= x1; x++) {
        if (draw) {
            if (steep) {
                gfx_draw_pixel(y, x, color);
            } else {
                gfx_draw_pixel(x, y, color);
            }
        }

        segment_length++;
        
        if ((draw && segment_length >= dash_len) || (!draw && segment_length >= gap_len)) {
            segment_length = 0;
            draw = !draw;
        }

        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
}

#define SEGMENT_DEG 6.0f

void gfx_draw_arc(int16_t x0, int16_t y0, int16_t r, int16_t start_angle, int16_t end_angle, 
                uint8_t thickness, uint16_t color)
{
    // Normalize angles
    start_angle %= 360; if(start_angle < 0) start_angle += 360;
    end_angle   %= 360; if(end_angle   < 0) end_angle   += 360;
    if(end_angle < start_angle)       end_angle += 360;
    if(end_angle - start_angle > 360) end_angle  = start_angle + 360;

    // Compute segment count and radians
    float spanDeg = end_angle - start_angle;
    int   N       = (int)roundf(spanDeg / SEGMENT_DEG);
    if(N < 1) N = 1;
    double spanRad = spanDeg * (M_PI/180.0);
    double a0      = start_angle * (M_PI/180.0);
    int16_t rin    = r - (thickness - 1);
    if(rin < 0) rin = 0;

    // Preallocate point arrays on the stack (N is small, e.g. <=60)
    int16_t xO[N+1], yO[N+1], xI[N+1], yI[N+1];

    // Compute boundary points exactly
    for(int i = 0; i <= N; i++) {
        double ang = a0 + spanRad * i / N;
        xO[i] = x0 + (int16_t)lround(r   * cos(ang));
        yO[i] = y0 + (int16_t)lround(r   * sin(ang));
        xI[i] = x0 + (int16_t)lround(rin * cos(ang));
        yI[i] = y0 + (int16_t)lround(rin * sin(ang));
    }

    // Fill each trapezoid with two triangles
    for(int i = 0; i < N; i++) {
        gfx_fill_triangle(xO[i],  yO[i],
                        xO[i+1],yO[i+1],
                        xI[i],  yI[i], color);
        gfx_fill_triangle(xI[i],  yI[i],
                        xO[i+1],yO[i+1],
                        xI[i+1],yI[i+1], color);
    }
}

// Draw an ellipse
void gfx_draw_ellipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color) {
    if (rx < 2) rx = 2;
    if (ry < 2) ry = 2;
    
    int16_t x, y;
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t fx2 = 4 * rx2;
    int32_t fy2 = 4 * ry2;
    int32_t s;
    
    // First set of points, y' > -1
    for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        
        if (s >= 0) {
            s += fx2 * (1 - y);
            y--;
        }
        s += ry2 * ((4 * x) + 6);
    }
    
    // Second set of points, y' <= -1
    for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
        gfx_draw_pixel(x0 + x, y0 + y, color);
        gfx_draw_pixel(x0 - x, y0 + y, color);
        gfx_draw_pixel(x0 + x, y0 - y, color);
        gfx_draw_pixel(x0 - x, y0 - y, color);
        
        if (s >= 0) {
            s += fy2 * (1 - x);
            x--;
        }
        s += rx2 * ((4 * y) + 6);
    }
}

// Fill an ellipse
void gfx_fill_ellipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color) {
    if (rx < 2) rx = 2;
    if (ry < 2) ry = 2;
    
    int16_t x, y;
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t fx2 = 4 * rx2;
    int32_t fy2 = 4 * ry2;
    int32_t s;
    
    // First set of points, y' > -1
    for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
        gfx_draw_fast_h_line(x0 - x, y0 - y, 2 * x + 1, color);
        gfx_draw_fast_h_line(x0 - x, y0 + y, 2 * x + 1, color);
        
        if (s >= 0) {
            s += fx2 * (1 - y);
            y--;
        }
        s += ry2 * ((4 * x) + 6);
    }
    
    // Second set of points, y' <= -1
    for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
        gfx_draw_fast_h_line(x0 - x, y0 - y, 2 * x + 1, color);
        gfx_draw_fast_h_line(x0 - x, y0 + y, 2 * x + 1, color);
        
        if (s >= 0) {
            s += fy2 * (1 - x);
            x--;
        }
        s += rx2 * ((4 * y) + 6);
    }
}

// Draw a polygon
void gfx_draw_polygon(int16_t *points, uint8_t num_points, uint16_t color) {
    if (num_points < 3) return; // Need at least 3 points for a polygon
    
    for (uint8_t i = 0; i < num_points - 1; i++) {
        gfx_draw_line(points[i*2], points[i*2+1], points[(i+1)*2], points[(i+1)*2+1], color);
    }
    // Connect last point back to first
    gfx_draw_line(points[(num_points-1)*2], points[(num_points-1)*2+1], points[0], points[1], color);
}

// Fill a polygon
// This uses a simple scanline algorithm
void gfx_fill_polygon(int16_t *points, uint8_t num_points, uint16_t color) {
    if (num_points < 3) return; // Need at least 3 points for a polygon
    
    // Find min and max Y values
    int16_t min_y = points[1];
    int16_t max_y = points[1];
    
    for (uint8_t i = 1; i < num_points; i++) {
        if (points[i*2+1] < min_y) min_y = points[i*2+1];
        if (points[i*2+1] > max_y) max_y = points[i*2+1];
    }
    
    // Allocate space for nodes (intersections with scanline)
    int16_t *nodes = (int16_t*)osal_mem_alloc(num_points * sizeof(int16_t));
    if (!nodes) return; // Memory allocation failed
    
    // Process scanlines from min_y to max_y
    for (int16_t y = min_y; y <= max_y; y++) {
        // Build node list
        int nodes_count = 0;
        int16_t j = num_points - 1;
        
        for (int16_t i = 0; i < num_points; i++) {
            if ((points[i*2+1] < y && points[j*2+1] >= y) || (points[j*2+1] < y && points[i*2+1] >= y)) {
                nodes[nodes_count++] = points[i*2] + 
                    (y - points[i*2+1]) * (points[j*2] - points[i*2]) / (points[j*2+1] - points[i*2+1]);
            }
            j = i;
        }
        
        // Sort nodes (simple bubble sort for small lists)
        for (int16_t i = 0; i < nodes_count - 1; i++) {
            for (int16_t j = 0; j < nodes_count - 1 - i; j++) {
                if (nodes[j] > nodes[j+1]) {
                    int16_t temp = nodes[j];
                    nodes[j] = nodes[j+1];
                    nodes[j+1] = temp;
                }
            }
        }
        
        // Fill between node pairs
        for (int16_t i = 0; i < nodes_count; i += 2) {
            if (nodes[i] >= _width) break;
            if (nodes[i+1] > 0) {
                if (nodes[i] < 0) nodes[i] = 0;
                if (nodes[i+1] > _width) nodes[i+1] = _width;
                gfx_draw_fast_h_line(nodes[i], y, nodes[i+1] - nodes[i], color);
            }
        }
    }
    
    osal_mem_free(nodes);
}

// Scrolling functions
void gfx_scroll_up(uint16_t lines, uint16_t bg_color) {
    if (lines <= 0 || lines >= _height) {
        gfx_fill_screen(bg_color);
        return;
    }
    
    // Create a buffer to store the rows
    uint16_t *buffer = (uint16_t *)osal_mem_alloc(_width * sizeof(uint16_t));
    if (!buffer) return;
    
    // For each row, copy data from the row below it
    for (int16_t y = 0; y < _height - lines; y++) {
        // Read row from below
        for (int16_t x = 0; x < _width; x++) {
            // This should be replaced with actual display_read_pixel function
            // buffer[x] = display_read_pixel(x, y + lines);
            // Since we don't have access to read pixels, this might be a limitation
            buffer[x] = 0; // Default value
        }
        
        // Write to current row
        display_fill_window(0, y, _width - 1, y, buffer, _width);
    }
    
    // Fill the bottom rows with background color
    for (int16_t y = _height - lines; y < _height; y++) {
        for (int16_t x = 0; x < _width; x++) {
            buffer[x] = bg_color;
        }
        display_fill_window(0, y, _width - 1, y, buffer, _width);
    }
    
    osal_mem_free(buffer);
}

void gfx_scroll_down(uint16_t lines, uint16_t bg_color) {
    if (lines <= 0 || lines >= _height) {
        gfx_fill_screen(bg_color);
        return;
    }
    
    // Create a buffer to store the rows
    uint16_t *buffer = (uint16_t *)osal_mem_alloc(_width * sizeof(uint16_t));
    if (!buffer) return;
    
    // For each row, copy data from the row above it
    for (int16_t y = _height - 1; y >= lines; y--) {
        // Read row from above
        for (int16_t x = 0; x < _width; x++) {
            // This should be replaced with actual display_read_pixel function
            // buffer[x] = display_read_pixel(x, y - lines);
            buffer[x] = 0; // Default value
        }
        
        // Write to current row
        display_fill_window(0, y, _width - 1, y, buffer, _width);
    }
    
    // Fill the top rows with background color
    for (int16_t y = 0; y < lines; y++) {
        for (int16_t x = 0; x < _width; x++) {
            buffer[x] = bg_color;
        }
        display_fill_window(0, y, _width - 1, y, buffer, _width);
    }
    
    osal_mem_free(buffer);
}