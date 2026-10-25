/* SPDX-License-Identifier: MIT
 * Original classic ESP32 UART1/UHCI0 integrated UDMA implementation.
 * Numeric facts from pinned ESP-IDF v5.3.1 SoC headers and ESP32 TRM.
 * No SDK dependency or CPU copy in place of DMA. UART0 console is preserved.
 */
#ifndef FUN_UART_UDMA_ESP32_H
#define FUN_UART_UDMA_ESP32_H
#include "fun_dma_buffer.h"
#include "fun_clock_esp32.h"
#define FUN_UART1_REG(offset) FUN_REG(0x3ff50000u + (offset))
#define FUN_UHCI_REG(offset) FUN_REG(0x3ff54000u + (offset))
#include "fun_uart_errors.h"
#define FUN_UART_DMA_ENGINE_ERRORS (FUN_BIT(6) | FUN_BIT(9) | FUN_BIT(10) | FUN_BIT(11) | FUN_BIT(12))
#define FUN_UART_UDMA_RX_EOF FUN_BIT(5)
#define FUN_UART_UDMA_TX_EOF FUN_BIT(13)
#ifndef FUN_UART_ESP32_XTAL_MHZ
#define FUN_UART_ESP32_XTAL_MHZ FUN_ESP32_XTAL_MHZ
#endif

static inline int fun_uart_udma_clock_sync(void)
{
    /* Also reconcile after an application changes its supported clock mode. */
    return fun_esp32_clock_sync(FUN_UART_ESP32_XTAL_MHZ) ? FUN_DMA_INVALID : 0;
}

static inline unsigned fun_uart_dma_status(void) { return FUN_UHCI_REG(0x04); }

/* Stop and reset the integrated DMA bus/FIFOs without resetting UART1. */
static inline void fun_uart_dma_stop(void)
{
    FUN_UHCI_REG(0x24) = FUN_BIT(28);
    FUN_UHCI_REG(0x28) = FUN_BIT(28);
    unsigned conf = FUN_UHCI_REG(0x00) & ~15u;
    FUN_UHCI_REG(0x00) = conf | 15u;
    FUN_UHCI_REG(0x00) = conf;
    FUN_UHCI_REG(0x0c) = 0;
    FUN_UHCI_REG(0x10) = 0x1ffff;
    fun_dma_fence();
}

static inline void fun_uart_dma_start_receive(fun_dma_desc *head)
{
    fun_dma_fence();
    FUN_UHCI_REG(0x28) = ((uint32_t)(uintptr_t)head & 0xfffffu) | FUN_BIT(29);
}

/* Classic RX FIFO reset has a documented silicon erratum. Drain its APB
 * FIFO using both count and memory pointers, with a bounded deadline. */
static inline int fun_uart_dma_discard_rx(void)
{
    unsigned start = fun_cycle_count(), budget = fun_cpu_mhz() * 1000u;
    for (;;) {
        unsigned count = FUN_UART1_REG(0x1c) & 255u, mem = FUN_UART1_REG(0x60);
        if (!count && ((mem >> 2) & 2047u) == ((mem >> 13) & 2047u)) break;
        (void)FUN_UART1_REG(0x00);
        if ((unsigned)(fun_cycle_count()-start) >= budget) return FUN_DMA_ERROR;
    }
    FUN_UART1_REG(0x10) = 0x7ffff;
    return 0;
}

static inline int fun_uart_dma_init(unsigned tx_pin, unsigned rx_pin, unsigned baud)
{
    /* REF_TICK is independent of inherited APB/PLL state. Limit this small
     * backend to <=1 MHz rather than claiming unsupported REF clock divisors. */
    if (tx_pin == rx_pin || baud < 40000 || baud > 1000000 ||
        fun_uart_udma_clock_sync() ||
        fun_pin_mode(tx_pin,FUN_OUTPUT) || fun_pin_mode(rx_pin,FUN_INPUT)) return FUN_DMA_INVALID;
    fun_digital_write(tx_pin,1);
    FUN_REG(0x3ff000c0u) |= FUN_BIT(5) | FUN_BIT(8);
    FUN_REG(0x3ff000c4u) |= FUN_BIT(5) | FUN_BIT(8);
    FUN_REG(0x3ff000c4u) &= ~(FUN_BIT(5) | FUN_BIT(8));
    unsigned divisor = (1000000u*16u + baud/2u)/baud;
    FUN_UART1_REG(0x14) = (divisor >> 4) | ((divisor & 15u) << 20);
    FUN_UART1_REG(0x20) = FUN_BIT(25) | (3u << 2) | (1u << 4);
    FUN_UART1_REG(0x24) = 0;
    /* Classic idle counter counts source-clock ticks, not C3 baud clocks. */
    FUN_UART1_REG(0x40) = (40000000u + baud-1u)/baud;
    FUN_UART1_REG(0x0c) = 0;
    FUN_UART1_REG(0x10) = 0x7ffff;
    FUN_GPIO_OUTPUT_SELECT(tx_pin) = 17u | FUN_GPIO_ENABLE_FROM_REG;
    FUN_REG(FUN_GPIO_BASE + 0x130u + 4u*17u) = rx_pin | FUN_BIT(7);
    /* Raw transport: no header, CRC, separator or escaping. EOF follows idle.
     * TX EOF is reported after FIFO consumption; descriptor owners checked. */
    FUN_UHCI_REG(0x00) = FUN_BIT(22) | FUN_BIT(19) | FUN_BIT(10) | FUN_BIT(8) | FUN_BIT(6);
    FUN_UHCI_REG(0x2c) = FUN_BIT(6) | FUN_BIT(2);
    FUN_UHCI_REG(0x64) = 0;
    fun_uart_dma_stop();
    return fun_uart_dma_discard_rx();
}

