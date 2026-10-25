/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
#if FUN_ARCH_XTENSA
#error "This CSR-instruction modification example requires a RISC-V target"
#endif
extern volatile uint32_t modified_instruction;
__attribute__((noinline)) static uint32_t read_modified(void)
{
    uint32_t result;
    __asm__ volatile (
        ".option push\n.option norvc\n.balign 4\n"
        ".global modified_instruction\nmodified_instruction:\n"
        "csrr %0, %1\n.option pop\n" : "=r"(result) : "i"(FUN_CYCLE_CSR));
    return result;
}
int main(void)
{
    /* Store through the data-bus alias of the instruction's SRAM address. */
    volatile uint32_t *writable = (volatile uint32_t *)((uintptr_t)&modified_instruction - FUN_IRAM_ALIAS);
    uint32_t original = *writable;
    fun_printf("cycle counter via patched instruction: %08x\n", (unsigned)read_modified());
    *writable = (original & 0x000fffffu) | (0xf11u << 20); /* mvendorid */
    __asm__ volatile ("fence rw,rw\nfence.i" ::: "memory");
    fun_printf("mvendorid via patched instruction: %08x\n", (unsigned)read_modified());
    *writable = original;
    __asm__ volatile ("fence rw,rw\nfence.i" ::: "memory");
    return 0;
}
