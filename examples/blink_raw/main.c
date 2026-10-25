/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
#ifndef LED_PIN
#define LED_PIN 4
#endif
_Static_assert(LED_PIN < 32 && LED_PIN >= 0, "Raw example uses GPIO bank zero");
int main(void)
{
    if (fun_pin_mode(LED_PIN, FUN_OUTPUT)) return 1;
    for (;;) {
        FUN_GPIO_OUT_SET = FUN_BIT(LED_PIN);
        Delay_Ms(500);
        FUN_GPIO_OUT_CLEAR = FUN_BIT(LED_PIN);
        Delay_Ms(500);
    }
}
