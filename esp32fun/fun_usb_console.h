/*
 * SPDX-FileCopyrightText: 2010-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Adapted printf-enable workaround from ESP-IDF v5.3.1
 * components/esp_rom/patches/esp_rom_sys.c. Changes: C3/S3 native USB callback
 * selection; no SDK dependency. Full license: ../licenses/Apache-2.0.txt.
 */
#ifndef FUN_USB_CONSOLE_H
#define FUN_USB_CONSOLE_H

extern volatile uint8_t fun_rom_uart_print_enabled;
extern volatile uint8_t fun_rom_usb_print_enabled;
extern void fun_rom_install_putc1(void (*callback)(char));
extern void fun_rom_install_putc2(void (*callback)(char));

static inline void fun_usb_console_install(void (*callback)(char))
{
    /* ROM download mode can leave printf disabled independently of callbacks. */
    fun_rom_uart_print_enabled = 1;
    fun_rom_usb_print_enabled = 1;
    fun_rom_install_putc1(callback);
    fun_rom_install_putc2(0);
}
#endif
