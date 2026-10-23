# AI Factory adoption report

- Target repository: /Users/polakovic/Development/indigo; clean starting revision `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`
- Template repository: https://github.com/polakovic/ai_factory
- TEMPLATE_COMMIT: `11392a11253bae4d1c5d5db97e8a532917728c09`
- Separate clean detached checkout: /private/tmp/indigo-ai-factory-pin-20261021
- Lock: .factory/template-lock.json; final reviewed mapping: template-map.json (83 files)
- Adoption task: 20261021193138-70b4-adopt-ai-factory
- Coordinator/date: current Codex agent, 2026-10-21, Europe/Bratislava
- Status: verified adoption with explicit owner-authorized initial full-audit deferral

## Read-only preflight and coverage gaps

See [discovery.md](discovery.md): clean target/registry, existing branches/remotes/history preserved, no submodules, no pre-existing local changes. Vendor/build/ignored private configuration, protected drivers and ordinary indigo_tests content are excluded. No secret values or inventory contents are copied. Partial code-audit coverage is explicit in repository-review.md.

## Existing AI artifacts

| Path | Consumer and scope | Preserved requirements | Conflict | Disposition |
| --- | --- | --- | --- | --- |
| AGENTS.md | Codex/shared, entire project | driver/property/generator/formatting/testing/hygiene/change-log requirements | old REVIEW index/workflow | Factory first; retain project clauses; replace retired review mechanism |
| CLAUDE.md | Claude root adapter | root rules + actual test scope | duplicate root policy pointer | native imports plus scoped driver/test references |
| indigo_drivers/AGENTS.override.md | previous Codex scoped driver entry | full driver refactor/lifecycle/test rules | requested filename change | identical content in indigo_drivers/AGENTS.md |
| indigo_test/AGENTS.md | scoped automated tests | complete existing license, fixture, simulator, test-record and cleanup requirements | none | prepend pinned generic contract, retain original content |
| indigo_tools/driver_editor/AGENTS.md | scoped editor | all rules | none | retain unchanged |
| indigo_libs/externals/libusb/AGENTS.md | vendor scope | vendor instructions | outside adoption | retain untouched |
| GEMINI.md / Cursor / Copilot adapters | client-specific startup | canonical shared rules | absent | add pinned regular files; native loading unverified |
| exact REVIEW.md files | legacy incremental review | unresolved aliases and historical checkpoints | owner retires mechanism | migrate 43 unresolved entries, then delete 15 files; no application fixes |

## Repository map

See the component table in [.factory/project.md](../../../project.md) and evidence/inventory in [discovery.md](discovery.md). Sources are indigo_libs/, indigo_drivers/, platform/optional driver roots, indigo_server/, indigo_tools/, indigo_examples/, tools/ and platform/infrastructure roots. Actual automated tests are indigo_test/; Factory regressions are under indigo_test/factory/. No application src/ or tests/ directories are added.

## Commands and environment

| Command / root working directory | Evidence and environment | Side effects / authority |
| --- | --- | --- |
| `git diff --check` | existing safe baseline; Git 2.54.0 | read-only |
| `plutil -lint indigo.xcodeproj/project.pbxproj` | macOS local Xcode plist parser | read-only |
| `make -C indigo_test test-unit` | native documented unit target; Python 3.9.6/macOS arm64 host | cannot start: library absent; no test cases executed |
| `./.factory/scripts/check` | reviewed command arrays: diff, findings, ADR index, project boundaries | read-only local checks |
| `./.factory/scripts/test` | Python unittest discovery in indigo_test/factory | disposable synthetic repos, local fake CLI executables; no real provider or network calls |
| `./.factory/scripts/security` | project metadata ignore/regular-file/provider-disable checks | read-only local boundary validation; not a product security scan |
| `template-update init/plan/fleet` | reviewed installed helper; exact source/full pin | init writes lock only; plan/fleet read-only |

No environment secret values or new dependency manager is required. Native build/test commands remain documented separately; application/hardware testing is not claimed.

