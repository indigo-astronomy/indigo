"""Git option compatibility for the Factory helpers.

The helpers read local objects only and must never start a lazy fetch from a promisor remote. Git 2.44 added
`--no-lazy-fetch` for that. Older Git cannot disable lazy fetching, but it can only fetch lazily when a promisor
remote is configured (a partial clone: `extensions.partialClone`, `remote.<name>.promisor` or
`remote.<name>.partialCloneFilter`). Without one, omitting the flag is equivalent. With one, the helpers fail closed.
"""
from functools import lru_cache
from pathlib import Path
import re
import subprocess

NO_LAZY_FETCH_VERSION = (2, 44)


@lru_cache(maxsize=None)
def git_version():
    try:
        result = subprocess.run(['git', '--version'], capture_output=True, text=True)
    except OSError:
        return None
    match = re.search(r'(\d+)\.(\d+)', result.stdout) if result.returncode == 0 else None
    return (int(match.group(1)), int(match.group(2))) if match else None


def supports_no_lazy_fetch():
    version = git_version()
    return version is not None and version >= NO_LAZY_FETCH_VERSION


def has_promisor_remote(root):
    # Configuration is read without objects, so these commands cannot fetch.
    partial = subprocess.run(['git', 'config', '--get', 'extensions.partialclone'], cwd=root, capture_output=True, text=True)
    if partial.returncode == 0:
        return True
    if partial.returncode != 1:
        raise ValueError('Git configuration unavailable')
    # A partial-clone filter alone makes its remote a promisor remote, whatever its value.
    filtered = subprocess.run(['git', 'config', '--get-regexp', r'^remote\..*\.partialclonefilter$'], cwd=root, capture_output=True, text=True)
    if filtered.returncode == 0:
        return True
    if filtered.returncode != 1:
        raise ValueError('Git configuration unavailable')
    promisor = subprocess.run(['git', 'config', '--type=bool', '--get-regexp', r'^remote\..*\.promisor$'], cwd=root, capture_output=True, text=True)
    if promisor.returncode == 1:
        return False
    if promisor.returncode != 0:
        raise ValueError('Git configuration unavailable')
    return any(line.rsplit(' ', 1)[-1] == 'true' for line in promisor.stdout.splitlines())


def no_lazy_fetch(root):
    """Return the global Git options that keep a command from lazy fetching in the repository at root."""
    if supports_no_lazy_fetch():
        return ['--no-lazy-fetch']
    if has_promisor_remote(Path(root)):
        raise ValueError('Git 2.44 or later is required for a partial clone (--no-lazy-fetch)')
    return []
