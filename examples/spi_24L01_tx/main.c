/* SPDX-License-Identifier: MIT */
#include "nrf24.h"
int main(void)
{
    int error = nrf_init(0);
    if (error) { fun_printf("nRF24 init error %d\n", error); return 1; }
    for (uint32_t count = 0;; ++count) {
        uint8_t data[4] = {(uint8_t)count, (uint8_t)(count >> 8), (uint8_t)(count >> 16), (uint8_t)(count >> 24)};
        nrf_write(7, 0x70); nrf_command(0xa0, data, 0, 4);
        fun_digital_write(NRF_CE, 1); Delay_Us(20); fun_digital_write(NRF_CE, 0);
        unsigned waited; uint8_t status = 0;
        for (waited = 0; waited < 100; ++waited) {
            status = nrf_command(0xff, 0, 0, 0);
            if (status & 0x30u) break;
            Delay_Ms(1);
        }
        fun_printf("packet %u: %s\n", (unsigned)count, status & 0x20u ? "ACK" : "failed");
        if (!(status & 0x20u)) nrf_command(0xe1, 0, 0, 0);
        Delay_Ms(500);
    }
}
