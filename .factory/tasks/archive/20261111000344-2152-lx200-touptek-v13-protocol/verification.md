# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: coordinator (implementer)
Date: 2026-11-11
Revision and working-tree state: branch `task/20261111000344-2152-lx200-touptek-v13-protocol` at master `da414d9fd`, with the uncommitted diff of the five product files
Review independence: implementer; the diff was also reviewed in a fresh context (quality-review.md)

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `lx200_touptek_goto_sync_and_refusals`: `1#` goto OK with no "Slew failed"; `0#` ALERT "rejected"; `e5#` ALERT "error e5"; `:Sd` `0#` and `e2#` send no `:MS#`; sync `1#` OK, `e9#` ALERT |
| AC-002 | pass | `lx200_touptek_rates_buzzer_and_meridian`: `Sgr0.7` sent, `Sgr0.70` count 0 |
| AC-003 | pass | same case: no `X_TOUPTEK_MERIDIAN_LIMIT`; `STa01+00`, `STa11+00`; injected `0#` to `STa10+00` gives ALERT with `RESUME_TRACKING` read back true |
| AC-004 | pass | `lx200_touptek_connect_publishes_the_device_state`: `AUTO_FLIP`, `RESUME_TRACKING` from `:GTa#`; reconnect in the rates case |
| AC-005 | pass | `lx200_touptek_site_and_time_signs`: `St+48*30:00`, `Sg+017*15:00`, `St-33*45:00`, `Sg-020*00:00`; the simulator rejects fractional seconds |
| AC-006 | pass | connect case: `MOUNT_HOME` count 2 and no `MFH`; `lx200_touptek_home` checks count 2 |
| AC-007 | pass | `lx200_touptek_home`: OK at GHS 2 with GHS polled; injected GHS `0#` shows AWAY although GU has H; abort gives AWAY; `MH` dropped gives ALERT "did not start"; GHS `0#` after busy gives ALERT "stopped before"; `lx200_home_touptek` |
| AC-008 | pass | `lx200_touptek_park_unpark_and_park_position`: `MP0` dropped gives ALERT "not confirmed", still parked; then OK with `:GPS#` read at least twice around `:MP0#` |
| AC-009 | pass | quality-review.md; PROTOCOL.md SH20 section and sources, PROPERTIES.md mount_lx200 section |
| AC-010 | pass | full `test_mount_lx200_simulator` 163/163, including `lx200_onstep_park_and_home_abort_settle`, which now also asserts AWAY |
| AC-011 | pass | `indigo_mount_lx200.c` `DRIVER_VERSION 0x03000054` (was `0x03000053`); `.driver` `version = 84` |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv all` (baseline, original driver) | repository root, Ubuntu 24.04 x86_64, gcc 13.3.0 | 0 | built |
| `./build/integration/test_mount_lx200_simulator touptek` (baseline, original driver and tests) | `indigo_test` | 0 | 13/13 SH20 cases pass |
| New tests and simulator with the HEAD `indigo_mount_lx200.c` compiled into the test (`gcc … integration/test_mount_lx200_simulator.c <HEAD .c> …`, as the `_sanitize` rule does but without sanitizers; temporary binary removed) | `indigo_test` | 1 | 6 fail at the defect checks: connect (`AUTO_FLIP`, D4), goto (`"Slew failed" == NULL` after a `1#` goto, D1), rates (`Sgr0.7` missing, D2), park (unconfirmed unpark not ALERT, D8), home (no `GHS` read, D7), site (site refused by V1.3 simulator, D5); 7 pass |
| `make -C indigo_drivers/mount_lx200 -f ../../Makefile.drv all` (fixed `.driver`, regenerated) | repository root | 0 | 2 `-Wformat-truncation` warnings in the unchanged `meade_read_meade_firmware`; the same warnings come from the HEAD `.c` |
| `./build/integration/test_mount_lx200_simulator touptek` (fixed driver, before the review fixes) | `indigo_test` | 0 | 13/13 pass |
| `./build/integration/test_mount_lx200_simulator` (final candidate, after the review fixes) | `indigo_test` | 0 | 163/163 pass, "all tests passed", 1017 s |
| Generator reproducibility (quality reviewer, scratch directory, unchanged `build/bin/indigo_generator`) | scratch | 0 | `.c`, `.h` and `_main.c` byte-identical to the working tree (before the Q2/Q3 fix; regenerated again by `Makefile.drv` after it) |

An earlier full-suite run of the pre-review candidate was stopped after 51 passing cases and no failure, so that the final candidate could be run. That run is not counted.

## Baseline comparison and regressions
Baseline: 13/13 SH20 cases. Final: 13/13 SH20 cases (one renamed, `lx200_touptek_home_and_home_search` → `lx200_touptek_home`) and 163/163 for the whole suite. The number of registered cases is unchanged at 163. `MIGRATION_STATUS.md` records `166 / 35` for mount_lx200; this task does not change that number, and the existing difference from 163 is not caused by this task.

## Findings
Found defects (source audit against V1.3; D1, D2, D4, D5, D7 and D8 reproduced with the old driver):

| Defect | Impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| D1 | an accepted goto is reported "Slew failed"; a rejected one is reported started | success read as `0` | `1` for ToupTek, message on `0` | `lx200_touptek_goto_sync_and_refusals` |
| D2 | the guide rate is not set | `%.2f` | `%.1f` | `lx200_touptek_rates_buzzer_and_meridian` |
| D3 | non-zero meridian offsets sent | limit property | property removed, `+00` always | rates and connect cases |
| D4 | wrong meaning of `b` | ZWO semantics copied | items `AUTO_FLIP`, `RESUME_TRACKING` | connect and rates cases |
| D5 | site possibly rejected | `.000` suffix | whole seconds | `lx200_touptek_site_and_time_signs` |
| D6 | undocumented `:MFH#` sent | SEARCH item | item and request path removed | connect and home cases |
| D7 | home completion from an undocumented flag | GU `H` | `:GHS#` | `lx200_touptek_home` |
| D8 | unpark from a wrong state, never confirmed | blind `:MP0#` | GPS check and confirmation | `lx200_touptek_park_unpark_and_park_position` |
| D9 | misleading comments and docs | pre-V1.3 sources | rewritten | review |

There are no central FND records: every defect is fixed in this task.

## Manual checks and limitations
- No hardware. The V1.3 behaviour is verified against the simulator modelled on the document only. The `:MS#` polarity on real firmware 0.5.2 is not confirmed.
- Linux x86_64 only.
- The GPS 0/1 unpark branches and the GHS 3/4/5 codes are source-audited only (quality-review.md Q4).

## Verification verdict rationale
Every acceptance criterion passes on the final candidate, and the reproducers show that the new tests detect the old behaviour.

## Final test summary
- Simulated tests: 163 run, 163 passed (final candidate, full `test_mount_lx200_simulator`).
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No FND IDs.
