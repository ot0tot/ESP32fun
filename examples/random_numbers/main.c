/******************************************************************************
* Psuedo Random Number Generator using a Linear Feedback Shift Register
* - Demo Program
* See the GitHub for more information:
* https://github.com/ADBeta/CH32V003_lib_rand
*
* Ver 2.0    09 Sep 2024
*
* Released under the MIT Licence
* Copyright ADBeta (c) 2024
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
* USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

/* SPDX-License-Identifier: MIT
 * Same LFSR taps and strength-2 bit ordering as ADBeta's upstream lib_rand.
 * See THIRD_PARTY_NOTICES.md for the original copyright and MIT notice.
 */
#include "esp32fun.h"
static uint32_t state = 0x12345678;
static uint32_t next(void)
{
    uint32_t value = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        unsigned feedback = ((state >> 31) ^ (state >> 21) ^ (state >> 1) ^ state) & 1u;
        state = (state >> 1) | (feedback << 31);
        value = (value << 1) | feedback;
    }
    return value;
}
int main(void)
{
    const uint32_t expected[] = {3443170572u, 2761041505u, 3238759778u, 3045866432u};
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t value = next();
        fun_printf("Random number %u: %u (%s)\n", i, (unsigned)value,
                   value == expected[i] ? "PASS" : "FAIL");
    }
    return 0;
}
