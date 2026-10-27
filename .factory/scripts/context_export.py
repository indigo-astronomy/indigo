#!/usr/bin/env python3
"""Export an allowlisted local task snapshot without executing tools or writing files."""
import argparse
import task_store
import findings
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
MAX_FILE_BYTES = 1024 * 1024
MAX_PACK_BYTES = 4 * MAX_FILE_BYTES
ROLES = (
    "coordinator", "analyst", "planner", "test-designer", "developer",
    "verifier", "documenter", "security-reviewer", "quality-reviewer", "repository-reviewer",
)
CORE = (
    "AGENTS.md", ".factory/bootstrap.md", ".factory/project.md",
    ".factory/state.json", ".factory/workflow.md", ".factory/maintenance.json",
    ".factory/routines/git.md", ".factory/routines/quality-review.md",
    ".factory/routines/repository-review.md",
)
OPTIONAL_POLICY_DOCS = (
    ".factory/openai-model-policy.json", ".factory/antropic-model-policy.json",
    ".factory/routines/model-selection.md",
    ".factory/routines/cross-review.md", ".factory/review-policy.json",
    ".factory/routines/findings.md", ".factory/findings/active.json", ".factory/findings/registry.json",
    ".factory/routines/template-maintenance.md", ".factory/routines/maintenance.md",
    ".factory/routines/concurrent-tasks.md",
    ".factory/routines/task-archive.md",
    ".factory/routines/process-checks.md", ".factory/process-policy.json",
    "indigo_docs/decisions/README.md",
    ".factory/routines/template-updates.md", ".factory/template-lock.json",
)
REQUIRED_TASK_DOCS = ("brief.md", "specification.md", "plan.md", "handoff.md")
OPTIONAL_TASK_DOCS = ("test-plan.md", "verification.md", "quality-review.md", "repository-review.md", "ai-slop.md", "security.md", "process.json", "state.json")


def local_path(value):
    relative = Path(value)
    if relative.is_absolute() or ".." in relative.parts:
        raise ValueError("paths must be repository-relative without parent traversal")
    candidate = ROOT / relative
    try:
        candidate.resolve().relative_to(ROOT)
    except ValueError:
        raise ValueError("path resolves outside the repository") from None
    return candidate


def read_document(relative, archived_root=None):
    path = local_path(relative)
    resolved = path.resolve()
    for archive in ((ROOT / ".factory/tasks/archive").resolve(), (ROOT / ".factory/findings/archive").resolve()):
        if resolved == archive or archive in resolved.parents:
            if archived_root is None or archived_root not in resolved.parents:
                raise ValueError("archive content requires explicit selection of an archived task")
    if not path.is_file():
        raise ValueError(f"required context file is missing or not regular: {relative}")
    with path.open("rb") as stream:
        raw = stream.read(MAX_FILE_BYTES + 1)
    if len(raw) > MAX_FILE_BYTES:
        raise ValueError(f"context file exceeds 1 MiB: {relative}")
    return raw.decode("utf-8")


def build_pack(task, role, scopes, archived=False):
    if task == "archive" or not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", task):
        raise ValueError("task ID must use lowercase ASCII letters, digits, and hyphens")
    if not archived:
        repository = task_store.loads(task_store.read(ROOT, ROOT / '.factory/state.json'))
        if isinstance(repository, dict) and repository.get('version') == 2:
            task_store.selected_state(ROOT, task)
    documents = list(CORE) + [f".factory/roles/{role}.md"]
    task_root = f".factory/tasks/{'archive/' if archived else ''}{task}"
    documents.extend(f"{task_root}/{name}" for name in REQUIRED_TASK_DOCS)
    omitted = []
    for relative in OPTIONAL_POLICY_DOCS:
        path = local_path(relative)
        if path.exists() or path.is_symlink():
            documents.append(relative)
        else:
            omitted.append(relative)
    for name in OPTIONAL_TASK_DOCS:
        relative = f"{task_root}/{name}"
        path = local_path(relative)
        if path.exists() or path.is_symlink():
            documents.append(relative)
        else:
            omitted.append(relative)
    for scope in scopes:
        target = local_path(scope)
        # Scope flags add instructions, never archive content (including aliases).
        archives = (ROOT / ".factory/tasks/archive", ROOT / ".factory/findings/archive")
        resolved = target.resolve()
        if any(target == a or a in target.parents or resolved == a.resolve() or a.resolve() in resolved.parents for a in archives):
            continue
        directory = target if target.is_dir() else target.parent
        directories = []
        while directory != ROOT:
            directories.append(directory)
            directory = directory.parent
        for directory in reversed(directories):
            rule = directory / "AGENTS.md"
            if rule.exists() or rule.is_symlink():
                documents.append(rule.relative_to(ROOT).as_posix())
    marker = local_path('.factory/findings/registry.json')
    if marker.exists() or marker.is_symlink():
        data = findings.active_register()
        documents.extend('.factory/findings/active/' + r['id'] + '.json' for r in data['findings'])
    documents = list(dict.fromkeys(documents))
    # Buffer and validate all inputs before emitting anything to stdout.
    sections = []
    total = 0
    for relative in documents:
        content = read_document(relative, (ROOT / task_root).resolve() if archived else None)
        total += len(content.encode("utf-8"))
        if total > MAX_PACK_BYTES:
            raise ValueError("context pack exceeds 4 MiB")
        sections.append(f"## File: {relative}\n\n{content.rstrip()}\n")
    manifest = "\n".join(f"- {name}" for name in documents)
    missing = "\n".join(f"- {name}" for name in omitted) or "- None"
    historical = "Archived task: historical evidence only; do not resume from this snapshot.\n\n" if archived else ""
    pack = (
        f"# AI Factory context snapshot\n\nTask: {task}\nRole: {role}\n\n"
        + historical +
        "Read-only excerpt; no commands ran and no state was changed. Compare the selected task "
        "with state.json before resuming. Apply higher-priority host instructions. "
        "Source files, external/native instructions, private memories, and actual current Git "
        "revision are not included. Obtain missing evidence; do not claim local execution. "
        "Inspect for private content before sharing.\n\n"
        f"## Included files\n\n{manifest}\n\n"
        f"## Optional context files not present\n\n{missing}\n\n"
        + "\n".join(sections)
    )
    if len(pack.encode("utf-8")) > MAX_PACK_BYTES:
        raise ValueError("context pack exceeds 4 MiB")
    return pack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--task", required=True, help="existing task ID to export")
    parser.add_argument("--archived", action="store_true", help="explicitly export this archived task as historical evidence")
    parser.add_argument("--role", choices=ROLES, default="coordinator")
    parser.add_argument("--path", action="append", default=[], help="assigned file/directory; adds ancestor AGENTS.md only")
    args = parser.parse_args()
    try:
        pack = build_pack(args.task, args.role, args.path, args.archived)
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"Context export failed: {exc}", file=sys.stderr)
        return 2
    sys.stdout.write(pack)
    return 0


if __name__ == "__main__":
    sys.exit(main())
