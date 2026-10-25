# self_modify_code

RISC-V CSR instruction changed via SRAM data alias then fence.i; restored afterward. Xtensa has no matching instruction.

Build targets for the current implementation: esp32c2, esp32c3, esp32c6, esp32h2, esp32p4, esp32c5.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32c2 --project examples/self_modify_code
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
