#!/usr/bin/env python3
"""Manage and validate local task-scoped findings."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ACTIVE = ".factory/findings/active.json"
REGISTRY = ".factory/findings/registry.json"
ACTIVE_DIR = ".factory/findings/active"
ARCHIVE = ".factory/findings/archive"
MAX_BYTES = 1024 * 1024
GIT_FLAGS = ("--no-replace-objects", "--no-lazy-fetch", "--no-optional-locks")
ID = re.compile(r"FND-(?:[0-9]{6}|[a-z0-9]+(?:-[a-z0-9]+)*-[0-9]{3,})")
LEGACY_ID = re.compile(r"FND-[0-9]{6}")
TASK = re.compile(r"[a-z0-9]+(?:-[a-z0-9]+)*")
REVISION = re.compile(r"(?:[0-9a-f]{40}|[0-9a-f]{64})")
FIELDS = {"id", "title", "category", "severity", "status", "owner", "blocking", "tasks", "summary", "evidence", "resolution"}


def nonempty(value, limit=2000):
    return isinstance(value, str) and bool(value.strip()) and len(value) <= limit and not any(ord(c) < 32 for c in value)


def matches(pattern, value):
    return isinstance(value, str) and pattern.fullmatch(value) is not None


def local(relative):
    path = ROOT / relative
    path.resolve().relative_to(ROOT.resolve())
    current = ROOT
    for part in path.relative_to(ROOT).parts:
        current /= part
        if current.is_symlink():
            raise ValueError("symbolic links are not supported for findings")
    return path


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def read_json(relative):
    path = local(relative)
    if not path.is_file():
        raise ValueError("required registry input is missing or not a regular file")
    with path.open("rb") as stream:
        raw = stream.read(MAX_BYTES + 1)
    if len(raw) > MAX_BYTES:
        raise ValueError("registry input exceeds 1 MiB")
    return json.loads(raw.decode("utf-8"), object_pairs_hook=unique_object)


def validate_record(record, archived=False):
    if not isinstance(record, dict) or set(record) != FIELDS:
        raise ValueError("finding fields do not match the schema")
    if not matches(ID, record["id"]) or record["id"] == "FND-000000":
        raise ValueError("invalid finding ID")
    if not all(nonempty(record[key]) for key in ("title", "owner", "summary", "evidence")):
        raise ValueError("finding needs nonempty title, owner, summary and evidence")
    if record["category"] not in ("functional", "security", "quality"):
        raise ValueError("invalid finding category")
    if record["severity"] not in ("critical", "high", "medium", "low", "informational"):
        raise ValueError("invalid finding severity")
    statuses = ("closed",) if archived else ("open", "in_progress", "blocked", "deferred")
    if record["status"] not in statuses or type(record["blocking"]) is not bool:
        raise ValueError("invalid status or blocking flag for this register")
    tasks = record["tasks"]
    if not isinstance(tasks, list) or not tasks:
        raise ValueError("finding needs at least one task/report reference")
    seen = set()
    for task in tasks:
        if not isinstance(task, dict) or set(task) != {"id", "report", "revision"}:
            raise ValueError("invalid task reference fields")
        if task["id"] == "archive" or not matches(TASK, task["id"]) or not isinstance(task["report"], str) or not re.fullmatch(r"[a-z][a-z0-9-]*\.md", task["report"]):
            raise ValueError("invalid task ID or report path")
        if task["revision"] is not None and not matches(REVISION, task["revision"]):
            raise ValueError("task revision must be a full commit SHA or null")
        key = (task["id"], task["report"])
        if key in seen:
            raise ValueError("duplicate task/report reference")
        seen.add(key)
    if not matches(LEGACY_ID, record['id']):
        origin, number = record['id'][4:].rsplit('-', 1)
        if int(number) < 1 or number != f'{int(number):03d}' or origin not in {t['id'] for t in tasks}:
            raise ValueError('task-qualified finding ID needs its origin reference and a canonical positive sequence')
    resolution = record["resolution"]
    if not archived:
        if resolution is not None:
            raise ValueError("active finding cannot have a terminal resolution")
        return
    if not isinstance(resolution, dict) or set(resolution) != {"kind", "evidence", "approved_by", "duplicate_of"}:
        raise ValueError("closed finding needs structured resolution")
    if resolution["kind"] not in ("fixed", "duplicate", "not_a_bug", "accepted_risk") or not nonempty(resolution["evidence"]):
        raise ValueError("closed finding needs a valid resolution and evidence")
    approval = resolution["approved_by"]
    if approval is not None and not nonempty(approval):
        raise ValueError("invalid owner approval reference")
    if resolution["kind"] == "accepted_risk" and approval is None:
        raise ValueError("risk acceptance requires owner attribution and authorization reference")
    duplicate = resolution["duplicate_of"]
    if resolution["kind"] == "duplicate":
        if not matches(ID, duplicate) or duplicate == record["id"]:
            raise ValueError("duplicate resolution needs a different canonical finding ID")
    elif duplicate is not None:
        raise ValueError("only duplicate resolution may name duplicate_of")


def active_register(snapshot=None):
    modern = snapshot.exists(REGISTRY) if snapshot else local(REGISTRY).exists()
    if modern:
        marker = snapshot.json(REGISTRY) if snapshot else read_json(REGISTRY)
        if not isinstance(marker, dict) or type(marker.get('version')) is not int or marker != {'version': 2, 'storage': 'per-record'}:
            raise ValueError('invalid per-record findings marker')
        if snapshot.exists(ACTIVE) if snapshot else local(ACTIVE).exists():
            raise ValueError('legacy active.json and per-record registry coexist; finish migration')
        records = directory_records(ACTIVE_DIR, False, snapshot)
        return {'version': 2, 'findings': records}
    data = snapshot.json(ACTIVE) if snapshot else read_json(ACTIVE)
    if not isinstance(data, dict) or set(data) != {"version", "next_id", "findings"}:
        raise ValueError("invalid active register fields")
    if type(data["version"]) is not int or data["version"] != 1:
        raise ValueError("unsupported register version")
    if type(data["next_id"]) is not int or not 1 <= data["next_id"] <= 1000000:
        raise ValueError("invalid next_id")
    if not isinstance(data["findings"], list):
        raise ValueError("findings must be a list")
    for record in data["findings"]:
        validate_record(record)
    ids = [record["id"] for record in data["findings"]]
    if len(ids) != len(set(ids)) or any(int(value[4:]) >= data["next_id"] for value in ids):
        raise ValueError("active IDs must be unique and below next_id")
    return data



def directory_records(directory, archived, snapshot=None):
    if not (snapshot.exists(directory) if snapshot else local(directory).is_dir()):
        raise ValueError('finding record directory is missing')
    paths = [Path(name) for name in snapshot.entries if name.startswith(directory + '/')] if snapshot else local(directory).iterdir()
    records = []
    for path in sorted(paths):
        if snapshot and path.parent.as_posix() != directory:
            raise ValueError('unexpected nested finding entry')
        if snapshot:
            if snapshot.entries[path.as_posix()][0] not in ('100644', '100755'):
                raise ValueError('finding entries must be regular files')
        else:
            local(path.relative_to(ROOT))
        if path.name == '.gitkeep':
            continue
        if path.suffix != '.json' or not matches(ID, path.stem):
            raise ValueError('unexpected finding record entry')
        record = snapshot.json(path.as_posix()) if snapshot else read_json(path.relative_to(ROOT))
        validate_record(record, archived=archived)
        if path.stem != record['id']:
            raise ValueError('record filename does not match finding ID')
        records.append(record)
    return records

def all_records(data, snapshot=None):
    records = list(data['findings']) + directory_records(ARCHIVE, True, snapshot)
    ids = [record["id"] for record in records]
    if len(ids) != len(set(ids)):
        raise ValueError("duplicate finding ID across active register/archive")
    if data["version"] == 1 and any(not matches(LEGACY_ID, value) or int(value[4:]) >= data["next_id"] for value in ids):
        raise ValueError("next_id must exceed every allocated finding ID")
    for record in records:
        resolution = record["resolution"]
        if resolution and resolution["duplicate_of"] and resolution["duplicate_of"] not in ids:
            raise ValueError("duplicate resolution refers to an unknown finding")
    check_duplicate_chains(records)
    return records


def check_duplicate_chains(records):
    by_id = {record["id"]: record for record in records}
    for record in records:
        visited = set()
        current = record
        while current["resolution"] and current["resolution"]["kind"] == "duplicate":
            if current["id"] in visited:
                raise ValueError("duplicate resolution cycle")
            visited.add(current["id"])
            target = current["resolution"]["duplicate_of"]
            if target not in by_id:
                raise ValueError("duplicate resolution refers to an unknown finding")
            current = by_id[target]


def git_ok(*args):
    result = subprocess.run(["git", *GIT_FLAGS, *args], cwd=ROOT, capture_output=True, text=True)
    return result.returncode == 0


def git_blob(revision, path):
    result = subprocess.run(["git", *GIT_FLAGS, "ls-tree", revision, "--", path], cwd=ROOT, capture_output=True, text=True)
    return result.returncode == 0 and result.stdout.startswith(("100644 blob ", "100755 blob "))


def check_tasks(records, pruning, snapshot=None):
    for record in records:
        for task in record["tasks"]:
            roots = [f".factory/tasks/{prefix}{task['id']}" for prefix in ("", "archive/")]
            paths = [f"{root}/{task['report']}" for root in roots]
            exists = snapshot.exists if snapshot else lambda path: local(path).exists()
            if sum(exists(root) for root in roots) > 1:
                raise ValueError(f"{record['id']}: ambiguous task locations")
            present = any(path in snapshot.entries and snapshot.entries[path][0] in ('100644', '100755') for path in paths) if snapshot else any(local(path).is_file() for path in paths)
            revision = task["revision"]
            if task["id"] in pruning and revision is None:
                raise ValueError(f"{record['id']}: pin the task report before pruning")
            if revision is None:
                if not present:
                    raise ValueError(f"{record['id']}: current task report is missing")
            else:
                if snapshot:
                    pinned = snapshot.at(revision)
                    if sum(path in pinned.entries and pinned.entries[path][0] in ('100644', '100755') for path in paths) != 1:
                        raise ValueError(f"{record['id']}: pinned task report is unavailable")
                    if (task['id'] in pruning or not present) and not snapshot.ancestor(revision):
                        raise ValueError(f"{record['id']}: pruned report commit must be retained in candidate history")
                    continue
                if not git_ok("cat-file", "-e", f"{revision}^{{commit}}") or sum(git_blob(revision, path) for path in paths) != 1:
                    raise ValueError(f"{record['id']}: pinned task commit/report is unavailable")
                if (task["id"] in pruning or not present) and not git_ok("merge-base", "--is-ancestor", revision, "HEAD"):
                    raise ValueError(f"{record['id']}: pruned report commit must be retained in candidate history")



def next_task_id(data, task):
    import task_store
    if not matches(TASK, task) or task == 'archive' or not task_store.ORDERED.fullmatch(task):
        raise ValueError('valid --task required for per-task allocation')
    prefix = 'FND-' + task + '-'
    numbers = [int(record['id'][len(prefix):]) for record in all_records(data)
               if record['id'].startswith(prefix) and record['id'][len(prefix):].isdigit()]
    return prefix + f'{max(numbers, default=0) + 1:03d}'


def create(task, filename):
    import task_store
    with task_store.lifecycle_lock(ROOT):
        _, state = task_store.selected_state(ROOT, task)
        if state.get('status') not in ('active', 'blocked'):
            raise ValueError('findings creation requires an unfinished origin task')
        branch = subprocess.run(['git', 'branch', '--show-current'], cwd=ROOT, capture_output=True, text=True)
        if branch.returncode or branch.stdout.strip() != 'task/' + task:
            raise ValueError('findings creation requires the matching origin task branch')
        if not task_store.ORDERED.fullmatch(task):
            raise ValueError('new finding origin requires a canonical ordered task ID; migrate task IDs first')
        data = active_register()
        if data['version'] != 2:
            raise ValueError('migrate findings before using per-task creation')
        with filename.open('rb') as stream:
            raw = stream.read(MAX_BYTES + 1)
        if len(raw) > MAX_BYTES:
            raise ValueError('finding input exceeds 1 MiB')
        record = task_store.loads(raw)
        if not isinstance(record, dict) or 'id' in record:
            raise ValueError('create input must be a finding object without its generated id')
        record['id'] = next_task_id(data, task)
        validate_record(record)
        check_tasks([record], set())
        path = local(ACTIVE_DIR + '/' + record['id'] + '.json')
        if path.exists():
            raise ValueError('finding destination exists')
        write_record(path, record)
        return record['id']


def write_record(destination, record, source=None):
    """Commit a finding record edit, restoring its original location on error."""
    import task_store
    original = source.read_bytes() if source else None
    if destination.exists():
        raise ValueError('finding destination exists')
    try:
        task_store.atomic(destination, (json.dumps(record, indent=2, allow_nan=False) + '\n').encode())
        if source:
            source.unlink()
    except (OSError, ValueError):
        if source and not source.exists():
            task_store.atomic(source, original)
        destination.unlink(missing_ok=True)
        raise


def archive(identifier, filename):
    import task_store
    with task_store.lifecycle_lock(ROOT):
        if not matches(ID, identifier):
            raise ValueError('invalid finding ID')
        data = active_register()
        if data['version'] != 2:
            raise ValueError('migrate findings before using archive')
        records = all_records(data)
        if not any(record['id'] == identifier for record in data['findings']):
            raise ValueError('finding is not active')
        with filename.open('rb') as stream:
            raw = stream.read(MAX_BYTES + 1)
        if len(raw) > MAX_BYTES:
            raise ValueError('finding input exceeds 1 MiB')
        closed = task_store.loads(raw)
        validate_record(closed, archived=True)
        if closed['id'] != identifier:
            raise ValueError('archive cannot change finding identity')
        # Validate report pins and duplicate chains against the proposed register.
        records = [closed if record['id'] == identifier else record for record in records]
        check_tasks(records, set())
        check_duplicate_chains(records)
        write_record(local(ARCHIVE + '/' + identifier + '.json'), closed, local(ACTIVE_DIR + '/' + identifier + '.json'))
        return identifier

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    check = commands.add_parser("check", help="validate active/archive and task evidence")
    check.add_argument("--prune-task", action="append", default=[], help="task whose reports will be deleted")
    commands.add_parser("list", help="show active finding summaries only")
    next_id = commands.add_parser("next-id", help="suggest a task-scoped ID without reserving it (legacy counter in v1)")
    next_id.add_argument('--task')
    new = commands.add_parser('create', help='allocate and atomically create a per-task active finding')
    new.add_argument('--task', required=True)
    new.add_argument('--record', required=True, type=Path, help='complete finding JSON without id')
    close = commands.add_parser('archive', help='archive a verified closed record')
    close.add_argument('id')
    close.add_argument('--record', required=True, type=Path)
    show = commands.add_parser("show", help="explicitly read one active or archived finding")
    show.add_argument("id")
    args = parser.parse_args()
    try:
        if args.command == "create":
            print(create(args.task, args.record))
            return 0
        if args.command == 'archive':
            print(archive(args.id, args.record))
            return 0
        data = active_register()
        if args.command == "list":
            output = json.dumps([{key: record[key] for key in ("id", "title", "severity", "status", "owner", "blocking")} for record in data["findings"]], indent=2)
        elif args.command == "show":
            if not matches(ID, args.id):
                raise ValueError("invalid finding ID")
            selected = [record for record in data["findings"] if record["id"] == args.id]
            record = selected[0] if selected else read_json(f"{ARCHIVE}/{args.id}.json")
            validate_record(record, archived=not bool(selected))
            if record["id"] != args.id:
                raise ValueError("finding ID mismatch")
            output = json.dumps(record, indent=2)
        else:
            records = all_records(data)
            if args.command == "next-id":
                if data['version'] == 2:
                    if not args.task:
                        raise ValueError('--task is required for version 2 next-id')
                    output = next_task_id(data, args.task)
                elif data["next_id"] > 999999:
                    raise ValueError("finding ID space exhausted; schema migration required")
                else:
                    output = f"FND-{data['next_id']:06d}"
            else:
                if any(not matches(TASK, task) for task in args.prune_task):
                    raise ValueError("invalid prune task ID")
                check_tasks(records, set(args.prune_task))
                output = f"Findings valid: {len(data['findings'])} active, {len(records) - len(data['findings'])} archived; task evidence checks passed."
    except (OSError, ValueError, TypeError, RecursionError, RuntimeError) as exc:
        # Never include finding payloads or subprocess output in errors.
        print(f"Findings check failed: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 2
    print(output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
