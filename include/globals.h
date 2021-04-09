#pragma once

#include <stdint.h>
#include "light.h"

void delay(unsigned int cycles);
void short_delay(void);
void long_delay(void);

extern uint16_t brightness;
extern uint16_t max_brightness;
extern const color_t WARM;
extern const color_t OFF;
