/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
typedef struct {
    volatile uint32_t reserved;
    volatile uint32_t output;
    volatile uint32_t set;
    volatile uint32_t clear;
} gpio_bank_zero;
int main(void)
{
    gpio_bank_zero *gpio = (gpio_bank_zero *)(uintptr_t)FUN_GPIO_BASE;
    if (fun_pin_mode(4, FUN_OUTPUT)) return 1;
    for (;;) { gpio->set = FUN_BIT(4); Delay_Ms(500); gpio->clear = FUN_BIT(4); Delay_Ms(500); }
}