## Integration and collision plan

The full initial pre-bootstrap plan is retained in integration-plan.json. Additional scoped-rule, regression-test and local-state paths were reviewed before their writes. Final transferable mapping is template-map.json (83 entries); local configuration/state/registries/reports are excluded. Every final path disposition follows:

| Target / source | Existed | Action | Preserved requirement / decision | Validation | Rollback |
| --- | --- | --- | --- | --- | --- |
| `.cursor/rules/ai-factory.mdc` ← `.cursor/rules/ai-factory.mdc` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/antropic-model-policy.json` ← `.factory/antropic-model-policy.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/bootstrap.md` ← `.factory/bootstrap.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/migration.md` ← `.factory/migration.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/openai-model-policy.json` ← `.factory/openai-model-policy.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/portability.md` ← `.factory/portability.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/analyst.md` ← `.factory/roles/analyst.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/coordinator.md` ← `.factory/roles/coordinator.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/developer.md` ← `.factory/roles/developer.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/documenter.md` ← `.factory/roles/documenter.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/planner.md` ← `.factory/roles/planner.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/quality-reviewer.md` ← `.factory/roles/quality-reviewer.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/repository-reviewer.md` ← `.factory/roles/repository-reviewer.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/security-reviewer.md` ← `.factory/roles/security-reviewer.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/test-designer.md` ← `.factory/roles/test-designer.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/roles/verifier.md` ← `.factory/roles/verifier.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/cross-review.md` ← `.factory/routines/cross-review.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/findings.md` ← `.factory/routines/findings.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/git.md` ← `.factory/routines/git.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/model-selection.md` ← `.factory/routines/model-selection.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/process-checks.md` ← `.factory/routines/process-checks.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/quality-review.md` ← `.factory/routines/quality-review.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/repository-review.md` ← `.factory/routines/repository-review.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/task-archive.md` ← `.factory/routines/task-archive.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/template-maintenance.md` ← `.factory/routines/template-maintenance.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/routines/template-updates.md` ← `.factory/routines/template-updates.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/README.md` ← `.factory/scripts/README.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/archive-task` ← `.factory/scripts/archive-task` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/archive_task.py` ← `.factory/scripts/archive_task.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/check` ← `.factory/scripts/check` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/context` ← `.factory/scripts/context` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/context_export.py` ← `.factory/scripts/context_export.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/decision_index.py` ← `.factory/scripts/decision_index.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/findings` ← `.factory/scripts/findings` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/findings.py` ← `.factory/scripts/findings.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/git_snapshot.py` ← `.factory/scripts/git_snapshot.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/new-task` ← `.factory/scripts/new-task` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/new_task.py` ← `.factory/scripts/new_task.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/process-check` ← `.factory/scripts/process-check` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/process_check.py` ← `.factory/scripts/process_check.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/review` ← `.factory/scripts/review` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/review.py` ← `.factory/scripts/review.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/review_adapters.py` ← `.factory/scripts/review_adapters.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/review_protocol.py` ← `.factory/scripts/review_protocol.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/risk_check.py` ← `.factory/scripts/risk_check.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/run_checks.py` ← `.factory/scripts/run_checks.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/security` ← `.factory/scripts/security` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/template-update` ← `.factory/scripts/template-update` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/template_update.py` ← `.factory/scripts/template_update.py` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/scripts/test` ← `.factory/scripts/test` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/adopters.json` ← `.factory/templates/adopters.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/checks.json` ← `.factory/templates/checks.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/finding.json` ← `.factory/templates/finding.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/migration-report.md` ← `.factory/templates/migration-report.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/repository-review.md` ← `.factory/templates/repository-review.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/review-policy.json` ← `.factory/templates/review-policy.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/brief.md` ← `.factory/templates/task/brief.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/handoff.md` ← `.factory/templates/task/handoff.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/plan.md` ← `.factory/templates/task/plan.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/process.json` ← `.factory/templates/task/process.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/quality-review.md` ← `.factory/templates/task/quality-review.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/security.md` ← `.factory/templates/task/security.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/specification.md` ← `.factory/templates/task/specification.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/test-plan.md` ← `.factory/templates/task/test-plan.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/task/verification.md` ← `.factory/templates/task/verification.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/templates/template-map.json` ← `.factory/templates/template-map.json` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/workflow.md` ← `.factory/workflow.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.github/copilot-instructions.md` ← `.github/copilot-instructions.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `AGENTS.md` ← `AGENTS.md` | yes | merge | Factory first, then all nonconflicting INDIGO rules; replace only legacy review mechanism; driver instruction link follows rename | content/helper/context checks | restore base blob |
| `CLAUDE.md` ← `CLAUDE.md` | yes | merge | Native imports plus retained explicit driver/test scope pointers | content/helper/context checks | restore base blob |
| `GEMINI.md` ← `GEMINI.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_libs/AGENTS.md` ← `src/AGENTS.md` | no | add | Relocated to the actual INDIGO component; preserved scoped requirements | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/AGENTS.md` ← `tests/AGENTS.md` | yes | merge | Relocated to the actual INDIGO component; preserved scoped requirements | content/helper/context checks | restore base blob |
| `indigo_docs/AGENTS.md` ← `docs/AGENTS.md` | no | add | Relocated to the actual INDIGO component; preserved scoped requirements | content/helper/context checks | remove only adoption-added path after later-edit check |
| `docs/decisions/template.md` ← `docs/decisions/template.md` | no | add | Pinned support; preserve executable mode | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_context_export.py` ← `tests/unit/test_context_export.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_decision_index.py` ← `tests/unit/test_decision_index.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_findings.py` ← `tests/unit/test_findings.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_new_task.py` ← `tests/unit/test_new_task.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_process_check.py` ← `tests/unit/test_process_check.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_review.py` ← `tests/unit/test_review.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_task_archive.py` ← `tests/unit/test_task_archive.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `indigo_test/factory/test_template_update.py` ← `tests/unit/test_template_update.py` | no | add | Pinned helper regressions, required INDIGO test headers; template export policy remains a synthetic fixture | content/helper/context checks | remove only adoption-added path after later-edit check |
| `.factory/project.md` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/checks.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/state.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/maintenance.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/findings/active.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/findings/archive/.gitkeep` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/review-policy.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/scripts/validate_project.py` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.factory/template-lock.json` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `docs/decisions/README.md` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `docs/decisions/ADR-001-adopt-factory.md` | no | add project-owned data | Fresh local data; not an upstream-update destination | local checks / review | remove only adoption-added path after later-edit check |
| `.gitignore` | yes | merge | Exact private Factory ignores / new groups and paths; preserve existing content | ignore checks / plist + graph reconciliation | restore only adoption hunks |
| `indigo.xcodeproj/project.pbxproj` | yes | merge | Exact private Factory ignores / new groups and paths; preserve existing content | ignore checks / plist + graph reconciliation | restore only adoption hunks |
| `indigo_drivers/AGENTS.override.md` → `indigo_drivers/AGENTS.md` | yes | relocate | Content unchanged; correct scoped host entry | byte equality / context export / Xcode | restore original path and Xcode reference |
| `.factory/tasks/archive/20261021193138-70b4-adopt-ai-factory/*` | no | add task evidence | new-task templates populated with this adoption evidence; no imported history | task/manifest/report checks | remove only adoption-created evidence after checking later work |
| `REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_docs/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_drivers/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_examples/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_libs/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_linux_drivers/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_mac_drivers/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_optional_drivers/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_server/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_test/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_tests/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_tools/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `indigo_windows/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `systemd/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |
| `tools/REVIEW.md` | yes | remove after findings migration | Owner authorized removal; unresolved aliases in legacy-findings.md; full original history retained at base | 43-alias reconciliation | restore exact base blob |

## Decisions and unresolved questions

Owner authorized Factory-first root rules, review retirement, scoped override rename, local registration and Xcode navigation updates. Owner explicitly selected migration/deletion of only indigo_tests/REVIEW.md; the rest of that subtree is excluded. Existing application README files, protected drivers, generator, SDKs, application source and CI remain unchanged.

Task branches use the Factory helper's required task/<id> convention. The current host's higher-priority instructions govern tool use; template rules do not override them. No provider change or independent/subagent review is claimed. Existing security issues remain open, with no implied risk acceptance.

The owner explicitly approved postponing the initial full code audit until before the first subsequent development task or release; the sampled adoption review does not reset maintenance checkpoints. Native non-Codex client sessions cannot be verified in this chat; supported adapter structure and explicit reads/context export are separate evidence.

## Bootstrap and task registration

Created task/001-adopt-ai-factory from `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8` after an authorized origin/master fetch confirmed the same SHA. Installed only reviewed support, then ran the resolved new-task wrapper; state.json selects this adoption task. No existing Factory state/lock/task was present.

Initialized the lock with the reviewed installed helper, --project `/Users/polakovic/Development/indigo`, --source `/private/tmp/indigo-ai-factory-pin-20261021`, --revision `11392a11253bae4d1c5d5db97e8a532917728c09`, --map `/private/tmp/indigo-template-map.json`, --task `001-adopt-ai-factory`. The initial 75-entry map was extended, on the same adoption branch and same baseline SHA, for eight reviewed relocated helper regressions. No lock baseline was replaced or relabelled; final map is retained with the task.

Persistent registration: alias `indigo`, canonical root `/Users/polakovic/Development/indigo`. Registry checkout origin matches the approved repository; its main branch, tracked files and local configuration are preserved. Existing version-1 inventory was ignored before writing and had one valid entry. Registration used duplicate-key/schema/alias/canonical-path checks, an exclusive advisory file lock, a same-inode/mtime/size/content check immediately before atomic replace, fsync and preserved mode. Existing entries are preserved. No local-exclude edit was necessary. No inventory content or other project paths are retained here.

Fleet at the pin: `indigo`: current; separate existing project `indigoapps6`: current. No other project was modified. Final fleet validation is recorded in verification.md.

## Baseline and post-integration evidence

See [verification.md](verification.md) for exact commands, exit codes, counts, retries and limitations. Baseline native unit execution is unavailable due to missing INDIGO build/lib; the Factory-only tests exercise installed support without claiming application coverage.

## Review and exit gates

- Instruction preservation: Factory-first merge; root legacy-review clauses replaced under explicit owner instruction; original scoped driver/test/editor requirements retained.
- Source/test map: actual roots recorded; indigo_tests excluded apart from approved review retirement.
- Helpers and task lifecycle: installed source-pin support; disposable fixture tests and scope export checks recorded in verification.md.
- Secret and execution boundaries: separate security.md, ignored inventory, no real provider invocation.
- Scoped diff and rollback: Xcode navigation and workflow/test support only; base provides recoverable original files.
- Review independence: one agent in explicit sequential roles, as required by the no-subagent/no-provider-switch scope.
- Initial full audit: owner-authorized deferral, checkpoint before next development task or release; no full-audit completion is claimed.

## Final outcome and next task

Implementation and verified local registration are complete; final acceptance/check results and the owner-authorized initial-audit exception are in verification.md and handoff.md. The next assignment is the initial full maintained-repository audit before unrelated development/release. No merge, push, publication, deployment or future upgrade is performed.

## Archive-time project exclusions

The installed archive_task.py is deliberately adapted to skip the five owner-protected driver subtrees, indigo_tests/, obsoleted_drivers/ and externals/bin_externals before reading Markdown. This preserves project owner restrictions during archive-time link repair. One additional disposable-fixture regression uses unreadable-as-UTF-8 Markdown in each excluded tree and proves those paths are neither read nor rewritten. The upstream policy and this local customization remain mapped for future three-way review.
