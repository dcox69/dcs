#pragma once

#include <ml_common.h>
#include <ml_i2c.h>

#if defined(ML_PLATFORM_PICO)
#include <pico/stdlib.h>
#endif

#define ML_SSD1306_FLAG_BOXED   0x01
#define ML_SSD1306_FLAG_VCENTER 0x02
#define ML_SSD1306_FLAG_HCENTER 0x04
#define ML_SSD1306_FLAG_INVERT  0x08

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t pages;
    ml_i2c_t i2c;
    bool     external_vcc;
    uint8_t *buff;
    size_t   buff_size;
    void *(*alloc)(size_t size);
    void (*free)(void *p);
} ml_ssd1306_t;

typedef struct
{
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;
    ml_i2c_port_t i2c_port;
    bool          external_vcc;
    void *(*alloc)(size_t size);
    void (*free)(void *p);
} ml_ssd1306_config_t;

typedef struct
{
    const uint8_t *data;
    uint8_t        width;
    uint8_t        height;
    uint8_t        first;
    uint8_t        count;
} ml_ssd1306_font_t;

typedef struct
{
    uint16_t       width;
    uint16_t       height;
    size_t         length;
    const uint8_t *data;
} ml_ssd1306_image_t;

// Prepare the controller, allocate the frame buffer, and run the power-on sequence
bool ml_ssd1306_init(ml_ssd1306_t *dev, ml_ssd1306_config_t *cfg);

// Enter low-power standby mode
void ml_ssd1306_power_off(ml_ssd1306_t *dev);

// Exit standby and enable panel output
void ml_ssd1306_power_on(ml_ssd1306_t *dev);

// Clear the in-memory frame buffer
void ml_ssd1306_clear(ml_ssd1306_t *dev);

// Invert display colors if inv is non-zero
void ml_ssd1306_invert(ml_ssd1306_t *dev, uint8_t inv);

// Flush the frame buffer to the display over I2C
void ml_ssd1306_show(ml_ssd1306_t *dev);

// Set contrast (brightness) to a value between 0 and 255
void ml_ssd1306_contrast(ml_ssd1306_t *p, uint8_t val);

// Set a single pixel in the frame buffer
void ml_ssd1306_draw_pixel(ml_ssd1306_t *dev, uint16_t x, uint16_t y);

// Clear a single pixel in the frame buffer
void ml_ssd1306_clear_pixel(ml_ssd1306_t *dev, uint16_t x, uint16_t y);

// Draw a straight line
void ml_ssd1306_draw_line(ml_ssd1306_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Outline an axis-aligned rectangle
void ml_ssd1306_draw_rect(ml_ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Outline an ellipse centered at the given coordinates (can be negative)
void ml_ssd1306_draw_ellipse(ml_ssd1306_t *dev, int16_t x_center, int16_t y_center, uint16_t r_horiz, uint16_t r_vert);

// Outline a circle at the given coordinates
void ml_ssd1306_draw_circle(ml_ssd1306_t *dev, int16_t x_center, int16_t y_center, uint16_t r);

// Fill an axis-aligned rectangle
void ml_ssd1306_fill_rect(ml_ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Clear an axis-aligned rectangle
void ml_ssd1306_clear_rect(ml_ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Render a null-terminated utf-8 string (using 437 codepage conversion) using the supplied bitmap font
void ml_ssd1306_draw_str(ml_ssd1306_t *display, int x, int y, const char *text, const ml_ssd1306_font_t *font);

// Render a null-terminated utf-8 string (using 437 codepage conversion) using the supplied bitmap font & flags
void ml_ssd1306_draw_str_ex(ml_ssd1306_t            *dev,
                            uint8_t                  x,
                            uint8_t                  y,
                            uint8_t                  w,
                            uint8_t                  h,
                            const char              *str,
                            const ml_ssd1306_font_t *font,
                            uint8_t                  flags);

// Copy a monochrome bitmap into the frame buffer
void ml_ssd1306_draw_image(ml_ssd1306_t *dev, uint16_t x, uint16_t y, const ml_ssd1306_image_t *image);

// Start horizontal scroll effect across a page range
void ml_ssd1306_scroll_horiz(ml_ssd1306_t *dev, bool right, uint8_t start_page, uint8_t end_page, uint8_t speed);

// Halt any active horizontal scroll
void ml_ssd1306_scroll_horiz_stop(ml_ssd1306_t *dev);

// Scroll vertically by one position
void ml_ssd1306_scroll_row_vert(ml_ssd1306_t *dev, bool down);
