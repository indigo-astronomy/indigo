# INDIGO Driver Testing Basics

Revision: 26.09.2026 (draft)

Author: **Peter Polakovic**

e-mail: *peter.polakovic@cloudmakers.eu*

Co-authored by: **Claude** (Anthropic, Claude Opus 5.5)

## Overview

This document explains how INDIGO drivers, and the framework code they depend on, are tested: which kinds of tests exist, where they live, how to write a new one, how to run them, and what a new generated driver needs before it counts as tested. It describes what is in the repository at the revision of this document. It does not replace the rule files. When they differ, the rule files win:

* [AGENTS.md](../AGENTS.md), section *Driver Testing*: the four run modes, commit units and how runs are recorded.
* [indigo_test/AGENTS.md](../indigo_test/AGENTS.md): test layout, harness conventions, Makefile wiring, configuration isolation, parallel runners, hardware hosts and the `README.md` test record format.
* [indigo_test/DRIVER_TESTING_RULES.md](../indigo_test/DRIVER_TESTING_RULES.md): the acceptance checklist for each driver class (CCD, wheel, focuser, guider, AO, GPS, rotator, dome, mount, polar aligner, AUX).
* [indigo_drivers/AGENTS.override.md](../indigo_drivers/AGENTS.override.md): the refactoring workflow (`REFACTOR.md`, baseline, reference traces, MountSim).
* [SERIAL_DEVICE_SIMULATORS.md](SERIAL_DEVICE_SIMULATORS.md): the contract for host-side serial simulators.

---

## Kinds of Tests

| Kind | Location | Backend | In `make test` |
| --- | --- | --- | --- |
| Unit | `indigo_test/unit/` | none; library code only | yes (`test-unit`) |
| Bus and framework integration | `indigo_test/integration/test_bus_lifecycle.c`, `test_ccd_countdown.c`, `test_detach_abort.c`, ... | in-process bus | yes |
| Virtual simulator driver | `indigo_test/integration/test_<class>_simulator.c` | the INDIGO simulator driver itself (`mount_simulator`, `ccd_simulator`, ...) | yes |
| Serial protocol simulator | `indigo_test/integration/test_<driver>_simulator.c` | host-side pseudo-terminal simulator process | yes |
| Fake SDK, USB, HID or sysfs | `indigo_test/integration/test_<driver>_{sdk,usb,hid,sysfs}.c` | fake vendor API linked in place of the real one | yes (sysfs cases on Linux only) |
| Transport and motion | `test_<driver>_{transport,motion}.c` | scripted transport or simulator | yes |
| Agents | `indigo_test/integration/test_agent_*.c` | real agent code, simulator drivers, stubbed I/O | yes |
| Generator | `indigo_test/integration/test_generator_architecture.c` | `build/bin/indigo_generator` | yes |
| Opt-in network variants | `--tcp` / `--network` modes of some simulator suites, `test_detach_abort --network` | loopback sockets | no |
| Sanitizer variants | `*_sanitize` / `*_asan` targets | ASan/UBSan build of the driver source | no |
| MountSim acceptance | `indigo_test/mountsim/` | MountSim macOS application | no, macOS only |
| Physical hardware | `indigo_test/hardware/test_<driver>_hw.c` | the real device | no |
| Benchmarks | `indigo_test/benchmark/` and `-D*_TIMING_BENCHMARK` builds | any | no (`make benchmark`) |
| Shell compliance scripts | `indigo_tests/*_compliance.sh` | a running `indigo_server`, via `indigo_prop_tool` | no |
| Manual testing | `indigo_server` with drivers, a client or `indigo_prop_tool` | simulator or hardware | no |

