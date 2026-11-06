# Security review

Verdict: pass
Reviewer: implementer, plus the independent diff review in quality-review.md
Date: 2026-11-06
Revision and working-tree state: base c180d7b32 plus the uncommitted task diff
Review independence: the security pass was done by the implementer; the independent reviewer covered buffer bounds and reply parsing

## Scope, assets, and trust boundaries
The trust boundary is the serial or TCP data from the mount, which is untrusted. The asset is the driver process (`indigo_server`) and the
mount's motion safety.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | The transport model is unchanged |
| Input, injection, and output handling | Yes | Replies are read with bounded `indigo_uni_read_section2` into the 128-byte buffer or a 16-byte local buffer. The copies into `MOUNT_INFO` items are bounded by the smaller source. Digit replies are range-checked before use as an index (fixes N1 of the INDI reference). Error codes go through `atoi` into a message only |
| Data, secrets, cryptography, and logs | No | None handled |
| Filesystem and outbound network access | Yes | The only new transport detail is the default TCP port 8080 for `lx200://` URLs the user configures. No new listener |
| Dependencies and configuration | No | No new dependencies |
| Abuse and failure behavior | Yes | Refusals end ALERT. A park or home that never starts times out to ALERT. A bootloader refuses the connection. Firmware-update commands are never sent |

## Automated checks
The simulator suite, including fault-injection cases, and the sanitizer build are listed in verification.md.

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. There is no hardware validation (D6). Wi-Fi and BLE transports were not exercised against a real hand controller.

## Register reconciliation
No canonical FND records were created or changed.
