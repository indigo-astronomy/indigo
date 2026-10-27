# Security review

Verdict: pass
Reviewer: Claude Fable 5.1 (security-reviewer role, same agent as the developer)
Date: 2026-10-27
Revision and working-tree state: task branch `task/20261027192244-16bd-solver-hints-input-diagnostics` from master `ac3c53e2c`, uncommitted working tree reviewed before the single task commit; the reviewed content identities are in process.json
Review independence: none; the developer, verifier and reviewers are one agent in one session. Recorded as a limitation.

## Scope, assets, and trust boundaries
The Solver Agent receives image blobs from related agents or client uploads (untrusted data) and hint values through
framework-validated number items. Assets: the process memory of `indigo_server`, the index cache files (read only
in this task), the correctness of the reported WCS used by mount workflows. No network, file or process access is
added: the task adds a catalogue filter over already loaded data, loop limits, hint arithmetic, a Bayer flag, a
luminance weighting and a message.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | The agent exposes no new property; access control is the bus's |
| Input, injection, and output handling | Yes | `raw_bayer_keyword()` scans the RAW appendix within `size - data_size` bytes without writing (the library's `indigo_is_bayered_image()` writes a NUL into the buffer, which is why it is not used); FITS parsing keeps its header/data bounds; `indigo_solver_filter_quads()` sizes its array by a counting pass; stage star counts never exceed the detected count (`depth_stages()`); hint RA/DEC are used in trigonometry only; the solved message is formatted from integers and a float with a fixed format string |
| Data, secrets, cryptography, and logs | No new | Debug lines carry star counts, cone sizes and index paths already logged before |
| Filesystem and outbound network access | No change | Index loading and download unchanged; the cache reuse only skips a file read |
| Dependencies and configuration | No change | libjpeg and libraw were already linked by the previous task |
| Abuse and failure behavior | Yes | A hostile or huge frame is now bounded by CPU_LIMIT (default 180 s from the shared property) and the abort flag inside the quadratic voting loop, which previously had no bound (`MAX_QUAD_MATCH_COUNT` 1 000 000 allows 5·10¹¹ pair iterations); a hint outside the catalogue yields no quads and a clean failure; the parity check only rejects solutions |

## Automated checks
`./.factory/scripts/security` (`validate_project.py --security`): see verification.md for the exact result. No
dependency or secret scanners are configured for this project beyond that check.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass for the reviewed diff. The review is a same-agent inspection of the changed code paths; it does not certify the
solver against adversarial image inputs beyond the bounds checks noted, and the time limit is cooperative (checked
between iterations), not pre-emptive.

## Register reconciliation
No canonical FND record is created; the active register (`./.factory/scripts/findings list`) holds no Solver Agent
finding.
