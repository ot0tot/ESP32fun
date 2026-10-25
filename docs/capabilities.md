# Driver and chip capabilities

Generated from `profiles.json` and the actual source header gates by
`python tools/capabilities.py --write`. **Implemented** means the driver
header accepts that target. **Backend needed** means chip-specific code
is missing; it is not a claim that the silicon lacks the peripheral.
Neither result describes hardware validation. Tested boards and scenarios
are recorded separately in [validation notes](validation.md).

| Driver | esp32 | esp32s2 | esp32s3 | esp32c2 | esp32c3 | esp32c6 | esp32h2 | esp32p4 | esp32c5 |
|---|---|---|---|---|---|---|---|---|---|
| GPIO, delay, ROM console | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented |
| Software SPI/I2C | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented | Implemented |
| LEDC PWM | Implemented | Implemented | Implemented | Implemented | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |
| Hardware I2C | Implemented | Backend needed | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |
| SPI GDMA | Backend needed | Backend needed | Implemented | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |
| UART UHCI DMA | Implemented | Backend needed | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |
| ADC DMA | Backend needed | Backend needed | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |
| Parallel LCD GDMA | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed | Backend needed | Backend needed |
| RMT RX GDMA | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed | Backend needed | Backend needed |
| AES block accelerator | Backend needed | Backend needed | Backend needed | Backend needed | Implemented | Backend needed | Backend needed | Backend needed | Backend needed |

Source support is distinct from physical qualification.
Core profiles reserve 8 KiB of stack. GPIO reservations and console
routing depend on the chip and board. DMA drivers exclusively own
their channels and peripherals; simultaneous operation is not implied.
See [driver contracts](drivers.md) and [limits](limits.md).
