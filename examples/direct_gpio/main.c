/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
int main(void)
{
#ifdef ESP32FUN_ESP32
    const unsigned third = 18;
#else
    const unsigned third = 6;
#endif
    if (fun_pin_mode(4, FUN_OUTPUT) || fun_pin_mode(5, FUN_OUTPUT) || fun_pin_mode(third, FUN_OUTPUT)) return 1;
    for (;;) {
        FUN_GPIO_OUT_SET = FUN_BIT(4);
        FUN_GPIO_OUT_SET = FUN_BIT(5);
        FUN_GPIO_OUT |= FUN_BIT(third);
        Delay_Ms(950);
        FUN_GPIO_OUT_CLEAR = FUN_BIT(4);
        FUN_GPIO_OUT_CLEAR = FUN_BIT(5);
        FUN_GPIO_OUT &= ~FUN_BIT(third);
        Delay_Ms(50);
    }
}
