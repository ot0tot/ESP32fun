/* SPDX-License-Identifier: MIT */
#include "fun_bus.h"
#ifndef ONEWIRE_PIN
#define ONEWIRE_PIN 4
#endif
static int reset(void)
{
    fun_digital_write(ONEWIRE_PIN, 0); Delay_Us(480);
    fun_digital_write(ONEWIRE_PIN, 1); Delay_Us(70);
    int present = !fun_digital_read(ONEWIRE_PIN); Delay_Us(410);
    return present;
}
static void write_byte(uint8_t byte)
{
    for (unsigned bit = 0; bit < 8; ++bit) {
        unsigned value = byte & 1u;
        fun_digital_write(ONEWIRE_PIN, 0); Delay_Us(value ? 6 : 60);
        fun_digital_write(ONEWIRE_PIN, 1); Delay_Us(value ? 64 : 10);
        byte >>= 1;
    }
}
static uint8_t read_byte(void)
{
    uint8_t value = 0;
    for (unsigned bit = 0; bit < 8; ++bit) {
        fun_digital_write(ONEWIRE_PIN, 0); Delay_Us(6);
        fun_digital_write(ONEWIRE_PIN, 1); Delay_Us(9);
        value |= (uint8_t)(fun_digital_read(ONEWIRE_PIN) << bit); Delay_Us(55);
    }
    return value;
}
int main(void)
{
    if (fun_pin_mode(ONEWIRE_PIN, FUN_OPEN_DRAIN)) return 1;
    /* One externally powered DS18B20; 4.7k pull-up to 3.3 V. No parasite power. */
    for (;;) {
        if (!reset()) { fun_puts("No 1-Wire presence pulse"); Delay_Ms(1000); continue; }
        write_byte(0xcc); write_byte(0x44); Delay_Ms(750);
        if (!reset()) continue;
        write_byte(0xcc); write_byte(0xbe);
        uint8_t data[9]; for (unsigned i = 0; i < 9; ++i) data[i] = read_byte();
        if (fun_onewire_crc(data, 8) != data[8]) { fun_puts("1-Wire CRC error"); continue; }
        int16_t raw = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
        fun_printf("DS18B20: %d / 16 degrees C\n", (int)raw);
        Delay_Ms(250);
    }
}
