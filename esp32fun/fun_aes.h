/* SPDX-License-Identifier: MIT */
#ifndef FUN_AES_H
#define FUN_AES_H
#include "esp32fun.h"
#if !FUN_AES_SUPPORTED
#error "Polling AES backend not implemented for this target; current register backend is ESP32-C3"
#endif
static inline uint32_t fun_load_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
/* One AES-128 ECB block, for accelerator demonstration only. */
static inline int fun_aes128(const uint8_t key[16], const uint8_t input[16], uint8_t output[16], int decrypt)
{
    FUN_REG(FUN_AES_BUS_CLOCK) |= FUN_AES_BUS_ENABLE;
    FUN_REG(FUN_AES_BUS_RESET) |= FUN_AES_RESET_MASK;
    FUN_REG(FUN_AES_BUS_RESET) &= ~(FUN_AES_RESET_MASK | FUN_AES_DS_RESET_MASK);
    FUN_REG(FUN_AES_DMA_ENABLE) = 0;
    FUN_REG(FUN_AES_MODE) = decrypt ? 4 : 0;
    for (unsigned i = 0; i < 4; ++i) {
        FUN_REG(FUN_AES_BASE + 4u * i) = fun_load_le32(key + 4u * i);
        FUN_REG(FUN_AES_INPUT + 4u * i) = fun_load_le32(input + 4u * i);
    }
    FUN_REG(FUN_AES_TRIGGER) = 1;
    uint32_t start = fun_cycle_count(), timeout = fun_cpu_mhz() * 10000u;
    while (FUN_REG(FUN_AES_STATE) != 0) {
        if ((uint32_t)(fun_cycle_count() - start) >= timeout) return -1;
    }
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t word = FUN_REG(FUN_AES_OUTPUT + 4u * i);
        for (unsigned byte = 0; byte < 4; ++byte) output[4u * i + byte] = (uint8_t)(word >> (8u * byte));
        FUN_REG(FUN_AES_BASE + 4u * i) = 0;
    }
    return 0;
}
#endif
