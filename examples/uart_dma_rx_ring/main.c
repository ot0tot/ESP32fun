/* SPDX-License-Identifier: MIT
 * Experimental UHCI/GDMA finite-owner ring with explicit overrun detection.
 */
#include "fun_uart_dma.h"
#define BLOCKS 4u
#ifdef ESP32FUN_ESP32
/* 128-byte idle packets need four spare bytes on classic UDMA. The 256-byte
 * test splits into 132 + 124 bytes, leaving room in its final descriptor. */
#define BYTES 132u
#else
#define BYTES 128u
#endif
static uint8_t buffers[BLOCKS][BYTES] __attribute__((aligned(4)));
static fun_dma_desc descriptors[BLOCKS];
int main(void)
{
    if (fun_uart_dma_init(4, 5, 115200)) return 1;
    for (unsigned i = 0; i < BLOCKS; ++i)
        if (fun_dma_descriptor(&descriptors[i], buffers[i], BYTES,
                               &descriptors[(i+1) % BLOCKS], 1)) return 2;
    fun_puts("UART1 circular DMA: RX GPIO5, 115200 8N1; short idle-delimited packets");
    fun_uart_dma_start_receive(descriptors);
    for (unsigned index = 0;; index = (index + 1u) % BLOCKS) {
        fun_dma_desc *d = &descriptors[index];
        for (;;) {
            if ((fun_uart_dma_status() & FUN_UART_DMA_ENGINE_ERRORS) || (FUN_UART1_REG(0x04) & FUN_UART_DMA_ERRORS)) {
                fun_uart_dma_stop();
                fun_puts("DMA ring stopped: overrun, descriptor or UART error");
                return 3;
            }
            if (!(d->control & FUN_DMA_OWNER)) break;
        }
        fun_dma_fence();
        unsigned bytes = (d->control >> 12) & 4095u;
        if (!bytes || bytes > BYTES) {
            fun_printf("DMA invalid block %u control=%08x raw=%08x\n",index,
                (unsigned)d->control,(unsigned)fun_uart_dma_status());
            fun_uart_dma_stop(); return 4;
        }
        /* Consume CPU-owned data before publishing the buffer back to DMA. */
        fun_printf("DMA ring block %u, %u bytes, CRC32=%08x\n", index, bytes,
                   (unsigned)fun_crc32(buffers[index], bytes));
        d->control = FUN_DMA_OWNER | BYTES;
        fun_dma_fence();
        /* The engine reports a descriptor-owner error if producer catches us.
         * No silent overwrite or claim of loss-free RX under a slow consumer.
         */
    }
}