`indigo_test/` (singular) contains the automated C suite. `indigo_tests/` (plural) is a separate bash framework described in [End-to-End Tests With indigo_server](#end-to-end-tests-with-indigo_server).

### Unit tests

Unit tests are fast, deterministic and hardware-free. They cover library helpers such as tokens, base64, MD5, timers, property helpers, the XML and JSON protocol parsers, RAW images, alignment and dome math, `indigo_uni_io` and the driver loader. They link against the built `libindigo` and never include production `.c` files. Parser tests read small fixed inputs from `indigo_test/fixtures/protocol/`.

### Simulator-driver tests

The in-process integration tests start the bus with `indigo_start()`, attach an in-process client, call the driver entry point (`indigo_mount_simulator(INDIGO_DRIVER_INIT, NULL)`), and then drive the driver only through public bus requests such as `indigo_change_switch_property_1()`. The same shape is used for the virtual simulator drivers (`test_mount_simulator.c`, `test_ccd_simulator.c`, `test_dome_simulator.c`, `test_gps_simulator.c`, `test_rotator_simulator.c`, `test_polaralign_simulator.c`) and for every real driver that has a hardware-free backend. The normal integration target never launches `indigo_server` and never opens network sockets.

### Serial protocol simulators

A serial driver is tested against a standalone simulator process that emulates the device's protocol on a pseudo terminal. The driver connects through its normal `DEVICE_PORT` path, so its real I/O, parsing and timing run unchanged. The contract (`--headless`, `--ready-file`, the `INDIGO_SIMULATOR_PORT` hand-off, signal handling, non-blocking reads) is in [SERIAL_DEVICE_SIMULATORS.md](SERIAL_DEVICE_SIMULATORS.md).

* **Source:** the simulator lives next to the driver, in `indigo_drivers/<class>_<device>/<class>_<device>_simulator/<class>_<device>_simulator.c`, for example `indigo_drivers/focuser_askar/focuser_askar_simulator/`. An `.ino` firmware sketch, where one exists, sits in the same directory.
* **Shared code:** `indigo_test/simulator_common/serial_simulator_common.h` (PTY creation, ready file, tracing, full writes), `serial_motion.h` (elapsed-time motion, stop and sync; use it for every simulator with motion) and `aux_simulator_common.h`.
* **Executables:** built by `indigo_test/Makefile` into `indigo_test/build/integration/<class>_<device>_simulator`. The test source compiles in a default path relative to `indigo_test/` (`#define ..._SIMULATOR_EXECUTABLE "build/integration/..."`), so run serial simulator tests from the `indigo_test` directory.
* **Variants:** one simulator may emulate several models or dialects through `--model`. For example the LX200 simulator serves `indigo_mount_lx200` profiles and, through its `asi` profile, `indigo_mount_asi`. With `--tcp` it listens on an ephemeral `127.0.0.1` port and publishes an `lx200://127.0.0.1:<port>` URL in the ready file instead of a PTY path.
* **Traces:** simulators write a command event log next to the ready file. Some suites compare it with checked-in reference traces in `indigo_test/fixtures/<driver>/`, such as `original_reference_trace.txt` and `generated_reference_trace.txt`.

Firmware-only `.ino` sketches, network simulators such as `relio_simulator.pl`, and the INDIGO-native `*_simulator` drivers are not PTY simulators. See *Out of Scope* in [SERIAL_DEVICE_SIMULATORS.md](SERIAL_DEVICE_SIMULATORS.md).

### Fake SDK, USB, HID and sysfs tests

Drivers that talk to a vendor SDK, libusb, hidapi or sysfs are tested against a fake at that boundary. The Makefile compiles the production driver source into a test object and redirects the boundary symbols with `-D` replacements, for example `-Dlibusb_hotplug_register_callback=eaf_test_usb_register -Dindigo_attach_device=eaf_test_attach` for `test_focuser_asi_sdk`. The test file then implements the replacement functions as a scriptable fake. Larger fakes are shared headers or sources in `indigo_test/integration/`, for example `fli_fake_sdk.h`, `qsi_fake_sdk.{h,cpp}` and `sysfs_gpio_fake.h`. `indigo_test/fakes/` holds only substitute system headers (`fakes/linux/joystick.h`, `fakes/malloc.h`). A fake must model the real SDK or device, including quirks observed on hardware, as required by *Simulators and Fake SDKs Model the Real Thing* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md).

### Agent tests

`test_agent_imager`, `test_agent_guider`, `test_agent_mount`, `test_agent_config`, `test_agent_astrometry`, `test_agent_scripting`, `test_agent_scripting_sequencer`, `test_agent_imager_guider_mount` and `test_agent_imager_instances` build the real agent source, plus `indigo_driver.c` and `indigo_filter.c` where needed, into test objects and link them with simulator driver archives. Time, sleeps, sockets or file replacement are redirected with `-D` where a scenario needs it. For example `test_agent_mount` replaces `time`, `indigo_usleep` and the `indigo_uni_*` socket calls with a scripted LX200 transport, so no socket is opened. These tests are the place to check how drivers and agents cooperate.

### Client-detach motion release

`test_detach_abort` covers the detach-abort registry (`indigo_register_detach_abort()` / `indigo_unregister_detach_abort()`) and the mount manual-motion ownership built on it (`indigo_mount_record_motion_client()` / `indigo_mount_commit_motion_client()`, see *Manual Motion and Client Detach* in [DRIVER_GENERATOR_BASICS.md](DRIVER_GENERATOR_BASICS.md)). It links the generated `mount_simulator` and `mount_lx200` drivers and the Mount Agent, and runs the LX200 driver against its serial simulator (OnStep and classic profiles).

