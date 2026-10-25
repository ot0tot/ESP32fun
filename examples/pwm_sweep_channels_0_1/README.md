# pwm_sweep_channels_0_1

LEDC channels 0/1 on GPIO4/5; two duty ramps offset by half a ramp cycle.

Build targets for the current implementation: esp32, esp32s2, esp32s3, esp32c2, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/pwm_sweep_channels_0_1
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
