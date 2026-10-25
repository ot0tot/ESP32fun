#!/usr/bin/env python3
"""Check preserved source notices and optional pinned upstream checkouts."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def normalized(text):
    return '\n'.join(line.rstrip() for line in text.splitlines()).strip()


def check(manifest, root=ROOT, reference=None, idf=None):
    seen = set()
    for entry in manifest['files']:
        path = entry['path']
        if path in seen:
            raise ValueError('duplicate attributed file: ' + path)
        seen.add(path)
        raw = (root/path).read_bytes()
        text = normalized(raw.decode('utf-8'))
        for notice in [entry['notice'], entry.get('original_header'),
                       *[item['original_header'] for item in entry.get('related_sources', [])]]:
            if notice and normalized(notice) not in text:
                raise ValueError('missing original license notice: ' + path)
        checkout = idf if entry.get('origin') == 'esp-idf' else reference
        if checkout:
            original = (checkout/entry['upstream']).read_bytes()
            if entry.get('sha256_normalization') == 'LF':
                original = original.replace(b'\r\n', b'\n')
            if hashlib.sha256(original).hexdigest() != entry['upstream_sha256']:
                raise ValueError('pinned upstream bytes changed: ' + path)
            for related in entry.get('related_sources', []):
                original = (checkout/related['upstream']).read_bytes()
                if related.get('sha256_normalization') == 'LF':
                    original = original.replace(b'\r\n', b'\n')
                if hashlib.sha256(original).hexdigest() != related['upstream_sha256']:
                    raise ValueError('pinned secondary source changed: ' + path)
    for name in ['LICENSE', 'licenses/Apache-2.0.txt']:
        if not (root/name).is_file():
            raise ValueError('missing complete license: ' + name)
    return len(seen)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--idf', type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT/'docs/upstream-files.json').read_text(encoding='utf-8'))
    for checkout, revision in [(args.reference, manifest['commit']),
                               (args.idf, 'c8fc5f643b7a7b0d3b182d3df610844e3dc9bd74')]:
        if checkout:
            actual = subprocess.check_output(['git', '-C', str(checkout), 'rev-parse', 'HEAD'], text=True).strip()
            if actual != revision:
                raise ValueError('upstream checkout is not the pinned revision')
    print(f'{check(manifest, reference=args.reference, idf=args.idf)} source notices verified.')


if __name__ == '__main__':
    main()
