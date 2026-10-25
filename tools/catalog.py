#!/usr/bin/env python3
"""Check and list the included ESP32 examples and their build targets."""
import argparse
import json
from pathlib import Path
from targets import TARGETS

ROOT = Path(__file__).resolve().parent.parent


def check(projects):
    if any(path.is_dir() for path in ROOT.glob('examples_*')):
        raise ValueError('example projects must live under examples/')
    discovered = {p.parent.relative_to(ROOT).as_posix() for p in ROOT.glob("examples/**/main.*")
                  if "build" not in p.relative_to(ROOT).parts and p.suffix in {'.c', '.cpp', '.cc', '.S'}}
    if discovered != set(projects): raise ValueError("project inventory differs from source directories")
    for name, metadata in projects.items():
        parts = Path(name).parts
        if len(parts) != 2 or parts[0] != 'examples' or parts[1] in {'.', '..'}:
            raise ValueError(f'invalid project path: {name}')
        chips = metadata['chips']
        if not chips or len(set(chips)) != len(chips) or set(chips) - set(TARGETS):
            raise ValueError(f"invalid chips for {name}")
        if not metadata.get('description'):
            raise ValueError(f'missing description for {name}')
        if not (ROOT / name / "Makefile").is_file(): raise ValueError(f"missing Makefile for {name}")


def markdown(projects):
    lines = ['# ESP32 examples', '',
             f"{len(projects)} projects and {sum(len(p['chips']) for p in projects.values())} project/chip build combinations.", '',
             'Build targets describe the current implementation. Board tests are listed',
             'separately in [validation notes](validation.md); an untested board is not',
             'an unsupported board. See [wiring](examples.md) before connecting hardware.', '',
             '| Project | Build targets | Description |', '|---|---|---|']
    for name, item in sorted(projects.items()):
        project = f"[{name.removeprefix('examples/')}](../{name}/)"
        lines.append(f"| {project} | {', '.join(item['chips'])} | {item['description'].replace('|', '/')} |")
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-docs", action="store_true")
    parser.add_argument('--chip', choices=sorted(TARGETS), help='list projects with this build target')
    args = parser.parse_args()
    projects = json.loads((ROOT / "examples/projects.json").read_text(encoding="utf-8"))
    check(projects)
    if args.write_docs:
        (ROOT / "docs/examples-index.md").write_text(markdown(projects), encoding="utf-8")
    else:
        for name, item in sorted(projects.items()):
            if args.chip is None or args.chip in item['chips']:
                print(f"{name:40} {', '.join(item['chips'])}")
    print(f"{len(projects)} projects; {sum(len(p['chips']) for p in projects.values())} build combinations.")


if __name__ == "__main__": main()
