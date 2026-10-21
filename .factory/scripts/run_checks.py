#!/usr/bin/env python3
"""Execute an explicitly configured quality group without a shell."""
import json
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in {"check", "test", "security"}:
        print("Usage: run_checks.py {check|test|security}", file=sys.stderr)
        return 2
    group = sys.argv[1]
    try:
        config = json.loads((ROOT / ".factory/checks.json").read_text())
        commands = config[group]
        if not isinstance(commands, list) or not commands:
            raise ValueError("group must contain at least one command")
        for command in commands:
            if not isinstance(command, list) or not command:
                raise ValueError("each command must be a nonempty argument array")
            if any(not isinstance(arg, str) or not arg or "\x00" in arg for arg in command):
                raise ValueError("arguments must be nonempty strings without NUL characters")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"Configuration error for {group}: {exc}", file=sys.stderr)
        return 2
    print(f"Working directory: {ROOT}", flush=True)
    for command in commands:
        print(f"[{group}] {shlex.join(command)}", flush=True)
        try:
            result = subprocess.run(command, cwd=ROOT, check=False)
        except OSError as exc:
            print(f"Could not execute command: {exc}", file=sys.stderr)
            return 2
        if result.returncode != 0:
            print(f"Command failed with exit code {result.returncode}", file=sys.stderr)
            return 1
    print(f"Configured {group} commands passed. Assess coverage separately.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
