# Hardware validation — October 25, 2026

These notes record tests performed, not a list of the only compatible MCUs.
The boards used were a Seeed XIAO ESP32-C3, ESP32-PICO-D4, ESP32-S3 Super Mini
and Waveshare ESP32-C6-LCD-1.47. Digital waveform checks used a Saleae Logic;
board-to-board tests used a separate ESP32 peer and a common ground.

| Function | Tested with | Checks performed and remaining scope |
|---|---|---|
| Runtime and known answers | XIAO C3, PICO-D4, S3 Super Mini | Startup data/BSS, constructors, virtual methods, RAM execution, deterministic LFSR and software CRC. RISC-V instruction modification was tested on C3. |
| GPIO and LEDC examples | XIAO C3, PICO-D4, S3 Super Mini; Saleae | Output timing, full duty sweeps, routed pins, two-channel ramp relationships and HPOINT phase. Pulse-density breathing and SPI-generated LED data were checked electrically; physical LED brightness and strips were not tested. |
| Software I2C | PICO-D4 and S3 Super Mini with C3 peer; Saleae | Payloads, repeated START, ACK/NACK, clock stretching, stuck lines, bounded timeout and recovery. D4 tests used external 4.7 kΩ pullups after internal-pullup failures. Real sensors and displays remain untested. |
| Hardware I2C master/target | C3 and PICO-D4; Saleae, external 4.7 kΩ pullups | Both roles at nominal 100/400 kHz, FIFO boundaries and recovery. Four 1,000-pair repetition runs checked 512,000 payload bytes without mismatches/errors. Analog rise-time margins remain unmeasured. |
| SPI/GDMA and SRAM copy | C3 and S3 Super Mini; Saleae and MOSI–MISO jumper | Finite output, chained loopback and byte-for-byte SRAM copies. S3 tests covered descriptor boundaries, fault/reinitialization and a two-minute finite-transfer soak. External decoding reached nominal 10 MHz; 20 MHz passed physical loopback CRC without independent edge qualification. |
| UART/UHCI DMA | C3 and PICO-D4; peer board and Saleae | TX, idle-delimited RX/echo, RX ring ownership, overrun/error handling and recovery. These checks do not establish indefinite lossless operation. |
| RMT/GDMA pulse capture | S3 Super Mini with C3 pulse source; Saleae | 50 Hz–50 kHz, 10–90% duty, buffer boundaries, finite bursts, real DMA errors, timeout/recovery and a ten-minute repeatedly armed soak. Continuous capture is not implemented. |
| Parallel LCD/GDMA output | S3 Super Mini; Saleae | All eight data pins at nominal 5 MSamples/s, 1,024 divider/payload cases and a ten-minute finite-transfer soak. Tiny descriptors at 10 MHz and analog setup/hold margins remain unqualified. |
| AES block accelerator | XIAO C3 | AES-128 encryption/decryption known-answer test. This is a block demonstration, not a security protocol review. |
| Logical button input | PICO-D4 and S3 Super Mini; peer GPIO | Held-low/released input and expected console characters. Mechanical button bounce remains untested. |
| ADC DMA | C3 build and ROM-image checks | Analog hardware tests were deferred. Known input levels, channel tags, sample cadence, restart and voltage accuracy still need measurement. The included backend has C3-specific registers; other ADC backends require implementation. |
| Onboard LCD and RGB LED | Waveshare ESP32-C6-LCD-1.47 | Color bars and red/green/blue sequence confirmed visually. This is the board-specific example, not validation of other display modules. |
| External device examples | Build and ROM-image checks | 74HC595, SSD1306, MAX7219, BH1750, SHT31, DS18B20, nRF24L01 and SX127x still need their actual devices for application validation. Bus tests alone do not validate them. |

C5 shared-runtime, GPIO and software-bus examples have build/image checks;
physical validation of these examples on C5 remains pending.

Untested revisions, modules, pins and operating conditions remain open for
validation. Build targets are listed in the [example index](examples-index.md).
[Driver contracts](drivers.md) identify actual backend gaps and resource ownership.
Finite transfers with gaps do not demonstrate continuous streaming, and digital
captures do not establish analog accuracy or signal-integrity margins.
