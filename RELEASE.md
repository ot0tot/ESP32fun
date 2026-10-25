# ESP32fun source release — October 25, 2026

This source release includes the SDK-free C/C++ runtime, GPIO and bus drivers,
56 example projects, pinned compiler setup, portable unit tests and Linux CI.
It supports 343 project/chip build combinations across nine chip profiles.

To start, follow the [README](README.md). Check the
[driver/chip table](docs/capabilities.md) and [example wiring](docs/examples.md)
for your board. [Validation notes](docs/validation.md) describe the boards
and test conditions used so far; remaining backend work is listed separately.

## Contents

- A ROM-loaded runtime with fixed memory allocation and C++ constructor support.
- GPIO, software SPI/I2C and chip-specific PWM, hardware I2C, DMA and AES drivers.
- Python build, flash, RAM-load and monitor tools, with an optional Make wrapper.
- Compiler downloads with pinned versions and SHA-256 checks.
- An [example index](docs/examples-index.md) with build targets and wiring instructions.

The [project manifest](examples/projects.json) defines the included examples and
their chip profiles. These are implementation declarations, not a list of the
boards that have been tested.

## Support limits

- The runtime has no heap, scheduler, interrupt dispatcher or second-core startup.
- DMA backends implement the documented finite transfer modes. They do
  not provide continuous lossless queues or general peripheral sharing.
- Build profiles and host tests do not validate board wiring, analog accuracy,
  external devices or every operating condition.

See the [driver details](docs/drivers.md) and [known limits](docs/limits.md) for
the remaining peripheral and application gaps.

## Release checks and licenses

CI builds every declared project/chip combination, checks the generated images
and runs the host tests. It also checks the example inventory, capability-table
freshness and license notices. The commands are in the
[development guide](docs/development.md). CI does not access physical devices.

At a release tag, record the exact commit and passing CI artifacts. Existing
licenses and third-party notices are retained; source revisions and license
terms are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
