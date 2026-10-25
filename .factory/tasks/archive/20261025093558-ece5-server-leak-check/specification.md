# Specification
Status: final

## Problem and outcome
`make -C indigo_test test-server-leaks` runs indigo_server with the simulator drivers under Valgrind, exercises every
device and reports the leaks by allocating INDIGO function.

## In scope
`indigo_test/leak/run_server_leak_check.py`, the `test-server-leaks` target, `indigo_test/AGENTS.md`,
`indigo_docs/DRIVER_TESTING_BASICS.md`, the Xcode project.

## Out of scope
Leak fixes; default test targets (the check is slow and needs Valgrind).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The check starts the server under Valgrind with a private HOME on a free port, connects all 19 simulator devices, runs exposures, guiding, wheel, focuser, mount (unpark, tracking, slew, park), dome (unpark, slew, park), rotator and flat panel operations, disconnects, `--cycles` times, and stops the server so that Valgrind reports. | runs with 1 and 3 cycles; the operations checked one by one against a server without Valgrind (all OK) |
| AC-002 | The report lists the totals and every loss record with its allocating INDIGO frame, driver frames named; exit 1 on definite or indirect loss, 0 without, 2 when it cannot run. | the runs |
| AC-003 | Confirmed leaks are registered as findings. | FND-...-001 to 003 |

## Failure behavior and edge cases
No Valgrind, no built server, a server that does not start or stop: exit 2 with the reason.

## Permissions, data, and security requirements
Private HOME, so the user's `~/.indigo` is untouched; loopback port only.

## Compatibility and nonfunctional requirements
Linux with Valgrind; a run takes about 12 minutes per cycle.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: add tests for it.
