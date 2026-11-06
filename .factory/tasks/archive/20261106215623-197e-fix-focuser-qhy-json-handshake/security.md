# Security review

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-fable-5-1), separate pass over the final diff
Date: 2026-11-06
Revision and working-tree state: master 86d3e884e plus the uncommitted task diff on task/20261106215623-197e-fix-focuser-qhy-json-handshake
Review independence: same agent as the implementation; the parser change was also inspected by the independent quality reviewer (quality-review.md) and exercised under AddressSanitizer (verification.md)

## Scope, assets, and trust boundaries
The only untrusted input is the serial reply of the focuser, read by `qhy_command()` into the 192-byte NUL-terminated
response buffer in private data (bounded by `indigo_uni_read_section2()`). `qhy_parse_json()` walks that buffer and
copies keys (max 31 bytes) and values (max 63 bytes) into fixed fields with length checks. The change alters only how
a backslash inside a quoted string advances the cursor and whether `version`/`bv` may be unquoted.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | serial device protocol, no credentials |
| Input, injection, and output handling | Yes | escape handling: `if (*cursor == '\\' && !*++cursor) return false;` then `cursor++` never moves past the NUL terminator; a trailing backslash makes the reply malformed. The key/value length checks (`key_length >= sizeof(key)`, `value_length >= sizeof(value)`) are unchanged, so the raw escape text is bounded. `qhy_json_text()` copies with `snprintf()` after a `strlen() >= size` check; the published `INFO` text is the raw field text (no decoding, no format string use). |
| Data, secrets, cryptography, and logs | No | nothing new is logged or stored |
| Filesystem and outbound network access | No | the simulator writes only its event/fault/control files in the test fixture directory, unchanged |
| Dependencies and configuration | No | no new dependency; one new simulator option and fault action |
| Abuse and failure behavior | Yes | malformed replies (escaped closing quote, partial, overlong, trailing) still fail the command; the `init_version_escaped_quote` case checks the connect is refused without a descriptor leak |

## Automated checks
ASan build and run of the suite: see verification.md (`test_focuser_qhy_simulator_asan`).

## Findings
None.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Limitation: no hardware run; the reply format is modelled from the user's raw capture.

## Register reconciliation
No canonical FND record created or changed.
