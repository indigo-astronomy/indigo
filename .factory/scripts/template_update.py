#!/usr/bin/env python3
"""Plan and apply pinned, project-local AI Factory template updates."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

LOCK = '.factory/template-lock.json'
LIMIT = 1024 * 1024
MAX_FILES = 256
REPOSITORY = 'https://github.com/polakovic/ai_factory'
# These are instance data, not distributable policy. Protect both mapping sides.
PROTECTED = (
    '.factory/project.md', '.factory/state.json', '.factory/maintenance.json',
    '.factory/state.yaml', '.factory/maintenance.yaml',  # Legacy instance data remains protected.
    '.factory/checks.json', '.factory/process-policy.json', LOCK, '.factory/tasks', '.factory/findings',
    '.factory/telemetry',  # Retired local data must never be overwritten by an upgrade.
    '.factory/adopters.local.json', '.factory/review-policy.json', '.factory/review.local.json',
)


def git(root, *args, ok=(0,)):
    result = subprocess.run(
        ['git', '--no-replace-objects', '--no-lazy-fetch', '--no-optional-locks', '-c', 'core.fsmonitor=false', '-C', str(root), *args],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    if result.returncode not in ok:
        # Do not echo private remote addresses, Git configuration or blob data.
        raise ValueError('Git operation failed: ' + args[0])
    return result


def relative(value):
    if not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*', value):
        raise ValueError('mapping paths must be plain relative paths')
    if any(part.casefold() in ('.', '..', '.git') for part in value.split('/')):
        raise ValueError('unsafe mapping path')
    return value


def protected(value):
    value = value.casefold()
    return any(value == p or value.startswith(p + '/') or p.startswith(value + '/') for p in PROTECTED)


def local(root, name):
    path = root
    parts = relative(name).split('/')
    for index, part in enumerate(parts):
        path = path / part
        if path.is_symlink():
            raise ValueError('symbolic links are not supported: ' + name)
        if index < len(parts) - 1 and path.exists() and not path.is_dir():
            raise ValueError('parent path must be a directory: ' + name)
    return path


def read(path):
    if not path.exists():
        return None
    if not path.is_file():
        raise ValueError('expected a regular file')
    with path.open('rb') as stream:
        data = stream.read(LIMIT + 1)
    text_file(data)
    return data, '100755' if path.stat().st_mode & 0o111 else '100644'


def text_file(data):
    if len(data) > LIMIT or b'\0' in data:
        raise ValueError('only text files up to 1 MiB are supported')
    data.decode('utf-8')


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def load(path):
    item = read(path)
    if item is None:
        raise ValueError('required JSON file is missing')
    return json.loads(item[0], object_pairs_hook=unique_pairs)


def mapping(files):
    if not isinstance(files, dict) or not 1 <= len(files) <= MAX_FILES:
        raise ValueError('map must contain 1 to 256 source-to-project paths')
    for src, dst in files.items():
        relative(src)
        relative(dst)
        if protected(src) or protected(dst):
            raise ValueError('project-owned data cannot be mapped: ' + src)
    for original in (list(files), list(files.values())):
        paths = [p.casefold() for p in original]
        if len(set(paths)) != len(paths):
            raise ValueError('mapping paths must be unique')
        for first in paths:
            if any(other.startswith(first + '/') for other in paths if other != first):
                raise ValueError('mapping paths cannot overlap')
    return files


def lock_data(value):
    if not isinstance(value, dict) or set(value) != {'version', 'repository', 'revision', 'files'}:
        raise ValueError('invalid template lock fields')
    if type(value['version']) is not int or value['version'] != 1:
        raise ValueError('unsupported template lock version')
    repository(value['repository'])
    revision(value['revision'])
    mapping(value['files'])
    return value


def repository(url):
    if not isinstance(url, str):
        raise ValueError('repository URL must be a string')
    # Support the canonical public URL and equivalent SSH clones, including approved forks.
    url = re.sub(r'^git@([^:]+):', r'https://\1/', url).removesuffix('.git')
    if not re.fullmatch(r'https://[A-Za-z0-9.-]+/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', url):
        raise ValueError('repository must be a credential-free HTTPS or SSH Git URL')
    return url


def revision(value):
    if not isinstance(value, str) or not re.fullmatch(r'[0-9a-f]{40}|[0-9a-f]{64}', value):
        raise ValueError('revision must be a full lowercase commit SHA')
    return value


def source_check(source, url, sha):
    root = Path(git(source, 'rev-parse', '--show-toplevel').stdout.decode().strip()).resolve()
    if root != source:
        raise ValueError('--source must be the upstream Git repository root')
    actual = git(source, 'remote', 'get-url', 'origin').stdout.decode().strip()
    if repository(actual) != repository(url):
        raise ValueError('source origin does not match the approved lock repository')
    commit = git(source, 'rev-parse', '--verify', revision(sha) + '^{commit}').stdout.decode().strip()
    if commit != sha:
        raise ValueError('source revision must identify the exact commit')


def blob(source, sha, name):
    entry = git(source, 'ls-tree', '-z', sha, '--', name).stdout
    if not entry:
        return None
    entries = entry.rstrip(b'\0').split(b'\0')
    if len(entries) != 1:
        raise ValueError('source mapping must name one regular blob')
    metadata, actual = entries[0].split(b'\t', 1)
    mode, kind, oid = metadata.decode('ascii').split()
    if actual.decode('utf-8') != name or kind != 'blob' or mode not in ('100644', '100755'):
        raise ValueError('source mapping must name a regular non-symlink blob: ' + name)
    size = int(git(source, 'cat-file', '-s', oid).stdout)
    if size > LIMIT:
        raise ValueError('source file exceeds 1 MiB')
    data = git(source, 'cat-file', 'blob', oid).stdout
    text_file(data)
    return data, mode


def merge(base, ours, theirs):
    if ours == theirs or theirs == base:
        return ours, False
    if ours == base:
        return theirs, False
    if base is None or ours is None or theirs is None:
        return ours, True
    # Merge data and execute permission independently; no repository merge drivers/hooks.
    b, o, t = base[0], ours[0], theirs[0]
    if o == t or t == b:
        data, conflict = o, False
    elif o == b:
        data, conflict = t, False
    else:
        with tempfile.TemporaryDirectory(prefix='factory-merge-') as directory:
            paths = [Path(directory) / name for name in ('local', 'base', 'upstream')]
            for path, data in zip(paths, (o, b, t)):
                path.write_bytes(data)
            result = subprocess.run(
                ['git', 'merge-file', '--stdout', '--diff3', *(str(p) for p in paths)],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            )
            if result.returncode > 127 or result.returncode < 0:
                raise ValueError('text merge failed')
            data, conflict = result.stdout, result.returncode != 0
    mode = ours[1] if theirs[1] == base[1] else theirs[1]
    text_file(data)
    return (data, mode), conflict


def task_guard(project, task):
    if not task or not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', task):
        raise ValueError('writes require --task with an existing factory task ID')
    root = Path(git(project, 'rev-parse', '--show-toplevel').stdout.decode().strip()).resolve()
    if root != project:
        raise ValueError('--project must be the Git repository root')
    branch = git(project, 'symbolic-ref', '--quiet', '--short', 'HEAD').stdout.decode().strip()
    if branch != 'task/' + task:
        raise ValueError('writes require the matching task/<task-id> branch')
    for name in ('brief.md', 'handoff.md'):
        if read(local(project, '.factory/tasks/' + task + '/' + name)) is None:
            raise ValueError('task brief and handoff must exist')
    state = load(local(project, '.factory/state.json'))
    if not isinstance(state, dict) or state.get('active_task') != task:
        raise ValueError('state.json must select the requested task')


def clean_paths(project, paths):
    # A committed baseline makes interrupted-write recovery possible. Task records may be dirty.
    status = git(project, 'status', '--porcelain', '--untracked-files=all', '--', *paths).stdout
    if status:
        raise ValueError('commit mapped files and template lock before applying')


def atomic(path, item):
    if item is None:
        path.unlink(missing_ok=True)
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix='.factory-update-', dir=str(path.parent))
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(item[0])
            stream.flush()
            os.fsync(stream.fileno())
            os.fchmod(stream.fileno(), 0o755 if item[1] == '100755' else 0o644)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def serialize(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode('utf-8'), '100644'


def apply_files(project, before, after):
    # Preflight everything before the first write. Caller owns exclusive worktree access.
    for name, expected in before.items():
        if read(local(project, name)) != expected:
            raise ValueError('project changed during planning; rerun the update')
    changed = []
    try:
        for name, item in after.items():
            changed.append(name)
            atomic(local(project, name), item)
    except OSError:
        failures = []
        for name in reversed(changed):
            try:
                atomic(local(project, name), before[name])
            except OSError:
                failures.append(name)
        if failures:
            raise ValueError('write and rollback failed; recover committed files before retrying') from None
        raise ValueError('write failed; original file contents restored') from None


def plan(project, source, lock, sha, accepted):
    source_check(source, lock['repository'], lock['revision'])
    source_check(source, lock['repository'], sha)
    git(source, 'merge-base', '--is-ancestor', lock['revision'], sha)
    before, after, actions = {}, {}, []
    renamed = {}
    files = lock['files']
    if set(accepted) - set(files.values()):
        raise ValueError('--accept-local must name a mapped project path')
    for src, dst in sorted(files.items()):
        base, theirs = blob(source, lock['revision'], src), blob(source, sha, src)
        # Preserve the old pin's merge base across the Factory helper source move.
        # Project destinations are relocated explicitly by the adopter, never guessed here.
        if base is not None and theirs is None and src.startswith('scripts/'):
            replacement = '.factory/' + src
            relocated = blob(source, sha, replacement)
            if relocated is not None:
                if replacement in files:
                    raise ValueError('relocated helper source is already mapped: ' + replacement)
                renamed[src] = replacement
                theirs = relocated
        ours = read(local(project, dst))
        before[dst] = ours
        candidate, conflict = merge(base, ours, theirs)
        if dst in accepted:
            candidate, conflict = ours, False
        status = 'conflict' if conflict else 'unchanged' if candidate == ours else 'delete' if candidate is None else 'add' if ours is None else 'update'
        actions.append({'source': src, 'target': dst, 'action': status, 'accepted_local': dst in accepted})
        if src in renamed:
            actions[-1]['renamed_source'] = renamed[src]
        if not conflict and candidate != ours:
            after[dst] = candidate
    names = git(source, 'diff-tree', '--no-ext-diff', '--no-textconv', '--no-renames', '--no-commit-id', '--name-only', '-r', '-z', lock['revision'], sha).stdout
    mapping({renamed.get(src, src): dst for src, dst in files.items()})
    covered = set(files) | set(renamed.values())
    unmapped = sorted(n.decode('utf-8') for n in names.split(b'\0') if n and n.decode('utf-8') not in covered)
    report = {'from_revision': lock['revision'], 'to_revision': sha, 'files': actions, 'unmapped_changes': unmapped}
    return report, before, after


def fleet(inventory, source, sha):
    value = load(inventory)
    if not isinstance(value, dict) or set(value) != {'version', 'projects'} or type(value['version']) is not int or value['version'] != 1:
        raise ValueError('invalid adopter inventory')
    projects = value['projects']
    if not isinstance(projects, list) or not 1 <= len(projects) <= 64:
        raise ValueError('inventory must contain 1 to 64 projects')
    names, paths = set(), set()
    for entry in projects:
        if not isinstance(entry, dict) or set(entry) != {'name', 'path'}:
            raise ValueError('invalid adopter entry')
        if not isinstance(entry['name'], str) or not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', entry['name']):
            raise ValueError('adopter name must be a lowercase identifier')
        if not isinstance(entry['path'], str) or not Path(entry['path']).is_absolute():
            raise ValueError('adopter path must be absolute')
        resolved = Path(entry['path']).resolve()
        if entry['name'] in names or resolved in paths:
            raise ValueError('duplicate adopter name or path')
        names.add(entry['name'])
        paths.add(resolved)
    reports = []
    failed = False
    for entry in projects:
        try:
            project = Path(entry['path']).resolve(strict=True)
            separate(project, source)
            lock = lock_data(load(local(project, LOCK)))
            report, _, _ = plan(project, source, lock, sha, [])
            conflict = any(f['action'] == 'conflict' for f in report['files'])
            status = 'conflict' if conflict else 'current' if lock['revision'] == sha else 'pending'
            reports.append({'project': entry['name'], 'status': status, 'plan': report})
            failed = failed or conflict
        except (OSError, UnicodeError, ValueError, TypeError, RecursionError):
            reports.append({'project': entry['name'], 'status': 'unavailable',
                            'next_action': 'Inspect this project locally; no files changed.'})
            failed = True
    return {'revision': sha, 'projects': reports}, 1 if failed else 0


def separate(project, source):
    if project == source or project in source.parents or source in project.parents:
        raise ValueError('source and project must be separate, non-nested checkouts')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('init', 'plan', 'apply', 'fleet'))
    parser.add_argument('--project', type=Path)
    parser.add_argument('--inventory', type=Path, help='private local adopter inventory (fleet only)')
    parser.add_argument('--source', required=True, type=Path, help='local upstream Git checkout; never executed or fetched')
    parser.add_argument('--revision', required=True, help='full immutable upstream commit SHA')
    parser.add_argument('--repository', default=REPOSITORY, help='approved upstream URL (init only)')
    parser.add_argument('--map', type=Path, help='reviewed JSON source-to-target path object (init only)')
    parser.add_argument('--task', help='existing matching task branch/active task (writes only)')
    parser.add_argument('--accept-local', action='append', default=[], metavar='PATH', help='explicitly accept this reviewed local file/deletion as its resolution')
    parser.add_argument('--review-unmapped', action='store_true', help='acknowledge reviewed upstream changes outside the map (apply only)')
    args = parser.parse_args(argv)
    try:
        source = args.source.resolve(strict=True)
        sha = revision(args.revision)
        if args.command == 'fleet':
            if args.inventory is None or args.project or args.map or args.task or args.accept_local or args.review_unmapped:
                raise ValueError('fleet requires --inventory and no per-project write options')
            report, code = fleet(args.inventory, source, sha)
            print(json.dumps(report, indent=2))
            return code
        if args.project is None or args.inventory is not None:
            raise ValueError('init/plan/apply require --project and no --inventory')
        project = args.project.resolve(strict=True)
        separate(project, source)
        lockpath = local(project, LOCK)
        if args.command == 'init':
            task_guard(project, args.task)
            if lockpath.exists() or args.map is None or args.accept_local or args.review_unmapped:
                raise ValueError('init requires --map, no existing lock and no resolution flags')
            files = mapping(load(args.map))
            source_check(source, args.repository, sha)
            for src, dst in files.items():
                if blob(source, sha, src) is None:
                    raise ValueError('initial source file is absent: ' + src)
                read(local(project, dst))
            lock = lock_data({'version': 1, 'repository': repository(args.repository), 'revision': sha, 'files': files})
            apply_files(project, {LOCK: None}, {LOCK: serialize(lock)})
            report = {'initialized': LOCK, 'revision': sha, 'mapped_files': len(files)}
        else:
            if args.map is not None:
                raise ValueError('--map is only valid for init; review lock mapping edits separately')
            original = read(lockpath)
            lock = lock_data(load(lockpath))
            report, before, after = plan(project, source, lock, sha, args.accept_local)
            if args.command == 'apply':
                task_guard(project, args.task)
                clean_paths(project, [LOCK, *lock['files'].values()])
                if any(item['action'] == 'conflict' for item in report['files']):
                    print(json.dumps(report, indent=2))
                    return 1
                if report['unmapped_changes'] and not args.review_unmapped:
                    raise ValueError('review unmapped upstream changes, then use --review-unmapped')
                if sha != lock['revision']:
                    renamed = {item['source']: item['renamed_source'] for item in report['files']
                               if 'renamed_source' in item}
                    lock['files'] = mapping({renamed.get(src, src): dst for src, dst in lock['files'].items()})
                    lock['revision'] = sha
                    after[LOCK] = serialize(lock)  # Advance provenance last.
                before[LOCK] = original
                apply_files(project, before, after)
                report['applied'] = True
        print(json.dumps(report, indent=2))
        return 1 if any(item['action'] == 'conflict' for item in report.get('files', [])) else 0
    except (OSError, UnicodeError, ValueError, TypeError, RecursionError) as exc:
        print('Template update failed: ' + str(exc), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
