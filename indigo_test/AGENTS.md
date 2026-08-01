# Automated Test Guidelines

## Scope

This directory contains the automated, hardware-free INDIGO test suite. Keep this guide focused on tests only. General repository coding rules, driver conventions, and build-system guidance live in the repository root `AGENTS.md`.

## Test Layout

- `Makefile` defines all automated test targets.
- `test_runner.h` provides the local dependency-free C test runner and assertion macros.
- `unit/` contains fast deterministic tests for library helpers, property helpers, protocol adapters/parsers, tokens, RAW helpers, math helpers, and similar code.
- `integration/` contains in-process bus and simulator-driver tests.
- `integration/simulator_test_common.h` contains shared simulator lifecycle helpers, the in-process client, property cache, wait helpers, and compliance-style assertions.
- `fixtures/protocol/` contains fixed XML and JSON protocol parser fixtures.
- `CHANGES.md` summarizes current coverage and deferred automated-test work.

## Running Tests

Run `make all` from the repository root first if `build/lib/libindigo` or the required simulator driver archives are missing.

- `make -C indigo_test test` runs all automated unit and integration tests.
- `make -C indigo_test test-unit` runs unit tests only.
- `make -C indigo_test test-integration` runs bus and simulator-driver integration tests only.
- `make -C indigo_test test-clean` removes generated `indigo_test/build` binaries and dSYM files.

After validating changes that build tests, run `make -C indigo_test test-clean` unless the user asks to keep build artifacts.

## Harness Conventions

- Use `test_runner.h` for new C tests.
- Keep tests dependency-free unless a dependency is explicitly approved and compatible with the project license.
- Each test executable must return `0` on success and non-zero on failure.
- Prefer small named test functions registered in the `indigo_test_case tests[]` array.
- Use the existing assertion macros, adding new macros only when they improve clarity across multiple tests.
- Print enough context before assertions when a helper failure would otherwise hide the property, item, fixture, or device under test.
- Avoid long sleeps. Use bounded polling helpers for asynchronous bus or simulator behavior.
- Never change any file outside indigo_test folder.

## Unit Test Rules

- Unit tests should be deterministic and hardware-free.
- Link against the built INDIGO library instead of including production `.c` files directly.
- Use fixed expected values and stable fixtures.
- Keep protocol tests fixture-driven when parsing serialized input.
- Put new protocol fixtures under `fixtures/protocol/` and keep them minimal.
- Avoid environment-dependent tests unless the environment is fully controlled by the test.

## Integration Test Rules

- Integration tests must stay hardware-free.
- Exercise public bus APIs and public driver entry points.
- Do not launch `indigo_server` in the normal integration target.
- Do not open network sockets in the normal integration target.
- Simulator tests should validate driver metadata, `INDIGO_DRIVER_INIT`, property enumeration, connection/disconnection, representative property changes, and `INDIGO_DRIVER_SHUTDOWN`.
- Prefer one integration executable per simulator driver archive.
- For multi-device simulator drivers, cover **every** exposed logical device in the relevant simulator test file, one registered test case per device (see "Multi-Device Serial Drivers" below).
- Always cleanly disconnect simulator devices before shutdown.

## Simulator Compliance

- Use `simulator_test_common.h` for simulator compliance checks.
- Use `DRIVER_TESTING_RULES.md` for all simulator devices.

## Makefile Rules

- Add new unit executables to `UNIT_TESTS`.
- Add new integration executables to `INTEGRATION_TESTS`.
- Keep test-specific linker flags local and explicit.
- Do not add generated binaries or `build/` artifacts to the repository.
- Keep `test-clean` able to remove all generated test outputs.

## Wiring a Host-Side Serial Simulator Into the Test Build

When a serial `.ino` sketch is refactored into a host-side pseudo-terminal
simulator (see `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`), the simulator source
lives beside the sketch under `indigo_drivers/`, but its build and test targets
are added here. Adding one simulator requires exactly four `Makefile` edits.
Use an existing entry such as `aux_wcv4ec` (threaded) or `aux_arteskyflat`
(non-threaded) as the copy source.

1. Add the test executable to the `INTEGRATION_TESTS` list:

   ```make
   	$(INTEGRATION_BUILD)/test_<class>_<device>_simulator \
   ```

2. Add a local, explicit linker-flags variable naming the driver archive:

   ```make
   <CLASS>_<DEVICE>_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_<class>_<device>.a $(TEST_LDFLAGS) -lindigocat
   ```

3. Add a rule that compiles the simulator binary from the source under
   `indigo_drivers/`. Include `-pthread` **only if** the simulator uses threads
   (for example a background status stream); omit it for simple
   request/response simulators:

   ```make
   $(INTEGRATION_BUILD)/<class>_<device>_simulator: ../indigo_drivers/<class>_<device>/<class>_<device>_simulator/<class>_<device>_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
   	$(CC) $(TEST_CFLAGS) -o $@ $<
   ```

