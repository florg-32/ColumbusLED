#include "peripherals.h"

#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/exti.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/timer.h"
#include "libopencm3/stm32/pwr.h"
#include "libopencm3/cm3/scb.h"

void init_gpio(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_AFIO);

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    nvic_enable_irq(NVIC_EXTI0_IRQ);
    nvic_set_priority(NVIC_EXTI0_IRQ, 16);
    exti_select_source(EXTI0, GPIOA);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI0);

    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6 | GPIO7);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO0);
}

void init_timer(void)
{
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_reset_pulse(RST_TIM2);
    rcc_periph_reset_pulse(RST_TIM3);
    rcc_periph_reset_pulse(RST_TIM4);

    // Debounce Timer
    timer_set_prescaler(TIM2, 48000); // = 1 kHz
    timer_set_period(TIM2, 500);      // 250ms threshold for double tap or hold
    timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_FROZEN);
    timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_FROZEN);
    timer_set_oc_value(TIM2, TIM_OC1, 80); // 50ms for debounce
    timer_set_oc_value(TIM2, TIM_OC2, 300);
    timer_one_shot_mode(TIM2);
    timer_generate_event(TIM2, TIM_EGR_UG); // Generate update once before enabling irqs
    timer_clear_flag(TIM2, TIM_SR_UIF);
    timer_enable_irq(TIM2, TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_UIE);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    // PWM Timer
    timer_set_prescaler(TIM3, 480); // = 100 kHz
    timer_set_period(TIM3, 1000);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC2, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);

    timer_set_oc_value(TIM3, TIM_OC1, 0);
    timer_set_oc_value(TIM3, TIM_OC2, 0);
    timer_set_oc_value(TIM3, TIM_OC3, 0);

    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_oc_output(TIM3, TIM_OC2);
    timer_enable_oc_output(TIM3, TIM_OC3);

    // Overheat Timer (might not be 100% necessary)
    timer_set_prescaler(TIM4, 48000); // = 1 kHz
    timer_set_period(TIM4, 60000); // = 60s
    timer_one_shot_mode(TIM4);
    timer_generate_event(TIM4, TIM_EGR_UG);
    timer_clear_flag(TIM4, TIM_SR_UIF);
    timer_enable_irq(TIM4, TIM_DIER_UIE);
    nvic_enable_irq(NVIC_TIM4_IRQ);
}

void setup_standby_mode(void)
{
    PWR_CR |= PWR_CR_CWUF;      // clear wakeup flag
    SCB_SCR |= SCB_SCR_SLEEPDEEP; // now wait for wfi
    PWR_CR |= PWR_CR_PDDS;        // power down deep sleep
    PWR_CSR |= PWR_CSR_EWUP;      // enable wakeup pin
}

void set_all_pins_analog_in(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    GPIOA_CRL = 0;
    GPIOB_CRL = 0;
    GPIOC_CRL = 0;
}