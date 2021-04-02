#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/exti.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/timer.h"
#include "libopencm3/stm32/dbgmcu.h"
#include "power.h"
#include "cdcacm.h"

void init_gpio(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_AFIO);

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    nvic_enable_irq(NVIC_EXTI0_IRQ);
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
    rcc_periph_reset_pulse(RST_TIM2);
    rcc_periph_reset_pulse(RST_TIM3);

    // Debounce Timer
    timer_set_prescaler(TIM2, 48000); // = 1 kHz
    timer_set_period(TIM2, 500);      // 250ms threshold for double tap or hold
    timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_FROZEN);
    timer_set_oc_value(TIM2, TIM_OC1, 150); // 50ms for debounce
    timer_one_shot_mode(TIM2);
    timer_generate_event(TIM2, TIM_EGR_UG); // Generate update once before enabling irqs
    timer_clear_flag(TIM2, TIM_SR_UIF);
    timer_enable_irq(TIM2, TIM_DIER_CC1IE | TIM_DIER_UIE);
    nvic_enable_irq(NVIC_TIM2_IRQ);

    // PWM Timer
    timer_set_prescaler(TIM3, 480); // = 100 kHz
    timer_set_period(TIM3, 1000);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC2, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);

    timer_set_oc_value(TIM3, TIM_OC1, 20);
    timer_set_oc_value(TIM3, TIM_OC2, 500);
    timer_set_oc_value(TIM3, TIM_OC3, 1000);

    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_oc_output(TIM3, TIM_OC2);
    timer_enable_oc_output(TIM3, TIM_OC3);

    timer_enable_counter(TIM3);
}

int main(void)
{
    //rcc_clock_setup_in_hsi_out_24mhz();
    rcc_clock_setup_in_hsi_out_48mhz();
    cdcacm_init();
    while (!cdcacm_get_configuration()){}
    cdcacm_write_now("Hello\n", 6);

    init_gpio();
    init_timer();
    DBGMCU_CR |= DBGMCU_CR_TIM2_STOP;

    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    while (1)
    {
        __asm__("wfi");
    }
}

int16_t dimming_step = 5;
uint16_t dimming_value = 980;
bool recent_press = false;
void exti0_isr(void)
{
    cdcacm_write_now("Press!\n", 7);
    exti_reset_request(EXTI0);
    gpio_toggle(GPIOC, GPIO13);

    if (recent_press)
    {
        cdcacm_write_now("Entering Standby...\n", 20);
        setup_standby_mode();
    }
    else
    {
        cdcacm_write_now("First press\n", 12);
        recent_press = true;
        exti_disable_request(EXTI0);
        timer_enable_counter(TIM2);
    }
}

void tim2_isr(void)
{
    if (timer_get_flag(TIM2, TIM_SR_CC1IF)) // 50ms after button press
    {
        timer_clear_flag(TIM2, TIM_SR_CC1IF);
        exti_enable_request(EXTI0);
        cdcacm_write_now("50ms passed\n", 13);
    }
    else if (timer_get_flag(TIM2, TIM_SR_UIF)) // 250ms
    {
        cdcacm_write_now("250ms passed", 14);
        timer_clear_flag(TIM2, TIM_SR_UIF);
        recent_press = false;
        dimming_step = -dimming_step;
        while (gpio_get(GPIOA, GPIO0))
        {
            cdcacm_write_now(".", 1);
            dimming_value += dimming_step;
            timer_set_oc_value(TIM3, TIM_OC3, dimming_value);
            for (int i = 0; i < 100000; i++)
            {
                __asm__("nop");
            }

            if (dimming_value < 30 || dimming_value > 980)
            {
                break;
            }
        }
        cdcacm_write_now("\n", 1);
    }
}