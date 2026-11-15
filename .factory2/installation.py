"""Install a self-contained standard-library runtime; init is not an upgrade."""
from pathlib import Path
import factory_platform as platform_ops

from errors import require
atomic_bytes = platform_ops.atomic_bytes


# Explicit ownership: never include config.json, project-rules.json, local state,
# tickets, archives or host files containing project instructions in this list.
RUNTIME_FILES = (
    'factory_platform.py', 'installation.py', 'factory.py', 'tickets.py', 'flow.py', 'config.py', 'rules.py',
    'errors.py', 'policy.py', 'test_runner.py', 'default_config.json', 'console.pyz', 'Factory Console.cmd',
)


# Also embedded for source snapshots made before the launcher was a check input.
WINDOWS_LAUNCHER = b'@echo off\r\npython -B "%~dp0console.pyz" --desktop-setup\r\nif errorlevel 1 pause\r\n'


def install_missing(store):
    source = Path(__file__).parent
    for name in RUNTIME_FILES:
        target = store.safe(store.root / name)
        if target.exists():
            require(target.is_file(), 'INVALID_INSTALLATION', 'A runtime file path is occupied by a directory.')
            continue
        original = source / name
        if name == 'Factory Console.cmd' and not original.exists():
            atomic_bytes(target, WINDOWS_LAUNCHER)
            continue
        require(original.is_file() and not platform_ops.is_link_or_reparse(original), 'INVALID_INSTALLATION',
                'The source runtime is incomplete; restore it before initializing an adopter.')
        atomic_bytes(target, original.read_bytes())
        if name == 'console.pyz' and not platform_ops._is_windows():
            platform_ops.apply_mode(target, platform_ops.file_mode(target) | 0o100)
