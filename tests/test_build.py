"""Cross-compiler/image checks; no serial ports or hardware are touched."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from esp32fun import find_compiler, find_python, sibling_tool, output_directory


class BuildChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Fail clearly when the environment is incomplete, rather than skip.
        find_compiler()
        cls.python = find_python()
        (ROOT / "build").mkdir(exist_ok=True)

    def cli(self, *args, expected=0, cwd=ROOT, env=None):
        result = subprocess.run([self.python, str(ROOT / "tools" / "esp32fun.py"), *map(str, args)],
                                cwd=cwd, env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def check_image(self, path):
        image = path.read_bytes()
        magic, count, mode, speed_size, entry = struct.unpack_from("<BBBBI", image)
        self.assertEqual(magic, 0xe9)
        self.assertEqual(mode, 2)  # DIO
        self.assertEqual(speed_size, 0x20)  # 4 MiB / 40 MHz
        self.assertEqual(struct.unpack_from("<H", image, 12)[0], 5)  # C3 chip ID
        self.assertEqual(image[23], 1)  # SHA-256 appended
        offset, checksum, physical_ranges, segments = 24, 0xef, [], []
        for _ in range(count):
            address, length = struct.unpack_from("<II", image, offset)
            offset += 8
            data = image[offset:offset + length]
            self.assertEqual(len(data), length)
            self.assertEqual(length % 4, 0)
            offset += length
            for byte in data:
                checksum ^= byte
            if 0x403cc710 <= address < 0x403d5710:
                self.assertLessEqual(address + length, 0x403d5710)
                physical_ranges.append((address - 0x700000, address + length - 0x700000))
            else:
                self.assertGreaterEqual(address, 0x3fcd5710)
                self.assertLessEqual(address + length, 0x3fcda710)
                physical_ranges.append((address, address + length))
            segments.append((address, data))
        self.assertTrue(any(address <= entry < address + len(data)
                            for address, data in segments if address >= 0x403cc710))
        ranges = sorted(physical_ranges)
        for left, right in zip(ranges, ranges[1:]):
            self.assertLessEqual(left[1], right[0], "SRAM aliases overlap")
        checksum_offset = offset + (15 - offset % 16)
        self.assertEqual(image[checksum_offset], checksum)
        hashed_end = checksum_offset + 1
        self.assertEqual(len(image), hashed_end + 32)
        self.assertEqual(image[hashed_end:], hashlib.sha256(image[:hashed_end]).digest())
        return segments

    def test_examples_are_valid_rom_images(self):
        for name in ("blink", "hello"):
            with self.subTest(example=name):
                project = ROOT / "examples" / name
                self.cli("build", "--project", project)
                self.check_image(project / "build" / "firmware.bin")
                commands = json.loads((project / "build" / "compile_commands.json").read_text())
                self.assertEqual(len(commands), 3)

    def test_constructor_table_retains_priority_on_both_architectures(self):
        from esptool.bin_image import LoadFirmwareImage
        project = ROOT / 'examples/constructors'
        for chip in ('esp32', 'esp32c3'):
            with self.subTest(chip=chip):
                self.cli('build', '--project', project, '--chip', chip)
                out = output_directory(project, chip)
                listing = subprocess.check_output([str(sibling_tool(find_compiler(chip=chip), 'nm')),
                                                   str(out / 'firmware.elf')], text=True)
                symbols = {fields[2]: int(fields[0], 16) for line in listing.splitlines()
                           if len(fields := line.split()) == 3}
                start, end = symbols['__init_array_start'], symbols['__init_array_end']
                self.assertEqual(end - start, 8, 'both constructors must survive linker GC')
                image = LoadFirmwareImage(chip, str(out / 'firmware.bin'))
                segment = next(s for s in image.segments if s.addr <= start and end <= s.addr + len(s.data))
                self.assertEqual(struct.unpack_from('<II', segment.data, start - segment.addr),
                                 (symbols['first'], symbols['second']), 'priority 101 must precede 102')

    def test_project_with_spaces_and_initialized_data(self):
        with tempfile.TemporaryDirectory(prefix="project with spaces ", dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.c").write_text(
                '#include "esp32fun.h"\n'
                'volatile uint32_t initialized = 0x89abcdef;\n'
                'volatile uint32_t zeros[32];\n'
                'int main(void) { zeros[0] = initialized; fun_printf("%u", (unsigned)zeros[0]); return 0; }\n')
            # No --project: the CLI recognizes main.c in the working directory.
            self.cli("build", cwd=project)
            segments = self.check_image(project / "build" / "firmware.bin")
            self.assertTrue(any(b"\xef\xcd\xab\x89" in data for _, data in segments))
            self.cli("clean", cwd=project)
            self.assertFalse((project / "build").exists())
            self.assertTrue((project / "main.c").exists())

    def test_static_data_overflow_fails_to_link(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.c").write_text(
                'volatile unsigned char too_large[0x6000];\n'
                'int main(void) { return too_large[0]; }\n')
            result = self.cli("build", "--project", project, expected=1)
            self.assertIn("overflow", result.stderr.lower())
            self.assertFalse((project / "build" / "firmware.bin").exists())

    def test_clean_c3_preserves_other_target_outputs(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.c").write_text('int main(void) { return 0; }\n')
            (project / "build/esp32s3").mkdir(parents=True)
            (project / "build/firmware.bin").write_bytes(b'C3')
            other = project / "build/esp32s3/firmware.bin"
            other.write_bytes(b'S3')
            self.cli("clean", "--project", project)
            self.assertFalse((project / "build/firmware.bin").exists())
            self.assertEqual(other.read_bytes(), b'S3')

    def test_cpp_project_detected_from_working_directory(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.cpp").write_text('int main() { return 0; }\n')
            self.cli("build", cwd=project)
            self.check_image(project / "build/firmware.bin")

    def test_compile_error_stops_flash_before_any_serial_access(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.c").write_text('#error deliberate failure\n')
            result = self.cli("flash", "--project", project, "--port", "INVALID_PORT", expected=1)
            self.assertIn("deliberate failure", result.stderr)
            self.assertNotIn("write_flash", result.stdout)

    def test_missing_port_fails_before_build(self):
        env = os.environ.copy()
        env.pop("ESPPORT", None)
        result = self.cli("flash", expected=2, env=env)
        self.assertIn("Specify --port", result.stderr)
        self.assertNotIn("gcc", result.stdout)

    def test_reserved_blink_pin_rejected(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            project = Path(folder)
            (project / "main.c").write_text((ROOT / "examples" / "blink" / "main.c").read_text())
            result = self.cli("build", "--project", project, "-D", "LED_PIN=18", expected=1)
            self.assertIn("Keep USB pins available", result.stderr)

    def test_unsupported_peripheral_target_rejected_before_compiler(self):
        result = self.cli("build", "--chip", "esp32c6", "--project", ROOT / "examples/pwm_sweep_channels_0_1", expected=2)
        self.assertIn("does not support esp32c6", result.stderr)
        self.assertNotIn("gcc", result.stdout)

    def test_selected_verification_rejects_unsupported_target(self):
        result = subprocess.run([self.python, str(ROOT / "tools/check_examples.py"),
                                 "--chip", "esp32c6", "--project", "examples/dma_spi"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertIn("do not support esp32c6", result.stderr)
        self.assertNotIn("0/0", result.stdout)


if __name__ == "__main__":
    unittest.main()
