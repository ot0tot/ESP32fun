# dma_scheduled_io

GDMA/SPI single-output 1 MHz scheduled waveform; no arbitrary register scheduling.

Build targets for the current implementation: esp32s3, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_scheduled_io
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
