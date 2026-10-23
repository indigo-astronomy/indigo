#!/usr/bin/env python3
"""Plan/apply independent finding storage and evidenced completion history."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import findings
import maintenance
import task_store as store

ROOT = Path(__file__).resolve().parents[2]
DERIVED = {'completed_non_audit_tasks_since_full_review', 'last_full_review_task', 'last_full_review_date',
           'last_full_review_revision', 'last_task_review_task'}


def git(root, *args):
    result = subprocess.run(['git', '--no-replace-objects', '--no-lazy-fetch', '-C', str(root), *args], capture_output=True, text=True)
    if result.returncode:
        raise ValueError('Git operation failed: ' + args[0])
    return result.stdout.strip()


def prepare(root, task, manifest):
    store.valid_task(task)
    if Path(git(root, 'rev-parse', '--show-toplevel')).resolve() != root.resolve():
        raise ValueError('project must be the Git repository root')
    if git(root, 'branch', '--show-current') != 'task/' + task:
        raise ValueError('migration requires the matching upgrade task branch')
    _, state = store.selected_state(root, task)
    if state.get('status') not in ('active', 'blocked'):
        raise ValueError('migration requires an unfinished upgrade task')
    if not isinstance(manifest, dict) or set(manifest) != {'version', 'template_export_baseline', 'records'} or manifest['version'] != 1 or type(manifest['template_export_baseline']) is not bool or not isinstance(manifest['records'], list):
        raise ValueError('reviewed completion manifest requires version, template_export_baseline and records')
    values = manifest['records']
    for record in values:
        maintenance.validate(record)
        if record['evidence_revision'] is None:
            raise ValueError('imported completion needs a retained evidence revision')
    if len({r['task'] for r in values}) != len(values):
        raise ValueError('duplicate imported completion task')
    old_findings_root = findings.ROOT
    try:
        findings.ROOT = root
        data = findings.active_register()
        findings.check_tasks(findings.all_records(data), set())
    finally:
        findings.ROOT = old_findings_root
    config_path = root / maintenance.CONFIG
    original_config = store.loads(store.read(root, config_path))
    if not isinstance(original_config, dict):
        raise ValueError('legacy maintenance must be an object')
    writes, deletes = {}, []
    backup = root / '.factory/tasks' / task
    if data['version'] == 1:
        writes[backup / 'legacy-findings.json'] = store.read(root, root / findings.ACTIVE)
        for record in data['findings']:
            writes[root / findings.ACTIVE_DIR / (record['id'] + '.json')] = maintenance_bytes(record)
        writes[root / findings.ACTIVE_DIR / '.gitkeep'] = b''
        writes[root / findings.REGISTRY] = maintenance_bytes({'version': 2, 'storage': 'per-record'})
        deletes.append(root / findings.ACTIVE)
    if original_config.get('version', 1) == 1:
        policy = dict(original_config)
        review = dict(policy.get('repository_review', {}))
        policy.update(version=2, timezone=policy.get('timezone', 'UTC'))
        policy['repository_review'] = {k: v for k, v in review.items() if k not in DERIVED}
        # Validate policy and imported history through a disposable in-memory snapshot.
        virtual = Virtual(root, policy, values)
        maintenance.config(root, virtual)
        maintenance.check(root, virtual)
        projected = maintenance.status(policy, values, None)
        if not manifest['template_export_baseline']:
            for key in DERIVED:
                if key in review and review[key] != projected[key]:
                    raise ValueError('imported completions do not reproduce legacy maintenance field: ' + key)
        else:
            text = store.read(root, root / '.factory/project.md').decode()
            if 'Status: TEMPLATE' not in text:
                raise ValueError('template export baseline exception is only for the template repository')
        # Every retained complete task must be represented; unfinished upgrade is excluded.
        known = {r['task'] for r in values}
        for parent in (root / '.factory/tasks', root / '.factory/tasks/archive'):
            store.local(root, parent)
            if parent.exists():
                for directory in parent.iterdir():
                    store.local(root, directory)
                    state_file = directory / 'state.json'
                    if directory.name != 'archive' and directory.is_dir() and state_file.exists():
                        task_state = store.loads(store.read(root, state_file))
                        if task_state.get('status') == 'complete' and directory.name not in known:
                            raise ValueError('retained completed task missing from imported history: ' + directory.name)
        writes[backup / 'legacy-maintenance.json'] = store.read(root, config_path)
        for record in values:
            writes[root / maintenance.EVENTS / (record['task'] + '.json')] = maintenance_bytes(record)
        writes[root / maintenance.EVENTS / '.gitkeep'] = b''
        writes[config_path] = maintenance_bytes(policy)
    else:
        current = maintenance.check(root)
        if {r['task']: r for r in current} != {r['task']: r for r in values}:
            raise ValueError('already-migrated history differs; do not overwrite completion records')
    inputs = {}
    candidates = {config_path, root / findings.ACTIVE, root / findings.REGISTRY}
    for parent in (root / '.factory/findings', root / maintenance.EVENTS):
        store.local(root, parent)
        if parent.exists():
            for path in parent.rglob('*'):
                store.local(root, path)
                if path.is_file():
                    candidates.add(path)
    for path in candidates:
        store.local(root, path)
        inputs[path.relative_to(root).as_posix()] = [hashlib.sha256(store.read(root, path)).hexdigest(), path.stat().st_mode & 0o777] if path.exists() else None
    for path in writes:
        store.local(root, path)
        if path.exists() and path not in (config_path,):
            raise ValueError('migration destination exists; reconcile explicitly')
    plan = {'version': 1, 'task': task, 'head': git(root, 'rev-parse', 'HEAD'), 'manifest': manifest,
            'inputs': inputs, 'writes': {p.relative_to(root).as_posix(): hashlib.sha256(raw).hexdigest() for p, raw in writes.items()},
            'deletes': [p.relative_to(root).as_posix() for p in deletes]}
    return plan, writes, deletes


def maintenance_bytes(value):
    return (json.dumps(value, indent=2, allow_nan=False) + '\n').encode()


class Virtual:
    """Use existing immutable completion proof validation for reviewed import data."""
    def __init__(self, root, policy, records):
        from git_snapshot import Snapshot
        self.head = Snapshot(root, 'HEAD')
        self.values = {maintenance.CONFIG: policy, **{maintenance.EVENTS + '/' + r['task'] + '.json': r for r in records}}
        self.entries = dict(self.head.entries)
        self.entries[maintenance.EVENTS + '/.gitkeep'] = ('100644', 'blob', '')
        self.entries.update({p: ('100644', 'blob', '') for p in self.values})
    def exists(self, path):
        return path in self.entries or path in self.values or any(p.startswith(path + '/') for p in self.entries)
    def json(self, path):
        return self.values[path] if path in self.values else self.head.json(path)
    def at(self, revision):
        return self.head.at(revision)
    def ancestor(self, revision):
        return self.head.ancestor(revision)


def apply(root, plan):
    actual, writes, deletes = prepare(root, plan['task'], plan['manifest'])
    if actual != plan:
        raise ValueError('migration plan is stale or edited; regenerate and review')
    before = {p: store.read(root, p) if p.exists() else None for p in [*writes, *deletes]}
    modes = {p: p.stat().st_mode & 0o777 if p.exists() else None for p in before}
    created, touched = [], []
    try:
        for path, raw in writes.items():
            missing = []
            parent = path.parent
            while not parent.exists():
                missing.append(parent)
                parent = parent.parent
            for directory in reversed(missing):
                directory.mkdir()
                created.append(directory)
            source = None
            if path.name == 'legacy-findings.json' or path.parent == root / findings.ACTIVE_DIR:
                source = root / findings.ACTIVE
            elif path.name == 'legacy-maintenance.json':
                source = root / maintenance.CONFIG
            mode = source.stat().st_mode & 0o777 if source is not None and source.exists() else None
            touched.append(path)
            store.atomic(path, raw, mode=mode)
        for path in deletes:
            touched.append(path)
            path.unlink()
    except (OSError, ValueError):
        try:
            for path in reversed(touched):
                if before[path] is None:
                    path.unlink(missing_ok=True)
                else:
                    store.atomic(path, before[path], mode=modes[path])
            for directory in reversed(created):
                directory.rmdir()
        except OSError:
            raise ValueError('metadata migration rollback failed; preserve checkout and recover from Git') from None
        raise ValueError('metadata migration failed; original files restored') from None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=ROOT)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--plan', metavar='TASK')
    group.add_argument('--apply', type=Path, metavar='PLAN')
    parser.add_argument('--completions', type=Path, help='reviewed import manifest, required for --plan')
    args = parser.parse_args()
    root = args.project.resolve()
    try:
        def load(path):
            with path.open('rb') as stream:
                raw = stream.read(16 * store.LIMIT + 1)
            if len(raw) > 16 * store.LIMIT:
                raise ValueError('migration input exceeds 16 MiB')
            return store.loads(raw)
        if args.plan:
            if args.completions is None:
                raise ValueError('--plan requires --completions')
            plan, _, _ = prepare(root, args.plan, load(args.completions))
            print(json.dumps(plan, indent=2))
        else:
            with store.lifecycle_lock(root):
                apply(root, load(args.apply))
            print('Findings and maintenance migrated; legacy IDs/evidence preserved.')
    except (OSError, ValueError, TypeError, KeyError, RuntimeError) as exc:
        print('Metadata upgrade failed: ' + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
