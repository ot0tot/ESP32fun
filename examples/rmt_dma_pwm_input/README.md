# rmt_dma_pwm_input

RMT/GDMA PWM frequency and duty capture; GPIO4 test output wired to GPIO5 input.

Build targets for the current implementation: esp32s3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32s3 --project examples/rmt_dma_pwm_input
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
