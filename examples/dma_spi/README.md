# dma_spi

SPI2 mode-0 TX via GDMA, eight bytes, software CS held until SPI TRANS_DONE.

Build targets for the current implementation: esp32s3, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_spi
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
