# pwm_pin_routing

LEDC output routed through GPIO matrix; PWM_PIN defaults to GPIO5.

Build targets for the current implementation: esp32, esp32s2, esp32s3, esp32c2, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/pwm_pin_routing
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
