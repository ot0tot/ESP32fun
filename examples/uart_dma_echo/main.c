/* SPDX-License-Identifier: MIT */
#include "fun_uart_dma.h"
#ifdef ESP32FUN_ESP32
/* Classic UDMA needs four unused bytes in the final RX descriptor. */
static uint8_t packet[260] __attribute__((aligned(4)));
#else
static uint8_t packet[256] __attribute__((aligned(4)));
#endif
int main(void)
{
    if (fun_uart_dma_init(4, 5, 115200)) return 1;
    fun_puts("UART1 DMA idle-packet echo: TX GPIO4, RX GPIO5, 115200 8N1");
    for (;;) {
        int bytes = fun_uart_dma_receive(packet, sizeof(packet), 1000000);
        if (bytes == FUN_DMA_TIMEOUT) continue;
        if (bytes < 0) { fun_printf("UART RX DMA error %d\n", bytes); return 2; }
        int result = fun_uart_dma_send(packet, (unsigned)bytes, 100000);
        if (result) { fun_printf("UART TX DMA error %d\n", result); return 3; }
        fun_printf("Echoed %d DMA bytes\n", bytes);
    }
}
