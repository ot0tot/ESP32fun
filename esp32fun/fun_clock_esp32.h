/* SPDX-License-Identifier: MIT
 * Original classic ESP32 clock decoding. Numeric fields checked against
 * pinned ESP-IDF v5.3.1 rtc_cntl_reg.h, dport_reg.h and apb_ctrl_reg.h.
 * Reconcile ROM delay bookkeeping without switching the hardware clock.
 */
#ifndef FUN_CLOCK_ESP32_H
#define FUN_CLOCK_ESP32_H
#include <stdint.h>
#ifndef FUN_ESP32_XTAL_MHZ
#define FUN_ESP32_XTAL_MHZ 40u
#endif

/* Zero means an unsupported/uncalibrated source or invalid metadata. */
static inline unsigned fun_esp32_clock_mhz(uint32_t clock, uint32_t period,
                                         uint32_t stored, uint32_t divider,
                                         unsigned fallback_xtal)
{
    unsigned source = (clock >> 27) & 3u;
    if (source == 1u) {
        unsigned select = period & 3u;
        return select <= 2u ? 80u * (select + 1u) : 0u;
    }
    if (source != 0u) return 0;
    unsigned xtal = fallback_xtal;
    if ((stored & 65535u) == (stored >> 16) && (stored & 0x7fffu))
        xtal = stored & 0x7fffu;
    if (xtal != 24u && xtal != 26u && xtal != 40u) return 0;
    return xtal / ((divider & 1023u) + 1u);
}

#ifdef ESP32FUN_ESP32
#include "esp32fun.h"
static inline int fun_esp32_clock_sync(unsigned fallback_xtal)
{
    unsigned mhz = fun_esp32_clock_mhz(FUN_REG(0x3ff48070u), FUN_REG(0x3ff0003cu),
                                      FUN_REG(0x3ff480b0u), FUN_REG(0x3ff66000u),
                                      fallback_xtal);
    if (!mhz) return -1;
    fun_rom_set_cpu_mhz(mhz);
    return 0;
}
#endif
#endif
