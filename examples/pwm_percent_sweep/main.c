/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
int main(void)
{
    const fun_pwm output = {0, 0, 4, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&output)) return 1;
    for (unsigned percent = 0;; percent = (percent + 1u) % 101u) {
        /* Eight-bit resolution supports exact 100% duty (256/256). */
        fun_pwm_duty(&output, percent * 256u / 100u, 0);
        Delay_Ms(15);
    }
}
