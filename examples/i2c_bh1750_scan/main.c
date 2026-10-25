/* SPDX-License-Identifier: MIT */
#include "fun_bus.h"
int main(void)
{
    const fun_i2c bus = {I2C_SDA, I2C_SCL, 5, 1000};
    if (fun_i2c_init(&bus)) return 1;
    fun_puts("Scanning 7-bit I2C addresses (external pullups required)");
    for (unsigned address = 8; address < 0x78; ++address)
        if (!fun_i2c_write(&bus, (uint8_t)address, 0, 0)) fun_printf("Found 0x%02x\n", address);
    uint8_t power = 1, high_resolution = 0x10;
    int error = fun_i2c_write(&bus, 0x23, &power, 1);
    if (!error) error = fun_i2c_write(&bus, 0x23, &high_resolution, 1);
    if (error) { fun_printf("BH1750 setup error %d\n", error); return 1; }
    for (;;) {
        Delay_Ms(200);
        uint8_t raw[2];
        error = fun_i2c_read(&bus, 0x23, raw, 2);
        if (error) fun_printf("BH1750 read error %d\n", error);
        else fun_printf("BH1750 %u lux\n", (((unsigned)raw[0] << 8) | raw[1]) * 5u / 6u);
    }
}
