# aes

C3 hardware AES-128 ECB encryption/decryption using the FIPS-197 known-answer vector; not a cryptographic application.

Build targets for the current implementation: esp32c3.
This list is separate from [boards and scenarios tested](../../docs/validation.md).

From the repository root:

```sh
python tools/esp32fun.py build --chip esp32c3 --project examples/aes
```

Check [example wiring](../../docs/examples.md) and [driver contracts](../../docs/drivers.md)
before connecting hardware. Select pins available on your board.
