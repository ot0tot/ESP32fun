/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
#ifndef PWM_PIN
#define PWM_PIN 5
#endif
int main(void)
{
    const fun_pwm output = {0, 0, PWM_PIN, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&output)) return 1;
    for (uint32_t duty = 0;; duty = (duty + 1u) & 255u) {
        fun_pwm_duty(&output, duty, 0);
        Delay_Ms(5);
    }
}