* **Default mode** (part of `test-integration`, also `make -C indigo_test test-detach-abort`): everything runs on one in-process bus and the owners are extra in-process clients that detach. `registry_*` cases exercise the bus API against a minimal test device (release sent with the device's access token and a `NULL` client, refusal without an attached owner, replacement, unregister, full registry, device detach, re-entrant release handler). `simulator_*`, `agent_*` and `lx200_*` cases start manual motion from an owner and check the outcome of its detach: orderly release, no release after an own release, a `MOUNT_ABORT_MOTION` or a release by another client, takeover by another client, device disconnect and driver shutdown, a finite goto that keeps running, the parked-mount guard refusing a request with and without a registered motion, both axes, a direction change, a detach before the handler's commit, a detach before the commit followed by the attachment of a client at the same address (which must not adopt the motion; the registry-level `registry_stale_attachment_is_refused` checks the same for a stale `indigo_client_ref`), a repeated detach race, a device locked by an access token, motion requested through the Mount Agent, a detach during LX200 serial I/O and the classic LX200 "Classic guiding is active" refusal.
* **Network mode** (`make -C indigo_test test-detach-abort-network`, opt-in because it opens loopback sockets): the same bus also runs the production TCP server (`indigo_server_start()`) on an ephemeral port, and the owners are real XML peers that close orderly, reset the connection (`SO_LINGER` 0), lose it through an in-process cuttable proxy while the peer stays alive, own motion through the Mount Agent, race the close with the request, or drive the LX200 driver. The reuse of a freed XML adapter's address by the next connection depends on the allocator and cannot be forced from the peer side, so the reused-address case is covered deterministically only in default mode, where the same `indigo_client` is detached and attached again at the same address.
* **Observation without sleeps:** the log handler is redirected and the cases count the markers the bus and the mount base class log (`Aborting '<device>'.<property> started by detached client ...`, `Releasing '<device>'.<property>, ...`, `Forgetting ...`). A fence queued on the device's handler queue proves that a motion handler and its commit have finished, a gate occupying the queue admits a request before its handler runs, and `indigo_unregister_detach_abort()` returning `INDIGO_NOT_FOUND` is the final probe that no entry is left. Race cases accept both "registered, then released on detach" and "refused by the commit, released by the handler" but require exactly one release and the end of the motion. `INDIGO_TEST_DEBUG=1` prints the log and `INDIGO_TEST_TRACE=1` the bus traffic.
* Only `mount_simulator`, `mount_lx200` and `agent_mount` are exercised at run time. The other generated mount drivers (`mount_ioptron`, `mount_nexstar`, `mount_nexstaraux`, `mount_pmc8`, `mount_rainbow`, `mount_starbook`, `mount_synscan`, `mount_temma`) get the same generated record and commit calls, which their own suites compile and run, but no test detaches the owner of a running motion on them (see [Known Gaps](#known-gaps)).

### Opt-in variants: network, sanitizers, MountSim, hardware

* **Network:** targets such as `test-mount-lx200-tcp`, `test-mount-ioptron-tcp`, `test-detach-abort-network` and `test-dome-<name>-simulator-network` run the `--tcp` or `--network` mode of a suite. Some bind fixed loopback ports; `test-dome-baader-simulator-network` uses 8080, for example. They are not part of `test-integration`.
* **Sanitizers:** many suites have a `$(INTEGRATION_BUILD)/test_<driver>_simulator_sanitize` (or `_asan`) build that compiles the driver source with `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer`. A `test-<driver>-sanitize` phony target runs it with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`. Coverage is per driver, not global; check `indigo_test/Makefile` for the driver you are working on. Several sanitize rules hard-code `-arch arm64`, so they target Apple Silicon as written. Valgrind is not used or wired anywhere in the repository.
* **MountSim:** `make -C indigo_test test-mount-<driver>-mountsim` (temma, rainbow, ioptron, lx200, nexstar, synscan) drives the real driver against the MountSim macOS application through `mountsim/run_mountsim.py`. It needs macOS, a GUI session and a built MountSim (`MOUNTSIM_APP`). See [MountSim harness usage](../indigo_test/mountsim/USAGE.md) and *Testing mounts with MountSim* in [indigo_drivers/AGENTS.override.md](../indigo_drivers/AGENTS.override.md).
* **Hardware:** `make -C indigo_test test-<driver>-hw` builds `indigo_test/hardware/test_<driver>_hw.c` on top of `hardware/hardware_test_common.h`. The binaries refuse to run without `--run`, which the targets pass. Ports and options come from per-driver environment variables documented above each target, such as `MOUNT_PMC8_HW_PORT`, `HW_DEBUG=1`, `HW_PARK=1` and `HW_TRACE=1`. Mounts and focusers move during these tests. A hardware run is started only when explicitly requested, in one of the four modes defined in [AGENTS.md](../AGENTS.md).

---

## Test Harness

### `test_runner.h`

`indigo_test/test_runner.h` is the dependency-free runner every C test uses.

* `indigo_test_case tests[] = { { "name", function }, ... }` and `indigo_run_tests(suite, tests, ARRAY_SIZE(tests))`. It prints `PASS`/`FAIL` per case and returns 0 only if every case passed. Each test executable returns non-zero on failure.
* Assertions: `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_EQ_INT`, `ASSERT_EQ_TOKEN`, `ASSERT_NEAR`, `ASSERT_STREQ`. A failed assertion prints file and line, counts a failure and `return`s from the current test function.
* `INDIGO_TEST_CASE_FILTER=<substring>` runs only the cases whose names contain the substring.
* `indigo_test_use_private_home()` / `indigo_test_remove_private_home()` point `HOME` at a private temporary directory, so configuration saves never touch `~/.indigo`. Call the first as the first statement of `main()` in any suite that saves or loads configuration. Never redirect `indigo_uni_config_folder` with `-D` (see *Configuration Isolation* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md)).
* `INDIGO_DRIVER_API_GENERATION(version)` for version checks. Tests assert the API generation (`INDIGO_DRIVER_API_3`) and never an exact `DRIVER_VERSION`.

### `integration/simulator_test_common.h`

This header contains the in-process client and a property cache for **one device at a time**.

* `simulator_driver_case` describes the driver under test: label, driver name, device name, entry point, multi-device flag, and the lists of expected visible or hidden base and connected properties.
* The global `context` records defines, updates (including updates for undefined properties), per-property revisions, per-state revisions and the connection state.
* Lookups: `find_cached_property()`, `find_cached_item()`, `cached_number_value()`, `property_revision()`.
* Assertions: `assert_defined_property()`, `assert_not_defined_property()`, `assert_defined_property_count()`, `assert_property_has_item[s]()`, `assert_device_interface()`, `assert_switch_item_value()`, `assert_number_item_in_range()`, `assert_rejected_number_change()`, `assert_rejected_switch_change()`, `assert_simulator_driver_info()`, `assert_simulator_properties()`.
* Bounded waits: `wait_for_property_state()`, `wait_for_property_state_after(name, state, revision)`, `wait_for_property_state_seen_after()`, `wait_for_property_not_busy[_after]()`, `wait_for_number_item_value()`. They poll every 100 ms for up to 10 s. `wait_for_simulator_connection_state()` waits up to 5 s. Take the revision before the request and use the `_after` variants, so a stale OK state from an earlier request cannot satisfy the wait.
* Life cycle: `start_connected_simulator()` / `stop_connected_simulator()` for virtual drivers, and `save_configuration()`, which waits for the cleared `SAVE` item rather than for an update.

### `integration/serial_simulator_test_common.h`

* `start_external_serial_simulator(&sim, executable)` and `start_external_serial_simulator_with_args()` create `/tmp/indigo-serial-sim.XXXXXX`, fork and exec the simulator with `--headless --ready-file <dir>/ready.env` plus any extra arguments, and wait up to 5 s for the ready file. `stop_external_serial_simulator()` sends SIGTERM, reaps the process, captures the trace and removes the directory.
* `start_serial_driver(&case, port)` / `stop_serial_driver(&case)` bring the driver up, set `DEVICE_PORT` and connect, then disconnect and shut down. The finer-grained helpers `bring_up_serial_driver()`, `connect_serial_device()`, `disconnect_serial_device()` and `tear_down_serial_driver()` are for lifecycle scenarios. `start_shared_serial_device()` connects a secondary logical device through its master.
* `SERIAL_CHECK_TRUE` / `SERIAL_CHECK_EQ_INT` jump to a `cleanup:` label instead of returning, so the simulator and the driver are always torn down.
* `assert_serial_<class>_class_property_completeness()` checks the class base properties.
* `INDIGO_SIMULATOR_TRACE_DIR=<dir>` copies each fixture's event log to `<dir>/<test case>-<n>.events`.

### Other shared helpers

* `integration/parallel_case_runner.h`: runs each case in its own forked child with its own bus, simulator and fixture directory, several at a time, and replays output in registration order. `INDIGO_TEST_JOBS` sets parallelism: by default the processor count, at least 2 and capped at 8. Set `INDIGO_TEST_JOBS=1` for a serial run.
* `integration/abort_queue_test_common.h`, `aux_test_isolation.h`, `lunatico_test_common.h`, `fli_sdk_test_common.h` and `ccd_test_noise.h`: class- or family-specific helpers.
* `hardware/hardware_test_common.h`, `usb_hotplug_test_common.h` and `powerbox_hotplug_test_common.h`: a multi-device client cache and hot-plug helpers for hardware suites.
* `mountsim/mountsim_test_common.h` and `run_mountsim.py`: the MountSim launcher, PTY relay and artifacts.

---

## Writing a New Test

### File layout and naming

* Name every driver-specific file and target with the exact driver name and the backend: `integration/test_<class>_<device>_simulator.c`, `test_<driver>_sdk.c`, `_usb.c`, `_hid.c`, `_sysfs.c`, `_transport.c`, `_motion.c`, `hardware/test_<driver>_hw.c`, `mountsim/test_<driver>_mountsim.c`. Do not add driver cases to a generic shared file.
* New unit tests go in `unit/test_<topic>.c`. Put new parser fixtures in `fixtures/protocol/` and driver reference traces in `fixtures/<driver>/`.
* Give every new file under `indigo_test/` the license header and the `// Code is generated by AI` notice exactly as shown in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md). Test agents may change files only inside `indigo_test/`. The simulator source under `indigo_drivers/` is the documented exception.

### Structure of a case

Copy the closest existing test of the same class. The serial skeleton is in *Serial Simulator Integration Test Skeleton* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md). In short:

```c
static void <device>_goto_reports_busy_then_ok(void) {
	external_serial_simulator simulator = { 0 };
	SERIAL_CHECK_TRUE(start_external_serial_simulator(&simulator, <CLASS>_<DEVICE>_SIMULATOR_EXECUTABLE));
	SERIAL_CHECK_TRUE(start_serial_driver(&<device>_case, simulator.port));
	unsigned int revision = property_revision(<PROPERTY>_PROPERTY_NAME);
	// issue the request through the public bus API
	SERIAL_CHECK_TRUE(wait_for_property_state_after(<PROPERTY>_PROPERTY_NAME, INDIGO_OK_STATE, revision));
	// assert readback and, where it matters, the protocol trace
cleanup:
	if (context.connected) {
		stop_serial_driver(&<device>_case);
	}
	stop_external_serial_simulator(&simulator);
}
```

* Drive the production driver through public bus requests only. Keep real queues and timers, and do not model the driver's implementation.
* For a multi-device driver, give each logical device its own `simulator_driver_case` and its own case with a fresh driver lifecycle. Count `indigo_attach_device(` calls to find them all (*Multi-Device Serial Drivers* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md)).
* Test only driver-owned behaviour. Do not inject framework-validated values such as NaN, out-of-range numbers or fractional steps. Do test device replies, protocol failures, BUSY conflicts, aborts, disconnects and recovery (see *Test scope shared by all driver classes* in [DRIVER_TESTING_RULES.md](../indigo_test/DRIVER_TESTING_RULES.md)).
* Print enough context before an assertion that a helper failure still identifies the property, item and device.

### Timeouts and waiting

* Use the bounded wait helpers, and a deterministic gate in the simulator or fake when an intermediate state must be observed. Avoid long `indigo_usleep()` sleeps. A short sleep is acceptable only to check that nothing happens during an interval, as `mount_manual_motion_disconnect` does after a disconnect.
* A hardware-driver timeout that a case must exercise dominates the run time. Put such cases in a `parallel_case_runner` suite rather than stretching the wait helpers.

### Ports, isolation and cleanup

* Serial tests never assume a port name. The PTY path, or the TCP URL for `--tcp`, comes from the ready file of a simulator started in a private directory. Simulator TCP listeners bind `127.0.0.1` port 0, so the port is ephemeral and conflict-free.
* Cases that bind a **fixed** loopback port, such as the dome `--network` modes with port 8080, must stay serial (`jobs = 1`) and outside `test-integration`.
* Isolate configuration with `indigo_test_use_private_home()`. A case that changes a persistent setting restores it, and a case that needs a setting establishes it itself.
* Always disconnect every device, shut the driver down, detach the client, call `indigo_stop()`, stop the simulator and remove temporary directories, including on the failure path.
* Keep images and other output in a directory of the test's own, not in the redirected `HOME`.

### Parallel safety

`make test` runs the executables one after another. Inside an executable, cases run sequentially unless the suite uses `parallel_case_runner.h`. In that case derive every event log, fault file, control file and `HOME` from the per-case `fixture` argument **inside** the child, and never print results from the parent. Because each test binary has its own in-process bus, private simulator directory and private `HOME`, separate binaries do not share state. The fixed-port opt-in modes are the exception.

### Registering the test in the build

In `indigo_test/Makefile` (see *Makefile Rules* and *Wiring a Host-Side Serial Simulator Into the Test Build* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md)):

