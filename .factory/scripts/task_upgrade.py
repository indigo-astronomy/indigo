#!/usr/bin/env python3
"""Preview/apply ordered task identities and task-local execution state."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

import task_store as store

ROOT = Path(__file__).resolve().parents[2]
TEXT_SUFFIXES = ('.md', '.json', '.txt', '.yaml', '.yml', '.toml', '.sh', '.py', '.mdc')
SHA = re.compile(r'(?<![0-9a-f])[0-9a-f]{40}(?:[0-9a-f]{24})?(?![0-9a-f])')


def git(root, *args):
    result = subprocess.run(['git', '--no-replace-objects', '--no-lazy-fetch', '-C', str(root), *args], capture_output=True)
    if result.returncode:
        raise ValueError('Git operation failed: ' + args[0])
    return result.stdout.decode().strip()


def task_directories(root):
    tasks = store.local(root, root / '.factory/tasks')
    result = []
    for parent in (tasks, tasks / 'archive'):
        store.local(root, parent)
        if not parent.exists():
            continue
        if not parent.is_dir():
            raise ValueError('task archive must be a real directory')
        for path in sorted(parent.iterdir()):
            store.local(root, path)
            if path.name == 'archive' or path.name.startswith('.'):
                continue
            if not path.is_dir():
                raise ValueError('unexpected task entry')
            store.valid_task(path.name)
            result.append(path)
    ids = [p.name for p in result]
    if len(ids) != len(set(ids)):
        raise ValueError('duplicate live/archive task ID; reconcile before upgrading')
    return result


def inventory(root, paths):
    names = set(git(root, 'ls-files', '-z', '--cached', '--others', '--exclude-standard').split('\0')) - {''}
    # Archives are explicitly selected migration input, including ignored retained reports.
    selected = paths + [root / '.factory/findings']
    for directory in selected:
        store.local(root, directory)
        if directory.exists():
            for path in directory.rglob('*'):
                store.local(root, path)
                if path.is_file():
                    names.add(path.relative_to(root).as_posix())
                elif not path.is_dir():
                    raise ValueError('nonregular migration input')
    result = {}
    for name in sorted(names):
        path = store.local(root, root / name)
        if path.exists():
            if not path.is_file():
                raise ValueError('migration input must be a regular file')
            if not name.endswith(TEXT_SUFFIXES):
                continue
            result[name] = store.read(root, path)
    return result


def substitute(text, ids):
    if not ids:
        return text
    pattern = re.compile(r'(?<![a-z0-9-])(' + '|'.join(re.escape(k) for k in ids) + r')(?![a-z0-9-])')
    lines = []
    for line in text.splitlines(keepends=True):
        # Conservative: a full commit pin makes the entire line historical.
        if SHA.search(line):
            lines.append(line)
            continue
        chunks = re.split(r'([a-zA-Z][a-zA-Z0-9+.-]*://[^\s<>)]*)', line)
        lines.append(''.join(chunk if index % 2 else pattern.sub(lambda match: ids[match.group(0)], chunk)
                             for index, chunk in enumerate(chunks)))
    return ''.join(lines)


def rewrite_json(value, ids, key=None):
    if key in ('reviewed_content', 'reviews'):
        return value
    if isinstance(value, dict):
        if 'revision' in value and value['revision'] is not None:
            return value
        return {k: rewrite_json(v, ids, k) for k, v in value.items()}
    if isinstance(value, list):
        return [rewrite_json(v, ids, key) for v in value]
    return substitute(value, ids) if isinstance(value, str) else value


def serialized(value):
    return (json.dumps(value, indent=2, allow_nan=False) + '\n').encode()


def prepare(root, task):
    store.valid_task(task)
    if Path(git(root, 'rev-parse', '--show-toplevel')).resolve() != root.resolve():
        raise ValueError('--project must be the repository root')
    branch = git(root, 'symbolic-ref', '--short', 'HEAD')
    if branch != 'task/' + task:
        raise ValueError('migration requires the matching upgrade task branch')
    _, state = store.selected_state(root, task)
    if state.get('status') not in ('active', 'blocked'):
        raise ValueError('migration requires an unfinished upgrade task')
    paths = task_directories(root)
    contents = inventory(root, paths)
    repository = store.loads(contents['.factory/state.json'])
    mapping = {}
    used = {p.name for p in paths}
    root_identity = git(root, 'rev-parse', '--show-toplevel')
    for path in paths:
        if store.ORDERED.fullmatch(path.name):
            continue
        if path.parent.name != 'archive' and path.name != task:
            raise ValueError('another legacy live task needs an explicit handoff before migration')
        name = path.relative_to(root).as_posix()
        history = git(root, 'log', '--reverse', '--diff-filter=A', '--format=%aI', '--', name)
        if history:
            moment = datetime.fromisoformat(history.splitlines()[0])
            time_source = 'first retained Git addition (author time)'
        else:
            store.read(root, path / 'brief.md')
            moment = datetime.fromtimestamp((path / 'brief.md').stat().st_mtime, timezone.utc)
            time_source = 'brief.md modification time; review fallback'
        slug = re.sub(r'^\d+-', '', path.name)
        seed = int(hashlib.sha256((name + '\0' + root_identity).encode()).hexdigest()[:4], 16)
        for attempt in range(65536):
            suffix = format((seed + attempt) % 65536, '04x')
            new = store.allocate(slug, moment, suffix)
            if new not in used:
                break
        else:
            raise ValueError('migration task ID space exhausted')
        used.add(new)
        mapping[name] = {'id': new, 'time_source': time_source}
    ids = {Path(name).name: value['id'] for name, value in mapping.items()}
    new_task = ids.get(task, task)
    if new_task != task:
        branches = git(root, 'for-each-ref', '--format=%(refname:short)', 'refs/heads').splitlines()
        if 'task/' + new_task in branches:
            raise ValueError('destination task branch already exists')
    writes = {}
    for name, raw in contents.items():
        if name.endswith('/registry.json') and name.startswith('.factory/findings/'):
            continue
        if '/reviews/' in name or name == '.factory/template-lock.json':
            continue
        try:
            text = raw.decode('utf-8')
        except UnicodeError:
            continue
        if name.endswith('.json'):
            value = store.loads(raw)
            updated = rewrite_json(value, ids)
            after = serialized(updated) if updated != value else raw
        else:
            after = substitute(text, ids).encode()
        if after != raw:
            writes[root / name] = after
    global_path = root / '.factory/state.json'
    if repository.get('version', 1) == 1:
        execution_keys = {'active_task', 'phase', 'status', 'assigned_role', 'next_action', 'blocker'}
        global_data = {k: v for k, v in repository.items() if k not in execution_keys}
        global_data.update(version=2, task_state='per-task')
        writes[global_path] = serialized(global_data)
        for path in paths:
            state_file = store.local(root, path / 'state.json')
            if state_file.exists():
                raise ValueError('legacy task already has state.json; reconcile explicitly')
            if path.name == task:
                data = dict(rewrite_json(repository, ids), version=2)
            elif path.parent.name == 'archive':
                data = dict(version=2, active_task=ids.get(path.name, path.name), phase='complete', status='complete',
                            assigned_role='coordinator', next_action='Historical archived task; do not resume.', blocker=None)
            else:
                raise ValueError('another legacy live task needs an explicit handoff before migration')
            writes[state_file] = serialized(data)
    moves = {root / name: (root / name).with_name(value['id']) for name, value in mapping.items()}
    for source in moves:
        if (root / '.factory/completions' / (source.name + '.json')).exists():
            raise ValueError('rename legacy task IDs before migrating completion history')
        for name in contents:
            if name.startswith(('.factory/findings/active/', '.factory/findings/archive/')) and Path(name).name.startswith('FND-' + source.name + '-'):
                raise ValueError('rename legacy task IDs before allocating task-qualified findings')
    for destination in moves.values():
        store.local(root, destination)
        if destination.exists():
            raise ValueError('migration destination exists')
    plan = {'version': 1, 'task': task, 'new_task': new_task, 'branch': branch,
            'head': git(root, 'rev-parse', 'HEAD'), 'mapping': mapping,
            'inputs': {name: [hashlib.sha256(raw).hexdigest(), (root / name).stat().st_mode & 0o777]
                       for name, raw in contents.items()},
            'writes': [p.relative_to(root).as_posix() for p in sorted(writes)]}
    return plan, writes, moves


def build_plan(root, task):
    plan, _, _ = prepare(root, task)
    return plan


def apply_plan(root, plan):
    actual, writes, moves = prepare(root, plan['task'])
    if plan != actual:
        raise ValueError('migration plan is stale or edited; regenerate and review it')
    global_path = root / '.factory/state.json'
    ordered = sorted(writes, key=lambda path: path == global_path)
    before = {p: store.read(root, p) if p.exists() else None for p in ordered}
    written, moved = [], []
    renamed = False
    try:
        for path in ordered:
            written.append(path)
            store.atomic(path, writes[path])
        for source, destination in moves.items():
            source.rename(destination)
            moved.append((source, destination))
        if plan['new_task'] != plan['task']:
            git(root, 'branch', '-m', 'task/' + plan['new_task'])
            renamed = True
    except (OSError, ValueError):
        try:
            if renamed:
                git(root, 'branch', '-m', plan['branch'])
            for source, destination in reversed(moved):
                destination.rename(source)
            for path in reversed(written):
                if before[path] is None:
                    path.unlink(missing_ok=True)
                else:
                    store.atomic(path, before[path])
        except (OSError, ValueError):
            raise ValueError('migration rollback failed; preserve checkout and recover using Git and the reviewed plan') from None
        raise ValueError('migration failed; original files, task paths and branch restored') from None
    return plan['new_task']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=ROOT)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--plan', metavar='TASK', help='print a read-only JSON task migration plan')
    group.add_argument('--apply', type=Path, metavar='PLAN', help='apply the exact reviewed JSON plan')
    args = parser.parse_args()
    root = args.project.resolve()
    try:
        if args.plan:
            print(json.dumps(build_plan(root, args.plan), indent=2))
        else:
            with args.apply.open('rb') as stream:
                raw = stream.read(16 * store.LIMIT + 1)
            if len(raw) > 16 * store.LIMIT:
                raise ValueError('migration plan exceeds 16 MiB')
            plan = store.loads(raw)
            with store.lifecycle_lock(root):
                task = apply_plan(root, plan)
            print('Migrated tasks; active upgrade task: ' + task)
    except (OSError, ValueError, KeyError, TypeError, RecursionError) as exc:
        print('Task upgrade failed: ' + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
