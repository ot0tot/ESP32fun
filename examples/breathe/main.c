/*
 * MIT License
 *
 * Copyright (c) 2023-2024 CNLohr <lohr85@gmail.com>, et. al.
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Copyright (c) 2023-2024 E. Brombaugh
 * Copyright (c) 2023-2024 A. Mandera
 * Copyright (c) 2005-2020 Rich Felker, et al.
 * Copyright (c) 2013,2014 Michal Ludvig <michal@logix.cz>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* SPDX-License-Identifier: MIT
 * Pulse-density breathing algorithm adapted from cnlohr/ch32fun/examples/breathe.
 * Original project copyright notices are retained in THIRD_PARTY_NOTICES.md.
 */
#include "esp32fun.h"
#ifndef LED_PIN
#define LED_PIN 4
#endif
int main(void)
{
    if (fun_pin_mode(LED_PIN, FUN_OUTPUT)) return 1;
    uint32_t lung = 1, roll = 0, volume = 1;
    int32_t direction = 1;
    for (;;) {
        for (unsigned r = 0; r < 64; ++r) {
            roll += lung;
            fun_digital_write(LED_PIN, roll < lung);
        }
        volume += direction;
        if (volume == 1 || volume == 65535) direction = -direction;
        else if (direction == 1) lung += (volume << 1) - 1;
        else lung -= (volume << 1) - 1;
    }
}
