/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
int main(void)
{
    const fun_pwm a = {0, 0, 4, 8}, b = {1, 0, 5, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&a) || fun_pwm_bind(&b)) return 1;
    for (unsigned count = 0;; count = (count + 1u) & 511u) {
        unsigned triangle = (count & 255u) ^ ((count >> 8) * 255u);
        fun_pwm_duty(&a, triangle, 0);
        fun_pwm_duty(&b, 255u - triangle, 0);
        Delay_Ms(5);
    }
}
