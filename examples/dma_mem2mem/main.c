/* SPDX-License-Identifier: MIT
 * Additional ESP32-specific demo, not a replacement for DMA-to-GPIO mem2mem.
 */
#include "fun_dma.h"
static uint32_t source[2048], destination[2048];
static fun_dma_desc tx[3], rx[3];
int main(void)
{
    /* 8192 bytes force three descriptors: 4092 + 4092 + 8. */
    for (unsigned frame = 0;; ++frame) {
        for (unsigned i = 0; i < 2048; ++i) {
            source[i] = (i * 0x9e3779b9u) ^ frame;
            destination[i] = ~source[i];
        }
        unsigned offset = 0;
        for (unsigned i = 0; i < 3; ++i) {
            unsigned bytes = i == 2 ? 8u : FUN_DMA_MAX_BYTES;
            if (fun_dma_descriptor(&tx[i], (uint8_t *)source + offset, bytes, i == 2 ? 0 : &tx[i+1], 0) ||
                fun_dma_descriptor(&rx[i], (uint8_t *)destination + offset, bytes, i == 2 ? 0 : &rx[i+1], 1)) return 1;
            offset += bytes;
        }
        unsigned start = fun_cycle_count();
        int result = fun_dma_copy(tx, 3, rx, 3, 10000);
        unsigned cycles = fun_cycle_count() - start;
        if (result) { fun_printf("M2M GDMA error %d\n", result); return 2; }
        unsigned errors = 0;
        for (unsigned i = 0; i < 2048; ++i) errors += source[i] != destination[i];
        fun_printf("8192-byte M2M GDMA frame %u: %s, cycles=%u, mismatches=%u\n",
                   frame, errors ? "FAIL" : "PASS", cycles, errors);
        if (errors) return 3;
        Delay_Ms(1000);
    }
}
