/* SPDX-License-Identifier: MIT
 * Original ESP32-C3/S3 register-level GDMA/SPI2 implementation.
 * Register facts checked against ESP-IDF v5.3.1 soc and hal for each chip.
 * No SDK, interrupt dispatcher, heap, or CPU payload-copy fallback.
 */
#ifndef FUN_DMA_H
#define FUN_DMA_H
#include "esp32fun.h"
#if !defined(ESP32FUN_ESP32C3) && !defined(ESP32FUN_ESP32S3)
#error "fun_dma.h currently supports ESP32-C3 and ESP32-S3 only"
#endif

/* This deliberately small driver exclusively owns GDMA channel pair 0 and SPI2.
 * Buffers and descriptors must be word-aligned internal DRAM, lengths multiples
 * of four. No PSRAM, flash, circular lists, concurrency, or cache maintenance.
 * Caller must not change descriptors/buffers until the synchronous call returns.
 */
#include "fun_dma_buffer.h"

#define FUN_GDMA_REG(offset) FUN_REG(0x6003f000u + (offset))
#ifdef ESP32FUN_ESP32C3
#define FUN_GDMA_RAW FUN_GDMA_REG(0x00)
#define FUN_GDMA_IN_CONF FUN_GDMA_REG(0x70)
#define FUN_GDMA_IN_OWNER_CHECK FUN_GDMA_REG(0x74)
#define FUN_GDMA_IN_LINK FUN_GDMA_REG(0x80)
#define FUN_GDMA_IN_PERIPHERAL FUN_GDMA_REG(0xa0)
#define FUN_GDMA_OUT_CONF FUN_GDMA_REG(0xd0)
#define FUN_GDMA_OUT_OWNER_CHECK FUN_GDMA_REG(0xd4)
#define FUN_GDMA_OUT_LINK FUN_GDMA_REG(0xe0)
#define FUN_GDMA_OUT_PERIPHERAL FUN_GDMA_REG(0x100)
#define FUN_DMA_BUS_CLOCK 0x600c0014u
#define FUN_DMA_BUS_RESET 0x600c001cu
#define FUN_DMA_MISC_CLOCK FUN_GDMA_REG(0x44)
#define FUN_DMA_MISC_ENABLE FUN_BIT(3)
#define FUN_DMA_SPI_CLOCK 0x600c0010u
#define FUN_DMA_SPI_RESET 0x600c0018u
#define FUN_DMA_SPI_CLOCK_SIGNAL 63u
#define FUN_DMA_SPI_MOSI_SIGNAL 65u
#define FUN_DMA_SPI_MISO_SIGNAL 64u
#define FUN_DMA_MATRIX_INPUT_ENABLE FUN_BIT(6)
#else
/* S3 has separate RX/TX raw interrupt registers. Normalize EOF/error flags to
 * the C3 layout used by the small common completion loop below.
 */
#define FUN_GDMA_RAW ((FUN_GDMA_REG(0x08) & 3u) | ((FUN_GDMA_REG(0x68) & 8u) << 5) | \
    (((FUN_GDMA_REG(0x08) & 0x3dcu) || (FUN_GDMA_REG(0x68) & 0xf4u)) ? FUN_BIT(5) : 0u))
#define FUN_GDMA_IN_CONF FUN_GDMA_REG(0x00)
#define FUN_GDMA_IN_OWNER_CHECK FUN_GDMA_REG(0x04)
#define FUN_GDMA_IN_LINK FUN_GDMA_REG(0x20)
#define FUN_GDMA_IN_PERIPHERAL FUN_GDMA_REG(0x48)
#define FUN_GDMA_OUT_CONF FUN_GDMA_REG(0x60)
#define FUN_GDMA_OUT_OWNER_CHECK FUN_GDMA_REG(0x64)
#define FUN_GDMA_OUT_LINK FUN_GDMA_REG(0x80)
#define FUN_GDMA_OUT_PERIPHERAL FUN_GDMA_REG(0xa8)
#define FUN_DMA_BUS_CLOCK 0x600c001cu
#define FUN_DMA_BUS_RESET 0x600c0024u
#define FUN_DMA_MISC_CLOCK FUN_GDMA_REG(0x3c8)
#define FUN_DMA_MISC_ENABLE FUN_BIT(4)
#define FUN_DMA_SPI_CLOCK 0x600c0018u
#define FUN_DMA_SPI_RESET 0x600c0020u
#define FUN_DMA_SPI_CLOCK_SIGNAL 101u
#define FUN_DMA_SPI_MOSI_SIGNAL 103u
#define FUN_DMA_SPI_MISO_SIGNAL 102u
#define FUN_DMA_MATRIX_INPUT_ENABLE FUN_BIT(7)
#endif
#define FUN_GDMA_RX_EOF FUN_BIT(1)
#define FUN_GDMA_TX_EOF FUN_BIT(8)
#define FUN_GDMA_ERRORS (FUN_BIT(2) | FUN_BIT(5) | FUN_BIT(6) | FUN_BIT(7) | (15u << 9))
#define FUN_SPI2_REG(offset) FUN_REG(0x60024000u + (offset))
#define FUN_SPI2_COMMAND FUN_SPI2_REG(0x00)
#define FUN_SPI2_USER FUN_SPI2_REG(0x10)
#define FUN_SPI2_DMA_CONF FUN_SPI2_REG(0x30)
#define FUN_SPI2_INT_CLEAR FUN_SPI2_REG(0x38)
#define FUN_SPI2_INT_RAW FUN_SPI2_REG(0x3c)
#define FUN_SPI2_UPDATE FUN_BIT(23)
#define FUN_SPI2_START FUN_BIT(24)
#define FUN_SPI2_DONE FUN_BIT(12)

