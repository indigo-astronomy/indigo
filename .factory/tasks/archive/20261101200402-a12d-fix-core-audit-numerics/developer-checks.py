"""Reproduce scoped native and sanitizer checks using current production objects."""
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=Path(__file__).resolve().parent, text=True).strip())
SUPPORT = Path('/Users/polakovic/Development/indigo/build/lib')

def run(args, cwd=ROOT, env=None):
	print('$', shlex.join(map(str, args)), '(cwd=' + str(cwd) + ')', flush=True)
	subprocess.run(list(map(str, args)), cwd=cwd, env=env, check=True, timeout=120)

for path in [ROOT / p for p in ['indigo_libs/indigo_json.c', 'indigo_libs/indigo_ser.c', 'indigo_libs/indigo_stretch.cpp', 'indigo_tools/indigo_solver_indexer.c']] + [SUPPORT / p for p in ['libindigo.a', 'libindigo_client.a', 'libhidapi.a', 'libusb-1.0.dylib']]:
	print(hashlib.sha256(path.read_bytes()).hexdigest(), path, flush=True)
with tempfile.TemporaryDirectory(prefix='indigo-numerics-') as temporary:
	build = Path(temporary)
	run_root = build / 'run'
	(run_root / 'build/unit').mkdir(parents=True)
	(run_root / 'fixtures/protocol').mkdir(parents=True)
	for fixture in ['change_number.json', 'change_switch.json', 'malformed.json']:
		shutil.copyfile(ROOT / 'indigo_test/fixtures/protocol' / fixture, run_root / 'fixtures/protocol' / fixture)
	for mode in ['native', 'sanitizer']:
		flags = ['-g', '-O1', '-I' + str(ROOT / 'indigo_libs'), '-Wall', '-Wextra', '-Wno-unused-parameter']
		if mode == 'sanitizer':
			flags += ['-fsanitize=address,undefined,float-cast-overflow', '-fno-omit-frame-pointer']
		env = dict(os.environ, UBSAN_OPTIONS='halt_on_error=1', ASAN_OPTIONS='detect_leaks=0')
		for name, source, compiler, standard in [('json', 'indigo_json.c', 'cc', '-std=gnu11'), ('ser', 'indigo_ser.c', 'cc', '-std=gnu11'), ('stretch', 'indigo_stretch.cpp', 'c++', '-std=c++11')]:
			run([compiler, standard, *flags, '-c', ROOT / 'indigo_libs' / source, '-o', build / (name + '.o')])
		for suite, obj in [('protocol_json', 'json'), ('image_math', 'stretch'), ('recording_io', 'ser')]:
			run(['cc', '-std=gnu11', *flags, '-c', ROOT / 'indigo_test/unit' / ('test_' + suite + '.c'), '-o', build / 'test.o'])
			libs = [SUPPORT / 'libindigo.a', SUPPORT / 'libhidapi.a']
			if suite == 'recording_io':
				libs += [SUPPORT / 'libusb-1.0.dylib']
			run(['c++', *flags, build / 'test.o', build / (obj + '.o'), *libs, '-lz', '-ldl', '-lm', '-lpthread', '-framework', 'IOKit', '-framework', 'CoreFoundation', '-o', build / suite])
			run([build / suite], cwd=run_root, env=env)
			if suite == 'recording_io':
				for zone in ['UTC0', 'UTC-5', 'UTC5']:
					run([build / suite], cwd=run_root, env=dict(env, TZ=zone))
		env['INDIGO_TEST_LIBRARY_DIR'] = str(SUPPORT)
		if mode == 'sanitizer':
			env['SANITIZERS'] = 'address,undefined,float-cast-overflow'
		run(['python3', '-B', ROOT / 'indigo_test/unit/test_tool_input_safety.py'], env=env)
