/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 *
 * Timing calculations adapted from ESP-IDF v5.3.1 hal/{esp32,esp32c3}/i2c_ll.h.
 * Original SDK-independent command/FIFO implementation by ESP32fun contributors.
 * Hardware I2C0, polling service, 7-bit single-master operation; no DMA claim.
 */
#ifndef FUN_I2C_HW_H
#define FUN_I2C_HW_H
#include "esp32fun.h"
#if defined(ESP32FUN_ESP32)
#include "fun_uart_udma_esp32.h"
#define FUN_I2C_REG(o) FUN_REG(0x3ff53000u+(o))
#define FUN_I2C_SDA_SIGNAL 30u
#define FUN_I2C_SCL_SIGNAL 29u
#define FUN_I2C_RESTART 0u
#define FUN_I2C_READ 2u
#define FUN_I2C_STOP 3u
#elif defined(ESP32FUN_ESP32C3)
#define FUN_I2C_REG(o) FUN_REG(0x60013000u+(o))
#define FUN_I2C_SDA_SIGNAL 54u
#define FUN_I2C_SCL_SIGNAL 53u
#define FUN_I2C_RESTART 6u
#define FUN_I2C_READ 3u
#define FUN_I2C_STOP 2u
#else
#error "Hardware I2C currently implements classic ESP32 and ESP32-C3 only"
#endif
/* Classic ESP32 must access the FIFO through its AHB alias. */
#define FUN_I2C_FIFO FUN_REG(0x6001301cu)
#include "fun_i2c_errors.h"
/* Both I2C stacks share INVALID/TIMEOUT/NACK/BUSY (-1..-4). Hardware
 * additionally reports ARBITRATION (-5) and FIFO_ERROR (-6). Success is 0. */
typedef struct { unsigned sda,scl,hz; uint32_t timeout_us; } fun_i2c_hw;

