#include "power.h"

void setup_standby_mode(void)
{
    SCB_SCR |= SCB_SCR_SLEEPDEEP; // now wait for wfi
    PWR_CR |= PWR_CR_PDDS;        // power down deep sleep
    PWR_CSR &= ~PWR_CSR_WUF;      // clear wakeup flag
    PWR_CSR |= PWR_CSR_EWUP;      // enable wakeup pin
}