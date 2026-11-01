#!/usr/bin/env python3
"""Derive maintenance from immutable per-task completion records."""
import argparse
from datetime import date, datetime, timezone
import json
from pathlib import Path
import re
import sys
from zoneinfo import ZoneInfo

import task_store as store

ROOT = Path(__file__).resolve().parents[2]
CONFIG = '.factory/maintenance.json'
EVENTS = '.factory/completions'
SHA = re.compile(r'[0-9a-f]{40}|[0-9a-f]{64}')
FIELDS = {'version', 'task', 'completed_at', 'kind', 'audit', 'evidence_revision'}


def read(root, name, snapshot=None):
    data = snapshot.json(name) if snapshot else store.loads(store.read(root, root / name))
    json.dumps(data, allow_nan=False)
    return data


def config(root, snapshot=None):
    exists = snapshot.exists(CONFIG) if snapshot else store.local(root, root / CONFIG).exists()
    if not exists:
        return None
    value = read(root, CONFIG, snapshot)
    if not isinstance(value, dict):
        raise ValueError('maintenance configuration must be an object')
    if value.get('version', 1) == 1:
        return None
    if value.get('version') != 2:
        raise ValueError('unsupported maintenance version')
    policy = value.get('repository_review')
    if not isinstance(policy, dict):
        raise ValueError('repository_review policy missing')
    for key in ('full_review_every_completed_tasks', 'full_review_every_calendar_days', 'incremental_review_every_completed_tasks'):
        if key == 'incremental_review_every_completed_tasks' and key not in policy:
            continue
        if key == 'full_review_every_completed_tasks' and policy.get(key) is None and key in policy:
            continue
        if type(policy.get(key)) is not int or policy[key] < 1:
            raise ValueError('positive maintenance threshold required; only the full task threshold may be null')
    baseline = policy.get('incremental_review_baseline')
    if baseline is not None:
        if 'incremental_review_every_completed_tasks' not in policy or not isinstance(baseline, dict) or set(baseline) != {'revision', 'authority', 'report_task'}:
            raise ValueError('incremental migration baseline needs revision, authority and report_task')
        if not isinstance(baseline['revision'], str) or not SHA.fullmatch(baseline['revision']):
            raise ValueError('incremental baseline must reference a full retained commit')
        if not isinstance(baseline['authority'], str) or not baseline['authority'].strip():
            raise ValueError('incremental baseline needs explicit owner authority')
        store.valid_task(baseline['report_task'])
    if not isinstance(value.get('timezone'), str) or not value['timezone']:
        raise ValueError('maintenance timezone required')
    forbidden = {'completed_non_audit_tasks_since_full_review', 'last_full_review_task',
                 'last_full_review_date', 'last_full_review_revision', 'last_task_review_task'}
    if forbidden & policy.keys():
        raise ValueError('version 2 maintenance stores policy, not mutable counters/checkpoints')
    return value


def moment(value):
    if not isinstance(value, str):
        raise ValueError('completion timestamp required')
    result = datetime.fromisoformat(value.replace('Z', '+00:00'))
    if result.tzinfo is None or result.utcoffset().total_seconds() != 0:
        raise ValueError('completion timestamp must be UTC with an explicit offset')
    return result


