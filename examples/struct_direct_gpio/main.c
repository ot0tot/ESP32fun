/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
typedef union {
    uint32_t value;
    struct { uint32_t unused:4, pin4:1, pin5:1, pin6:1, unused2:11, pin18:1, remaining:13; } bits;
} bank_bits;
/* Structure construction produces a single W1TS/W1TC write. No bitfield
 * read-modify-write against write-only registers.
 */
int main(void)
{
#ifdef ESP32FUN_ESP32
    if (fun_pin_mode(4, FUN_OUTPUT) || fun_pin_mode(5, FUN_OUTPUT) || fun_pin_mode(18, FUN_OUTPUT)) return 1;
    const bank_bits mask = {.bits = {.pin4 = 1, .pin5 = 1, .pin18 = 1}};
#else
    if (fun_pin_mode(4, FUN_OUTPUT) || fun_pin_mode(5, FUN_OUTPUT) || fun_pin_mode(6, FUN_OUTPUT)) return 1;
    const bank_bits mask = {.bits = {.pin4 = 1, .pin5 = 1, .pin6 = 1}};
#endif
    for (;;) {
        FUN_GPIO_OUT_SET = mask.value;
        Delay_Ms(950);
        FUN_GPIO_OUT_CLEAR = mask.value;
        Delay_Ms(50);
    }
}
