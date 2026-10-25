/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
/* Bidirectional console over the selected ESP32 ROM transport. */
int main(void)
{
    if (fun_pin_mode(4, FUN_OUTPUT) || fun_pin_mode(5, FUN_OUTPUT)) return 1;
    uint32_t count = 0;
    uint8_t last = ' ', incoming;
    for (;;) {
        fun_digital_write(4, 1); fun_digital_write(5, 1);
        fun_printf("+%u\n", (unsigned)count++);
        for (unsigned i = 0; i < 100; ++i) {
            while (!fun_getchar(&incoming)) { last = incoming; ++count; }
            Delay_Ms(1);
        }
        fun_digital_write(4, 0); fun_digital_write(5, 0);
        fun_printf("-%u[%c]\n", (unsigned)count++, last);
        Delay_Ms(100);
    }
}
