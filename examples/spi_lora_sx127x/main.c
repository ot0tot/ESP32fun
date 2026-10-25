/* SPDX-License-Identifier: MIT */
#include "fun_bus.h"
#ifndef LORA_FREQUENCY
#define LORA_FREQUENCY 915000000u
#endif
#ifndef LORA_RESET
#define LORA_RESET FUN_DEFAULT_DEVICE_RESET
#endif
static const fun_spi radio = {SPI_CLOCK, SPI_MOSI, SPI_MISO, SPI_SELECT, 1, 0};
static uint8_t access_register(uint8_t reg, uint8_t value)
{
    fun_digital_write(SPI_SELECT, 0);
    fun_spi_transfer(&radio, reg);
    uint8_t result = fun_spi_transfer(&radio, value);
    fun_digital_write(SPI_SELECT, 1);
    return result;
}
static uint8_t read_register(uint8_t reg) { return access_register(reg & 0x7fu, 0); }
static void write_register(uint8_t reg, uint8_t value) { access_register(reg | 0x80u, value); }
static void set_mode(uint8_t mode) { write_register(0x01, 0x80u | (LORA_FREQUENCY < 525000000u ? 8u : 0u) | mode); }
static void receive_mode(void) { set_mode(5); } /* RX continuous. */
int main(void)
{
    if (fun_spi_init(&radio) || fun_pin_mode(LORA_RESET, FUN_OUTPUT)) return 1;
    Delay_Ms(10); fun_digital_write(LORA_RESET, 1); Delay_Ms(10);
    if (read_register(0x42) != 0x12) { fun_puts("SX1276/77/78/79 not detected"); return 1; }
    set_mode(0); /* Sleep before selecting LoRa frequency. */
    uint32_t frequency = (uint32_t)(((uint64_t)LORA_FREQUENCY << 19) / 32000000u);
    write_register(0x06, (uint8_t)(frequency >> 16));
    write_register(0x07, (uint8_t)(frequency >> 8));
    write_register(0x08, (uint8_t)frequency);
    write_register(0x0e, 0); write_register(0x0f, 0);
    write_register(0x1d, 0x72); /* 125 kHz, CR 4/5, explicit header. */
    write_register(0x1e, 0x74); /* SF7, payload CRC. */
    write_register(0x26, 0x04); /* AGC enabled. */
    write_register(0x0c, read_register(0x0c) | 3u);
    write_register(0x09, 0x8f); /* PA_BOOST 17 dBm; no +20 dBm mode. */
    write_register(0x39, 0x12); /* Private LoRa sync word. */
    write_register(0x12, 0xff);
    receive_mode();
    uint32_t elapsed_ms = 0, counter = 0;
    for (;;) {
        uint8_t irq = read_register(0x12);
        if (irq & 0x40u) {
            unsigned length = read_register(0x13);
            write_register(0x0d, read_register(0x10));
            if (!(irq & 0x20u)) {
                int rssi = read_register(0x1a) - (LORA_FREQUENCY < 525000000u ? 157 : 164);
                int snr_quarters = (int8_t)read_register(0x19);
                fun_printf("RX %u bytes RSSI=%d dBm SNR=%d quarter-dB: ", length, rssi, snr_quarters);
                for (unsigned i = 0; i < length; ++i) fun_printf("%02x", (unsigned)read_register(0));
                fun_puts("");
            } else fun_puts("LoRa payload CRC error");
            write_register(0x12, irq);
        }
        if (++elapsed_ms >= 3000) {
            elapsed_ms = 0;
            uint8_t message[4] = {(uint8_t)counter, (uint8_t)(counter >> 8), (uint8_t)(counter >> 16), (uint8_t)(counter >> 24)};
            set_mode(1); write_register(0x12, 0xff);
            write_register(0x0d, 0);
            for (unsigned i = 0; i < sizeof(message); ++i) write_register(0, message[i]);
            write_register(0x22, sizeof(message)); set_mode(3);
            unsigned waited;
            for (waited = 0; waited < 2000 && !(read_register(0x12) & 8u); ++waited) Delay_Ms(1);
            fun_printf("TX %u %s\n", (unsigned)counter++, waited < 2000 ? "done" : "timeout");
            write_register(0x12, 0xff); receive_mode();
        }
        Delay_Ms(1);
    }
}
