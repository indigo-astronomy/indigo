"""Task identities, strict state selection and cooperating local lifecycle locking."""
from contextlib import contextmanager
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import tempfile
import secrets

LIMIT = 1024 * 1024
TASK = re.compile(r'[a-z0-9]+(?:-[a-z0-9]+)*')
ORDERED = re.compile(r'\d{14}-[0-9a-f]{4}-[a-z0-9]+(?:-[a-z0-9]+)*')


def local(root, path):
    path = Path(path)
    current = root
    for part in path.relative_to(root).parts:
        current /= part
        if current.is_symlink():
            raise ValueError('symbolic links are not supported for task paths')
    return path


def pairs(values):
    result = {}
    for key, value in values:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def invalid(value):
    raise ValueError('nonfinite JSON constants are not supported')


def loads(raw):
    value = json.loads(raw, object_pairs_hook=pairs, parse_constant=invalid)
    json.dumps(value, allow_nan=False)
    return value


def read(root, path):
    path = local(root, path)
    if not path.is_file():
        raise ValueError('task state must be a regular file')
    with path.open('rb') as stream:
        raw = stream.read(LIMIT + 1)
    if len(raw) > LIMIT:
        raise ValueError('task input exceeds 1 MiB')
    return raw


def valid_task(task):
    if task == 'archive' or not TASK.fullmatch(task):
        raise ValueError('invalid or reserved task ID')
    return task


def allocate(slug, moment=None, suffix=None):
    valid_task(slug)
    moment = moment or datetime.now(timezone.utc)
    stamp = moment.astimezone(timezone.utc).strftime('%Y%m%d%H%M%S')
    return stamp + '-' + (suffix or secrets.token_hex(2)) + '-' + slug


def unique_id(root, slug):
    # Branch creation reserves allocated IDs across worktrees sharing this Git repository.
    result = subprocess.run(['git', 'for-each-ref', '--format=%(refname:short)', 'refs/heads/task/'],
                            cwd=root, capture_output=True, text=True)
    branches = set(result.stdout.splitlines()) if result.returncode == 0 else set()
    for _ in range(128):
        task = allocate(slug)
        paths = [local(root, root / '.factory/tasks' / task), local(root, root / '.factory/tasks/archive' / task)]
        if any(p.exists() or p.is_symlink() for p in paths) or 'task/' + task in branches:
            continue
        repository = subprocess.run(['git', 'rev-parse', '--show-toplevel'], cwd=root, capture_output=True)
        if repository.returncode == 0:
            history = subprocess.run(['git', '--no-replace-objects', '--no-lazy-fetch', 'log', '--all', '-1', '--format=%H', '--',
                                      '.factory/tasks/' + task, '.factory/tasks/archive/' + task],
                                     cwd=root, capture_output=True, text=True)
            if history.returncode:
                raise ValueError('task ID history could not be checked')
            if history.stdout.strip():
                continue
        return task
    raise ValueError('could not allocate an unused task ID after 128 attempts; retry later')


def selected_state(root, task):
    valid_task(task)
    global_path = root / '.factory/state.json'
    repository = loads(read(root, global_path))
    if not isinstance(repository, dict):
        raise ValueError('repository state must be an object')
    if repository.get('version', 1) == 2:
        path = root / '.factory/tasks' / task / 'state.json'
    elif repository.get('version', 1) == 1:
        path = global_path
    else:
        raise ValueError('unsupported repository state version')
    state = loads(read(root, path))
    if not isinstance(state, dict) or state.get('active_task') != task:
        raise ValueError('state must select the requested task')
    return path, state


def atomic(path, raw, mode=None):
    if mode is None:
        mode = path.stat().st_mode & 0o777 if path.exists() else 0o644
    descriptor, temporary = tempfile.mkstemp(prefix='.factory-task-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(raw)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def lock_path(root):
    result = subprocess.run(['git', 'rev-parse', '--git-common-dir'], cwd=root, capture_output=True, text=True)
    if result.returncode == 0:
        common = Path(result.stdout.strip())
        if not common.is_absolute():
            common = root / common
        return common.resolve() / 'factory-task.lock'
    return local(root, root / '.factory') / '.task-lock'


@contextmanager
def lifecycle_lock(root):
    path = lock_path(root)
    try:
        path.mkdir()
    except FileExistsError:
        raise ValueError('lifecycle lock held; stop the other writer or inspect and recover its stale lock explicitly') from None
    try:
        (path / 'owner.json').write_text(json.dumps({'pid': os.getpid(), 'host': socket.gethostname()}))
        yield
    finally:
        (path / 'owner.json').unlink(missing_ok=True)
        path.rmdir()