1. Add the executable to `UNIT_TESTS`, `INTEGRATION_TESTS` or `BENCHMARKS`. Platform-specific tests are appended conditionally, as `test_ccd_ptp_ica` (macOS) and the sysfs tests (not macOS) are.
2. Add a local `<CLASS>_<DEVICE>_SIMULATOR_TEST_LDFLAGS` naming the driver archive `$(BUILD_DRIVERS)/indigo_<driver>.a`.
3. For a serial simulator, add a rule building `$(INTEGRATION_BUILD)/<class>_<device>_simulator` from its source under `indigo_drivers/` (add `-pthread` only if it uses threads).
4. Add the test rule. List the shared headers, the simulator binary **and the driver archive** as prerequisites, so a rebuilt driver relinks the test.
5. Optionally add a `test-<driver>-...` phony target, and a `_sanitize` build for drivers with non-trivial state.

Then add every new source, header and fixture to its group in `indigo.xcodeproj/project.pbxproj`. After changing a driver's test coverage, update the driver's `REFACTOR.md` and the *Automated Tests (Sim / HW)* counts in the root `MIGRATION_STATUS.md`.

---

## Running Tests

The build must exist first. Run `make all` from the repository root: the test Makefile's `check-lib` target stops if `build/lib/libindigo` is missing, and tests link the driver archives from `build/drivers/`. The root `Makefile` has no `test` target. Tests are run through `indigo_test/Makefile`.

