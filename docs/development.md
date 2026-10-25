# Development

Python 3.12+, esptool 4.8.1 and pyserial 3.5 are the host requirements. Firmware
uses Espressif GCC `esp-13.2.0_20240530`; download URLs and SHA-256 values are
pinned in `tools/toolchains.json`. `python tools/setup.py --all` installs both
compiler families. Portable host C tests additionally require GCC or Clang.

Use the checkout's virtual-environment Python for these commands:

```sh
python -m unittest discover -s tests -v
python tools/check_examples.py --matrix
python tools/check_release.py
python tools/check_licenses.py
python tools/capabilities.py --check
python tools/catalog.py
```

The image checker independently checks ROM headers, chip IDs, entry points,
SRAM bounds/aliases, checksums and SHA-256. It never opens a device. The build
frontend uses explicit argument vectors, warnings as errors and fixed linker
reservations. Build outputs remain under ignored `build/` directories.

After changing header gates or profiles, run `tools/capabilities.py --write`
and review both generated files. `tools/catalog.py --write-docs` regenerates
the ESP32 example index. Do not alter original third-party license headers;
update `docs/upstream-files.json` when adding an adaptation. Optional upstream
checks use `check_licenses.py --reference <ch32fun-checkout> --idf <idf-checkout>`.

S3 RAM execution requires `esp32fun.py ram` or `build --ram-image`; this keeps
code out of the lower ROM-download workspace. Flash builds retain their own
layout. Native C3/S3 USB console I/O requires `-D FUN_CONSOLE_USB=1`.

Classic ESP32 startup reconciles supported XTAL/PLL clock bookkeeping without
switching clocks. When ROM crystal metadata is absent, set
`FUN_ESP32_XTAL_MHZ` to the board's crystal frequency (default 40). Other clock
modes remain uncalibrated. The UART backend shares this decoder.
