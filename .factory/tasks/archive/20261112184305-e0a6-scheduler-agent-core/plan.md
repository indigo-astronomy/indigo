# Development plan

Status: complete

## Approach and affected components
New agent `indigo_drivers/agent_scheduler/`:
- `indigo_agent_scheduler_header.c/.h`: task header parser (pure C).
- `indigo_agent_scheduler_sky.c/.h`: ephemeris grid and constraint evaluation (pure C, indigocat).
- `indigo_agent_scheduler.c/.h`: device + client, task store, properties, persistence, wrapper, run/abort/preempt, selection.
- `Makefile.inc`: `Sequencer.js.dat` hexdump rule, `-lindigocat`.
Registration: root `Makefile` driver list, `indigo_server/indigo_server.c`, Windows project files and solution, Xcode project. `indigo_names.h`, `indigo_docs/PROPERTIES.md`. Tests under `indigo_test/`.

## Baseline
- Commands and environment: see verification.md "Baseline".
- Existing failures: recorded in verification.md.
- Repository revision / working-tree state: branch `scheduler` at cbf8e03bf plus this task.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Baseline: build libindigo, scripting agent; run `test-agent-scripting` | — | None | verification.md | done: 36/37, pre-existing failure `change bindings forward local access tokens` (verification.md) |
| 2 | Header parser + unit test `test_agent_scheduler_header` | AC-001 | 1 | unit test passes | done: 9/9 after correcting the test's body expectation (the blank line ending the header belongs to the body) |
| 3 | Sky grid and constraints + unit test `test_agent_scheduler_sky` | AC-002 | 1 | unit test passes | done: 13/13; grid extended to 48 h so that a 24 h duration and a 12 h look-ahead fit; a task with `@target` also requires the target above the horizon |
| 4 | Property names in `indigo_names.h` | AC-008 | — | build | done: 11 properties, phase 2 items only |
| 5 | Agent: store, properties, persistence, commands | AC-003, AC-004 | 2, 4 | integration cases pass | done: found that INDIGO suppresses updates without a value or state change; requests are now acknowledged Busy and completed Ok/Alert (also required by the configuration restore) |
| 6 | Agent: wrapper, run, abort, urgent preemption, selection, repeat/retry, control | AC-005..AC-007 | 3, 5 | integration cases pass | done: 17/17 integration cases, three consecutive runs, ASan/UBSan clean; an empty task list no longer stops the scheduler; the sky grid samples Sun/Moon hourly (0.8 s -> 0.088 s) |
| 7 | Registration: Makefile, server, Windows projects, Xcode project, `PROPERTIES.md`, README | AC-008 | 6 | build of agent and server | done: UNTESTED_DRIVERS, static server table (syntax check), vcxproj + sln + server reference, Xcode group/targets/Sequencer.js.dat phase/tests, PROPERTIES.md, README, HOME.md, INDIGO_AGENTS.md |
| 8 | Design document alignment (deviations below) | — | 6 | inspection | done |
| 9 | Recorded run `python3 tools/run_driver_test.py agent_scheduler`, sanitizer run, quality and security review | all | 7 | verification.md | done: independent review found 1 blocking and 12 should-fix items, fixed or tracked (quality-review.md); final recorded run 22/22, ASan/UBSan clean |

## Test strategy
Unit tests for parser and sky with fixed inputs. Integration test links the real Scripting Agent (Duktape) and the scheduler; task bodies use `Sequence.wait()` steps so no device simulators are needed for the scheduling state machine; urgent preemption aborts a waiting sequence. A stand-in "Guider Agent" device records the guider abort.

## Security risks and mitigations
Untrusted task text: header validated, length limits, JSON encoding of strings placed into the wrapper script; body evaluated inside `eval()` in a function with errors caught.

## Dependencies and architecture decisions
No new dependencies (indigocat, Duktape via the existing Scripting Agent).

## Migration, rollout, and rollback
New agent only; removing it from the driver list rolls it back.

## Deviations and owner decisions
- Plain device + client like the Scripting Agent, not a filter agent; fixed default agent names as used by `Sequence.start()`. Reason: the scheduler controls agents by name and does not select hardware devices; the filter machinery adds device lists and connection handling it does not need.
- `@twilight none` means no limit; civil, nautical and astronomical mean the Sun below −6°, −12° and −18° (the design table listed four values for four keywords, which is corrected).
- Sequencer.js is not changed (owner); the library-reload risk stays open and is documented.
- An empty task list keeps the scheduler running (ready for urgent tasks); the design said it stops when all tasks finished.
- Sky grid 48 h with hourly Sun/Moon samples instead of 24 h with 5-minute samples.
- The agent is listed in UNTESTED_DRIVERS, like the Platesolver Agent, until it is validated in real use.
