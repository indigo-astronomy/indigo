# Security review

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-fable-5-1), separate pass over the final diff
Date: 2026-11-07
Revision and working-tree state: master 0d1e4672e plus the uncommitted task diff on task/20261107080809-a02b-fix-ccd-svb-gain-after-open
Review independence: same agent as the implementation; the diff was also inspected by the independent quality reviewer (quality-review.md)

## Scope, assets, and trust boundaries
The driver change adds one SDK write of a constant exposure (1 s) after a successful control enumeration at
connect, under the master-device lock the connect already holds, and log lines in `svb_set_number()` that print
the property name, device id, control type, requested value, the two SDK result codes and the value read back. No
new input, file, network path, permission or credential is involved; `svb_set_number()` is reached only through
the framework-validated `CCD_GAIN`, `CCD_GAMMA` and `CCD_OFFSET` change handlers. The test changes add fixture
state to the fake SDK and one in-process case; the hardware suite step writes and restores the gain through the
public bus API.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | USB camera SDK, no credentials |
| Input, injection, and output handling | Yes | the log lines use fixed format strings with numeric arguments and the driver-owned property name; `value` is initialised before the readback so a failed read never logs an indeterminate value |
| Data, secrets, cryptography, and logs | Yes | the new log lines contain SDK result codes and control values only; nothing sensitive |
| Filesystem and outbound network access | No | unchanged |
| Dependencies and configuration | No | no new dependency; the bundled SDK is unchanged (v1.13.4) |
| Abuse and failure behavior | Yes | a failed exposure write at connect is logged and the connect proceeds; a subsequently refused gain is published in ALERT with its restored value, as before, now with a log line |

## Automated checks
The fake-SDK suite and the SV305PRO hardware suite (verification.md); no sanitizer run was needed for a
constant write and log lines.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Limitation: no SV705C/SV905C2 run; the hardware run covers an SV305PRO.

## Diff risk signals
The process check flags `MIGRATION_STATUS.md` under the `data-model` rule because its name contains "migration".
The file is the driver migration status table; this task changes only the automated-test count of the `ccd_svb`
row (48 / 1 -> 49 / 1). There is no data model, schema or stored-data change.

## Register reconciliation
No canonical FND record created or changed.
