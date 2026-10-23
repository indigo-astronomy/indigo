# Adoption verification

Verdict: pass

Reviewed snapshot: synchronized adoption working tree on task/001-adopt-ai-factory, based on `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. The explicit final origin refs/heads/master fetch resolved to that same revision; no merge was needed or performed. The final committed candidate and post-commit process-check outcome are reported in the delivery message, avoiding a self-referential commit hash.

Verifier: same Codex agent that implemented the change, in a separate explicitly loaded verifier pass. No independent verification or native client startup is claimed.

## Acceptance evidence
| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | template-source.json; clean detached source SHA `11392a11253bae4d1c5d5db97e8a532917728c09` verified before use and at final reconciliation |
| AC-002 | pass | Factory root text is the prefix; scoped driver bytes equal original; existing test guide is the full suffix; context export includes real source/test/doc/driver scopes |
| AC-003 | pass | all 43 unresolved base-table aliases exactly equal migrated aliases; next_id 44; 15 exact REVIEW.md files absent; findings validator passes |
| AC-004 | pass | new-task selected the actual task; current project context/check arrays use real paths; 144 helper regression cases pass |
| AC-005 | pass | init recorded same source/full pin; final lock equals retained 83-entry map; same-pin plan is wholly unchanged |
| AC-006 | pass | alias indigo registered in ignored persistent inventory; atomic validated update preserved the other entry; fleet reports current |
| AC-007 | pass | Xcode plist lint and graph traversal: every new file and ancestor directory group is registered; no REVIEW.md/AGENTS.override.md references remain |
| AC-008 | pass with explicit owner audit exception | Owner approved postponing the full audit until before the first subsequent development task or release. Partial audit recorded separately; canonical full-audit checkpoint remains null. Native application test baseline and other native host sessions are unavailable, not passes |
| AC-009 | pass | tracked diff limited to workflow/instructions/review retirement/Xcode; existing README files, application/generator/vendor/protected drivers and remotes/history unchanged |

## Baseline commands
Working directory: /Users/polakovic/Development/indigo; starting clean revision `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. Environment: macOS arm64, Python 3.9.6, Git 2.54.0 (Apple Git-157).

| Exact command | Exit | Result |
| --- | --- | --- |
| `git status --short` | 0 | empty starting state |
| `git diff --check` | 0 | clean baseline |
| `plutil -lint indigo.xcodeproj/project.pbxproj` | 0 | OK |
| `make -C indigo_test test-unit` | 2 | prerequisite failure: Missing INDIGO library in build/lib; zero native cases executed |

Native build/test unavailability does not certify product behavior. This adoption makes no application-facing source/build/CI changes and uses applicable static/helper regressions instead of forcing an unrelated full build with protected drivers. No driver/hardware test mode was requested or run, and no driver Testing record or TEST_SUMMARY change was made.

## Post-integration commands
| Exact command | Exit | Result |
| --- | --- | --- |
| `./.factory/scripts/check` | 0 | diff whitespace, 43-finding integrity, ADR index and local project boundaries pass |
| `./.factory/scripts/test` | 0 | executes `python3 -B -m unittest discover -s indigo_test/factory -p 'test_*.py'`; 144/144 pass in 102.875 seconds |
| `./.factory/scripts/security` | 0 | ignored/private metadata, mapped regular-file boundaries, external review disabled |
| `./.factory/scripts/findings check` | 0 | 43 active, 0 archived; task references valid |
| `python3 .factory/scripts/decision_index.py --check` | 0 | current |
| `plutil -lint indigo.xcodeproj/project.pbxproj` | 0 | valid project syntax |
| `python3 /private/tmp/indigo-reconcile-adoption.py` | 0 | base aliases, full scoped instructions, pin/map, exclusion and all new Xcode paths/groups reconciled; temporary inspection script, no product writes |
| `./.factory/scripts/context --task 20261021193138-70b4-adopt-ai-factory --role coordinator --path indigo_drivers --path indigo_test/factory --path indigo_libs --path indigo_docs` | 0 | shared bootstrap, coordinator and all four real scoped instruction files included; private inventory omitted |
| `./.factory/scripts/template-update plan --project /Users/polakovic/Development/indigo --source /private/tmp/indigo-ai-factory-pin-20261021 --revision 11392a11253bae4d1c5d5db97e8a532917728c09` | 0 | 83 unchanged mappings; no unmapped pin changes |
| `./.factory/scripts/template-update fleet --source /private/tmp/indigo-ai-factory-pin-20261021 --revision 11392a11253bae4d1c5d5db97e8a532917728c09 --inventory /Users/polakovic/Development/ai_factory/.factory/adopters.local.json` | 0 | indigo current; separate indigoapps6 current; no other project modified |

One preliminary direct helper-suite run overlapped bootstrap and returned exit 1: 143 cases, one error because project.md had not yet been created. After writing current project context, the complete configured test group was rerun and passed. No test assertion was removed or relaxed for that error.

## Host loading evidence
Current Codex chat: root owner instructions plus explicitly read pinned coordinator/analyst/planner/documenter/verifier/security/quality/repository-reviewer roles, bootstrap/workflow, driver/test/editor scoped guides. Context-export smoke verifies shared/scoped path resolution, not automatic fresh-session loading. No fresh Claude, Gemini, Cursor or Copilot session was invoked, installed or charged; their native adapter loading remains unverified. The current host does not expose a reliable exact model/effort/version identity here. No provider switches or subagents.

## Final candidate gate
After archival, repeat static/security/findings/Xcode/scope checks, capture staged reviewed content, commit the single outcome, and run `python3 .factory/scripts/process_check.py --base eba315b7041b1a19f64a1ad7864dc1a6c0019ea8 --head HEAD`. This is structural evidence validation, not independent approval. Source or target changes require affected evidence refresh. The configured full helper regression suite already passed; only report/archival/navigation metadata changes follow it.

## Archive exclusion retest

After adding the project-specific no-read/no-rewrite exclusion to archive_task.py and its disposable-fixture regression, reran the entire configured test group: `./.factory/scripts/test`, exit 0, 144 cases in 102.875 seconds, all passed. This supersedes the earlier 143-case pass. The full set includes permission/lifecycle/provenance/context/findings/process/review cases; no real provider or hardware was invoked.

## Retention validation

Before and after `./.factory/scripts/archive-task 20261021193138-70b4-adopt-ai-factory`, findings check passed with 43 active and 0 archived findings. Dry-run and actual task archive both exited 0, updating four Markdown files. The adoption task group is reparented under the Xcode archive group, retaining all file-reference identities. Archival preserves finding logical references; it does not prune their reports or reset the counter. Owner exception/current next action remain in maintenance/state. No existing application README or protected/vendor file was read or rewritten by archive link maintenance.
