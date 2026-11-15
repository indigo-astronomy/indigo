"""Small, explicit configuration overlay; no rule engine."""

import copy
import json
import re
from pathlib import Path

import factory_platform as platform_ops
from errors import FactoryError, require


GIT_FLAGS = {"auto_commit", "auto_merge_and_cleanup", "auto_push"}
# Current top-level settings; upgrade.py removes every other key from an adopter's configuration.
KEYS = ("schema_version", "git", "rules", "models", "checks", "flows")
# Obsolete settings: accepted with any value and ignored until upgrade.py removes them.
OBSOLETE_KEYS = ("positive_contracts", "simple_exclude_paths")
LANGUAGE_RULE = (
    "Communicate with the human in the language they use to address Factory. "
    "Always write project documents, reports, instructions and code documentation in English."
)


def read_json(path, root=None):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "INVALID_JSON", "Duplicate JSON object key.")
            result[key] = value
        return result

    try:
        path = Path(path)
        if root is None:
            # Generic configuration callers trust the already-selected parent;
            # protected rule callers supply the checkout root explicitly.
            parent = path.parent.resolve()
            path = parent / path.name
            root = parent
        raw = platform_ops.read_regular(path, root=root)
        return json.loads(raw.decode('utf-8'), object_pairs_hook=unique)
    except (ValueError, UnicodeError) as error:
        raise FactoryError("INVALID_JSON", "A managed JSON file is invalid.") from error


def validate(layer):
    require(isinstance(layer, dict), "INVALID_CONFIG", "Configuration must be an object.")
    require(set(layer) <= set(KEYS + OBSOLETE_KEYS), "INVALID_CONFIG", "Unknown configuration setting.")
    checks = layer.get("checks", [])
    require(isinstance(checks, list) and all(
        isinstance(c, dict) and set(c) <= {"name", "command", "paths", "timeout_seconds"}
        and all(isinstance(c.get(k), str) and c[k].strip() for k in ("name", "command"))
        and isinstance(c.get("paths", []), list) and all(isinstance(p, str) and p for p in c.get("paths", []))
        and (c.get("timeout_seconds") is None or type(c["timeout_seconds"]) is int and c["timeout_seconds"] > 0)
        for c in checks) and len({c["name"] for c in checks}) == len(checks),
        "INVALID_CONFIG", "Checks need a unique name, a command and optional path patterns and timeout.")
    if "schema_version" in layer:
        require(type(layer["schema_version"]) is int and layer["schema_version"] == 1,
                "INVALID_CONFIG", "Unsupported configuration version.")
    flows = layer.get("flows", {})
    require(isinstance(flows, dict) and all(
        isinstance(name, str) and name.strip() and isinstance(flow, dict) and set(flow) <= {"approvals", "reviews"}
        and all(isinstance(flow.get(key, []), list) and all(isinstance(item, str) and item for item in flow.get(key, []))
                for key in ("approvals", "reviews"))
        and set(flow.get("approvals", [])) <= {"analysis", "design", "mockup"}
        for name, flow in flows.items()),
        "INVALID_CONFIG", "A flow profile lists approvals (analysis, design, mockup) and review kinds.")
    git = layer.get("git", {})
    require(isinstance(git, dict) and set(git) <= GIT_FLAGS | {"target_branch"},
            "INVALID_CONFIG", "Invalid Git configuration.")
    for key, value in git.items():
        if key in GIT_FLAGS:
            require(type(value) is bool, "INVALID_CONFIG", "Git permissions must be booleans.")
        else:
            require(isinstance(value, str) and value.strip() and not value.startswith(("-", "/"))
                    and not value.endswith(("/", ".")) and value not in {'@', 'HEAD'} and not value.startswith('factory/')
                    and not any(c.isspace() or ord(c) < 32 or ord(c) == 127 or c in '~^:?*[\\' for c in value)
                    and not any(s in value for s in ('..', '@{', '//'))
                    and all(not p.startswith('.') and not p.endswith('.lock') for p in value.split('/')),
                    "INVALID_CONFIG", "target_branch must be a nonempty branch name.")
    rules = layer.get("rules", {})
    require(isinstance(rules, dict), "INVALID_CONFIG", "Rules must be name-to-text pairs.")
    for key, value in rules.items():
        require(isinstance(key, str) and key.strip() and isinstance(value, str) and value.strip(),
                "INVALID_CONFIG", "Each rule needs a name and nonempty text.")
    if "language" in rules:
        require(rules["language"] == LANGUAGE_RULE, "INVALID_CONFIG",
                "Document language is always English; the language rule cannot be overridden.")
    models = layer.get("models", {})
    import policy
    policy.validate_models_patch(models)


