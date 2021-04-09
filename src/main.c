#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/exti.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/timer.h"
#include "libopencm3/stm32/dbgmcu.h"
#include "peripherals.h"
#include "cdcacm.h"
#include "globals.h"
#include "light.h"

int main(void)
{
    rcc_clock_setup_in_hsi_out_48mhz();
    rcc_periph_clock_enable(RCC_PWR);

    set_all_pins_analog_in();
    init_gpio();
    init_timer();
    timer_enable_counter(TIM3);
    timer_enable_counter(TIM4);

    set_color(WARM);
    while (step_brightness(10)) // ramp up brightness
    {
        short_delay();
    }

    while (1)
    {
        __asm__("wfi");
    }
}

bool recent_press = false;
void exti0_isr(void)
{
    exti_reset_request(EXTI0);
    gpio_toggle(GPIOC, GPIO13);

    if (recent_press) // = double tap
    {
        timer_disable_counter(TIM2);
        while (gpio_get(GPIOA, GPIO0))
            ;
        while (step_brightness(-15))
        {
            short_delay();
        }
        setup_standby_mode();
        short_delay();
        short_delay();
    }
    else
    {
        recent_press = true;
        exti_disable_request(EXTI0);
        timer_enable_counter(TIM2);
    }
}

int16_t dimming_step = 8;
void tim2_isr(void)
{
    if (timer_get_flag(TIM2, TIM_SR_CC1IF)) // 80ms after button press
    {
        timer_clear_flag(TIM2, TIM_SR_CC1IF);
        exti_enable_request(EXTI0);
    }
    else if (timer_get_flag(TIM2, TIM_SR_CC2IF)) // 150ms
    {
        timer_clear_flag(TIM2, TIM_SR_CC2IF);
        if (!gpio_get(GPIOA, GPIO0))
        {
            cycle_colors(); // cycle only if pin was released
        }
    }
    else if (timer_get_flag(TIM2, TIM_SR_UIF)) // 250ms
    {
        timer_clear_flag(TIM2, TIM_SR_UIF);
        recent_press = false;
        if (gpio_get(GPIOA, GPIO0)) // if holding
        {
            dimming_step = -dimming_step;
            while (gpio_get(GPIOA, GPIO0))
            {
                step_brightness(dimming_step);
                short_delay();
            }
            long_delay(); // debounce
        }
    }
}

void tim4_isr(void)
{
    timer_clear_flag(TIM4, TIM_SR_UIF);
    max_brightness = 800;
    while (brightness > max_brightness)
    {
        brightness -= 5;
        update_pwm();
        short_delay();
    }
}

void delay(unsigned int cycles)
{
    for (unsigned int i = 0; i < cycles; i++)
    {
        __asm__("nop");
    }
}

void short_delay(void)
{
    delay(100000);
}
void long_delay(void)
{
    delay(1000000);
}