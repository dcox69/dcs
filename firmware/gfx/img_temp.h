#ifndef IMG_TEMP_H
#define IMG_TEMP_H

#include <ml_ssd1306.h>
static const uint8_t img_temp_data[8] = {
    0x13, 0x28, 0x2B, 0x28, 0x6C, 0xC6, 0xC6, 0x7C
};

static const ml_ssd1306_image_t img_temp = {
    .width = 8,
    .height = 8,
    .length = sizeof(img_temp_data),
    .data = img_temp_data,
};

#endif // IMG_TEMP_H
