/* SPDX-License-Identifier: MIT */
#include "fun_pwm.h"
/* Hardware phase offset via LEDC HPOINT. */
int main(void)
{
    const fun_pwm a = {0, 0, 4, 10}, b = {1, 0, 5, 10};
    if (fun_pwm_timer(0, 10, 1000) || fun_pwm_bind(&a) || fun_pwm_bind(&b)) return 1;
    for (;;) for (unsigned percent = 9; percent < 100; percent += 10) {
        unsigned duty = percent * 1024u / 100u;
        fun_pwm_duty(&a, duty, 0);
        fun_pwm_duty(&b, duty, 512);
        fun_printf("%u%% duty; channel B phase=180 degrees\n", percent);
        Delay_Ms(2000);
    }
}
