# Known limits

- Build profiles and unit tests do not qualify every chip revision, module,
  pin, clock mode, supply condition or external device. Check your schematic.
- No heap, interrupt dispatcher, scheduler, runtime stack guard or second-core
  startup is provided. There is no general low-power or programmable USB stack.
- DMA drivers own their channels and peripherals exclusively. Finite captures
  and transfers are not continuous lossless queues. Refilling, concurrency,
  saturation handling and indefinite operation need application-specific work.
- C6/H2/P4/C5 LEDC layouts, hardware I2C beyond C3/classic, and ADC DMA beyond
  C3 are not implemented by the included drivers. C5 uses conservative limits.
- ADC output is raw and uncalibrated. Analog setup includes a private ROM call
  with no software timeout. No voltage-accuracy or spectral-quality claim follows.
- S3 parallel output uses finite LCD/GDMA bursts. Tiny descriptor geometries
  near the fastest clocks need separate qualification; no all-geometry guarantee.
- Device examples require their real sensors, displays, shift registers or
  external radio modules. A decoded bus waveform cannot establish device behavior.
- [Validation notes](validation.md) list tested boards and scenarios. Untested
  boards are not excluded from use. Automated CI checks builds and deterministic
  behavior without accessing physical boards.
