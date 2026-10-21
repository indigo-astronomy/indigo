# Model selection and cost discipline

## Principle and configuration
Use the least expensive available model that can reliably satisfy the task's quality and risk requirements. Optimize total successful-delivery cost, including retries, review, context transfer, and regressions, rather than price per token alone. An expensive model is not automatically correct; a cheap model does not justify weaker gates.

[openai-model-policy.json](../openai-model-policy.json) and [antropic-model-policy.json](../antropic-model-policy.json) each map three project-defined tiers to approved model IDs and optional reasoning settings for their provider. The configured defaults below were approved for inclusion by the owner. Validate access and data-sharing permission in each adopting project; a configured model does not establish subscription entitlement or permission to incur new charges. Recheck vendor documentation when changing a mapping and record the date verified. Roles describe responsibility, not mandatory model identities.

This is a manual recommendation policy, not automatic routing, budget enforcement or a host-setting override. The opt-in [cross-review runner](cross-review.md) can apply an explicitly selected mapping to a separate review CLI invocation; the policy alone never invokes a provider. Host/owner rules and existing explicit model choices take precedence. If the host cannot switch models, record that limitation and provide a handoff rather than pretend a switch occurred. A different model does not establish review independence by itself.

## Configured models and rationale

Each provider file is the source of truth for its own `tiers`. Select `openai-model-policy.json` for OpenAI or `antropic-model-policy.json` for Anthropic; the latter filename follows the owner-requested spelling, while its provider ID remains `anthropic`. Both files use the same structure: `provider`, `tiers`, default/retry settings, budget metadata. The former combined `model-policy.json` is replaced; consumers must use the selected provider file. Budget metadata applies within the owner-defined project/task ceiling, not two additive spending allowances. `verified_on` means checked against vendor documentation on that date, not tested in the owner's account. These are project starting choices, not measured equivalence between providers.

| Tier | OpenAI model | Reasoning | Anthropic model | Effort |
| --- | --- | --- | --- | --- |
| economy | `gpt-6-luna` | `low` | `claude-haiku-5-5` | `low` |
| balanced | `gpt-6.1-sol` | `medium` | `claude-sonnet-5-5` | `medium` |
| strong | `gpt-6-astra` | `high` | `claude-opus-5-5` | `high` |

