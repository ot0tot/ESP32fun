/* SPDX-License-Identifier: MIT
 * Original SPI/GDMA WS2812 serializer; no CPU writes to GPIO during waveform.
 */
#include "fun_dma.h"
#include "fun_board.h"
#define LEDS 24u
#define RESET_BYTES 100u
static uint8_t waveform[2u * RESET_BYTES + LEDS * 9u] __attribute__((aligned(4)));
static fun_dma_desc tx;
/* GRB byte -> eight three-clock symbols: 100 for zero, 110 for one. */
static void encode(uint8_t value, uint8_t *out)
{
    uint32_t symbols = 0;
    for (unsigned i = 0; i < 8; ++i)
        symbols = (symbols << 3) | ((value & (0x80u >> i)) ? 6u : 4u);
    out[0] = (uint8_t)(symbols >> 16);
    out[1] = (uint8_t)(symbols >> 8);
    out[2] = (uint8_t)symbols;
}
int main(void)
{
    if (fun_spi_dma_init(SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 16)) return 1;
    /* XTAL40/16 = 2.5 MHz: T0H=.4 us, T1H=.8 us, bit=1.2 us.
     * 100 zero bytes at each end provide 320 us low reset/latch time.
     */
    for (unsigned frame = 0;; ++frame) {
        for (unsigned led = 0; led < LEDS; ++led) {
            uint8_t *p = waveform + RESET_BYTES + led * 9u;
            encode(led == frame % LEDS ? 16 : 0, p); /* green chase */
            encode(led == (frame + LEDS / 3) % LEDS ? 16 : 0, p + 3);
            encode(led == (frame + 2 * LEDS / 3) % LEDS ? 16 : 0, p + 6);
        }
        if (fun_dma_descriptor(&tx, waveform, sizeof(waveform), 0, 0)) return 2;
        int result = fun_spi_dma_transfer(&tx, 1, 0, 0, SPI_SELECT, 10000);
        if (result) { fun_printf("WS2812 GDMA error %d\n", result); return 3; }
        Delay_Ms(50);
    }
}
