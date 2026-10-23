"""Read-only Clang analyzer evidence for the repository audit; not a product test."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import subprocess

root = Path(__file__).resolve().parent
paths = json.loads((root / 'analyzer-targets.json').read_text())

def analyze(path):
    command = ['clang', '--analyze', '-Xanalyzer', '-analyzer-output=text', '-fno-color-diagnostics', '-std=gnu11', '-DINDIGO_MACOS', '-Duint=unsigned', '-Iindigo_libs', '-Ibuild/include', '-Iindigo_drivers', '-o', '/dev/null', path]
    try:
        run = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
        return {'path': path, 'command': command, 'exit': run.returncode, 'diagnostics': run.stdout}
    except subprocess.TimeoutExpired:
        return {'path': path, 'command': command, 'exit': None, 'diagnostics': 'Analyzer exceeded 40-second per-file timeout.'}

with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(analyze, paths))
(root / 'analyzer-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps({'files': len(results), 'completed': sum(r['exit'] == 0 for r in results), 'unavailable': sum(r['exit'] != 0 for r in results), 'warning_files': [r['path'] for r in results if 'warning:' in r['diagnostics']]}, indent=2))
