# Shell compliance regression design and implementation

## Scope and independence

Test-designer/developer subagent, separate from shared-framework and entrypoint authors. Assigned source: `indigo_test/unit/test_shell_compliance.py` and the existing GPS fixture in `test_gps_compliance_script.py`; no product/framework/entrypoint edits, Git mutation, hardware or external service access. Tests derive outcomes from AC-001 through AC-005 and the actual property-tool output contract in `indigo_tools/indigo_prop_tool.c`, especially list/state output at lines 310–425 and scalar get output at lines 448–626. Host: macOS arm64; Bash subprocesses use the available system Bash.

The new source contains **33 unittest methods**, with parameterized cases. It runs the actual Bash framework and all six entry points through an isolated executable Python property-tool fixture. Every fixture has private state, trace and HOME paths; no live server is contacted. The normal full-entrypoint fixture provides mandatory class properties and ranges, completes synthetic guide pulses by resetting their values to zero, and requires every entrypoint to reach the restoration footer with exit zero and zero reported failures. Optional unsupported properties may be skipped; this remains synthetic class coverage, not hardware certification. The existing nine-method GPS suite retains its separate all-success and optional-property acceptance coverage.

## Coverage

- AC-001: executable paths with spaces, one remote argv value containing spaces, clearing a prior remote address, scalar/status/set failures with valid-looking stdout or stderr, failed reads that must not turn into numeric zero, and failed property enumeration that must not become an optional skip.
- AC-002: direct session normal and failed exits for original ON/OFF; INT/TERM preservation of exit 130/143 for both states; cleanup failure makes a successful body fail. Actual six-entrypoint normal exits cover ON/OFF; TERM during the connection battery covers ON/OFF for every entry point. Early range failures and required range/text/boolean read failures must restore the connection and issue no motion/configuration writes after the bad read.
- AC-003: observed BUSY is aborted immediately, completed motion produces an honest skip without an abort, missing BUSY after a successful command and nonzero tool exits fail, and all recorded synthetic helper PIDs have exited/reaped before return. The timestamp comparison detects reintroducing the fixed two-second delay.
- AC-004: exact item names reject suffix collisions; exact properties reject unrelated prefixes; metacharacters are literal; verbose range parsing ignores label and item decoys, rejects malformed suffixes and failed commands. Both valid property and item list selectors receive consistent response fixtures. Numeric getters accept signed/exponent finite values and reject empty, malformed, NaN/inf and overflow values. Capture destinations/getters reject unsupported forms and failed reads preserve a prior destination value. All three numeric comparator paths must reject a benign `1+1` reply rather than evaluate it as the expected value `2`, while accepting genuine signed and scientific-notation numeric literals.
- AC-005: emitted summary includes passed, failed and skipped counts; missing optional properties remain separately exercised by the GPS suite.

## Baseline evidence

Saved the original framework before its writer started at `/tmp/indigo-shell-framework-baseline.sh`. Original entrypoints were reconstructed read-only from base `66d6815dd` into `/tmp/indigo-shell-entrypoints-baseline/`.

At the 15-framework-method checkpoint, ran from the repository root:

`INDIGO_SHELL_FRAMEWORK_SOURCE=/tmp/indigo-shell-framework-baseline.sh python3 -B indigo_test/unit/test_shell_compliance.py FrameworkContractTests`

Result: exit 1; 15 methods, 15 recorded failure outcomes including parameterized subcases, 8.864 seconds. Retained output: [framework baseline](shell-framework-baseline.txt). These are expected pre-fix failures. Later helper/numeric coverage increased the current framework method count; the retained checkpoint must not be represented as today's full source count.

Original entrypoint comparison:

`INDIGO_SHELL_FRAMEWORK_SOURCE=/tmp/indigo-shell-framework-baseline.sh INDIGO_SHELL_ENTRYPOINT_SOURCE=/tmp/indigo-shell-entrypoints-baseline python3 -B indigo_test/unit/test_shell_compliance.py EntryPointRestorationTests.test_six_entrypoints_restore_after_normal_control_flow`

Result: exit 1; one parameterized method, five failing original-ON cases across the five non-GPS classes, 26.144 seconds. At that checkpoint the original OFF cases and GPS restoration behave correctly; this baseline used only connection-state assertions, before independent review strengthened current normal-flow assertions to require successful completion. Retained output: [entrypoint baseline](shell-entrypoints-baseline.txt).

