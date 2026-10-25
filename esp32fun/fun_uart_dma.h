/* SPDX-License-Identifier: MIT
 * Original ESP32 UART1/UHCI0 register implementations: C3 GDMA and classic UDMA.
 * Numeric facts from ESP-IDF v5.3.1, including UART core synchronization and
 * UHCI's raw (no header, escaping, separator, or CRC) UART transport mode.
 */
#ifndef FUN_UART_DMA_H
#define FUN_UART_DMA_H
#ifdef ESP32FUN_ESP32
#include "fun_uart_udma_esp32.h"
#else
#include "fun_dma.h"
#ifndef ESP32FUN_ESP32C3
#error "UART/UHCI DMA currently supports ESP32-C3 and classic ESP32 only"
#endif
#define FUN_UART1_REG(offset) FUN_REG(0x60010000u + (offset))
#define FUN_UHCI_REG(offset) FUN_REG(0x60014000u + (offset))
/* Raw byte transport has no BREAK framing: reject its synthetic zero byte
 * together with parity, framing and FIFO-overflow failures. */
#include "fun_uart_errors.h"
#define FUN_UART_DMA_ENGINE_ERRORS FUN_GDMA_ERRORS
static inline unsigned fun_uart_dma_status(void) { return FUN_GDMA_RAW; }
static inline void fun_uart_dma_stop(void) { fun_dma_reset(); }
static inline void fun_uart_dma_start_receive(fun_dma_desc *head)
{
    FUN_GDMA_IN_PERIPHERAL = 2;
    fun_dma_fence();
    FUN_GDMA_IN_LINK = ((uint32_t)(uintptr_t)head & 0xfffffu) | FUN_BIT(22);
}

static inline int fun_uart_dma_sync(void)
{
    FUN_UART1_REG(0x80) = (FUN_UART1_REG(0x80) & ~FUN_BIT(30)) | FUN_BIT(31);
    unsigned start = fun_cycle_count(), budget = fun_cpu_mhz() * 1000u;
    while (FUN_UART1_REG(0x80) & FUN_BIT(31))
        if ((unsigned)(fun_cycle_count() - start) >= budget) return FUN_DMA_TIMEOUT;
    return 0;
}

/* UART1 leaves the ROM console UART0 untouched. 40 MHz XTAL, 8N1, no flow control.
 * Driver exclusively owns UART1, UHCI0 and GDMA pair 0. UART RX idle threshold
 * is 40 baud clocks (four 8N1 character times), delimiting incoming packets.
 */
static inline int fun_uart_dma_init(unsigned tx_pin, unsigned rx_pin, unsigned baud)
{
    if (tx_pin == rx_pin || baud < 10000 || baud > 2000000 ||
        fun_pin_mode(tx_pin, FUN_OUTPUT) || fun_pin_mode(rx_pin, FUN_INPUT)) return FUN_DMA_INVALID;
    fun_digital_write(tx_pin, 1);
    FUN_REG(0x600c0010u) |= FUN_BIT(5) | FUN_BIT(8);
    FUN_REG(0x600c0018u) |= FUN_BIT(5) | FUN_BIT(8);
    FUN_REG(0x600c0018u) &= ~(FUN_BIT(5) | FUN_BIT(8));
    FUN_UART1_REG(0x78) = FUN_BIT(25) | FUN_BIT(24) | FUN_BIT(22) | (3u << 20);
    unsigned divisor = (40000000u * 16u + baud / 2u) / baud;
    FUN_UART1_REG(0x14) = (divisor >> 4) | ((divisor & 15u) << 20);
    FUN_UART1_REG(0x20) = FUN_BIT(25) | (3u << 2) | (1u << 4);
    FUN_UART1_REG(0x24) = 0;
    FUN_UART1_REG(0x48) = 40; /* RX_IDLE_THRHD, NOT FIFO interrupt timeout */
    FUN_UART1_REG(0x0c) = 0; /* interrupts disabled */
    FUN_UART1_REG(0x10) = 0x7ffff;
    if (fun_uart_dma_sync()) return FUN_DMA_TIMEOUT;
    FUN_GPIO_OUTPUT_SELECT(tx_pin) = 9 | FUN_GPIO_ENABLE_FROM_REG;
    FUN_REG(FUN_GPIO_BASE + 0x154u + 4u * 9u) = rx_pin | FUN_BIT(6);
    FUN_UHCI_REG(0x00) = FUN_BIT(11) | FUN_BIT(8) | FUN_BIT(3);
    FUN_UHCI_REG(0x14) = 0;
    FUN_UHCI_REG(0x20) = 0; /* disable all escape sequences */
    FUN_UHCI_REG(0x0c) = 0;
    FUN_UHCI_REG(0x10) = 0x1ff;
    fun_dma_reset();
    return 0;
}

