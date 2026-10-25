# dma_gpio

Eight GPIO parallel byte patterns via LCD_CAM/GDMA at 5 MSamples/s; finite bursts.

Build targets for the current implementation: esp32s3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_gpio
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
