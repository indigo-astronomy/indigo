import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
sys.path.insert(0, '.factory/scripts')
from findings_project import OBJECT, CHILDREN
path = Path('indigo.xcodeproj/project.pbxproj')
text = path.read_text()
p = json.loads(subprocess.check_output(['plutil','-convert','json','-o','-',str(path)]))
objects = p['objects']; groups = {}; files = set()
def walk(key, base):
    obj = objects[key]
    if obj.get('sourceTree','<group>') not in ('<group>','SOURCE_ROOT'): return
    resolved = Path(obj.get('path','')) if obj.get('sourceTree') == 'SOURCE_ROOT' else base / obj.get('path','')
    name = resolved.as_posix()
    if obj['isa'] == 'PBXGroup': groups[name] = key
    if obj['isa'] == 'PBXFileReference': files.add(name)
    for child in obj.get('children',[]): walk(child, resolved)
walk(objects[p['rootObject']]['mainGroup'], Path('.'))
blocks = {m[1]:m for m in OBJECT.finditer(text)}
children = {}; additions = {'PBXGroup':[], 'PBXFileReference':[]}
def identifier(name):
    key = hashlib.sha256(('indigo-navigator:' + name).encode()).hexdigest()[:24].upper()
    if key in objects: raise ValueError('PBX collision')
    return key

def ensure_group(name):
    if name in groups: return groups[name]
    parent = Path(name).parent.as_posix()
    parent_key = ensure_group(parent)
    key = identifier(name); groups[name] = key
    children.setdefault(parent_key, []).append((key, Path(name).name))
    children[key] = []
    return key
names = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z','--','.factory','indigo_test/factory/test_findings_project.py']).decode().split('\0')
count = 0
for name in sorted(set(names)-{''}):
    if name in files or not Path(name).is_file() or '/__pycache__/' in name: continue
    if name.startswith('.factory/tasks/20261023195506-cdf8-fix-findings-xcode/'):continue
    group = ensure_group(Path(name).parent.as_posix())
    key = identifier(name); label = Path(name).name
    children.setdefault(group,[]).append((key,label))
    file_type = {'.json':'text.json','.py':'text.script.python','.md':'net.daringfireball.markdown'}.get(Path(name).suffix,'text')
    additions['PBXFileReference'].append(f'\t\t{key} /* {label} */ = {{isa = PBXFileReference; lastKnownFileType = {file_type}; path = "{label}"; sourceTree = "<group>"; }};\n')
    count += 1
edits = []
for name, key in groups.items():
    if key not in children:continue
    listing = ''.join(f'\t\t\t\t{k} /* {label} */,\n' for k,label in children[key])
    if key in blocks:
        block=blocks[key];match=CHILDREN.search(block[0]); offset=block.start()+match.end(1)
        edits.append((offset,offset,listing))
    else:
        label=Path(name).name
        additions['PBXGroup'].append(f'\t\t{key} /* {label} */ = {{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n{listing}\t\t\t);\n\t\t\tpath = "{label}";\n\t\t\tsourceTree = "<group>";\n\t\t}};\n')
for section, lines in additions.items():
    offset=text.index(f'/* End {section} section */');edits.append((offset,offset,''.join(lines)))
for start,end,value in sorted(edits,reverse=True):text=text[:start]+value+text[end:]
path.write_text(text)
print('Added navigator references:',count)