/* Validate a finite list before touching hardware; reject loops and bad owners. */
static inline int fun_dma_validate(fun_dma_desc *head, unsigned count, int receive)
{
    if (!count || count > 64u) return FUN_DMA_INVALID;
    fun_dma_desc *first = head;
    unsigned total = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (!fun_dma_ram(head, sizeof(*head))) return FUN_DMA_INVALID;
        uint32_t word = head->control;
        unsigned bytes = word & 4095u;
        if (!(word & FUN_DMA_OWNER) || !fun_dma_ram((void *)(uintptr_t)head->buffer, bytes) ||
            (bytes & 3u) || bytes > FUN_DMA_MAX_BYTES ||
            ((word >> 12) & 4095u) != (receive ? 0u : bytes) ||
            !!(word & FUN_DMA_EOF) != (!receive && i + 1u == count) ||
            (i + 1u == count ? head->next != 0 : head->next == 0)) return FUN_DMA_INVALID;
        total += bytes;
        head = (fun_dma_desc *)(uintptr_t)head->next;
    }
    /* A DMA write into its own descriptor list can turn a bounded transfer into
     * an arbitrary bus write. Check every descriptor against every payload.
     */
    for (fun_dma_desc *a = first; a; a = (fun_dma_desc *)(uintptr_t)a->next)
        for (fun_dma_desc *b = first; b; b = (fun_dma_desc *)(uintptr_t)b->next) {
            if (fun_dma_overlap((uint32_t)(uintptr_t)a, sizeof(*a), b->buffer, b->control & 4095u) ||
                (a != b && fun_dma_overlap((uint32_t)(uintptr_t)a, sizeof(*a),
                                          (uint32_t)(uintptr_t)b, sizeof(*b))) ||
                (receive && a != b && fun_dma_overlap(a->buffer, a->control & 4095u,
                                                       b->buffer, b->control & 4095u)))
                return FUN_DMA_INVALID;
        }
    return (int)total;
}

/* Call only on two separately validated finite lists. No in-place transfers. */
static inline int fun_dma_lists_disjoint(fun_dma_desc *tx, fun_dma_desc *rx)
{
    for (fun_dma_desc *a = tx; a; a = (fun_dma_desc *)(uintptr_t)a->next)
        for (fun_dma_desc *b = rx; b; b = (fun_dma_desc *)(uintptr_t)b->next)
            if (fun_dma_overlap(a->buffer, a->control & 4095u, b->buffer, b->control & 4095u) ||
                fun_dma_overlap((uint32_t)(uintptr_t)a, sizeof(*a), b->buffer, b->control & 4095u) ||
                fun_dma_overlap((uint32_t)(uintptr_t)b, sizeof(*b), a->buffer, a->control & 4095u) ||
                fun_dma_overlap((uint32_t)(uintptr_t)a, sizeof(*a),
                                (uint32_t)(uintptr_t)b, sizeof(*b))) return 0;
    return 1;
}

