/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"

#ifndef LED_PIN
#define LED_PIN 4
#endif

/* GPIO4 -> 330 ohm resistor -> LED anode; LED cathode -> GND. */
_Static_assert(LED_PIN >= 0 && LED_PIN < FUN_GPIO_COUNT, "Invalid GPIO number");
_Static_assert(((LED_PIN < 32 ? FUN_GPIO_OUTPUT_LOW : FUN_GPIO_OUTPUT_HIGH) >> (LED_PIN & 31)) & 1u, "Not an output GPIO");
#ifdef ESP32FUN_ESP32C3
_Static_assert(LED_PIN < 11 || LED_PIN > 17, "Avoid flash/power pins");
_Static_assert(LED_PIN != 18 && LED_PIN != 19, "Keep USB pins available");
#endif

int main(void)
{
    fun_pin_mode(LED_PIN, FUN_OUTPUT);
    fun_printf("ESP32fun: blinking GPIO%d\n", LED_PIN);
    for (;;) {
        fun_digital_write(LED_PIN, 1);
        Delay_Ms(500);
        fun_digital_write(LED_PIN, 0);
        Delay_Ms(500);
    }
}
