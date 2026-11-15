"""Factory-owned defaults and project-owned rules scoped to explicit file paths."""
from pathlib import Path, PurePosixPath

import config
import factory_platform as platform_ops
from errors import require


def validate(document):
    require(isinstance(document, dict) and set(document) <= {'schema_version', 'rules'}
            and 'rules' in document, 'INVALID_RULES', 'A rule file must contain a rules object.')
    config.validate(document)
    return document['rules']


def safe(project, path):
    require(path == project or project in path.parents, 'UNSAFE_PATH', 'Rule paths must stay inside the project.')
    for part in (path, *path.parents):
        require(not platform_ops.is_link_or_reparse(part), 'UNSAFE_PATH', 'Rule and assigned paths must not use symlinks.')
        if part == project:
            break
    return path


def read(project, path, optional=False):
    safe(project, path)
    if optional and not path.exists():
        return {}
    require(path.is_file(), 'RULES_MISSING', 'Initialize Factory rule files before using this project.')
    try:
        return validate(config.read_json(path, root=project))
    except OSError as error:
        require(False, 'UNSAFE_PATH', 'Rule files must remain accessible without symlinks or reparse points.')


def paths(project, values):
    require(isinstance(values, list), 'INVALID_PATH', 'Assigned paths must be a list of project-relative file paths.')
    result = []
    for value in values:
        require(isinstance(value, str) and value and not any(c in value for c in '\\*?[]\x00\n\r'),
                'INVALID_PATH', 'Use concrete file paths with / separators, without glob patterns.')
        path = PurePosixPath(value)
        require(not path.is_absolute() and all(part not in {'', '.', '..'} for part in value.split('/')),
                'INVALID_PATH', 'Use canonical project-relative file paths without traversal.')
        target = safe(project, project / value)
        require(not target.is_dir(), 'INVALID_PATH', 'Specify files, not directory-wide or recursive scopes.')
        require(value not in result, 'INVALID_PATH', 'Assigned paths must be unique.')
        result.append(value)
    return result


def directory_rules(project, scope='.', local_project=None, local_rules=None, overrides=None):
    """Resolve a directory once, retaining the actual winning owner of each key."""
    project = Path(project)
    primary = Path(local_project) if local_project is not None else project
    chain = [project]
    current = project
    if scope != '.':
        for part in PurePosixPath(scope).parts:
            current /= part
            chain.append(current)
    values = {'language': config.LANGUAGE_RULE}
    sources = {'language': '.factory2/rules.json'}
    def overlay(mapping, source):
        for name, text in mapping.items():
            values[name], sources[name] = text, source
    overlay(read(project, project / '.factory2/rules.json'), '.factory2/rules.json')
    for directory in chain:
        path = directory / '.factory2/project-rules.json'
        overlay(read(project, path, optional=True), path.relative_to(project).as_posix())
    inherited = dict(values)
    overlay(local_rules or {}, 'local')
    for directory in chain[1:]:
        relative = directory.relative_to(project).as_posix()
        # Planned files and runtime metadata remain valid assignments, but only
        # existing eligible project directories own scoped Local rules.
        if not directory.is_dir() or any(p.casefold() in {'.git', '.factory2'} for p in PurePosixPath(relative).parts):
            continue
        from tickets import Tickets
        store = Tickets(primary)
        path = local_target(store, relative)
        overlay(read(primary, path, optional=True), 'local/' + relative)
    overlay(overrides or {}, 'ticket')
    return {'rules': values, 'sources': sources, 'inherited': inherited}


def local_target(store, scope):
    """Private destination; checkout scope validation belongs to the caller."""
    require(isinstance(scope, str) and scope != '.' and not PurePosixPath(scope).is_absolute()
            and all(p.casefold() not in {'', '.', '..', '.git', '.factory2'} for p in scope.split('/'))
            and not any(c in scope for c in '\\\x00\n\r'),
            'INVALID_SCOPE', 'Choose an existing project-relative directory.')
    return store.safe(store.local / 'rules' / scope / '.factory2/rules.json')


def compose(project, assigned=(), overrides=None, *, local_project=None, local_rules=None):
    project = Path(project)
    assigned = paths(project, list(assigned))
    root = directory_rules(project, local_project=local_project, local_rules=local_rules, overrides=overrides)
    scoped, sources = {}, {}
    for value in assigned:
        scope = PurePosixPath(value).parent.as_posix()
        resolved = directory_rules(project, scope, local_project, local_rules, overrides)
        scoped[value], sources[value] = resolved['rules'], resolved['sources']
    return {'rules': root['rules'], 'rules_by_path': scoped,
            'rules_sources': root['sources'], 'rules_sources_by_path': sources}


def private_instructions(projection):
    maps = projection['rules_sources_by_path'].values() if projection['rules_by_path'] else [projection['rules_sources']]
    # Project directories may be named local. Their reserved rule-file suffix
    # cannot occur in a canonical Local scope, which excludes .factory2.
    return any(source == 'local' or (source.startswith('local/')
               and not source.endswith('/.factory2/project-rules.json'))
               for mapping in maps for source in mapping.values())


def instructions(projection):
    def render(values):
        return '\n'.join(name + ': ' + text for name, text in sorted(values.items()))
    if not projection['rules_by_path']:
        return ('Active rules (project scope; no file scope assigned)\n' + render(projection['rules'])
                + '\nFile edits require an assignment with explicit file paths and their applicable rules.')
    return ('Active rules by assigned file; each complete rule set applies only to its named file. '
            'Request a new scoped assignment before editing additional files.\n\n'
            + '\n\n'.join('File: ' + path + '\n' + render(values)
                           for path, values in projection['rules_by_path'].items()))
