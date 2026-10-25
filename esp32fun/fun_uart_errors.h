/* Common raw UART error bits: parity, framing, FIFO overflow and BREAK. */
#ifndef FUN_UART_ERRORS_H
#define FUN_UART_ERRORS_H
#include "esp32fun.h"
#define FUN_UART_DMA_ERRORS (FUN_BIT(2) | FUN_BIT(3) | FUN_BIT(4) | FUN_BIT(7))
#endif
