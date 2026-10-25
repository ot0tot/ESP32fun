/* SPDX-License-Identifier: MIT */
    .section .text.start,"ax",@progbits
    .global _start
    .type _start,@function
    .balign 4
_start:
    /* Global masking is sufficient for this polling runtime. C2/C3 do not
     * implement standard mie (0x304); writing it traps before stack setup.
     */
    csrci mstatus, 8
#if FUN_CYCLE_CSR == 0x7e2
    /* Espressif performance counter: count CPU cycles, enable, wrap on overflow.
     * The standard mcycle CSR is absent on C2/C3/C6/H2.
     */
    csrwi 0x7e0, 1
    csrwi 0x7e1, 1
#endif
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, __stack_top
    la t0, _trap
    csrw mtvec, t0
    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sw zero, 0(t0)
    addi t0, t0, 4
    j 1b
2:
    /* .data and .rodata have already been loaded by the chip's ROM. */
    fence.i
    call fun_runtime_start
3:
    wfi
    j 3b
    .size _start, .-_start

    .global _trap
    .type _trap,@function
    /* ESP RISC-V implementations reserve low mtvec address bits. */
    .balign 256
_trap:
    /* Do not depend on a valid stack when recording a fault. */
    la t0, fun_last_trap
    csrr t1, mcause
    sw t1, 0(t0)
    csrr t1, mepc
    sw t1, 4(t0)
    csrr t1, mtval
    sw t1, 8(t0)
4:
    wfi
    j 4b
    .size _trap, .-_trap
    .section .note.GNU-stack,"",@progbits
