"""CLI discovery and bounded noninteractive Codex/Claude protocol adapters."""
import os
from pathlib import Path
import selectors
import shutil
import signal
import subprocess
import sys
import time

from review_protocol import MAX_BYTES, loads, encode, schema


def executable(name, overrides):
    if name in overrides:
        candidate = Path(overrides[name]).expanduser()
        if not candidate.is_absolute() or not candidate.is_file() or not os.access(candidate, os.X_OK):
            raise ValueError('configured reviewer executable is not an absolute executable file')
        return str(candidate)
    found = shutil.which(name)
    if found:
        return found
    candidates = [Path.home() / '.local/bin' / name]
    if sys.platform == 'darwin' and name == 'codex':
        candidates += [Path('/Applications/ChatGPT.app/Contents/Resources/codex-cli/bin/codex'),
                       Path('/Applications/ChatGPT.app/Contents/Resources/codex-cli/CodexCLI.app/Contents/MacOS/codex')]
    for candidate in candidates:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return str(candidate)
    raise ValueError(name + ' CLI unavailable; install/authenticate separately or configure an absolute local executable')


def invoke(argv, cwd, stdin=b'', timeout=20):
    """POSIX process group with bounded output and a deadline, including descendants."""
    env = dict(os.environ, AI_FACTORY_REVIEW_CHILD='1', DISABLE_AUTOUPDATER='1')
    # Never pass a caller's terminal/session identity to the child.
    env.pop('CLAUDECODE', None)
    with subprocess.Popen(argv, cwd=cwd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, env=env, start_new_session=True) as child:
        selector = selectors.DefaultSelector()
        selector.register(child.stdout, selectors.EVENT_READ, 'out')
        selector.register(child.stderr, selectors.EVENT_READ, 'err')
        selector.register(child.stdin, selectors.EVENT_WRITE, 'in')
        output, errors, offset = bytearray(), bytearray(), 0
        deadline = time.monotonic() + timeout
        try:
            while selector.get_map():
                if time.monotonic() >= deadline:
                    raise ValueError('reviewer timeout')
                for key, _ in selector.select(min(0.1, max(0, deadline - time.monotonic()))):
                    if key.data == 'in':
                        if offset == len(stdin):
                            selector.unregister(key.fileobj)
                            key.fileobj.close()
                        else:
                            try:
                                offset += os.write(key.fd, stdin[offset:offset + 4096])
                            except BrokenPipeError:
                                selector.unregister(key.fileobj)
                                key.fileobj.close()
                    else:
                        chunk = os.read(key.fd, 65536)
                        if not chunk:
                            selector.unregister(key.fileobj)
                        else:
                            (output if key.data == 'out' else errors).extend(chunk)
                            if len(output) + len(errors) > MAX_BYTES:
                                raise ValueError('reviewer output exceeds 1 MiB')
            try:
                child.wait(timeout=max(0.01, deadline - time.monotonic()))
            except subprocess.TimeoutExpired:
                raise ValueError('reviewer timeout') from None
            return child.returncode, bytes(output), bytes(errors)
        finally:
            selector.close()
            # Also terminate a descendant that closed its inherited stdio early.
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            child.wait()


REQUIRED = {
    'codex': ('--output-schema', '--ignore-user-config', '--ephemeral', '--sandbox', '--json'),
    'claude': ('--safe-mode', '--tools', '--output-format', '--json-schema', '--effort', '--no-session-persistence'),
}


def probe(name, path, cwd):
    code, version, _ = invoke([path, '--version'], cwd)
    if code:
        raise ValueError(name + ' version probe failed')
    args = [path, 'exec', '--help'] if name == 'codex' else [path, '--help']
    code, help_text, _ = invoke(args, cwd)
    if code or any(flag not in help_text.decode('utf-8', 'replace') for flag in REQUIRED[name]):
        raise ValueError(name + ' CLI lacks required noninteractive/safety flags')
    return version.decode('utf-8', 'replace').strip()[:200]


def command(name, path, model, effort, workspace):
    if name == 'codex':
        return [path, 'exec', '--ignore-user-config', '--ignore-rules', '--ephemeral',
                '--skip-git-repo-check', '--sandbox', 'read-only', '--json',
                '-c', 'features.shell_tool=false', '-c', 'features.unified_exec=false',
                '-c', 'features.multi_agent=false', '-c', 'features.multi_agent_v2=false',
                '-c', 'features.apps=false', '-c', 'features.computer_use=false',
                '-c', 'features.browser_use=false', '-c', 'features.in_app_browser=false',
                '-c', 'web_search="disabled"',
                '--model', model, '-c', 'model_reasoning_effort="' + effort + '"',
                '--output-schema', str(workspace / 'schema.json'),
                '--output-last-message', str(workspace / 'answer.json'), '-']
    return [path, '-p', '--safe-mode', '--tools', '', '--disallowedTools', 'mcp__*',
            '--no-session-persistence', '--output-format', 'json', '--model', model,
            '--effort', effort, '--json-schema', encode(schema()).decode()]


def answer(name, workspace, stdout):
    if name == 'codex':
        path = workspace / 'answer.json'
        if path.is_symlink() or not path.is_file() or path.stat().st_size > MAX_BYTES:
            raise ValueError('Codex final structured response missing/invalid')
        # JSONL events are not treated as proof of model/effort identity.
        return loads(path.read_bytes()), None
    envelope = loads(stdout)
    if not isinstance(envelope, dict) or envelope.get('is_error') or envelope.get('subtype') != 'success':
        raise ValueError('Claude did not return a successful structured response')
    models = envelope.get('modelUsage')
    observed = next(iter(models)) if isinstance(models, dict) and len(models) == 1 else None
    return envelope.get('structured_output'), observed
