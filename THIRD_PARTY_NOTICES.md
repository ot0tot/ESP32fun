# Third-party attribution

Copied and adapted files retain their original copyright and license headers.
Pinned paths, revisions, source hashes and required notices are recorded in
[the provenance manifest](docs/upstream-files.json). Full Apache-2.0 terms
are in [licenses/Apache-2.0.txt](licenses/Apache-2.0.txt); MIT terms are
carried in the source headers and [LICENSE](LICENSE).

| File | Source | License |
|---|---|---|
| `examples/breathe/main.c` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/breathe/breathe.c) | MIT |
| `examples/random_numbers/main.c` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/random_numbers/random_numbers.c) | MIT |
| `examples/i2c_oled/main.c` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/i2c_oled/i2c_oled.c) | MIT |
| `examples/i2c_oled/ssd1306.h` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/extralibs/ssd1306.h) | MIT |
| `examples/i2c_oled/font_8x8.h` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/extralibs/font_8x8.h) | MIT |
| `examples/i2c_oled/bomb.h` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/i2c_oled/bomb.h) | MIT |
| `examples/spi_max7219/main.c` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/spi_max7219/spi_max7219.c) | MIT |
| `examples/spi_max7219/max7219_spi_driver.h` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/spi_max7219/max7219_spi_driver.h) | MIT |
| `examples/spi_max7219/max7219_spi_driver_extended.h` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/spi_max7219/max7219_spi_driver_extended.h) | MIT |
| `examples/dma_spi/main.c` | [cnlohr/ch32fun](https://github.com/cnlohr/ch32fun/blob/6b247c7c10f1a96c1f0117edc47a695e9b0ab538/examples/dma_spi/dma_spi.c) | MIT |
| `esp32fun/fun_adc_dma.h` | [espressif/esp-idf](https://github.com/espressif/esp-idf/blob/c8fc5f643b7a7b0d3b182d3df610844e3dc9bd74/components/hal/esp32c3/include/hal/adc_ll.h) | Apache-2.0 |
| `esp32fun/fun_usb_console.h` | [espressif/esp-idf](https://github.com/espressif/esp-idf/blob/c8fc5f643b7a7b0d3b182d3df610844e3dc9bd74/components/esp_rom/patches/esp_rom_sys.c) | Apache-2.0 |
| `esp32fun/fun_i2c_hw.h` | [espressif/esp-idf](https://github.com/espressif/esp-idf/blob/c8fc5f643b7a7b0d3b182d3df610844e3dc9bd74/components/hal/esp32/include/hal/i2c_ll.h) | Apache-2.0 |

The runtime uses Espressif register and ROM-address facts without linking
ESP-IDF libraries. The ADC, hardware-I2C and native USB-console adaptations retain their
individual Espressif Apache-2.0 notices. That license covers those adaptations;
it does not replace the root MIT license for original ESP32fun code. Other example adaptations retain
their individual upstream notices, including the ADBeta LFSR license.
