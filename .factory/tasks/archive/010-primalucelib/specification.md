# Specification

Status: approved for planning (requester decisions of 2026-10-22, see brief.md)

## Problem and outcome
The PrimaLuce Lab JSON protocol exists only inside `focuser_primaluce`. A reusable library
`indigo_drivers/shared/primalucelib/` provides the transport, framing, reply parsing, error
detection, named protocol constants and device-level functions for every known JSON
controller. `focuser_primaluce` uses it with unchanged behavior and unchanged wire traffic.

## In scope
- `primalucelib.h` / `primalucelib.c` (+ the relocated `jsmn.h`) and a protocol reference
  `PROTOCOL.md` in `indigo_drivers/shared/primalucelib/`.
- `focuser_primaluce` `.driver` refactored to call the library, regenerated outputs.
- Unit tests of the library; trace capture in the existing simulator suite.
- Xcode / Visual Studio / test Makefile registration; driver `REFACTOR.md`.

## Out of scope
New drivers, Sesto Senso v1 ASCII protocol, property changes, generator changes, behavior
fixes in the driver, the pre-existing Linux PTY reconnect and `status_polling` timing
failures (registered as findings, see verification.md).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The library offers named constants for protocol keys, values, request strings and reply paths, and functions with self-describing names for the controller, focuser (MOT1), ARCO rotator (MOT2), GIOTTO and ALTO command sets. | Review of `primalucelib.h`; unit tests call each group. |
| AC-002 | The library contains no `PRIVATE_DATA`, `DRIVER_NAME` or property code and is built as a static library on every platform: `build/lib/libprimalucelib.a` (Make), `primalucelib.lib` (Visual Studio), compiled once into the Xcode library targets. Drivers include only its header and link it; the library exports only `primalucelib_`/`PRIMALUCE_` symbols and keeps jsmn internal. | Library and driver builds; `nm` of the archive, the driver archive and the driver library; unit test linked against the archive. |
| AC-003 | Every request the original driver sends is produced byte-for-byte identically by the corresponding library call. | Unit test: golden requests captured over a pseudo-terminal, compared with the strings of the original driver. |
| AC-004 | Reply handling is unchanged: newline framing, ignored `[` log lines, ERROR under `cmd`/`set`/`get` with last-error text, quoted-string garbage, parse failure, link failure flag, NUL-terminated string tokens, missing paths (NULL / 0). | Unit tests against canned replies over a pseudo-terminal. |
| AC-005 | `focuser_primaluce` no longer contains protocol strings, path arrays, the jsmn parser or transport code of its own; it calls the library. | `grep` of the `.driver`; review. |
| AC-006 | Observable behavior of the driver is unchanged: the same simulator cases pass and fail as in the baseline, for the same reasons. | Baseline vs. post-change runs of `test_focuser_primaluce_simulator` and `test_focuser_primaluce_motion`; final recorded run through `tools/run_driver_test.py`. |
| AC-007 | Wire traffic is unchanged: the per-case sets of distinct requests in the simulator event logs are identical before and after the change. | Trace capture (`INDIGO_SIMULATOR_TRACE_DIR`) before and after; normalized comparison. |
| AC-008 | New persistent files are registered in `indigo.xcodeproj`, `indigo_windows.sln`, the driver `.vcxproj`/filters and the test Makefile; the relocated `jsmn.h` keeps its license header unchanged; the library files carry Rumen G. Bogdanovski's copyright with a history line crediting the original driver code. | Review of project files; `git diff -M`. |

## Failure behavior and edge cases
Identical to the original driver (AC-004, AC-006). Library functions report failure with
`false`/`NULL`; no function exits, blocks indefinitely or allocates.

## Permissions, data, and security requirements
Replies are untrusted: bounded buffers (8192-byte response, 1024 tokens), escaped URL
encoding over HTTP, names containing `"` or `\` are refused before they reach a JSON string.
No credentials are added; the simulator's synthetic WiFi credentials stay synthetic.

## Compatibility and nonfunctional requirements
Portable C (gnu11 / MSVC), `indigo_uni_io` only, no platform-specific code. Driver version is
not bumped: there is no behavior change (root `AGENTS.md` requires a bump for fixed behavior).

## Assumptions and open questions
- GIOTTO, ALTO and some SESTO SENSO 3 commands are known only from the INDI drivers; they are
  marked unverified in `PROTOCOL.md` and the header. Low risk: no driver uses them yet.
- No blocking questions.

## Owner decisions and requirement changes
- 2026-10-22, Rumen G. Bogdanovski: build the library as a static library on all platforms and link the drivers against it (was: included source with internal linkage); library copyright Rumen G. Bogdanovski, with the history line crediting Peter Polakovic's original protocol code; keep commit 7d46bc02e (Linux fix) and recreate the later, unpushed commits. Affects AC-002 and AC-008.
- 2026-10-22, requester (Rumen G. Bogdanovski): directory `primalucelib`; JSON protocol for all
  devices; non-interactive simulator test run, local commit, no push.