```sh
make -C indigo_test test               # unit + integration
make -C indigo_test test-unit          # unit only
make -C indigo_test test-integration   # integration only; stops at the first failing executable
make -C indigo_test benchmark          # measurements, never pass/fail
make -C indigo_test test-clean         # remove indigo_test/build (do this after validating)
```

Run a single suite by building its target and running the binary from `indigo_test/`, because simulator paths are relative to it:

```sh
make -C indigo_test build/integration/test_mount_simulator
cd indigo_test && build/integration/test_mount_simulator
INDIGO_TEST_CASE_FILTER=goto build/integration/test_mount_simulator   # only cases whose name contains "goto"
```

Some suites have their own named targets, such as `test-agent-mount`, `test-generator-architecture`, `test-mount-lx200-tcp` and `test-dome-baader-simulator-sanitize`. Some suites also select cases themselves: `test_mount_lx200_simulator <substring>` runs matching cases, and a few read `INDIGO_TEST_FILTER` or a driver-specific `*_TEST_FILTER` instead of `INDIGO_TEST_CASE_FILTER`. Check the suite's `main()`.

### Logs and trace levels

The library defaults to `INDIGO_LOG_ERROR`, and tests do not parse `-v` style arguments. Diagnostics are therefore per suite:

* Several agent and sysfs suites raise the level to debug with `INDIGO_TEST_DEBUG=1`, and print client traffic with `INDIGO_TEST_TRACE=1`. Some driver suites have their own switch, such as `BAADER_DEBUG`, `NEXDOME_DEBUG`, `DSD_DEBUG`, `PRIMALUCE_TEST_TRACE` or `QHY_TEST_TRACE`. Hardware suites use `HW_DEBUG=1` / `--debug` and `HW_TRACE=1`.
* When a suite has no switch, add a temporary `indigo_set_log_level(INDIGO_LOG_DEBUG)` (or `INDIGO_LOG_TRACE`, `INDIGO_LOG_TRACE_BUS`) at the start of `main()` locally, and do not commit it.
* A serial simulator accepts `--trace` when run by hand. In automated runs, use `INDIGO_SIMULATOR_TRACE_DIR` to keep the event logs.
* `INDIGO_TEST_JOBS=1` serializes a parallel suite so its output and timing can be read.

