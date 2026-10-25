# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025093558-ece5-server-leak-check` at master `4ce776bb3`, uncommitted task changes.
Review independence: not independent; separate passes.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V1, V2, V3 |
| AC-002 | pass | V2, V3 |
| AC-003 | pass | findings 001 to 003 |

## Executed checks
Linux x86_64 (Ubuntu), Valgrind 3.22.0, Python 3, server and drivers built from `4ce776bb3`.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 the exercise operations one by one against `indigo_server --do-not-fork` without Valgrind (`indigo_prop_tool get_state` after each) | root | 0 | all OK (mount and dome after unpark) |
| V2 `python3 indigo_test/leak/run_server_leak_check.py --cycles 1` | root | 1 | definitely lost 22464 bytes, indirectly 0, possibly 13468 |
| V3 `python3 indigo_test/leak/run_server_leak_check.py --cycles 3` (final, with mount and dome exercised) | root | 1 | the same totals: no leak grows with connects or operations |
| V4 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Result (V3)

| Kind | Bytes | Blocks | Allocated at | Cause |
| --- | --- | --- | --- | --- |
| definitely lost | 18280 | 5 | `indigo_ccd_attach` (`indigo_ccd_driver.c:242`), one per simulator CCD device | `CCD_FPS_PROPERTY` never released in `indigo_ccd_detach()` (FND-20261025093558-ece5-server-leak-check-001) |
| definitely lost | 3656 | 1 | `agent_device_attach` (`indigo_agent_guider.c:2782`) | `AGENT_GUIDER_RESET_MKGP_PROPERTY` never released (002) |
| definitely lost | 528 | 1 | `indigo_agent_astrometry` INIT (`indigo_agent_astrometry.c:1340`) | device private data not freed at shutdown (003) |
| possibly lost | 5998 + 5998 | 38 + 38 | `libusb_init_context` from `indigo_start_usb_event_handler` and `hid_init` | libusb contexts kept for the process lifetime; not freed at exit (third-party, one-time) |
| possibly lost | 1472 | 4 | `pthread_create` thread-local storage | threads still alive at exit; not a leak |

All definite leaks happen once per device or driver lifetime (attach/detach, load/unload). None grows with connects,
exposures, guiding, slews, dome or rotator moves; 1 and 3 cycles give the same totals.

## Baseline comparison and regressions
No previous leak check exists.

## Findings
FND-20261025093558-ece5-server-leak-check-001, -002, -003 (open, low).

## Manual checks and limitations
Only the simulator drivers and the agents the server loads by default; hardware drivers, the BLOB download path to a
client (not requested by `indigo_prop_tool`) and macOS were not covered. Valgrind reports at shutdown, so memory that is
held until exit but should be freed earlier is not seen as lost.

## Verification verdict rationale
The check runs, reports reproducibly and names every leak; the leaks are confirmed in the source.

## Register reconciliation
Three findings registered.
