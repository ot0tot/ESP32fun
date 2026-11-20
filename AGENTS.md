# Working with ESP32fun

ESP32fun is a small, freestanding C/C++ environment for ESP32 chips. Applications
use direct registers, a few ROM functions and fixed SRAM. Python builds a ROM
boot image; Make is an optional wrapper. Firmware builds without an ESP-IDF
installation, Arduino, FreeRTOS or Rust. Some drivers adapt Espressif source and
retain its license notices.

Keep applications small and readable. Use the existing runtime and drivers
before adding another abstraction or dependency. Follow the user's requested
scope and preserve unrelated work in the checkout.

## Start here

1. Check `git status --short` and the current branch before editing.
2. Read [README.md](README.md) for application setup and
   [docs/development.md](docs/development.md) for the development workflow.
3. Find the closest example in [docs/examples-index.md](docs/examples-index.md).
   Read its `main.c` and the driver header it uses.
4. Check [docs/capabilities.md](docs/capabilities.md),
   [docs/drivers.md](docs/drivers.md) and [docs/examples.md](docs/examples.md)
   for the selected chip, resource ownership and wiring.

The profiles are `esp32`, `esp32s2`, `esp32s3`, `esp32c2`, `esp32c3`, `esp32c5`,
`esp32c6`, `esp32h2` and `esp32p4`. C3 is the default; specify `--chip` in
commands so the intended target is clear. Chip names are not board models.
Ask for the board model and wiring when they affect the task; host-only work
does not require a connected board.

## Repository map

| Location | Purpose |
|---|---|
| `esp32fun/esp32fun.h`, `esp32fun/esp32fun.c` | GPIO, delays, ROM console and common startup support |
| `esp32fun/startup_riscv.S` | Shared startup for RISC-V targets |
| `esp32fun/startup_xtensa.S` | Xtensa startup and ROM exception-vector assumptions |
| `esp32fun/fun_*.h` | Peripheral drivers and their buffer, timing and ownership contracts |
| `esp32fun/targets/profiles.json`, `esp32fun/targets/*.h` | Per-chip memory, ROM symbols, GPIO and register facts |
| `tools/esp32fun.py`, `tools/targets.py` | Build/flash frontend, target selection and linker-script generation |
| `tools/setup.py`, `tools/toolchains.json` | Host setup and pinned compiler downloads |
| `examples/`, `examples/projects.json` | Runnable applications and their declared build targets |
| `tests/` | Host models, compiler checks and image-validation tests |
| `docs/`, `THIRD_PARTY_NOTICES.md`, `licenses/` | Usage, validation, design limits and source attribution |

## Build and use an application

Use Python 3.12 or newer. From the repository root:

```sh
python tools/setup.py --chip esp32c3
```

Setup creates `.venv/` and uses or installs the pinned compiler. Use
`--all` instead of `--chip esp32c3` when both RISC-V and Xtensa compilers are
needed. After setup, use `.venv/Scripts/python.exe` on Windows or
`.venv/bin/python` on Linux/macOS. In the commands below, `python` means that
interpreter.

```sh
python tools/esp32fun.py targets
python tools/esp32fun.py doctor --chip esp32c3
python tools/esp32fun.py build --chip esp32c3 --project examples/hello -D FUN_CONSOLE_USB=1
```

`FUN_CONSOLE_USB=1` selects the native USB console on C3/S3. Omit it for the
default UART console. A USB-to-UART adapter is a UART connection, not the native
USB console. Use the same definitions for `build`, `flash` and `ram`.

For a new application, copy `examples/template` to `examples/my_app`, edit
`main.c` and build it:

```sh
python tools/esp32fun.py build --chip esp32c3 --project examples/my_app
```

The Python frontend also accepts an application directory outside the repo.
It compiles `.c`, `.cpp`, `.cc`, `.s` and `.S` files directly in that directory;
it does not recursively discover sources. The template's Makefile assumes the
usual `examples/<name>` location, so adjust its include path if moving it.

C3 outputs go in the application's `build/`; other chips use `build/<chip>/`.
Inspect `firmware.elf`, `firmware.map` and generated `target.ld` for memory
usage. `compile_commands.json` is available for editor tooling. Do not edit
generated build files to fix a source or linker-generation problem.

