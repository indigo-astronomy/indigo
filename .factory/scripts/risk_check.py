"""Conservative diff risk signals; baseline policy cannot be weakened by the candidate."""
import fnmatch
import re

POLICY = '.factory/process-policy.json'
DEFAULT_POLICY = {
    'version': 1,
    'rules': [
        {'id': 'authentication', 'paths': ['*auth*', '*permission*', '*identity*', '*access-control*'], 'report': 'security.md'},
        {'id': 'data-model', 'paths': ['*migration*', '*schema*', '*.sql', '*models/*'], 'report': 'security.md'},
        {'id': 'process-policy', 'paths': ['AGENTS.md', '*/AGENTS.md', 'CLAUDE.md', 'GEMINI.md',
                                         '.factory/roles/*', '.factory/routines/*', '.factory/bootstrap.md',
                                         '.factory/workflow.md', '.factory/process-policy.json',
                                         '.factory/checks.json', '.cursor/rules/*', '.github/copilot-instructions.md',
                                         '.factory/scripts/process_check.py', '.factory/scripts/risk_check.py', '.factory/scripts/git_snapshot.py',
                                         '.factory/scripts/findings.py', '.factory/scripts/decision_index.py', '.factory/scripts/process-check',
                                         '.factory/review-policy.json', '.factory/scripts/review', '.factory/scripts/review.py',
                                         '.factory/scripts/review_adapters.py', '.factory/scripts/review_protocol.py',
                                         '.factory/scripts/task_store.py', '.factory/scripts/task_upgrade.py',
                                         '.factory/scripts/metadata_upgrade.py', '.factory/scripts/maintenance.py',
                                         # Historical ranges and adopters may still use legacy destinations.
                                         'scripts/process_check.py', 'scripts/risk_check.py', 'scripts/git_snapshot.py',
                                         'scripts/findings.py', 'scripts/decision_index.py', 'scripts/process-check',
                                         'scripts/review', 'scripts/review.py', 'scripts/review_adapters.py',
                                         'scripts/review_protocol.py'],
         'report': 'security.md'},
        {'id': 'ci-deployment', 'paths': ['.github/workflows/*', '.gitlab-ci.yml', 'azure-pipelines*.yml',
                                        '*Dockerfile*', '*docker-compose*', '*deploy*', '*terraform*', '*.tf'],
         'report': 'security.md'},
    ],
    'dependency_manifests': ['package.json', '*/package.json', '*requirements*.txt', '*pyproject.toml',
                             '*Pipfile*', '*poetry.lock', '*uv.lock', '*package-lock.json', '*yarn.lock',
                             '*pnpm-lock.yaml', '*Cargo.toml', '*Cargo.lock', '*go.mod', '*go.sum',
                             '*Gemfile*', '*pom.xml', '*build.gradle*', '*composer.json', '*composer.lock', '*.csproj'],
}


def patterns(value):
    return isinstance(value, list) and len(value) <= 128 and all(
        isinstance(p, str) and 0 < len(p) <= 256 and not p.startswith('/') and '..' not in p.split('/')
        and '\\' not in p and not any(ord(c) < 32 for c in p) for p in value)


def policy(base):
    data = base.json(POLICY) if POLICY in base.entries else DEFAULT_POLICY
    if not isinstance(data, dict) or set(data) != {'version', 'rules', 'dependency_manifests'}:
        raise ValueError('invalid process policy fields')
    if type(data['version']) is not int or data['version'] != 1 or not patterns(data['dependency_manifests']):
        raise ValueError('invalid process policy version/dependency patterns')
    rules, ids = data['rules'], {'dependencies'}
    if not isinstance(rules, list) or len(rules) > 64:
        raise ValueError('risk rules must be a list of at most 64 entries')
    for rule in rules:
        if not isinstance(rule, dict) or set(rule) != {'id', 'paths', 'report'}:
            raise ValueError('invalid risk rule fields')
        key = rule['id']
        if not isinstance(key, str) or not re.fullmatch(r'[a-z][a-z0-9-]{0,63}', key) or key in ids:
            raise ValueError('invalid/duplicate risk rule ID')
        if not patterns(rule['paths']) or rule['report'] not in ('security.md', 'verification.md'):
            raise ValueError('invalid risk rule paths/report')
        ids.add(key)
    return data


def matches(path, globs):
    return any(fnmatch.fnmatchcase(path.lower(), pattern.lower()) for pattern in globs)


def package_names(snapshot, path):
    if path not in snapshot.entries:
        return set()
    data = snapshot.json(path)
    if not isinstance(data, dict):
        raise ValueError('package.json must be an object')
    names = set()
    for group in ('dependencies', 'devDependencies', 'peerDependencies', 'optionalDependencies'):
        entries = data.get(group, {})
        if not isinstance(entries, dict) or any(not isinstance(v, str) for v in entries.values()):
            raise ValueError('invalid package dependency group')
        names.update(entries)
    return names


def requirement_names(snapshot, path):
    if path not in snapshot.entries:
        return set()
    names = set()
    for line in snapshot.read(path).splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        match = re.match(r'([A-Za-z0-9][A-Za-z0-9_.-]*)(?:\[|\s*@|\s*[<>=!~;]|\s*$)', line)
        # Preserve directives/URLs as conservative signals rather than silently ignore them.
        names.add(re.sub(r'[-_.]+', '-', match[1]).lower() if match else line)
    return names


def classify(base, head, changed):
    data = policy(base)
    if POLICY in changed:
        policy(head)  # Validate the next policy too, while applying only the baseline's rules.
    reasons = []
    for path in changed:
        if path.startswith('.factory/tasks/'):
            continue  # Evidence directory names must not classify the implementation's risk.
        for rule in data['rules']:
            if matches(path, rule['paths']):
                reasons.append({'rule': rule['id'], 'path': path, 'report': rule['report'],
                                'detail': 'changed path matches baseline risk policy'})
        if not matches(path, data['dependency_manifests']):
            continue
        if path.split('/')[-1] == 'package.json':
            added = package_names(head, path) - package_names(base, path)
        elif re.fullmatch(r'.*requirements[^/]*\.txt', path.split('/')[-1]):
            added = requirement_names(head, path) - requirement_names(base, path)
        else:
            # Other manifest and lock formats need review on any change; no partial parser pretends certainty.
            added = {'manifest/lockfile changed; inspect dependencies'}
        if added:
            reasons.append({'rule': 'dependencies', 'path': path, 'report': 'security.md',
                            'detail': 'new dependency entries or conservative manifest signal',
                            'entry_count': len(added)})
    return reasons
