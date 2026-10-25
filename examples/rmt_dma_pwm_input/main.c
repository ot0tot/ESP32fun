/* SPDX-License-Identifier: MIT */
#include "fun_rmt_dma.h"
#include "fun_pwm.h"
static uint32_t symbols[128];
int main(void)
{
    const fun_pwm generator = {0, 0, 4, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&generator) ||
        fun_pwm_duty(&generator, 64, 0) || fun_rmt_dma_init(5)) return 1;
    fun_puts("Wire GPIO4 to GPIO5: expected ~1000 Hz, 25% duty; RMT/GDMA 1 us ticks");
    for (;;) {
        int count = fun_rmt_dma_read(symbols, 128, 1000000);
        if (count < 0) { fun_printf("PWM capture GDMA error %d\n", count); return 2; }
        unsigned high = 0, total = 0, periods = 0;
        for (int i = 1; i < count; ++i) { /* initial symbol may be a partial cycle */
            unsigned a = symbols[i] & 32767u, b = (symbols[i] >> 16) & 32767u;
            if (!a || !b) break;
            if (((symbols[i] >> 15) & 1u) == (symbols[i] >> 31)) continue;
            high += (symbols[i] & FUN_BIT(15)) ? a : b;
            total += a + b;
            ++periods;
        }
        if (!periods || !total) { fun_puts("No complete PWM cycles captured"); continue; }
        fun_printf("%u cycles: frequency=%u Hz duty=%u.%u%%\n", periods,
                   1000000u * periods / total, (high * 1000u / total) / 10u,
                   (high * 1000u / total) % 10u);
        Delay_Ms(100);
    }
}
