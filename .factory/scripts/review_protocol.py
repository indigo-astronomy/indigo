"""Pure review packet/result validation shared by runner and committed process checks."""
import hashlib
import json
from pathlib import PurePosixPath
import re

from git_snapshot import object_pairs

ROLES = ('verifier', 'security-reviewer', 'quality-reviewer', 'repository-reviewer')
PROVIDERS = {'codex': 'openai', 'claude': 'anthropic'}
POLICY = '.factory/review-policy.json'
MAX_BYTES = 1024 * 1024


def encode(value):
    return (json.dumps(value, sort_keys=True, ensure_ascii=True, allow_nan=False) + '\n').encode()


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def loads(raw):
    if len(raw) > MAX_BYTES:
        raise ValueError('review JSON exceeds 1 MiB')
    return json.loads(raw, object_pairs_hook=object_pairs,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('non-finite JSON')))


def relative(path):
    if (not isinstance(path, str) or not path or path.startswith('/') or '\\' in path or
            any(ord(c) < 32 for c in path) or any(p in ('', '.', '..') for p in path.split('/'))):
        raise ValueError('invalid repository-relative review path')
    return path


def allowed(path):
    relative(path)
    parts = PurePosixPath(path).parts
    return not (path.startswith(('.factory/tasks/archive/', '.factory/findings/archive/', '.factory/telemetry/')) or
                any(p in ('.git', '.ssh', 'node_modules', '.venv') for p in parts) or
                any(p.startswith('.env') or p.endswith(('.pem', '.key', '.local.json', '.local.jsonl')) for p in parts))


def policy(data):
    fields = {'version', 'allowed_reviewers', 'share_paths', 'required_roles', 'timeout_seconds', 'max_attempts'}
    if not isinstance(data, dict) or set(data) != fields or type(data['version']) is not int or data['version'] != 1:
        raise ValueError('invalid review policy fields/version')
    for key, choices in (('allowed_reviewers', PROVIDERS), ('required_roles', ROLES)):
        values = data[key]
        if not isinstance(values, list) or any(not isinstance(v, str) or v not in choices for v in values) or len(set(values)) != len(values):
            raise ValueError('invalid review policy ' + key)
    if not isinstance(data['share_paths'], list) or any(relative(p) != p for p in data['share_paths']):
        raise ValueError('invalid review share paths')
    for field, high in (('timeout_seconds', 3600), ('max_attempts', 10)):
        if type(data[field]) is not int or not 1 <= data[field] <= high:
            raise ValueError('invalid review policy ' + field)
    return data


def schema():
    text = {'type': 'string'}
    finding = {'type': 'object', 'additionalProperties': False,
               'properties': {'path': text, 'line': {'type': 'integer', 'minimum': 1},
                              'severity': {'enum': ['critical', 'high', 'medium', 'low', 'informational']},
                              'title': text, 'evidence': text, 'impact': text},
               'required': ['path', 'line', 'severity', 'title', 'evidence', 'impact']}
    props = {'packet_sha256': text, 'status': {'enum': ['completed', 'incomplete']},
             'verdict': {'enum': ['pass', 'findings', 'inconclusive']}, 'summary': text,
             'findings': {'type': 'array', 'items': finding},
             'questions': {'type': 'array', 'items': text},
             'limitations': {'type': 'array', 'items': text}}
    return {'type': 'object', 'additionalProperties': False, 'properties': props, 'required': list(props)}