When reviewing a mapping change, compare the intended workload with the [OpenAI model catalog](https://developers.openai.com/api/docs/models). Record the documentation check, date and outcome in that task's evidence; retain only the current mapping and necessary selection rationale in shared policy. The provider configuration's `verified_on` is current metadata, not a verification history.

Anthropic's [model comparison](https://platform.claude.com/docs/en/models/overview) places Haiku 5.5 at the lowest price and latency, Sonnet 5.5 between Haiku and Opus for speed/intelligence, and Opus 5.5 above Sonnet for sustained agentic work. That supports Haiku for bounded extraction/mechanical work, Sonnet for ordinary development, and Opus for difficult debugging/review. Although Anthropic recommends starting with Opus for most workloads, this project's cost-first policy starts ordinary work at Sonnet and retains all verification gates. Fable 5.1 offers a further capability/cost step for difficult long-running work; it is outside the configured tiers. If Opus is insufficient, propose a separately reviewed mapping change rather than silently select Fable. No local comparative benchmarks or savings estimates are available.

`low`/`medium`/`high` are project effort choices. They do not promise the same reasoning, cost, or quality across models/providers. Current client support must be confirmed; do not rely on vendor defaults.

## Manual selection and setup in practice

1. Identify the assignment's role, complexity, risk and starting tier using the assessment and table below; use the selected provider file's `default_tier` (`balanced`) when no more specific rule applies. Honor an explicit owner model choice before applying defaults. If it cannot satisfy a mandatory gate, report the conflict rather than silently overriding it.
2. Select the already approved provider/environment: OpenAI for a Codex assignment, Anthropic for an assignment explicitly placed in an approved Claude environment. Provider choice is not price-based failover and does not authorize a context upload. Preserve the same task/branch and one writer through [bootstrap.md](../bootstrap.md).
3. Resolve the row manually: open the selected provider file and read `tiers[tier]`. Require both the file's `provider` and the row's `provider` to match `openai` or `anthropic` as selected. Read retry/budget settings from that same file and apply the owner's project/task ceiling. Do not merge fields across providers. A missing file, missing/null model, unknown provider/tier, or mismatched provider is unconfigured: resolve the configuration before starting that assignment.
4. Record provider, exact model ID, requested effort, rationale, permissions/budget and checks in `handoff.md`. Check that the host/account offers the model and setting. If unavailable, record the limitation and arrange a permitted handoff or owner decision. Never silently downgrade required strong work, buy credits, or change provider.
5. Apply the pair through the host's supported controls as below. Writing either JSON file or asking the current model to act as a different model does not change inference settings. A host without switching controls requires an operator handoff.
6. Confirm the actual model and effort from host status/metadata before substantive work and after resume, switch, or fallback. Record requested and observed values separately; use unknown when unobservable, and do not claim the switch succeeded. If status contradicts the requested pair, resolve the mismatch before relying on that tier. Reassess at the selected policy's `max_same_problem_attempts_before_reassessment` (currently two) or earlier under the conditions below.

### Codex

For a new CLI session in the task checkout, use one of these explicit pairs from the mapping. These are launch examples for the operator, not commands this policy runs:

```sh
codex --model gpt-6-luna -c 'model_reasoning_effort="low"'
codex --model gpt-6.1-sol -c 'model_reasoning_effort="medium"'
codex --model gpt-6-astra -c 'model_reasoning_effort="high"'
```

Codex also supports `model` and `model_reasoning_effort` in configuration. Prefer assignment-scoped controls over changing global defaults. In a client with a model/effort picker, select the exact pair there and verify the host's displayed settings. This repository neither installs host profiles nor changes the active chat's model. See [OpenAI Docs: CLI options](https://learn.chatgpt.com/docs/cli/reference) and [configuration reference](https://learn.chatgpt.com/docs/config-file/config-reference); account availability and managed settings still apply.

### Claude Code

Use exact Anthropic IDs to avoid moving `haiku`/`sonnet`/`opus` aliases. On a supported Claude Code version, launch the selected pair:

```sh
claude --model claude-haiku-5-5 --effort low
claude --model claude-sonnet-5-5 --effort medium
claude --model claude-opus-5-5 --effort high
```

For an existing session, use `/model` and `/effort` pickers; their session-only selection avoids changing saved defaults. `/status` reports the active model. Confirm effort in the picker as well. Claude Code requires v2.1.293+ for Haiku 5.5, v2.1.284+ for Sonnet 5.5, and v2.1.280+ for Opus 5.5. See [Claude Code model configuration](https://code.claude.com/docs/en/model-config). Third-party deployments require their own resolved deployment IDs and access checks; the rows above target Anthropic model IDs. Do not copy them blindly to another provider or treat any host fallback as the configured model.

For example, ordinary development in Codex resolves `balanced` to `gpt-6.1-sol`/`medium`; the same assignment in an approved Claude environment resolves to `claude-sonnet-5-5`/`medium`. A substantive security review starts at `strong`: Astra/`high` or Opus/`high` in the selected environment. Release the outgoing writer and update the handoff before another host continues.

## Suggested starting tiers
| Work | Starting tier | Upgrade when |
| --- | --- | --- |
| Coordinator bookkeeping, structured extraction, simple inventory | economy | Planning, ambiguity, cross-task ownership, or integration decisions require balanced reasoning |
| Requirements analyst | balanced | Many interacting constraints or complex domain analysis; owner still resolves product ambiguity |
| Development planner | balanced | Architecture changes, multi-service dependencies, migrations, or costly reversibility |
| Regression test designer | balanced | Adversarial cases, complex state/concurrency, permission boundaries, or subtle regressions |
| Developer: bounded mechanical edit with clear tests | economy | Scope expands or behavior cannot be confidently verified |
| Developer: ordinary feature or bug fix | balanced | Difficult debugging, unfamiliar subsystem, concurrency, broad impact, or repeated failed repairs |
| Verifier | balanced | Critical behavior, architecture, migration, or incomplete/conflicting evidence |
| Documentation and scoped prose review | economy | Operational/security claims, complex API contracts, or uncertain behavior need balanced analysis |
| Repository-wide quality/design review | balanced | Cross-cutting architecture or hard-to-establish usage requires strong analysis |
| Security review: substantive sensitive/auth/payment change | strong | Strong tier is the starting requirement; unavailable capacity means blocked or owner-approved qualified review, not silent downgrade |
| Security review: low-risk triage | balanced | A substantive security concern appears; route to strong/qualified review |

## Complexity and risk assessment
The analyst records complexity for the proposed work, with concrete evidence (relevant paths, contracts, tests or unresolved technical questions), separately from risk:

| Complexity | Criteria |
| --- | --- |
| low | Clear local change, understood behavior and existing meaningful verification; no high-complexity signal. |
| medium | Requires reasoning across understood components or additional straightforward verification, without high-complexity signals. |
| high | Requires reasoning about concurrency, idempotency, background work or external synchronization; unresolved technical uncertainty affects the solution; several coupled changes must preserve one invariant; or verification needs nontrivial state/data scenarios. |

Record which signals apply and what remains uncertain. File or call-site counts help locate coordination work but never determine complexity alone: a mechanical rename across many files can remain low. Product ambiguity must be resolved through the owner, not scored away. Update the assessment when new evidence changes it.

Risk describes the consequence and reversibility of an error, including permissions, sensitive data, financial integrity, data loss and operational impact. A one-line authorization change may have low complexity and high risk. Documentation and tests can carry complex or high-impact contracts; their file type does not justify a cheaper tier. Use existing project risk categories and mandatory review gates; this assessment introduces no new approval ceremony.

## Per-assignment decision
Before each bounded assignment, the coordinator uses its role, complexity, risk, available context and verification to choose a starting tier from the table. This includes initial analysis: use a provisional assessment from the brief until the analyst supplies evidence. No separate evaluator agent is required. Reassess at a role/scope transition, new material evidence, or stalled retry; do not switch on every tool call.

Record in handoff.md: assignment/role and allowed paths; complexity with evidence and uncertainty; risk and verification; selected tier and why it is sufficient; provider, requested model/effort and observed settings separately (unknown when unavailable); unsuccessful attempts, reason/trigger for reassessment, and any budget ceiling. Apply existing owner model choices and host permissions before the recommendation. Missing host controls require an explicit recommendation and permitted handoff, not a claimed switch. This policy itself neither schedules agents nor implements automatic routing.

Start directly at strong for high-complexity reasoning or high-impact security, irreversibility and interacting-system risks where a cheap trial is unlikely to be informative. For ordinary uncertain work, start balanced. Limit economy to clear, reversible scope with meaningful verification and no mandatory stronger-tier requirement. Routine low-risk bookkeeping may remain economy under the table. Missing evidence is a reason to investigate, not proof that economy is sufficient.

Split hard decisions from well-defined execution when useful: strong analysis may produce a bounded balanced implementation, followed by economy mechanical finishing. Each assignment keeps its own risk and review requirements. Do not downgrade automatically for docs-only, test-only, single-file work or a lean budget; never downgrade a mandatory strong review to fit a budget. Preserve task/context and relevant source evidence; do not split tasks to hide aggregate cost. Model reasoning settings remain host-specific and must be supported and allowed.

## Escalation and retry limits
After at most two unsuccessful attempts on the same problem, reassess instead of repeating the same prompt. Reassess earlier for failed acceptance criteria, contradictory results, unresolved technical uncertainty, inability to explain the change, or expanded scope/risk, including newly discovered authentication, authorization or financial effects. Evidence from checks and review drives the decision; an agent's claim of success is insufficient. Record symptoms, failed approaches, evidence and what stronger reasoning is expected to resolve. If capability is the issue, choose the next suitable approved tier (or strong immediately when required); do not mechanically exhaust every tier. At the strongest available tier, narrow/investigate or report the blocker rather than loop indefinitely.

Missing credentials, unavailable tools, broken infrastructure and unresolved product requirements need the right resource/owner decision, not merely a larger model. Keep the current permission and quality boundaries. Return well-defined remaining work to a cheaper suitable tier only at a new assignment or coherent handoff, after the difficult uncertainty is resolved and its checks are recorded.

Hand off through bootstrap.md, preserving the same task/branch and one writer. Include a concise problem summary, reproducer, relevant files and already-tried approaches. Never hide failed or aborted attempts to make a model look efficient.

## Partial results and checkpoints
An assignment may name an approximate tool-call or time checkpoint to review progress and context consumption. It is optional guidance, not a billing limit, an enforced host capability or permission to exceed an owner budget. At a checkpoint, return at a coherent point when useful; avoid stopping critical work midway solely to hit a count. Do not weaken checks or claim completion to fit it.

For incomplete work, record these fields in handoff.md or the assignment report (plain text or structured data; no host-specific format required):

- Status: partial or blocked, with the specific blocker when present.
- Completed work and changed paths, including checks actually run and their results.
- Remaining work and unperformed checks.
- Open questions/uncertainties; state none explicitly when resolved.
- Next concrete assignment, recommended tier/reassessment reason, and current writer/release state.

The coordinator uses this evidence to continue with a narrower scope, investigate a blocker or escalate. A partial result is not a passing assignment or a completed task. Retain unsuccessful-attempt history across continuations; a new context or narrower brief does not reset retries on the same unresolved problem. Partial status alone does not mean failure, but repeated partials without meaningful progress trigger reassessment within the existing retry limit.

## Budget and approval
Record an owner-defined task budget in project currency or an explicit unknown budget; null means not configured, not unlimited spending. Existing subscription access does not authorize buying extra credits, changing providers, exposing data, or launching paid API calls. Apply existing owner authorization without repeatedly asking for already approved actions. Ask only when a material new charge, provider/data boundary, or required capability exceeds that scope.

At assignment and reassessment, compare known/estimated spend with the configured ceiling. Near or over it, stop optional retries and present remaining work and alternatives. Missing cost evidence means the budget cannot be proven satisfied; do not claim it is under budget. This rule is a coordinator check, not a hard billing control. Keep mandatory safety/verification gates intact; a budget limit may leave work blocked or ready for owner action rather than falsely complete.

## Learn from evidence
At a full quality audit, or after repeated escalation, review accepted outcomes, regression evidence and failed attempts. Compare similar assignments and disclose model/settings changes, missing evidence and review limitations. Do not promise cost savings without attributable comparable evidence.