## Development verification and correction

The first complete current-source run had 25 methods and only two failing range subcases. Those were fixture-control errors: the test keyed a forced response to `list P.ITEM`, while the implementation validly requests `list -e P` and selects the exact item afterward. Corrected the fixture to supply identical controlled list output for either selector, preserving every malformed/decoy assertion. No production code was changed to satisfy that fixture correction.

Subsequent actual runs from the repository root:

| Command / source checkpoint | Result |
| --- | --- |
| `python3 -B indigo_test/unit/test_shell_compliance.py FrameworkContractTests ConnectionSessionTests AbortMotionTests` | 22/22 pass, 31.745 s |
| `python3 -B indigo_test/unit/test_shell_compliance.py` at the 28-method checkpoint | 28/28 pass, 43.065 s |
| `python3 -B indigo_test/unit/test_shell_compliance.py FrameworkContractTests ConnectionSessionTests AbortMotionTests EntryPointRestorationTests.test_required_range_text_and_boolean_read_errors_stop_and_restore` after numeric/capture additions | 28/28 pass, 13.006 s |
| `python3 -B indigo_test/unit/test_shell_compliance.py EntryPointRestorationTests.test_required_range_text_and_boolean_read_errors_stop_and_restore EntryPointRestorationTests.test_malformed_original_position_stops_before_motion_and_restores` with final extra AO/guider/wheel failures | 2/2 methods pass, nine parameterized cases, 9.984 s |
| `python3 -B indigo_test/unit/test_gps_compliance_script.py` after fixture-format correction | 9/9 pass, 49.523 s |
| `git diff --check -- indigo_test/unit/test_shell_compliance.py indigo_test/unit/test_gps_compliance_script.py` | pass |

GPS fixture changes correct its legacy invented `Device.PROPERTY [STATE]` output to real CLI `[STATE]\tDevice.PROPERTY.ITEM = value` and spaced list records; scalar get remains scalar and get_state returns a state. Existing acceptance assertions were retained. No expectation was weakened to make the repaired framework pass.

## Independent review follow-up

Independent QA found that the earlier minimal entrypoint fixture permitted AO/guider to exit early after missing required range reads, so connection restoration alone did not establish successful normal completion (task quality finding 001). Expanded mandatory AO/GUIDER guide properties and all four axis ranges, reset guide pulse values to zero after synthetic BUSY completion, and supplied the missing mandatory focuser, rotator and GPS properties. Normal coverage now requires the restoration footer, summary, exit zero, zero failed count and no FAIL output for every script and initial connection state. This stronger one-method/twelve-case test passed in 24.969 seconds. Failure/signal tests remain separate.

The same review found that the synthetic `never_busy` mode returned a command error, which masked the missing-BUSY guard. It now emits only OK and exits zero; the explicit `failed` mode separately exits seven. The focused abort missing-BUSY/error method passes with both independent controls.

Security review found three numeric comparison helpers evaluated tool replies as arithmetic expressions (task security finding 002). Before the root fix, the new benign `1+1` reply / expected `2` test against `/tmp/indigo-shell-numeric-before.sh` failed all three subcases because all three helpers falsely printed PASS and returned zero: one method, three failures, 0.726 seconds. Snapshot copied from the working framework immediately before the coordinator's getter correction. No command execution payload is used.

## Handoff

After the coordinator froze the typed numeric getter and data-only comparison correction, ran `python3 -B indigo_test/unit/test_shell_compliance.py` from `/Users/polakovic/Development/indigo`: **33/33 methods passed in 57.000 seconds**, exit zero. Output: `/tmp/indigo-shell-regressions-33.txt`. This includes every strengthened twelve-case normal-flow assertion, the successful-command/no-BUSY negative control, three benign expression-reply rejection subcases, and genuine finite signed/scientific literal success controls for all three comparison helpers. AST parsing and scoped `git diff --check` also passed.

All assigned writes are released. Historical checkpoints above remain identified separately; the final full 33-method pass is the current source result. The coordinator owns the final combined Makefile target/GPS rerun and final-diff review. No physical hardware, Linux or Windows run or source commit was performed by this agent.
