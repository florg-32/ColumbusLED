#include "light.h"
#include "globals.h"

#include "libopencm3/stm32/timer.h"

const color_t WHITE = {255, 255, 255};
const color_t WARM = {255, 100, 4};
const color_t RED = {255, 0, 0};
const color_t BLUE = {0, 0, 255};
const color_t PURPLE = {180, 0, 255};
const color_t DARKGREEN = {80, 220, 0};
const color_t CYAN = {0, 170, 255};
const color_t OFF = {0, 0, 0};


color_t *cycle_cols[] = {
    &WHITE,
    &RED,
    &CYAN,
    &DARKGREEN,
    &PURPLE,
    &BLUE,
    &WARM
};

color_t current_color = {0, 0, 0};
void set_color(color_t c)
{
    current_color = c;
    update_pwm();
}

uint16_t max_brightness = 1000;
uint16_t brightness = 20;

/// increases or decreases the brightness according to the provided value
/// if it is in within 0 and max_brightness
/// @returns 1 if the step could be performed, 0 otherwise
int step_brightness(int16_t step)
{
    if (brightness + step > max_brightness || brightness + step < 20)
    {
        return false;
    }
    else
    {
        brightness += step;
        update_pwm();
    }
    return true;
}

void update_pwm(void)
{
    timer_set_oc_value(TIM3, TIM_OC1, (current_color.r * brightness) / 255);
    timer_set_oc_value(TIM3, TIM_OC2, (current_color.g * brightness) / 255);
    timer_set_oc_value(TIM3, TIM_OC3, (current_color.b * brightness) / 255);
}

void cycle_colors(void)
{
    static int index = -1;
    index++;
    transition_to_color(*cycle_cols[index % 7]);
}

void transition_to_color(color_t to)
{
    for (int i=0; i < 255; i++)
    {
        current_color.r = current_color.r + (to.r > current_color.r) - (to.r < current_color.r);
        current_color.g = current_color.g + (to.g > current_color.g) - (to.g < current_color.g);
        current_color.b = current_color.b + (to.b > current_color.b) - (to.b < current_color.b);
        update_pwm();
        delay(5000);
    }
    set_color(to); 
}
