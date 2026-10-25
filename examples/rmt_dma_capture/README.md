# rmt_dma_capture

RMT channel7/GDMA captures finite pulse-duration blocks at 1 us resolution.

Build targets for the current implementation: esp32s3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/rmt_dma_capture
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
