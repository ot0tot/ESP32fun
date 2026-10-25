/* SPDX-License-Identifier: MIT
 * Original ESP32-S3 RMT pulse-duration capture via GDMA.
 * Hardware RX channel 7 is the S3's DMA-capable receiver (LL RX index 3).
 */
#ifndef FUN_RMT_DMA_H
#define FUN_RMT_DMA_H
#include "fun_dma.h"
#include "fun_rmt_dma_status.h"
#ifndef ESP32FUN_ESP32S3
#error "RMT DMA capture currently supports ESP32-S3 only"
#endif
#define FUN_RMT_REG(offset) FUN_REG(0x60016000u + (offset))
#define FUN_RMT_RX_CONF0 FUN_RMT_REG(0x48)
#define FUN_RMT_RX_CONF1 FUN_RMT_REG(0x4c)

/* One microsecond duration ticks, 20 ms idle end marker, no input glitch filter.
 * Owns the RMT group (shared clock) and GDMA pair 0. Input is a 3.3 V signal.
 */
static inline int fun_rmt_dma_init(unsigned input_pin)
{
    if (fun_pin_mode(input_pin, FUN_INPUT)) return FUN_DMA_INVALID;
    FUN_REG(0x600c0018u) |= FUN_BIT(9);
    FUN_REG(0x600c0020u) |= FUN_BIT(9);
    FUN_REG(0x600c0020u) &= ~FUN_BIT(9);
    /* Force RAM/register clocks, source XTAL40, group divider=1. */
    FUN_RMT_REG(0xc0) = FUN_BIT(31) | FUN_BIT(26) | (3u << 24) | FUN_BIT(3) | FUN_BIT(1) | 1u;
    FUN_RMT_RX_CONF0 = (1u << 24) | FUN_BIT(23) | (20000u << 8) | 40u;
    FUN_RMT_RX_CONF1 = FUN_BIT(13) | FUN_BIT(3) | FUN_BIT(15);
    FUN_RMT_REG(0xbc) = 48; /* one hardware RMT memory block */
    FUN_RMT_REG(0x78) = 0;
    FUN_RMT_REG(0x7c) = 0xffffffffu;
    FUN_REG(FUN_GPIO_BASE + 0x154u + 4u * 84u) = input_pin | FUN_BIT(7);
    fun_dma_reset();
    return 0;
}

/* Returns the number of 32-bit symbols. Each symbol contains two level/duration
 * pairs: duration0[14:0], level0[15], duration1[30:16], level1[31]. A zero
 * duration terminates the capture. Full buffers stop the receiver; this is a
 * finite snapshot, not a loss-free continuously streaming timer-capture ring.
 */
static inline int fun_rmt_dma_read(uint32_t *symbols, unsigned capacity, unsigned timeout_us)
{
    fun_dma_desc rx;
    unsigned mhz = fun_cpu_mhz();
    if (!capacity || capacity > FUN_DMA_MAX_BYTES / 4u || !mhz || !timeout_us ||
        timeout_us > 0x7fffffffu / mhz || fun_dma_descriptor(&rx, symbols, capacity * 4u, 0, 1))
        return FUN_DMA_INVALID;
    FUN_RMT_RX_CONF1 = FUN_BIT(13) | FUN_BIT(3) | FUN_BIT(1) | FUN_BIT(2) | FUN_BIT(14);
    FUN_RMT_RX_CONF1 = FUN_BIT(13) | FUN_BIT(3);
    FUN_RMT_REG(0x7c) = 0xffffffffu;
    fun_dma_reset();
    FUN_GDMA_IN_PERIPHERAL = 9; /* RMT */
    fun_dma_fence();
    FUN_GDMA_IN_LINK = ((uint32_t)(uintptr_t)&rx & 0xfffffu) | FUN_BIT(22);
    FUN_RMT_RX_CONF1 |= 1u | FUN_BIT(15);
    unsigned start = fun_cycle_count(), budget = timeout_us * mhz;
    int result = 0;
    for (;;) {
        unsigned dma = FUN_GDMA_REG(0x08); /* S3 RX raw flags */
        unsigned rmt = FUN_RMT_REG(0x70);
        fun_dma_fence(); /* observe descriptor writeback after interrupt status */
        int state = fun_rmt_dma_poll(dma, rmt, rx.control, capacity * 4u);
        if (state == FUN_RMT_POLL_ERROR) {
            result = FUN_DMA_ERROR; break;
        }
        if (state == FUN_RMT_POLL_READY) break;
        if ((unsigned)(fun_cycle_count() - start) >= budget) {
            result = (dma & FUN_BIT(4)) ? FUN_DMA_ERROR : FUN_DMA_TIMEOUT;
            break;
        }
    }
#ifdef FUN_RMT_DMA_DIAGNOSTIC
    unsigned last_dma = FUN_GDMA_REG(0x08), last_rmt = FUN_RMT_REG(0x70);
    unsigned last_channel = FUN_RMT_REG(0x6c);
#endif
    FUN_RMT_RX_CONF1 = (FUN_RMT_RX_CONF1 & ~1u) | FUN_BIT(15);
    fun_dma_reset();
    if (result) {
#ifdef FUN_RMT_DMA_DIAGNOSTIC
        fun_printf("RMT diagnostic result=%d dma=%08x rmt=%08x channel=%08x descriptor=%08x\n",
                   result, last_dma, last_rmt, last_channel, (unsigned)rx.control);
#endif
        return result;
    }
    unsigned bytes = (rx.control >> 12) & 4095u;
    if ((rx.control & FUN_DMA_OWNER) || !bytes || bytes > capacity * 4u || (bytes & 3u))
        return FUN_DMA_ERROR;
    return (int)(bytes / 4u);
}
#endif
