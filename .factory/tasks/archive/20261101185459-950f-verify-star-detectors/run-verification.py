#!/usr/bin/env python3
"""Reproduce the reported defects and compare precise detection with its baseline."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = next(path for path in Path(__file__).resolve().parents if (path / '.factory/project.md').is_file())
TASK = Path(__file__).resolve().parent
BUILD = Path(tempfile.mkdtemp(prefix='indigo-detector-verify-'))
FLAGS = ['clang', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-I', str(ROOT / 'indigo_libs')]
LINK = ['-L', str(ROOT / 'build/lib'), '-lindigo', '-Wl,-rpath,' + str(ROOT / 'build/lib')]

def run(args, **kwargs):
    print('COMMAND:', repr([str(arg) for arg in args]), flush=True)
    return subprocess.run([str(arg) for arg in args], cwd=ROOT, **kwargs)

before = BUILD / 'before.c'
before.write_bytes(subprocess.check_output(['git', 'show', '21d2af5c5^:indigo_libs/indigo_raw_utils.c'], cwd=ROOT))
source = ROOT / 'indigo_libs/indigo_raw_utils.c'
for name, src in [('current', source), ('before', before)]:
    run(FLAGS + [str(src), str(TASK / 'detector-review-fixture.c')] + LINK + ['-o', str(BUILD / name)], check=True)
for binary, mode, expected in [('current', 'edge', 'heap-buffer-overflow'), ('current', 'edge-precise', 'heap-buffer-overflow'), ('current', 'radius', 'signed integer overflow'), ('current', 'precise', 'signed integer overflow'), ('before', 'precise', 'result=0 count=0')]:
    result = run([BUILD / binary, mode], capture_output=True, text=True, env={**os.environ, 'ASAN_OPTIONS': 'symbolize=0'})
    output = result.stdout + result.stderr
    print('RETURN:', result.returncode, '\n' + output, flush=True)
    assert expected in output
    assert (result.returncode == 0) == (binary == 'before')
outputs = []
for name, src in [('current', source), ('before', before)]:
    flags = ['-DCHECK_MEDIAN'] if name == 'current' else []
    executable = BUILD / ('compat-' + name)
    run(FLAGS + flags + ['-DRAW_SOURCE="' + str(src) + '"', str(TASK / 'compatibility-fixture.c')] + LINK + ['-o', str(executable)], check=True)
    outputs.append(run([executable], capture_output=True, check=True).stdout)
assert outputs[0] == outputs[1]
print('PASS: 5 expected reproduction outcomes; 8192 median cases; 288 precise image cases identical.')
print('Artifacts:', BUILD)
