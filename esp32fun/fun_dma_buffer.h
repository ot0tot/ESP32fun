/* SPDX-License-Identifier: MIT
 * Original descriptor/buffer contracts shared by GDMA and classic UART UDMA.
 * Numeric descriptor and SRAM facts from pinned ESP-IDF v5.3.1 and our linker.
 */
#ifndef FUN_DMA_BUFFER_H
#define FUN_DMA_BUFFER_H
#include "esp32fun.h"
#if defined(ESP32FUN_ESP32)
#define FUN_DMA_RAM_START 0x3fff0000u
#define FUN_DMA_RAM_END 0x3fff6000u
#elif defined(ESP32FUN_ESP32C3)
#define FUN_DMA_RAM_START 0x3fcd5710u
#define FUN_DMA_RAM_END 0x3fcdc710u
#elif defined(ESP32FUN_ESP32S3)
#define FUN_DMA_RAM_START 0x3fce2700u
#define FUN_DMA_RAM_END 0x3fce9700u
#else
#error "DMA buffer contracts currently cover ESP32, C3 and S3 only"
#endif
typedef struct {
    volatile uint32_t control;
    uint32_t buffer;
    uint32_t next;
} fun_dma_desc;
_Static_assert(sizeof(fun_dma_desc) == 12, "DMA descriptor layout");
#define FUN_DMA_OWNER FUN_BIT(31)
#define FUN_DMA_EOF FUN_BIT(30)
#define FUN_DMA_MAX_BYTES 4092u
enum { FUN_DMA_INVALID = -1, FUN_DMA_TIMEOUT = -2, FUN_DMA_ERROR = -3 };

static inline void fun_dma_fence(void)
{
#if FUN_ARCH_XTENSA
    __asm__ volatile ("memw" ::: "memory");
#else
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
#endif
}

static inline int fun_dma_ram(const void *p, size_t size)
{
    /* Linker reserves this DRAM range, including the independent stack. */
    uintptr_t address = (uintptr_t)p;
    return size && !(address & 3u) && address >= FUN_DMA_RAM_START &&
        address < FUN_DMA_RAM_END && size <= FUN_DMA_RAM_END - address;
}

static inline int fun_dma_overlap(uint32_t a, unsigned a_size, uint32_t b, unsigned b_size)
{
    /* All addresses and lengths have already passed internal-DRAM bounds checks. */
    return a < b + b_size && b < a + a_size;
}

static inline int fun_dma_timeout_valid(unsigned timeout_us)
{
    unsigned mhz = fun_cpu_mhz();
    return mhz && timeout_us && timeout_us <= 0x7fffffffu / mhz;
}

/* One descriptor per segment, final EOF only. Run before giving list to DMA. */
static inline int fun_dma_descriptor(fun_dma_desc *d, void *buffer, unsigned bytes,
                                     fun_dma_desc *next, int receive)
{
    if (!fun_dma_ram(d, sizeof(*d)) || !fun_dma_ram(buffer, bytes) ||
        (bytes & 3u) || bytes > FUN_DMA_MAX_BYTES ||
        (next && !fun_dma_ram(next, sizeof(*next))) ||
        fun_dma_overlap((uint32_t)(uintptr_t)d, sizeof(*d),
                        (uint32_t)(uintptr_t)buffer, bytes)) return FUN_DMA_INVALID;
    d->buffer = (uint32_t)(uintptr_t)buffer;
    d->next = (uint32_t)(uintptr_t)next;
    d->control = FUN_DMA_OWNER | ((!receive && !next) ? FUN_DMA_EOF : 0u) |
        (receive ? 0u : bytes << 12) | bytes;
    return 0;
}


#endif
