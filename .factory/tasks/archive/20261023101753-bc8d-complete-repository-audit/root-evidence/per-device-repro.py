"""Exercise real result recording with synthetic records; never run hardware/make."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
root = Path.cwd()
sys.path.insert(0, str(root / 'tools'))
spec = importlib.util.spec_from_file_location('driver_runner', root / 'tools/run_driver_test.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
with tempfile.TemporaryDirectory(prefix='indigo-audit-recorder-') as temp:
    work = Path(temp)
    (work / 'README.md').write_text('# Synthetic\n')
    runner.find_driver = lambda driver: str(work)
    runner.driver_version = lambda *args: '3.0.0.1'
    def run(command, cwd, env=None):
        assert env and 'INDIGO_TEST_RESULTS' in env
        Path(env['INDIGO_TEST_RESULTS']).write_text('plan\t1\tsynthetic\ncase_device\tsynthetic\tcase\tindigo_system_alpaca\tunit\ndevice\tindigo_system_alpaca\t0\tunit\tSynthetic model\npass\tsynthetic\tcase\n')
        return 1
    runner.run = run
    sys.argv = ['run_driver_test.py', 'system_alpaca', '--hw', '--no-build', '--dry-run']
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        status = runner.main()
    result = {'exit': status, 'output': output.getvalue(), 'records_OK_after_process_failure': 'Synthetic model 1/1 OK' in output.getvalue()}
    print(json.dumps(result, indent=2))
    assert status == 1 and result['records_OK_after_process_failure']
