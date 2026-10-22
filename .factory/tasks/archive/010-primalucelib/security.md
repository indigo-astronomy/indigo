# Security review

Verdict: pass
Reviewer: security-reviewer role (same agent as the developer)
Date: 2026-10-22
Revision and working-tree state: base `80d073bcb`, final uncommitted task diff
Review independence: not independent; same agent implemented the change.

## Scope, assets, and trust boundaries
- **Assets:** the INDIGO process running the driver (memory safety), the controller's WiFi
  credentials held in properties, and the serial port claim.
- **Trust boundaries:**
  - Controller replies over USB serial or HTTP are untrusted input to the parser.
  - Property values from INDIGO clients are inserted into requests.
  - The HTTP host comes from the client-set `DEVICE_PORT`.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No new entry point; property access rules unchanged. The controller's HTTP interface has no authentication, as before. |
| Input, injection, and output handling | Yes | The reply parser moved verbatim: bounded 8192-byte buffer, `PRIMALUCE_MAX_RESPONSE_SIZE - 1` read limit, 1024 tokens, jsmn strict mode, the `res` path walk stops at `JSMN_UNDEFINED`. The HTTP reply is bounded and copied only when shorter than the buffer. Requests: URL encoding is unchanged; stored-position names with `"` or `\` are refused before any request (unit test). WiFi SSID/password are inserted unescaped, as before (FND-000052, low): a malformed request reaches only the user's own controller, which refuses it. |
| Data, secrets, cryptography, and logs | Yes | No new logging of credentials. Requests are logged at debug level by `indigo_uni_io`, as before. The unit test uses the simulator's synthetic credentials. |
| Filesystem and outbound network access | Yes | Outbound HTTP only to the host the client configured, unchanged. The test harness writes traces only when `INDIGO_SIMULATOR_TRACE_DIR` is set, under that directory, with bounded path formatting. |
| Dependencies and configuration | Yes | jsmn (MIT) relocated unchanged and compiled into the static library with `JSMN_STATIC`, so it is not exported. The library exports only prefixed `primalucelib_`/`PRIMALUCE_` symbols; drivers are loaded without `RTLD_GLOBAL`, so a second PrimaLuce driver cannot interpose another copy. No new dependency. |
| Abuse and failure behavior | Yes | `TIOCNXCL` is issued only on descriptors this process opened and claimed. It does not let another process take a port this process still holds: the flag is cleared just before the process's own close. Silent or garbage replies end with `false` within the 5 s port timeout, as before (unit tests). |

## Automated checks
`./.factory/scripts/security` (Factory validate_project.py --security), exit 0: "Factory local
security boundaries valid. Configured security commands passed." This covers only Factory
boundaries; there is no application scanner configured. The library compiles without warnings
under `-Wall -Wextra` (verification.md C4).

Data model: `MIGRATION_STATUS.md` changes only the `Automated Tests (Sim / HW)` count of the focuser_primaluce row (53 to 55, the actual case count); no schema, column or Comment change.

## Findings
No security finding. FND-000052 (unescaped WiFi credentials) is registered as functional/low: the
only party affected is the user's own controller receiving a malformed request.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. The parsing and transport code is moved without change, and wire traffic is byte-identical
(verification.md AC-007). Limits:
- No fuzzing of the reply parser.
- The HTTP path was not executed in this task.
- Same-agent review.

## Register reconciliation
No security-category findings. FND-000052 is active (functional, low, nonblocking).
FND-000049/50/51 are archived as fixed (verification.md).
