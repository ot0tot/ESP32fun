/* SPDX-License-Identifier: MIT */
#include "fun_adc_dma.h"
static uint32_t samples[100];
int main(void)
{
    if (fun_adc_dma_init(0, 20000)) return 1;
    fun_puts("ADC1 GPIO0, 0 dB, 100-sample timer-paced DMA blocks; raw, uncalibrated");
    for (;;) {
        int result = fun_adc_dma_read(samples, 100, 100000);
        if (result) { fun_printf("ADC GDMA error %d\n", result); return 2; }
        unsigned sum = 0, minimum = 4095, maximum = 0, invalid = 0;
        for (unsigned i = 0; i < 100; ++i) {
            unsigned value = samples[i] & 4095u;
            invalid += (samples[i] & (15u << 13)) != 0; /* channel0, ADC1 */
            sum += value;
            if (value < minimum) minimum = value;
            if (value > maximum) maximum = value;
        }
        fun_printf("100 DMA samples: min=%u max=%u mean=%u invalid=%u\n",
                   minimum, maximum, sum / 100u, invalid);
        if (invalid) return 3;
        Delay_Ms(100);
    }
}