def defaults():
    value = read_json(Path(__file__).with_name("default_config.json"))
    validate(value)
    return value


def resolve(*layers):
    result = {"schema_version": 1, "git": {key: False for key in sorted(GIT_FLAGS)},
              "rules": {"language": LANGUAGE_RULE}, "models": {}}
    for layer in layers:
        validate(layer)
        for section in ("git", "rules"):
            result[section].update(copy.deepcopy(layer.get(section, {})))
        if 'flows' in layer:
            result.setdefault('flows', {}).update(copy.deepcopy(layer['flows']))
        # Flow checks overlay by name: a later layer replaces or adds one check.
        for check in layer.get('checks', []):
            merged = [c for c in result.get('checks', []) if c['name'] != check['name']]
            result['checks'] = merged + [copy.deepcopy(check)]
        _merge(result['models'], layer.get('models', {}))
    import policy
    if result['models']:
        for provider in sorted(policy.PROVIDERS):
            policy.load(result, provider)
    return result


def _merge(target, patch):
    """Overlay policy leaves without dropping unrelated mappings or providers."""
    for key, value in patch.items():
        if isinstance(value, dict) and isinstance(target.get(key), dict):
            _merge(target[key], value)
        else:
            target[key] = copy.deepcopy(value)


def settings_scope(project, scope):
    """Resolve a canonical existing directory without following symbolic links."""
    import rules
    require(isinstance(scope, str) and (scope == '.' or (scope and
            all(p.casefold() not in {'', '.', '..', '.git', '.factory2'} for p in scope.split('/'))
            and not any(c in scope for c in '\\\x00\n\r'))),
            'INVALID_SCOPE', 'Choose an existing project-relative directory.')
    target = rules.safe(project, project if scope == '.' else project / scope)
    require(target.is_dir(), 'INVALID_SCOPE', 'The selected directory no longer exists.')
    rules.safe(project, target / '.factory2/project-rules.json')
    return target


def settings_target(store, resource, layer, scope, local_store=None):
    require(resource in {'config', 'rules'} and layer in {'project', 'local'},
            'INVALID_ARGUMENT', 'Choose config or rules and project or local.')
    directory = settings_scope(store.project, scope)
    if layer == 'local':
        primary = local_store or store
        if resource == 'rules' and scope != '.':
            import rules
            return rules.local_target(primary, scope)
        return primary.safe(primary.local / 'config.json')
    return store.safe(store.root / 'config.json') if resource == 'config' else directory / '.factory2/project-rules.json'


def settings_document(path, resource, layer):
    import rules
    value = read_json(path) if path.exists() else {}
    if resource == 'rules' and (layer == 'project' or path.name == 'rules.json'):
        if path.exists():
            rules.validate(value)
    else:
        validate(value)
    return value


def settings_fields():
    import policy
    def leaves(value, prefix=''):
        result = []
        for key, item in value.items():
            field = prefix + '/' + key if prefix else key
            result.extend(leaves(item, field) if isinstance(item, dict) else [field])
        return result
    result = leaves({k: v for k, v in defaults().items() if k in {'git', 'models'}})
    result += ['git/target_branch']
    for provider in sorted(policy.PROVIDERS):
        result += ['models/' + provider + '/selection/' + key for key in ('tier', 'model', 'reasoning', 'choice')]
    return sorted(set(result))


def settings_get(value, field):
    if field.endswith('/selection/choice'):
        selected = settings_get(value, field[:-7])
        return {k: selected[k] for k in ('model', 'reasoning') if k in selected} if isinstance(selected, dict) and 'model' in selected else None
    for part in field.split('/'):
        if not isinstance(value, dict) or part not in value:
            return None
        value = value[part]
    return value


def settings_local_store(app):
    """The checkout that owns the Local layer: the primary of a linked worktree."""
    from tickets import Tickets
    return Tickets(app.store.local.parent.parent)


