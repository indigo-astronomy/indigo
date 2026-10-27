# Mount agent joystick forwarding regression (2026-10-19)

## Scope and audit

This is a focused correction of joystick RA/DEC forwarding, not a driver migration. The hand-written, platform-independent `indigo_agent_mount.c` uses the real filter client and bus to forward selected joystick properties to a selected mount. There is no `.driver` input or vendor protocol in this path. Public property names, counts, visibility, configuration and other agent operations remain unchanged. No new custom properties are introduced.

The joystick source declares NORTH/SOUTH and WEST/EAST, in that order, with AT_MOST_ONE semantics. The original agent sent each item as a separate request. `indigo_property_copy_values()` clears the destination vector for each request, so the trailing false item cancels NORTH or WEST. The opposite user-reported RA asymmetry is not established by this source audit. A1 overlay source and physical joystick/mount behavior are outside this checkout validation.

Existing coverage includes `test_aux_joystick_hid.c`, a deterministic fake Linux HID/event backend exercising the real joystick driver, and `test_agent_mount.c`, which supplies a simulated joystick and mount to the real agent/filter/bus. Its existing joystick case checks request arrival, not the resulting direction. The fake mount uses the production property-copy helper and thus reproduces the relevant AT_MOST_ONE semantics. No additional standalone joystick simulator is needed for this regression.

## Hardware and platform decision

User authorized non-interactive simulator testing, fixes and commits without push. No physical hardware testing is planned. Execution platform is macOS arm64; universal builds also compile x86_64. Linux, Windows, physical motion, DDHid delivery and A1 overlay behavior are not validated here. Guider pulse timing and serial motion simulation do not apply to this property-forwarding correction.

## Atomic plan and baseline

1. Complete: build the original agent and run its full existing suite. `make -C indigo_drivers/agent_mount -f ../../Makefile.drv all` passed. The first `make -C indigo_test test-agent-mount` could not link because the local libindigo archive lacked current `indigo_lock_bus`/`indigo_unlock_bus` symbols; `make -C indigo_libs all` restored the prerequisite and the full original suite passed 73/73.
2. Complete: extend the existing simulated joystick test with final RA/DEC readback, neutral/held/reversed motion, input item reordering and single-request assertions. The original suite joystick case passed; all three new cases failed as expected (1/4 joystick cases passed). Normalized trace below was captured before production edits.
3. Complete: forward each axis vector atomically, increment driver version from 3.0.0.27, and repeat the full agent suite. Property API and non-motion joystick handling are unchanged. The complete suite passed 76/76; final output now matches all four directions in both input orders, with one request per accepted update.
4. Complete: fake-HID 14/14 and final recorded agent 76/76 passed through `tools/run_driver_test.py`. Coverage counts, project registration, README test records and generated summary are synchronized. Diff and project-file checks passed; normal and sanitizer artifacts were removed with `make -C indigo_test test-clean` (and the sanitizer TEST_BUILD override). Joystick test records are in their own commit; this agent change is a separate commit unit.

## Found defects

- JOY-001 (reproduced on the original driver): per-item forwarding replaces the entire destination switch selection twice. Fixed: send recognized axis items together in one switch request. Regression: final direction and ordered request trace for both axes, both input orders and neutral/repeated/reversal sequences.
- JOY-002 (test audit): existing joystick assertions only prove that some request arrived. Fixed: assert final values and one request per axis update, preserving the simulated peers and actual bus semantics.

- JOY-003 (reproduced watchdog failure, lock-order audit): simulated peer completion and timer updates acquired `peer_mutex` before entering the bus, while bus-dispatched `peer_change` acquired them in the reverse order. A repeat normal run also timed out in `unpark waits for park property`. Fixed the three test/timer publication helpers to acquire the recursive bus lock first, as framework publication helpers do; no production synchronization changed. Full normal and sanitizer suites verify the change.
- JOY-004 (test-runner audit): record the selected case plan in the parent before forking so a child that fails before calling the runner remains counted; retain command-line and environment filtering.

## Original-driver trace

`make -C indigo_test build/integration/test_agent_mount` followed by `indigo_test/build/integration/test_agent_mount joystick`, original version 3.0.0.27. The tests log input state, ordered mount requests and final public values. Representative normalized trace (the full matrix also covers repeated hold, release and direct reversal):

