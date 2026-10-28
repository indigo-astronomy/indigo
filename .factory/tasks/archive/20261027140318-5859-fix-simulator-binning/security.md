# Security review

Verdict: pass
Reviewer: Claude Fable 5.1 acting as security reviewer (same agent as the developer, limitation recorded)
Date: 2026-10-27
Revision and working-tree state: task branch `task/20261027140318-5859-fix-simulator-binning` on `32d7396d7` (master `42c0960a2` plus the owner's server commit), uncommitted working tree
Review independence: not independent

## Scope, assets, and trust boundaries
The task changes pixel arithmetic in the exposure thread of the camera simulator (two loops of `create_frame()`),
adds two integration cases that read RAW frames from the bus and one opt-in solver case that sets CCD_BIN through the
Imager Agent. No new properties, files, environment variables, network access or privileges.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | no bus property or permission change |
| Input, injection, and output handling | Yes | the bin block indexes stay inside the fixed 1600 x 1200 image: `frame_left + i` and `frame_top + j` are binned coordinates bounded by `CCD_FRAME / bin` as before, the block offsets are below the bin, and the bin handler accepts 1, 2 and 4 only; the sum is an `unsigned` of at most 16 x 65535 and its mean fits the `uint16_t` store; the guider loop bounds are unchanged |
| Data, secrets, cryptography, and logs | No | no data at rest, no secrets, the tests print pixel statistics only |
| Filesystem and outbound network access | No change | the tests write to their private home; the solver case uses the already cached indexes |
| Dependencies and configuration | No | no new dependency; the generator is unchanged and the outputs reproduce |
| Abuse and failure behavior | Yes | the inner loop adds at most 16 reads per binned pixel, bounded by the frame size; no overflow or wrap |

## Automated checks
`./.factory/scripts/security` (validate_project.py --security): see verification.md for the exit code.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Same-agent review; no sanitizer run of the changed driver.

## Risk policy classification
`MIGRATION_STATUS.md` changes two case counts of the status table; `TEST_SUMMARY.md` and the README test record are
written by `tools/run_driver_test.py`. Neither is a data-model or stored-data migration; no process policy file
changes.

## Register reconciliation
Two functional findings created from this task (FND-20261027140318-5859-fix-simulator-binning-001 hot pixel row
bound, -002 solver margin), neither a security finding; nothing else to reconcile.
