# Verification report

Verdict: pass
Reviewer: single agent, verifier role (same agent implemented the fix; independence limited, disclosed)
Date: 21 Oct 2026 (Europe/Bratislava)
Revision and working-tree state: branch task/20261021203703-ee3d-fix-websocket-key-overflow on f1a28a893 with the uncommitted task diff: indigo_libs/indigo_server_tcp.c, indigo_test/integration/test_server_websocket.c, indigo_test/Makefile, indigo.xcodeproj/project.pbxproj, findings register and archive, task 003 corrections and this task's records.
Review independence: no separate implementer or reviewer under the no-delegation constraint.

## Acceptance evidence
| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 over-long key refused, no upgrade, connection closed, server keeps serving | pass | key_one_byte_too_long_is_refused (220 bytes) and over_long_key_is_refused_and_server_keeps_serving (900 bytes, then an RFC key upgrades on a new connection) pass in both builds. |
| AC-002 RFC 6455 sample key upgrades with s3pPLMBiTxaQ9kYGzzhZRbK+xOo= | pass | rfc6455_sample_key_upgrades_with_derived_accept passes before and after the fix. Value cross-checked with Python hashlib. |
| AC-003 219-byte key still upgrades with the accept derived from the whole key | pass | longest_fitting_key_still_upgrades asserts the exact accept ofAac6OErSs0N0DdzMrdBM3zQ2A=, computed independently. |
| AC-004 real server source clean under ASan; base source overflows | pass | test-server-websocket-sanitize passes 4/4 with no AddressSanitizer report. The same final test built with the base indigo_server_tcp.c aborts in both over-long cases with stack-buffer-overflow, WRITE of size 37, at indigo_server_tcp.c:152 on 'websocket_key'. |

## Executed checks
All commands run from the repository root on macOS 27 (arm64), Apple clang.

| Command | Exit | Result |
| --- | --- | --- |
| Pre-fix: test_server_websocket, each case alone | 0, 0, 133, 133 | RFC and 219-byte cases pass; the 220-byte and 900-byte cases kill the whole process with SIGTRAP from the fortified strcat, so even the release build crashes. |
| Pre-fix: test_server_websocket_sanitize, each case alone | 0, 0, 134, 134 | ASan stack-buffer-overflow in start_worker_thread, indigo_server_tcp.c:152. |
| `make -C indigo_libs` | 0 | libindigo rebuilt with the fix. |
| `make -C indigo_test test-server-websocket` | 0 | 4/4 pass. |
| `make -C indigo_test test-server-websocket-sanitize` | 0 | 4/4 pass, no AddressSanitizer report. UBSan prints one pre-existing SHA-1 shift report, registered as FND-000046. |
| Final test built against the base server source under ASan, each case alone | 0, 0, 134, 134 | Confirms the final test still detects the defect. |
| Reproducer 003/repro-websocket-key.c under ASan, control and 255-byte input | 0, 134 | Old copy/append operations overflow. |
| Reproducer repro-websocket-key-fixed.c under ASan, same inputs | 0, 0 | Fixed bound refuses the 255-byte key. |
| `make -C indigo_drivers/<d> -f ../../Makefile.drv all` for mount_simulator, mount_lx200, agent_mount | 0 | Archives needed by detach-abort; no tracked file changed. |
| `make -C indigo_test test-detach-abort` | 0 | 28/28 pass; exercises the production server over TCP. |
| `make -C indigo_test test-detach-abort-network` | 0 | 6/6 pass. |
| `make -C indigo_test test-unit` with UNIT_TESTS excluding test_system_alpaca_json | 2 | 13 suites pass; indigo_uni_io fails serial_open_reports_not_a_serial_port (FND-000047, pre-existing, unrelated) and the loop stops. |
| `indigo_test/build/unit/test_raw_image` run alone | 0 | All pass; this suite is skipped by the loop above. |
| `./.factory/scripts/findings check` | 0 | 45 active, 2 archived. |

Raw logs stay in the session scratch directory and are not committed.

## Baseline comparison and regressions
No regression: the RFC and boundary cases passed before and after the fix, and detach-abort and the unit suites match the audit baseline. The only unit failure is pre-existing and platform-specific.

## Findings
- FND-000045 fixed and archived with this report as evidence.
- FND-000046 registered: the bundled SHA-1 shifts int values into the sign bit. UBSan reports it before and after this change; no functional failure is observed. Not fixed here, it is outside the requested scope.
- FND-000047 registered: the uni_io test expects ENOTTY from /dev/null, which macOS 27 does not return. The audit had recorded the unit subset as passing; task 003 verification.md, repository-review.md and process.json were corrected.

## Manual checks and limitations
Not covered: Linux and Windows builds of the fix, a full `make all`, the alpaca unit suite (archive absent), and hardware. The fix uses only standard C string functions already in the file, so no platform-specific behavior is introduced.

## Verification verdict rationale
All four acceptance criteria pass, the regression test discriminates the old and new code, and the related server suites pass. Next assignment is in handoff.md.

## Register reconciliation
FND-000045: archive/FND-000045.json, resolution fixed. FND-000046 and FND-000047: active, open, nonblocking. next_id 48.
