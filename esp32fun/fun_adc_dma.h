/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Adapted ADC configuration and C3 EOF-counter workaround from ESP-IDF v5.3.1
 * components/hal/esp32c3/include/hal/adc_ll.h. See licenses/Apache-2.0.txt.
 * Standalone numeric register access and GDMA integration added for ESP32fun.
 */
#ifndef FUN_ADC_DMA_H
#define FUN_ADC_DMA_H
#include "fun_dma.h"
#ifndef ESP32FUN_ESP32C3
#error "ADC DMA backend not implemented for this target; current register backend is ESP32-C3"
#endif
#define FUN_ADC_REG(offset) FUN_REG(0x60040000u + (offset))

static inline void fun_adc_analog_write(uint8_t reg, uint8_t msb, uint8_t lsb, uint8_t value)
{
    /* C3 ROM rom_i2c_writeReg_Mask; private analog configuration bus.
     * ROM internal-I2C operation itself has no software timeout in this driver.
     */
    typedef void (*rom_write)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
    ((rom_write)(uintptr_t)0x40001960u)(0x69, 0, reg, msb, lsb, value);
}

/* ADC1 channel 0..4 maps to GPIO0..4. Uncalibrated 12-bit raw results, 0 dB.
 * Owns ADC/SAR analog controls and GDMA pair 0. No radio, temperature sensor,
 * RNG entropy claim, attenuation calibration, ADC2, or voltage conversion.
 */
static inline int fun_adc_dma_init(unsigned channel, unsigned samples_per_second)
{
    if (channel > 4 || samples_per_second < 10000 || samples_per_second > 80000)
        return FUN_DMA_INVALID;
    if (fun_pin_mode(channel, FUN_INPUT)) return FUN_DMA_INVALID;
    FUN_IO_MUX(channel) &= ~(FUN_IO_INPUT_ENABLE | FUN_IO_PULLUP | FUN_IO_PULLDOWN);
    FUN_REG(0x600c0010u) |= FUN_BIT(28);
    FUN_REG(0x600c0018u) |= FUN_BIT(28);
    FUN_REG(0x600c0018u) &= ~FUN_BIT(28);
    /* Enable private analog I2C bus and SAR power under digital controller. */
    FUN_REG(0x6000e044u) &= ~FUN_BIT(18);
    FUN_REG(0x6000e048u) |= FUN_BIT(16);
    FUN_REG(0x60008034u) |= FUN_BIT(22);
    FUN_REG(0x6000811cu) &= ~(3u << 30); /* RTC SAR power follows FSM */
    fun_adc_analog_write(2, 2, 0, 2); /* sample cycle */
    fun_adc_analog_write(2, 6, 4, 1); /* ADC1 reference setting */
    fun_adc_analog_write(7, 5, 5, 0); /* disconnect calibration ground */
    /* Initial code remains inherited from ROM; eFuse/software calibration absent. */
    FUN_ADC_REG(0x0c) = (100u << 16) | (8u << 8) | 5u;
    FUN_ADC_REG(0x00) = (3u << 27) | (1u << 7) | FUN_BIT(6) | FUN_BIT(23);
    FUN_ADC_REG(0x00) &= ~FUN_BIT(23); /* reset pattern pointer, length=1 */
    FUN_ADC_REG(0x18) = (channel << 2) << 18;
    FUN_ADC_REG(0x1c) = 0;
    unsigned apb_mhz = fun_cpu_mhz();
    if (apb_mhz > 80) apb_mhz = 80;
    unsigned interval = apb_mhz * 1000000u / 16u / 2u / samples_per_second;
    if (interval < 30 || interval > 4095) return FUN_DMA_INVALID;
    FUN_ADC_REG(0x04) = interval << 12; /* inversion off, conversion limit off */
    FUN_ADC_REG(0x54) = (2u << 21) | FUN_BIT(20) | (1u << 8) | 15u;
    FUN_ADC_REG(0x50) = FUN_BIT(30);
    FUN_ADC_REG(0x50) = 0;
    fun_dma_reset();
    Delay_Us(100);
    return 0;
}

/* Finite timer-paced block: ADC writes samples through GDMA, CPU polls EOF.
 * Count is limited to one descriptor. Raw word: data[11:0], channel[15:13],
 * unit[16]. Stop the timer before releasing the descriptor or printing results.
 */
static inline int fun_adc_dma_read(uint32_t *samples, unsigned count, unsigned timeout_us)
{
    fun_dma_desc rx;
    if (!count || count > FUN_DMA_MAX_BYTES / 4u || !fun_dma_timeout_valid(timeout_us) ||
        fun_dma_descriptor(&rx, samples, count * 4u, 0, 1)) return FUN_DMA_INVALID;
    FUN_ADC_REG(0x04) &= ~FUN_BIT(24);
    /* C3 erratum: the sample counter is not cleared by FSM reset.
     * Sweeping EOF threshold through its previous value clears the counter.
     */
    unsigned previous = FUN_ADC_REG(0x50) & 0xffffu;
    FUN_ADC_REG(0x50) &= ~FUN_BIT(31);
    for (unsigned i = 0; i <= previous; ++i) FUN_ADC_REG(0x50) = i;
    FUN_ADC_REG(0x50) = count;
    fun_dma_reset();
    FUN_GDMA_IN_PERIPHERAL = 8; /* ADC */
    fun_dma_fence();
    FUN_GDMA_IN_LINK = ((uint32_t)(uintptr_t)&rx & 0xfffffu) | FUN_BIT(22);
    FUN_ADC_REG(0x50) |= FUN_BIT(31);
    FUN_ADC_REG(0x04) |= FUN_BIT(24);
    int result = fun_dma_wait(FUN_GDMA_RX_EOF, timeout_us, 0);
    FUN_ADC_REG(0x04) &= ~FUN_BIT(24);
    FUN_ADC_REG(0x50) &= ~FUN_BIT(31);
    fun_dma_reset();
    if (!result && ((rx.control & FUN_DMA_OWNER) || ((rx.control >> 12) & 4095u) != count * 4u))
        result = FUN_DMA_ERROR;
    return result;
}
#endif
