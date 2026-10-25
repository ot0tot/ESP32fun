#!/usr/bin/env python3
"""Generate source-support capabilities from target profiles and header gates."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
from targets import TARGETS
ROOT=Path(__file__).resolve().parents[1]
DRIVERS={
    'GPIO, delay, ROM console':'esp32fun.h',
    'Software SPI/I2C':'fun_bus.h',
    'LEDC PWM':'fun_pwm.h',
    'Hardware I2C':'fun_i2c_hw.h',
    'SPI GDMA':'fun_dma.h',
    'UART UHCI DMA':'fun_uart_dma.h',
    'ADC DMA':'fun_adc_dma.h',
    'Parallel LCD GDMA':'fun_lcd_dma.h',
    'RMT RX GDMA':'fun_rmt_dma.h',
    'AES block accelerator':'fun_aes.h',
}
def inputs():
    files=[ROOT/'esp32fun/targets/profiles.json',ROOT/'tools/capabilities.py',
           *sorted((ROOT/'esp32fun').glob('*.h')),*sorted((ROOT/'esp32fun/targets').glob('*.h'))]
    # Source freshness survives Git's platform newline conversion. This does
    # not normalize files or the byte hashes of pinned/hardware evidence.
    return {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest() for p in files}
def generate(cc):
    result={}
    for driver,header in DRIVERS.items():
        result[driver]={}
        for chip in TARGETS:
            cmd=[cc,'-E','-x','c','-I',str(ROOT/'esp32fun'),'-include',str(ROOT/f'esp32fun/targets/{chip}.h'),'-']
            run=subprocess.run(cmd,input=f'#include "{header}"\n',text=True,capture_output=True)
            if run.returncode and '#error' not in run.stderr:
                raise RuntimeError(f'Preprocessor failed for {driver}/{chip}: {run.stderr}')
            # Diagnostics should identify repo files, not the checkout/worktree.
            diagnostic=run.stderr.replace(str(ROOT)+'\\','').replace(str(ROOT)+'/', '').replace(ROOT.as_posix()+'/', '')
            result[driver][chip]=dict(source_supported=run.returncode==0,
                reason='; '.join(line.strip() for line in diagnostic.splitlines() if '#error' in line))
    return dict(scope='Header-gate preprocessing only; input hashes use canonical LF text. Not hardware validation.',
                inputs=inputs(),drivers=result)
def markdown(report):
    chips=list(TARGETS);rows=['# Driver and chip capabilities','',
        'Generated from `profiles.json` and the actual source header gates by',
        '`python tools/capabilities.py --write`. **Implemented** means the driver',
        'header accepts that target. **Backend needed** means chip-specific code',
        'is missing; it is not a claim that the silicon lacks the peripheral.',
        'Neither result describes hardware validation. Tested boards and scenarios',
        'are recorded separately in [validation notes](validation.md).','',
        '| Driver | '+' | '.join(chips)+' |','|---|'+'---|'*len(chips)]
    for driver,targets in report['drivers'].items():
        rows.append('| '+driver+' | '+' | '.join('Implemented' if targets[c]['source_supported'] else 'Backend needed' for c in chips)+' |')
    rows += ['', 'Source support is distinct from physical qualification.',
             'Core profiles reserve 8 KiB of stack. GPIO reservations and console',
             'routing depend on the chip and board. DMA drivers exclusively own',
             'their channels and peripherals; simultaneous operation is not implied.',
             'See [driver contracts](drivers.md) and [limits](limits.md).', '']
    return '\n'.join(rows)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--write',action='store_true');p.add_argument('--check',action='store_true');p.add_argument('--cc')
    a=p.parse_args()
    if a.check:
        report=json.loads((ROOT/'docs/capabilities.json').read_text())
        if report['inputs']!=inputs():raise SystemExit('Capability inputs changed; regenerate with --write')
        if (ROOT/'docs/capabilities.md').read_text(encoding='utf-8')!=markdown(report):raise SystemExit('Capability table is stale')
        print('Capability source hashes and table match.');return
    cc=a.cc or os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
    if not cc:raise SystemExit('Host GCC/Clang required for header-gate preprocessing')
    report=generate(cc)
    if a.write:
        (ROOT/'docs/capabilities.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
        (ROOT/'docs/capabilities.md').write_bytes(markdown(report).encode())
    else:print(markdown(report))
if __name__=='__main__':main()
