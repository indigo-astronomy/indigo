# Specification

Status: approved design (`indigo_docs/SCHEDULER.md`), phase 2 scope per brief.md

## Problem and outcome
A Scheduler Agent named "Scheduler Agent" that implements the phase 2 behaviour of `indigo_docs/SCHEDULER.md`.

## Current-state audit
- No scheduler exists in INDIGO. Sequences are run by `Sequencer.js` inside the Scripting Agent (`indigo_drivers/agent_scripting/`), one at a time (`indigo_sequencer.start()` refuses a second one).
- Interfaces used: `AGENT_SCRIPTING_RUN_SCRIPT` (synchronous evaluation, `indigo_agent_scripting.c:1308`), Sequencer properties `SEQUENCE_STATE`, `SEQUENCE_NAME`, `AGENT_ABORT_PROCESS` on "Scripting Agent"; `AGENT_ABORT_PROCESS`/`AGENT_START_PROCESS` of the Imager, Mount and Guider agents; `GEOGRAPHIC_COORDINATES` of the Mount Agent.
- Persistence constraints: config restore replays only defined, visible, writable properties and waits for an update per replayed change (`indigo_driver.c:636-676`); `indigo_save_property()` skips hidden and read-only properties (`indigo_driver.c:752`). The bus sends CONFIG LOAD to every local device after attach (`indigo_bus.c:885-890`).
- Known existing risk (not fixed here, owner deferred Sequencer changes): reloading `Sequencer.js` replaces the `indigo_sequencer` singleton (`Sequencer.js:758`); a client that reloads the library during a scheduled run orphans it.
- Platforms: Linux, macOS, Windows; portable C, INDIGO timers/handlers, no OS-specific code.
- Hardware: none. Hardware-test decision: no hardware testing; all tests are hardware-free.

## In scope / out of scope
See brief.md.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The header parser accepts every documented key with its defaults and rejects unknown, repeated, malformed and out-of-range keys, missing `@task`, constraints without `@target`, empty bodies and oversized text, naming the line | unit test `test_agent_scheduler_header` |
| AC-002 | The constraint engine evaluates altitude, airmass, HA, moon separation (fixed and Lorentzian, ignored with the moon below the horizon), illumination, twilight and windows (absolute and nightly) correctly for fixed dates and sites | unit test `test_agent_scheduler_sky` |
| AC-003 | Tasks can be uploaded, read back, edited, renamed, deleted, enabled, disabled, reset and reordered through the documented properties; invalid uploads are rejected with Alert and a message | integration test `test_agent_scheduler` |
| AC-004 | Tasks and their status persist across an agent restart through the configuration | integration test |
| AC-005 | With START, the agent runs eligible tasks one at a time through the Scripting Agent, by class and score, honouring `@window`, `@repeat` and `@retry`, and never interrupts a running non-urgent task for a non-urgent one | integration test |
| AC-006 | An eligible urgent task preempts a running non-urgent task: the sequence is aborted, the guider is aborted, the preempted task becomes RETRY (no retry consumed) or FAILED per `@preempted`, then the urgent task runs; a second urgent task waits for the first; an urgent task that is not eligible does not preempt | integration test |
| AC-007 | A body with a script error and a body without `start()` count as failures; STOP lets the task finish; ABORT aborts it and marks it RETRY | integration test |
| AC-008 | The agent is registered in the server, the build, the Windows and Xcode projects; its properties are in `indigo_names.h` and `PROPERTIES.md` | build, inspection |

## Failure behavior and edge cases
As in `indigo_docs/SCHEDULER.md` sections Validation, Task states, Urgent preemption, Aborting a sequence and Running a task.

## Permissions, data, and security requirements
Task text is untrusted: header validated, size limited (16 KiB), task names JSON-encoded into the wrapper; the body has the trust level of `AGENT_SCRIPTING_RUN_SCRIPT`.

## Compatibility and nonfunctional requirements
No change to existing agents or `Sequencer.js`. Bus callbacks do not block; work runs on the agent handler queue.

## Assumptions and open questions
Default agent names ("Scripting Agent", "Imager Agent", "Mount Agent", "Guider Agent") as used by `Sequence.start()`.

## Owner decisions and requirement changes
2026-11-12, Rumen G. Bogdanovski: implementation on branch `scheduler`; no Sequencer.js changes yet; copyright Rumen G. Bogdanovski.
