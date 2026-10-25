# dma_mem2mem

8192-byte real GDMA SRAM-to-SRAM copy across three descriptors, byte-for-byte verification.

Build targets for the current implementation: esp32s3, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/dma_mem2mem
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
