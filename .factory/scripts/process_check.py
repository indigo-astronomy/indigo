#!/usr/bin/env python3
"""Check committed task evidence and change coverage without executing candidate commands."""
import argparse
import fnmatch
import json
from pathlib import Path
import re
import sys

import decision_index
import findings
import risk_check
import review_protocol
from git_snapshot import Snapshot, git

ROOT = Path(__file__).resolve().parents[2]
TASK = re.compile(r'[a-z0-9]+(?:-[a-z0-9]+)*')
TASK_PATH = re.compile(r'^\.factory/tasks/(?:archive/)?([a-z0-9]+(?:-[a-z0-9]+)*)/')
REPORTS = ('verification.md', 'security.md', 'quality-review.md')
DOCS = ('brief.md', 'specification.md', 'plan.md', 'test-plan.md', 'handoff.md') + REPORTS
BOOKKEEPING = {'.factory/state.json', '.factory/maintenance.json'}


def source_path(path):
    return not path.startswith('.factory/tasks/') and path not in BOOKKEEPING


def valid_path(path):
    return (isinstance(path, str) and bool(path) and not path.startswith('/') and
            all(part not in ('', '.', '..') for part in path.split('/')) and
            '\\' not in path and not any(ord(c) < 32 for c in path))


def content_entries(data, scope, hash_length):
    if not isinstance(data, dict):
        raise ValueError('reviewed_content must be a path-to-identity object')
    result = {}
    for path, identity in data.items():
        if not valid_path(path) or not source_path(path) or not matches_scope(path, scope):
            raise ValueError('reviewed content path is invalid or outside task source scope')
        if identity is None:
            result[path] = None  # Explicitly reviewed deletion.
            continue
        if (not isinstance(identity, list) or len(identity) != 3 or
                not all(isinstance(value, str) for value in identity) or
                (identity[0], identity[1]) not in {('100644', 'blob'), ('100755', 'blob'),
                                                     ('120000', 'blob'), ('160000', 'commit')} or
                not re.fullmatch('[0-9a-f]{' + str(hash_length) + '}', identity[2])):
            raise ValueError('invalid reviewed content identity: ' + path)
        result[path] = tuple(identity)
    return result


def staged_content(root, base_ref):
    """Describe changed index entries; never stage files or execute candidate code."""
    base = Snapshot(root, base_ref)
    entries = {}
    for record in git(root, 'ls-files', '--stage', '-z').split(b'\0'):
        if not record:
            continue
        metadata, raw_path = record.split(b'\t', 1)
        mode, oid, stage = metadata.decode().split()
        if stage != '0':
            raise ValueError('resolve unmerged index entries before recording reviewed content')
        entries[raw_path.decode('utf-8')] = (mode, 'commit' if mode == '160000' else 'blob', oid)
    return {path: entries.get(path) for path in sorted(set(base.entries) | set(entries))
            if source_path(path) and base.entries.get(path) != entries.get(path)}


def meaningful(text):
    body = re.sub(r'^\s*#+[^\n]*$', '', text, flags=re.M).strip()
    return bool(body) and not re.search(r'\bTODO\b|^\s*(?:[-*] )?(?:TBD|Not yet verified)\s*$', text, re.M)


def matches_scope(path, scope):
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in scope)


def task_root(snapshot, task):
    roots = [f'.factory/tasks/{prefix}{task}' for prefix in ('', 'archive/')]
    present = [path for path in roots if snapshot.exists(path)]
    if len(present) > 1:
        raise ValueError('ambiguous live/archive task: ' + task)
    return present[0] if present else None


