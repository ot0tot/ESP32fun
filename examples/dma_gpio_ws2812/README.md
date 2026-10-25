# dma_gpio_ws2812

24 WS2812 pixels on MOSI using GDMA/SPI serialization, .4/.8 us high pulses and 320 us reset.

Build targets for the current implementation: esp32s3, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_gpio_ws2812
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
