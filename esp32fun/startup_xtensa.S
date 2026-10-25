/* SPDX-License-Identifier: MIT */
    .section .text.start,"ax",@progbits
    .global _start
    .type _start,@function
    .align 4
_start:
    /* Windowed ABI matches the mask ROM. Keep ROM's exception vectors for
     * register-window overflow/underflow; mask ordinary interrupts at level 15.
     * The second CPU remains in its reset state.
     */
    movi a0, 0
    wsr a0, WINDOWBASE
    movi a0, 1
    wsr a0, WINDOWSTART
    rsync
    movi a0, 0x4002f
    wsr a0, PS
    rsync
    movi a1, __stack_top
    movi a0, 0
    call4 fun_runtime_start
1:
    j 1b
    .size _start, .-_start
    .section .note.GNU-stack,"",@progbits
