/* Original finite S3 RMT/GDMA completion policy; no MMIO or SDK dependency.
 * Numeric flags: ESP-IDF v5.3.1 esp32s3 gdma_reg.h and rmt_reg.h.
 */
#ifndef FUN_RMT_DMA_STATUS_H
#define FUN_RMT_DMA_STATUS_H
#include <stdint.h>

enum { FUN_RMT_POLL_ERROR = -1, FUN_RMT_POLL_WAIT = 0, FUN_RMT_POLL_READY = 1 };

static inline int fun_rmt_dma_poll(uint32_t dma, uint32_t rmt,
                                   uint32_t descriptor, unsigned capacity_bytes)
{
    /* FIFO/descriptor/data errors remain fatal even when DONE is also set. */
    if ((dma & 0x3ccu) || (rmt & ((1u << 23) | (1u << 29))))
        return FUN_RMT_POLL_ERROR;
    /* EMPTY can precede DONE/writeback at a terminal full descriptor. */
    if (!(dma & 3u) || (descriptor & (1u << 31))) return FUN_RMT_POLL_WAIT;
    unsigned bytes = (descriptor >> 12) & 4095u;
    if (!bytes || bytes > capacity_bytes || (bytes & 3u) ||
        ((dma & (1u << 4)) && bytes != capacity_bytes)) return FUN_RMT_POLL_ERROR;
    return FUN_RMT_POLL_READY;
}
#endif