Flashing and RAM loading require a board and authorization to replace its
running firmware. Replace `COM5` with the actual port:

```sh
python tools/esp32fun.py flash --chip esp32c3 --project examples/hello --port COM5 -D FUN_CONSOLE_USB=1
python tools/esp32fun.py monitor --chip esp32c3 --port COM5
```

`flash` rebuilds first and selects the target's ROM-image offset. Set
`--flash-size` to the board's actual capacity; the default is 4 MB. These images
replace the boot image and do not use an IDF partition table. `ram` also rebuilds;
for S3 use `ram` or `build --ram-image` to preserve the ROM-download workspace.
The serial monitor runs at 115200 baud. Do not guess a port or flash offset.

## Firmware changes

- Read [docs/design.md](docs/design.md) and [docs/limits.md](docs/limits.md)
  before changing startup, clocks, memory or shared drivers. The current public
  runtime has no heap, scheduler, interrupt dispatcher or second-core startup.
  C++ uses a restricted freestanding runtime; do not assume exceptions, RTTI,
  threads or the full standard library.
- Check each chip's registers, clocks, ROM bindings and DMA routing. Sharing
  an instruction set does not make two chips' peripherals interchangeable.
  Keep `esp32fun/targets/profiles.json` and the corresponding target headers consistent.
  `tools/import_idf_targets.py` is a reference-import tool; an SDK checkout must
  not become a normal firmware build dependency.
- Preserve linker bounds, physical SRAM alias checks and stack reservations.
  Do not enlarge a region merely to silence an overflow. Clock bookkeeping
  updates do not switch hardware clocks or independently calibrate timing.
- Keep polling bounded and failures explicit. Respect descriptor alignment,
  internal-SRAM requirements, partial-data validity and stop/reinitialization
  rules in the relevant header. DMA engines and peripherals are exclusive;
  combining working examples does not establish safe concurrent ownership.
- Check the board schematic as well as chip pin masks. Flash/PSRAM pins,
  boot straps, onboard devices and console routing can reserve valid GPIOs.
  Use `fun_board.h` defaults or explicit definitions after checking the board.
- Keep copied/adapted source headers intact. Record origins, revisions and
  notices in `docs/upstream-files.json` and `THIRD_PARTY_NOTICES.md`, and include
  required license texts. Register definitions adapted from another project
  still need their provenance tracked. Do not relicense third-party code.

## Examples, generated docs and checks

For an example intended for distribution, add its entry to
`examples/projects.json` and document wiring, expected output and limitations.
A user's standalone application does not need a catalog entry. Prefer the
closest existing example's layout and make pin assignments easy to override.

Regenerate the example index after catalog changes:

```sh
python tools/catalog.py --write-docs
```

After changing target profiles or driver header gates, regenerate and review
both capability files:

```sh
python tools/capabilities.py --write
```

Follow `docs/development.md` for the full checks. Shared firmware/build changes
need the relevant host tests and target builds; release checks are:

```sh
python -m unittest discover -s tests -v
python tools/check_examples.py --matrix
python tools/check_release.py
python tools/check_licenses.py
python tools/capabilities.py --check
```

Host C models require GCC or Clang in addition to the embedded compilers.
For a focused example build, use `tools/check_examples.py --chip <chip>
--project examples/<name>`. It builds and validates the image without opening
a serial port. Builds, checks and setup failures must be reported as such;
time spent waiting for user approval is not evidence of a hung process.

Keep host/model results, target build results and physical validation separate.
An untested MCU is not an unsupported MCU. Describe missing backends separately
from behavior validated only on particular boards. Record actual hardware tests
in [docs/validation.md](docs/validation.md) with the board, configuration and
remaining scope. A waveform decode does not establish analog accuracy or that
an attached device works; finite transfers do not establish continuous streaming.

Before handing off, report what changed, the exact checks run and what still
needs a board or fixture. Keep generated artifacts, local captures, credentials
and workstation-specific paths out of commits. Commit or publish according to
the user's request; do not fold unrelated experimental work into the change.