def evidence_snapshot(base, head, task):
    directory = task_root(head, task)
    if directory:
        return head, directory
    # Legacy pruned-template ranges retain their evidence in ancestor commits.
    paths = [f'.factory/tasks/{prefix}{task}/process.json' for prefix in ('', 'archive/')]
    history = git(head.root, 'log', '--format=%H', '--max-count=201', base.revision + '..' + head.revision, '--', *paths).decode().splitlines()
    if len(history) > 200:
        raise ValueError('task history exceeds 200 commits; narrow the base range')
    for revision in history:
        snapshot = head.at(revision)
        directory = task_root(snapshot, task)
        if directory and directory + '/process.json' in snapshot.entries:
            return snapshot, directory
    raise ValueError('task evidence missing from candidate and retained range: ' + task)


def task_evidence(base, head, task):
    snapshot, directory = evidence_snapshot(base, head, task)
    data = snapshot.json(directory + '/process.json')
    identity_key = 'reviewed_content' if isinstance(data, dict) and data.get('version') == 2 else 'reviewed_revision'
    fields = {'version', 'task', 'scope', 'checks', 'risk_reviews', identity_key}
    if not isinstance(data, dict) or set(data) not in (fields, fields | {'reviews'}):
        raise ValueError('invalid process evidence fields: ' + task)
    if type(data['version']) is not int or data['version'] not in (1, 2) or data['task'] != task:
        raise ValueError('process evidence version/task mismatch: ' + task)
    revision = data.get('reviewed_revision')
    if data['version'] == 1 and (not isinstance(revision, str) or not head.ancestor(revision)):
        raise ValueError('reviewed revision must be a retained full commit SHA: ' + task)
    scope = data['scope']
    if not isinstance(scope, list) or not scope or any(not isinstance(p, str) or not p or p.startswith('/') or
                                                     '..' in p.split('/') or '\\' in p or any(ord(c) < 32 for c in p) for p in scope):
        raise ValueError('task needs repository-relative scope globs: ' + task)
    reviewed = (content_entries(data['reviewed_content'], scope, len(head.revision)) if data['version'] == 2
                else head.at(revision).entries)
    reports = {}
    for name in DOCS:
        text = snapshot.read(directory + '/' + name)
        if not meaningful(text):
            raise ValueError('task document is incomplete: ' + task + '/' + name)
        if name in REPORTS:
            if re.findall(r'^\s*(?:- )?Verdict: (.+?)\s*$', text, re.M) != ['pass']:
                raise ValueError('task report needs one explicit passing verdict: ' + task + '/' + name)
            reports[name] = text
    checks = data['checks']
    if not isinstance(checks, list) or not checks:
        raise ValueError('task needs recorded check results: ' + task)
    for check in checks:
        if not isinstance(check, dict) or set(check) != {'command', 'result', 'evidence'}:
            raise ValueError('invalid recorded check fields: ' + task)
        command = check['command']
        if not isinstance(command, list) or not command or any(not isinstance(a, str) or not a or '\x00' in a for a in command):
            raise ValueError('recorded command must be an argument array: ' + task)
        if check['result'] != 'pass' or check['evidence'] not in reports:
            raise ValueError('recorded check must pass and reference a reviewed report: ' + task)
    risk_reviews = data['risk_reviews']
    if not isinstance(risk_reviews, dict) or any(not isinstance(k, str) or not k or v not in REPORTS for k, v in risk_reviews.items()):
        raise ValueError('risk reviews must map rule IDs to passing report names: ' + task)
    review_entries = data.get('reviews', [])
    if not isinstance(review_entries, list):
        raise ValueError('reviews must be a list: ' + task)
    if data['version'] == 2 and review_entries:
        raise ValueError('commit-bound cross-review packets require version 1 evidence: ' + task)
    required = []
    approved = None
    if review_protocol.POLICY in base.entries:
        baseline_policy = review_protocol.policy(base.json(review_protocol.POLICY))
        required = baseline_policy['required_roles']
        approved = baseline_policy['allowed_reviewers']
    covered = set()
    for entry in review_entries:
        if (not isinstance(entry, dict) or set(entry) != {'role', 'directory'} or
                entry['role'] not in review_protocol.ROLES):
            raise ValueError('invalid cross-review reference: ' + task)
        logical = entry['directory']
        if not isinstance(logical, str):
            raise ValueError('invalid cross-review directory')
        match = re.fullmatch(r'\.factory/tasks/(?:archive/)?' + re.escape(task) + r'/reviews/([a-zA-Z0-9-]+)', logical)
        if not match:
            raise ValueError('cross-review reference must belong to the selected task')
        actual = directory + '/reviews/' + match[1]
        record, packet = review_protocol.validate_evidence(snapshot.read, actual, task, revision, entry['role'])
        if approved is not None and record['reviewer'] not in approved:
            raise ValueError('cross-review provider not authorized by baseline policy')
        # Rebuild from immutable objects; packet hashes alone cannot establish source identity.
        import review
        expected, _, _ = review.build_packet(head.root, task, entry['role'], base.revision, revision, packet['selected'])
        if packet != expected:
            raise ValueError('cross-review packet differs from the committed source range')
        covered.add(entry['role'])
    if set(required) - covered:
        raise ValueError('required cross-review roles missing: ' + ', '.join(sorted(set(required) - covered)))
    return {'task': task, 'revision': snapshot.revision, 'reviewed_revision': revision, 'directory': directory,
            'scope': scope, 'risk_reviews': risk_reviews, 'reviewed_content': reviewed}


