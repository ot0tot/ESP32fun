"""Preserve real third-party opening notices, including secondary attributions."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from check_licenses import check


class LicenseChecks(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((ROOT/'docs/upstream-files.json').read_text(encoding='utf-8'))

    def test_all_carried_notices(self):
        self.assertEqual(check(self.manifest), len(self.manifest['files']))

    def test_removed_original_notice_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            shutil.copyfile(ROOT/'LICENSE', root/'LICENSE')
            shutil.copytree(ROOT/'licenses', root/'licenses')
            for entry in self.manifest['files']:
                target = root/entry['path']
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT/entry['path'], target)
            target = root/'esp32fun/fun_usb_console.h'
            text = target.read_text(encoding='utf-8')
            target.write_text(text.replace('2010-2023 Espressif Systems (Shanghai) CO LTD', 'removed'), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'original license notice'):
                check(self.manifest, root=root)

    def test_secondary_copyright_is_required(self):
        entry = next(e for e in self.manifest['files'] if e['path']=='esp32fun/fun_i2c_hw.h')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root/entry['path'];target.parent.mkdir(parents=True)
            text = (ROOT/entry['path']).read_text(encoding='utf-8')
            target.write_text(text.replace('2020-2024 Espressif Systems (Shanghai) CO LTD', 'removed'), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'original license notice'):
                check({'files':[entry]}, root=root)

    def test_duplicate_provenance_is_rejected(self):
        entry = self.manifest['files'][0]
        with self.assertRaisesRegex(ValueError, 'duplicate attributed file'):
            check({'files':[entry,entry]})


if __name__ == '__main__':
    unittest.main()
