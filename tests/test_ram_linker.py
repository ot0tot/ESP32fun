"""RAM boot excludes the measured S3 ROM workspace without widening SRAM use."""
from pathlib import Path
import re
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from targets import TARGETS,linker_script


def regions(text):
    return {name:(int(origin,16),int(size,16)) for name,origin,size in
            re.findall(r'(iram|dram|stack) \([^)]*\) : ORIGIN = (0x[0-9a-f]+), LENGTH = (0x[0-9a-f]+)',text)}


class RamPlacement(unittest.TestCase):
    def test_s3_ram_excludes_loader_workspace_and_preserves_end(self):
        normal=regions(linker_script('esp32s3'));ram=regions(linker_script('esp32s3',True))
        self.assertGreaterEqual(ram['iram'][0],0x403c9000)
        self.assertEqual(sum(ram['iram']),sum(normal['iram']))
        self.assertEqual(ram['dram'],normal['dram']);self.assertEqual(ram['stack'],normal['stack'])

    def test_other_targets_are_unchanged(self):
        for chip in TARGETS:
            if chip!='esp32s3':self.assertEqual(linker_script(chip),linker_script(chip,True))
