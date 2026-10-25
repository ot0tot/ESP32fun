# uart_dma_rx_ring

Four-block UHCI/GDMA RX ring with descriptor ownership and explicit overrun stop.

Build targets for the current implementation: esp32, esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32 --project examples/uart_dma_rx_ring
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
