#ifndef IMG_TIME_H
#define IMG_TIME_H

#include <ml_ssd1306.h>
static const uint8_t img_time_data[8] = {
    0x18, 0x66, 0x52, 0x91, 0x99, 0x4E, 0x66, 0x18
};

static const ml_ssd1306_image_t img_time = {
    .width = 8,
    .height = 8,
    .length = sizeof(img_time_data),
    .data = img_time_data,
};

#endif // IMG_TIME_H
