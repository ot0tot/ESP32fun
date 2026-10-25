"""Committed, SDK-independent ESP32 target metadata and linker generation."""
import json
from pathlib import Path

TARGET_DIRECTORY = Path(__file__).resolve().parent.parent / "esp32fun" / "targets"
TARGETS = json.loads((TARGET_DIRECTORY / "profiles.json").read_text(encoding="utf-8"))
DEFAULT_TARGET = "esp32c3"


def linker_script(chip, ram_boot=False):
    t = dict(TARGETS[chip])
    if ram_boot and chip == 'esp32s3':
        # ROM download overwrites the lower flash-boot window near 403c8fe0.
        # Keep RAM-loaded code above that observed workspace; preserve the
        # original end boundary and every data/stack reservation.
        end = t['iram'] + t['iram_length']
        t['iram'] = 0x403c9000
        t['iram_length'] = end - t['iram']
    checks = ""
    if t["alias"] or t["architecture"] == "riscv":
        checks = f'ASSERT(ORIGIN(iram) + LENGTH(iram) - {t["alias"]:#x} <= ORIGIN(dram), "SRAM aliases overlap");'
    rom = "\n".join(f"PROVIDE({symbol} = {address:#x});" for symbol, address in t["rom_symbols"].items())
    # Pinned Xtensa GCC emits .ctors, RISC-V GCC emits .init_array. Neither
    # toolchain has crtbegin/crtend linked here, so both contain plain pointers.
    constructors = ('.ctors' if t['architecture'] == 'xtensa' else '.init_array')
    return f'''/* Generated from committed profiles.json; no SDK required. */
ENTRY(_start)
MEMORY {{
  iram (rx) : ORIGIN = {t['iram']:#x}, LENGTH = {t['iram_length']:#x}
  dram (rw) : ORIGIN = {t['dram']:#x}, LENGTH = {t['dram_length']:#x}
  stack (rw) : ORIGIN = {t['stack']:#x}, LENGTH = {t['stack_length']:#x}
}}
SECTIONS {{
  .text : ALIGN(4) {{
    KEEP(*(.literal.start .text.start))
    *(.literal .literal.* .text .text.*)
    . = ALIGN(4); . += 16;
  }} > iram
  .data : ALIGN(4) {{
    __global_pointer$ = . + 0x800;
    *(.sdata .sdata.* .data .data.* .srodata .srodata.* .rodata .rodata.*)
    . = ALIGN(4);
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY({constructors}.*))) KEEP(*({constructors}))
    __init_array_end = .;
    . = ALIGN(4);
  }} > dram
  .bss (NOLOAD) : ALIGN(4) {{
    __bss_start = .;
    *(.sbss .sbss.* .bss .bss.* COMMON)
    . = ALIGN(4); __bss_end = .;
  }} > dram
  __stack_bottom = ORIGIN(stack);
  __stack_top = ORIGIN(stack) + LENGTH(stack);
  /DISCARD/ : {{ *(.eh_frame .eh_frame.* .comment .note.GNU-stack) }}
}}
{checks}
ASSERT(__bss_end <= __stack_bottom, "Static data overlaps stack");
{rom}
'''
