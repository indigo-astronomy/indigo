#!/usr/bin/env python3
"""Run the repaired-source sanitizer matrix and historical compatibility checks."""
from pathlib import Path
import os
import subprocess
import tempfile

TASK = Path(__file__).resolve().parent
ROOT = next(path for path in TASK.parents if (path / '.factory/project.md').is_file())
PREVIOUS = ROOT / '.factory/tasks/archive/20261101185459-950f-verify-star-detectors'
BUILD = Path(tempfile.mkdtemp(prefix='indigo-detector-fix-'))
FLAGS = ['clang', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-I', str(ROOT / 'indigo_libs')]
LINK = ['-L', str(ROOT / 'build/lib'), '-lindigo', '-Wl,-rpath,' + str(ROOT / 'build/lib')]
ENV = {**os.environ, 'ASAN_OPTIONS': 'symbolize=0'}

def run(args, **kwargs):
    print('COMMAND:', repr([str(arg) for arg in args]), flush=True)
    return subprocess.run([str(arg) for arg in args], cwd=ROOT, env=ENV, check=True, **kwargs)

source = ROOT / 'indigo_libs/indigo_raw_utils.c'
run(FLAGS + [str(source), str(ROOT / 'indigo_test/unit/test_star_detection.c')] + LINK + ['-o', str(BUILD / 'matrix')])
run([BUILD / 'matrix'])
run(FLAGS + [str(source), str(PREVIOUS / 'detector-review-fixture.c')] + LINK + ['-o', str(BUILD / 'original')])
for mode in ['edge', 'edge-precise', 'radius', 'precise']:
    result = run([BUILD / 'original', mode], capture_output=True, text=True)
    print(mode, result.stdout, flush=True)
    assert result.stderr == ''
    assert ('count=0' if mode.startswith('edge') else 'count=-1') in result.stdout
before = BUILD / 'before.c'
before.write_bytes(subprocess.check_output(['git', 'show', '21d2af5c5^:indigo_libs/indigo_raw_utils.c'], cwd=ROOT))
outputs = []
for name, src in [('current', source), ('before', before)]:
    executable = BUILD / name
    defines = ['-DCHECK_MEDIAN'] if name == 'current' else []
    run(FLAGS + defines + ['-DRAW_SOURCE="' + str(src) + '"', str(PREVIOUS / 'compatibility-fixture.c')] + LINK + ['-o', str(executable)])
    outputs.append(run([executable], capture_output=True).stdout)
assert outputs[0] == outputs[1]
print('PASS: 3960 small-image calls; 192 invalid radius/geometry calls; 60 normal-image calls including radius128; original reproducers clean; 8192 medians; 288 precise outputs identical.', flush=True)
print('Artifacts:', BUILD)
