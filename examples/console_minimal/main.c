/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
int main(void)
{
    fun_puts("Hello");
    for (;;) __asm__ volatile ("nop");
}
