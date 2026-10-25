#!/usr/bin/env python3
"""Install only GCC and host flashing tools, locally to this checkout."""
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import venv
import zipfile
import argparse
from targets import TARGETS, DEFAULT_TARGET

from esp32fun import ROOT, find_compiler, run


def host_key():
    system, machine = platform.system(), platform.machine().lower()
    if system == "Windows" and machine in {"amd64", "x86_64"}:
        return "win64"
    if system == "Linux" and machine in {"amd64", "x86_64"}:
        return "linux-amd64"
    if system == "Linux" and machine in {"aarch64", "arm64"}:
        return "linux-arm64"
    if system == "Darwin" and machine in {"amd64", "x86_64"}:
        return "macos"
    if system == "Darwin" and machine == "arm64":
        return "macos-arm64"
    raise RuntimeError(f"No pinned toolchain for {system}/{machine}. "
                       "Install riscv32-esp-elf-gcc manually and set ESP32FUN_CC.")


def install_compiler(chip=DEFAULT_TARGET):
    architecture = TARGETS[chip]["architecture"]
    lock = json.loads((ROOT / "tools" / "toolchains.json").read_text())[architecture]
    asset = lock["hosts"][host_key()]
    tool_root = ROOT / ".tools"
    tool_root.mkdir(exist_ok=True)
    destination = tool_root / (architecture + "-" + lock["version"])
    if destination.exists():
        raise RuntimeError(f"Incomplete toolchain at {destination}; move it aside before retrying.")
    with tempfile.TemporaryDirectory(dir=tool_root) as staging:
        staging = Path(staging)
        archive = staging / asset["url"].rsplit("/", 1)[-1]
        print(f"Downloading {asset['url']}", flush=True)
        with urllib.request.urlopen(asset["url"], timeout=60) as response, archive.open("wb") as output:
            shutil.copyfileobj(response, output)
        digest = hashlib.sha256()
        with archive.open("rb") as downloaded:
            for chunk in iter(lambda: downloaded.read(1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != asset["sha256"]:
            raise RuntimeError("Toolchain checksum mismatch; archive will not be extracted.")
        extracted = staging / "extracted"
        extracted.mkdir()
        if archive.suffix == ".zip":
            with zipfile.ZipFile(archive) as bundle:
                for name in bundle.namelist():
                    if not (extracted / name).resolve().is_relative_to(extracted):
                        raise RuntimeError("Unsafe archive path")
                bundle.extractall(extracted)
        else:
            with tarfile.open(archive) as bundle:
                # data filtering rejects paths and symlinks escaping extraction.
                if not hasattr(tarfile, "data_filter"):
                    raise RuntimeError("Use Python 3.12+ (or a patched 3.10/3.11) for safe tar extraction.")
                bundle.extractall(extracted, filter="data")
        shutil.move(str(extracted), str(destination))
    return find_compiler(chip=chip)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chip", choices=sorted(TARGETS), default=DEFAULT_TARGET)
    parser.add_argument("--all", action="store_true", help="set up both RISC-V and Xtensa toolchains")
    args = parser.parse_args()
    if sys.version_info < (3, 10):
        raise RuntimeError("Python 3.10 or newer is required.")
    for chip in (["esp32c3", "esp32s3"] if args.all else [args.chip]):
        try:
            compiler = find_compiler(chip=chip)
        except RuntimeError:
            if os.environ.get("ESP32FUN_CC"):
                raise
            compiler = install_compiler(chip)
        print(f"Using {compiler}")
    environment = ROOT / ".venv"
    python = environment / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    if not python.is_file():
        venv.EnvBuilder(with_pip=True).create(environment)
    run([python, "-m", "pip", "install", "-r", ROOT / "requirements.txt"])
    run([python, ROOT / "tools" / "esp32fun.py", "doctor", "--chip", args.chip])
    print("Ready. Run: python tools/esp32fun.py build")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"setup: {error}", file=sys.stderr)
        sys.exit(1)
