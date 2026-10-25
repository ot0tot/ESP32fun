/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
#ifdef ESP32FUN_ESP32
#include "fun_clock_esp32.h"
#endif

volatile struct fun_trap_record fun_last_trap;
const uintptr_t fun_pin_mux[FUN_GPIO_COUNT] = FUN_PIN_MUX_ADDRESSES;

extern uint32_t __bss_start[], __bss_end[];
extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
extern int main(void);

#ifdef ESP32FUN_ESP32C6
/* C6 direct ROM boot can leave the ROM's cached CPU frequency at 53 MHz
 * with a 480 MHz PLL / 3 root and no CPU divider (160 MHz). Read the
 * hardware tree instead. Numeric facts: C6 PCR SYSCLK_CONF/CPU_FREQ_CONF,
 * checked against esp32c6 PAC 0.25.0 and esp-hal 1.2.2 C6 clock tree.
 * This changes delay calibration, never the clock mux or dividers.
 */
uint32_t fun_cpu_mhz(void)
{
    uint32_t root=FUN_REG(0x60096110u),cpu=FUN_REG(0x60096118u);
    unsigned source=(root>>16)&3u;
    if(source==0u) return ((root>>24)&127u)/((root&255u)+1u)/((cpu&255u)+1u);
    if(source==1u) {
        if((cpu&FUN_BIT(16)) && !((cpu>>8)&255u)) return 120u;
        return 480u/(((root>>8)&255u)+1u)/(((cpu>>8)&255u)+1u);
    }
    /* FOSC is uncalibrated: preserve ROM behavior for that uncommon mode. */
    return fun_rom_cpu_mhz();
}
#endif

#if FUN_CONSOLE_USB
#include "fun_usb_console.h"
/* Native C3/S3 USB uses separate ROM character entry points. Install its printf
 * callback explicitly for both flash and ROM-download RAM boot paths.
 * These definitions override the linker's default PROVIDE aliases.
 */
extern int fun_usb_putchar(uint8_t character);
extern int fun_usb_getchar(uint8_t *character);
extern void fun_usb_flush(void);
int fun_putchar(uint8_t character)
{
    int result = fun_usb_putchar(character);
    if (character == '\n') fun_usb_flush();
    return result;
}
static void fun_usb_print_character(char character) { (void)fun_putchar((uint8_t)character); }
int fun_getchar(uint8_t *character) { return fun_usb_getchar(character); }
#endif

void __attribute__((noreturn)) fun_runtime_start(void)
{
    for (uint32_t *p = __bss_start; p < __bss_end; ++p) *p = 0;
    SystemInit();
    for (void (**p)(void) = __init_array_start; p < __init_array_end; ++p) (*p)();
    (void)main();
    for (;;) __asm__ volatile ("nop");
}

void __cxa_pure_virtual(void) { for (;;) __asm__ volatile ("nop"); }

