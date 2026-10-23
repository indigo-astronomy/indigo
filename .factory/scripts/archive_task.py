#!/usr/bin/env python3
"""Archive the selected completed task and repair standard local Markdown links."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import task_store
from urllib.parse import quote, unquote, urlsplit, urlunsplit

ROOT = Path(__file__).resolve().parents[2]
LIMIT = 1024 * 1024
# INDIGO owner exclusions also apply to archive-time link maintenance.
EXCLUDED_PREFIXES = ("indigo_drivers/ccd_apogee/", "indigo_drivers/ccd_sbig/", "indigo_drivers/mount_asi/", "indigo_drivers/mount_mxhd/", "indigo_drivers/system_ascol/", "indigo_tests/", "obsoleted_drivers/")
# Standard inline links/images and reference definitions; leave URLs and prose alone.
LINK = re.compile(r'''(?P<prefix>!?\[[^\]\n]*\]\(|^ {0,3}\[[^\]\n]+\]:[ \t]*)(?:<(?P<angle>[^>\n]+)>|(?P<bare>[^\s()]+)(?=[ \t]*(?:\)|["']|$)))''', re.M)


def local(path):
    path.relative_to(ROOT)
    current = ROOT
    for part in path.relative_to(ROOT).parts:
        current /= part
        if current.is_symlink():
            raise ValueError('symbolic links are not supported for archive inputs')
    return path


def read(path):
    local(path)
    if not path.is_file():
        raise ValueError('archive input must be a regular file')
    with path.open('rb') as stream:
        data = stream.read(LIMIT + 1)
    if len(data) > LIMIT:
        raise ValueError('archive text input exceeds 1 MiB')
    return data.decode('utf-8')


def atomic(path, data):
    mode = path.stat().st_mode & 0o777
    descriptor, temporary = tempfile.mkstemp(prefix='.factory-archive-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
            os.fchmod(stream.fileno(), mode)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def relocate_links(text, document, source, destination):
    moved_document = destination / document.relative_to(source) if source in document.parents else document

    def replace(match):
        original = match.group('angle') or match.group('bare')
        url = urlsplit(original)
        if url.scheme or url.netloc or not url.path or url.path.startswith('/'):
            return match.group(0)
        target = Path(os.path.normpath(document.parent / unquote(url.path)))
        if target == source or source in target.parents:
            target = destination / target.relative_to(source)
        if moved_document == document and not (target == destination or destination in target.parents):
            return match.group(0)
        rewritten = os.path.relpath(target, moved_document.parent).replace(os.sep, '/')
        rewritten = quote(rewritten, safe='/._~-')
        value = urlunsplit(('', '', rewritten, url.query, url.fragment))
        return match.group('prefix') + ('<' + value + '>' if match.group('angle') else value)

    return LINK.sub(replace, text)


def prepare(task):
    if task == 'archive' or not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', task):
        raise ValueError('invalid or reserved task ID')
    state_path, data = task_store.selected_state(ROOT, task)
    state = read(state_path)
    modern = state_path != ROOT / '.factory/state.json'
    required = {'active_task', 'phase', 'status', 'assigned_role', 'next_action', 'blocker'}
    if not isinstance(data, dict) or not required <= data.keys():
        raise ValueError('state needs standard workflow fields')
    for key, expected in (('active_task', task), ('phase', 'complete'), ('status', 'complete'), ('blocker', None)):
        if data[key] != expected:
            raise ValueError('state must select this task with phase/status complete; finish its gates first')
    source = local(ROOT / '.factory/tasks' / task)
    destination = local(ROOT / '.factory/tasks/archive' / task)
    if not source.is_dir() or destination.exists():
        raise ValueError('task must exist and archive destination must be unused')
    # Preserve custom/binary reports too, but never move a linked directory or file.
    for path in source.rglob('*'):
        local(path)
        if not (path.is_dir() or path.is_file()):
            raise ValueError('task contains a nonregular entry')
    for name in ('brief.md', 'handoff.md'):
        read(source / name)
    result = subprocess.run(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'],
        cwd=ROOT, capture_output=True,
    )
    if result.returncode:
        raise ValueError('Git file inventory unavailable; no files changed')
    changes = {}
    names = set(result.stdout.decode('utf-8').split('\0')) - {''}
    names.update(path.relative_to(ROOT).as_posix() for path in source.rglob('*.md'))
    for name in sorted(names):
        if not name.endswith('.md') or name.startswith(EXCLUDED_PREFIXES) or any(part in ('externals', 'bin_externals') for part in Path(name).parts):
            continue
        document = ROOT / name
        if not document.exists():  # A tracked deletion is not a link-edit input.
            continue
        text = read(document)
        updated = relocate_links(text, document, source, destination)
        if updated != text:
            changes[document] = (text.encode('utf-8'), updated.encode('utf-8'))
    if not modern:
        data.update(active_task=None, phase='idle', status='idle', assigned_role='coordinator',
                    next_action='Select or create the next task.', blocker=None)
    idle = json.dumps(data, indent=2, allow_nan=False) + '\n'
    return source, destination, changes, state_path, state.encode('utf-8'), idle.encode('utf-8')


def archive(task, dry_run=False):
    source, destination, changes, state_path, state, idle = prepare(task)
    completion = None
    config_path = ROOT / '.factory/maintenance.json'
    if config_path.exists():
        import maintenance
        completion = maintenance.completion_for_archive(ROOT, task, task_store.loads(state))
    if dry_run:
        return len(changes)
    moved = False
    written = []
    completion_written = False
    created_archive = not destination.parent.exists()
    try:
        destination.parent.mkdir(exist_ok=True)
        for path, (before, after) in changes.items():
            if path.read_bytes() != before:
                raise ValueError('Markdown file changed during planning; retry with one writer')
            written.append(path)
            atomic(path, after)
        if state_path.read_bytes() != state:
            raise ValueError('state changed during planning; retry with one writer')
        source.rename(destination)
        moved = True
        if state_path == ROOT / '.factory/state.json':
            atomic(state_path, idle)
        if completion is not None:
            completion_written = True
            task_store.atomic(*completion)
    except (OSError, ValueError):
        try:
            if completion_written:
                completion[0].unlink(missing_ok=True)
            if moved:
                destination.rename(source)
            for path in reversed(written):
                atomic(path, changes[path][0])
            if moved and state_path == ROOT / '.factory/state.json':
                atomic(state_path, state)
            if created_archive:
                destination.parent.rmdir()
        except OSError:
            raise ValueError('archive rollback failed; preserve both locations and recover from Git before retrying') from None
        raise ValueError('archive failed; original task and edited files restored') from None
    return len(changes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task')
    parser.add_argument('--dry-run', action='store_true', help='validate and count link updates without writing')
    args = parser.parse_args()
    try:
        with task_store.lifecycle_lock(ROOT):
            count = archive(args.task, args.dry_run)
    except (OSError, ValueError, RuntimeError) as exc:
        print('Task archive failed: ' + str(exc), file=sys.stderr)
        return 2
    print(f"{'Would archive' if args.dry_run else 'Archived'} {args.task}; Markdown files requiring link updates: {count}.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
