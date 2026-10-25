# Runtime and build design

The mask ROM loads a checksummed image into fixed instruction/data SRAM.
Assembly startup establishes a stack and the selected ABI, masks ordinary
interrupts and enters a small C runtime. The runtime initializes BSS and C++
constructors, disables boot watchdog modes and calls `main`. It provides no
heap, scheduler, second-core startup or interrupt dispatcher.

`esp32fun/targets/profiles.json` and generated target headers carry numeric
GPIO, watchdog, memory and ROM facts. The linker reserves an 8 KiB stack and
asserts that static data and physical SRAM aliases do not overlap. Stack
reservation is not runtime overflow protection. ROM console functions provide
limited printing and input; they are not a complete libc.

The Python frontend compiles freestanding C11/C++11 with section garbage
collection, then uses esptool to form a ROM image. GNU Make delegates to the
same frontend. Flashing is explicit; image validation and normal builds do
not access serial devices. Setup downloads only pinned host tools.

Peripheral implementations use explicit chip gates, bounded polling, fixed
buffers and documented ownership. [The capability table](capabilities.md)
describes source acceptance; [driver contracts](drivers.md) describe behavior.
Software and hardware I2C share error values. UART backends share raw UART
error masks while retaining their different DMA engines.

Primary references are [ESP-IDF v5.3.1 SoC headers](https://github.com/espressif/esp-idf/tree/v5.3.1/components/soc),
[ROM symbols](https://github.com/espressif/esp-idf/tree/v5.3.1/components/esp_rom)
and [boot memory layouts](https://github.com/espressif/esp-idf/tree/v5.3.1/components/bootloader/subproject/main/ld).
These supply numeric facts; firmware does not link ESP-IDF. Adapted source
retains its own licenses and pinned [provenance](upstream-files.json).
