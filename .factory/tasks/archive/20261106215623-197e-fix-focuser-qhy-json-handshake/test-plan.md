# Test and regression plan

Status: accepted

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 hardware handshake reply is accepted | AC-001, AC-002 | integration, `test_focuser_qhy_simulator.c` `capabilities` | simulator default reply | CONNECTION OK, INFO `20231207` / `108`, connect sequence `1, 5, 13, 7, 4` | before the fix (fails), after (passes) |
| T-002 quoted-string handshake reply is accepted | AC-003 | integration, new `quoted_version_handshake` | simulator `--quoted-version --firmware 1.2.3` | CONNECTION OK, INFO `1.2.3` / `108` | after the fix |
| T-003 escaped closing quote is rejected | AC-004 | integration, new `init_version_escaped_quote` (`rejected_connection` with the `escaped-quote` fault) | fault `VERSION escaped-quote` | CONNECTION ALERT, no leaked descriptor, reconnect OK | after the fix |
| T-004 simulator reply bytes | AC-005 | integration, `simulator_protocol`, `simulator_split_protocol` | raw serial exchange | exact reply `{"idx":1,"id":"@\u0019SC8HP\u0006TH2A","version":20231207,"bv":108}` | with step 2 |
| T-005 preserved behaviour | AC-006 | the remaining 42 cases | unchanged | all pass | baseline and final |
| T-006 memory safety of the parser | AC-001..004 | `test_focuser_qhy_simulator_asan` | ASan build | no report | final |

## Baseline evidence
Recorded in verification.md after step 1.

## Bug reproduction
Step 3 runs the updated simulator and tests against the unchanged driver: `capabilities`, `split_transport` and
every case that connects through the default reply fail at CONNECTION, reproducing the user's report hardware-free.

## Negative cases and permission boundaries
T-003 and the existing `init_version_failure`, `move_malformed_reply`, `move_partial_reply`, `motion_overlong_read`
cases keep the malformed-reply rejection.

## Isolation and cleanup
The suite uses a private fixture directory and PTYs; `make -C indigo_test test-clean` after the run.

## Deferred tests and coverage limitations
No hardware run (no device available); the hardware reply is modelled from the user's capture. Owner: Peter
Polakovic; follow-up: a user confirmation with the fixed driver.