def validate(record):
    if not isinstance(record, dict) or set(record) != FIELDS or type(record['version']) is not int or record['version'] != 1:
        raise ValueError('invalid completion record fields/version')
    store.valid_task(record['task'])
    moment(record['completed_at'])
    if record['kind'] not in ('work', 'audit', 'incremental-audit', 'partial-audit'):
        raise ValueError('invalid completion kind')
    revision = record['evidence_revision']
    if revision is not None and (not isinstance(revision, str) or not SHA.fullmatch(revision)):
        raise ValueError('completion evidence revision must be a full commit or null')
    audit = record['audit']
    if record['kind'] in ('audit', 'incremental-audit') and audit is None:
        raise ValueError('completed full/incremental audit requires explicit audit metadata')
    if record['kind'] == 'partial-audit' and audit is not None:
        raise ValueError('partial audit cannot declare completed review coverage')
    if audit is not None:
        if not isinstance(audit, dict) or set(audit) != {'reviewed_on', 'revision', 'covered_tasks'}:
            raise ValueError('invalid audit completion metadata')
        date.fromisoformat(audit['reviewed_on'])
        if not isinstance(audit['revision'], str) or not SHA.fullmatch(audit['revision']):
            raise ValueError('audit source must be a full commit')
        covered = audit['covered_tasks']
        if not isinstance(covered, list) or any(not isinstance(t, str) for t in covered) or len(set(covered)) != len(covered):
            raise ValueError('audit covered_tasks must be a unique task list')
        for task in covered:
            store.valid_task(task)
        if record['task'] in covered:
            raise ValueError('audit task cannot declare its own final completion covered')


def records(root, snapshot=None):
    exists = snapshot.exists(EVENTS) if snapshot else store.local(root, root / EVENTS).is_dir()
    if not exists:
        raise ValueError('completion directory missing')
    paths = [Path(p) for p in snapshot.entries if p.startswith(EVENTS + '/')] if snapshot else (root / EVENTS).iterdir()
    result = []
    for path in sorted(paths):
        if snapshot and path.parent.as_posix() != EVENTS:
            raise ValueError('completion directory must contain flat records')
        if snapshot:
            if snapshot.entries[path.as_posix()][0] not in ('100644', '100755'):
                raise ValueError('completion entries must be regular files')
        else:
            store.local(root, path)
        if path.name == '.gitkeep':
            continue
        if path.suffix != '.json':
            raise ValueError('unexpected completion entry')
        record = read(root, path.as_posix() if snapshot else path.relative_to(root).as_posix(), snapshot)
        validate(record)
        if path.stem != record['task']:
            raise ValueError('completion filename/task mismatch')
        result.append(record)
    known = {record['task'] for record in result}
    for record in result:
        if record['audit'] and set(record['audit']['covered_tasks']) - known:
            raise ValueError('audit coverage refers to unknown completion records')
    return result



def audit_report(root, task, snapshot=None, verdict='pass', migration=False):
    reports = []
    for prefix in ('', 'archive/'):
        name = '.factory/tasks/' + prefix + task + '/repository-review.md'
        exists = snapshot.exists(name) if snapshot else store.local(root, root / name).exists()
        if exists:
            reports.append(snapshot.read(name) if snapshot else store.read(root, root / name).decode('utf-8'))
    values = re.findall(r'(?m)^(?:- )?Verdict:\s*(.*?)\s*$', reports[0]) if len(reports) == 1 else []
    accepted = values == [verdict]
    if migration and len(values) == 1:
        accepted = re.fullmatch(r'partial(?:\s+[—-].*)?', values[0]) is not None
    if not accepted:
        raise ValueError('audit needs exactly one repository-review.md verdict: ' + verdict)


def baseline_tasks(root, policy, snapshot=None):
    baseline = policy['repository_review'].get('incremental_review_baseline')
    if baseline is None:
        return set()
    from git_snapshot import Snapshot
    candidate = snapshot if snapshot is not None else Snapshot(root, 'HEAD')
    if not candidate.ancestor(baseline['revision']):
        raise ValueError('incremental baseline must remain in candidate ancestry')
    source = candidate.at(baseline['revision'])
    audit_report(root, baseline['report_task'], source, verdict='partial', migration=True)
    return {r['task'] for r in records(root, source) if r['kind'] == 'work'}

