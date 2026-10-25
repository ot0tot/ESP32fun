/* SPDX-License-Identifier: MIT */
#include "esp32fun.h"
class Printer { public: virtual unsigned value(unsigned input) = 0; };
class Example : public Printer {
    unsigned offset;
public:
    Example() : offset(10) {}
    unsigned value(unsigned input) override { return input + offset; }
};
static Example example;
/* Keep the demo's vtable dispatch observable rather than allowing IPA to
 * specialize this function for the only call site and fold the virtual call.
 */
__attribute__((noipa)) static void print(Printer &printer)
{
    fun_printf("Virtual method result: %u\n", printer.value(10));
}
int main()
{
    for (;;) { print(example); Delay_Ms(1000); }
}
