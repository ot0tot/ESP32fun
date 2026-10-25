/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
#ifndef LED_PIN
#define LED_PIN 4
#endif
_Static_assert(LED_PIN < 32 && LED_PIN >= 0, "Bank-zero example");
static void branchless_write(unsigned high)
{
    /* Select the W1TC/W1TS address arithmetically; avoid an output RMW race. */
    FUN_REG(FUN_GPIO_BASE + 0x0cu - 4u * (high & 1u)) = FUN_BIT(LED_PIN);
}
int main(void)
{
    if (fun_pin_mode(LED_PIN, FUN_OUTPUT)) return 1;
    for (unsigned count = 0;; ++count) { branchless_write(count); Delay_Ms(500); }
}
