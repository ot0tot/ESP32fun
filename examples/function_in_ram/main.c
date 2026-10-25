/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
/* In this environment all .text is ROM-loaded into SRAM, including this fn. */
__attribute__((noinline)) unsigned ram_function(unsigned input) { return input * 3u + 1u; }
int main(void)
{
    fun_printf("Function at %08x returns %u\n", (unsigned)(uintptr_t)ram_function, ram_function(7));
    return ram_function(7) != 22;
}
