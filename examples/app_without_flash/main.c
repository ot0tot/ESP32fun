/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
/* Load using the frontend's ram command and ROM download mode. */
int main(void)
{
    if (fun_pin_mode(4, FUN_OUTPUT)) return 1;
    fun_puts("RAM-loaded application; reset returns to the flash image");
    for (unsigned count = 0;; ++count) {
        fun_digital_write(4, 1); Delay_Ms(250);
        fun_digital_write(4, 0); Delay_Ms(250);
        if (!(count & 3u)) fun_printf("RAM application running, tick %u\n", count);
    }
}
