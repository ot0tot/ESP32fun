/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
int main(void)
{
    const fun_pwm output = {3, 0, 4, 10};
    if (fun_pwm_timer(0, 10, 1000) || fun_pwm_bind(&output)) return 1;
    for (;;) {
        for (unsigned duty = 0; duty < 1024; duty += 10) { fun_pwm_duty(&output, duty, 0); Delay_Ms(10); }
        for (int duty = 1023; duty >= 0; duty -= 10) { fun_pwm_duty(&output, (unsigned)duty, 0); Delay_Ms(10); }
    }
}