def inspect(root, base_ref, head_ref, requested, risk_only=False):
    if git(root, 'rev-parse', '--is-shallow-repository').strip() != b'false':
        raise ValueError('complete local history required; fetch the approved base/history first')
    base, head = Snapshot(root, base_ref), Snapshot(root, head_ref)
    if not head.ancestor(base.revision):
        raise ValueError('base must be an ancestor of head; pass the merge base or synchronize first')
    changed = [p.decode('utf-8') for p in git(root, 'diff', '--no-ext-diff', '--no-renames', '--name-only', '-z',
                                             base.revision, head.revision, '--').split(b'\0') if p]
    risks = risk_check.classify(base, head, changed)
    if risk_only:
        return {'base': base.revision, 'head': head.revision, 'risk_reasons': risks, 'errors': [],
                'limitation': 'Risk signals only; no task evidence or approval was checked.'}
    # Find task evidence even when it was committed then pruned within this range.
    historical = git(root, 'log', '--format=', '--name-only', base.revision + '..' + head.revision,
                     '--', '.factory/tasks/').decode().splitlines()
    touched = {m[1] for path in historical + changed if (m := TASK_PATH.match(path)) and m[1] != 'archive'}
    tasks = sorted(set(requested) if requested else touched)
    if any(not TASK.fullmatch(task) or task == 'archive' for task in tasks):
        raise ValueError('invalid task ID')
    if set(tasks) - touched:
        raise ValueError('selected task has no evidence changes in this range')
    errors, evidence = [], []
    if changed and not tasks:
        errors.append('changed range has no task evidence')
    for task in tasks:
        try:
            evidence.append(task_evidence(base, head, task))
        except ValueError as exc:
            errors.append(str(exc))
    def reviewed_path(item, path):
        entries = item['reviewed_content']
        # A missing v2 deletion is not an explicitly reviewed deletion.
        return ((item['reviewed_revision'] is not None or path in entries) and
                entries.get(path) == head.entries.get(path))
    for path in changed:
        match = TASK_PATH.match(path)
        if match and match[1] in tasks:
            continue
        owners = [item for item in evidence if matches_scope(path, item['scope'])]
        if not owners:
            errors.append('changed path is outside evidenced task scope: ' + path)
        elif source_path(path) and not any(reviewed_path(item, path) for item in owners):
            errors.append('changed path differs from its reviewed revision: ' + path)
    for reason in risks:
        if not any(matches_scope(reason['path'], item['scope']) and
                   reviewed_path(item, reason['path']) and
                   item['risk_reviews'].get(reason['rule']) == reason['report'] for item in evidence):
            errors.append('risk needs recorded review: ' + reason['rule'] + ': ' + reason['path'])
    try:
        data = findings.active_register(head)
        records = findings.all_records(data, head)
        findings.check_tasks(records, set(), head)
        for record in data['findings']:
            if record['blocking']:
                errors.append('unresolved blocking finding: ' + record['id'])
    except (ValueError, TypeError) as exc:
        errors.append('findings integrity failed: ' + str(exc))
    try:
        expected = decision_index.render(head.entries, head.read)
        if head.read(decision_index.INDEX) != expected:
            errors.append('decision index is stale')
    except ValueError as exc:
        errors.append('decision index failed: ' + str(exc))
    for item in evidence:
        del item['reviewed_content']  # Do not print the full repository inventory for legacy reports.
    return {'base': base.revision, 'head': head.revision, 'tasks': evidence,
            'changed_paths': changed, 'risk_reasons': risks, 'errors': errors,
            'limitation': 'Structural evidence only; no recorded command or approval was independently verified.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', help='existing ancestor commit or ref; no fetch is performed')
    parser.add_argument('--head', default='HEAD', help='committed candidate; dirty files are ignored')
    parser.add_argument('--task', action='append', default=[], help='restrict to changed task IDs; repeatable')
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--risk-only', action='store_true', help='describe diff risk without checking task evidence')
    modes.add_argument('--show-policy', action='store_true', help='print built-in defaults for project configuration')
    modes.add_argument('--record-content', action='store_true', help='print staged source identities for one task; no writes')
    args = parser.parse_args()
    if args.show_policy:
        print(json.dumps(risk_check.DEFAULT_POLICY, indent=2))
        return 0
    if not args.base:
        parser.error('--base is required unless --show-policy is used')
    try:
        if args.record_content:
            if len(args.task) != 1 or not TASK.fullmatch(args.task[0]) or args.task[0] == 'archive' or args.risk_only:
                raise ValueError('--record-content requires exactly one valid --task and no --risk-only')
            # Capture only the assigned scope from a regular live or archived manifest.
            directories = [ROOT / '.factory/tasks' / prefix / args.task[0] for prefix in ('', 'archive')]
            present = [path for path in directories if path.exists()]
            if len(present) != 1:
                raise ValueError('expected exactly one live/archive task for content recording')
            manifest = present[0] / 'process.json'
            current = ROOT
            for part in manifest.relative_to(ROOT).parts:
                current /= part
                if current.is_symlink():
                    raise ValueError('symlinks are not supported for process evidence paths')
            if not manifest.is_file():
                raise ValueError('process evidence must be a regular file')
            with manifest.open('rb') as stream:
                raw = stream.read(1024 * 1024 + 1)
            if len(raw) > 1024 * 1024:
                raise ValueError('process evidence exceeds 1 MiB')
            from git_snapshot import object_pairs
            data = json.loads(raw, object_pairs_hook=object_pairs)
            if (not isinstance(data, dict) or type(data.get('version')) is not int or data.get('version') != 2 or data.get('task') != args.task[0] or
                    not risk_check.patterns(data.get('scope')) or not data['scope']):
                raise ValueError('content recording needs version 2 task evidence and valid scope')
            report = {path: identity for path, identity in staged_content(ROOT, args.base).items()
                      if matches_scope(path, data['scope'])}
            report = {path: list(identity) if identity is not None else None for path, identity in report.items()}
            content_entries(report, data['scope'], len(Snapshot(ROOT, args.base).revision))
        else:
            report = inspect(ROOT, args.base, args.head, args.task, args.risk_only)
    except (OSError, ValueError, TypeError, RecursionError) as exc:
        print('Process check unavailable: ' + str(exc), file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2))
    return 1 if not args.record_content and report['errors'] else 0


if __name__ == '__main__':
    sys.exit(main())
