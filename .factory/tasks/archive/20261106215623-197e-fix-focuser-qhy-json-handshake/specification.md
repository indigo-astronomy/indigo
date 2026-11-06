# Specification

Status: accepted

## Problem and outcome
`qhy_parse_json()` in `indigo_focuser_qhy.driver` rejects any backslash inside a JSON string (key or value), and
`qhy_parse_response()` requires the `version` and `bv` fields of the `idx:1` reply to be quoted strings. Real
firmware 20231207 sends an `id` string containing `\u0019` and `\u0006` escapes and sends `version` and `bv` as
numbers, so the handshake fails and the connection is refused. The hand-written driver before the generator
migration copied backslashes literally and took `version`/`bv` whether quoted or not, so this is a regression.

Outcome: the driver connects to such firmware, publishes the reported version and board version in `INFO`, and
keeps rejecting malformed replies.

## In scope
- Accept JSON escape sequences inside string keys and values: a backslash followed by any character is part of the
  string and does not terminate it. The text is kept as received; escapes are not decoded (the driver uses only
  `version` and `bv`, which carry no escapes).
- Accept `version` and `bv` whether quoted or unquoted; publish their text as received.
- Ignore unknown fields such as `id` (already the case).
- The simulator answers the handshake with the reply observed on hardware by default and can be switched to the
  quoted-string form.
- Driver version 15 -> 16; integration tests; `MIGRATION_STATUS.md` test count.

## Out of scope
Decoding `\uXXXX` into characters, other commands, the generator, hardware testing.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With the handshake reply `{"idx":1,"id":"@\u0019SC8HP\u0006TH2A","version":20231207,"bv":108}` the driver connects (CONNECTION OK) and the connect sequence is unchanged | integration test `capabilities` against the simulator default reply |
| AC-002 | `INFO` `DEVICE_FW_REVISION` is `20231207` and `DEVICE_HW_REVISION` is `108` for that reply | integration test `capabilities` |
| AC-003 | With quoted `"version":"1.2.3","bv":"108"` the driver connects and `INFO` shows `1.2.3` and `108` | new integration test `quoted_version_handshake` |
| AC-004 | A handshake reply whose string is not terminated because the closing quote is escaped (`{"idx":1,"id":"x\"}`) is rejected: CONNECTION goes to ALERT, no descriptor leaks, a later connect succeeds | new integration test `init_version_escaped_quote` |
| AC-005 | The simulator sends the hardware-observed handshake reply byte for byte | integration test `simulator_protocol` |
| AC-006 | Every other case of the existing suite passes unchanged | full suite run through `tools/run_driver_test.py focuser_qhy` |
| AC-007 | The driver version is 16 (`3.0.0.16`) | `.driver` source and the recorded test line |

## Failure behavior and edge cases
- A backslash as the last character of the buffer (no following character) makes the reply malformed; the parser
  returns false and the command fails as before.
- A backslash before the closing quote escapes it; the string continues, so a reply like `"x\"}` is malformed.
- Unquoted `version`/`bv` values are taken up to the next comma, brace or white space; an empty value is rejected.
- Field value length stays limited to 63 characters; a longer `id` makes the reply malformed (unchanged limit).

## Permissions, data, and security requirements
Input comes from the serial device. The parser works on a bounded NUL-terminated buffer and copies into fixed
fields with length checks; the change must keep every bounds check.

## Compatibility and nonfunctional requirements
No property change. Linux, macOS and Windows builds use the same portable C. The quoted-string handshake form keeps
working.

## Assumptions and open questions
Assumption: the reply quoted in the user's report is exact (captured with pyserial as raw bytes). Assumption: firmware
sends no escape inside `version`/`bv`; if it did, the raw escape text would be published, which is acceptable.

## Owner decisions and requirement changes
2026-11-06: the owner asked to fix the driver after the assessment of the user's report (chat).
