/* SPDX-License-Identifier: MIT */
#ifndef NRF24_PORT_H
#define NRF24_PORT_H
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
#ifndef SPI_SELECT
#define SPI_SELECT 7
#endif
#ifndef NRF_CE
#define NRF_CE 10
#endif
static const fun_spi radio = {SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 1, 0};
static uint8_t nrf_command(uint8_t command, const uint8_t *out, uint8_t *in, unsigned length)
{
    fun_digital_write(radio.select, 0);
    uint8_t status = fun_spi_transfer(&radio, command);
    for (unsigned i = 0; i < length; ++i) {
        uint8_t byte = fun_spi_transfer(&radio, out ? out[i] : 0xff);
        if (in) in[i] = byte;
    }
    fun_digital_write(radio.select, 1);
    return status;
}
static void nrf_write(uint8_t reg, uint8_t value) { nrf_command(0x20u | reg, &value, 0, 1); }
static int nrf_init(int receive)
{
    if (fun_spi_init(&radio) || fun_pin_mode(NRF_CE, FUN_OUTPUT)) return -1;
    Delay_Ms(100);
    const uint8_t address[5] = {'f','u','n','3','2'};
    nrf_write(0, 0x0c); nrf_write(1, 1); nrf_write(2, 1); nrf_write(3, 3);
    nrf_write(4, 0x2f); nrf_write(5, 76); nrf_write(6, 0x06); nrf_write(0x11, 4);
    nrf_command(0x2a, address, 0, 5); nrf_command(0x30, address, 0, 5);
    nrf_write(7, 0x70); nrf_command(0xe1, 0, 0, 0); nrf_command(0xe2, 0, 0, 0);
    uint8_t config;
    nrf_command(0x06, 0, &config, 1);
    if (config != 0x06) return -2;
    nrf_write(0, receive ? 0x0f : 0x0e); Delay_Ms(5);
    fun_digital_write(NRF_CE, receive);
    return 0;
}
#endif
