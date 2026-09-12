# Optec FocusLynx driver refactoring

Status: complete, 2026-09-12. Baseline version 0x02000001, completed version 0x02000002. The driver remains hand-written and README remains unchanged.

Generator migration is **deferred by explicit user decision**. The generator derives every emitted symbol from the device class (`device->type` in `parse_device_block`, `indigo_tools/indigo_generator.c:757`), so two `focuser` blocks would collide on `focuser_attach`, `focuser_connection_handler`, `FOCUSER_DEVICE_NAME` and the `focuser.*` code-block markers. The DSL has no device-block identifier and no `.driver` in the repository declares two blocks of one class. FocusLynx is one hub with two focusers behind a single serial port, so the topology cannot be expressed without a generator change; that change was offered and declined. The driver therefore stays hand-written.

Scope agreed with the user: protocol audit, simulator completion, **replacement of timers by handler queues**, correctness fixes found during the audit, and full automated test coverage. No generator edits, no `.driver` file, no hardware testing.

## Protocol audit

Primary source: bundled `FocusLynx_Command_Processing_rev3.pdf`, 18 pages, Revision 3 – March 2013.

Framing is `<` … `>`; the two characters after `<` address `F1`, `F2` or the hub `FH`. Every command is acknowledged with `!` and a newline before the payload; a syntax error or unknown command returns an error code and message instead of the expected payload. The document calls the terminator "newline (ASCII 0x10)", which is a documentation error — 0x10 is DLE and the wire terminator is LF.

Commands and their acknowledgement payloads: `HELLO` returns the nickname; `HALT` returns `HALTED` and silently disables temperature compensation; `HOME` returns `H`; `CENTER` returns `M`; `MAzzzzzz` (6 digits, zero-padded) returns `M`; `MIRz` / `MORz` (relative in/out, z = speed 0 high, 1 low) return `M`; `ERM` returns `STOPPED` and resumes temperature compensation if it was enabled before the relative move. All `SC*` setters and `RESET` return `SET`.

Block replies are delimited by a header line and `END`. `GETSTATUS` emits `STATUSn` plus `Temp(C)`, `Curr Pos`, `Targ Pos`, `IsMoving`, `IsHoming`, `IsHomed`, `FFDetect`, `TmpProbe`, `RemoteIO`, `Hnd Ctlr`. `GETCONFIG` emits `CONFIGn` plus `Nickname`, `Max Pos`, `Dev Typ`, `TComp ON`, `TempCo A`–`TempCo E`, `TC Mode`, `BLC En`, `BLC Stps`, `LED Brt`, `TC@Start`. `FHGETHUBINFO` emits `HUB INFO` plus `Hub FVer`, `Sleeping`, `Wired IP`, `WF Atchd`, `WF Conn`, `WF FVer`, `WF FV OK`, `WF SSID`, `WF IP`, `WF SecMd`, `WF SecKy`, `WF WepKI`. The `GETCONFIG` example in the PDF prints a bare `CONFIG` header while its prose specifies `CONFIGn`; the prose is treated as authoritative and the discrepancy is recorded here.

Two semantics matter for driver behavior. `SCCPzzzzzz` (sync) "is only accepted for focusers which are unable to home. All Optec Focusers must home and thus cannot use this command", so SYNC is not universally available and depends on the configured device type. There is **no reverse-direction command** anywhere in revision 3, so `FOCUSER_REVERSE_MOTION` is necessarily a driver-local convention rather than a device setting.

Appendix A defines device types `OA OB OC OD OE OF OG`, `FA FB FC`, `SA`–`SN`, `SO SP SQ`, `TA` and `ZZ` (default, no function).

## Legacy driver findings

**D1 — abort never validates its reply.** `indigo_focuser_optecfl.c:339` calls `strncpy(line, "HALTED", 6)` where `strncmp` was intended. The call returns a non-NULL pointer, so the condition is always true, and it simultaneously overwrites the reply that was being checked. A failed HALT reports success.

**D2 — poll loop corrupts the connection refcount.** `focuser_timer_callback` increments `PRIVATE_DATA->count` on every successful status block (`:74`). `count` is the shared-connection reference count consumed by `optecfl_close` (`:205`), so it grows once per second per connected focuser and the `count > 1` branch always wins. The serial handle is therefore never closed and the port stays open after both devices disconnect.

