/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
int main(void)
{
    if (fun_pin_mode(4, FUN_OUTPUT)) return 1;
    uint8_t input;
    for (uint32_t count = 0;; ++count) {
        fun_digital_write(4, 1); Delay_Ms(50);
        fun_digital_write(4, 0); Delay_Ms(50);
        fun_printf("Count: %u\n", (unsigned)count);
        while (!fun_getchar(&input)) fun_putchar(input);
    }
}
