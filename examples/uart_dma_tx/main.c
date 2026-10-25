/* SPDX-License-Identifier: MIT */
#include "fun_uart_dma.h"
#ifdef ESP32FUN_ESP32
#define MESSAGE "UART1 via UHCI0 and real UDMA\r\n"
#else
#define MESSAGE "UART1 via UHCI0 and real GDMA\r\n"
#endif
static char message[(sizeof(MESSAGE) + 3u) & ~3u] __attribute__((aligned(4))) = MESSAGE;
int main(void)
{
    if (fun_uart_dma_init(4, 5, 115200)) return 1;
    for (;;) {
        int result = fun_uart_dma_send(message, sizeof(MESSAGE) - 1u, 100000);
        fun_printf("UART1 DMA transmission: %s (%d)\n", result ? "ERROR" : "DONE", result);
        if (result) return 2;
        Delay_Ms(1000);
    }
}