static inline int fun_uart_dma_send(void *buffer, unsigned bytes, unsigned timeout_us)
{
    unsigned capacity = (bytes + 3u) & ~3u;
    if (!bytes || bytes > FUN_DMA_MAX_BYTES || !fun_dma_ram(buffer, capacity) ||
        !fun_dma_timeout_valid(timeout_us)) return FUN_DMA_INVALID;
    fun_dma_desc tx = {FUN_DMA_OWNER | FUN_DMA_EOF | (bytes << 12) | capacity,
                       (uint32_t)(uintptr_t)buffer, 0};
    fun_dma_reset();
    FUN_GDMA_OUT_PERIPHERAL = 2; /* UHCI0 */
    fun_dma_fence();
    FUN_GDMA_OUT_LINK = ((uint32_t)(uintptr_t)&tx & 0xfffffu) | FUN_BIT(21);
    int result = fun_dma_wait(FUN_GDMA_TX_EOF, timeout_us, 0);
    if (!result) {
        unsigned start = fun_cycle_count(), mhz = fun_cpu_mhz();
        unsigned budget = timeout_us * mhz; /* already checked by fun_dma_wait */
        while ((FUN_UART1_REG(0x1c) & (0x3ffu << 16)) || (FUN_UART1_REG(0x6c) & (15u << 4))) {
            if ((unsigned)(fun_cycle_count() - start) >= budget) { result = FUN_DMA_TIMEOUT; break; }
        }
    }
    fun_dma_reset();
    if (result) {
        /* Abort pending FIFO contents before caller can modify the buffer. */
        FUN_UHCI_REG(0x00) |= FUN_BIT(0);
        FUN_UHCI_REG(0x00) &= ~FUN_BIT(0);
        FUN_UART1_REG(0x20) |= FUN_BIT(18);
        (void)fun_uart_dma_sync();
        FUN_UART1_REG(0x20) &= ~FUN_BIT(18);
        (void)fun_uart_dma_sync();
    }
    return result;
}

/* Finite packet receive. CPU polls completion; UHCI/GDMA writes every byte.
 * Returns received length, or a negative error. Idle delimiter can end early.
 * Bytes arriving between calls may be lost: use the ring demo for continuous RX.
 */
static inline int fun_uart_dma_receive(void *buffer, unsigned capacity, unsigned timeout_us)
{
    fun_dma_desc rx;
    if (!fun_dma_timeout_valid(timeout_us) || fun_dma_descriptor(&rx, buffer, capacity, 0, 1))
        return FUN_DMA_INVALID;
    fun_dma_reset();
    FUN_UART1_REG(0x10) = 0x7ffff;
    FUN_UHCI_REG(0x10) = 0x1ff;
    FUN_GDMA_IN_PERIPHERAL = 2;
    fun_dma_fence();
    FUN_GDMA_IN_LINK = ((uint32_t)(uintptr_t)&rx & 0xfffffu) | FUN_BIT(22);
    int result = fun_dma_wait(FUN_GDMA_RX_EOF, timeout_us, 0);
    if (FUN_UART1_REG(0x04) & FUN_UART_DMA_ERRORS) result = FUN_DMA_ERROR;
    fun_dma_reset();
    unsigned bytes = (rx.control >> 12) & 4095u;
    if (!result && ((rx.control & FUN_DMA_OWNER) || !bytes || bytes > capacity)) result = FUN_DMA_ERROR;
    if (result) {
        /* Discard an incomplete idle-delimited packet after an error/timeout.
         * Reset both UHCI RX and UART FIFO before another receive can be armed.
         */
        FUN_UHCI_REG(0x00) |= FUN_BIT(1);
        FUN_UHCI_REG(0x00) &= ~FUN_BIT(1);
        FUN_UART1_REG(0x20) |= FUN_BIT(17);
        int reset_result = fun_uart_dma_sync();
        FUN_UART1_REG(0x20) &= ~FUN_BIT(17);
        if (fun_uart_dma_sync() || reset_result) return FUN_DMA_ERROR;
        return result;
    }
    return (int)bytes;
}
#endif /* C3 implementation */
#endif
