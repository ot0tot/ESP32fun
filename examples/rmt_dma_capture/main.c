/* SPDX-License-Identifier: MIT */
#include "fun_rmt_dma.h"
#include "fun_pwm.h"
static uint32_t symbols[128];
int main(void)
{
    const fun_pwm generator = {0, 0, 4, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&generator) ||
        fun_pwm_duty(&generator, 64, 0) || fun_rmt_dma_init(5)) return 1;
    fun_puts("Wire GPIO4 PWM to GPIO5 RMT input; pulse durations in us, finite DMA blocks");
    for (;;) {
        int count = fun_rmt_dma_read(symbols, 128, 1000000);
        if (count < 0) { fun_printf("Capture GDMA error %d\n", count); return 2; }
        fun_printf("Captured %d symbols via GDMA\n", count);
        for (int i = 1; i < count && i < 9; ++i) {
            unsigned a = symbols[i] & 32767u, b = (symbols[i] >> 16) & 32767u;
            if (!a || !b) break;
            fun_printf("%u:%u us, %u:%u us\n", (unsigned)((symbols[i] >> 15) & 1u), a,
                       (unsigned)(symbols[i] >> 31), b);
        }
        Delay_Ms(100);
    }
}
