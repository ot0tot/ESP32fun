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
#ifndef SPI_SELECT
#define SPI_SELECT 7
#endif
#ifndef OLED_DC
#define OLED_DC 10
#endif
#ifndef OLED_RESET
#define OLED_RESET 3
#endif
static uint8_t frame[1024], next_frame[1024];
static unsigned random_state = 0x12345678;
static unsigned random_bit(void)
{
    random_state ^= random_state << 13; random_state ^= random_state >> 17; random_state ^= random_state << 5;
    return random_state & 1u;
}
static int pixel(unsigned x, unsigned y)
{
    return (frame[(y >> 3) * 128 + x] >> (y & 7u)) & 1u;
}
static void transfer(const fun_spi *bus, int data, const uint8_t *bytes, unsigned length)
{
    fun_digital_write(OLED_DC, data); fun_digital_write(bus->select, 0);
    while (length--) fun_spi_transfer(bus, *bytes++);
    fun_digital_write(bus->select, 1);
}
int main(void)
{
    const fun_spi bus = {SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 1, 0};
    if (fun_spi_init(&bus) || fun_pin_mode(OLED_DC, FUN_OUTPUT) || fun_pin_mode(OLED_RESET, FUN_OUTPUT)) return 1;
    Delay_Ms(10); fun_digital_write(OLED_RESET, 1); Delay_Ms(10);
    const uint8_t init[] = {0xae, 0xd5, 0x80, 0xa8, 63, 0xd3, 0, 0x40, 0x8d, 0x14,
                           0x20, 0, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0x7f, 0xd9, 0xf1,
                           0xdb, 0x40, 0xa4, 0xa6, 0xaf};
    transfer(&bus, 0, init, sizeof(init));
    for (unsigned i = 0; i < sizeof(frame); ++i)
        for (unsigned bit = 0; bit < 8; ++bit) frame[i] |= (uint8_t)(random_bit() << bit);
    for (;;) {
        const uint8_t position[] = {0x21, 0, 127, 0x22, 0, 7};
        transfer(&bus, 0, position, sizeof(position)); transfer(&bus, 1, frame, sizeof(frame));
        for (unsigned y = 0; y < 64; ++y) for (unsigned x = 0; x < 128; ++x) {
            unsigned neighbours = 0;
            for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy) neighbours += pixel((x + dx) & 127u, (y + dy) & 63u);
            unsigned alive = neighbours == 3 || (neighbours == 2 && pixel(x, y));
            unsigned index = (y >> 3) * 128 + x, mask = 1u << (y & 7u);
            next_frame[index] = (uint8_t)((next_frame[index] & ~mask) | (alive ? mask : 0));
        }
        for (unsigned i = 0; i < sizeof(frame); ++i) frame[i] = next_frame[i];
        Delay_Ms(50);
    }
}