static inline int fun_uart_udma_wait(unsigned wanted, unsigned timeout_us)
{
    unsigned start = fun_cycle_count(), budget = fun_cpu_mhz()*timeout_us;
    for (;;) {
        unsigned raw = fun_uart_dma_status();
        if (raw & FUN_UART_DMA_ENGINE_ERRORS) return FUN_DMA_ERROR;
        if (raw & wanted) { fun_dma_fence(); return 0; }
        if ((unsigned)(fun_cycle_count()-start) >= budget) return FUN_DMA_TIMEOUT;
    }
}

static inline int fun_uart_dma_send(void *buffer, unsigned bytes, unsigned timeout_us)
{
    unsigned capacity = (bytes+3u)&~3u;
    if (!bytes || bytes > FUN_DMA_MAX_BYTES || !fun_dma_ram(buffer,capacity) ||
        !fun_dma_timeout_valid(timeout_us)) return FUN_DMA_INVALID;
    fun_dma_desc tx = {FUN_DMA_OWNER | FUN_DMA_EOF | (bytes << 12) | capacity,
                       (uint32_t)(uintptr_t)buffer,0};
    fun_uart_dma_stop(); fun_dma_fence();
    FUN_UHCI_REG(0x24) = ((uint32_t)(uintptr_t)&tx & 0xfffffu) | FUN_BIT(29);
    int result = fun_uart_udma_wait(FUN_UART_UDMA_TX_EOF,timeout_us);
    unsigned start = fun_cycle_count(), budget = fun_cpu_mhz()*timeout_us;
    if (!result) while (FUN_UART1_REG(0x1c) & ((255u << 16) | (15u << 24))) {
        if ((unsigned)(fun_cycle_count()-start) >= budget) { result = FUN_DMA_TIMEOUT; break; }
    }
    fun_uart_dma_stop();
    if (result) {
        /* UART1's FIFO reset also resets UART2 on classic ESP32. This backend
         * therefore reserves UART2 during TX abort; UART0 is never reset. */
        FUN_UART1_REG(0x20) |= FUN_BIT(18);
        FUN_UART1_REG(0x20) &= ~FUN_BIT(18);
    }
    return result;
}

/* capacity includes the four spare bytes required by classic UDMA at RX EOF.
 * The maximum packet is capacity-4; callers must allocate the whole capacity.
 * Near/full final descriptors can fail to return ownership and are not valid
 * packet buffers. A cyclic list must satisfy the same final-descriptor rule. */
static inline int fun_uart_dma_receive(void *buffer, unsigned capacity, unsigned timeout_us)
{
    fun_dma_desc rx;
    if (capacity < 8 || !fun_dma_timeout_valid(timeout_us) ||
        fun_dma_descriptor(&rx,buffer,capacity,0,1)) return FUN_DMA_INVALID;
    fun_uart_dma_stop();
    FUN_UART1_REG(0x10) = 0x7ffff;
    fun_uart_dma_start_receive(&rx);
    int result = fun_uart_udma_wait(FUN_UART_UDMA_RX_EOF,timeout_us);
    if (!result) {
        unsigned start = fun_cycle_count(), budget = fun_cpu_mhz()*timeout_us;
        while (rx.control & FUN_DMA_OWNER) {
            if ((fun_uart_dma_status() & FUN_UART_DMA_ENGINE_ERRORS) ||
                (FUN_UART1_REG(0x04) & FUN_UART_DMA_ERRORS)) { result = FUN_DMA_ERROR; break; }
            if ((unsigned)(fun_cycle_count()-start) >= budget) { result = FUN_DMA_TIMEOUT; break; }
        }
        fun_dma_fence();
    }
    if (FUN_UART1_REG(0x04) & FUN_UART_DMA_ERRORS) result = FUN_DMA_ERROR;
    fun_uart_dma_stop();
    unsigned bytes = (rx.control >> 12) & 4095u;
    if (!result && ((rx.control & FUN_DMA_OWNER) || !bytes || bytes > capacity-4u)) result = FUN_DMA_ERROR;
    if (result) return fun_uart_dma_discard_rx() ? FUN_DMA_ERROR : result;
    return (int)bytes;
}
#endif
