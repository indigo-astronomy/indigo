# Rigel Systems nFOCUS generated migration

Status: complete, 2026-09-12. Baseline hand-written version was `0x02000006`; generated driver version is `0x03000007`. Hardware tests are explicitly out of scope. No generator implementation change was made.

## Sources and protocol confidence

- Repository guidance: root `AGENTS.md`, `indigo_test/AGENTS.md`, driver-development basics, generator migration guide, serial-simulator contract, focuser testing standard, and completed generated focuser migrations with `REFACTOR.md`.
- Production sources: hand-written `indigo_focuser_nfocus.c`, public header, standalone main, README, host pseudo-terminal simulator, Arduino simulator sketch, property inventory and existing integration test.
- Related local reference: `focuser_nstep` shares Rigel command style, 19200 baud, `0x06` identity, fixed-length replies and `:F...#` motion commands, but it exposes additional nSTEP-only position, mode, backlash and compensation controls that nFOCUS does not expose.
- Manufacturer documentation found: current Rigel Systems nFOCUS web page and downloadable nFOCUS one-page manual. They document nFOCUS DC motor behavior, USB-nFOCUS computer operation, relative focusing, forward/reverse direction, step size/interval, stored positions and optional external temperature sensor. They do not document the serial command grammar, reply framing, error replies or abort command.
- Protocol grammar for this migration therefore remains implementation-derived from the existing driver plus Arduino/host simulators. Simulator tests prove driver behavior against this inherited grammar, not every possible real firmware variant.

## Existing behavior and findings

- The driver is a single 19200 baud serial focuser with additional instances. It probes `0x06 -> n`, reads temperature with `:RT` and outgoing speed with `:RO`, writes startup pulse interval with `:CS001#`, moves with `:F<direction>1<steps>#`, polls status with `S`, and stops with `:F11000#`.
- Exposed properties: `DEVICE_PORT`, `DEVICE_PORTS`, `FOCUSER_SPEED`, `FOCUSER_DIRECTION`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION` and optionally `FOCUSER_TEMPERATURE`. `FOCUSER_POSITION` and `FOCUSER_REVERSE_MOTION` are hidden, so absolute GOTO/SYNC, position readback, limits, backlash, compensation and mode are non-applicable at the driver boundary.
- Baseline low-level I/O used legacy `indigo_io`, raw integer file descriptors and helper-local response buffers. The generated migration now uses `indigo_uni_io`, checks writes and fixed-length replies, rejects extra bytes after fixed replies, and stores the reusable response buffer in private data.
- Baseline connection treated malformed optional startup reads as non-fatal. The generated migration makes identity, temperature read, speed read and startup `:CS001#` transactional so failed or malformed startup responses roll back the connection and can be retried.
- Motion completion is status-only. The driver has no absolute position readback, so `FOCUSER_STEPS` transitions BUSY -> OK/ALERT based on `S`. The generated migration adds a bounded malformed/silent status limit, sends stop on persistent status failure, and leaves the driver in a recoverable state after abort.
- `focuser_speed_handler()` in the baseline formatted `:CO%03d#` and immediately overwrote the same buffer with `:CF%03d#`, so only `:CF` was sent. The generated driver preserves the actually emitted `:CF` command and the simulator/test suite assert that no `:CO` command is sent by this driver.
- The host simulator already had ready-file support and nominal commands. It now also uses shared `serial_motion`, supports an event journal, one-shot and sticky fault injection, optional absent-temperature mode and elapsed status-only relative motion. It deliberately does not invent unexposed absolute-position or stored-position behavior.
- The previous integration test was a single smoke scenario and exited with signal 139 during baseline execution. It has been replaced with isolated direct-protocol and driver-behavior scenarios.

## Atomic plan and progress

1. **Baseline and evidence capture - complete.** Read repository/test standards, generator docs, existing nFOCUS sources, simulator and tests; inspected current Rigel web/manual references; built the unchanged driver; ran the existing test and recorded the signal-139 baseline failure.
2. **Migration design and documentation - complete.** This `REFACTOR.md` records supported capabilities, non-applicable properties, simulator limits, implementation choices and final validation results.
3. **Authoritative generator migration - complete.** Added `indigo_focuser_nfocus.driver`, migrated protocol/state into generator-owned blocks, switched to uniform I/O and generated lifecycle/handler queues, preserved the public property surface, raised the version to `0x03000007`, and regenerated checked-in `.c`, `.h` and `_main.c`.
4. **Simulator support for tests - complete.** Extended the host simulator with elapsed `serial_motion` state, command journal, one-shot/sticky fault hooks and optional temperature absence while keeping the model limited to properties the driver exposes.
5. **Full applicable simulator tests - complete.** Replaced the smoke test with 10 named scenarios covering direct protocol behavior, property/capability inventory, normal and absent-temperature connection, transactional startup failure/retry, speed command/readback, inward/outward/no-op relative motion, overlap guard, moving abort, status failure/recovery and independent additional instances.
6. **Repository synchronization and validation - complete.** Updated property inventory, simulator inventory, test coverage notes and the nFOCUS row in `MIGRATION_STATUS.md`; verified the driver build and simulator suite.

## Validation

- `make -f ../../Makefile.drv` in `indigo_drivers/focuser_nfocus`: passed.
- `make build/integration/test_focuser_nfocus_simulator && ./build/integration/test_focuser_nfocus_simulator` in `indigo_test`: passed; all 10 scenarios passed.

## Coverage boundaries

Automated coverage includes all behavior exposed by this driver through INDIGO properties and its inherited nFOCUS protocol grammar. It does not include real motor direction, electrical serial behavior, physical travel, stored-position UI behavior, ASCOM/Windows vendor software behavior, unknown firmware variants, actual temperature accuracy or hardware interruption. Linux/Windows runtime and hardware validation remain unverified because they are out of scope for this task.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

**Not covered.** The only `reject_change` condition in this driver is `FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE`, and that state is only published by `INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE` for the moment between accepting the abort request and running the queued handler. `focuser_abort_motion_handler` ends in OK or ALERT and `nfocus_stop()` writes without reading a reply, so the handler never parks in BUSY. Throughout that window `FOCUSER_STEPS` is itself BUSY from the move being aborted, so the framework BUSY guard in `INDIGO_COPY_VALUES_PROCESS_CHANGE` refuses the request before the driver guard can. An attempt to widen the window with a silent `S` status fault was written and discarded because the guard still did not fire. The guard is kept for symmetry with the other focusers but is unreachable through the simulator; decide separately whether to drop it or to give abort an observable BUSY phase as `focuser_lakeside` has.
