# uart_dma_tx

Real UART1 TX DMA through UHCI0; raw 115200 8N1, no header or escaping.

Build targets for the current implementation: esp32, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/uart_dma_tx
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