def result(data, packet_hash, paths):
    if not isinstance(data, dict) or set(data) != set(schema()['properties']):
        raise ValueError('invalid review result fields')
    if data['packet_sha256'] != packet_hash or data['status'] not in ('completed', 'incomplete') or data['verdict'] not in ('pass', 'findings', 'inconclusive'):
        raise ValueError('review result identity/status/verdict mismatch')
    if not isinstance(data['summary'], str) or not data['summary'].strip():
        raise ValueError('review summary required')
    for key in ('findings', 'questions', 'limitations'):
        if not isinstance(data[key], list):
            raise ValueError('review arrays required')
    for key in ('questions', 'limitations'):
        if any(not isinstance(t, str) or not t.strip() for t in data[key]):
            raise ValueError('review text entries must be nonempty')
    for f in data['findings']:
        if not isinstance(f, dict) or set(f) != {'path', 'line', 'severity', 'title', 'evidence', 'impact'}:
            raise ValueError('invalid finding fields')
        if not isinstance(f['path'], str) or f['path'] not in paths or type(f['line']) is not int or f['line'] < 1:
            raise ValueError('finding must identify an input path and positive line')
        if f['severity'] not in ('critical', 'high', 'medium', 'low', 'informational') or any(
                not isinstance(f[k], str) or not f[k].strip() for k in ('title', 'evidence', 'impact')):
            raise ValueError('finding needs severity and concrete evidence/impact')
    if data['verdict'] == 'pass' and (data['status'] != 'completed' or data['findings'] or data['questions']):
        raise ValueError('passing review cannot be incomplete or contain findings/questions')
    if data['verdict'] == 'findings' and not data['findings']:
        raise ValueError('findings verdict requires findings')
    return data


def validate_evidence(read, directory, task, head, role=None):
    """Validate content integrity; this does not authenticate a reviewer or approval."""
    relative(directory)
    if not re.fullmatch(r'\.factory/tasks/(?:archive/)?' + re.escape(task) + r'/reviews/[a-zA-Z0-9-]+', directory):
        raise ValueError('review evidence must belong to the selected task')
    record = loads(read(directory + '/result.json'))
    packet_raw = read(directory + '/packet.json').encode('utf-8')
    packet = loads(packet_raw)
    manifest = loads(read(directory + '/manifest.json'))
    if not all(isinstance(v, dict) for v in (record, packet, manifest)):
        raise ValueError('review evidence must contain JSON objects')
    record_fields = {'version', 'status', 'reviewer', 'role', 'packet_sha256', 'requested_model',
                     'requested_effort', 'observed_model', 'observed_effort', 'cli_version',
                     'started_at', 'finished_at', 'review', 'failure'}
    packet_fields = {'version', 'task', 'role', 'base', 'head', 'selected', 'sources', 'files', 'before', 'diff'}
    if set(record) != record_fields or set(packet) != packet_fields:
        raise ValueError('invalid review evidence fields')
    if (not isinstance(record['requested_model'], str) or not record['requested_model'] or
            record['requested_effort'] not in ('low', 'medium', 'high') or
            record['observed_model'] not in (None, record['requested_model']) or
            record['observed_effort'] not in (None, record['requested_effort']) or
            record['failure'] is not None):
        raise ValueError('review metadata mismatch/failure')
    if type(record.get('version')) is not int or record['version'] != 1 or record.get('status') != 'completed':
        raise ValueError('review execution did not complete')
    if (packet.get('task') != task or packet.get('head') != head or packet.get('role') not in ROLES or
            (role and packet['role'] != role)):
        raise ValueError('review is stale or covers a different task/role')
    if (not isinstance(packet.get('files'), dict) or
            any(not isinstance(p, str) or not isinstance(v, str) for p, v in packet['files'].items()) or
            not isinstance(packet.get('selected'), list) or
            any(not isinstance(p, str) for p in packet['selected'])):
        raise ValueError('invalid review packet file manifest')
    expected = {p: digest(v.encode('utf-8')) for p, v in packet['files'].items()}
    if (manifest != {'version': 1, 'packet_sha256': digest(packet_raw), 'files': expected} or
            record.get('packet_sha256') != digest(packet_raw)):
        raise ValueError('review packet/manifest integrity failed')
    if record.get('reviewer') not in PROVIDERS or record.get('role') != packet['role']:
        raise ValueError('review record provider/role mismatch')
    result(record['review'], digest(packet_raw), packet['files'])
    if record['review']['verdict'] != 'pass':
        raise ValueError('review has unresolved findings or is inconclusive')
    return record, packet
