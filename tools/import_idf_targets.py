#!/usr/bin/env python3
"""Regenerate numeric chip profiles from a local Espressif reference checkout.

The reference is only needed to regenerate profiles, never to build firmware.
No SDK implementation or vendor headers are copied into the firmware.
"""
import argparse
import ast
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
# Conservative bootloader ranges; reserve the final 8 KiB for our own stack.
LAYOUTS = {
    "esp32": (0x40080400, 0xfc00, 0x3fff0000, 0x4000, 0x3fff4000, 0),
    "esp32s2": (0x4004a000, 0xb000, 0x3ffe5000, 0x3b00, 0x3ffe8b00, 0x70000),
    "esp32s3": (0x403c8700, 0xa000, 0x3fce2700, 0x5000, 0x3fce7700, 0x6f0000),
    "esp32c2": (0x403acb70, 0x9000, 0x3fcd5b70, 0x5000, 0x3fcdab70, 0x6e0000),
    "esp32c3": (0x403cc710, 0x9000, 0x3fcd5710, 0x5000, 0x3fcda710, 0x700000),
    "esp32c6": (0x4086c110, 0x9500, 0x40875610, 0x5000, 0x4087a610, 0),
    # C5 CPU-only window, below ROM download buffers and the RF dump bank.
    "esp32c5": (0x40810000, 0x9000, 0x40819000, 0x5000, 0x4081e000, 0),
    "esp32h2": (0x4083cad0, 0x9500, 0x40845fd0, 0x5000, 0x4084afd0, 0),
    "esp32p4": (0x4ff2abd0, 0x9000, 0x4ff33bd0, 0x5000, 0x4ff38bd0, 0),
}


class Macros:
    def __init__(self, files):
        self.values = {f"BIT{n}": str(1 << n) for n in range(64)}
        for file in files:
            source = file.read_text(encoding="utf-8").replace("\\\n", " ")
            for name, expr in re.findall(r"^#define\s+(\w+)[ \t]+(.+)$", source, re.M):
                self.values[name] = expr.split("//")[0].split("/*")[0].strip()

    def get(self, name, default=None):
        if name not in self.values:
            if default is not None:
                return default
            raise KeyError(name)
        return self.evaluate(self.values[name])

    def first(self, *names):
        for name in names:
            if name in self.values:
                return self.get(name)
        raise KeyError(' or '.join(names))

    def evaluate(self, expr):
        expr = re.sub(r"\b(0x[0-9a-fA-F]+|[0-9]+)[uUlL]+\b", r"\1", expr)
        node = ast.parse(expr, mode="eval").body

        def calc(node):
            if isinstance(node, ast.Constant) and isinstance(node.value, int): return node.value
            if isinstance(node, ast.Name): return self.get(node.id)
            if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Invert): return ~calc(node.operand)
            if isinstance(node, ast.BinOp):
                a, b = calc(node.left), calc(node.right)
                operations = {ast.Add: lambda: a+b, ast.Sub: lambda: a-b,
                              ast.Mult: lambda: a*b, ast.LShift: lambda: a << b,
                              ast.RShift: lambda: a >> b, ast.BitOr: lambda: a | b,
                              ast.BitAnd: lambda: a & b}
                if type(node.op) in operations: return operations[type(node.op)]()
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
                if node.func.id in {"BIT", "BIT64"}: return 1 << calc(node.args[0])
            raise ValueError(f"Unsupported macro expression: {expr}")
        return calc(node)


