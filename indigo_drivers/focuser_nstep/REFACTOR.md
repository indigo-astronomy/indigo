# Rigel Systems nSTEP generated migration

Status: complete, 2026-09-12. Baseline hand-written version was `0x02000006`; generated driver version is `0x03000007` (`version = 7`). Hardware tests are explicitly out of scope. No generator implementation change was made. This migration follows the completed `focuser_nfocus` migration as its template; nSTEP is the richer sibling of nFOCUS and exposes several additional controls.

## Sources and protocol confidence

- Repository guidance: root `AGENTS.md`, `indigo_test/AGENTS.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (focuser standard), and the completed `focuser_nfocus` migration with its `REFACTOR.md`, `.driver`, host simulator and integration test.
- Production sources: hand-written `indigo_focuser_nstep.c`, public header, standalone main, README, host pseudo-terminal simulator (`focuser_nstep_simulator.c`), the Arduino simulator sketch (`.ino`), property inventory (`indigo_docs/PROPERTIES.md`) and the existing single-scenario integration test.
- Related local reference: `focuser_nfocus` shares the Rigel command style, 19200 baud, `0x06` identity, fixed-length replies and `:F...#` motion commands. nSTEP differs in identity reply (`S` vs `n`), and it exposes additional absolute position readback, stepping mode, phase wiring, backlash, temperature compensation and manual/automatic mode controls that nFOCUS does not.
- Manufacturer documentation: Rigel Systems nSTEP web page (linked from README). It documents stepper-motor operation, wave/half/full stepping, phase wiring, temperature compensation and stored positions, but not the serial command grammar, reply framing or error replies. The protocol grammar is therefore implementation-derived from the existing driver plus the Arduino/host simulators; simulator tests prove behavior against this inherited grammar, not every real firmware variant.

## Inherited protocol grammar (driver ⇄ device)

Fixed-length replies, no line terminator. All commands are sent verbatim; `#` terminates variable commands.

| Command | Reply | Meaning |
|---------|-------|---------|
| `\006` (0x06) | `S` (1) | Identity probe; `S` identifies nSTEP |
| `:RT` | 4 bytes | Temperature in tenths of °C (e.g. `+275` = 27.5); `-888` = no sensor |
| `:RA` | 4 bytes | Temperature-compensation coefficient (tenths) |
| `:RB` | 3 bytes | Temperature-compensation step count |
| `:RG` | 1 byte | Compensation mode; `2` = automatic, else manual |
| `:RE` | 3 bytes | Backlash steps |
| `:RP` | 7 bytes | Absolute position, signed (e.g. `+000050`) |
| `:RO` | 3 bytes | Out speed raw; displayed speed = `255 - raw` |
| `:RW` | 1 byte | Phase wiring `0`/`1`/`2` |
| `S` | 1 byte | Motion status; `1` = moving, `0` = idle |
| `:CC1` | none | Startup: disable continuous mode |
| `:CS001#` | none | Startup: set pulse interval |
| `:CO<nnn>#` | none | Set out speed raw (`255 - value`) |
| `:F<dir><mode><nnn>#` | none | Relative move: dir `1`=inward/`0`=outward, mode `0`=wave/`1`=half/`2`=full, `nnn` steps |
| `:F10000#` | none | Stop / abort |
| `:CW<n>#` | none | Phase wiring `0`/`1`/`2` |
| `:TA2:TC30#` / `:TA0` | none | Automatic (with 30 s interval) / manual compensation mode |
| `:TT±010#:TS<nnn>#` | none | Temperature-compensation coefficient sign + step count |
| `:TB<nnn>#` | none | Set backlash |

Note: the current host simulator also answers `:RH` (2 bytes) and `:RS` (3 bytes), which the driver never sends; these will be trimmed to the driver's actual command surface.

## Exposed property surface

nSTEP exposes the full manual-mode focuser surface plus optional controls:

- Standard: `DEVICE_PORT`, `DEVICE_PORTS`, `FOCUSER_SPEED` (1–254), `FOCUSER_DIRECTION`, `FOCUSER_STEPS` (0–999), `FOCUSER_ABORT_MOTION`, `FOCUSER_POSITION` (read-only, signed absolute readback), `FOCUSER_BACKLASH` (0–999), `FOCUSER_TEMPERATURE`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE` (manual/automatic).
- Hidden: `FOCUSER_REVERSE_MOTION`.
- Driver-specific (`X_`): `X_FOCUSER_STEPPING_MODE` (WAVE/HALF/FULL, one-of-many, default FULL) and `X_FOCUSER_PHASE_WIRING` (0/1/2, one-of-many, default 0).
- When `:RT` returns `-888`, the device has no temperature sensor and `FOCUSER_TEMPERATURE`, `FOCUSER_COMPENSATION` and `FOCUSER_MODE` are hidden.

## Existing behavior and findings

- Baseline low-level I/O uses legacy `indigo_io`, a raw integer file descriptor, a `pthread_mutex_t` for serialization and per-helper response buffers. The generated migration will use `indigo_uni_io`, a variadic `nstep_command(device, reply_length, format, ...)` helper (following the nFOCUS/dmfc pattern), a reusable response buffer in private data, checked writes, fixed-length reply validation and rejection of trailing bytes.
- Baseline connection treats malformed optional reads as non-fatal and never rolls back a partially opened session. The generated migration will make identity, temperature/compensation/mode reads, position, speed, phase wiring and the `:CC1`/`:CS001#` startup writes transactional via `connection_result`, closing on failure so a failed connect leaves no descriptor open and can be retried.
- Motion completion is status-only polling in the timer callback. nSTEP does have absolute position readback (`:RP`), so the migration keeps position polling but moves motion completion to a named `motion_finalizer` that polls `S` (and refreshes `:RP`), transitions `FOCUSER_STEPS`/`FOCUSER_POSITION` BUSY → OK/ALERT, enforces a bounded malformed/silent stall limit, and sends stop on persistent status failure so the driver stays recoverable after abort.
- The timer callback continues to poll `:RT` temperature (when present) and `:RP` position at the idle cadence; motion polling runs at the faster cadence through the finalizer.
- `FOCUSER_SPEED` uses the `:CO`/`:RO` pair with `255 - value`, which is self-consistent (unlike nFOCUS, which had a `:CO`/`:CF` bug). No behavioral speed bug is expected here; the migration will preserve the `:CO<nnn>#` command and range 1–254.
- Compensation display is `:RB / :RA` (step ÷ coefficient) with divide-by-zero guarded; setting compensation writes `:TT±010#:TS<nnn>#`. Mode automatic writes `:TA2:TC30#`, manual `:TA0`. These are preserved.
- The previous integration test is a single smoke scenario. It will be replaced with isolated direct-protocol and driver-behavior scenarios mirroring the nFOCUS suite, extended for nSTEP-only capabilities (position readback, stepping mode, phase wiring, backlash, compensation, mode).
- The host simulator will be extended to use shared `serial_motion` (elapsed-time relative motion with absolute position), an event journal, one-shot/sticky fault injection and optional absent-temperature mode, and trimmed to the driver's real command surface.

## Atomic plan and progress

1. **Baseline and evidence capture — complete.** Read repository/test standards, generator docs, existing nSTEP sources, simulator, `.ino` and test; reviewed the completed nFOCUS migration; captured protocol grammar and property surface above; built the unchanged driver (passed).
2. **Migration design and documentation — complete.** This `REFACTOR.md` records supported capabilities, non-applicable properties, simulator limits, implementation choices and final validation results.
3. **Authoritative generator migration — complete.** Added `indigo_focuser_nstep.driver` (serial; version 7). Protocol/state live in generator-owned blocks: `define`, `include`, `data` (response buffer, motion `stalled`/`active`/`uncertain` state), shared `code` (`nstep_command`, `nstep_integer`, temperature/speed/position/moving parsers, `nstep_stop`, `nstep_motion_state`, `motion_finalizer`, `nstep_open`/`nstep_close`), `focuser.on_attach`, `focuser.on_connect`, `focuser.on_disconnect`, `focuser.on_timer`, and per-property `on_change` blocks for `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `X_FOCUSER_STEPPING_MODE` (empty copy-only, applied at move time) and `X_FOCUSER_PHASE_WIRING`. Preserved the public property surface, raised the version to `0x03000007`, regenerated checked-in `.c`, `.h`, `_main.c`.
4. **Simulator support for tests — complete.** Extended the host simulator with elapsed `serial_motion` absolute position, command journal, one-shot/sticky fault hooks and optional temperature absence; trimmed the unused `:RH`/`:RS` commands. Model limited to properties the driver exposes.
5. **Full applicable simulator tests — complete.** Replaced the smoke test with 12 named scenarios covering direct protocol behavior, property/capability inventory (including `X_` properties, ranges and read-only position), normal and absent-temperature connection, transactional startup failure/retry, speed command/readback, stepping-mode and phase-wiring commands, backlash/compensation/mode commands, inward/outward/no-op relative motion with position readback, overlap guard, moving abort, status failure/recovery and independent additional instances.
6. **Repository synchronization and validation — complete.** Updated `indigo_docs/PROPERTIES.md`, `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_test/CHANGES.md`, the nSTEP row in `MIGRATION_STATUS.md` and the `indigo_test/Makefile` simulator dependency; the `.driver` and `REFACTOR.md` were already present in the Xcode project group. Verified the driver build and the simulator suite.

## Validation

- `make -f ../../Makefile.drv` in `indigo_drivers/focuser_nstep`: passed (driver regenerated and built; only pre-existing `hidden = false` informational generator notes).
- `make build/integration/test_focuser_nstep_simulator && ./build/integration/test_focuser_nstep_simulator` in `indigo_test`: passed; all 12 scenarios passed.

## Coverage boundaries

Automated coverage will include all behavior exposed by this driver through INDIGO properties and its inherited nSTEP protocol grammar. It will not include real motor direction, electrical serial behavior, physical travel, stored-position UI behavior, ASCOM/Windows vendor software behavior, unknown firmware variants, actual temperature accuracy or hardware interruption. Linux/Windows runtime and hardware validation remain out of scope for this task.
