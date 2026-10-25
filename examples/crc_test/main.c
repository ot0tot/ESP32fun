/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
/* Portable software implementation of reflected CRC32. */
int main(void)
{
    const char vector[] = "123456789";
    uint32_t crc = fun_crc32(vector, 9);
    fun_printf("CRC32=%08x %s\n", (unsigned)crc, crc == 0xcbf43926u ? "PASS" : "FAIL");
    for (;;) __asm__ volatile ("nop");
}
