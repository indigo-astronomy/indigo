# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261024194838-6cb6-primaluce-panel-cover-wifi` at master `1a0252d5e`, uncommitted task changes.
Review independence: not independent. Mitigation: the focuser regression cases were run against 3.0.0.41 and failed; hardware sessions on both devices.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V3, V4, V7, V8 |
| AC-002 | pass | V1, V3, V4, V5, V7, V8 |
| AC-003 | pass | V1, V3, V4, V5, V7, V8 |
| AC-004 | pass | V1, V3, V4 |
| AC-005 | pass | V3, V4, V5 |

## Executed checks
Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), repository root unless noted, `libindigo` from `1a0252d5e`.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `./build/unit/test_primalucelib`, and the same built with `-fsanitize=address,undefined` | indigo_test | 0 | 33/33 both, no sanitizer report |
| V2 focuser `controller_settings`, `wifi_station_mode` against `indigo_focuser_primaluce.c` of `HEAD` (3.0.0.41) | indigo_test | 1 | both FAIL: no reason message for a 3 character password; no `LANCFG` `ap` for OFF from STA |
| V3 `python3 tools/run_driver_test.py aux_giotto` | root | 0 | 29/29, recorded `2026-10-24 23:12 3.0.0.4 linux x64 simulator 29/29 OK` |
| V4 `python3 tools/run_driver_test.py aux_alto` | root | 0 | 32/32, recorded `2026-10-24 23:14 3.0.0.4 linux x64 simulator 32/32 OK` |
| V5 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 72/72, recorded `2026-10-24 23:17 3.0.0.42 linux x64 simulator 72/72 OK` |
| V6 raw probe of the ALTO1 (`/dev/ttyUSB0`, scripted requests) | session scratchpad | 0 | credentials and `AP_SET_STATUS` acknowledged and read back; firmware stored a 3 character password and `LANCFG` `xyz` (restored before any restart); `sta`/`ap` with `REBOOT` answered after about 2 s; access point `on` about 6 s after `REBOOT`; all values restored |
| V7 ALTO1 through `indigo_aux_alto` 3.0.0.4 (`indigo_server -p 7655`, `indigo_prop_tool`) | root | 0 | connect values matched; short password ALERT with reason, device kept its password; station credentials read back on reconnect; OFF/AP 0.12 s, STA and back 4.9/4.6 s; restored |
| V8 GIOTTO through `indigo_aux_giotto` 3.0.0.4 (same setup) | root | 0 | connect values matched; short password ALERT with reason; station credentials read back; OFF/AP 0.1 s, STA and back 4.4/4.2 s; reconnect AP; light stayed off, brightness kept, `CALRESTART` 0; restored |
| V9 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
GIOTTO 26/26 to 29/29, ALTO 29/29 to 32/32 (three new cases each), focuser 72/72 (two cases extended), unit 30/30 to
33/33. No regression.

## Findings
None registered; the focuser's OFF from station mode and the unchecked credentials were found and fixed here (focuser
REFACTOR.md).

## Manual checks and limitations
Not verified: the HTTP path of a mode change (no network access to the devices), whether a changed access point password
applies before the next restart, the focuser on hardware for this change, Windows and macOS builds. After a restart the
access point reports `off` for about 4 s, so a reconnect within about 6 s of a mode change shows OFF.

## Verification verdict rationale
All criteria pass in the simulator and on both devices.

## Register reconciliation
No finding.
