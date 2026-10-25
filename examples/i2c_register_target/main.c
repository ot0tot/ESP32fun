/* SPDX-License-Identifier: MIT
 * Hardware I2C register target: address 0x42, 256 writable byte registers.
 * Write a register pointer then data. Reads stream from the current pointer;
 * the read pointer restarts at that value after STOP (not EEPROM semantics).
 * Service must run continuously; this polling demo has no IRQ dispatcher.
 */
#include "fun_i2c_hw.h"
#ifdef ESP32FUN_ESP32
#define TARGET_SDA 5u
#define TARGET_SCL 4u
#else
#define TARGET_SDA 4u
#define TARGET_SCL 5u
#endif
static fun_i2c_hw bus={TARGET_SDA,TARGET_SCL,100000,100000};
static uint8_t registers[256],position,read_position;
static unsigned received,rx_bytes,transactions,tx_bytes;
static void prepare(void) {
    fun_i2c_hw_fifo_reset(FUN_BIT(13)); read_position=position;
    for(unsigned i=0;i<32;++i) FUN_I2C_FIFO=registers[read_position++];
}
static void service(void) {
    unsigned changed=0;
    while(fun_i2c_hw_rx_count()) {
        uint8_t value=(uint8_t)FUN_I2C_FIFO;
        if(!received++) { position=value; changed=1; } else registers[position++]=value;
        ++rx_bytes;
    }
    if(changed) prepare();
    unsigned raw=FUN_I2C_REG(0x20);
    if(raw&FUN_BIT(7)) {
        ++transactions; received=0; FUN_I2C_REG(0x24)=0x3ffff; prepare();
    }
    unsigned count=fun_i2c_hw_tx_count();
    for(unsigned i=count;i<32;++i) { FUN_I2C_FIFO=registers[read_position++]; ++tx_bytes; }
}
int main(void) {
    for(unsigned i=0;i<256;++i) registers[i]=(uint8_t)(i^0x5a);
    if(fun_i2c_hw_target_init(&bus,0x42)) return 1;
    prepare();
    fun_printf("I2C HW REGISTER TARGET READY addr=42 SDA=%u SCL=%u CPU=%u\n",
               TARGET_SDA,TARGET_SCL,(unsigned)fun_cpu_mhz());
    for(;;) {
        service();
        if(!(FUN_I2C_REG(8)&FUN_BIT(4))) {
            uint8_t c;
            if(!fun_getchar(&c) && c=='Q') fun_printf("TARGET transactions=%u rx_bytes=%u tx_refills=%u memory_crc=%08x\n",
                transactions,rx_bytes,tx_bytes,(unsigned)fun_crc32(registers,256));
        }
    }
}
