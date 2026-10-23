"""Regenerate every in-scope driver in a disposable tree; never write repository outputs."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import difflib
import json
import shutil
import subprocess
import tempfile

report = Path(__file__).resolve().parent
repository = Path.cwd()
definition_paths = [entry['path'] for entry in json.loads((report / 'driver-inventory.json').read_text()) if entry['path'] != 'indigo_tools/template.driver']

def inspect(filename):
    path = repository / filename
    result = {'path': filename, 'differences': []}
    with tempfile.TemporaryDirectory(prefix='indigo-audit-generate-') as temporary:
        root = Path(temporary)
        work = root / 'indigo_drivers' / 'audit'
        work.mkdir(parents=True)
        bases = root / 'indigo_libs'
        bases.mkdir()
        for base in (repository / 'indigo_libs').glob('indigo_*_driver.c'):
            shutil.copyfile(base, bases / base.name)
        shutil.copyfile(path, work / path.name)
        process = subprocess.run(['/tmp/indigo-audit-generator-20261023', path.name], cwd=work, text=True, capture_output=True, timeout=20)
        result['exit'] = process.returncode
        result['diagnostic'] = process.stderr + process.stdout
        generated = [p for p in work.iterdir() if p.suffix in {'.c', '.cpp', '.h'}]
        result['outputs'] = [p.name for p in generated]
        for output in generated:
            existing = path.parent / output.name
            if not existing.exists():
                result['differences'].append({'output': output.name, 'kind': 'missing checked-in output'})
            elif existing.read_bytes() != output.read_bytes():
                changes = list(difflib.unified_diff(existing.read_text().splitlines(), output.read_text().splitlines(), n=1))
                result['differences'].append({'output': output.name, 'kind': 'content differs', 'diff_lines': len(changes), 'excerpt': changes[:35]})
    return result

with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(inspect, definition_paths))
(report / 'generation-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps({'definitions': len(results), 'generation_failures': [r['path'] for r in results if r['exit'] != 0], 'different': [{'path': r['path'], 'outputs': len(r['differences'])} for r in results if r['differences']]}, indent=2))
