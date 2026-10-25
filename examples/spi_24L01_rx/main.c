/* SPDX-License-Identifier: MIT */
#include "../spi_24L01_tx/nrf24.h"
int main(void)
{
    int error = nrf_init(1);
    if (error) { fun_printf("nRF24 init error %d\n", error); return 1; }
    for (;;) {
        uint8_t fifo;
        nrf_command(0x17, 0, &fifo, 1);
        if (!(fifo & 1u)) {
            uint8_t data[4]; nrf_command(0x61, 0, data, sizeof(data)); nrf_write(7, 0x40);
            uint32_t count = data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
            fun_printf("received %u\n", (unsigned)count);
        }
        Delay_Ms(1);
    }
}
