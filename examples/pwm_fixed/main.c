/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
int main(void)
{
    const fun_pwm output = {0, 0, 4, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&output)) return 1;
    fun_pwm_duty(&output, 128, 0);
    for (;;) __asm__ volatile ("nop");
}