def generate(idf, chips=None):
    directory = ROOT / "esp32fun" / "targets"
    directory.mkdir(exist_ok=True)
    profiles = directory / 'profiles.json'
    targets = json.loads(profiles.read_text()) if profiles.is_file() else {}
    version_source = (idf / 'tools/cmake/version.cmake').read_text()
    version = '.'.join(re.search(r'set\(IDF_VERSION_' + p + r'\s+(\d+)\)', version_source)[1]
                       for p in ('MAJOR', 'MINOR', 'PATCH'))
    selected = chips or [c for c in LAYOUTS if (idf / 'components/soc' / c).is_dir()]
    for chip in selected:
        layout = LAYOUTS[chip]
        headers = idf / "components/soc" / chip / "include/soc"
        macros = Macros([*headers.glob("*.h"),
                         *(idf / 'components/soc' / chip / 'register/soc').glob('*.h'),
                         *(idf / 'components/esp_hal_timg' / chip / 'include/hal').glob('timg_ll.h')])
        rom = {}
        rom_files = idf / "components/esp_rom" / chip / "ld"
        for filename in [chip + ".rom.ld", chip + ".rom.newlib.ld", chip + ".rom.newlib-funcs.ld",
                         chip + '.rom.libc.ld', chip + '.rom.libc-suboptimal_for_misaligned_mem.ld']:
            file = rom_files / filename
            if not file.is_file(): continue
            for symbol, address in re.findall(r"\b(\w+)\s*=\s*(0x[0-9a-fA-F]+)\s*[;)]",
                                               file.read_text(encoding="utf-8")):
                rom[symbol] = int(address, 16)
        iram, iram_len, dram, dram_len, stack, alias = layout
        xtensa = chip in {"esp32", "esp32s2", "esp32s3"}
        t = dict(architecture="xtensa" if xtensa else "riscv",
                 compiler=f"xtensa-{chip}-elf-gcc" if xtensa else "riscv32-esp-elf-gcc",
                 flash_offset=0x2000 if chip == 'esp32c5' else 0x1000 if chip in {"esp32", "esp32s2"} else 0,
                 flash_frequency="30m" if chip == "esp32c2" else "24m" if chip == "esp32h2" else "40m",
                 iram=iram, iram_length=iram_len, dram=dram, dram_length=dram_len,
                 stack=stack, stack_length=0x2000, alias=alias,
                 reference=f"ESP-IDF v{version} register and ROM reference",
                 hardware_verified=False)
        count = macros.get("SOC_GPIO_PIN_COUNT")
        valid = macros.get("SOC_GPIO_VALID_GPIO_MASK")
        outputs = macros.get("SOC_GPIO_VALID_OUTPUT_GPIO_MASK")
        mux = []
        for pin in range(count):
            mux.append(macros.get(f"IO_MUX_GPIO{pin}_REG", 0) if valid & (1 << pin) else 0)
        defs = {
            "FUN_TARGET_NAME": json.dumps(chip),
            "FUN_ARCH_XTENSA": int(xtensa),
            "FUN_CYCLE_CSR": 0x7e2 if macros.get("SOC_CPU_HAS_CSR_PC", 0) else 0xb00,
            "FUN_IRAM_ALIAS": alias,
            "FUN_GPIO_COUNT": count,
            "FUN_GPIO_BASE": macros.get("DR_REG_GPIO_BASE"),
            "FUN_GPIO_VALID_LOW": valid & 0xffffffff,
            "FUN_GPIO_VALID_HIGH": valid >> 32,
            "FUN_GPIO_OUTPUT_LOW": outputs & 0xffffffff,
            "FUN_GPIO_OUTPUT_HIGH": outputs >> 32,
            "FUN_GPIO_PIN_OFFSET": macros.get("GPIO_PIN0_REG") - macros.get("DR_REG_GPIO_BASE"),
            "FUN_GPIO_SELECT_OFFSET": macros.get("GPIO_FUNC0_OUT_SEL_CFG_REG") - macros.get("DR_REG_GPIO_BASE"),
            "FUN_GPIO_SIGNAL": macros.get("SIG_GPIO_OUT_IDX"),
            "FUN_GPIO_ENABLE_FROM_REG": macros.first("GPIO_FUNC0_OEN_SEL", "GPIO_FUNC0_OE_SEL"),
            "FUN_IO_FUNCTION_GPIO": macros.get("PIN_FUNC_GPIO") << macros.get("MCU_SEL_S"),
            "FUN_IO_FUNCTION_MASK": macros.get("MCU_SEL_M"),
            "FUN_IO_PULLUP": macros.get("FUN_PU"),
            "FUN_IO_PULLDOWN": macros.get("FUN_PD"),
            "FUN_IO_INPUT_ENABLE": macros.get("FUN_IE"),
            "FUN_GPIO_CLOCK_GATE_ADDRESS": macros.get("GPIO_CLOCK_GATE_REG", 0),
            "FUN_TIMER_BASE": macros.first("DR_REG_TIMERGROUP0_BASE", "DR_REG_TIMERG0_BASE"),
            "FUN_TIMER_GROUPS": macros.first("SOC_TIMER_GROUPS", "TIMG_LL_INST_NUM"),
            "FUN_TIMER_WDT_CONFIG_OFFSET": 0x48,
            "FUN_TIMER_WDT_PROTECT_OFFSET": 0x64,
            "FUN_TIMER_WDT_FLASH_MASK": macros.get("TIMG_WDT_FLASHBOOT_MOD_EN"),
        }
        if chip == 'esp32c5':
            t['minimum_chip_revision'] = 100  # v1.0 / ECO2 production ROM table
            t['hardware_scope'] = 'Build/image checks only; no board execution or RF initialization'
            for short, name in {'ENABLE_SET_OFFSET': 'GPIO_ENABLE_W1TS_REG',
                                'ENABLE_SET_HIGH_OFFSET': 'GPIO_ENABLE1_W1TS_REG',
                                'IN_LOW_OFFSET': 'GPIO_IN_REG', 'IN_HIGH_OFFSET': 'GPIO_IN1_REG'}.items():
                defs['FUN_GPIO_' + short] = macros.get(name) - macros.get('DR_REG_GPIO_BASE')
        # This small driver supports the original LEDC duty/fade register layout.
        # C6/H2/P4 use a different gamma engine and are intentionally excluded.
        pwm = chip in {"esp32", "esp32s2", "esp32s3", "esp32c2", "esp32c3"}
        defs["FUN_PWM_SUPPORTED"] = int(pwm)
        defs["FUN_AES_SUPPORTED"] = int(chip == "esp32c3")
        if chip == "esp32c3":
            for short, macro in {
                "BASE": "DR_REG_AES_BASE", "MODE": "AES_MODE_REG", "TRIGGER": "AES_TRIGGER_REG",
                "STATE": "AES_STATE_REG", "DMA_ENABLE": "AES_DMA_ENABLE_REG",
                "INPUT": "AES_TEXT_IN_BASE", "OUTPUT": "AES_TEXT_OUT_BASE",
                "BUS_CLOCK": "SYSTEM_PERIP_CLK_EN1_REG", "BUS_ENABLE": "SYSTEM_CRYPTO_AES_CLK_EN",
                "BUS_RESET": "SYSTEM_PERIP_RST_EN1_REG", "RESET_MASK": "SYSTEM_CRYPTO_AES_RST",
                "DS_RESET_MASK": "SYSTEM_CRYPTO_DS_RST",
            }.items():
                defs["FUN_AES_" + short] = macros.get(macro)
        if pwm:
            def first(*names):
                return next(macros.get(n) for n in names if n in macros.values)
            defs.update({
                "FUN_PWM_CHANNELS": macros.get("SOC_LEDC_CHANNEL_NUM"),
                "FUN_PWM_BITS": macros.get("SOC_LEDC_TIMER_BIT_WIDTH"),
                "FUN_PWM_CHANNEL_BASE": first("LEDC_LSCH0_CONF0_REG", "LEDC_CH0_CONF0_REG"),
                "FUN_PWM_CHANNEL_STRIDE": first("LEDC_LSCH1_CONF0_REG", "LEDC_CH1_CONF0_REG") - first("LEDC_LSCH0_CONF0_REG", "LEDC_CH0_CONF0_REG"),
                "FUN_PWM_TIMER_BASE": first("LEDC_LSTIMER0_CONF_REG", "LEDC_TIMER0_CONF_REG"),
                "FUN_PWM_TIMER_UPDATE": first("LEDC_LSTIMER0_PARA_UP", "LEDC_TIMER0_PARA_UP"),
                "FUN_PWM_TIMER_RESET": first("LEDC_LSTIMER0_RST", "LEDC_TIMER0_RST"),
                # Classic ESP32 selects APB with tick_sel=1. On S2, zero selects
                # the global source (one selects REF_TICK); C2/C3/S3 leave zero.
                "FUN_PWM_TIMER_GLOBAL": first("LEDC_TICK_SEL_LSTIMER0", "LEDC_TICK_SEL_TIMER0") if chip == "esp32" else 0,
                "FUN_PWM_DIV_SHIFT": first("LEDC_CLK_DIV_LSTIMER0_S", "LEDC_CLK_DIV_TIMER0_S", "LEDC_DIV_NUM_LSTIMER0_S"),
                "FUN_PWM_DIV_MAX": first("LEDC_CLK_DIV_LSTIMER0_V", "LEDC_CLK_DIV_TIMER0_V", "LEDC_DIV_NUM_LSTIMER0_V"),
                "FUN_PWM_CONF": macros.get("LEDC_CONF_REG"),
                "FUN_PWM_CONF_CLOCK": macros.get("LEDC_CLK_EN", 0),
                "FUN_PWM_CLOCK_SELECT_MASK": first("LEDC_APB_CLK_SEL_M", "LEDC_CLK_SEL_M"),
                "FUN_PWM_CLOCK_SELECT": (1 if chip == "esp32" else 3) << first("LEDC_APB_CLK_SEL_S", "LEDC_CLK_SEL_S"),
                "FUN_PWM_SIGNAL": macros.get("LEDC_LS_SIG_OUT0_IDX"),
                "FUN_PWM_BUS_CLOCK": first("SYSTEM_PERIP_CLK_EN0_REG", "DPORT_PERIP_CLK_EN_REG"),
                "FUN_PWM_BUS_ENABLE": first("SYSTEM_LEDC_CLK_EN", "DPORT_LEDC_CLK_EN"),
                "FUN_PWM_BUS_RESET": first("SYSTEM_PERIP_RST_EN0_REG", "DPORT_PERIP_RST_EN_REG"),
                "FUN_PWM_BUS_RESET_MASK": first("SYSTEM_LEDC_RST", "DPORT_LEDC_RST"),
            })
        if "DR_REG_LP_WDT_BASE" in macros.values:
            defs.update(FUN_RTC_WDT_CONFIG=macros.get("LP_WDT_CONFIG0_REG"),
                        FUN_RTC_WDT_PROTECT=macros.get("LP_WDT_WPROTECT_REG"),
                        FUN_RTC_WDT_FLASH_MASK=macros.get("LP_WDT_WDT_FLASHBOOT_MOD_EN"),
                        FUN_SWD_CONFIG=macros.get("LP_WDT_SWD_CONFIG_REG"),
                        FUN_SWD_PROTECT=macros.get("LP_WDT_SWD_WPROTECT_REG"),
                        FUN_SWD_AUTO_FEED_MASK=macros.get("LP_WDT_SWD_AUTO_FEED_EN"))
        else:
            defs.update(FUN_RTC_WDT_CONFIG=macros.get("RTC_CNTL_WDTCONFIG0_REG"),
                        FUN_RTC_WDT_PROTECT=macros.get("RTC_CNTL_WDTWPROTECT_REG"),
                        FUN_RTC_WDT_FLASH_MASK=macros.get("RTC_CNTL_WDT_FLASHBOOT_MOD_EN"),
                        FUN_SWD_CONFIG=macros.get("RTC_CNTL_SWD_CONF_REG", 0),
                        FUN_SWD_PROTECT=macros.get("RTC_CNTL_SWD_WPROTECT_REG", 0),
                        FUN_SWD_AUTO_FEED_MASK=macros.get("RTC_CNTL_SWD_AUTO_FEED_EN", 0))
        lines = ["/* Numeric hardware facts; generated by tools/import_idf_targets.py. */",
                 "/* SPDX-License-Identifier: MIT */", f"#define ESP32FUN_{chip.upper()} 1"]
        lines += [f"#define {key} " + (value if isinstance(value, str) else hex(value) + "u")
                  for key, value in defs.items()]
        lines.append("#define FUN_PIN_MUX_ADDRESSES { " + ", ".join(hex(a) + "u" for a in mux) + " }")
        (directory / (chip + ".h")).write_text("\n".join(lines) + "\n", encoding="utf-8")
        t["gpio_count"] = count
        t["gpio_valid_mask"] = valid
        t["gpio_output_mask"] = outputs
        symbols = {"fun_printf":"ets_printf", "Delay_Us":"ets_delay_us",
                   "fun_putchar":"uart_tx_one_char", "fun_getchar":"uart_rx_one_char",
                   "fun_cpu_mhz":"ets_get_cpu_frequency", "fun_rom_set_cpu_mhz":
                   "ets_update_cpu_frequency_rom" if chip == "esp32" else "ets_update_cpu_frequency",
                   "fun_reset":"software_reset", "fun_reset_reason":"rtc_get_reset_reason",
                   "memset":"memset", "memcpy":"memcpy", "memmove":"memmove", "memcmp":"memcmp"}
        t["rom_symbols"] = {name:rom[source] for name, source in symbols.items()}
        if chip == "esp32c6":
            t["rom_symbols"]["fun_rom_cpu_mhz"] = rom["ets_get_cpu_frequency"]
        if chip == "esp32c3":
            t["rom_symbols"].update(fun_usb_putchar=rom["usb_uart_tx_one_char"],
                                    fun_usb_getchar=rom["usb_uart_rx_one_char"],
                                    fun_usb_flush=rom["usb_uart_tx_flush"],
                                    fun_rom_install_putc1=rom["ets_install_putc1"],
                                    fun_rom_install_putc2=rom["ets_install_putc2"],
                                    fun_rom_uart_print_enabled=rom["g_uart_print"],
                                    fun_rom_usb_print_enabled=rom["g_usb_print"])
        if chip == "esp32s3":
            t["rom_symbols"].update(fun_usb_putchar=rom["usb_uart_device_tx_one_char"],
                                    fun_usb_getchar=rom["usb_uart_device_rx_one_char"],
                                    fun_usb_flush=rom["usb_uart_device_tx_flush"],
                                    fun_rom_install_putc1=rom["ets_install_putc1"],
                                    fun_rom_install_putc2=rom["ets_install_putc2"],
                                    fun_rom_uart_print_enabled=rom["g_uart_print"],
                                    fun_rom_usb_print_enabled=rom["g_usb_print"])
        targets[chip] = t
    (directory / "profiles.json").write_text(json.dumps(targets, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--idf", type=Path, required=True)
    parser.add_argument('--chip', action='append', choices=sorted(LAYOUTS))
    args = parser.parse_args()
    generate(args.idf, args.chip)
