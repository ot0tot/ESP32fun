/* SPDX-License-Identifier: MIT */
#include "fun_bus.h"
#ifndef SPI_CLOCK
#define SPI_CLOCK 4
#endif
#ifndef SPI_MOSI
#define SPI_MOSI 5
#endif
#ifndef SPI_MISO
#define SPI_MISO 6
#endif
#ifndef LATCH_PIN
#define LATCH_PIN SPI_SELECT
#endif
int main(void)
{
    const fun_spi bus = {SPI_CLOCK, SPI_MOSI, SPI_MISO, LATCH_PIN, 2, 0};
    if (fun_spi_init(&bus)) return 1;
    /* Connect /OE low, /SRCLR high, VCC to 3.3 V, and common ground. */
    for (unsigned position = 0;; position = (position + 1) & 7u) {
        fun_digital_write(LATCH_PIN, 0);
        fun_spi_transfer(&bus, (uint8_t)(1u << position));
        fun_digital_write(LATCH_PIN, 1);
        Delay_Ms(100);
    }
}
