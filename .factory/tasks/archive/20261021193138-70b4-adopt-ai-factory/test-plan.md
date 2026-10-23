# Adoption verification plan

| Scenario | Acceptance | Check |
| --- | --- | --- |
| Immutable provenance / same source | AC-001, AC-005 | full SHA, detached HEAD, clean status, map validation and same-pin plan |
| Task lifecycle and active-task protection | AC-004 | new_task and archive_task disposable fixture regressions; selected active pointer |
| Legacy evidence preservation | AC-003 | 43 unique aliases matched against base review tables; findings validator; source-history references |
| Shared/scoped instructions | AC-002 | compare retained instruction clauses; context export for actual driver/test/source/doc paths |
| Helper failure boundaries | AC-004, AC-009 | findings/context/process/template-update/review negative fixture tests, no real providers |
| Registry integrity/privacy | AC-006, AC-009 | origin/canonical root/ignored file/schema/uniqueness/concurrent-change preflight, atomic replacement, fleet |
| Project navigation | AC-007 | Xcode plist parse, new-file and folder-group path traversal reconciliation |
| Final scoped changes | AC-008, AC-009 | diff check and separate quality/security review passes |
| Native behavior preserved | AC-009 | no application source/build/SDK changes; native unit baseline recorded unavailable |

No hardware or driver integration run was requested or performed. No package installation or networked test is needed for workflow-only integration. The native application suite is an explicitly separate command requiring built libraries; its unavailability must never be reported as test success.

Factory regression tests are copied from the same pin into indigo_test/factory/, with required INDIGO test license/AI headers. One template-export test uses the upstream .gitattributes bytes as a synthetic disposable fixture; the template-only export policy is never installed at project root. The template-specific layout/default-state test is excluded because its assertions apply to an empty template rather than an adopted project. No test is weakened to hide application failures.

## Archive-time project exclusions

The installed archive_task.py is deliberately adapted to skip the five owner-protected driver subtrees, indigo_tests/, obsoleted_drivers/ and externals/bin_externals before reading Markdown. This preserves project owner restrictions during archive-time link repair. One additional disposable-fixture regression uses unreadable-as-UTF-8 Markdown in each excluded tree and proves those paths are neither read nor rewritten. The upstream policy and this local customization remain mapped for future three-way review.