**D3 — open is not transactional.** On a `GETHUBINFO` failure `optecfl_open` closes the shared handle and zeroes `count` (`:163`–`:165`), destroying a still-connected sibling device's session rather than rolling back only its own attempt.

**D4 — device type is never parsed.** The driver matches `strncmp(key, "Dev Type", 8)` (`:181`) but the hardware sends `Dev Typ`. The eighth character is a space against `e`, so the comparison always fails and `X_FOCUSER_TYPE` never reflects the hardware value. The simulator reproduces the driver's spelling rather than the protocol's, which is why the defect is invisible under simulation.

**D5 — disconnect deadlock.** `focuser_connection_handler` takes `PRIVATE_DATA->mutex` (`:215`) and then calls `indigo_cancel_timer_sync` (`:233`), which waits for `focuser_timer_callback` to finish; that callback blocks on the same mutex (`:64`).

**D6 — device type table disagrees with Appendix A.** The list invents `FD` ("DirectSync TEC") and `RA` ("Robo-Focus"), neither of which exists in revision 3; it omits `SP` (FeatherTouch Hi-Torque) and `ZZ` (default); and the labels for `OC`, `OD`, `FA`, `FB`, `FC`, `SO` and `SQ` do not match the appendix.

**D7 — unset device type sends a NULL pointer.** `focuser_type_handler` leaves `value` NULL when no switch is on and passes it to `%2s` (`:256`).

**D8 — SYNC is unconditional and publishes the wrong property.** `SCCP` is issued regardless of device type even though the protocol rejects it for homing focusers, and the success branch sets `FOCUSER_STEPS_PROPERTY->state` while updating `FOCUSER_POSITION_PROPERTY` (`:292`–`:294`).

**D9 — unbounded read loop.** The `GETHUBINFO` loop (`:149`–`:161`) has no `else break` on a failed read, unlike the `GETCONFIG` and `GETSTATUS` loops; a truncated reply spins forever.

**D10 — non-portable transport.** The driver uses raw descriptors with `indigo_io.h` (`indigo_printf`, `indigo_read_line`) instead of `indigo_uni_io`, contrary to the repository rule, and has no `indigo_uni_discard` / `indigo_uni_read_section2`, no explicit first-byte and inter-byte timeouts and no completeness validation.

**D11 — per-function response buffers.** Every helper declares its own `char line[80]` instead of one buffer in private data.

**D12 — inconsistent parsing.** `GETSTATUS` scans `"%15[^=]=%15[^\n]s"` while `GETCONFIG`/`GETHUBINFO` scan `"%15[^=]= %15[^\n]s"`; the trailing `s` is meaningless and `value[80]` is only ever filled to 15 characters.

**D13 — unchecked string copy.** `strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, value, INDIGO_VALUE_SIZE)` (`:156`) does not guarantee termination; `INDIGO_COPY_VALUE` is the house helper.

**D14 — polling gated on a disabled feature.** Status polling runs only while `FOCUSER_MODE_PROPERTY->state == INDIGO_OK_STATE && FOCUSER_MODE_MANUAL_ITEM->sw.value`, although `FOCUSER_MODE` is hidden and its handler is commented out.

**D15 — timers instead of handler queues.** The driver uses `indigo_set_timer` / `indigo_reschedule_timer` / `indigo_cancel_timer_sync` with two `indigo_timer *` slots and a `pthread_mutex_t` performing the serialization that a shared handler queue provides. The two logical devices are not linked through `master_device`, so nothing serializes their access to the shared handle at framework level.

**D16 — hand-rolled change dispatch.** `focuser_change_property` copies values, sets BUSY and updates by hand instead of using `INDIGO_COPY_*_PROCESS_CHANGE`, so it lacks the framework BUSY guard against overlapping requests.

**D17 — unused protocol capability.** `HOME`, `CENTER`, relative moves (`MIR`/`MOR`/`ERM`), nickname, temperature compensation (`SCTE`, `SCTM`, `SCTC`, `SCTS`), backlash (`SCBE`, `SCBS`), LED brightness and `RESET` are all documented and unimplemented. `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION` and `FOCUSER_MODE` exist in the INDIGO focuser base for the first three groups.

**D18 — shared mutex destroyed twice.** `focuser_detach` calls `pthread_mutex_destroy` on the shared mutex once per logical device.

## Simulator findings