### Platforms

The automated suite is POSIX only. It uses `fork()`, pseudo terminals, `nftw()` and Unix paths, and runs on macOS and Linux. There is no Windows test project: the Windows solution (`indigo_windows.sln`, `indigo_windows/`) builds the library, drivers and tools but contains no INDIGO tests. The only `*test*.vcxproj` files belong to vendored libraries (libusb, hidapi, libraw).

### Continuous integration

The repository has no active CI test job. There is no `.github/workflows`. The legacy `.travis.yml` builds on Linux, macOS and Windows and calls `indigo_test/test_suite.sh --driver-test`, but that script does not exist in the repository, so the Travis test step is stale. Run the suite locally, or on the hardware host as described in *Driving the Hardware Test Host Over SSH* in [indigo_test/AGENTS.md](../indigo_test/AGENTS.md).

---

## End-to-End Tests With indigo_server

The automated suite deliberately avoids `indigo_server`. End-to-end checks through the network protocol are manual or script-driven.

### Running drivers in the server

```sh
build/bin/indigo_server -vv indigo_mount_simulator indigo_ccd_simulator
```

Drivers are named by their driver name (`indigo_<class>_<device>`). `-v`, `-vv`, `-vvb` and `-vvv` enable info, debug, bus trace and full trace logging. `-p` changes the port (default 7624), `-b-` disables Bonjour, and `-l` logs to syslog. See [INDIGO_SERVER_AND_DRIVERS_GUIDE.md](INDIGO_SERVER_AND_DRIVERS_GUIDE.md). Point a client (INDIGO Control Panel, a web app or `indigo_prop_tool`) at the server.