static inline void fun_dma_reset(void)
{
    FUN_REG(FUN_DMA_BUS_CLOCK) |= FUN_BIT(6); /* SYSTEM DMA bus clock */
    FUN_REG(FUN_DMA_BUS_RESET) &= ~FUN_BIT(6); /* GDMA starts held in reset */
    FUN_DMA_MISC_CLOCK |= FUN_DMA_MISC_ENABLE;
    FUN_GDMA_IN_LINK = FUN_BIT(21); /* stop RX */
    FUN_GDMA_OUT_LINK = FUN_BIT(20); /* stop TX */
    FUN_GDMA_IN_CONF = 1; FUN_GDMA_IN_CONF = 0;
    FUN_GDMA_OUT_CONF = 1; FUN_GDMA_OUT_CONF = FUN_BIT(2) | FUN_BIT(3);
    FUN_GDMA_IN_OWNER_CHECK |= FUN_BIT(12);
    FUN_GDMA_OUT_OWNER_CHECK |= FUN_BIT(12);
    FUN_GDMA_IN_PERIPHERAL = 63;
    FUN_GDMA_OUT_PERIPHERAL = 63;
#ifdef ESP32FUN_ESP32C3
    FUN_GDMA_REG(0x08) = 0; /* poll raw flags; do not enable CPU interrupts */
    FUN_GDMA_REG(0x0c) = 0x1fff;
#else
    FUN_GDMA_REG(0x10) = 0; FUN_GDMA_REG(0x70) = 0;
    FUN_GDMA_REG(0x14) = 0x3ff; FUN_GDMA_REG(0x74) = 0xff;
#endif
    fun_dma_fence();
}

static inline void fun_dma_start(fun_dma_desc *tx, fun_dma_desc *rx, int memory)
{
    fun_dma_reset();
    FUN_GDMA_IN_CONF = memory ? FUN_BIT(4) : 0;
    FUN_GDMA_IN_PERIPHERAL = rx ? 0 : 63; /* SPI2 or valid M2M selection */
    FUN_GDMA_OUT_PERIPHERAL = tx ? 0 : 63;
    fun_dma_fence(); /* publish descriptor and payload writes before bus master */
    if (rx) FUN_GDMA_IN_LINK = ((uint32_t)(uintptr_t)rx & 0xfffffu) | FUN_BIT(22);
    if (tx) FUN_GDMA_OUT_LINK = ((uint32_t)(uintptr_t)tx & 0xfffffu) | FUN_BIT(21);
}

static inline int fun_dma_wait(unsigned wanted, unsigned timeout_us, int spi)
{
    unsigned start = fun_cycle_count();
    unsigned mhz = fun_cpu_mhz();
    if (!mhz || !timeout_us || timeout_us > 0x7fffffffu / mhz) return FUN_DMA_INVALID;
    unsigned budget = timeout_us * mhz;
    for (;;) {
        unsigned status = FUN_GDMA_RAW;
        if ((status & FUN_GDMA_ERRORS) || (spi && (FUN_SPI2_INT_RAW & 3u))) return FUN_DMA_ERROR;
        if ((status & wanted) == wanted && (!spi || (FUN_SPI2_INT_RAW & FUN_SPI2_DONE))) {
            fun_dma_fence();
            return 0;
        }
        if ((unsigned)(fun_cycle_count() - start) >= budget) return FUN_DMA_TIMEOUT;
    }
}

static inline int fun_dma_copy(fun_dma_desc *tx, unsigned tx_count,
                                fun_dma_desc *rx, unsigned rx_count, unsigned timeout_us)
{
    int bytes = fun_dma_validate(tx, tx_count, 0);
    if (bytes < 0 || !fun_dma_timeout_valid(timeout_us) ||
        fun_dma_validate(rx, rx_count, 1) != bytes || !fun_dma_lists_disjoint(tx, rx))
        return FUN_DMA_INVALID;
    /* GDMA is a streaming copy, so reject any overlapping source/destination
     * segments. There is no safe CPU-copy fallback or memmove semantics.
     */
    fun_dma_start(tx, rx, 1);
    int result = fun_dma_wait(FUN_GDMA_RX_EOF | FUN_GDMA_TX_EOF, timeout_us, 0);
    fun_dma_reset(); /* also quiesce hardware before returning on timeout */
    return result;
}

/* Mode 0, MSB first. 40 MHz XTAL / divider; divider must be even in [2,64].
 * Matrix routing uses software CS; SPI hardware CS outputs stay disconnected.
 */
