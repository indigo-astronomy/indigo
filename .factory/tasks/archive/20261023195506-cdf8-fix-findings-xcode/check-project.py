import json
from pathlib import Path
import subprocess

def read_project(raw):
    return json.loads(subprocess.check_output(['plutil', '-convert', 'json', '-o', '-', '--', '-'], input=raw))
p = read_project(Path('indigo.xcodeproj/project.pbxproj').read_bytes())
base = read_project(subprocess.check_output(['git','show','a23045210:indigo.xcodeproj/project.pbxproj']))
objects = p['objects']; paths = {}; group_paths = {}
def walk(key, parent, trail=()):
    assert key not in trail, 'cycle'
    obj = objects[key]
    tree = obj.get('sourceTree', '<group>')
    if tree not in ('<group>', 'SOURCE_ROOT'):return
    path = Path(obj.get('path','')) if tree == 'SOURCE_ROOT' else parent / obj.get('path','')
    if obj['isa'] == 'PBXFileReference':
        assert key not in paths, 'shared reference'
        paths[key] = path.as_posix()
    if obj['isa'] == 'PBXGroup':group_paths[key] = path.as_posix()
    for child in obj.get('children',[]):walk(child, path, (*trail,key))
walk(objects[p['rootObject']]['mainGroup'], Path('.'))
for directory in ('active','archive'):
    prefix = '.factory/findings/' + directory + '/'
    actual = [v for v in paths.values() if v.startswith(prefix) and v.endswith('.json')]
    expected = {path.as_posix() for path in Path(prefix).glob('FND-*.json')}
    assert len(actual) == len(set(actual)), 'duplicate findings path'
    assert set(actual) == expected, (directory, expected-set(actual), set(actual)-expected)
    print(directory, len(actual), 'references match disk exactly')
tracked = set(subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z','--','.factory']).decode().split('\0'))-{''}
tracked = {name for name in tracked if Path(name).is_file() and not name.startswith('.factory/tasks/20261023195506-cdf8-fix-findings-xcode/')}
assert not tracked - set(paths.values()), sorted(tracked - set(paths.values()))
assert not [v for v in paths.values() if v.startswith('.factory/') and not Path(v).exists()]
changed = []
for key,obj in base['objects'].items():
    if objects.get(key) != obj:
        assert obj['isa']=='PBXGroup', ('changed non-group object', key)
        path = group_paths.get(key,'')
        assert path.startswith('.factory') or path == 'indigo_test/factory', ('unexpected changed group', key,path)
        changed.append(path)
print('All current Factory inventory references exist; no missing Factory references.')
print('All pre-existing non-group PBX objects unchanged; changed groups restricted to Factory navigator.')
print('Changed groups:',len(changed))