**S1** Motion is a background thread stepping one unit per 10 ms rather than the mandated `indigo_test/simulator_common/serial_motion.h` elapsed-time model. **S2** The hub header is `STATUS` instead of `HUB INFO`. **S3** The configuration block emits `Dev Type`, matching D4 rather than the protocol. **S4** `GETSTATUS` omits `IsHoming`, `IsHomed`, `FFDetect`, `RemoteIO`, `Hnd Ctlr`. **S5** `GETCONFIG` omits `Nickname`, `TComp ON`, `TempCo A`–`E`, `TC Mode`, `BLC En`, `BLC Stps`, `LED Brt`, `TC@Start`. **S6** Hub info omits every field after `Hub FVer`. **S7** `HELLO`, `HOME`, `CENTER`, `MIR`, `MOR`, `ERM`, `SCNN`, `SCTE`, `SCTM`, `SCTC`, `SCTS`, `SCBE`, `SCBS`, `FHSCLB` and `RESET` are unimplemented. **S8** Unknown and malformed commands are silently ignored instead of returning the documented error string. **S9** There is no fault injection and no command journal, unlike the refactored reference simulators. **S10** Move and sync clamp the upper bound only, never negatives. **S11** Device type is accepted without validation against Appendix A. **S12** Motion rate is fixed and unrelated to the configured Max Pos.

## Atomic plan

1. **Baseline and audit.** Record the starting tree, read the protocol document in full, audit driver and simulator against it, and write this file. *(no code changes)*
2. **Simulator completion.** Rebuild the simulator on `serial_motion.h`; correct the hub and configuration headers and the `Dev Typ` field; emit the full documented status, configuration and hub-info field sets; implement the missing commands; return the documented error reply for unknown and malformed input; validate device types and clamp both bounds; add a parent-owned command journal and one-shot fault injection. Fixes S1–S12.
3. **Driver lifecycle on handler queues.** Replace both timers and the mutex with handler queues, link focuser #2 to focuser #1 through `master_device` so one queue serializes the shared handle, make `optecfl_open` / `optecfl_close` transactional with a correct reference count, and cancel pending handlers without the synchronous timer wait. Fixes D2, D3, D5, D15, D18.
4. **Driver transport.** Move to `indigo_uni_io` with `indigo_uni_discard` and `indigo_uni_read_section2`, explicit first-byte and inter-byte timeouts, NUL capacity and completeness validation; add a variadic command helper in the `dmfc_command` style and a single response buffer in private data; bound every block read. Fixes D1, D9, D10, D11, D12, D13.
5. **Protocol and property correctness.** Parse `Dev Typ`; rebuild the device type table from Appendix A; gate SYNC on the configured device type and publish the property actually changed; drive change dispatch through `INDIGO_COPY_*_PROCESS_CHANGE`; decouple polling from the disabled mode property; guard the unset-type case. Fixes D4, D6, D7, D8, D14, D16.
6. **Test coverage.** Extend `indigo_test/integration/test_focuser_optecfl_simulator.c` from its two smoke scenarios to the full focuser class standard for both logical devices, including shared lifetime, fault and recovery paths, and record the scenario mapping in `indigo_test/CHANGES.md`.
7. **Integration and closure.** Bump `DRIVER_VERSION`, keep `indigo_docs/PROPERTIES.md`, `MIGRATION_STATUS.md` status columns, the simulator inventory and Xcode project entries consistent, and record final results here.

## Acceptance limits

No hardware testing is performed, by user instruction. PTY-based simulation cannot certify real encoder travel, homing behavior, firmware error strings or the device-type-dependent rejection of `SCCP`; those remain hardware acceptance gaps. Temperature compensation, backlash, homing, centering, relative moves, nickname, LED brightness and factory reset (D17) are **out of scope for this pass** and remain unimplemented; they are recorded here so the capability gap is explicit rather than forgotten.

Verification is simulator-only. No hardware test was run, by explicit user instruction.

## Progress log

### Step 1 — baseline and audit

Complete. Starting tree is clean for this driver; `git log` for `indigo_drivers/focuser_optecfl` shows no in-flight work. Read all 18 pages of the protocol document and extracted the full command, reply and block-field inventory recorded above. Audited the 578-line driver and the 392-line simulator line by line against it, producing findings D1–D18 and S1–S12. Confirmed the generator blocker by reading `parse_device_block` and by scanning every `.driver` in the repository for duplicate device classes; none exists. No code has been changed in this step.

