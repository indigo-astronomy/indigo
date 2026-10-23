"""Maintain findings navigator references without rewriting unrelated PBX objects.

Supports Xcode's line-oriented OpenStep object layout; rejects unsupported or
ambiguous findings groups before changing anything. No macOS tools required.
"""
import hashlib
import re
import task_store

def project_path(root):
    projects = sorted(root.glob('*.xcodeproj'))
    if len(projects) > 1:
        raise ValueError('multiple root Xcode projects; select and reconcile them explicitly')
    return projects[0] / 'project.pbxproj' if projects else None
OBJECT = re.compile(r'^\t\t([A-F0-9]{24})(?: /\*[^\n]*?\*/)? = \{(?:[^\n]*\};\n|\n.*?^\t\t\};\n)', re.M | re.S)
CHILDREN = re.compile(r'children = \(\n(.*?)^\t\t\t\);', re.M | re.S)
REFERENCE = re.compile(r'\s*([A-F0-9]{24})(?: /\*[^\n]*?\*/)?\s*,')


def field(text, name):
    match = re.search(r'\b' + name + r' = ("[^"\n]*"|[^;\n]+);', text)
    return match.group(1).strip('"') if match else None


def plan(root):
    candidate = project_path(root)
    if candidate is None:
        return None
    path = task_store.local(root, candidate)
    if not path.exists():
        raise ValueError('Xcode project.pbxproj is missing')
    if not path.is_file() or path.stat().st_size > 16 * 1024 * 1024:
        raise ValueError('unsupported Xcode project input')
    before = path.read_bytes()
    text = before.decode('utf-8')
    objects = {}
    for match in OBJECT.finditer(text):
        if match[1] in objects:
            raise ValueError('duplicate PBX object ID')
        objects[match[1]] = match
    groups = {key: match for key, match in objects.items() if field(match[0], 'isa') == 'PBXGroup'}
    parents = {}
    for key, match in groups.items():
        children = CHILDREN.search(match[0])
        if children:
            for child in REFERENCE.findall(children[1]):
                parents.setdefault(child, []).append(key)

    def group_path(key, seen=()):
        if key in seen or len(parents.get(key, [])) > 1:
            raise ValueError('ambiguous or cyclic Xcode group hierarchy')
        body = groups[key][0]
        tree = field(body, 'sourceTree')
        if tree not in ('<group>', 'SOURCE_ROOT'):
            return None
        part = field(body, 'path') or ''
        parent = parents.get(key, [])
        base = group_path(parent[0], (*seen, key)) if tree == '<group>' and parent else ''
        return (base + '/' + part).strip('/') if base is not None else None

    selected = {}
    for key in groups:
        resolved = group_path(key)
        if resolved in ('.factory/findings/active', '.factory/findings/archive'):
            if resolved in selected:
                raise ValueError('duplicate Xcode findings group')
            selected[resolved] = key
    if len(selected) != 2:
        raise ValueError('Xcode project must contain active and archive findings groups')
    wanted = {}
    for directory in selected:
        folder = task_store.local(root, root / directory)
        names = set()
        for entry in folder.iterdir():
            if entry.name.startswith('FND-') and entry.suffix == '.json':
                task_store.local(root, entry)
                if not entry.is_file() or not re.fullmatch(r'FND-[a-zA-Z0-9-]+\.json', entry.name):
                    raise ValueError('unsupported finding file')
                names.add(entry.name)
        wanted[directory] = names
    existing = {}
    retained = {directory: [] for directory in selected}
    edits = []
    for directory, key in selected.items():
        block = groups[key]
        children = CHILDREN.search(block[0])
        if not children:
            raise ValueError('unsupported findings children list')
        if REFERENCE.sub('', children[1]).strip():
            raise ValueError('unsupported findings child syntax')
        for child in REFERENCE.findall(children[1]):
            obj = objects.get(child)
            if obj is None or field(obj[0], 'isa') != 'PBXFileReference' or field(obj[0], 'sourceTree') != '<group>':
                raise ValueError('findings children must be local file references')
            name = field(obj[0], 'path')
            if name == '.gitkeep':
                retained[directory].append(f'\t\t\t\t{child} /* .gitkeep */,\n')
                continue
            if not name or not re.fullmatch(r'FND-[a-zA-Z0-9-]+\.json', name):
                raise ValueError('unsupported findings reference path')
            if parents.get(child) != [key] or re.search(r'\bfileRef = ' + child + r'\b', text):
                raise ValueError('finding reference is shared or has build membership')
            existing.setdefault(name, []).append(child)
    additions = []
    used = set()
    for directory, key in selected.items():
        lines = list(retained[directory])
        for name in sorted(wanted[directory]):
            if name in used:
                raise ValueError('finding exists in both active and archive')
            used.add(name)
            ids = existing.get(name, [])
            if ids:
                child = ids[0]
            else:
                child = hashlib.sha256(('factory-finding:' + name).encode()).hexdigest()[:24].upper()
                if child in objects:
                    raise ValueError('generated PBX ID collision')
                additions.append(f'\t\t{child} /* {name} */ = {{isa = PBXFileReference; lastKnownFileType = text.json; path = "{name}"; sourceTree = "<group>"; }};\n')
            lines.append(f'\t\t\t\t{child} /* {name} */,\n')
        block = groups[key]
        children = CHILDREN.search(block[0])
        edits.append((block.start() + children.start(1), block.start() + children.end(1), ''.join(lines)))
    for name, ids in existing.items():
        for child in ids[1:] if name in used else ids:
            match = objects[child]
            edits.append((match.start(), match.end(), ''))
    marker = '/* End PBXFileReference section */'
    if text.count(marker) != 1:
        raise ValueError('unsupported PBX file reference section')
    offset = text.index(marker)
    edits.append((offset, offset, ''.join(additions)))
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    return path, before, text.encode('utf-8')


def sync(root, check=False):
    change = plan(root)
    if change is None or change[1] == change[2]:
        return False
    if check:
        raise ValueError('Xcode findings references are stale; run findings sync-project')
    task_store.atomic(change[0], change[2])
    return True
