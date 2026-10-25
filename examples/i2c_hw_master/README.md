# i2c_hw_master

Polling hardware I2C0 master, 100/400 kHz, probes, FIFO boundaries and repeated START.

Build targets for the current implementation: esp32, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/i2c_hw_master
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