### Step 2 — simulator completion

Complete and verified in isolation. The simulator was rewritten against the protocol document. Motion now runs on the shared `serial_motion.h` elapsed-time model with stop and sync; homing, centering and relative moves reuse it. The hub block header is `HUB INFO` and carries all twelve documented fields, `GETCONFIG` emits `CONFIGn` with the full field set including the corrected `Dev Typ` spelling, and `GETSTATUS` emits all ten documented flags. `HELLO`, `HOME`, `CENTER`, `MIR`, `MOR`, `ERM`, `RESET`, every `SC*` setter, the hub `SCLB` and the Wi-Fi commands are implemented, each validating its parameter shape. Unknown or malformed input returns an error reply; its concrete shape (`ER=<code>` followed by a message line) is simulator-defined because revision 3 documents that an error code and message are returned without specifying the encoding. Device types are validated against Appendix A, `Max Pos` is derived from the configured type, `SCCP` is refused for focusers that must home, and both motion bounds are clamped. A command journal (`INDIGO_OPTECFL_EVENTS`) and one-shot fault injection (`INDIGO_OPTECFL_FAULT`, actions `close`, `silent`, `partial`, `overlong`, `noack`, `reply=`, `error`, `position=`, `temp=`) were added, plus `--profile normal|split|nohome|noprobe`.

The `syncable` profile starts with a non-Optec `SO` type so driver tests can verify non-default `Dev Typ` readback without first writing the property. The simulator is a self-contained translation unit, so it was compiled and exercised directly. It compiles cleanly, and a direct protocol probe over its pseudo-terminal passes 68 assertions covering every block field, every command, parameter validation, error replies, the homing-type `SCCP` refusal, elapsed-time motion progress and journal capture. This validates the simulator against the document; driver validation is recorded separately below.

### Steps 3–5 — driver refactoring

Complete and build-verified. Version raised to 0x02000002.

Lifecycle now runs entirely on handler queues. `indigo_set_timer`, `indigo_reschedule_timer`, `indigo_cancel_timer_sync`, both `indigo_timer *` slots and the `pthread_mutex_t` are gone; connection changes dispatch through `indigo_execute_handler`, polling reschedules through `indigo_execute_handler_in` and teardown uses `indigo_cancel_pending_handlers`. Focuser #2 is linked to focuser #1 through `master_device`, so a single queue serializes every access to the shared serial handle and the former mutex has no remaining purpose. This removes the mutex/`cancel_timer_sync` deadlock (D5) and the double `pthread_mutex_destroy` (D18) by construction. `optecfl_open` is transactional: it opens the port only when no sibling holds it, rolls back exactly what its own attempt acquired, and increments the shared reference count only after full success; `optecfl_close` releases the handle on the last disconnect. The status poll no longer touches the reference count (D2) and a failed hub handshake no longer tears down a sibling session (D3).

Transport moved to `indigo_uni_io`. Requests go through a variadic helper forwarding `va_list` to `indigo_uni_vprintf`, preceded by `indigo_uni_discard`; replies are read with `indigo_uni_read_section2` using a one-second first-byte and a 100 ms inter-byte timeout, reserving capacity for the terminating NUL and validating that the line is complete and NUL-free before the terminator is stripped. One response buffer lives in private data and is reused across serialized transactions. Block replies are parsed through a shared reader bounded by a line budget, which removes the unbounded hub-info loop (D9). The acknowledgement `!` and each payload token are now checked explicitly, so the `strncpy`/`strncmp` defect (D1) cannot recur; `INDIGO_COPY_VALUE` replaced the unterminated firmware copy (D13), and the divergent `sscanf` formats were replaced by one key/value splitter with trimming (D11, D12).

Protocol correctness: the configuration parser matches `Dev Typ` (D4); the device-type table was rebuilt from Appendix A, dropping the invented `FD` and `RA`, adding the missing `SP` and `ZZ` and correcting seven labels (D6); an unselected type is rejected before formatting (D7); SYNC is refused for device types that must home and publishes the property it actually changed, and a type change re-reads the configuration because `Max Pos` follows the type (D8); polling no longer depends on the hidden, unimplemented `FOCUSER_MODE` (D14); and change dispatch uses `INDIGO_COPY_TARGETS_PROCESS_CHANGE` for position, `INDIGO_COPY_VALUES_PROCESS_CHANGE` for steps and type and `INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME` for abort, so the framework BUSY guard now rejects overlapping motion (D16). `FOCUSER_LIMITS` is exposed because the controller reports the maximum position. `FOCUSER_REVERSE_MOTION` remains driver-resolved, since revision 3 has no reverse command.