To exercise a serial driver without hardware, build its simulator (`make -C indigo_test build/integration/<class>_<device>_simulator`) and start it without `--headless`. It prints the pseudo-terminal path. Set that path as the driver's `DEVICE_PORT` before connecting.

### Scripting with `indigo_prop_tool`

`build/bin/indigo_prop_tool` reads and changes properties on a running server:

```sh
indigo_prop_tool list "Mount Simulator"
indigo_prop_tool set "<Device>.DEVICE_PORT.PORT=/dev/ttys012"
indigo_prop_tool set -w OK -t 10 "<Device>.CONNECTION.CONNECTED=ON"
indigo_prop_tool get "<Device>.CONNECTION.CONNECTED"
indigo_prop_tool get_state "<Device>.CONNECTION"
```

`-r host[:port]` selects a remote server, `-w OK|BUSY|ALERT|IDLE|ANY` waits for a state, `-t` sets the wait in seconds (default 2), and `-v`/`-vv`/`-vvv` enable logging. Run `indigo_prop_tool -h` for the full list.

### Shell compliance scripts (`indigo_tests/`)

`indigo_tests/indigo_test_framework.sh` is a bash library on top of `indigo_prop_tool`, documented in [indigo_tests/INDIGO_TEST_FRAMEWORK.md](../indigo_tests/INDIGO_TEST_FRAMEWORK.md). Class scripts (`ao_`, `filter_wheel_`, `focuser_`, `gps_`, `guider_`, `rotator_compliance.sh`) run against any device on a running server:

```sh
INDIGO_PROP_TOOL=build/bin/indigo_prop_tool indigo_tests/focuser_compliance.sh "Focuser Simulator" localhost:7624
```

They are useful for a quick check of a real device or a server build. They are not part of `make test`, and [DRIVER_TESTING_RULES.md](../indigo_test/DRIVER_TESTING_RULES.md) folds their scenarios into the C suite.

---

## Testing a New Generated Driver

How the generator itself works is in [DRIVER_GENERATOR_BASICS.md](DRIVER_GENERATOR_BASICS.md). For testing a generated driver:

1. **Backend first.** A serial or hidapi driver gets a PTY simulator (`indigo_drivers/<driver>/<driver>_simulator/`, using `serial_motion.h` for motion). A libusb or SDK driver gets a fake at the SDK/USB boundary using `-D` replacements. A virtual driver is tested directly.
2. **Baseline for migrations.** When a hand-written driver is converted, the suite must first pass against the original driver, and its reference trace must be recorded (`INDIGO_SIMULATOR_TRACE_DIR`, `fixtures/<driver>/original_reference_trace.txt`) before reverse extraction. See [indigo_drivers/AGENTS.override.md](../indigo_drivers/AGENTS.override.md) and [DRIVER_GENERATOR_MIGRATION.md](DRIVER_GENERATOR_MIGRATION.md).
3. **Use public names.** Generated headers expose only the entry point. Tests use the generated device names and public property names, not removed private macros.
4. **Cover generator semantics.** Beyond the class checklist, test the paths the generator owns: connect failure rollback and reconnect, shared-connection reference counting for multi-device drivers (both connection orders, last close), the BUSY guard that rejects overlapping requests, finalizer completion versus abort and disconnect, persistent properties through a save/load cycle in a private `HOME`, the mount parked-mount guard, and hot-plug attach, detach and duplicate events for `libusb`/`hid`/`sdk` drivers.
5. **Regenerate, build, run narrow, then full.** After each `.driver` edit, regenerate, build the driver archive (the test rule relinks because the archive is a prerequisite), inspect the generated diff, run the narrowest case with `INDIGO_TEST_CASE_FILTER`, and then the driver's full suite and its sanitize target if present.
6. **Generator changes** are validated by `test_generator_architecture`, which runs `build/bin/indigo_generator` on fixture `.driver` files and compiles the output. Generator changes need explicit user approval ([AGENTS.md](../AGENTS.md)).
7. **Record.** Update `REFACTOR.md` (scenario-to-test mapping, gaps), `MIGRATION_STATUS.md` status columns and test counts, the driver `README.md` `## Testing` line, and regenerate `TEST_SUMMARY.md` with `python3 tools/make_test_summary.py`.

