/* SPDX-License-Identifier: MIT
 * Original ESP32fun test for Waveshare ESP32-C6-LCD-1.47(-M).
 * Pin map and ST7789 geometry: https://docs.waveshare.com/ESP32-C6-LCD-1.47
 * Polling SPI and cycle-timed RGB; this example does not qualify GDMA/RMT.
 */
#include "esp32fun.h"
#ifndef ESP32FUN_ESP32C6
#error "This example uses the Waveshare C6 board pin map"
#endif
enum { MOSI=6, SCK=7, CS=14, DC=15, RST=21, BL=22, RGB=8, SD_CS=4 };
static unsigned mhz;

static void byte(uint8_t data)
{
    for (unsigned bit=0;bit<8;++bit) {
        fun_digital_write(MOSI,data & 0x80);
        fun_digital_write(SCK,1);
        fun_digital_write(SCK,0);
        data <<= 1;
    }
}
static void command(uint8_t cmd,const uint8_t *data,unsigned count)
{
    fun_digital_write(CS,0);fun_digital_write(DC,0);byte(cmd);
    fun_digital_write(DC,1);
    while(count--)byte(*data++);
    fun_digital_write(CS,1);
}
static void rgb(uint8_t red,uint8_t green,uint8_t blue)
{
    const uint8_t grb[]={green,red,blue};
    for (unsigned i=0;i<3;++i)for(unsigned bit=0;bit<8;++bit) {
        unsigned high=(grb[i] & (0x80u>>bit)) ? mhz*7/10 : mhz*35/100;
        FUN_GPIO_OUT_SET=FUN_BIT(RGB);
        uint32_t start=fun_cycle_count();
        while((uint32_t)(fun_cycle_count()-start)<high){}
        FUN_GPIO_OUT_CLEAR=FUN_BIT(RGB);
        while((uint32_t)(fun_cycle_count()-start)<mhz*13/10){}
    }
    Delay_Us(100); /* low latch, outside bit timing */
}
int main(void)
{
    /* Verified C6 ROM entry point, ets_install_usb_printf. USB pins 12/13
     * are left in their ROM configuration. All application code is in SRAM.
     */
    ((void (*)(void))(uintptr_t)0x40000038u)();
    mhz=fun_cpu_mhz();
    const unsigned pins[]={MOSI,SCK,CS,DC,RST,BL,RGB,SD_CS};
    for(unsigned i=0;i<sizeof(pins)/sizeof(pins[0]);++i)
        if(fun_pin_mode(pins[i],FUN_OUTPUT))return 1;
    fun_digital_write(SD_CS,1);fun_digital_write(CS,1);
    fun_digital_write(RST,0);Delay_Ms(20);
    fun_digital_write(RST,1);Delay_Ms(120);
    command(0x01,0,0);Delay_Ms(150);
    command(0x11,0,0);Delay_Ms(120);
    const uint8_t format[]={0x55},orientation[]={0x00};
    command(0x3a,format,1);command(0x36,orientation,1);
    command(0x21,0,0);command(0x29,0,0);Delay_Ms(20);
    const uint8_t columns[]={0,34,0,205},rows[]={0,0,1,63};
    command(0x2a,columns,4);command(0x2b,rows,4);
    command(0x2c,0,0);fun_digital_write(CS,0);fun_digital_write(DC,1);
    const uint16_t colors[]={0xffff,0xf800,0x07e0,0x001f,0x0000};
    for(unsigned y=0;y<320;++y)for(unsigned x=0;x<172;++x) {
        uint16_t color=colors[x*5/172];byte(color>>8);byte(color);
    }
    fun_digital_write(CS,1);
    fun_printf("C6 LCD READY pixels=55040 rgb_gpio=8 cpu_mhz=%u backlight_percent=25\n",mhz);
    for(unsigned phase=0;;++phase) {
        rgb(phase%3==0?16:0,phase%3==1?16:0,phase%3==2?16:0);
        fun_printf("C6 RGB phase=%u color=%s mhz=%u\n",phase,phase%3==0?"red":phase%3==1?"green":"blue",mhz);
        /* Board vendor requests <=50% brightness. Use 25% software PWM;
         * no assumption about an unvalidated C6 LEDC register map.
         */
        for(unsigned ms=0;ms<1000;++ms) {
            fun_digital_write(BL,1);Delay_Us(250);
            fun_digital_write(BL,0);Delay_Us(750);
        }
    }
}
