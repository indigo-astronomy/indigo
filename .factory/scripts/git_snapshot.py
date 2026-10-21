"""Read regular UTF-8 blobs from immutable local Git commits without fetching or checking out."""
import json
from pathlib import Path
import re
import subprocess

LIMIT = 1024 * 1024


def git(root, *args):
    result = subprocess.run(['git', '--no-replace-objects', '--no-lazy-fetch', '--no-optional-locks',
                             *args], cwd=root, capture_output=True)
    if result.returncode:
        raise ValueError('local Git operation unavailable: ' + args[0])
    return result.stdout


def object_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


class Snapshot:
    def __init__(self, root, revision):
        self.root = Path(root)
        self.revision = git(root, 'rev-parse', '--verify', '--end-of-options', revision + '^{commit}').decode().strip()
        self.entries = {}
        for entry in git(root, 'ls-tree', '-rz', self.revision).split(b'\0'):
            if not entry:
                continue
            meta, name = entry.split(b'\t', 1)
            mode, kind, oid = meta.decode().split()
            self.entries[name.decode('utf-8')] = (mode, kind, oid)

    def exists(self, name):
        return name in self.entries or any(p.startswith(name.rstrip('/') + '/') for p in self.entries)

    def read(self, name):
        if name not in self.entries or self.entries[name][0] not in ('100644', '100755'):
            raise ValueError('required input missing or not a regular Git blob: ' + name)
        oid = self.entries[name][2]
        size = int(git(self.root, 'cat-file', '-s', oid))
        if size > LIMIT:
            raise ValueError('Git text input exceeds 1 MiB: ' + name)
        return git(self.root, 'cat-file', 'blob', oid).decode('utf-8')

    def json(self, name):
        return json.loads(self.read(name), object_pairs_hook=object_pairs)

    def at(self, revision):
        return Snapshot(self.root, revision)

    def ancestor(self, older):
        if not re.fullmatch(r'[0-9a-f]{40}|[0-9a-f]{64}', older):
            raise ValueError('ancestor must be a full commit SHA')
        result = subprocess.run(['git', '--no-replace-objects', '--no-lazy-fetch', '--no-optional-locks',
                                 'merge-base', '--is-ancestor', older, self.revision],
                                cwd=self.root, capture_output=True)
        if result.returncode not in (0, 1):
            raise ValueError('candidate ancestry unavailable')
        return result.returncode == 0
