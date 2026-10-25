/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
int main(void)
{
    for (;;) __asm__ volatile ("nop");
}