void SystemInit(void)
{
#ifdef ESP32FUN_ESP32
    /* Direct flash/RAM ROM entry can retain 13 MHz delay metadata at XTAL40.
     * Unsupported clock modes retain ROM behavior; no clock registers change.
     */
    (void)fun_esp32_clock_sync(FUN_ESP32_XTAL_MHZ);
#endif
#ifdef ESP32FUN_ESP32C6
    fun_rom_set_cpu_mhz(fun_cpu_mhz());
#endif
    /* Disable both normal and flash-boot watchdog modes, then re-lock. */
    FUN_REG(FUN_RTC_WDT_PROTECT) = 0x50d83aa1;
    FUN_REG(FUN_RTC_WDT_CONFIG) &= ~(FUN_BIT(31) | FUN_RTC_WDT_FLASH_MASK);
    FUN_REG(FUN_RTC_WDT_PROTECT) = 0;

    /* Super watchdog: hardware auto-feed, as used during IDF startup. */
#if FUN_SWD_CONFIG
    FUN_REG(FUN_SWD_PROTECT) = 0x8f1d312a;
    FUN_REG(FUN_SWD_CONFIG) |= FUN_SWD_AUTO_FEED_MASK;
    FUN_REG(FUN_SWD_PROTECT) = 0;
#endif

    for (unsigned group = 0; group < FUN_TIMER_GROUPS; ++group) {
        uintptr_t base = FUN_TIMER_BASE + group * 0x1000;
        FUN_REG(base + FUN_TIMER_WDT_PROTECT_OFFSET) = 0x50d83aa1;
        FUN_REG(base + FUN_TIMER_WDT_CONFIG_OFFSET) &= ~(FUN_BIT(31) | FUN_TIMER_WDT_FLASH_MASK);
        FUN_REG(base + FUN_TIMER_WDT_PROTECT_OFFSET) = 0;
    }
#if FUN_GPIO_CLOCK_GATE_ADDRESS
    FUN_REG(FUN_GPIO_CLOCK_GATE_ADDRESS) |= FUN_BIT(0);
#endif
#if FUN_CONSOLE_USB
    fun_usb_console_install(fun_usb_print_character);
#endif
}

int fun_pin_mode(unsigned pin, fun_gpio_mode mode)
{
    if (!fun_pin_valid(pin) || (unsigned)mode > FUN_OPEN_DRAIN) return -1;
#ifdef ESP32FUN_ESP32
    /* Classic ESP32's input-only pads do not contain pull resistors. */
    if (pin >= 34u && (mode == FUN_INPUT_PULLUP || mode == FUN_INPUT_PULLDOWN)) return -1;
#endif
    uint32_t mask = FUN_BIT(pin & 31u);
    if (mode >= FUN_OUTPUT && !((pin < 32u ? FUN_GPIO_OUTPUT_LOW : FUN_GPIO_OUTPUT_HIGH) & mask)) return -1;
    uintptr_t enable = FUN_GPIO_BASE + (pin < 32u ? FUN_GPIO_ENABLE_SET_OFFSET : FUN_GPIO_ENABLE_SET_HIGH_OFFSET);
    FUN_REG(enable + 4u) = mask;
    FUN_GPIO_PIN(pin) = 0; /* Push-pull; no GPIO interrupts. */
    FUN_GPIO_OUTPUT_SELECT(pin) = FUN_GPIO_SIGNAL | FUN_GPIO_ENABLE_FROM_REG;
    uint32_t mux = FUN_IO_MUX(pin);
    mux &= ~(FUN_IO_FUNCTION_MASK | FUN_IO_PULLUP | FUN_IO_PULLDOWN);
    mux |= FUN_IO_FUNCTION_GPIO | FUN_IO_INPUT_ENABLE;
    if (mode == FUN_INPUT_PULLUP) mux |= FUN_IO_PULLUP;
    if (mode == FUN_INPUT_PULLDOWN) mux |= FUN_IO_PULLDOWN;
    FUN_IO_MUX(pin) = mux;
    if (mode >= FUN_OUTPUT) {
        if (mode == FUN_OPEN_DRAIN) FUN_GPIO_PIN(pin) |= FUN_BIT(2);
        fun_digital_write(pin, mode == FUN_OPEN_DRAIN);
        FUN_REG(enable) = mask;
    }
    return 0;
}

void Delay_Ms(uint32_t milliseconds)
{
    /* Avoid overflow of milliseconds * 1000. */
    while (milliseconds--) Delay_Us(1000);
}

void fun_puts(const char *text)
{
    fun_printf("%s\n", text);
}

void fun_delay_cycles(uint32_t cycles)
{
    uint32_t start = fun_cycle_count();
    while ((uint32_t)(fun_cycle_count() - start) < cycles) __asm__ volatile ("nop");
}

uint32_t fun_crc32(const void *data, size_t length)
{
    const uint8_t *p = data;
    uint32_t crc = UINT32_MAX;
    while (length--) {
        crc ^= *p++;
        for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
