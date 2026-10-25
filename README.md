# ESP32fun

ESP32fun is a small C/C++ environment for ESP32 chips, inspired by
[ch32fun](https://github.com/cnlohr/ch32fun). It uses direct register access,
the ROM bootloader, fixed memory and Python/Make builds. The included firmware
builds without Arduino, an ESP-IDF installation, FreeRTOS or Rust.
A few drivers adapt ESP-IDF source; those files retain their Apache-2.0
notices. No ESP-IDF libraries are linked. See [source attribution](THIRD_PARTY_NOTICES.md).

## Quick start

Use Python 3.12 or newer. Setup installs pinned GCC and flashing dependencies
into `.tools/` and `.venv/` inside the checkout.

```sh
python tools/setup.py --chip esp32c3
```

The commands below use an ESP32-C3 with its native USB console. Replace the
port with your board's port. For the default UART console through a USB-to-UART
adapter, omit `-D FUN_CONSOLE_USB=1`.

On Windows:

```powershell
.venv/Scripts/python.exe tools/esp32fun.py build --chip esp32c3 --project examples/hello -D FUN_CONSOLE_USB=1
.venv/Scripts/python.exe tools/esp32fun.py flash --chip esp32c3 --project examples/hello --port COM5 -D FUN_CONSOLE_USB=1
.venv/Scripts/python.exe tools/esp32fun.py monitor --port COM5
```

On Linux or macOS:

```sh
.venv/bin/python tools/esp32fun.py build --chip esp32c3 --project examples/hello -D FUN_CONSOLE_USB=1
.venv/bin/python tools/esp32fun.py flash --chip esp32c3 --project examples/hello --port /dev/ttyACM0 -D FUN_CONSOLE_USB=1
.venv/bin/python tools/esp32fun.py monitor --port /dev/ttyACM0
```

`flash` rebuilds before writing the image. Standalone C3 images load at flash
offset `0x0`. S3 also supports the explicit native USB console option.

## Your application

Copy `examples/template`, keep its Makefile and edit `main.c`:

```c
#include "esp32fun.h"
int main(void) {
    if (fun_pin_mode(4, FUN_OUTPUT)) return 1;
    for (;;) {
        fun_digital_write(4, 1); Delay_Ms(500);
        fun_digital_write(4, 0); Delay_Ms(500);
    }
}
```

Choose a GPIO available on your board. Build with
`python tools/esp32fun.py build --chip <chip> --project <directory>`.
GNU Make is an optional wrapper. Startup initializes data/BSS and C++
constructors. The ROM console is not a full C library.

## Chips and driver support

The nine build profiles are ESP32, S2, S3, C2, C3, C5, C6, H2 and P4. There
are 56 example projects and 343 project/chip combinations.

The [example index](docs/examples-index.md) lists the included projects and
current build targets. Use the [driver table](docs/capabilities.md) for implemented
backends and the [wiring guide](docs/examples.md) to choose pins.

[Validation notes](docs/validation.md) record the boards and tests used so far.
An untested board is not an unsupported board. Driver backends that still need
implementation are identified separately; build checks do not test wiring,
analog accuracy or external devices.

## Documentation

- [Documentation](docs/index.md)
- [Driver behavior and limits](docs/drivers.md)
- [Design](docs/design.md), [limits](docs/limits.md) and [development](docs/development.md)
- [Release contents](RELEASE.md) and [third-party notices](THIRD_PARTY_NOTICES.md)

Source licenses are retained in each file and [LICENSE](LICENSE).
[Third-party attribution](THIRD_PARTY_NOTICES.md) lists the upstream sources,
revisions and licenses.
