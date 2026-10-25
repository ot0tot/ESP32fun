#!/usr/bin/env python3
"""Build runnable projects and validate images; never flash or open a serial port."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from check_images import validate_image
from esp32fun import find_python, output_directory
from targets import TARGETS

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chip", choices=sorted(TARGETS), default="esp32c3")
    parser.add_argument("--matrix", action="store_true", help="every declared project/target pair")
    parser.add_argument("--project", action="append", help="restrict to a declared project (repeatable)")
    args = parser.parse_args()
    projects = json.loads((ROOT / "examples/projects.json").read_text())
    if args.project:
        unknown = set(args.project) - set(projects)
        if unknown: parser.error("Unknown projects: " + ", ".join(sorted(unknown)))
        projects = {name: projects[name] for name in args.project}
        if not args.matrix:
            unsupported = [name for name, metadata in projects.items() if args.chip not in metadata["chips"]]
            if unsupported: parser.error(f"Projects do not support {args.chip}: " + ", ".join(unsupported))
    logs = ROOT / "build/checks"
    logs.mkdir(parents=True, exist_ok=True)
    report = {"hardware_verified": False, "results": []}
    failed = 0
    for project, metadata in sorted(projects.items()):
        for chip in metadata["chips"] if args.matrix else [args.chip]:
            if chip not in metadata["chips"]: continue
            command = [find_python(), str(ROOT / "tools/esp32fun.py"), "build", "--chip", chip,
                       "--project", str(ROOT / project)]
            result = subprocess.run(command, capture_output=True, text=True)
            log = logs / (project.replace("/", "_") + "_" + chip + ".log")
            log.write_text(result.stdout + result.stderr, encoding="utf-8")
            record = {"project": project, "chip": chip, "passed": False}
            try:
                if result.returncode: raise ValueError(f"build exit {result.returncode}")
                record.update(validate_image(output_directory(ROOT / project, chip) / "firmware.bin", chip))
                record["passed"] = True
            except (ValueError, OSError) as error:
                record["error"] = str(error)
                failed += 1
            report["results"].append(record)
            print(f"{'PASS' if record['passed'] else 'FAIL'} {chip:8} {project}", flush=True)
    report_name = "selected-report.json" if args.project else "report.json"
    (logs / report_name).write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(report['results']) - failed}/{len(report['results'])} builds and images passed; hardware unverified.")
    return 1 if failed else 0


if __name__ == "__main__": sys.exit(main())
