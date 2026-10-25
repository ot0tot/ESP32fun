/* SPDX-License-Identifier: MIT */
#include "fun_lcd_dma.h"
#ifndef LCD_DATA_FIRST
#define LCD_DATA_FIRST 8
#endif
#ifndef LCD_CLOCK_PIN
#define LCD_CLOCK_PIN 16
#endif
#ifndef LCD_CLOCK_DIVIDER
#define LCD_CLOCK_DIVIDER 4
#endif
static uint8_t waveform[1024] __attribute__((aligned(4)));
static fun_dma_desc tx;
int main(void)
{
    /* Default: GPIO8..15 plus GPIO16 clock, 5 MSamples/s. */
    if (fun_lcd_dma_init(LCD_DATA_FIRST, LCD_CLOCK_PIN, LCD_CLOCK_DIVIDER)) return 1;
    for (unsigned frame = 0;; ++frame) {
        for (unsigned i = 0; i < sizeof(waveform); ++i)
            waveform[i] = (uint8_t)((i >> (frame & 3u)) ^ (frame & 255u));
        if (fun_dma_descriptor(&tx, waveform, sizeof(waveform), 0, 0)) return 2;
        int result = fun_lcd_dma_transfer(&tx, 1, 10000);
        if (result) { fun_printf("Parallel GDMA error %d\n", result); return 3; }
        Delay_Ms(10); /* finite bursts: update only after hardware releases buffer */
    }
}
