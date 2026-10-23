#!/usr/bin/env python3
"""Create a task from the local templates without overwriting existing work."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import argparse
import tempfile
import task_store

ROOT = Path(__file__).resolve().parents[2]
LIMIT = 1024 * 1024


def local(path):
    current = ROOT
    for part in path.relative_to(ROOT).parts:
        current /= part
        if current.is_symlink():
            raise ValueError('symbolic links are not supported for task inputs or output paths')
    return path


def create(task_id):
    try:
        target = local(ROOT / ".factory/tasks" / task_id)
        state_path = local(ROOT / ".factory/state.json")
        with state_path.open('rb') as stream:
            raw = stream.read(LIMIT + 1)
        if len(raw) > LIMIT:
            raise ValueError('workflow state exceeds 1 MiB')
        state = task_store.loads(raw)
        if isinstance(state, dict) and state.get("version", 1) not in (1, 2):
            raise ValueError("unsupported repository state version")
        modern = isinstance(state, dict) and state.get("version") == 2
        if modern and not task_store.ORDERED.fullmatch(task_id):
            raise ValueError("version 2 requires a generated time-prefixed task ID")
        if not isinstance(state, dict) or (not modern and state.get("status") not in ("idle", "complete")):
            print("Refusing to replace an active or unrecognized workflow state. Resolve the current handoff first.", file=sys.stderr)
            return 2
        archived = local(ROOT / ".factory/tasks/archive" / task_id)
        if archived.parent.is_symlink() or (archived.parent.exists() and not archived.parent.is_dir()):
            print("Task archive must be a real directory when present.", file=sys.stderr)
            return 2
        if any(path.exists() or path.is_symlink() for path in (target, archived)):
            print(f"Task already exists: {target}", file=sys.stderr)
            return 2
        if shutil.which("git"):
            repository = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=ROOT, capture_output=True, text=True)
            if repository.returncode == 0:
                history = subprocess.run(
                    ["git", "--no-replace-objects", "--no-lazy-fetch", "log", "--all", "-1", "--format=%H", "--",
                     f".factory/tasks/{task_id}", f".factory/tasks/archive/{task_id}"],
                    cwd=ROOT, capture_output=True, text=True,
                )
                if history.returncode != 0 or history.stdout.strip():
                    print("Task ID is already used in Git history or history could not be checked.", file=sys.stderr)
                    return 2
        source = local(ROOT / ".factory/templates/task")
        for path in source.rglob('*'):
            local(path)
            if not (path.is_dir() or path.is_file()):
                raise ValueError('task templates must contain regular files and directories')
        required = {"brief.md", "specification.md", "plan.md", "test-plan.md", "verification.md", "security.md", "handoff.md"}
        if not all((source / name).is_file() for name in required):
            print("Task templates are missing required files.", file=sys.stderr)
            return 2
        if modern:
            state = {}
        state.update(version=2 if modern else 1, active_task=task_id, phase="specification", status="active",
                     assigned_role="analyst", next_action="Complete brief.md and produce a testable specification.",
                     blocker=None)
        updated_state = json.dumps(state, indent=2, allow_nan=False) + "\n"
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = Path(tempfile.mkdtemp(prefix=".new-task-", dir=target.parent))
        published = False
        try:
            shutil.copytree(source, temporary, dirs_exist_ok=True)
            brief = temporary / "brief.md"
            brief.write_text(brief.read_text().replace("Task ID: TODO", f"Task ID: {task_id}"))
            handoff = temporary / "handoff.md"
            handoff.write_text(handoff.read_text().replace("Task: TODO", f"Task: {task_id}"))
            evidence = temporary / "process.json"
            if evidence.is_file():
                evidence.write_text(evidence.read_text().replace('"task": "TODO"', f'"task": "{task_id}"'))
            if modern:
                task_store.atomic(temporary / "state.json", updated_state.encode("utf-8"))
            temporary.rename(target)
            published = True
            if not modern:
                task_store.atomic(state_path, updated_state.encode("utf-8"))
        except (OSError, ValueError):
            if published:
                shutil.rmtree(target)
            raise
        finally:
            if temporary.exists():
                shutil.rmtree(temporary)
    except (OSError, ValueError, RecursionError) as exc:
        print(f"Could not finish task creation: {exc}. Inspect the task directory and state before retrying.", file=sys.stderr)
        return 2
    print(f"Created {target}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name', help='readable slug, or previously allocated time-prefixed ID')
    parser.add_argument('--print-id', action='store_true', help='allocate an ID without writing; use it for branch/worktree setup')
    args = parser.parse_args()
    try:
        task_store.valid_task(args.name)
        if args.print_id:
            print(task_store.unique_id(ROOT, args.name))
            return 0
        with task_store.lifecycle_lock(ROOT):
            repository = task_store.loads(task_store.read(ROOT, ROOT / '.factory/state.json'))
            modern = isinstance(repository, dict) and repository.get('version') == 2
            task = task_store.unique_id(ROOT, args.name) if modern and not task_store.ORDERED.fullmatch(args.name) else args.name
            return create(task)
    except (OSError, ValueError, RecursionError) as exc:
        print('Task creation failed: ' + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
