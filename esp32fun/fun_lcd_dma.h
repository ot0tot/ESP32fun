/* SPDX-License-Identifier: MIT
 * Original ESP32-S3 eight-bit parallel output through LCD_CAM and GDMA.
 * Numeric facts checked against ESP-IDF v5.3.1 lcd_cam_reg.h and lcd_ll.h.
 */
#ifndef FUN_LCD_DMA_H
#define FUN_LCD_DMA_H
#include "fun_dma.h"
#ifndef ESP32FUN_ESP32S3
#error "Parallel LCD/GDMA output currently supports ESP32-S3 only"
#endif
#define FUN_LCD_REG(offset) FUN_REG(0x60041000u + (offset))
#define FUN_LCD_USER FUN_LCD_REG(0x14)
#define FUN_LCD_START FUN_BIT(27)
#define FUN_LCD_RESET FUN_BIT(28)
#define FUN_LCD_UPDATE FUN_BIT(20)
#define FUN_LCD_BLANK_ENABLE FUN_BIT(26)
#define FUN_LCD_SETUP_HOLD (FUN_LCD_BLANK_ENABLE | FUN_BIT(6))

/* Eight contiguous GPIOs, byte bit 0 on first_pin. Clock on clock_pin.
 * Sample rate = 40 MHz XTAL / divider / 2, divider in [2,255].
 * Owns LCD_CAM (including camera registers) and GDMA channel pair 0.
 */
static inline int fun_lcd_dma_init(unsigned first_pin, unsigned clock_pin, unsigned divider)
{
    if (first_pin > FUN_GPIO_COUNT - 8u || divider < 2 || divider > 255 ||
        (clock_pin >= first_pin && clock_pin < first_pin + 8u)) return FUN_DMA_INVALID;
    for (unsigned i = 0; i < 8; ++i)
        if (!fun_pin_valid(first_pin + i)) return FUN_DMA_INVALID;
    if (fun_pin_mode(clock_pin, FUN_OUTPUT)) return FUN_DMA_INVALID;
    for (unsigned i = 0; i < 8; ++i) {
        if (fun_pin_mode(first_pin + i, FUN_OUTPUT)) return FUN_DMA_INVALID;
        FUN_GPIO_OUTPUT_SELECT(first_pin + i) = (133u + i) | FUN_GPIO_ENABLE_FROM_REG;
    }
    FUN_GPIO_OUTPUT_SELECT(clock_pin) = 154u | FUN_GPIO_ENABLE_FROM_REG;
    FUN_REG(0x600c001cu) |= FUN_BIT(8); /* LCD_CAM clock */
    FUN_REG(0x600c0024u) |= FUN_BIT(8);
    FUN_REG(0x600c0024u) &= ~FUN_BIT(8);
    FUN_LCD_REG(0x00) = FUN_BIT(31) | (1u << 29) | (divider << 9) | 1u;
    FUN_LCD_USER = FUN_LCD_RESET;
    FUN_LCD_REG(0x18) = FUN_BIT(27) | FUN_LCD_SETUP_HOLD;
    FUN_LCD_REG(0x10) = 0; /* no YUV conversion */
    FUN_LCD_REG(0x1c) = 0; /* no RGB frame controller */
    FUN_LCD_REG(0x38) = 0; /* data delay modes */
    FUN_LCD_REG(0x64) = 0; /* CPU interrupts disabled */
    FUN_LCD_REG(0x70) = 15;
    fun_dma_reset();
    return 0;
}

static inline int fun_lcd_dma_transfer(fun_dma_desc *tx, unsigned count, unsigned timeout_us)
{
    int bytes = fun_dma_validate(tx, count, 0);
    unsigned mhz = fun_cpu_mhz();
    if (bytes < 0 || bytes > 8192 || !mhz || !timeout_us || timeout_us > 0x7fffffffu / mhz)
        return FUN_DMA_INVALID;
    FUN_LCD_USER = FUN_LCD_RESET;
    /* LCD-239: two setup PCLK cycles * prescale 2 = four LCD_CLK cycles,
     * strictly above the faulty <=2 range. One hold cycle after the data.
     */
    FUN_LCD_REG(0x18) = FUN_BIT(27) | FUN_LCD_SETUP_HOLD;
    FUN_LCD_REG(0x70) = 15;
    /* 8-bit stride, no command/dummy phases or byte swap, finite data length. */
    FUN_LCD_USER = FUN_BIT(24) | ((unsigned)bytes - 1u);
    fun_dma_reset();
    FUN_GDMA_OUT_PERIPHERAL = 5; /* LCD_CAM */
    fun_dma_fence();
    FUN_GDMA_OUT_LINK = ((uint32_t)(uintptr_t)tx & 0xfffffu) | FUN_BIT(21);
    Delay_Us(1); /* allow DMA to prefill LCD FIFO before the first output clock */
    FUN_LCD_USER |= FUN_LCD_UPDATE;
    FUN_LCD_USER |= FUN_LCD_START;
    unsigned start = fun_cycle_count(), budget = timeout_us * mhz;
    int result = 0;
    for (;;) {
        unsigned status = FUN_GDMA_RAW;
        if (status & FUN_GDMA_ERRORS) { result = FUN_DMA_ERROR; break; }
        if ((status & FUN_GDMA_TX_EOF) && (FUN_LCD_REG(0x68) & FUN_BIT(1))) break;
        if ((unsigned)(fun_cycle_count() - start) >= budget) { result = FUN_DMA_TIMEOUT; break; }
    }
    FUN_LCD_USER &= ~FUN_LCD_START;
    if (result) FUN_LCD_USER = FUN_LCD_RESET;
    fun_dma_reset();
    return result;
}
#endif
