/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
int main(void)
{
    const fun_pwm a = {0, 0, 4, 8}, b = {3, 0, 5, 8};
    if (fun_pwm_timer(0, 8, 1000) || fun_pwm_bind(&a) || fun_pwm_bind(&b)) return 1;
    for (uint32_t count = 0;; count = (count + 1u) & 255u) {
        fun_pwm_duty(&a, count, 0);
        fun_pwm_duty(&b, (count + 128u) & 255u, 0);
        Delay_Ms(5);
    }
}