The driver compiles clean under `-fsyntax-only` with `-Wall -Wextra -Wconversion` against the INDIGO headers in the agent environment. It has not been linked or executed there; see the acceptance limits.

### Step 6 — test coverage

Complete and executed. `indigo_test/integration/test_focuser_optecfl_simulator.c` grew from two smoke scenarios to 44 fork-isolated scenarios, each against a freshly started simulator with its own journal and fault file, following the harness pattern established by the Prodigy suite including per-property revision tracking so a stale OK state cannot satisfy a wait. The connection helpers wait for a fresh final `CONNECTION=OK` update, avoiding a race with connected-property definition. The non-default `syncable` simulator profile verifies `Dev Typ` readback independently of a local type change.

Coverage by class-standard area. Identity and lifecycle: driver metadata and interface bits, connect/disconnect for each logical focuser, both connection orders, repeated INIT/SHUTDOWN, and shutdown refused while a logical device is connected. Capabilities and readback: property contract before, during and after connection, the complete Appendix A table with explicit absence of the invented codes, non-default device-type readback on connect, and limits following the configured type. Motion: absolute GOTO with measured intermediate progress, no-op, both boundaries, relative steps inward and outward, driver-resolved reversal, clamping at zero, overlap rejection asserted through the command journal, and externally induced motion observed by polling. Sync: refused for a homing type with no command emitted, accepted for another type with no motion command emitted. Stop and failure: abort during motion, abort while idle, abort failure surfaced as ALERT, and a fresh move afterwards. Settings: device-type set and rejection with recovery. Transport and initialization faults: hub-info and configuration failures with file-descriptor accounting and successful reconnection, malformed, partial, overlong, missing-acknowledgement, silent and split replies, poll-failure recovery, disconnect during motion with polling proven to stop, and transport loss. Shared ownership: one hub open shared by both focusers, a failed second connection leaving its sibling fully operational, and handle release after sustained polling on both devices. Temperature: polling updates and the probe-absent idle state.

Three scenarios are deliberate regression tests for defects that the previous simulator could not reveal: `device_type_readback` for D4, `abort_failure_reported` for D1, and `handle_released_after_polling` for D2, with `sibling_survives_failed_connect` covering D3 and `sync_rejected_for_optec_type` covering D8.

Not covered, with reasons: temperature compensation, backlash, homing, centering, relative-move and nickname commands are not exercised because the driver does not implement them (D17); generic numeric range and step validation is the framework's responsibility per repository rules; and the unselected-device-type guard is unreachable through the bus because the property uses `INDIGO_ONE_OF_MANY_RULE`.

### Step 7 — integration

Complete. `indigo_test/Makefile` declares the simulator's dependency on `serial_motion.h` and provides a `test_focuser_optecfl_simulator_asan` target that instruments the production driver. `indigo_docs/PROPERTIES.md` was corrected: `FOCUSER_COMPENSATION` and `FOCUSER_MODE` were listed although they are unimplemented, and `FOCUSER_LIMITS` was missing; the driver-specific semantics of limits, reversal, speed and sync are now stated. `indigo_test/CHANGES.md` records the audit as complete with the scenario inventory, `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` describes the rebuilt simulator and its profiles and hooks, and `REFACTOR.md` is present in the driver's Xcode group. `MIGRATION_STATUS.md` records API 3, the explicitly non-generated driver, handler queues, simulator retesting and automated coverage.

### Verification status

The hand-written driver and simulator build successfully with the repository macOS toolchain. The complete 44-scenario simulator suite passes (`OPTECFL: 0 failing scenarios`). The same complete suite also passes when the production driver is compiled with AddressSanitizer. Leak detection is unavailable in Apple's AddressSanitizer runtime, so the ASan run covers address, bounds and lifetime instrumentation but not leak reporting. The expected error log lines during the run are deliberate hub/configuration/secondary-connect fault injections. Xcode project syntax was validated after adding `REFACTOR.md`; README was not modified. Hardware behavior and Windows execution remain unverified.
