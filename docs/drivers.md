# Driver contracts

The backend column describes code present in this release, not a hardware
compatibility or validation limit. See [validation notes](validation.md) for
boards and scenarios tested. Other chips with the same peripheral may need
register, clock or DMA-routing adaptations before using a driver.

| Driver | Implemented backends | Contract |
|---|---|---|
| GPIO and ROM console | All profiles | Chip pin masks and input/output modes; module reservations remain the application's responsibility. |
| Software SPI/I2C | All profiles | CPU polling. SPI modes 0–3; I2C open-drain, ACK/NACK and bounded stretching. Use external pullups to 3.3 V. |
| LEDC PWM | ESP32/S2/S3/C2/C3 | Duty and HPOINT phase control; remaining gamma-register layouts are absent. |
| Hardware I2C0 | ESP32/C3 | Polling command/FIFO master and register target, nominal 100/400 kHz, repeated START and bounded failure cleanup. No DMA or automatic arbitration recovery. |
| SPI GDMA | C3/S3 | Finite/chained full-duplex transfers through SPI2, internal aligned SRAM buffers, explicit ownership and recovery. |
| UART DMA | ESP32/C3 | UART1/UHCI0; classic integrated UDMA or C3 GDMA. Idle-delimited finite RX and polled rings. Gaps are required for rearming. |
| ADC DMA | C3 | ADC1 raw samples at fixed attenuation; no calibrated millivolts or ADC2 support. |
| Parallel LCD GDMA | S3 | Eight-bit finite output bursts and an explicit clock pin. This is not arbitrary DMA to GPIO MMIO. |
| RMT RX GDMA | S3 | Finite pulse-duration/level blocks, full-buffer or activity/idle termination, explicit errors and timeout. No continuous circular queue. |
| AES block accelerator | C3 | Bounded single-block AES-128 encryption/decryption. Not a complete security protocol. |

All DMA descriptors and buffers must meet the header's internal-SRAM and
alignment checks. Engines are exclusive, not allocated dynamically. Errors
invalidate incomplete data; follow the specific stop/reinitialize contract.
Classic UART RX needs spare final-descriptor capacity and also reserves UART2
during TX abort. The console uses UART0 separately, or explicit native USB on
C3/S3. Default ADC and GPIO pins must be checked against board wiring.

Both I2C stacks return 0 on success, -1 INVALID, -2 TIMEOUT, -3 NACK and -4 BUSY.
Hardware I2C additionally returns -5 ARBITRATION and -6 FIFO_ERROR. Software
I2C does not gain hardware-only capabilities by sharing the error vocabulary.

Use `ram` or `build --ram-image` for S3 RAM loading. Classic clock bookkeeping
uses the configured crystal fallback when ROM metadata is absent. See
[development](development.md), [wiring](examples.md) and [limits](limits.md).

## DMA example wiring

GPIO numbers below are chip pins. Check module reservations before connecting
anything; use a common ground and 3.3 V logic. Do not connect two outputs together.

| Example | Default connections |
|---|---|
| `dma_mem2mem` | C3/S3 internal SRAM copy; no external wiring. |
| `dma_spi` | C3/S3: GPIO4 clock, GPIO5 MOSI, GPIO6 MISO, GPIO7 active-low select. Observe clock/data/select for output tests. |
| `dma_spi_loopback` | Same SPI pins; jumper GPIO5 MOSI to GPIO6 MISO. Remove the jumper before running an example that drives both pins. |
| `dma_scheduled_io`, `dma_gpio_ws2812` | SPI-paced output on GPIO5; GPIO4/7 carry clock/select where used. A real LED strip also needs a suitable supply and compatible data levels. |
| `uart_dma_tx`, `examples/uart_dma_echo`, `uart_dma_rx_ring` | C3/classic: UART1 TX on GPIO4 and RX on GPIO5. Cross TX to peer RX and RX to peer TX; keep console connections separate. |
| `examples/adc_dma` | C3 ADC1 channel0/GPIO0. Use a known voltage inside the chip's 0 dB ADC input range; no digital source may drive the pin. |
| `dma_gpio`, `dma_gpio_mem2mem` | S3: eight data outputs GPIO8–15 and clock GPIO16. `LCD_DATA_FIRST` and `LCD_CLOCK_PIN` can remap the bus; the clock must not overlap the data pins. |
| `rmt_dma_capture`, `rmt_dma_pwm_input` | S3: generated PWM on GPIO4, RMT input GPIO5. Fit GPIO4→GPIO5 for loopback. Remove that jumper before using an independent source on GPIO5. |

The GPIO/DMA examples use finite transfers. Their source describes the exact
payload, buffer size and rearm interval; these are not continuous stream APIs.
