/* SPDX-License-Identifier: MIT
 * SPI/GDMA generates a hardware-paced waveform on one output.
 * This driver does not schedule arbitrary register reads or writes.
 */
#include "fun_dma.h"
#include "fun_board.h"
static uint8_t waveform[256] __attribute__((aligned(4)));
static fun_dma_desc tx;
int main(void)
{
    /* XTAL40/40 = 1 MHz. Each serialized bit is one microsecond. */
    if (fun_spi_dma_init(SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 40)) return 1;
    /* 8 us high, 8 us low, 24 us high, 24 us low, repeated in one transaction. */
    for (unsigned i = 0; i < sizeof(waveform); i += 8) {
        waveform[i] = 0xff; waveform[i+1] = 0;
        waveform[i+2] = waveform[i+3] = waveform[i+4] = 0xff;
        waveform[i+5] = waveform[i+6] = waveform[i+7] = 0;
    }
    for (;;) {
        if (fun_dma_descriptor(&tx, waveform, sizeof(waveform), 0, 0)) return 2;
        int result = fun_spi_dma_transfer(&tx, 1, 0, 0, SPI_SELECT, 10000);
        if (result) { fun_printf("Scheduled waveform GDMA error %d\n", result); return 3; }
        Delay_Ms(10); /* gap between finite hardware-paced bursts */
    }
}