static inline unsigned fun_i2c_hw_rx_count(void) { return (FUN_I2C_REG(8)>>8)&63u; }
static inline unsigned fun_i2c_hw_tx_count(void) { return (FUN_I2C_REG(8)>>18)&63u; }
static inline void fun_i2c_hw_fifo_reset(unsigned mask) {
    unsigned conf=FUN_I2C_REG(0x18); FUN_I2C_REG(0x18)=conf|mask; FUN_I2C_REG(0x18)=conf;
}
static inline void fun_i2c_hw_update(void) {
#ifdef ESP32FUN_ESP32C3
    FUN_I2C_REG(4)|=FUN_BIT(11);
#endif
}
static inline int fun_i2c_hw_pins(const fun_i2c_hw *bus) {
#ifdef ESP32FUN_ESP32
    if(fun_uart_udma_clock_sync()) return FUN_I2C_INVALID;
#endif
    if(!bus || bus->sda==bus->scl || !bus->timeout_us || !fun_cpu_mhz() ||
       bus->timeout_us>0x7fffffffu/fun_cpu_mhz()) return FUN_I2C_INVALID;
    if(fun_pin_mode(bus->sda,FUN_OPEN_DRAIN) || fun_pin_mode(bus->scl,FUN_OPEN_DRAIN)) return FUN_I2C_INVALID;
    FUN_IO_MUX(bus->sda)|=FUN_IO_PULLUP; FUN_IO_MUX(bus->scl)|=FUN_IO_PULLUP;
    FUN_GPIO_OUTPUT_SELECT(bus->sda)=FUN_I2C_SDA_SIGNAL|FUN_GPIO_ENABLE_FROM_REG;
    FUN_GPIO_OUTPUT_SELECT(bus->scl)=FUN_I2C_SCL_SIGNAL|FUN_GPIO_ENABLE_FROM_REG;
#ifdef ESP32FUN_ESP32
    FUN_REG(FUN_GPIO_BASE+0x130u+4u*FUN_I2C_SDA_SIGNAL)=bus->sda|FUN_BIT(7);
    FUN_REG(FUN_GPIO_BASE+0x130u+4u*FUN_I2C_SCL_SIGNAL)=bus->scl|FUN_BIT(7);
    FUN_REG(0x3ff000c0u)|=FUN_BIT(7);
    FUN_REG(0x3ff000c4u)|=FUN_BIT(7); FUN_REG(0x3ff000c4u)&=~FUN_BIT(7);
    FUN_I2C_REG(0x50)=15; FUN_I2C_REG(0x54)=15;
#else
    FUN_REG(FUN_GPIO_BASE+0x154u+4u*FUN_I2C_SDA_SIGNAL)=bus->sda|FUN_BIT(6);
    FUN_REG(FUN_GPIO_BASE+0x154u+4u*FUN_I2C_SCL_SIGNAL)=bus->scl|FUN_BIT(6);
    FUN_REG(0x600c0010u)|=FUN_BIT(7);
    FUN_REG(0x600c0018u)|=FUN_BIT(7); FUN_REG(0x600c0018u)&=~FUN_BIT(7);
    FUN_I2C_REG(0x54)=FUN_BIT(21); /* XTAL40, active, divide by 1 */
    FUN_I2C_REG(0x50)=FUN_BIT(9)|FUN_BIT(8)|(7u<<4)|7u;
#endif
    FUN_I2C_REG(0x28)=0; FUN_I2C_REG(0x24)=0x3ffff;
    FUN_I2C_REG(0x18)=4u|(4u<<5);
#ifdef ESP32FUN_ESP32C3
    FUN_I2C_REG(0x18)|=FUN_BIT(14); /* FIFO protect */
#endif
    fun_i2c_hw_fifo_reset(FUN_BIT(12)|FUN_BIT(13));
    return 0;
}
static inline int fun_i2c_hw_master_init(const fun_i2c_hw *bus) {
    if(!bus || (bus->hz!=100000u && bus->hz!=400000u)) return FUN_I2C_INVALID;
    int error=fun_i2c_hw_pins(bus); if(error) return error;
    FUN_I2C_REG(4)=3u|FUN_BIT(4)|FUN_BIT(8);
#ifdef ESP32FUN_ESP32
    unsigned source=fun_cpu_mhz()*1000000u; if(source>80000000u) source=80000000u;
    unsigned half=source/bus->hz/2u;
    FUN_I2C_REG(0)=half-1u; FUN_I2C_REG(0x38)=half-13u;
    FUN_I2C_REG(0x30)=half/2u; FUN_I2C_REG(0x34)=half/2u;
    FUN_I2C_REG(0x40)=half; FUN_I2C_REG(0x44)=half;
    FUN_I2C_REG(0x48)=half; FUN_I2C_REG(0x4c)=half;
    FUN_I2C_REG(0x0c)=source/1000000u*5000u;
#else
    unsigned half=40000000u/bus->hz/2u,wait_high=half/2u-2u;
    FUN_I2C_REG(0)=half-1u; FUN_I2C_REG(0x38)=(half-wait_high)|(wait_high<<9);
    FUN_I2C_REG(0x30)=half/4u-1u; FUN_I2C_REG(0x34)=half/2u-1u;
    FUN_I2C_REG(0x40)=half-1u; FUN_I2C_REG(0x44)=half-1u;
    FUN_I2C_REG(0x48)=half-1u; FUN_I2C_REG(0x4c)=half-1u;
    FUN_I2C_REG(0x0c)=18u|FUN_BIT(5); /* Stall threshold; measured ~13 ms on this C3. */
#endif
    fun_i2c_hw_update(); return 0;
}
static inline uint32_t fun_i2c_hw_cmd(unsigned opcode,unsigned n,unsigned ack_check,unsigned nack) {
    return (opcode<<11)|n|(ack_check<<8)|(nack<<10);
}
/* Write, read, or write/repeated-START/read. Zero/zero probes the address.
 * Single command list; FIFO streamed by the CPU while hardware clocks the bus.
 * Whole-transaction deadline is bus->timeout_us, not a per-byte timeout.
 */
