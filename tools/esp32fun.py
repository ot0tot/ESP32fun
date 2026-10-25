#!/usr/bin/env python3
"""Small, shell-free build/flash frontend for bare-metal ESP32 firmware."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from targets import TARGETS, DEFAULT_TARGET, TARGET_DIRECTORY, linker_script

ROOT = Path(__file__).resolve().parent.parent
RUNTIME = ROOT / "esp32fun"


def find_compiler(explicit=None, chip=DEFAULT_TARGET, cxx=False):
    name = TARGETS[chip]["compiler"].replace("gcc", "g++") if cxx else TARGETS[chip]["compiler"]
    requested = explicit or os.environ.get("ESP32FUN_CC")
    if requested:
        resolved = shutil.which(requested)
        if resolved:
            return Path(resolved).resolve()
        raise RuntimeError(f"Compiler not found: {requested}")
    found = shutil.which(name)
    if found:
        return Path(found).resolve()
    suffix = ".exe" if os.name == "nt" else ""
    roots = [ROOT / ".tools", Path(os.environ.get("IDF_TOOLS_PATH",
                                                    str(Path.home() / ".espressif"))) / "tools"]
    for root in roots:
        candidates = sorted(root.glob(f"**/bin/{name}{suffix}"))
        if candidates:
            return candidates[-1].resolve()
    raise RuntimeError(f"Install {name} with python tools/setup.py --chip {chip}, "
                       "or set ESP32FUN_CC to the compiler path.")


def find_python():
    local = ROOT / ".venv" / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    if local.is_file():
        return str(local)
    if importlib.util.find_spec("esptool") is not None:
        return sys.executable
    raise RuntimeError("esptool is missing. Run python tools/setup.py first.")


def run(command, **kwargs):
    command = [str(item) for item in command]
    print(subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, check=True, **kwargs)


def sibling_tool(compiler, name):
    return compiler.with_name(compiler.name.replace("gcc", name))


def build(args, project):
    sources = sorted(p for p in project.iterdir() if p.suffix in {".c", ".s", ".S", ".cpp", ".cc"})
    if not sources:
        raise RuntimeError(f"No C or assembly sources in {project}")
    target = TARGETS[args.chip]
    compiler = find_compiler(args.cc, args.chip)
    python = find_python()
    out = output_directory(project, args.chip)
    out.mkdir(exist_ok=True, parents=True)
    (out / "target.ld").write_text(linker_script(args.chip, ram_boot=getattr(args,'ram_image',False)), encoding="utf-8")
    arch_flags = (["-mlongcalls", "-mtext-section-literals"] if target["architecture"] == "xtensa"
                  else ["-march=rv32imc_zicsr_zifencei", "-mabi=ilp32", "-msmall-data-limit=0"])
    flags = [*arch_flags, "-Os", "-g3",
             "-ffreestanding", "-fno-builtin", "-fno-common", "-fno-pie",
             "-fno-stack-protector", "-ffunction-sections", "-fdata-sections",
             "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
             "-Wall", "-Wextra", "-Werror",
             "-include", str(TARGET_DIRECTORY / (args.chip + ".h")),
             "-I", str(RUNTIME), "-I", str(project)]
    flags += ["-D" + define for define in args.define]
    objects, commands = [], []
    startup = "startup_xtensa.S" if target["architecture"] == "xtensa" else "startup_esp32c3.S"
    all_sources = [RUNTIME / startup, RUNTIME / "esp32fun.c"] + sources
    for index, source in enumerate(all_sources):
        obj = out / f"{index}_{source.name}.o"
        cxx = source.suffix in {".cpp", ".cc"}
        extra = (["-std=c++11", "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics", "-fno-use-cxa-atexit"]
                 if cxx else ["-std=c11"] if source.suffix == ".c" else [])
        cc = sibling_tool(compiler, "g++") if cxx else compiler
        command = [str(cc), *flags, *extra, "-c", str(source), "-o", str(obj)]
        run(command)
        objects.append(obj)
        commands.append({"directory": str(project), "file": str(source),
                         "arguments": command})
    (out / "compile_commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    elf = out / "firmware.elf"
    run([compiler, *flags, "-nostdlib", "-nostartfiles", "-static",
         "-Wl,--gc-sections,--build-id=none", "-Wl,-Map," + str(out / "firmware.map"),
         "-T", out / "target.ld", *objects, "-lgcc", "-o", elf])
    revision_flags = (['--min-rev-full', str(target['minimum_chip_revision'])]
                      if 'minimum_chip_revision' in target else [])
    run([python, "-m", "esptool", "--chip", args.chip, "elf2image", *revision_flags,
         "--flash_mode", "dio", "--flash_freq", target["flash_frequency"], "--flash_size", args.flash_size,
         "-o", out / "firmware.bin", elf])
    run([sibling_tool(compiler, "size"), elf])
    print(f"ROM boot image: {out / 'firmware.bin'} (flash at {target['flash_offset']:#x})")


def output_directory(project, chip):
    return project / "build" if chip == DEFAULT_TARGET else project / "build" / chip


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["build", "flash", "ram", "monitor", "clean", "size", "doctor", "targets"])
    parser.add_argument("--chip", choices=sorted(TARGETS), default=DEFAULT_TARGET)
    parser.add_argument("--project", type=Path,
                        default=Path.cwd() if any((Path.cwd() / name).is_file()
                                                 for name in ("main.c", "main.cpp", "main.cc", "main.S"))
                        else ROOT / "examples" / "blink")
    parser.add_argument("--cc", help="path to the selected ESP GCC compiler")
    parser.add_argument("-D", "--define", action="append", default=[], help="C macro, e.g. LED_PIN=4")
    parser.add_argument('--ram-image', action='store_true', help='Reserve ROM-download workspace (S3); automatic for the ram command')
    parser.add_argument("--port", default=os.environ.get("ESPPORT"), help="e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=460800, help="flash baud rate")
    parser.add_argument("--flash-size", choices=["1MB", "2MB", "4MB", "8MB", "16MB"], default="4MB")
    args = parser.parse_args(argv)
    args.ram_image |= args.command == 'ram'
    if args.command == "targets":
        for name, t in TARGETS.items():
            print(f"{name:10} {t['architecture']:7} offset={t['flash_offset']:#x} hardware=unverified")
        return
    project = args.project.resolve()
    if args.command == "doctor":
        compiler = find_compiler(args.cc, args.chip)
        run([compiler, "--version"])
        run([find_python(), "-m", "esptool", "version"])
        print(f"Target: {args.chip}; firmware uses no ESP-IDF or RTOS.")
        return
    if not project.is_dir():
        parser.error(f"Project directory does not exist: {project}")
    metadata = json.loads((ROOT / "examples/projects.json").read_text(encoding="utf-8"))
    try:
        name = project.relative_to(ROOT).as_posix()
    except ValueError:
        name = None
    if name in metadata and args.chip not in metadata[name]["chips"]:
        parser.error(f"{name} does not support {args.chip}; declared chips: " + ", ".join(metadata[name]["chips"]))
    if args.command in {"flash", "ram", "monitor"} and not args.port:
        parser.error("Specify --port or set ESPPORT.")
    if args.baud <= 0:
        parser.error("--baud must be positive")
    out = output_directory(project, args.chip)
    if args.command == "clean":
        # Refuse links/junctions; only remove this project's build directory.
        if out.exists():
            if out.is_symlink() or out.resolve() != out.absolute() or not out.resolve().is_relative_to(project):
                raise RuntimeError(f"Refusing to clean redirected build directory: {out}")
            if args.chip == DEFAULT_TARGET:
                # C3 artifacts live at build's top level for compatibility;
                # other targets have independent subdirectories inside it.
                for artifact in out.iterdir():
                    if artifact.is_file() and not artifact.is_symlink():
                        artifact.unlink()
                if not any(out.iterdir()):
                    out.rmdir()
            else:
                shutil.rmtree(out)
        return
    if args.command == "monitor":
        # Request deasserted DTR/RTS; some serial adapters can still glitch on open.
        run([find_python(), "-m", "serial.tools.miniterm", "--dtr", "0", "--rts", "0",
             args.port, "115200"])
        return
    if args.command == "size":
        elf = out / "firmware.elf"
        if not elf.is_file():
            raise RuntimeError("No firmware.elf. Build the project first.")
        run([sibling_tool(find_compiler(args.cc, args.chip), "size"), elf])
        return
    build(args, project)
    if args.command == "flash":
        run([find_python(), "-m", "esptool", "--chip", args.chip, "--port", args.port,
             "--baud", args.baud, "write_flash", hex(TARGETS[args.chip]["flash_offset"]), out / "firmware.bin"])
    elif args.command == "ram":
        run([find_python(), "-m", "esptool", "--chip", args.chip, "--port", args.port,
             "--no-stub", "load_ram", out / "firmware.bin"])


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(f"esp32fun: {error}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)
