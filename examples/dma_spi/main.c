/*
SPI DMA example with software NSS
Based on cnlohr's example for the CH32V203
Author: Fredrik Saevland (fsaev)
		https://github.com/fsaev

MIT License

Copyright (c) 2024 Fredrik Saevland

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* SPDX-License-Identifier: MIT
 * SPI DMA demonstration, adapted from Fredrik Saevland's ch32fun example.
 * Original notice is preserved above; ESP32 hardware implementation is original.
 */
#include "fun_dma.h"
#include "fun_board.h"
static uint8_t sendbuff[8] __attribute__((aligned(4))) = {0, 1, 2, 3, 4, 5, 6, 7};
static fun_dma_desc tx;
int main(void)
{
    if (fun_spi_dma_init(SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 16)) return 1;
    for (;;) {
        if (fun_dma_descriptor(&tx, sendbuff, sizeof(sendbuff), 0, 0)) return 2;
        int result = fun_spi_dma_transfer(&tx, 1, 0, 0, SPI_SELECT, 10000);
        fun_printf("SPI2/GDMA eight bytes: %s (%d)\n", result ? "ERROR" : "DONE", result);
        if (result) return 3;
        Delay_Ms(1000);
    }
}
