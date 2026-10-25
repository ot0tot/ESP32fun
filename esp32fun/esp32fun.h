/* SPDX-License-Identifier: MIT */
#ifndef ESP32FUN_H
#define ESP32FUN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(FUN_TARGET_NAME)
#error "Build with tools/esp32fun.py --chip <target> to select a target header"
#endif

#define FUN_BIT(n) (UINT32_C(1) << (n))
#define FUN_REG(address) (*(volatile uint32_t *)(uintptr_t)(address))

#ifndef FUN_CONSOLE_USB
#define FUN_CONSOLE_USB 0
#endif
#if FUN_CONSOLE_USB && !defined(ESP32FUN_ESP32C3) && !defined(ESP32FUN_ESP32S3)
#error "Explicit native USB console currently supports ESP32-C3 and ESP32-S3 only"
#endif

/* Target-specific numeric definitions are supplied by the build frontend. */
#define FUN_GPIO_OUT FUN_REG(FUN_GPIO_BASE + 0x04)
#define FUN_GPIO_OUT_SET FUN_REG(FUN_GPIO_BASE + 0x08)
#define FUN_GPIO_OUT_CLEAR FUN_REG(FUN_GPIO_BASE + 0x0c)
#ifndef FUN_GPIO_ENABLE_SET_OFFSET
#define FUN_GPIO_ENABLE_SET_OFFSET 0x24u
#define FUN_GPIO_ENABLE_SET_HIGH_OFFSET 0x30u
#define FUN_GPIO_IN_LOW_OFFSET 0x3cu
#define FUN_GPIO_IN_HIGH_OFFSET 0x40u
#endif
#define FUN_GPIO_ENABLE_SET FUN_REG(FUN_GPIO_BASE + FUN_GPIO_ENABLE_SET_OFFSET)
#define FUN_GPIO_ENABLE_CLEAR FUN_REG(FUN_GPIO_BASE + FUN_GPIO_ENABLE_SET_OFFSET + 4u)
#define FUN_GPIO_IN FUN_REG(FUN_GPIO_BASE + FUN_GPIO_IN_LOW_OFFSET)
#define FUN_GPIO_PIN(pin) FUN_REG(FUN_GPIO_BASE + FUN_GPIO_PIN_OFFSET + 4u * (pin))
#define FUN_GPIO_OUTPUT_SELECT(pin) FUN_REG(FUN_GPIO_BASE + FUN_GPIO_SELECT_OFFSET + 4u * (pin))
extern const uintptr_t fun_pin_mux[FUN_GPIO_COUNT];
#define FUN_IO_MUX(pin) FUN_REG(fun_pin_mux[pin])

static inline int fun_pin_valid(unsigned pin)
{
    if (pin >= FUN_GPIO_COUNT) return 0;
    return (int)(((pin < 32u ? FUN_GPIO_VALID_LOW : FUN_GPIO_VALID_HIGH) >> (pin & 31u)) & 1u);
}

typedef enum {
    FUN_INPUT,
    FUN_INPUT_PULLUP,
    FUN_INPUT_PULLDOWN,
    FUN_OUTPUT,
    FUN_OPEN_DRAIN
} fun_gpio_mode;

/* Returns -1 for an invalid/unsupported pin or mode. Push-pull starts low;
 * open drain starts released. Classic input-only pads reject pull modes.
 */
int fun_pin_mode(unsigned pin, fun_gpio_mode mode);

static inline void fun_digital_write(unsigned pin, int high)
{
    if (fun_pin_valid(pin)) {
        uintptr_t bank = pin < 32u ? 0x08u : 0x14u;
        FUN_REG(FUN_GPIO_BASE + bank + (high ? 0 : 4u)) = FUN_BIT(pin & 31u);
    }
}

static inline int fun_digital_read(unsigned pin)
{
    return fun_pin_valid(pin) ? (int)((FUN_REG(FUN_GPIO_BASE + (pin < 32u ? FUN_GPIO_IN_LOW_OFFSET : FUN_GPIO_IN_HIGH_OFFSET)) >> (pin & 31u)) & 1u) : 0;
}

static inline uint32_t fun_cycle_count(void)
{
    uint32_t cycles;
#if FUN_ARCH_XTENSA
    __asm__ volatile ("rsr %0, ccount" : "=a" (cycles));
#else
    __asm__ volatile ("csrr %0, %1" : "=r" (cycles) : "i" (FUN_CYCLE_CSR));
#endif
    return cycles;
}

/* ROM-backed calls; no libc or scheduler. Startup reconciles supported clock
 * bookkeeping where implemented, without switching the inherited clock.
 */
void Delay_Us(uint32_t microseconds);
void Delay_Ms(uint32_t milliseconds);
int fun_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
void fun_puts(const char *text);
int fun_putchar(uint8_t character);
int fun_getchar(uint8_t *character); /* 0 when a character was read, 1 when empty */
uint32_t fun_cpu_mhz(void);
#ifdef ESP32FUN_ESP32C6
uint32_t fun_rom_cpu_mhz(void); /* ROM fallback; address from profiles.json. */
#endif
void fun_rom_set_cpu_mhz(uint32_t mhz); /* delay calibration only; not a clock switch */
void fun_reset(void) __attribute__((noreturn));
int fun_reset_reason(int cpu);
void fun_delay_cycles(uint32_t cycles);
uint32_t fun_crc32(const void *data, size_t length);

/* Called by startup before main; safe to call again. Disables boot watchdogs. */
void SystemInit(void);

/* On an unexpected trap, startup records these values and stops. */
extern volatile struct fun_trap_record {
    uint32_t cause;
    uint32_t pc;
    uint32_t value;
} fun_last_trap;

#ifdef __cplusplus
}
#endif
#endif
