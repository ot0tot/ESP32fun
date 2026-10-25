# waveshare_c6_lcd

Waveshare C6 onboard ST7789 color bars and RGB LED, polling SPI/cycle timing; 25% backlight.

Build targets for the current implementation: esp32c6.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32c6 --project examples/waveshare_c6_lcd
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
