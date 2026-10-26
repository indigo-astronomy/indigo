# Finding reconciliation

All ten owner-selected findings are verified fixed; shell compliance and unrelated functional findings remain out of scope. Evidence and platform limits: [verification](verification.md).

## AC-001 — FND-000028

Existing current Makefile registration verified with the final serial/UDP 64/64 run; historical missing registration is no longer present.

## AC-002 — FND-000029

Controlled cooler polling and SDK guide/abort gates remove state sampling/queue races; focused 40/40 and full 52/52 pass.

## AC-003 — FND-20261023065605-60d4-repository-audit-003

Owner retired Travis; configuration, badge, Xcode reference and maintained documentation references removed. Historical evidence and generic manual build helpers retained.

## AC-004 — FND-20261023101753-bc8d-complete-repository-audit-030

Per-device OK requires successful process exits; complete/partial/untagged/normal subprocess result regressions pass.

## AC-005 — FND-20261024121920-3bd2-repository-audit-040

Dry-run help explicitly describes builds/tests/temporary-file/hardware side effects and previews only final record writes.

## AC-006 — FND-20261024121920-3bd2-repository-audit-043

Scoped role paths corrected to indigo_test/AGENTS.md and indigo_docs/AGENTS.md; archive prose distinguishes retained v2 completion state from legacy reset.

## AC-007 — FND-20261025211159-e308-fix-audit-findings-batch-001

Optec initializes the process-owned USB monitor before exact repeated lifecycle descriptor checks; focused and full suites pass.

## AC-008 — FND-20261025211159-e308-fix-audit-findings-batch-002

Mount abort test waits fresh completed publications and actual motion; exact stopped-position and recovery assertions retained, focused/full suites pass.

## AC-009 — FND-20261025211159-e308-fix-audit-findings-batch-003

RA movement and stability are signed angular differences; deterministic midnight-crossing setup and actual reverse-motion checks pass all dialects/full suite.

## AC-010 — FND-20261025211159-e308-fix-audit-findings-batch-004

Temma GOTO/park and NexStar park deadline branches have real stop/ALERT/recovery regressions; default-limit negative controls establish expiry sensitivity.

## In-task quality regression — FND-20261026181024-758b-fix-test-findings-001

Independent review found the new Temma recovery revision snapshot occurred after its request. Capturing it before the request removes the extra-publication race. Corrected deadline 2/2 and final full Temma suite pass; see [quality review](quality-review.md). This intermediate issue is closed as fixed, not risk acceptance.
