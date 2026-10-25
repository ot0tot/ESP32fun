/* SPDX-License-Identifier: MIT */
#ifndef FUN_SSD1306_I2C_H
#define FUN_SSD1306_I2C_H
#include "fun_bus.h"
#ifndef OLED_ADDRESS
#define OLED_ADDRESS 0x3c
#endif
static const fun_i2c oled_bus = {I2C_SDA, I2C_SCL, 5, 1000};
static inline int ssd1306_i2c_init(void) { return fun_i2c_init(&oled_bus); }
/* Four-pin I2C modules use their power-on reset. Allow it to settle. */
static inline void ssd1306_rst(void) { Delay_Ms(100); }
static inline uint8_t ssd1306_pkt_send(const uint8_t *data, int length, int command)
{
    if (length < 0 || length > 32) return 1;
    uint8_t packet[33];
    packet[0] = command ? 0 : 0x40;
    for (int i = 0; i < length; ++i) packet[i + 1] = data[i];
    return fun_i2c_write(&oled_bus, OLED_ADDRESS, packet, (size_t)length + 1u) != 0;
}
#endif
