#pragma once

#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

void set_color(color_t col);
void cycle_colors(void);

int step_brightness(int16_t step);

void update_pwm(void);