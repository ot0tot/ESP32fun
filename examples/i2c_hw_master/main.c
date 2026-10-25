/* SPDX-License-Identifier: MIT
 * Hardware I2C master to the register-target demo, with repeated START and
 * FIFO-boundary tests. GPIO4/5 are intentionally crossed on the current boards.
 */
#include "fun_i2c_hw.h"
#ifdef ESP32FUN_ESP32
#define MASTER_SDA 5u
#define MASTER_SCL 4u
#else
#define MASTER_SDA 4u
#define MASTER_SCL 5u
#endif
#ifndef I2C_HW_HZ
#define I2C_HW_HZ 100000u
#endif
#ifndef I2C_HW_PAIRS
#define I2C_HW_PAIRS 1000u
#endif
static fun_i2c_hw bus={MASTER_SDA,MASTER_SCL,I2C_HW_HZ,100000};
static uint8_t tx[129],rx[128];
static unsigned test(unsigned n,unsigned seed) {
    tx[0]=0x20;
    for(unsigned i=0;i<n;++i) tx[1+i]=(uint8_t)(i^seed);
    int w=fun_i2c_hw_transfer(&bus,0x42,tx,n+1,0,0);
    Delay_Us(500);
    int r=fun_i2c_hw_transfer(&bus,0x42,tx,1,rx,n);
    unsigned bad=0;
    for(unsigned i=0;i<n;++i) bad+=rx[i]!=tx[i+1];
    fun_printf("HW CASE bytes=%u seed=%u write=%d repeated_read=%d mismatches=%u CRC=%08x\n",
               n,seed,w,r,bad,(unsigned)fun_crc32(rx,n));
    Delay_Ms(10); return (w!=0)+(r!=0)+bad;
}
int main(void) {
    int init=fun_i2c_hw_master_init(&bus);
    fun_printf("I2C HW MASTER READY SDA=%u SCL=%u hz=%u init=%d CPU=%u\n",
               MASTER_SDA,MASTER_SCL,I2C_HW_HZ,init,(unsigned)fun_cpu_mhz());
    if(init) return 1;
    for(;;) {
        uint8_t c; if(fun_getchar(&c)) continue;
        if(c=='M') {
            int probe=fun_i2c_hw_transfer(&bus,0x42,0,0,0,0);
            Delay_Ms(10);
            int absent=fun_i2c_hw_transfer(&bus,0x43,0,0,0,0);
            fun_printf("HW PROBE present=%d absent=%d\n",probe,absent);
            Delay_Ms(10);
            static const unsigned sizes[]={1,2,3,7,16,31,32,33,64,127,128};
            unsigned errors=(probe!=0)+(absent!=FUN_I2C_NACK);
            for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) errors+=test(sizes[i],37+i);
            fun_printf("HW SUITE DONE errors=%u hz=%u\n",errors,bus.hz);
        } else if(c=='F') {
            unsigned errors=0,bad=0,done=0;
            for(unsigned k=0;k<I2C_HW_PAIRS;++k) {
                tx[0]=0x20; for(unsigned i=0;i<64;++i) tx[1+i]=(uint8_t)(i^(k*73u+37u));
                int w=fun_i2c_hw_transfer(&bus,0x42,tx,65,0,0); Delay_Us(500);
                int r=fun_i2c_hw_transfer(&bus,0x42,tx,1,rx,64);
                unsigned mismatch=0; for(unsigned i=0;i<64;++i) mismatch+=rx[i]!=tx[1+i];
                errors+=(w!=0)+(r!=0); bad+=mismatch; ++done; Delay_Us(500);
                if(w || r || mismatch) break;
            }
            fun_printf("HW BENCH DONE hz=%u pairs=%u errors=%u mismatches=%u\n",bus.hz,done,errors,bad);
        } else if(c=='S' || c=='N' || c=='W' || c=='T') {
            for(unsigned i=0;i<8;++i) tx[i]=(uint8_t)(i^0x53);
            unsigned begin=fun_cycle_count();
            int result=fun_i2c_hw_transfer(&bus,0x42,tx,8,0,0);
            fun_printf("HW FAULT command=%c write=%d elapsed_us=%u SDA=%d SCL=%d\n",c,result,
                       (unsigned)((fun_cycle_count()-begin)/fun_cpu_mhz()),
                       fun_digital_read(bus.sda),fun_digital_read(bus.scl));
            Delay_Ms(50);
            if(c!='N' && c!='T') {
                int read=fun_i2c_hw_transfer(&bus,0x42,0,0,rx,8);
                unsigned bad=0;for(unsigned i=0;i<8;++i) bad+=rx[i]!=(uint8_t)(tx[i]^0x5a);
                fun_printf("HW FAULT command=%c read=%d mismatches=%u\n",c,read,bad);
            }
        } else if(c=='B') {
            unsigned begin=fun_cycle_count();
            int result=fun_i2c_hw_transfer(&bus,0x42,0,0,0,0);
            fun_printf("HW BUSY result=%d elapsed_us=%u SDA=%d SCL=%d\n",result,
                       (unsigned)((fun_cycle_count()-begin)/fun_cpu_mhz()),
                       fun_digital_read(bus.sda),fun_digital_read(bus.scl));
        }
    }
}