4. Add a rule that links the test executable against the driver archive and
   depends on the simulator binary plus the shared test headers:

   ```make
   $(INTEGRATION_BUILD)/test_<class>_<device>_simulator: integration/test_<class>_<device>_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/<class>_<device>_simulator | $(INTEGRATION_BUILD)
   	$(CC) $(TEST_CFLAGS) -o $@ $< $(<CLASS>_<DEVICE>_SIMULATOR_TEST_LDFLAGS)
   ```

## Serial Simulator Integration Test Skeleton

Each simulator gets one `integration/test_<class>_<device>_simulator.c`. Copy
the closest existing test in the same device class and adjust the driver entry,
names, and exercised properties. The shape is fixed:

```c
#include <indigo_drivers/<class>_<device>/indigo_<class>_<device>.h>

#include "serial_simulator_test_common.h"

#ifndef <CLASS>_<DEVICE>_SIMULATOR_EXECUTABLE
#define <CLASS>_<DEVICE>_SIMULATOR_EXECUTABLE "build/integration/<class>_<device>_simulator"
#endif

// label, driver_name, device_name, driver entry, multi_device_support,
// then four {list, count} pairs for base / hidden-base / connected /
// hidden-connected properties (NULL/0 when the class needs no overrides).
static const simulator_driver_case <device>_case = {
	"<Human Label>",
	"indigo_<class>_<device>",
	"<Device Name>",
	indigo_<class>_<device>,
	false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

static void <device>_passes_serial_compliance_checks(void) {
	external_serial_simulator simulator = { 0 };

	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, <CLASS>_<DEVICE>_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&<device>_case, simulator.port));
	SERIAL_CHECK_TRUE(context.connected && context.last_connection_state == INDIGO_OK_STATE);

	assert_device_interface(INDIGO_INTERFACE_<CLASS>);
	assert_serial_<class>_class_property_completeness();
	// assert_property_has_item(...) for each driver-defined property/item.
	// Drive representative changes with indigo_change_*_property_1(...) and
	// confirm each with wait_for_property_state(name, INDIGO_OK_STATE).

cleanup:
	if (context.connected) {
		stop_serial_driver(&<device>_case);
	}
	stop_external_serial_simulator(&simulator);
}

int main(void) {
	const indigo_test_case tests[] = {
		{ "<device>_passes_serial_compliance_checks", <device>_passes_serial_compliance_checks }
	};
	return indigo_run_tests("<Device Name> serial simulator integration tests", tests, ARRAY_SIZE(tests));
}
```

Notes:

- The simulator is launched with `--headless --ready-file` automatically by
  `start_external_serial_simulator`; do not add those arguments in the test.
  Use `start_external_serial_simulator_with_args` to pass extra flags such as
  `--model` or `--device-id`.
- `SERIAL_CHECK_TRUE`/`SERIAL_CHECK_EQ_INT` jump to the `cleanup:` label on
  failure, so every test body must define one and always tear down the
  simulator and driver there.
- `assert_serial_aux_class_property_completeness()` is intentionally empty; AUX
  has no base-class properties, so assert the concrete driver-defined
  properties directly.

## Multi-Device Serial Drivers

Some serial drivers expose more than one logical device from a single driver
archive (for example AO + guider, AUX + focuser, or mount + guider). Every
exposed device must be exercised by the compliance test — do not test only the
primary device.

Give each device its own `simulator_driver_case` and its own registered test
case, each running an independent driver lifecycle over a fresh simulator (the
same shape as `test_focuser_optecfl_simulator.c`, which covers two focusers).
`serial_simulator_test_common.h` provides the connect/teardown helpers:

- Primary/master device — the one that owns the serial connection and exposes a
  visible `DEVICE_PORT` — uses the normal `start_serial_driver(&case, port)` /
  `stop_serial_driver(&case)` pair.
- A secondary device that **shares** the master's connection usually has a
  hidden `DEVICE_PORT` and opens the port through its master, so it cannot be
  connected stand-alone. Connect it with
  `start_shared_serial_device(&secondary_case, master_case.device_name, port)`,
  which brings the driver up, points the master device's `DEVICE_PORT` at the
  simulator, then connects the secondary. Tear it down with the normal
  `stop_serial_driver(&secondary_case)`.

Testing each device in a separate lifecycle keeps the single-device property
cache in `simulator_test_common.h` valid (it tracks one device at a time) and
avoids re-enumeration races. The granular helpers `bring_up_serial_driver`,
`connect_serial_device`, `disconnect_serial_device`, and
`tear_down_serial_driver` exist for unusual cases but are rarely needed
directly.

To tell whether a driver is multi-device, count `indigo_attach_device(` calls
in its `indigo_<name>.c`; more than one means multiple logical devices. The
master is the device that unhides `DEVICE_PORT` (`DEVICE_PORT_PROPERTY->hidden
= false`); the others open via `device->master_device`.

## Documentation

- Update `CHANGES.md` when adding meaningful new test coverage or deferring known work.
- Update `DRIVER_TESTING_RULES.md` when base driver property visibility, simulator compliance coverage, or driver-class test scenarios change.
