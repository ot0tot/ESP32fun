#!/usr/bin/env python3
"""Check distribution inventory and required source contracts."""
import json
from pathlib import Path
from catalog import check as check_catalog, markdown as example_markdown
from check_licenses import check as check_notices

ROOT = Path(__file__).resolve().parents[1]


def check():
    projects = json.loads((ROOT/'examples/projects.json').read_text(encoding='utf-8'))
    check_catalog(projects)
    if (ROOT/'docs/examples-index.md').read_text(encoding='utf-8') != example_markdown(projects):
        raise ValueError('example index is stale; run tools/catalog.py --write-docs')
    check_notices(json.loads((ROOT/'docs/upstream-files.json').read_text(encoding='utf-8')))
    tools = json.loads((ROOT/'tools/status.json').read_text(encoding='utf-8'))['tools']
    actual = {p.name for p in (ROOT/'tools').glob('*.py')}
    if set(tools) != actual or any(value != 'maintained' for value in tools.values()):
        raise ValueError('tool inventory differs from maintained frontends')
    for name in ['fun_uart_dma.h', 'fun_uart_udma_esp32.h']:
        text = (ROOT/'esp32fun'/name).read_text(encoding='utf-8')
        if '#define FUN_UART_DMA_ERRORS' in text or '#include "fun_uart_errors.h"' not in text:
            raise ValueError('UART error masks diverged')
    for name in ['fun_bus.h', 'fun_i2c_hw.h']:
        if '#include "fun_i2c_errors.h"' not in (ROOT/'esp32fun'/name).read_text(encoding='utf-8'):
            raise ValueError('I2C error vocabulary diverged')
    return len(projects), sum(len(item['chips']) for item in projects.values())


if __name__ == '__main__':
    projects, pairs = check()
    print(f'{projects} projects / {pairs} build pairs; source contracts and notices consistent.')