static inline int fun_i2c_hw_transfer(const fun_i2c_hw *bus,uint8_t address,
                                     const uint8_t *tx,size_t tx_length,uint8_t *rx,size_t rx_length) {
    if(!bus || address>127 || tx_length>254 || rx_length>255 ||
       (tx_length && !tx) || (rx_length && !rx) || !bus->timeout_us ||
       !fun_cpu_mhz() ||
       bus->timeout_us>0x7fffffffu/fun_cpu_mhz()) return FUN_I2C_INVALID;
    if(!fun_digital_read(bus->scl) || !fun_digital_read(bus->sda)) return FUN_I2C_BUSY;
    /* External stuck clocks can disturb the classic controller's idle FSM.
     * Reinitialize only after both lines are free, before issuing START. */
    int initialized=fun_i2c_hw_master_init(bus);if(initialized) return initialized;
    uint8_t outgoing[256]; unsigned total=0,command=0,written=0,received=0;
    fun_i2c_hw_fifo_reset(FUN_BIT(12)|FUN_BIT(13));
    FUN_I2C_REG(0x24)=0x3ffff;
    for(unsigned i=0;i<8;++i) FUN_I2C_REG(0x58+4u*i)=fun_i2c_hw_cmd(4,0,0,0);
    FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(FUN_I2C_RESTART,0,0,0);
    if(tx_length || !rx_length) {
        outgoing[total++]=(uint8_t)(address<<1);
        for(unsigned i=0;i<tx_length;++i) outgoing[total++]=tx[i];
        FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(1,total,1,0);
    }
    if(rx_length) {
        if(tx_length) FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(FUN_I2C_RESTART,0,0,0);
        outgoing[total++]=(uint8_t)((address<<1)|1u);
        FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(1,1,1,0);
        if(rx_length>1) FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(FUN_I2C_READ,rx_length-1u,0,0);
        FUN_I2C_REG(0x58+4u*command++)=fun_i2c_hw_cmd(FUN_I2C_READ,1,0,1);
    }
    unsigned last=command;
    FUN_I2C_REG(0x58+4u*command)=fun_i2c_hw_cmd(FUN_I2C_STOP,0,0,0);
    while(written<total && fun_i2c_hw_tx_count()<32u) FUN_I2C_FIFO=outgoing[written++];
    fun_i2c_hw_update();
    uint32_t started=fun_cycle_count(),budget=bus->timeout_us*fun_cpu_mhz();
    FUN_I2C_REG(4)|=FUN_BIT(5);
    int error=0;
    for(;;) {
        while(fun_i2c_hw_rx_count()) {
            uint8_t value=(uint8_t)FUN_I2C_FIFO;
            if(received>=rx_length) { error=FUN_I2C_FIFO_ERROR; break; }
            rx[received++]=value;
        }
        while(written<total && fun_i2c_hw_tx_count()<32u) FUN_I2C_FIFO=outgoing[written++];
        unsigned raw=FUN_I2C_REG(0x20);
        if(raw & FUN_BIT(10)) error=FUN_I2C_NACK;
        else if(raw & FUN_BIT(5)) error=FUN_I2C_ARBITRATION;
        else if(raw & FUN_BIT(8)) error=FUN_I2C_TIMEOUT;
        if(error) break;
        if(FUN_I2C_REG(0x58+4u*last)&FUN_BIT(31)) {
            if(received==rx_length && written==total) return 0;
            /* Drain any last byte on the next iteration. */
            if(!fun_i2c_hw_rx_count()) { error=FUN_I2C_FIFO_ERROR; break; }
        }
        if((uint32_t)(fun_cycle_count()-started)>=budget) { error=FUN_I2C_TIMEOUT; break; }
    }
    /* NACK aborts the command list before its STOP. Explicitly run STOP
     * before resetting; merely resetting can leave an unterminated frame. */
    if(error==FUN_I2C_NACK) {
        FUN_I2C_REG(0x24)=0x3ffff;
        FUN_I2C_REG(0x58)=fun_i2c_hw_cmd(FUN_I2C_STOP,0,0,0);
        FUN_I2C_REG(0x5c)=fun_i2c_hw_cmd(4,0,0,0);
        fun_i2c_hw_update(); FUN_I2C_REG(4)|=FUN_BIT(5);
        unsigned stop_start=fun_cycle_count();
        while(!(FUN_I2C_REG(0x58)&FUN_BIT(31)) &&
              (uint32_t)(fun_cycle_count()-stop_start)<fun_cpu_mhz()*1000u) {}
    }
    /* Reset a failed command engine and its FIFOs. A later call can retry;
     * this does not generate automatic GPIO bus-clear pulses. */
    (void)fun_i2c_hw_master_init(bus);
    return error;
}
static inline int fun_i2c_hw_target_init(const fun_i2c_hw *bus,uint8_t address) {
    if(address>127) return FUN_I2C_INVALID;
    int error=fun_i2c_hw_pins(bus); if(error) return error;
    FUN_I2C_REG(4)=3u;
    FUN_I2C_REG(0x10)=address; FUN_I2C_REG(0x30)=10; FUN_I2C_REG(0x34)=10;
#ifdef ESP32FUN_ESP32C3
    FUN_I2C_REG(4)|=FUN_BIT(8)|FUN_BIT(12); FUN_I2C_REG(0x84)=0;
    FUN_I2C_REG(0x0c)=0;
#else
    FUN_I2C_REG(0x0c)=0xfffff;
#endif
    fun_i2c_hw_update(); return 0;
}
#endif
