# Using AI Factory across models and coding tools

## Shared contract
The same project rules, roles, task files, Git procedure, tests, and quality gates apply regardless of the model. Tool adapters only load or point to [AGENTS.md](../AGENTS.md) and [bootstrap.md](bootstrap.md); they do not fork policy, select paid APIs, configure credentials, or launch agents. The host supplies capabilities and permission enforcement. A model by itself does not have repository access.

## Entry points
| Host | Repository entry | Loading mechanism / caveat |
| --- | --- | --- |
| An agent that supports AGENTS.md | Root and scoped AGENTS.md | Follow native support, then explicitly read bootstrap.md and the selected role. |
| Claude Code | CLAUDE.md | Native @ imports load AGENTS.md and bootstrap.md; explicitly read scoped rules and task-specific files. |
| Gemini CLI | GEMINI.md | Native @ imports load the same shared files; verify the session loaded them. |
| Cursor Agent | .cursor/rules/ai-factory.mdc | Always-applied bootstrap pointer; referenced contents must be read with file tools. |
| GitHub Copilot | .github/copilot-instructions.md | Repository-instruction entry for supported surfaces; file reading and execution depend on the client. |
| Another agent or chat-only model | Explicit startup prompt or .factory/scripts/context output | No automatic file discovery assumed; use a current context snapshot and attach needed source excerpts separately. |

Adapters must be real files, such as CLAUDE.md with supported imports of shared rules, rather than symlinks. Preserve one canonical policy through imports or explicit file references. Existing linked adapters require reviewed replacement that preserves their target content and project-specific rules; do not rewrite or delete the shared target. These adapters are supplied as files, not a claim that each vendor application has been integration-tested. Do not install every vendor tool to use the factory. Model identity is optional metadata, not a capability or trust level.

## Start or continue work
Open the repository in the selected host and use:

> Read AGENTS.md and .factory/bootstrap.md. Continue the active task using its assigned role, branch, and handoff. Before edits, confirm which instructions you loaded, task/role/allowed paths, and available capabilities. If there is no active work, act as coordinator and follow the Git/task-creation rules. Keep documents in English, preserve existing work, and report unperformed checks honestly.

The prompt does not authorize delegation, external operations, or publication beyond the owner's existing instructions. Follow higher-priority host instructions where they conflict with repository conventions and record any consequential limitation.

## Switch hosts/models mid-task
1. The outgoing writer updates the shared handoff and pauses writes. Record the known host/model, task branch, revision/uncommitted work, checked commands, outstanding findings, and exact next assignment.
2. Transfer the same checkout safely or fetch the authorized task branch in another checkout. Do not leave two hosts writing the same task. Uncommitted work does not move through Git automatically; preserve it explicitly and do not silently lose it.
3. The incoming host reads shared files afresh, checks branch/diff/state, and records its actual capabilities. Treat old chat summaries and private memories as hints requiring repository evidence.
4. Continue the same task and role unless the coordinator assigns another. A different model is not automatically an independent reviewer; disclose authoring/review context overlap.
5. Keep approvals scoped to their original action and actor/environment. Record references and scope, not secrets or broad claims of permission; a new host must honor its own permission controls.

## Model strength and budgets
Use [the model-selection policy](routines/model-selection.md), [OpenAI tier configuration](openai-model-policy.json), [Anthropic tier configuration](antropic-model-policy.json). Tier recommendations are manual and host-neutral. An explicitly assigned cross-review can apply the selected provider mapping through the bounded runner; it does not route ordinary role assignments automatically. A host without exposed model details records unknown values. No per-task cost can be inferred merely from the subscription plan or model name.

## Context export for hosts without repository access
Python 3 is sufficient. From the repository root, with an existing selected task:

```sh
./.factory/scripts/context --task 004-model-portability --role developer --path .factory/scripts/context_export.py
```

Replace the example task with the task you actually need. Add --path for each assigned file/directory to include its ancestor AGENTS.md files. The default role is coordinator. The exporter writes only to stdout, performs no network requests, executes no Git/test commands, and does not update state or handoff. For a local file, redirect stdout to a private path outside the repository, inspect it, and then provide only authorized content to the receiving host.

The pack contains an explicit file manifest, shared bootstrap/workflow/routines and tracking, one role, all existing standard documents of the requested task, and scoped AGENTS.md for requested paths. Available central findings rules and active register are included, but archived findings are never included automatically. It is a read-only snapshot, not an automatic code review or complete source archive. Task archives are excluded by default, even through --path or a scope alias. To inspect exactly one archived task, add `--archived --task TASK-ID`; this exports historical evidence and does not resume the task or change state. Missing archived selections fail rather than falling back to a live task. Archive-scoped AGENTS.md files are never added through --path. The receiver must compare any live selection with state.json before resuming. It does not discover native rule globs, ancestor files outside the repository, platform policies, or private memories. Supply relevant existing native rules through the host's normal instruction-loading mechanism; the migration inventory records those gaps.

The exporter rejects invalid task/path traversal, external symlink targets, missing required inputs, and oversized context before emitting a partial pack. A hard-linked file cannot be distinguished from a normal file. Allowlisted project/task documents may still contain private data or accidentally recorded secrets: inspect them before sharing. There is no secret detector, redactor, automatic upload, provider login, or API-key configuration.

## Adopt into an existing project
Inventory existing CLAUDE.md, GEMINI.md, Cursor/Copilot rules, user/org instruction effects, and other AI artifacts using [migration.md](migration.md). Merge entry points section by section; never overwrite local instructions with these defaults. Keep one canonical policy and preserve native path scopes. If the target already has an effective AGENTS.md entry, avoid redundant full policy copies. Check actual instruction loading in each intended host; record the host version, loaded files, effective scope, and any unsupported capability in the adoption task's migration report or the active task's verification report. Keep only the current supported/unverified client summary and evidence reference in project.md. Unknown support is not verified support.

## Validate adapters and add another host
For each intended client, start a fresh session and ask it to identify the loaded shared files, selected task/role, English-documentation rule, branch requirement, and inability to claim unexecuted tests. Verify a nested rule is read for an assigned path. Perform this read-only smoke check before authorizing edits; record actual results in that task's migration or verification report rather than an assumed pass. Follow AGENTS.md's persistent-context rule; keep this guide procedural.

To add a host, identify its documented native instruction format, add a small bootstrap adapter or explicit prompt, preserve existing host instructions, and run this smoke check. Do not copy every role into provider-specific subagent formats unless the project needs that integration. The opt-in [cross-provider review runner](routines/cross-review.md) can invoke Codex or Claude CLI from either host on macOS/Linux. It is a bounded static-review tool, not a general scheduler or universal permission system.

## Vendor reference documentation
- [Claude Code project instructions and imports](https://code.claude.com/docs/en/memory): CLAUDE.md imports are supported; AGENTS.md discovery also exists in newer versions but varies by session settings. The import adapter avoids depending on that fallback.
- [Gemini CLI project context](https://geminicli.com/docs/cli/gemini-md/) and [memory imports](https://geminicli.com/docs/reference/memport/): GEMINI.md supports importing shared Markdown files.
- [Cursor project rules](https://cursor.com/docs/rules): .mdc supports alwaysApply; file mentions require tool reads rather than inlining contents.
- [GitHub Copilot custom instructions](https://docs.github.com/en/copilot/concepts/prompting-and-context/custom-instructions): repository instructions and available capabilities vary by client/surface.
