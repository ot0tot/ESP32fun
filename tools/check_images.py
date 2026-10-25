"""Independent structural validation of ESP ROM boot images; no hardware access."""
import hashlib
from pathlib import Path
import struct
from targets import TARGETS

CHIP_IDS = {"esp32": 0, "esp32s2": 2, "esp32s3": 9, "esp32c2": 12,
            "esp32c3": 5, "esp32c6": 13, "esp32c5": 23, "esp32h2": 16, "esp32p4": 18}


def validate_image(path, chip):
    data = Path(path).read_bytes()
    def require(condition, message):
        if not condition: raise ValueError(f"{path}: {message}")
    require(len(data) >= 64, "short image")
    magic, count, mode, speed, entry = struct.unpack_from("<BBBBI", data)
    require(magic == 0xe9 and mode == 2 and 0 < count <= 16, "invalid ROM header")
    require(struct.unpack_from("<H", data, 12)[0] == CHIP_IDS[chip], "wrong chip ID")
    require(data[23] == 1, "missing SHA256 marker")
    target = TARGETS[chip]
    require(struct.unpack_from('<H', data, 15)[0] >= target.get('minimum_chip_revision', 0),
            'unsupported minimum chip revision')
    cursor, checksum, ranges, entry_found = 24, 0xef, [], False
    for _ in range(count):
        require(cursor + 8 <= len(data), "short segment header")
        address, length = struct.unpack_from("<II", data, cursor)
        cursor += 8
        require(length % 4 == 0 and cursor + length <= len(data), "short or unaligned segment")
        iram = target["iram"] <= address and address + length <= target["iram"] + target["iram_length"]
        dram = target["dram"] <= address and address + length <= target["dram"] + target["dram_length"]
        require(iram or dram, "segment outside reserved SRAM")
        if iram:
            entry_found |= address <= entry < address + length
            ranges.append((address - target["alias"], address + length - target["alias"]))
        else: ranges.append((address, address + length))
        for byte in data[cursor:cursor + length]: checksum ^= byte
        cursor += length
    require(entry_found, "entry outside executable segment")
    ranges.sort()
    require(all(a[1] <= b[0] for a, b in zip(ranges, ranges[1:])), "physical SRAM segments overlap")
    checksum_at = cursor + (15 - cursor % 16)
    require(checksum_at < len(data) and data[checksum_at] == checksum, "wrong XOR checksum")
    end = checksum_at + 1
    require(len(data) == end + 32, "wrong appended hash length")
    require(data[end:] == hashlib.sha256(data[:end]).digest(), "wrong SHA256 digest")
    return {"chip": chip, "bytes": len(data), "segments": count, "entry": entry, "flash_header": speed}
