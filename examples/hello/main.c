/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"

/* Also provides a board-level check of ROM .data loading and .bss clearing. */
static volatile uint32_t initialized = 0x12345678;
static volatile uint32_t zeroed[16];

int main(void)
{
    unsigned ok = initialized == 0x12345678;
    for (unsigned i = 0; i < 16; ++i) ok &= zeroed[i] == 0;
    fun_printf("ESP32fun %s: startup %s\n", FUN_TARGET_NAME, ok ? "PASS" : "FAIL");
    for (uint32_t count = 0;; ++count) {
        fun_printf("hello %u, cycles=%08x\n", (unsigned)count,
                   (unsigned)fun_cycle_count());
        Delay_Ms(1000);
    }
}
