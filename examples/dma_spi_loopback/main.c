/* SPDX-License-Identifier: MIT */
#include "fun_dma.h"
#include "fun_board.h"
static uint8_t source[512] __attribute__((aligned(4)));
static uint8_t received[512] __attribute__((aligned(4)));
static fun_dma_desc tx[2], rx[2];
int main(void)
{
    /* Connect GPIO5/MOSI to GPIO6/MISO. GPIO4=SCK, GPIO7=CS. */
    if (fun_spi_dma_init(SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 16)) return 1;
    for (unsigned frame = 0;; ++frame) {
        for (unsigned i = 0; i < sizeof(source); ++i) {
            source[i] = (uint8_t)(i ^ frame ^ (i >> 8));
            received[i] = (uint8_t)~source[i];
        }
        for (unsigned i = 0; i < 2; ++i) {
            if (fun_dma_descriptor(&tx[i], source + i * 256, 256, i ? 0 : &tx[1], 0) ||
                fun_dma_descriptor(&rx[i], received + i * 256, 256, i ? 0 : &rx[1], 1)) return 2;
        }
        int result = fun_spi_dma_transfer(tx, 2, rx, 2, SPI_SELECT, 10000);
        if (result) { fun_printf("DMA error %d\n", result); return 3; }
        unsigned errors = 0;
        for (unsigned i = 0; i < sizeof(source); ++i) errors += source[i] != received[i];
        fun_printf("512-byte chained SPI loopback frame %u: %s, mismatches=%u\n",
                   frame, errors ? "FAIL" : "PASS", errors);
        if (errors) return 4;
        Delay_Ms(1000);
    }
}
