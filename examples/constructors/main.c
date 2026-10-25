/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
static volatile unsigned count;
__attribute__((constructor(101))) static void first(void) { ++count; }
__attribute__((constructor(102))) static void second(void) { ++count; }
int main(void)
{
    fun_printf("Constructors called: %u (%s)\n", count, count == 2 ? "PASS" : "FAIL");
    return count != 2;
}
