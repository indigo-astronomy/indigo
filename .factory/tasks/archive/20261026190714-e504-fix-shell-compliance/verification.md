# Verification

Verdict: pass

## Candidate and limits

Working directory `/Users/polakovic/Development/indigo`; branch `task/20261026190714-e504-fix-shell-compliance`; base/local master `66d6815ddc2882895ebc351807977ad008b8274c`. Runtime: macOS arm64, system Bash 3.2.57. Root implemented documentation/integration and the numeric-review correction; separate implementation, test-design, quality and security contexts supplied independent work and source inspection. No physical hardware/live server or production driver changes. Linux/Windows runtime is unverified. ShellCheck is unavailable; no ShellCheck pass is claimed.

## Final commands and results

| Command from repository root | Outcome |
| --- | --- |
| `python3 -B indigo_test/unit/test_shell_compliance.py` after review corrections | 33/33 methods, 57.000s, exit 0; parameterized cases cover successful footers for all six entrypoints × ON/OFF, failures and signals |
| `make -C indigo_test test-gps-compliance-script` after numeric correction | 9/9, 34.789s, exit 0; existing acceptance assertions retained |
| `./.factory/scripts/test` | 190/190, 134.416s, exit 0 |
| `/bin/bash -n <script>` for all seven modified shell sources | 7/7 pass |
| `git diff --check`; `plutil -lint indigo.xcodeproj/project.pbxproj` | pass |
| `./.factory/scripts/check`; `./.factory/scripts/security` | pass during verification; repeat after retention and on the committed candidate as delivery gates |

The new Makefile target `test-shell-compliance-framework` invokes the same Python suite and joins the regular automated target. All I/O is synthetic; live compliance remains opt-in. No native test binary was built, so the full native/driver suite was not rerun for these Bash/fixture/registration changes.

## Acceptance

- AC-001: quoted executable/remote argv and status capture prevent filtering from masking nonzero exits. Failed required reads stop further mutation, restore connection and print the summary. Numeric replies are finite data, never calculator-language input. Wheel names no longer use eval and valid empty text remains supported.
- AC-002: actual six entrypoints cover original ON/OFF successful normal completion, failures and TERM; helper tests also preserve INT/TERM 130/143 and prior failures. Unknown initial state stops before mutation. Failed cleanup is visible and makes successful execution fail. Only connection state is globally restored by traps.
- AC-003: bounded synchronous BUSY observation and immediate abort replace fixed sleeps/background waiters. Helper PIDs are gone before return. Fast completion is SKIP, not false abort coverage; missing BUSY with exit 0, malformed states and command errors are FAIL.
- AC-004: literal full property/item matching and anchored finite ranges reject substring/regex/diagnostic decoys. Typed getters and constant AWK programs reject expression replies while accepting signed/scientific literals. CLI formats come from indigo_tools/indigo_prop_tool.c; GPS fixture corrections preserve acceptance assertions.
- AC-005: docs/examples and executable summary include passed/failed/skipped totals, skipped-coverage limits, quoted invocation, required reads, restoration/abort semantics and battery flag 1 for original ON / 0 for original OFF with query error 3.

## Baselines and review corrections

Original GPS fixture suite passed 9/9 in 38.298s. Independent original-framework checkpoint produced 15 expected failure outcomes; old entrypoints left five non-GPS original-ON devices disconnected. Read-only reconstruction used base 66d6815dd. Exact commands/output qualifications: [regression report](shell-regressions-report.md), [framework baseline](shell-framework-baseline.txt), [entrypoint baseline](shell-entrypoints-baseline.txt).

Initial 32/GPS 9 successes were not substituted for the stronger final gate: independent Q1 found minimal fixtures could exit early, so normal fixtures now provide mandatory capabilities, complete guide pulses and require footer/exit 0/zero failed counts in all 12 normal cases. The missing-BUSY fixture now returns 0 with OK, independent of exit 7 command failure. Corrected final 33 methods pass.

Independent security review showed raw reply 1+1 falsely accepted as numeric 2 in all three old comparator paths. The corrected typed finite getters and constant AWK/data comparison reject those benign expression replies and accept signed/scientific numeric literals. Q1 and S1 are separately registered and fixed; no risk acceptance. Intermediate fixture-selector and GPS restoration-header/duplicate-skip failures were corrected without weaker assertions. Bash 3.2 rejects printf-v array destinations, so captures use scalars followed by ordinary indexed assignments.

Details: [framework](framework-report.md), [entrypoints](entrypoints-report.md), [regressions](shell-regressions-report.md). Final quality/security conclusions belong to their reports. Git content identities are bound after archival; final immutable process/check/test/security outcomes and SHA are recorded externally. Any failing committed gate blocks delivery. The due repository audit remains due under the owner repair-only deferral.
