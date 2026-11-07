# Security review

Verdict: pass
Reviewer: Claude Code (claude-opus-5-5); the diff was also reviewed in a separate agent context (quality-review.md)
Date: 2026-11-07
Revision and working-tree state: base 86d3e884e plus the uncommitted task diff
Review independence: implementer's own security pass

## Scope, assets, and trust boundaries
The change covers the ALTO calibration: the stop request, a polled travel read after the store, hiding the position
during a calibration, a simulator model, tests and an opt-in hardware hot-plug case. The trust boundary is the
serial/HTTP link to the cover. New requests are compile-time constants; the only new input is the numeric `CAL_MAXPOS`
reply, bounded by the existing JSON parser and checked against 0, the previous value and the provisional range.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No access control involved |
| Input, injection, and output handling | Yes | The step string is the constant `"Motor_Stop"` formatted into the existing `CAL_ALTO` template; no client input reaches it. Device replies use the existing acknowledgement parsing |
| Data, secrets, cryptography, and logs | No | No data or credentials touched |
| Filesystem and outbound network access | No | Unchanged transport; the downloaded PrimaLuce packages stayed in the session scratchpad and are not part of the repository |
| Dependencies and configuration | No | No new dependency |
| Abuse and failure behavior | Yes | A refused stop ends ALERT and stores nothing (`calibration_stop_refused`). CANCEL and disconnect keep the hardware-verified `MOT_STOP`, so the arm can always be stopped; a disconnect cancels the pending travel finalizer. The travel wait is bounded (20 polls); motion steps are refused during it. The hardware test touches only the configured port (`access()`) |

## Diff risk signals
The process check flags `MIGRATION_STATUS.md` under the `data-model` rule, because its name contains "migration". The
file is the driver migration status table. This task changes only the automated-test count of the `aux_alto` row
(34 / 9 → 36 / 10). There is no data model, schema or stored-data change, and no data-model risk.

## Automated checks
The simulator suite, including the fault-injection case, and the unit suite are listed in verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
No security impact. Hardware behaviour of `Motor_Stop` is a functional limitation recorded in verification.md, not a
security risk.

## Register reconciliation
No finding opened or closed.
