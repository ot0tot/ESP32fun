/* SPDX-License-Identifier: MIT */
#ifndef FUN_PWM_H
#define FUN_PWM_H
#include "esp32fun.h"

#if !FUN_PWM_SUPPORTED
#error "This LEDC register driver supports ESP32/S2/S3/C2/C3; gamma-layout chips need a separate driver"
#endif

/* One global source: APB on ESP32, XTAL on S2/S3/C2/C3. Override for a
 * 26 MHz crystal. This does not change the CPU or assume a PLL is running.
 */
#ifndef FUN_PWM_SOURCE_HZ
#ifdef ESP32FUN_ESP32
#define FUN_PWM_SOURCE_HZ ((fun_cpu_mhz() < 80u ? fun_cpu_mhz() : 80u) * 1000000u)
#else
#define FUN_PWM_SOURCE_HZ 40000000u
#endif
#endif

typedef struct { unsigned channel, timer, pin, bits; } fun_pwm;

/* Initialize the shared timer once, then bind any number of channels to it. */
static inline int fun_pwm_timer(unsigned timer, unsigned bits, uint32_t hz)
{
    if (timer >= 4u || bits < 1u || bits > FUN_PWM_BITS || !hz) return -1;
    uint64_t denominator = (uint64_t)hz << bits;
    uint32_t divider = (uint32_t)(((uint64_t)FUN_PWM_SOURCE_HZ * 256u + denominator / 2u) / denominator);
    if (divider < 256u || divider > FUN_PWM_DIV_MAX) return -2;
    FUN_REG(FUN_PWM_BUS_CLOCK) |= FUN_PWM_BUS_ENABLE;
    FUN_REG(FUN_PWM_BUS_RESET) &= ~FUN_PWM_BUS_RESET_MASK;
    FUN_REG(FUN_PWM_CONF) = (FUN_REG(FUN_PWM_CONF) & ~FUN_PWM_CLOCK_SELECT_MASK) |
                           FUN_PWM_CLOCK_SELECT | FUN_PWM_CONF_CLOCK;
    uintptr_t config = FUN_PWM_TIMER_BASE + 8u * timer;
    uint32_t value = bits | (divider << FUN_PWM_DIV_SHIFT) | FUN_PWM_TIMER_GLOBAL;
    FUN_REG(config) = value | FUN_PWM_TIMER_RESET | FUN_PWM_TIMER_UPDATE;
    FUN_REG(config) = value | FUN_PWM_TIMER_UPDATE;
    return 0;
}

static inline int fun_pwm_duty(const fun_pwm *pwm, uint32_t duty, uint32_t phase)
{
    if (pwm->channel >= FUN_PWM_CHANNELS || pwm->bits < 1u || pwm->bits > FUN_PWM_BITS ||
        duty > (1u << pwm->bits) || phase >= (1u << pwm->bits)) return -1;
    /* Full duty at maximum resolution can overflow the hardware counter. */
    if (pwm->bits == FUN_PWM_BITS && duty == (1u << pwm->bits)) return -1;
    uintptr_t channel = FUN_PWM_CHANNEL_BASE + FUN_PWM_CHANNEL_STRIDE * pwm->channel;
    FUN_REG(channel + 4u) = phase;
    FUN_REG(channel + 8u) = duty << 4; /* Four fractional bits. */
    /* Single non-fading update: increment direction, one step, one cycle,
     * scale zero. Start and latch after all fields have been written.
     */
    FUN_REG(channel + 12u) = FUN_BIT(31) | FUN_BIT(30) | FUN_BIT(20) | FUN_BIT(10);
    FUN_REG(channel) |= FUN_BIT(4);
    return 0;
}

static inline int fun_pwm_bind(const fun_pwm *pwm)
{
    if (pwm->channel >= FUN_PWM_CHANNELS || pwm->timer >= 4u ||
        pwm->bits < 1u || pwm->bits > FUN_PWM_BITS || fun_pin_mode(pwm->pin, FUN_OUTPUT)) return -1;
    uintptr_t channel = FUN_PWM_CHANNEL_BASE + FUN_PWM_CHANNEL_STRIDE * pwm->channel;
    FUN_REG(channel) = pwm->timer | FUN_BIT(2);
    if (fun_pwm_duty(pwm, 0, 0)) return -1;
    FUN_GPIO_OUTPUT_SELECT(pwm->pin) = FUN_PWM_SIGNAL + pwm->channel;
    return 0;
}
#endif
