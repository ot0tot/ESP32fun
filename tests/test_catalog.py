"""Example inventory and independent corruption checks, without hardware."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from catalog import check
from check_images import validate_image
from targets import TARGETS, linker_script


class CatalogChecks(unittest.TestCase):
    def test_checkout_under_build_keeps_project_inventory_checks(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'build' / 'checkout'
            project = root / 'examples/demo'
            project.mkdir(parents=True)
            (project / 'main.c').write_text('int main(void){return 0;}\n')
            (project / 'Makefile').write_text('all:\n')
            generated = project / 'build/generated'
            generated.mkdir(parents=True)
            (generated / 'main.c').write_text('generated artifact\n')
            with patch('catalog.ROOT', root):
                projects = {'examples/demo': {'chips': ['esp32c3'], 'description': 'Demo'}}
                check(projects)
                extra = root / 'examples/unregistered'
                extra.mkdir()
                (extra / 'main.c').write_text('int main(void){return 0;}\n')
                with self.assertRaisesRegex(ValueError, 'project inventory'):
                    check(projects)

    def test_project_inventory_and_chip_declarations(self):
        projects = json.loads((ROOT / "examples/projects.json").read_text())
        check(projects)
        first = next(iter(projects))
        projects[first]['chips'] = ['invalid-chip']
        with self.assertRaisesRegex(ValueError, 'invalid chips'):
            check(projects)

    def test_project_without_makefile_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            project = root / 'examples/demo'
            project.mkdir(parents=True)
            (project / 'main.c').write_text('int main(void){return 0;}\n')
            with patch('catalog.ROOT', root):
                with self.assertRaisesRegex(ValueError, 'missing Makefile'):
                    check({'examples/demo': {'chips': ['esp32c3'], 'description': 'Demo'}})

    def test_linker_reserves_stack_and_nonoverlapping_sram(self):
        for chip, t in TARGETS.items():
            self.assertLessEqual(t["dram"] + t["dram_length"], t["stack"], chip)
            self.assertEqual(t["stack"] % 16, 0, chip)
            if t["alias"] or t["architecture"] == "riscv":
                self.assertLessEqual(t["iram"] + t["iram_length"] - t["alias"], t["dram"], chip)
            self.assertIn('ENTRY(_start)', linker_script(chip))
            self.assertFalse(t["hardware_verified"])

    def test_truncated_and_corrupted_images_are_rejected(self):
        # The regular build test produces this fixture; build if run alone.
        from esp32fun import find_python
        import subprocess
        fixture = ROOT / "examples/hello/build/firmware.bin"
        if not fixture.is_file():
            subprocess.run([find_python(), str(ROOT / "tools/esp32fun.py"), "build",
                            "--project", str(ROOT / "examples/hello")], check=True, capture_output=True)
        good = fixture.read_bytes()
        validate_image(fixture, "esp32c3")
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as folder:
            bad = Path(folder) / "bad.bin"
            for content in [good[:12], good[:-1], good[:32] + bytes([good[32] ^ 1]) + good[33:],
                            good[:-1] + bytes([good[-1] ^ 1])]:
                bad.write_bytes(content)
                with self.assertRaises(ValueError): validate_image(bad, "esp32c3")
        with self.assertRaisesRegex(ValueError, "wrong chip ID"): validate_image(fixture, "esp32s3")


if __name__ == "__main__": unittest.main()
