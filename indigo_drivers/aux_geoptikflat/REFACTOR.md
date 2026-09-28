# Geoptik Flat Field Generator Coverage Completion

## Current-State Audit

- Audit baseline: branch `refactoring` at commit `e4ff13e70`, macOS Darwin 25.6.0, universal arm64/x86_64 artifacts, arm64 execution.
- `indigo_aux_geoptikflat.driver` is the authoritative generator input; `indigo_aux_geoptikflat.c`, `.h` and `_main.c` are checked-in generated outputs. The driver is API 3, uses portable `indigo_uni_io`, generator-owned serialized handlers and one serial handle per logical AUX instance. There are no timers, long-running operations or shared logical devices.
- The device exposes `INDIGO_INTERFACE_AUX_LIGHTBOX`, runtime additional instances, persistent `AUX_LIGHT_SWITCH` (`ON`, `OFF`) and persistent `AUX_LIGHT_INTENSITY` (`LIGHT_INTENSITY`, 0-100%, step 1, default 50).
- Protocol reference: the Geoptik generator speaks the Alnitak generic command set documented in `../aux_flipflat/Alnitak_GenericCommandsR4.pdf`. Requests are `">XOOO"` terminated by CR, replies are `"*Xiizzz"` terminated by LF alone, `ii` is the two digit product id. The driver sends `>POOO`, `>VOOO`, `>Bxxx`, `>LOOO` and `>DOOO`; the documented `>JOOO` and `>SOOO` readbacks are unused.
- Before this work the driver had **no simulator and no test file of its own**. Its only hardware-free coverage was `indigo_test/integration/test_serial_outputs.c`, a source shared with `guider_cgusbst4` and `aux_rts` and parameterized by `TEST_KIND`. That fake answered every command with the single fixed string `"*V1.0"`, so it could not exercise the real framing, the product id, the firmware payload or any reply that does not belong to the command that was sent.

## Defects Found and Fixed (driver version 7 -> 8)

1. **Firmware version read from the wrong offset.** `geoptikflat_open()` copied `PRIVATE_DATA->response + 2` into `INFO_DEVICE_FW_REVISION_ITEM`, which is `iivvv` - the product id concatenated with the version. `"*V19003"` was reported as firmware `19003`. The documented payload starts at offset 4, which is also what the hardware-tested 2.0 driver used (`sscanf(response + 4, "%s", firmware)` in `f298ca3b0`). Fixed to `response + 4`, with a `strlen(response) > 4` guard so a truncated `*Vii` reply fails the handshake instead of reporting an empty firmware.
2. **Any reply starting with `*` was accepted as the answer to any command.** `geoptikflat_command()` only checked `response[0] == '*'`, so a foreign or stale reply such as `*S19000` satisfied a `>B127` request and the driver reported `OK` for a command the device never acknowledged. The 2.0 driver checked the echoed operation per command (`strncmp(response, "*B", 2)` and friends); the refactoring dropped it. The helper now also requires `response[1] == command[1]`.
3. **Light-switch command built with a runtime `%c`.** `">%cOOO"` made the expected echo unavailable to the command helper. Both call sites now pass the literal `">LOOO"` / `">DOOO"`, so the echoed operation is always `command[1]`.
4. **Driver label typo.** `label = "Geoptik flat field generato"` was missing its final letter; it reaches users through the driver description and, after a successful handshake, through `INFO_DEVICE_MODEL`. Corrected to `"Geoptik flat field generator"`, which is what `README.md` has always said.

Defects 1-3 were confirmed against version 7 before the fix: with the pre-fix driver the new suite fails `identity_inventory_and_handshake`, `firmware_is_read_from_the_documented_offset`, `unmatched_replies_fail_the_request`, `a_stale_reply_is_discarded_before_the_next_command` and `handshake_failures_leave_the_device_disconnected`, and passes them after it.

## Test Work

