# spi_lora_sx127x

SX1276/77/78/79 bidirectional LoRa counter packets via polling software SPI; default 915 MHz, SF7, BW125, CR4/5 and CRC.

Build targets for the current implementation: esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/spi_lora_sx127x
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
