"""Compile the actual clock decoder; exercise independent clock-tree cases."""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ClassicClock(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        source = folder/'clock.c'
        source.write_text('#include "fun_clock_esp32.h"\n'
                          'unsigned decode(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e)'
                          '{return fun_esp32_clock_mhz(a,b,c,d,e);}\n')
        library = folder/('clock.dll' if os.name == 'nt' else 'clock.so')
        compiler = os.environ.get('CC') or shutil.which('gcc')
        if not compiler: raise RuntimeError('host compiler required')
        subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-static-libgcc','-I',str(ROOT/'esp32fun'),str(source),'-o',str(library)],
                       check=True,capture_output=True)
        cls.dll = ctypes.CDLL(str(library))
        cls.decode = cls.dll.decode
        cls.decode.argtypes = [ctypes.c_uint]*5
        cls.decode.restype = ctypes.c_uint

    @classmethod
    def tearDownClass(cls):
        if os.name == 'nt': ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(cls.dll._handle))
        cls.temp.cleanup()

    def test_xtal40_with_empty_metadata(self):
        self.assertEqual(self.decode(0x3210,0,0,0x2000,40),40)

    def test_xtal_metadata_and_dividers(self):
        for xtal in (24,26,40):
            self.assertEqual(self.decode(0,0,xtal*0x10001,0,40),xtal)
            self.assertEqual(self.decode(0,0,xtal*0x10001,1,40),xtal//2)
        self.assertEqual(self.decode(0,0,0x12340028,0,26),26)
        self.assertEqual(self.decode(0,0,0,1023,40),0)
        self.assertEqual(self.decode(0,0,25*0x10001,0,40),0)

    def test_pll_and_unsupported_modes(self):
        for select,expected in enumerate((80,160,240,0)):
            self.assertEqual(self.decode(1<<27,select,0,0,40),expected)
        for source in (2,3): self.assertEqual(self.decode(source<<27,0,0,0,40),0)


if __name__ == '__main__': unittest.main()
