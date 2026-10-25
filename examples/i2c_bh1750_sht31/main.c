/* SPDX-License-Identifier: MIT */
#include "fun_bus.h"
#ifndef I2C_SDA
#define I2C_SDA 2
#endif
#ifndef I2C_SCL
#define I2C_SCL 3
#endif
static uint8_t sht_crc(const uint8_t *data)
{
    uint8_t crc = 0xff;
    for (unsigned i = 0; i < 2; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) crc = (uint8_t)((crc << 1) ^ ((crc & 0x80u) ? 0x31u : 0));
    }
    return crc;
}
int main(void)
{
    const fun_i2c bus = {I2C_SDA, I2C_SCL, 5, 1000};
    if (fun_i2c_init(&bus)) return 1;
    /* BH1750 at 0x23 and SHT31 at 0x44; both lines have external 4.7k pullups. */
    uint8_t power = 1, continuous = 0x10;
    int error = fun_i2c_write(&bus, 0x23, &power, 1);
    if (!error) error = fun_i2c_write(&bus, 0x23, &continuous, 1);
    if (error) fun_printf("BH1750 setup error %d\n", error);
    for (;;) {
        uint8_t command[] = {0x24, 0x00}, sht[6], light[2];
        error = fun_i2c_write(&bus, 0x44, command, sizeof(command));
        Delay_Ms(180);
        if (!error) error = fun_i2c_read(&bus, 0x44, sht, sizeof(sht));
        if (error) fun_printf("SHT31 bus error %d\n", error);
        else if (sht_crc(sht) != sht[2] || sht_crc(sht + 3) != sht[5]) fun_puts("SHT31 CRC error");
        else {
            uint32_t temp = ((uint32_t)sht[0] << 8) | sht[1];
            uint32_t humidity = ((uint32_t)sht[3] << 8) | sht[4];
            int centi_c = -4500 + (int)((17500u * temp) / 65535u);
            unsigned centi_percent = (unsigned)((10000u * humidity) / 65535u);
            fun_printf("SHT31: temperature=%d centi-C, humidity=%u centi-percent\n", centi_c, centi_percent);
        }
        error = fun_i2c_read(&bus, 0x23, light, sizeof(light));
        if (error) fun_printf("BH1750 bus error %d\n", error);
        else fun_printf("BH1750: %u lux\n", (((unsigned)light[0] << 8) | light[1]) * 5u / 6u);
        Delay_Ms(820);
    }
}
