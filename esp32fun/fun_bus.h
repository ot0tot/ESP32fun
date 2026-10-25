/* SPDX-License-Identifier: MIT
 * Portable polling buses. These do not claim hardware SPI/I2C/DMA operation.
 */
#ifndef FUN_BUS_H
#define FUN_BUS_H
#include "esp32fun.h"
#include "fun_board.h"
#include "fun_i2c_errors.h"
/* Software I2C returns the shared INVALID/TIMEOUT/NACK/BUSY (-1..-4);
 * ARBITRATION/FIFO_ERROR (-5/-6) are hardware-only. Success is 0. */

typedef struct {
    unsigned clock, mosi, miso, select;
    uint32_t half_period_us;
    unsigned mode;
} fun_spi;

static inline int fun_spi_init(const fun_spi *bus)
{
    if (bus->mode > 3u || !bus->half_period_us || bus->clock == bus->mosi ||
        bus->clock == bus->miso || bus->clock == bus->select || bus->mosi == bus->miso ||
        bus->mosi == bus->select || bus->miso == bus->select) return -1;
    if (fun_pin_mode(bus->clock, FUN_OUTPUT) || fun_pin_mode(bus->mosi, FUN_OUTPUT) ||
        fun_pin_mode(bus->select, FUN_OUTPUT) || fun_pin_mode(bus->miso, FUN_INPUT)) return -1;
    fun_digital_write(bus->clock, (bus->mode >> 1) & 1u);
    fun_digital_write(bus->select, 1);
    return 0;
}

static inline uint8_t fun_spi_transfer(const fun_spi *bus, uint8_t out)
{
    uint8_t in = 0;
    unsigned idle = (bus->mode >> 1) & 1u;
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (!(bus->mode & 1u)) fun_digital_write(bus->mosi, out & 0x80u);
        Delay_Us(bus->half_period_us);
        fun_digital_write(bus->clock, !idle);
        if (bus->mode & 1u) fun_digital_write(bus->mosi, out & 0x80u);
        else in = (uint8_t)((in << 1) | fun_digital_read(bus->miso));
        Delay_Us(bus->half_period_us);
        fun_digital_write(bus->clock, idle);
        if (bus->mode & 1u) in = (uint8_t)((in << 1) | fun_digital_read(bus->miso));
        out <<= 1;
    }
    return in;
}

typedef struct { unsigned sda, scl; uint32_t half_period_us, stretch_timeout_us; } fun_i2c;

static inline int fun_i2c_clock_high(const fun_i2c *bus)
{
    fun_digital_write(bus->scl, 1);
    if (!fun_digital_read(bus->scl)) {
        unsigned mhz=fun_cpu_mhz();
        if (!mhz || bus->stretch_timeout_us>0x7fffffffu/mhz) return FUN_I2C_INVALID;
        uint32_t start=fun_cycle_count(),budget=bus->stretch_timeout_us*mhz;
        while (!fun_digital_read(bus->scl)) {
            if ((uint32_t)(fun_cycle_count()-start)>=budget) return FUN_I2C_TIMEOUT;
            Delay_Us(1);
        }
    }
    Delay_Us(bus->half_period_us);
    return 0;
}

static inline void fun_i2c_clock_low(const fun_i2c *bus)
{
    fun_digital_write(bus->scl, 0);
    Delay_Us(bus->half_period_us);
}

static inline int fun_i2c_init(const fun_i2c *bus)
{
    unsigned mhz=fun_cpu_mhz();
    /* Keep deadlines below half the wrapping cycle-counter range. */
    if (bus->sda == bus->scl || !bus->half_period_us || !bus->stretch_timeout_us ||
        !mhz || bus->stretch_timeout_us>0x7fffffffu/mhz) return FUN_I2C_INVALID;
    if (fun_pin_mode(bus->sda, FUN_OPEN_DRAIN) || fun_pin_mode(bus->scl, FUN_OPEN_DRAIN)) return FUN_I2C_INVALID;
    return fun_i2c_clock_high(bus);
}

