/* SPDX-License-Identifier: MIT */
#ifndef FUN_BOARD_H
#define FUN_BOARD_H
/* Example wiring defaults, not a promise that a module exposes these pins.
 * Use -D to override after checking the board schematic.
 */
#include "esp32fun.h"
#ifdef ESP32FUN_ESP32
#define FUN_DEFAULT_SPI_CLOCK 18
#define FUN_DEFAULT_SPI_MOSI 23
#define FUN_DEFAULT_SPI_MISO 19
#define FUN_DEFAULT_SPI_SELECT 5
#define FUN_DEFAULT_DEVICE_DC 26
#define FUN_DEFAULT_DEVICE_RESET 25
#define FUN_DEFAULT_RADIO_CE 27
#define FUN_DEFAULT_I2C_SDA 21
#define FUN_DEFAULT_I2C_SCL 22
#else
#define FUN_DEFAULT_SPI_CLOCK 4
#define FUN_DEFAULT_SPI_MOSI 5
#define FUN_DEFAULT_SPI_MISO 6
#define FUN_DEFAULT_SPI_SELECT 7
#define FUN_DEFAULT_DEVICE_DC 10
#define FUN_DEFAULT_DEVICE_RESET 3
#define FUN_DEFAULT_RADIO_CE 10
#define FUN_DEFAULT_I2C_SDA 2
#define FUN_DEFAULT_I2C_SCL 3
#endif
#ifndef SPI_CLOCK
#define SPI_CLOCK FUN_DEFAULT_SPI_CLOCK
#endif
#ifndef SPI_MOSI
#define SPI_MOSI FUN_DEFAULT_SPI_MOSI
#endif
#ifndef SPI_MISO
#define SPI_MISO FUN_DEFAULT_SPI_MISO
#endif
#ifndef SPI_SELECT
#define SPI_SELECT FUN_DEFAULT_SPI_SELECT
#endif
#ifndef OLED_DC
#define OLED_DC FUN_DEFAULT_DEVICE_DC
#endif
#ifndef OLED_RESET
#define OLED_RESET FUN_DEFAULT_DEVICE_RESET
#endif
#ifndef NRF_CE
#define NRF_CE FUN_DEFAULT_RADIO_CE
#endif
#ifndef I2C_SDA
#define I2C_SDA FUN_DEFAULT_I2C_SDA
#endif
#ifndef I2C_SCL
#define I2C_SCL FUN_DEFAULT_I2C_SCL
#endif
#endif