def check(root, snapshot=None, pruning=()):
    policy = config(root, snapshot)
    if policy is None:
        return []
    baseline_tasks(root, policy, snapshot)
    values = records(root, snapshot)
    pins = {}
    def pinned(revision):
        if revision not in pins:
            if snapshot:
                pins[revision] = snapshot.at(revision)
            else:
                from git_snapshot import Snapshot
                pins[revision] = Snapshot(root, revision)
        candidate = snapshot
        if candidate is None:
            from git_snapshot import Snapshot
            if 'HEAD' not in pins:
                pins['HEAD'] = Snapshot(root, 'HEAD')
            candidate = pins['HEAD']
        if not candidate.ancestor(revision):
            raise ValueError('completion/audit evidence commit must remain in candidate ancestry')
        return pins[revision]
    for record in values:
        revision = record['evidence_revision']
        if record['task'] in pruning and revision is None:
            raise ValueError('pin completed task state before pruning its reports/state')
        source = pinned(revision) if revision else snapshot
        states = []
        locations = [f'.factory/tasks/{prefix}{record["task"]}' for prefix in ('', 'archive/')]
        if sum(source.exists(p) if source else store.local(root, root / p).exists() for p in locations) != 1:
            raise ValueError('completion task location is missing or ambiguous')
        for prefix in ('', 'archive/'):
            name = '.factory/tasks/' + prefix + record['task'] + '/state.json'
            present = source.exists(name) if source else store.local(root, root / name).exists()
            if present:
                states.append(read(root, name, source))
        if len(states) != 1 or any(not isinstance(s, dict) or s.get('active_task') != record['task'] or s.get('status') != 'complete' or s.get('phase') != 'complete' or s.get('blocker') is not None for s in states):
            raise ValueError('completion needs exactly one evidenced completed task state: ' + record['task'])
        if record['audit']:
            audit_report(root, record['task'], source)
            pinned(record['audit']['revision'])
        elif record['kind'] == 'partial-audit':
            audit_report(root, record['task'], source, verdict='partial')
    return values


def incremental_coverage(values, baseline):
    covered = set(baseline)
    for record in values:
        if record['audit']:
            covered.update(record['audit']['covered_tasks'])
    return covered


def status(policy, values, today=None, baseline=None):
    if today is None:
        today = datetime.now(timezone.utc if policy['timezone'] == 'UTC' else ZoneInfo(policy['timezone'])).date()
    ordered = sorted(values, key=lambda r: (moment(r['completed_at']), r['task']))
    audits = sorted((r for r in ordered if r['audit'] and r['kind'] != 'incremental-audit'), key=lambda r: (date.fromisoformat(r['audit']['reviewed_on']), moment(r['completed_at']), r['task']))
    latest = audits[-1] if audits else None
    covered = set(latest['audit']['covered_tasks']) if latest else set()
    outstanding = [r['task'] for r in ordered if r['kind'] == 'work' and r['task'] not in covered]
    elapsed = (today - date.fromisoformat(latest['audit']['reviewed_on'])).days if latest else None
    if elapsed is not None and elapsed < 0:
        raise ValueError('audit date is in the future relative to the requested date')
    rules = policy['repository_review']
    threshold = rules['full_review_every_completed_tasks']
    output = {'completed_tasks': len(values), 'completed_non_audit_tasks_since_full_review': len(outstanding),
            'uncovered_work_tasks': outstanding, 'last_full_review_task': latest['task'] if latest else None,
            'last_full_review_date': latest['audit']['reviewed_on'] if latest else None,
            'last_full_review_revision': latest['audit']['revision'] if latest else None,
            'last_task_review_task': ordered[-1]['task'] if ordered else None,
            'full_review_due': latest is None or (threshold is not None and len(outstanding) >= threshold) or elapsed >= rules['full_review_every_calendar_days']}
    if 'incremental_review_every_completed_tasks' in rules:
        if rules.get('incremental_review_baseline') is not None and baseline is None:
            raise ValueError('resolve the retained incremental baseline before deriving status')
        baseline = set(baseline or ())
        unseen = [r['task'] for r in ordered if r['kind'] == 'work' and r['task'] not in incremental_coverage(values, baseline)]
        incremental = sorted((r for r in ordered if r['kind'] == 'incremental-audit'), key=lambda r: (date.fromisoformat(r['audit']['reviewed_on']), moment(r['completed_at']), r['task']))
        last_incremental = incremental[-1] if incremental else None
        output.update(completed_non_audit_tasks_since_incremental_review=len(unseen), uncovered_incremental_work_tasks=unseen,
                      incremental_review_due=len(unseen) >= rules['incremental_review_every_completed_tasks'],
                      last_incremental_review_task=last_incremental['task'] if last_incremental else None,
                      last_incremental_review_date=last_incremental['audit']['reviewed_on'] if last_incremental else None,
                      incremental_review_baseline_revision=rules.get('incremental_review_baseline', {}).get('revision'),
                      incremental_review_baseline_task_count=len(baseline))
    return output