```text
RA input WEST=1 EAST=0 OK -> request WEST=1 -> request EAST=0 -> WEST=0 EAST=0
RA input WEST=0 EAST=1 OK -> request WEST=0 -> request EAST=1 -> WEST=0 EAST=1
DEC input NORTH=1 SOUTH=0 OK -> request NORTH=1 -> request SOUTH=0 -> NORTH=0 SOUTH=0
DEC input NORTH=0 SOUTH=1 OK -> request NORTH=0 -> request SOUTH=1 -> NORTH=0 SOUTH=1
RA reordered input EAST=1 WEST=0 OK -> request EAST=1 -> request WEST=0 -> WEST=0 EAST=0
DEC reordered input SOUTH=1 NORTH=0 OK -> request SOUTH=1 -> request NORTH=0 -> NORTH=0 SOUTH=0
neutral -> two false-item requests -> both false
control disabled or input IDLE/BUSY/ALERT -> no request, previous mount values preserved
```

The intended trace difference is exactly one request containing both axis items per accepted update, preserving the requested final direction. No ordering change is made to non-motion joystick commands.

## Scenario-to-test mapping

- `joystick motion RA` / `joystick motion DEC`: neutral, both directions, repeated holds, release after either direction, direct reversals, reordered items, final mount and agent readback, exactly one request per update and independence of the other axis.
- `joystick motion gates`: disabled joystick control and IDLE/BUSY/ALERT updates preserve the previous mount values without requests; subsequent OK neutral recovers normally on both axes.
- Existing `joystick`: park, tracking, home, slew-rate, abort and disabled-control compatibility.
- All 73 existing cases remain in the full agent run, including lifecycle, selection, configuration, failures, concurrency, coupled motion and scripted LX200 transport.
- Existing fake-HID suite: axis/button mapping, axis swap, invalid input, lifecycle, hot-plug, multiple devices and failure recovery. `python3 tools/run_driver_test.py aux_joystick` passed 14/14; its record was committed separately.

No property was added or removed, so `PROPERTIES.md` needs no change. The new record is registered in the agent's Xcode group. `MIGRATION_STATUS.md` now reports the actual 76 / 0 cases, preserving its Comment column.

## Additional verification

- Universal production build passed; version is now 3.0.0.28 (higher than 3.0.0.27).
- Strict arm64 syntax check passed with `-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers`; the last suppression is for the existing static client initializer, outside the edited block.
- ASan/UBSan: `ASAN_OPTIONS=detect_leaks=0 make -C indigo_test TEST_BUILD=/tmp/agent_mount_joystick_sanitize MOUNT_TEST_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' test-agent-mount`. Agent, filter, base driver and test code are instrumented; the linked prebuilt library is not. The first run, concurrent with the normal full suite, passed 74/76: `unpark waits for park property` hit its watchdog and `slew target check` timed out waiting for ALERT. All joystick cases passed and no sanitizer memory/UB diagnostic occurred. Repeating the entire sanitizer suite alone passed 76/76. A later normal run repeated the unpark watchdog failure (75/76), leading to the concrete lock-order fix JOY-003. The isolated slew timeout was not reproduced in the separate repeat; no production slew behavior was changed. Leak detection was disabled for retained framework globals.
- After JOY-003, the complete ASan/UBSan suite passed 76/76 and the final recorded normal suite passed 76/76. Strict syntax checking of the test source also passed (same warning flags, plus `-Wno-unused-function` for shared test helpers).
- `plutil -lint indigo.xcodeproj/project.pbxproj` and `git diff --check` passed.

## Final test summary

- Final recorded simulator cases: agent 76 run / 76 passed; fake-HID joystick 14 run / 14 passed; total 90 run / 90 passed.
- Final separate ASan/UBSan agent run: 76 run / 76 passed (the initial 74/76 attempt is documented above).
- Physical-hardware tests: 0 run / 0 passed.
- Final record commands: `python3 tools/run_driver_test.py agent_mount` and `python3 tools/run_driver_test.py aux_joystick`.
- A1 overlay and the opposite reported east/west asymmetry remain unverified. This reproducer establishes cancellation of NORTH/WEST with the driver's current item order.