---

## Checklist

* Backend exists: a PTY simulator following [SERIAL_DEVICE_SIMULATORS.md](SERIAL_DEVICE_SIMULATORS.md), a fake SDK/USB modelled on the real device, or a virtual driver.
* The test file and target use the exact driver name and backend suffix, with the `indigo_test/` license header and AI notice.
* Every logical device is covered in its own case with its own lifecycle.
* Driver info, interface bit, base visible and hidden properties before and after connect, and driver-specific properties and items are checked.
* The class checklist in [DRIVER_TESTING_RULES.md](../indigo_test/DRIVER_TESTING_RULES.md) is covered: protocol commands and readback, transitions with fresh revisions, rejection, BUSY conflicts, abort, failure injection at open, write, poll and stop, recovery, connect/disconnect/reconnect, and concurrency.
* Bounded waits only. No fixed ports in the default target, `HOME` is private, and configuration and settings are restored.
* Cleanup runs on every path: disconnect, shutdown, detach, `indigo_stop()`, simulator stop and directory removal.
* No exact `DRIVER_VERSION` asserts. Only the API generation is asserted.
* The Makefile lists the executable and has prerequisites for the headers, the simulator binary and the driver archive. Sources, headers and fixtures are registered in `indigo.xcodeproj`.
* Guider-capable drivers have a pulse-duration measurement recorded in `REFACTOR.md`.
* Full suite clean, plus the sanitize target where present. Run `make -C indigo_test test-clean` afterwards.
* `REFACTOR.md`, `MIGRATION_STATUS.md`, the driver `README.md` `## Testing` line and `TEST_SUMMARY.md` are updated. One driver per commit.

---

## Known Gaps

* **Client-detach motion release is exercised on two mount drivers only.** [`test_detach_abort`](#client-detach-motion-release) detaches motion owners on `mount_simulator`, `mount_lx200` (serial simulator) and through `agent_mount`. The other eight generated mount drivers are covered for this path only by compilation and by their own suites, which never detach the owner of a running motion. See [issue 10 in DRIVER_GENERATOR_BASICS.md](DRIVER_GENERATOR_BASICS.md#ki-10).
* **The simulator test cache tracks one device.** Tests that need to observe two logical devices at once must run separate lifecycles or keep their own cache.
* **No CI.** `.travis.yml` refers to a non-existent `indigo_test/test_suite.sh`, and no workflow runs `make -C indigo_test test`.
* **No Windows tests.** The suite depends on POSIX facilities, and nothing exercises drivers on Windows automatically.
* **Sanitizer coverage is selective and partly arm64-specific.** Only drivers with explicit `_sanitize`/`_asan` rules get it, and several of those rules force `-arch arm64`. There is no Valgrind or leak-checking target (`detect_leaks=0`).
* **Inconsistent diagnostics switches.** Log level and case filters differ between suites (`INDIGO_TEST_CASE_FILTER`, `INDIGO_TEST_FILTER`, driver-specific `*_TEST_FILTER`, `INDIGO_TEST_DEBUG`, ad hoc `*_DEBUG`).
* **`indigo_server` path untested automatically.** Network protocol, BLOB transfer and server-side driver loading are covered by unit parser tests and manual or `indigo_tests/` scripts, not by the default suite. The opt-in `test-detach-abort-network` runs the server's TCP listener and XML adapter in process, but only for connection loss during manual motion; it does not start the `indigo_server` executable.

---

## See Also

* [Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md)
* [Driver Generator Basics](DRIVER_GENERATOR_BASICS.md)
* [Driver Generator Migration Guide](DRIVER_GENERATOR_MIGRATION.md)
* [Serial Device Simulators](SERIAL_DEVICE_SIMULATORS.md)
* [Timers and Handler Queues](TIMERS_AND_QUEUES.md)
* [Makefiles](MAKEFILES.md)
* [Server and Drivers Guide](INDIGO_SERVER_AND_DRIVERS_GUIDE.md)
* [indigo_test/AGENTS.md](../indigo_test/AGENTS.md), [indigo_test/DRIVER_TESTING_RULES.md](../indigo_test/DRIVER_TESTING_RULES.md), [indigo_test/mountsim/USAGE.md](../indigo_test/mountsim/USAGE.md)
* [indigo_tests/INDIGO_TEST_FRAMEWORK.md](../indigo_tests/INDIGO_TEST_FRAMEWORK.md)
