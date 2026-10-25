# i2c_register_target

Polling hardware I2C0 target at address 0x42 with a writable 256-byte register window.

Build targets for the current implementation: esp32, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/i2c_register_target
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
