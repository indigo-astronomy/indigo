import fcntl
import importlib.util
import sys
from pathlib import Path
root = Path('/Users/polakovic/Development/indigo')
sys.path.insert(0, str(root / 'tools'))
spec = importlib.util.spec_from_file_location('run_driver_test', root / 'tools/run_driver_test.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
record_lock = open('/private/tmp/indigo-driver-record.lock', 'a')
original_update = runner.update_readme
original_run = runner.subprocess.run

def locked_update(*args, **kwargs):
    fcntl.flock(record_lock, fcntl.LOCK_EX)
    return original_update(*args, **kwargs)

def locked_run(command, *args, **kwargs):
    try:
        return original_run(command, *args, **kwargs)
    finally:
        if isinstance(command, (list, tuple)) and any(str(part).endswith('/tools/make_test_summary.py') for part in command):
            fcntl.flock(record_lock, fcntl.LOCK_UN)
runner.update_readme = locked_update
runner.subprocess.run = locked_run
runner.main()
