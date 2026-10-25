"""Compile the actual completion policy and inject hardware event orderings."""
import ctypes
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RmtCompletion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get('CC') or shutil.which('gcc')
        if not compiler:
            raise RuntimeError('host GCC required for RMT completion regression')
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        source = folder / 'poll.c'
        source.write_text('#include "fun_rmt_dma_status.h"\n'
                          'int poll_status(unsigned d,unsigned r,unsigned c,unsigned n)'
                          '{return fun_rmt_dma_poll(d,r,c,n);}\n', encoding='utf-8')
        library = folder / ('poll.dll' if os.name == 'nt' else 'poll.so')
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared',
                        '-fPIC', '-static-libgcc', '-I', str(ROOT / 'esp32fun'),
                        str(source), '-o', str(library)], check=True, capture_output=True)
        cls.dll = ctypes.CDLL(str(library))
        cls.poll = cls.dll.poll_status
        cls.poll.argtypes = [ctypes.c_uint] * 4
        cls.poll.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        if os.name == 'nt':
            ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(cls.dll._handle))
        cls.temp.cleanup()

    def test_empty_then_done_then_owner_writeback(self):
        # A terminal descriptor can expose these three events separately.
        full = 512 | (512 << 12)
        owned = full | (1 << 31)
        self.assertEqual(self.poll(0x10, 0, owned, 512), 0)
        self.assertEqual(self.poll(0x11, 0, owned, 512), 0)
        self.assertEqual(self.poll(0x11, 0, full, 512), 1)

    def test_partial_frame_eof(self):
        self.assertEqual(self.poll(2, 0, 512 | (80 << 12), 512), 1)

    def test_empty_with_partial_descriptor_rejected(self):
        self.assertEqual(self.poll(0x11, 0, 512 | (80 << 12), 512), -1)

    def test_no_input_stays_pending(self):
        self.assertEqual(self.poll(0, 0, (1 << 31) | 512, 512), 0)

    def test_genuine_errors_never_hidden_by_done(self):
        for bit in (2, 3, 6, 7, 8, 9):
            with self.subTest(bit=bit):
                self.assertEqual(self.poll(1 | (1 << bit), 0, 512 | (512 << 12), 512), -1)
        for bit in (23, 29):
            with self.subTest(rmt_bit=bit):
                self.assertEqual(self.poll(1, 1 << bit, 512 | (512 << 12), 512), -1)

    def test_invalid_lengths_rejected(self):
        for length in (0, 1, 3, 513, 516):
            with self.subTest(length=length):
                self.assertEqual(self.poll(1, 0, 512 | (length << 12), 512), -1)


if __name__ == '__main__':
    unittest.main()
