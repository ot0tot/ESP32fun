/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
#ifndef BUTTON_PIN
#define BUTTON_PIN 2
#endif
/* External active-low pushbutton to GND; internal pullup. */
int main(void)
{
    if (fun_pin_mode(BUTTON_PIN, FUN_INPUT_PULLUP)) return 1;
    const char text[] = "ESP32fun is awesome!\r\n";
    unsigned index = 0;
    for (;;) {
        if (!fun_digital_read(BUTTON_PIN)) {
            fun_putchar((uint8_t)text[index++]);
            if (index == sizeof(text) - 1) index = 0;
        }
        Delay_Ms(100);
    }
}