def settings_snapshot(app, scope):
    import os
    import rules
    store = app.store
    directory = settings_scope(store.project, scope)
    project = settings_document(settings_target(store, 'config', 'project', scope), 'config', 'project')
    local = settings_document(settings_target(settings_local_store(app), 'config', 'local', '.'), 'config', 'local')
    effective = app._effective_config()
    configured, directories = ['.'], ['.']
    for root, dirs, files in os.walk(store.project, followlinks=False):
        dirs[:] = sorted(d for d in dirs if d.casefold() not in {'.git', '.factory2'} and not platform_ops.is_link_or_reparse(Path(root) / d))
        path = Path(root)
        if path == store.project:
            continue
        relative = path.relative_to(store.project).as_posix()
        directories.append(relative)
        marker = path / '.factory2'
        if marker.is_dir() and not platform_ops.is_link_or_reparse(marker):
            configured.append(relative)
    owned = settings_document(directory / '.factory2/project-rules.json', 'rules', 'project')
    local_store = settings_local_store(app)
    local_owned = local if scope == '.' else settings_document(
        settings_target(store, 'rules', 'local', scope, local_store), 'rules', 'local')
    resolved = rules.directory_rules(store.project, scope, local_store.project, local.get('rules', {}))
    inherited, sources = resolved['rules'], resolved['sources']
    inherited_without_local = resolved['inherited']
    effective['rules'] = dict(inherited)
    fields = []
    for field in settings_fields():
        fields.append({'field': field, 'project': settings_get(project, field), 'local': settings_get(local, field),
                       'effective': settings_get(effective, field),
                       'source': 'local' if settings_get(local, field) is not None else 'project' if settings_get(project, field) is not None else 'default'})
    return {'project': store.project.name, 'scope': scope, 'scopes': configured, 'directories': directories,
            'config': {'project': project, 'local': local, 'effective': effective}, 'fields': fields,
            'rules': [{'name': name, 'project': owned.get('rules', {}).get(name), 'local': local_owned.get('rules', {}).get(name),
                       'effective': value, 'inherited': inherited_without_local.get(name), 'source': sources[name], 'protected': name == 'language'}
                      for name, value in sorted(inherited.items())]}


def settings_update(app, arguments, request_id, fingerprint):
    app._arguments(arguments, ['scope', 'resource', 'layer', 'field', 'value', 'remove'])
    scope, resource, layer = (arguments.get(k, default) for k, default in [('scope', '.'), ('resource', None), ('layer', None)])
    settings_scope(app.store.project, scope)
    target = settings_target(app.store, resource, layer, scope, settings_local_store(app))
    old = settings_document(target, resource, layer)
    rules_only = resource == 'rules' and (layer == 'project' or scope != '.')
    # Factory holds the write lock: patch the current document, never a client snapshot.
    field = arguments.get('field')
    remove = arguments.get('remove', False)
    require(type(remove) is bool and ('value' in arguments) != remove, 'INVALID_ARGUMENT', 'Supply a value or explicit removal.')
    require(isinstance(field, str) and bool(field.strip()) and len(field) <= 200,
            'INVALID_ARGUMENT', 'A named field is required.')
    if resource == 'config':
        require(field in settings_fields(), 'INVALID_ARGUMENT', 'This setting is unknown or read-only.')
        parts = field.split('/')
    else:
        require(field != 'language' and not any(c in field for c in '\x00\n\r'),
                'PROTECTED_RULE', 'Choose a rule name other than the protected language rule.')
        parts = ['rules', field]
    proposed = copy.deepcopy(old)
    node = proposed
    for key in parts[:-1]:
        node = node.setdefault(key, {})
    paired = resource == 'config' and field.endswith('/selection/choice')
    if remove:
        for key in (('model', 'reasoning') if paired else (parts[-1],)):
            node.pop(key, None)
        # Prune only the edited branch; unrelated empty mappings stay untouched.
        for depth in range(len(parts) - 1, 0, -1):
            parent = proposed
            for key in parts[:depth - 1]: parent = parent[key]
            if parent.get(parts[depth - 1]) == {}: del parent[parts[depth - 1]]
            else: break
        if rules_only: proposed.setdefault('rules', {})
    else:
        if paired:
            pair = arguments['value']
            require(isinstance(pair, dict) and set(pair) == {'model', 'reasoning'},
                    'INVALID_CONFIG', 'Supply both an explicit model and reasoning.')
            node.update(pair)
        else:
            node[parts[-1]] = arguments['value']
    if rules_only:
        import rules
        if not old: proposed.setdefault('schema_version', 1)
        rules.validate(proposed)
    else:
        validate(proposed)
        shared = proposed if layer == 'project' else settings_document(settings_target(app.store, 'config', 'project', scope), 'config', 'project')
        local = proposed if layer == 'local' else settings_document(settings_target(settings_local_store(app), 'config', 'local', '.'), 'config', 'local')
        resolve(shared)
        resolve(shared, local)
    # Settings affect future assignments; never change an already issued assignment.
    result = {'scope': scope, 'resource': resource, 'layer': layer, 'field': field, 'saved': True}
    app.store.settings_local_project = settings_local_store(app).project
    app.store.commit_settings(target, old if target.exists() else None, proposed,
                              request_id, {'fingerprint': fingerprint, 'result': result})
    return result