static inline int fun_i2c_start(const fun_i2c *bus)
{
    fun_digital_write(bus->sda, 1);
    Delay_Us(bus->half_period_us);
    if (fun_i2c_clock_high(bus)) return FUN_I2C_TIMEOUT;
    if (!fun_digital_read(bus->sda)) return FUN_I2C_BUSY; /* Busy/stuck bus. */
    fun_digital_write(bus->sda, 0);
    Delay_Us(bus->half_period_us);
    fun_i2c_clock_low(bus);
    return 0;
}

static inline int fun_i2c_stop(const fun_i2c *bus)
{
    fun_digital_write(bus->sda, 0);
    Delay_Us(bus->half_period_us);
    int error=fun_i2c_clock_high(bus);
    fun_digital_write(bus->sda, 1);
    Delay_Us(bus->half_period_us);
    return error;
}

static inline int fun_i2c_write_byte(const fun_i2c *bus, uint8_t byte)
{
    for (unsigned bit = 0; bit < 8; ++bit) {
        fun_digital_write(bus->sda, byte & 0x80u);
        Delay_Us(bus->half_period_us);
        if (fun_i2c_clock_high(bus)) return FUN_I2C_TIMEOUT;
        fun_i2c_clock_low(bus);
        byte <<= 1;
    }
    fun_digital_write(bus->sda, 1);
    if (fun_i2c_clock_high(bus)) return FUN_I2C_TIMEOUT;
    int ack = !fun_digital_read(bus->sda);
    fun_i2c_clock_low(bus);
    return ack ? 0 : FUN_I2C_NACK;
}

static inline int fun_i2c_read_byte(const fun_i2c *bus, uint8_t *byte, int ack)
{
    uint8_t value = 0;
    fun_digital_write(bus->sda, 1);
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (fun_i2c_clock_high(bus)) return FUN_I2C_TIMEOUT;
        value = (uint8_t)((value << 1) | fun_digital_read(bus->sda));
        fun_i2c_clock_low(bus);
    }
    fun_digital_write(bus->sda, !ack);
    if (fun_i2c_clock_high(bus)) return FUN_I2C_TIMEOUT;
    fun_i2c_clock_low(bus);
    fun_digital_write(bus->sda, 1);
    *byte = value;
    return 0;
}

static inline int fun_i2c_write(const fun_i2c *bus, uint8_t address, const uint8_t *data, size_t length)
{
    if (address > 0x7fu) return FUN_I2C_INVALID;
    int error = fun_i2c_start(bus);
    if (!error) error = fun_i2c_write_byte(bus, (uint8_t)(address << 1));
    while (!error && length--) error = fun_i2c_write_byte(bus, *data++);
    int stopped = fun_i2c_stop(bus);
    return error ? error : stopped;
}

static inline int fun_i2c_read(const fun_i2c *bus, uint8_t address, uint8_t *data, size_t length)
{
    if (address > 0x7fu) return FUN_I2C_INVALID;
    int error = fun_i2c_start(bus);
    if (!error) error = fun_i2c_write_byte(bus, (uint8_t)((address << 1) | 1u));
    for (size_t i = 0; !error && i < length; ++i) error = fun_i2c_read_byte(bus, data + i, i + 1 < length);
    int stopped = fun_i2c_stop(bus);
    return error ? error : stopped;
}

static inline uint8_t fun_onewire_crc(const uint8_t *data, size_t length)
{
    uint8_t crc = 0;
    while (length--) {
        uint8_t value = *data++;
        for (unsigned bit = 0; bit < 8; ++bit) {
            uint8_t mix = (crc ^ value) & 1u;
            crc >>= 1;
            if (mix) crc ^= 0x8cu;
            value >>= 1;
        }
    }
    return crc;
}
#endif
