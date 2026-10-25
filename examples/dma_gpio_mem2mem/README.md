# dma_gpio_mem2mem

Eight parallel GPIOs alternate 0x55/0xaa and zero frames via LCD_CAM/GDMA; finite bursts.

Build targets for the current implementation: esp32s3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_gpio_mem2mem
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