static inline int fun_spi_dma_init(unsigned clock, unsigned mosi, unsigned miso,
                                   unsigned select, unsigned divider)
{
    if (divider < 2 || divider > 64 || (divider & 1u) || clock == mosi ||
        clock == miso || clock == select || mosi == miso || mosi == select || miso == select)
        return FUN_DMA_INVALID;
    if (fun_pin_mode(clock, FUN_OUTPUT) || fun_pin_mode(mosi, FUN_OUTPUT) ||
        fun_pin_mode(miso, FUN_INPUT) || fun_pin_mode(select, FUN_OUTPUT)) return FUN_DMA_INVALID;
    fun_digital_write(select, 1);
    FUN_GPIO_OUTPUT_SELECT(clock) = FUN_DMA_SPI_CLOCK_SIGNAL | FUN_GPIO_ENABLE_FROM_REG;
    FUN_GPIO_OUTPUT_SELECT(mosi) = FUN_DMA_SPI_MOSI_SIGNAL | FUN_GPIO_ENABLE_FROM_REG;
    FUN_REG(FUN_GPIO_BASE + 0x154u + 4u * FUN_DMA_SPI_MISO_SIGNAL) = miso | FUN_DMA_MATRIX_INPUT_ENABLE;
    FUN_REG(FUN_DMA_SPI_CLOCK) |= FUN_BIT(6); /* SPI2 bus clock */
    FUN_REG(FUN_DMA_SPI_RESET) |= FUN_BIT(6);
    FUN_REG(FUN_DMA_SPI_RESET) &= ~FUN_BIT(6);
    FUN_SPI2_REG(0xe8) = 3; /* enable register/master clocks, source XTAL */
    FUN_SPI2_REG(0x08) = 0; /* single line, MSB first */
    FUN_SPI2_REG(0x0c) = ((divider - 1u) << 12) | ((divider / 2u - 1u) << 6) | (divider - 1u);
    FUN_SPI2_USER = 0;
    FUN_SPI2_REG(0x14) = 0;
    FUN_SPI2_REG(0x20) = 7; /* disable three hardware CS outputs; clock idle low */
    FUN_SPI2_REG(0x24) = 0; FUN_SPI2_REG(0x28) = 0; FUN_SPI2_REG(0x2c) = 0;
    FUN_SPI2_REG(0xe0) = 0; /* master */
    FUN_SPI2_REG(0x34) = 0; /* no interrupts */
    FUN_SPI2_DMA_CONF = 0;
    fun_dma_reset();
    return 0;
}

static inline int fun_spi_dma_transfer(fun_dma_desc *tx, unsigned tx_count,
                                       fun_dma_desc *rx, unsigned rx_count,
                                       unsigned select, unsigned timeout_us)
{
    int bytes = fun_dma_validate(tx, tx_count, 0);
    if (bytes < 0 || bytes > 32768 || !fun_pin_valid(select) || !fun_dma_timeout_valid(timeout_us) ||
        (rx ? fun_dma_validate(rx, rx_count, 1) != bytes || !fun_dma_lists_disjoint(tx, rx) : rx_count != 0))
        return FUN_DMA_INVALID;
    FUN_SPI2_INT_CLEAR = 0x7fff;
    unsigned conf = FUN_BIT(28) | (rx ? FUN_BIT(27) | FUN_BIT(21) : 0u);
    FUN_SPI2_DMA_CONF = conf | FUN_BIT(31) | FUN_BIT(29); /* TX/RX FIFO reset */
    FUN_SPI2_DMA_CONF = conf;
    FUN_SPI2_USER = FUN_BIT(27) | (rx ? FUN_BIT(28) | 1u : 0u);
    FUN_SPI2_REG(0x1c) = (unsigned)bytes * 8u - 1u;
    /* Arm the DMA stream before synchronizing SPI's clock domain. */
    fun_dma_start(tx, rx, 0);
    FUN_SPI2_COMMAND = FUN_SPI2_UPDATE;
    /* Clock-domain synchronization has its own bounded timeout. */
    unsigned start = fun_cycle_count(), budget = fun_cpu_mhz() * 1000u;
    while (FUN_SPI2_COMMAND & FUN_SPI2_UPDATE) {
        if ((unsigned)(fun_cycle_count() - start) >= budget) {
            fun_dma_reset();
            return FUN_DMA_TIMEOUT;
        }
    }
    fun_digital_write(select, 0);
    FUN_SPI2_COMMAND = FUN_SPI2_START;
    int result = fun_dma_wait(FUN_GDMA_TX_EOF | (rx ? FUN_GDMA_RX_EOF : 0u), timeout_us, 1);
    /* DMA EOF alone is too early to release CS: also wait for SPI TRANS_DONE. */
    if (result) {
        FUN_REG(FUN_DMA_SPI_RESET) |= FUN_BIT(6); /* abort SPI before releasing buffers */
        FUN_REG(FUN_DMA_SPI_RESET) &= ~FUN_BIT(6);
        /* Reinitialize SPI with fun_spi_dma_init() after any transfer error. */
    }
    fun_dma_reset();
    fun_digital_write(select, 1);
    return result;
}
#endif
