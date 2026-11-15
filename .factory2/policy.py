"""Executable provider policies; selection does not launch workers or grant spending."""

import copy
from errors import require
from tickets import digest

PROVIDERS = {"openai", "anthropic", "deepseek"}
TIERS = ["economy", "balanced", "strong"]
DIFFICULTIES = {"low", "medium", "high", "unknown"}
PHASE_TYPES = {"project": "planning", "analysis": "analysis", "design": "planning", "mockup": "implementation",
               "final": "review", "development": "implementation", "testing": "verification", "documentation": "documentation"}
ACTIONS = {"plan", "implement", "review"}
TYPES = {"analysis", "planning", "implementation", "test_design", "verification", "review",
         "documentation", "security_review", "repository_review"}


def nonempty(value):
    return isinstance(value, str) and bool(value.strip()) and len(value) <= 200


def task_type(phase, action):
    require(phase in PHASE_TYPES and action in ACTIONS and (phase != "final" or action == "review"), "INVALID_ARGUMENT", "Choose an explicit phase and plan, implement or review action.")
    return "planning" if action == "plan" else "review" if action == "review" else PHASE_TYPES[phase]


SHARED_FIELDS = {'assignment_types', 'difficulty_floor', 'retry_escalation'}
FIELDS = SHARED_FIELDS | {'schema_version', 'tiers'}


def validate_patch(value):
    require(isinstance(value, dict) and set(value) <= {'tiers', 'selection'},
            'INVALID_POLICY', 'Provider blocks contain only tiers and optional selection; routing rules belong directly under models.')
    validate_selection(value)


def validate_models_patch(models):
    require(isinstance(models, dict) and set(models) <= SHARED_FIELDS | PROVIDERS,
            'INVALID_CONFIG', 'models contains shared routing rules and provider blocks, not a default provider.')
    for provider in PROVIDERS & set(models):
        validate_patch(models[provider])


def validate_selection(value):
    selection = value.get('selection', {})
    require(isinstance(selection, dict) and set(selection) <= {'tier', 'model', 'reasoning'},
            'INVALID_CONFIG', 'Invalid explicit model selection.')
    require(all(nonempty(v) for v in selection.values()), 'INVALID_CONFIG', 'Explicit selections must be nonempty text.')
    require('tier' not in selection or selection['tier'] in TIERS, 'INVALID_CONFIG', 'Unsupported model tier.')


def validate(value, provider):
    require(isinstance(value, dict) and FIELDS <= set(value) <= FIELDS | {'selection'}
            and type(value.get('schema_version')) is int and value['schema_version'] == 1
            and provider in PROVIDERS,
            "INVALID_POLICY", "Invalid provider policy header.")
    validate_selection(value)
    tiers = value["tiers"]
    require(isinstance(tiers, dict) and set(tiers) == set(TIERS), "INVALID_POLICY", "All three model tiers must be explicit.")
    for choice in tiers.values():
        require(isinstance(choice, dict) and set(choice) == {"model", "reasoning"}
                and all(nonempty(v) for v in choice.values()), "INVALID_POLICY", "Every tier needs an explicit model and reasoning.")
    for key, required in (("assignment_types", TYPES), ("difficulty_floor", DIFFICULTIES)):
        mapping = value[key]
        require(isinstance(mapping, dict) and set(mapping) == required
                and all(v in TIERS for v in mapping.values()), "INVALID_POLICY", "Policy mappings must cover all supported types and difficulties.")
    retry = value["retry_escalation"]
    require(isinstance(retry, dict) and set(retry) == {"after_failed_attempts", "tier"}
            and type(retry["after_failed_attempts"]) is int and retry["after_failed_attempts"] >= 1
            and retry["tier"] in TIERS, "INVALID_POLICY", "Invalid retry escalation rule.")
    selection = value.get('selection', {})
    require(('model' in selection) == ('reasoning' in selection), 'INVALID_CONFIG',
            'An explicit model choice needs explicit reasoning too.')
    return value


def load(settings, provider):
    require(provider in PROVIDERS, "INVALID_POLICY", "Unsupported model provider.")
    models = settings.get('models', {})
    validate_models_patch(models)
    require(SHARED_FIELDS <= set(models), 'INVALID_POLICY', 'Shared model routing rules must be explicit.')
    require(provider in models, 'POLICY_MISSING', 'The host provider needs an explicit policy in config.json.')
    # The public projection and policy identity include exactly the shared rules
    # plus this host's choices. They do not duplicate the stored configuration.
    effective = {key: copy.deepcopy(models[key]) for key in SHARED_FIELDS}
    effective.update(schema_version=settings.get('schema_version', 1))
    effective.update(copy.deepcopy(models[provider]))
    return validate(effective, provider)


def select(settings, task, provider):
    require(provider in PROVIDERS, 'HOST_PROVIDER_REQUIRED', 'Report the current native host provider explicitly; no provider is inherited from configuration.')
    policy = load(settings, provider)
    options = policy.get('selection', {})
    base = policy["assignment_types"][task["type"]]
    difficulty = task.get("difficulty", "unknown")
    floor = policy["difficulty_floor"][difficulty]
    tier = max((base, floor), key=TIERS.index)
    reasons = ["assignment/" + task["type"], "difficulty/" + difficulty]
    failures = sum(a["status"] in {"failed", "interrupted"}
                   and (a.get("outcome") or {}).get("error_code") != "CREDIT_EXHAUSTED" for a in task["attempts"])
    escalation = policy["retry_escalation"]
    if failures >= escalation["after_failed_attempts"]:
        tier = max((tier, escalation["tier"]), key=TIERS.index)
        reasons.append("retry/escalation")
    if "tier" in options:
        tier = options["tier"]
        reasons.append("owner/tier")
    requested = {"provider": provider, **copy.deepcopy(policy["tiers"][tier])}
    require(("model" in options) == ("reasoning" in options), "INVALID_CONFIG", "An explicit model choice needs explicit reasoning too.")
    if "model" in options:
        requested.update(model=options["model"], reasoning=options["reasoning"])
        reasons.append("owner/model")
    return {"policy_id": provider + "/" + str(policy["schema_version"]) + "/" + digest(policy),
            "tier": tier, "requested": requested, "reasons": reasons,
            "assignment_type": task["type"], "difficulty": difficulty, "failed_attempts": failures,
            "spending_authorized": False}
