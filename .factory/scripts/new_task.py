#!/usr/bin/env python3
"""Create a task from the local templates without overwriting existing work."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LIMIT = 1024 * 1024


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def local(path):
    current = ROOT
    for part in path.relative_to(ROOT).parts:
        current /= part
        if current.is_symlink():
            raise ValueError('symbolic links are not supported for task inputs or output paths')
    return path


def invalid_constant(value):
    raise ValueError('nonfinite JSON constants are not supported')


def main():
    if len(sys.argv) != 2 or sys.argv[1] == "archive" or not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", sys.argv[1]):
        print("Usage: new_task.py TASK-ID (lowercase letters, digits, hyphens)", file=sys.stderr)
        return 2
    task_id = sys.argv[1]
    try:
        target = local(ROOT / ".factory/tasks" / task_id)
        state_path = local(ROOT / ".factory/state.json")
        with state_path.open('rb') as stream:
            raw = stream.read(LIMIT + 1)
        if len(raw) > LIMIT:
            raise ValueError('workflow state exceeds 1 MiB')
        state = json.loads(raw, object_pairs_hook=unique_pairs, parse_constant=invalid_constant)
        if not isinstance(state, dict) or state.get("status") not in ("idle", "complete"):
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
        state.update(version=1, active_task=task_id, phase="specification", status="active",
                     assigned_role="analyst", next_action="Complete brief.md and produce a testable specification.",
                     blocker=None)
        updated_state = json.dumps(state, indent=2, allow_nan=False) + "\n"
        shutil.copytree(source, target)
        brief = target / "brief.md"
        brief.write_text(brief.read_text().replace("Task ID: TODO", f"Task ID: {task_id}"))
        handoff = target / "handoff.md"
        handoff.write_text(handoff.read_text().replace("Task: TODO", f"Task: {task_id}"))
        evidence = target / "process.json"
        if evidence.is_file():
            evidence.write_text(evidence.read_text().replace('"task": "TODO"', f'"task": "{task_id}"'))
        state_path.write_text(updated_state, encoding="utf-8")
    except (OSError, ValueError, RecursionError) as exc:
        print(f"Could not finish task creation: {exc}. Inspect the task directory and state before retrying.", file=sys.stderr)
        return 2
    print(f"Created {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
