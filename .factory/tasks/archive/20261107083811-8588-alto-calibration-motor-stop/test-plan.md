# Test and regression plan

Status: executed (simulator, unit, ALTO1 hardware)

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-005 | Unit, `indigo_test/unit/test_primalucelib.c` `cover_commands` | Scripted peer replies | Request 5 is `{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Motor_Stop"}}}}`, acknowledged `"done"` | With step 2 |
| T-002 | AC-001, AC-006 | Integration, `test_aux_alto_simulator.c` `calibration` | ALTO simulator, normal profile | STOP sends `CAL_ALTO Motor_Stop`; the polled position stays between the ends over two polls | Before and after the driver change |
| T-003 | AC-002 | Same case | Same | Each STORE: `Motor_Stop` immediately followed by `StoreAsClosedPos` / `StoreAsMaxOpenPos`; 3 `Motor_Stop` in total; no `MOT_STOP` during the calibration; travel applied | Before and after |
| T-004 | AC-004 | Same case | Same | CANCEL sends exactly one more `MOT_STOP` | Before and after |
| T-005 | AC-003 | Integration, `calibration_stop_refused` | One-shot `reject` fault on `"Motor_Stop"` | STOP and STORE end ALERT with "Error: invalid command"; no `StoreAs...` sent; the next STORE completes the first phase | Before and after |
| T-006 | AC-007 | Regeneration | `build/bin/indigo_generator` | Only the intended lines change; `.h` and `_main.c` unchanged | After step 5 |
| T-008 | AC-009 | Integration `calibration`; `calibration_travel_not_reported` (profile stale-travel); unit `cover_commands` | Simulator stale `CAL_MAXPOS` for 1 s / forever | BUSY then "travel N steps" with N ≈ 98000; ALERT after the polls when never reported | Before (version 10, fails) and after |
| T-009 | AC-010 | Integration `calibration` | Simulator Init moves the count to the middle | IDLE position, value unchanged during the calibration; OK after CANCEL | Before (version 10, fails) and after |
| T-010 | AC-011 | Hardware `alto_survives_the_usb_unplug`, `run_driver_test.py aux_alto --hot-plug` | ALTO1, owner pulls and replugs the cable | ALERT, clean disconnect, reconnect and moves | After |
| T-011 | AC-012 | Hardware suite `run_driver_test.py aux_alto --hw`; guided calibration through `indigo_server` + `indigo_prop_tool` | ALTO1 | 9/9; calibration completes with `Motor_Stop` | Version 10 and 11 (suite), version 10 (calibration) |
| T-007 | AC-008, preserved behaviour | Full ALTO simulator suite through `tools/run_driver_test.py aux_alto`; full primalucelib unit suite | — | All pass | Final |

## Baseline evidence
See verification.md "Executed checks": unit 38/38 and ALTO simulator 34/34 on master 86d3e884e.

## Bug reproduction
T-002 and T-005 fail against the original driver (version 9) with the new simulator: the journal holds no
`CAL_ALTO Motor_Stop`, and STOP ends OK instead of ALERT. Evidence in verification.md.

## Negative cases and permission boundaries
Refused `Motor_Stop` (T-005); STORE and calibration motion outside a calibration are refused before anything is sent
(existing assertions in `calibration`).

## Isolation and cleanup
Every case runs its own simulator process on a pseudo-terminal, with its own journal and fault file; `make -C indigo_test
test-clean` removes the build afterwards.

## Deferred tests and coverage limitations
- Hardware: the calibration ran on version 10 only; version 11's polling of the travel is verified on the simulator.
  Whether firmware 1.03.11 accepts a store after a plain `MOT_STOP` stays unknown (not needed any more).
- The simulator still stops the arm by itself on `StoreAs...`, as before: the document requires a stopped arm, but the
  reply of the real device to a store while the arm moves is unknown, so no refusal is invented.
