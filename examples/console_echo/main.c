/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
/* Polling UART0 console transmit and receive without libc sprintf. */
int main(void)
{
    uint32_t count = 0, previous = fun_cycle_count();
    uint8_t byte;
    for (;;) {
        uint32_t now = fun_cycle_count();
        if ((uint32_t)(now - previous) >= fun_cpu_mhz() * 1000000u) {
            previous = now;
            fun_printf("hello bee %u\r\n", (unsigned)count++);
        }
        if (!fun_getchar(&byte)) fun_putchar(byte);
    }
}
