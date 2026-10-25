# adc_dma

ADC1 GPIO0, 100 timer-paced DMA samples, uncalibrated raw min/max/mean.

Build targets for the current implementation: esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32c3 --project examples/adc_dma
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
