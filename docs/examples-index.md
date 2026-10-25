# ESP32 examples

56 projects and 343 project/chip build combinations.

Build targets describe the current implementation. Board tests are listed
separately in [validation notes](validation.md); an untested board is not
an unsupported board. See [wiring](examples.md) before connecting hardware.

| Project | Build targets | Description |
|---|---|---|
| [adc_dma](../examples/adc_dma/) | esp32c3 | ADC1 GPIO0, 100 timer-paced DMA samples, uncalibrated raw min/max/mean. |
| [aes](../examples/aes/) | esp32c3 | C3 hardware AES-128 ECB encryption/decryption using the FIPS-197 known-answer vector; not a cryptographic application. |
| [app_without_flash](../examples/app_without_flash/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | ROM serial RAM loading; reset restores the flash program. |
| [blink](../examples/blink/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | External LED on GPIO4, 500 ms high / 500 ms low. |
| [blink_raw](../examples/blink_raw/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | GPIO4 blink using GPIO W1TS/W1TC register writes. |
| [branchless_gpio_lib](../examples/branchless_gpio_lib/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | GPIO4 blink using arithmetic register selection. |
| [breathe](../examples/breathe/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Quadratic pulse-density breathing algorithm; GPIO4, no hardware PWM. |
| [button_uart](../examples/button_uart/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Active-low button on GPIO2 with internal pullup; sends successive characters while held. |
| [console_echo](../examples/console_echo/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Periodic hello counter and receive echo using polling ROM UART0. |
| [console_minimal](../examples/console_minimal/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | ROM console hello and empty polling loop. |
| [constructors](../examples/constructors/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Two constructor priorities execute before main; reports constructor count. |
| [cpp_virtual_methods](../examples/cpp_virtual_methods/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Global C++ object construction and virtual dispatch; no heap, exceptions or RTTI. |
| [crc_test](../examples/crc_test/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Software reflected CRC32 known-answer test of 123456789, expected cbf43926. |
| [debugprintfdemo](../examples/debugprintfdemo/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | GPIO4/5 activity with bidirectional ROM console output and input. |
| [direct_gpio](../examples/direct_gpio/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Separate W1TS/W1TC writes and output read-modify-write; 950/50 ms three-pin pattern. |
| [dma_gpio](../examples/dma_gpio/) | esp32s3 | Eight GPIO parallel byte patterns via LCD_CAM/GDMA at 5 MSamples/s; finite bursts. |
| [dma_gpio_mem2mem](../examples/dma_gpio_mem2mem/) | esp32s3 | Eight parallel GPIOs alternate 0x55/0xaa and zero frames via LCD_CAM/GDMA; finite bursts. |
| [dma_gpio_ws2812](../examples/dma_gpio_ws2812/) | esp32s3, esp32c3 | 24 WS2812 pixels on MOSI using GDMA/SPI serialization, .4/.8 us high pulses and 320 us reset. |
| [dma_mem2mem](../examples/dma_mem2mem/) | esp32s3, esp32c3 | 8192-byte real GDMA SRAM-to-SRAM copy across three descriptors, byte-for-byte verification. |
| [dma_scheduled_io](../examples/dma_scheduled_io/) | esp32s3, esp32c3 | GDMA/SPI single-output 1 MHz scheduled waveform; no arbitrary register scheduling. |
| [dma_spi](../examples/dma_spi/) | esp32s3, esp32c3 | SPI2 mode-0 TX via GDMA, eight bytes, software CS held until SPI TRANS_DONE. |
| [dma_spi_loopback](../examples/dma_spi_loopback/) | esp32s3, esp32c3 | 512-byte full-duplex SPI2/GDMA loopback with two linked TX and RX descriptors. |
| [ds18b20](../examples/ds18b20/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Single externally-powered DS18B20, presence check, conversion, scratchpad CRC and signed temperature; no parasite power. |
| [function_in_ram](../examples/function_in_ram/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | All code resides in SRAM. Demonstrates a noinline RAM function and prints its address. |
| [hc595_demo](../examples/hc595_demo/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | 74HC595 walking LED via software SPI. /OE low and /SRCLR high; no hardware SPI. |
| [hello](../examples/hello/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Startup data/BSS check and periodic ROM console hello. |
| [i2c_bh1750_scan](../examples/i2c_bh1750_scan/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Software I2C scan and BH1750 lux measurements with corrected scaling. |
| [i2c_bh1750_sht31](../examples/i2c_bh1750_sht31/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | BH1750 lux and SHT31 temperature/humidity with CRC checks; software I2C. |
| [i2c_hw_master](../examples/i2c_hw_master/) | esp32, esp32c3 | Polling hardware I2C0 master, 100/400 kHz, probes, FIFO boundaries and repeated START. |
| [i2c_oled](../examples/i2c_oled/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | SSD1306 128x32 graphics, bitmap and scaled-text demo using software I2C and external pullups. |
| [i2c_register_target](../examples/i2c_register_target/) | esp32, esp32c3 | Polling hardware I2C0 target at address 0x42 with a writable 256-byte register window. |
| [pwm_complementary](../examples/pwm_complementary/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC triangle ramp and complementary brightness ramp on GPIO4/5. |
| [pwm_fixed](../examples/pwm_fixed/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | Hardware LEDC 1 kHz / 50% waveform on GPIO4. |
| [pwm_percent_sweep](../examples/pwm_percent_sweep/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC GPIO4 brightness ramp in percent; four fractional duty bits, 8-bit period. |
| [pwm_phase_shift](../examples/pwm_phase_shift/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | Two GPIO4/5 LEDC outputs at 1 kHz, with a duty sweep and 180-degree HPOINT phase offset. |
| [pwm_pin_routing](../examples/pwm_pin_routing/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC output routed through GPIO matrix; PWM_PIN defaults to GPIO5. |
| [pwm_sweep_channels_0_1](../examples/pwm_sweep_channels_0_1/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC channels 0/1 on GPIO4/5; two duty ramps offset by half a ramp cycle. |
| [pwm_sweep_channels_0_3](../examples/pwm_sweep_channels_0_3/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC channels 0/3 on GPIO4/5; two duty ramps offset by half a ramp cycle. |
| [pwm_triangle](../examples/pwm_triangle/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3 | LEDC channel 3 triangle brightness sweep on GPIO4. |
| [random_numbers](../examples/random_numbers/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Deterministic strength-2 LFSR and known sequence; not an entropy source. |
| [rmt_dma_capture](../examples/rmt_dma_capture/) | esp32s3 | RMT channel7/GDMA captures finite pulse-duration blocks at 1 us resolution. |
| [rmt_dma_pwm_input](../examples/rmt_dma_pwm_input/) | esp32s3 | RMT/GDMA PWM frequency and duty capture; GPIO4 test output wired to GPIO5 input. |
| [self_modify_code](../examples/self_modify_code/) | esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | RISC-V CSR instruction changed via SRAM data alias then fence.i; restored afterward. Xtensa has no matching instruction. |
| [spi_24L01_rx](../examples/spi_24L01_rx/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | nRF24L01 pipe-zero fixed 4-byte counter packets, channel 76, auto-ACK, 1 Mbps; software SPI. |
| [spi_24L01_tx](../examples/spi_24L01_tx/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | nRF24L01 pipe-zero fixed 4-byte counter packets, channel 76, auto-ACK, 1 Mbps; software SPI. |
| [spi_lora_sx127x](../examples/spi_lora_sx127x/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | SX1276/77/78/79 bidirectional LoRa counter packets via polling software SPI; default 915 MHz, SF7, BW125, CR4/5 and CRC. |
| [spi_max7219](../examples/spi_max7219/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Single-display animations and Code-B/segment font modes using software SPI. |
| [spi_oled](../examples/spi_oled/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | SSD1306 128x64 Conway life via software SPI, with toroidal boundaries and a xorshift seed. |
| [struct_direct_gpio](../examples/struct_direct_gpio/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Constructed register bitfield values, one full W1TS/W1TC write; three-pin 950/50 ms pattern. |
| [struct_gpio](../examples/struct_gpio/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | GPIO register-bank struct, atomic set/clear writes, GPIO4 blink. |
| [template](../examples/template/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | Empty polling application; startup and main return behavior supplied by the runtime. |
| [uart_dma_echo](../examples/uart_dma_echo/) | esp32, esp32c3 | Finite idle-delimited UART1/UHCI RX DMA followed by DMA echo; gaps between packets. |
| [uart_dma_rx_ring](../examples/uart_dma_rx_ring/) | esp32, esp32c3 | Four-block UHCI/GDMA RX ring with descriptor ownership and explicit overrun stop. |
| [uart_dma_tx](../examples/uart_dma_tx/) | esp32, esp32c3 | Real UART1 TX DMA through UHCI0; raw 115200 8N1, no header or escaping. |
| [uartdemo](../examples/uartdemo/) | esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5 | GPIO4 50/50 ms blink and counter on ROM console, with polling receive echo. |
| [waveshare_c6_lcd](../examples/waveshare_c6_lcd/) | esp32c6 | Waveshare C6 onboard ST7789 color bars and RGB LED, polling SPI/cycle timing; 25% backlight. |
