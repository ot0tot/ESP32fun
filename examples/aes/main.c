/* SPDX-License-Identifier: MIT */
#include "fun_aes.h"
/* FIPS-197 AES-128 known-answer vector, then decrypt and compare. */
int main(void)
{
    const uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    const uint8_t plain[16] = {0,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    const uint8_t expected[16] = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
    uint8_t cipher[16], decoded[16];
    if (fun_aes128(key, plain, cipher, 0) || fun_aes128(key, cipher, decoded, 1)) {
        fun_puts("AES hardware timeout"); return 1;
    }
    unsigned ok = 1;
    for (unsigned i = 0; i < 16; ++i) ok &= cipher[i] == expected[i] && decoded[i] == plain[i];
    fun_printf("hardware AES128 known answer and decrypt: %s\n", ok ? "PASS" : "FAIL");
    for (;;) __asm__ volatile ("nop");
}
