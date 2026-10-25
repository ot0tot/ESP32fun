# Example wiring

Select a declared chip, check the board schematic and use 3.3 V logic.
Build targets describe the available implementation. [Validation notes](validation.md)
list tested boards and scenarios; missing board tests do not exclude a chip.

## Common pin defaults

`esp32fun/fun_board.h` centralizes bus defaults. All devices use common ground and ESP32-compatible 3.3 V signal levels. ESP32 pins are not 5 V tolerant; a 5 V MAX7219 or 74HC595 module can require level translation. Check each module's supply and logic specifications.

| Signal / define | Classic ESP32 | S2/S3/C2/C3/C5/C6/H2/P4 |
| --- | --- | --- |
| `SPI_CLOCK` | 18 | 4 |
| `SPI_MOSI` | 23 | 5 |
| `SPI_MISO` | 19 | 6 |
| `SPI_SELECT` / default 595 latch | 5 | 7 |
| `OLED_DC` | 26 | 10 |
| `OLED_RESET` / default LoRa reset | 25 | 3 |
| `NRF_CE` | 27 | 10 |
| `I2C_SDA` | 21 | 2 |
| `I2C_SCL` | 22 | 3 |

GPIO4 is the usual plain LED/output default. The three-pin direct/struct demos use 4/5/18 on classic ESP32 and 4/5/6 on other families. A valid SoC pin can be reserved by a module for flash/PSRAM, USB, straps, crystal, or another board function. Do not use a bus default blindly on a different module. Runtime validation checks SoC pin masks and input-only pins, not module wiring.

## Runnable projects

| Project or group | Hardware and expected observation |
| --- | --- |
| `blink`, `blink_raw`, `struct_gpio` | GPIO4, resistor and plain LED; observe output levels and repeated pulses. Raw and struct demos illustrate different access styles. |
| `direct_gpio`, `struct_direct_gpio` | Three output pins above, 950 ms high / 50 ms low; direct example mixes atomic register writes and output RMW. |
| `breathe` | GPIO4 LED; pulse-density brightness with quadratic envelope. Scope pulse density, not an assumed fixed PWM carrier. |
| `branchless_gpio_lib` | GPIO4 blink via arithmetic register selection. |
| `hello`, `constructors`, `cpp_virtual_methods`, `function_in_ram`, `template` | ROM console or debugger. Hello checks initialized data/BSS. Constructor demo reports two calls. C++ demo reports a virtual method result. Function demo reports a RAM address. Template deliberately has no external behavior. |
| `debugprintfdemo`, `uartdemo`, `examples/console_echo` | ROM console at 115200 baud; counter output, incoming characters and GPIO4/5 activity as described in each source. |
| `examples/button_uart` | External button from `BUTTON_PIN` (default GPIO2) to GND; each press sends characters with an internal pullup. Holding a strap pin low across reset can change boot mode. |
| `hc595_demo` | 74HC595: DS=MOSI, SHCP=clock, STCP=`LATCH_PIN`, /OE=GND, /SRCLR=VCC; walking output bit. MISO is unused. Use LED current resistors. |
| `i2c_oled` | SSD1306 128x32 four-pin I2C module, address `OLED_ADDRESS` 0x3c. External pullups to 3.3 V on SDA/SCL. Cycles fill, pixel, line, circle, bitmap, text and scaled-text modes. Power-on reset is used. |
| `spi_oled` | SSD1306 128x64 SPI module: clock, MOSI, select, DC, reset above; MISO unused. Conway life runs with a toroidal boundary and xorshift seed. |
| `spi_max7219` | One eight-digit MAX7219 module: CLK=clock, DIN=MOSI, LOAD=select, MISO unused. Single-module animation modes use software SPI. |
| `spi_24L01_tx`, `spi_24L01_rx` | Two nRF24L01 modules with SPI plus CE. Channel 76, 1 Mbps, pipe-zero address `fun32`, four-byte counters, CRC and auto-ACK. Check received counters and transmitter ACK/timeout messages. RF hardware, antenna, supply stability and interoperability are unverified. |
| `spi_lora_sx127x` | SX1276/77/78/79 compatible SPI module plus `LORA_RESET`. Default 915 MHz, SF7, BW125, CR4/5, CRC, private sync word 0x12 and PA_BOOST 17 dBm. Use a module/antenna appropriate to the selected frequency. Two peers exchange counter packets nominally every three seconds. Scope SPI and check version 0x12, RX payloads, RSSI/SNR and bounded TX completion. |
| `examples/i2c_bh1750_sht31` | BH1750 at 0x23 and SHT31 at 0x44, external 4.7k pullups; reports lux, signed centi-C and humidity in centi-percent. Failed bus/CRC readings report errors. |
| `examples/i2c_bh1750_scan` | I2C scan and BH1750 at 0x23. |
| `examples/ds18b20` | Exactly one externally-powered DS18B20 on `ONEWIRE_PIN` GPIO4, 4.7k pullup to 3.3 V. Reports signed raw temperature in sixteenths of a degree after presence/CRC checks. No parasite power or multi-device ROM search. |
| `pwm_sweep_channels_0_3`, `pwm_sweep_channels_0_1`, `pwm_pin_routing`, `pwm_fixed`, `pwm_percent_sweep`, `pwm_triangle`, `pwm_complementary` | Hardware LEDC output; usually GPIO4/5. S2/S3/C2/C3 assume a 40 MHz crystal; set `FUN_PWM_SOURCE_HZ=26000000` for a 26 MHz board. Classic ESP32 uses boot APB clock inferred from ROM metadata. Check frequency, duty and pin routing on a scope. |
| `pwm_phase_shift` | GPIO4/5, common 1 kHz LEDC timer and 180-degree HPOINT offset. |
| `examples/aes` | The current register backend is for C3. Console reports AES-128 known-answer encryption and decryption. This exercises the block accelerator, not a complete security protocol. |
| `random_numbers` | Deterministic LFSR known sequence. This is not a hardware RNG or security entropy source. |
| `examples/crc_test` | Software reflected CRC32 of `123456789`, expected `cbf43926`. |
| `self_modify_code` | RISC-V only. Changes one RAM CSR instruction from cycle counter to vendor ID, fences, executes and restores it. |
| `app_without_flash` | Use `ram` in download mode, observe GPIO4 250/250 ms and console message. Reset returns to the flash boot image; no flash write is performed by `ram`. |

Software SPI/I2C examples are polling implementations. They are not counted as hardware bus/DMA ports. Software I2C detects NACKs, stuck SDA at start, and clock-stretch timeout; it has no multi-master arbitration or automatic recovery pulses.

The separate [hardware I2C demos](drivers.md) use I2C0 command/FIFO hardware
with CPU polling on C3/classic ESP32. SDA/SCL are XIAO GPIO4/5 and PICO GPIO5/4,
with external pullups. They have no DMA claim.

The [DMA guide](drivers.md) describes default wiring, descriptor ownership
rules and peripheral limits. C3/S3 support
SPI2 and GDMA memory copies; C3/classic ESP32 add UART/UHCI and C3 adds raw ADC; S3 adds eight-bit
parallel LCD output and RMT pulse-duration capture. Finite capture/waveform
examples do not provide continuous capture or refill APIs.