def completion_for_archive(root, task, state):
    if config(root) is None:
        return None
    directory = store.local(root, root / EVENTS)
    if not directory.is_dir():
        raise ValueError('completion directory missing; finish maintenance migration')
    path = store.local(root, directory / (task + '.json'))
    if path.exists():
        raise ValueError('completion already exists; preserve the original task identity/evidence')
    metadata = state.get('completion', {})
    if not isinstance(metadata, dict) or set(metadata) - {'kind', 'audit'}:
        raise ValueError('task completion metadata accepts kind and audit only')
    record = {'version': 1, 'task': task, 'completed_at': datetime.now(timezone.utc).isoformat(),
              'kind': metadata.get('kind', 'work'), 'audit': metadata.get('audit'), 'evidence_revision': None}
    validate(record)
    known = {r['task'] for r in records(root)}
    if record['audit'] and set(record['audit']['covered_tasks']) - known:
        raise ValueError('audit coverage refers to unknown task completions')
    if record['audit']:
        audit_report(root, task)
    elif record['kind'] == 'partial-audit':
        audit_report(root, task, verdict='partial')
    return path, (json.dumps(record, indent=2, allow_nan=False) + '\n').encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('status', 'check', 'audit-input'))
    parser.add_argument('--today', type=date.fromisoformat)
    parser.add_argument('--revision', help='reviewed source commit for audit-input')
    parser.add_argument('--on', type=date.fromisoformat, help='actual reviewed date for audit-input')
    parser.add_argument('--scope', choices=('full', 'incremental'), default='full', help='scope for audit-input; full remains the legacy default')
    parser.add_argument('--prune-task', action='append', default=[])
    args = parser.parse_args()
    try:
        policy = config(ROOT)
        if policy is None:
            raise ValueError('migrate legacy maintenance before deriving status')
        if args.command == 'audit-input':
            if not args.revision or not SHA.fullmatch(args.revision) or not args.on:
                raise ValueError('audit-input needs --revision FULL_SHA and --on DATE')
            from git_snapshot import Snapshot
            snap = Snapshot(ROOT, args.revision)
            if not Snapshot(ROOT, 'HEAD').ancestor(args.revision):
                raise ValueError('audit source must be retained in current ancestry')
            values = records(ROOT, snap)
            if args.scope == 'incremental':
                if 'incremental_review_every_completed_tasks' not in policy['repository_review']:
                    raise ValueError('incremental review policy is not configured')
                covered = incremental_coverage(values, baseline_tasks(ROOT, policy, snap))
                tasks = sorted(r['task'] for r in values if r['kind'] == 'work' and r['task'] not in covered)
            else:
                tasks = sorted(r['task'] for r in values)
            output = {'kind': 'incremental-audit' if args.scope == 'incremental' else 'audit',
                      'audit': {'reviewed_on': args.on.isoformat(), 'revision': args.revision, 'covered_tasks': tasks}}
        else:
            values = check(ROOT, pruning=args.prune_task)
            output = status(policy, values, args.today, baseline_tasks(ROOT, policy)) if args.command == 'status' else {'valid': True, 'completed_tasks': len(values)}
        print(json.dumps(output, indent=2))
    except (OSError, ValueError, TypeError, KeyError, RuntimeError) as exc:
        print('Maintenance failed: ' + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