- Added `aux_geoptikflat_simulator/aux_geoptikflat_simulator.c`, a standalone Alnitak-conformant PTY simulator implementing exactly the five commands the driver sends, with the documented reply framing (`LF` only, no `CR`), a configurable product id and firmware version, an append-only `<ready-file>.events` command log and a one-shot `<ready-file>.control` fault channel with the actions `drop`, `short`, `malformed`, `mismatch`, `stale` and `close`, each aimed at one operation letter or at `*`.
- Added `indigo_test/integration/test_aux_geoptikflat_simulator.c` with ten cases (see the mapping below), registered in `INTEGRATION_TESTS` together with an ASan/UBSan variant and in the Xcode project.
- Kept `test_aux_geoptikflat_transport`, which replaces the `indigo_uni_io` entry points and is the only place that can assert open/close balance and injected write failures at the transport boundary. Its fake now echoes the operation that was sent (`"*X19000"`) instead of the fixed `"*V1.0"`, because the fixed driver correctly rejects a reply that does not match the request. The `TEST_KIND == 1` and `TEST_KIND == 3` behavior of the shared source is unchanged and both other drivers still pass.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario |
| --- | --- |
| Driver metadata, API generation, interface bit, property inventory before and after connection, property/item type, permission, rule, range, default and format | `identity_inventory_and_handshake` |
| Exact handshake order `>POOO`, `>VOOO`, then the attach-time state as `>B127`, `>DOOO`; `INFO` model and firmware | `identity_inventory_and_handshake` |
| Firmware payload position in `*Viivvv`, independence from the product id, rejection of a truncated `*Vii`, identity cleared on disconnect | `firmware_is_read_from_the_documented_offset` |
| Percent to 0-255 conversion at 0/25/50/75/100 and the exact `>Bxxx` text | `intensity_maps_percent_to_the_device_scale` |
| `>LOOO` / `>DOOO` and the resulting one-of-many switch state | `light_switch_sends_the_documented_commands` |
| Dropped, non-`*` and wrong-operation replies fail the request; the next request recovers | `unmatched_replies_fail_the_request` |
| An unsolicited line ahead of the real reply fails the request and is drained by the discard before the next command | `a_stale_reply_is_discarded_before_the_next_command` |
| `CONFIG SAVE` / `CONFIG LOAD` of both persistent properties, the restore reaching the device, and the restored state pushed again by the next connection | `configuration_roundtrip_reaches_the_device` |
| Failed serial open, `>POOO` and `>VOOO` handshake failures, rollback with connected properties never published, later connection succeeds | `handshake_failures_leave_the_device_disconnected` |
| Transport loss reported on the request that discovers it, reconnect against a fresh device | `transport_loss_is_reported_and_reconnect_recovers` |
| Repeated disconnect tolerated, connected properties deleted, `INDIGO_DRIVER_SHUTDOWN` refused while connected | `repeated_disconnect_and_refused_shutdown` |
| Serial handle open/close balance and injected transport write failure | `test_aux_geoptikflat_transport` |

### Non-applicable and Deferred Coverage

- Motion, abort, polling, finalizers, guider timing and shared-interface ordering are not applicable; the driver exposes two immediate serialized controls on one AUX device and has no background callbacks.
- Hot-plug is not applicable; the driver has no USB discovery and publishes one device plus runtime additional instances.
- **Deferred, hardware-gated.** The driver still accepts a reply whose product id is not numeric and does not compare the echoed brightness in `*Biixxx` with the value it sent. `aux_arteskyflat` validates both. Adding the same checks here would reject a device that pads or echoes differently, and no physical Geoptik generator is available to confirm the exact bytes, so the stricter parse is left open.
- **Deferred.** A failed `AUX_LIGHT_INTENSITY` change leaves the rejected percentage in the property while the state goes `ALERT`; `aux_arteskyflat` keeps the last accepted value through `preserve_values`. Changing it here alters published values on a driver with no hardware available for confirmation; the suite asserts the `ALERT` state and the device-side command stream instead of the rejected value.
- Physical brightness, illumination uniformity, real USB interruption and Windows runtime require environments not available for this work.

## Hardware-Test Decision

No physical Geoptik flat field generator is available. No hardware run was performed and none of the results below are presented as verification of real panel behavior.

## Validation Evidence

- `indigo_generator indigo_aux_geoptikflat.driver` run twice leaves the generated `.c`, `.h` and `_main.c` unchanged.
- Strict driver build: `make -B -f ../../Makefile.drv CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed for generated source, archive, dynamic library and executable.
- Strict test build: `make -B build/integration/aux_geoptikflat_simulator build/integration/test_aux_geoptikflat_simulator CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed.
- `build/integration/test_aux_geoptikflat_simulator` passed 10 of 10 cases; `build/integration/test_aux_geoptikflat_transport` passed 2 of 2.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_geoptikflat_simulator_asan` passed all 10 cases with the production driver source instrumented and no ASan/UBSan report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- `test_aux_rts_transport` and `test_guider_cgusbst4_transport` still pass after the shared `test_serial_outputs.c` change.
- `plutil -lint indigo.xcodeproj/project.pbxproj` passed.

## Final Test Summary

- Simulated tests: **12 run, 12 passed** (10 simulator, 2 transport), plus the same 10 simulator cases under ASan/UBSan.
- Hardware tests: **0 run, 0 passed**; no compatible physical device is available.
