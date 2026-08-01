# Serial Device Simulators

This document defines the recommended contract for standalone serial device simulators used by automated INDIGO driver tests.

The reference model is a simulator executable that implements the hardware serial protocol behind a pseudo terminal. The INDIGO driver under test connects to the pseudo-terminal slave path through its normal `DEVICE_PORT` property, so the same driver code path is exercised as with real hardware.

## Goals

- Exercise real driver serial I/O, parsing, property updates, and connection handling without hardware.
- Keep simulators useful for both manual debugging and fully automated tests.
- Make simulator startup deterministic and machine-readable.
- Avoid test assumptions based on `/dev/pts/*` numbering, interactive terminal output, or `DEVICE_PORTS` enumeration.
- Keep every simulator process isolated so tests can run in parallel.

## Simulator Contract

Every serial simulator intended for automated testing should support:

| Option | Required | Description |
| --- | --- | --- |
| `--headless` | yes | Disable interactive UI and write diagnostics as plain text. |
| `--ready-file <path>` | yes | Write simulator metadata after the pseudo terminal is ready. |
| `--trace` | optional | Log protocol requests and replies for debugging. |
| `--model <name>` | optional | Select a protocol variant when one executable simulates multiple models. |
| `--device-id <id>` | optional | Override the simulated hardware identifier. |
| `--firmware <version>` | optional | Override the simulated firmware version. |

The simulator may keep an interactive default mode for manual use, but automated tests must use `--headless`.

## Ready File

The ready file is the handoff between the simulator process and the test harness. It must be written only after the pseudo-terminal slave path is valid and openable by the driver.

The file uses shell-style `KEY=value` lines:

```sh
INDIGO_SIMULATOR=falcon2
INDIGO_SIMULATOR_PORT=/dev/pts/7
INDIGO_SIMULATOR_PID=12345
```

Required keys:

| Key | Description |
| --- | --- |
| `INDIGO_SIMULATOR` | Short simulator name, for example `upb3` or `falcon2`. |
| `INDIGO_SIMULATOR_PORT` | Pseudo-terminal slave path to use as `DEVICE_PORT`. |
| `INDIGO_SIMULATOR_PID` | Simulator process id for cleanup and diagnostics. |

The simulator must write the ready file atomically:

1. Write to a temporary file in the same directory.
2. Flush and close it.
3. Rename the temporary file to the requested ready-file path.

This prevents the test harness from reading a partially written file.

## Test Harness Flow

A fully automated test should use this sequence:

1. Create a private temporary directory for the test run.
2. Start the simulator with `--headless --ready-file <tmp>/ready.env`.
3. Wait for the ready file with a bounded timeout.
4. Read `INDIGO_SIMULATOR_PORT`.
5. Start or attach the INDIGO driver under test.
6. Set `DEVICE_PORT.PORT` to `INDIGO_SIMULATOR_PORT`.
7. Set `CONNECTION.CONNECTED=ON` and wait for `CONNECTION` to reach `INDIGO_OK_STATE`.
8. Run driver compliance and property behavior tests.
9. Disconnect the driver.
10. Stop the simulator and remove the temporary directory.

Example:

```sh
tmp_dir=$(mktemp -d /tmp/indigo-falcon2.XXXXXX)
./rotator_falcon2_simulator --headless --ready-file "$tmp_dir/ready.env" &
sim_pid=$!

# The test harness should implement bounded waiting and validation here.
. "$tmp_dir/ready.env"

# Use "$INDIGO_SIMULATOR_PORT" as the value for DEVICE_PORT.PORT.

kill "$sim_pid"
rm -rf "$tmp_dir"
```

The ready file is preferred over parsing stdout.

## Driver Connection Rules

The driver must be configured through normal INDIGO properties:

- Do not add test-only connection paths to the driver.
- Set `DEVICE_PORT` before connecting.
- Do not depend on the simulator port being present in `DEVICE_PORTS`; pseudo-terminal enumeration is platform dependent.
- Run the same connection, enumeration, property-change, and disconnection scenarios used for real serial devices.

The simulator should behave like hardware from the driver perspective. If the real device requires baud-rate negotiation, model detection, or command retries, the simulator should implement enough of that behavior to exercise the real driver branch.

## Simulator Implementation Rules

Serial simulators should follow these implementation rules:

- Name new host-side serial simulator directories and source files as `<device_class>_<device>_simulator`, for example `focuser_askar_simulator/focuser_askar_simulator.c`.
- When generating a host-side pseudo-terminal simulator from an existing `.ino` simulator, keep the `.ino` sketch in the same simulator directory as the firmware-side reference and add the `.c` pseudo-terminal version beside it.
- Use the same pseudo-terminal master pattern used by existing working simulators: `open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK)`, `grantpt()`, `unlockpt()`, and `ptsname_r()`.
- Check errors from pseudo-terminal setup calls, including `grantpt()`, `unlockpt()`, and `ptsname_r()`.
- Exit with a nonzero status if the pseudo terminal or ready file cannot be created.
- Handle `SIGTERM` and `SIGINT` so automated cleanup is reliable.
- Avoid long idle sleeps in nonblocking readers. Driver probes can use short timeouts and may close and reopen the slave path quickly while detecting baud rate or model variants.
- Do not discard partial input when a nonblocking read returns `EAGAIN` or `EWOULDBLOCK`; keep waiting briefly until the line is complete.
- Treat `EIO` from the PTY master as an idle/no-slave condition where appropriate.
- Check `write()` return values and handle partial writes.
- Keep protocol state deterministic unless a test explicitly enables failure or timing variation.
- Keep protocol tracing separate from the ready file.
- Preserve the manual simulator mode when it is useful for development.

## Recommended Structure

Keep the simulator split conceptually into three parts:

| Part | Responsibility |
| --- | --- |
| PTY/runtime layer | Create the pseudo terminal, parse CLI options, write the ready file, handle shutdown. |
| Protocol layer | Parse commands and produce hardware-compatible replies. |
| State layer | Store and update simulated device state such as position, power ports, sensor values, or motion status. |

This split keeps the protocol reusable between headless automated tests and interactive manual simulators.

## Runtime Template

New host-side serial simulators should start from the following runtime template and then add only the device-specific state model and protocol dispatcher. Common pseudo-terminal setup, ready-file writing, tracing, and full-buffer writes live in `indigo_test/simulator_common/serial_simulator_common.h`; simulator sources should include it with the correct relative path for their directory.

```c
// Device serial simulator
//
// This source file was generated by a Codex agent.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL
};

static const char *simulator_name = "device";

static void usage(const char *name) {
	printf("Device serial simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  -h, --help              Show this help and exit\n");
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- runtime

static bool parse_device_option(int argc, char *argv[], int *index) {
	(void)argc;
	(void)argv;
	(void)index;
	return false;
}

static bool parse_args(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			exit(0);
		} else if (!strcmp(argv[i], "--headless")) {
			options.headless = true;
			options.trace = false;
		} else if (!strcmp(argv[i], "--trace")) {
			options.trace = true;
		} else if (!strcmp(argv[i], "--ready-file")) {
			if (++i == argc) {
				fprintf(stderr, "--ready-file requires a path\n");
				return false;
			}
			options.ready_file = argv[i];
		} else if (!parse_device_option(argc, argv, &i)) {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}

// ----------------------------------------------------------------- protocol

static int sim_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	int total_bytes = 0;

	while (running && total_bytes < length - 1) {
		ssize_t bytes_read = read(handle, &c, 1);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (total_bytes == 0) {
					return 0;
				}
				usleep(1000);
				continue;
			}
			if (errno == EIO) {
				return 0;
			}
			return -1;
		}
		if (bytes_read == 0) {
			return 0;
		}
		if (c == '\n') {
			break;
		}
		if (c == '\r') {
			continue;
		}
		buffer[total_bytes++] = c;
	}
	buffer[total_bytes] = '\0';
	if (*buffer) {
		serial_simulator_trace_line(options.trace, "->", buffer);
	}
	return total_bytes;
}

static bool sim_printf(int handle, const char *format, ...) {
	char buffer[128];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (length < 0) {
		return false;
	}
	if ((size_t)length >= sizeof(buffer)) {
		length = (int)sizeof(buffer) - 1;
	}

	if (options.trace) {
		fprintf(stderr, "<- %s", buffer);
	}
	return serial_simulator_write_all(handle, buffer, (size_t)length);
}

static void dispatch_command(int handle, const char *command) {
	(void)handle;
	(void)command;

	// Parse hardware commands here and reply with sim_printf().
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	char port[128];
	char buffer[128];

	if (!parse_args(argc, argv)) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0) {
		return 1;
	}

	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	if (!options.headless) {
		printf("%s simulator is running on %s\n", simulator_name, port);
		fflush(stdout);
	}

	while (running) {
		int count = sim_read_line(serial_fd, buffer, sizeof(buffer));
		if (count > 0) {
			dispatch_command(serial_fd, buffer);
		} else if (count == 0) {
			usleep(1000);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	return 0;
}
```

Device-specific simulators should replace `simulator_name`, extend `parse_device_option()` for options such as `--model`, `--device-id`, or fault injection, and implement `dispatch_command()` plus any state update threads or timers required by the hardware protocol.

The nonblocking reader must never treat `EAGAIN`, `EWOULDBLOCK`, or `EIO` as fatal: INDIGO drivers probe by opening and closing the pseudo-terminal slave repeatedly during baud-rate and model detection, so the read loop has to keep running until the process is signalled. Copy the read loop from an existing simulator (`rotator_falcon2_simulator.c`, `aux_wcv4ec_simulator.c`) rather than reimplementing it.

## Build and Test Integration

A refactored simulator is not complete until it is wired into the automated test build. The simulator source lives beside the `.ino` under `indigo_drivers/`, but its build target, its integration test, and the `Makefile` targets that link them live in `indigo_test/`. See the "Wiring a Host-Side Serial Simulator Into the Test Build" and "Serial Simulator Integration Test Skeleton" sections of `indigo_test/AGENTS.md` for the exact four `Makefile` edits (including when `-pthread` is required) and the integration-test template. After adding the test, register it in the implemented-simulators table below, remove the matching row from the candidates table, and update `indigo_test/CHANGES.md`.

## Simulators Fulfilling This Contract

The following simulators implement this contract. This list should grow as more host-side serial simulators are refactored.

| Simulator | Device class | Status |
| --- | --- | --- |
| `indigo_drivers/rotator_falcon/rotator_falcon2_simulator/rotator_falcon2_simulator.c` | Rotator | Implements the split runtime/state/protocol structure, `--headless`, `--ready-file`, `--trace`, `--model`, `--device-id`, and `--firmware`. |
| `indigo_drivers/aux_upb3/aux_upb3_simulator/aux_upb3_simulator.c` | AUX and focuser | Implements the split runtime/state/protocol structure, `--headless`, `--ready-file`, `--trace`, `--model`, `--device-id`, and `--firmware`. |
| `indigo_drivers/focuser_fc3/focuser_fc3_simulator/focuser_fc3_simulator.c` | Focuser | Implements the split runtime/state/protocol structure for FocusCube 3, `--headless`, `--ready-file`, `--trace`, `--model`, `--device-id`, and `--firmware`. |
| `indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c` | Focuser | Implements the split runtime/state/protocol structure for QHY Q-Focuser, `--headless`, `--ready-file`, `--trace`, `--model`, `--firmware`, `--board-version`, and `--no-out-temp`. |
| `indigo_drivers/focuser_askar/focuser_askar_simulator/focuser_askar_simulator.c` | Focuser | Implements the split runtime/state/protocol structure for Askar-WAF, `--headless`, `--ready-file`, `--trace`, `--model`, and `--firmware`. |
| `indigo_drivers/aux_svbpowerbox/aux_svbpowerbox_simulator/aux_svbpowerbox_simulator.c` | AUX | Implements the split runtime/state/protocol structure for SVBONY PowerBox, `--headless`, `--ready-file`, `--trace`, `--model`, `--no-ds18b20`, and `--no-sht40`. |
| `indigo_drivers/focuser_optecfl/focuser_optecfl_simulator/focuser_optecfl_simulator.c` | Focuser | Implements the split runtime/state/protocol structure for Optec FocusLynx, `--headless`, `--ready-file`, `--trace`, and `--model`. |
| `indigo_drivers/focuser_lacerta/focuser_lacerta_simulator/focuser_lacerta_simulator.c` | Focuser | Implements the split runtime/state/protocol structure for LACERTA Motorfocus, `--headless`, `--ready-file`, `--trace`, `--model`, and `--firmware`. |
| `indigo_drivers/aux_wbplusv3/aux_wbplusv3_simulator/aux_wbplusv3_simulator.c` | AUX | Implements the split runtime/state/protocol structure for WandererBox Plus V3, `--headless`, `--ready-file`, `--trace`, `--model`, and `--firmware`. |
| `indigo_drivers/aux_wbprov3/aux_wbprov3_simulator/aux_wbprov3_simulator.c` | AUX | Implements the split runtime/state/protocol structure for WandererBox Pro V3, `--headless`, `--ready-file`, `--trace`, `--model`, and `--firmware`. |
| `indigo_drivers/aux_wcv4ec/aux_wcv4ec_simulator/aux_wcv4ec_simulator.c` | AUX | Implements the split runtime/state/protocol structure for WandererCover V4-EC, `--headless`, `--ready-file`, `--trace`, `--model`, and `--firmware`. |
| `indigo_drivers/aux_arteskyflat/aux_arteskyflat_simulator/aux_arteskyflat_simulator.c` | AUX | Implements the split runtime/state/protocol structure for the Artesky Flat Box, `--headless`, `--ready-file`, `--trace`, and `--device-id`. |
| `indigo_drivers/aux_astromechanics/aux_astromechanics_simulator/aux_astromechanics_simulator.c` | AUX | Implements the split runtime/state/protocol structure for the ASTROMECHANICS LPM sky quality meter, `--headless`, `--ready-file`, `--trace`, and `--sky-brightness`. |
| `indigo_drivers/aux_fbc/aux_fbc_simulator/aux_fbc_simulator.c` | AUX | Implements the split runtime/state/protocol structure for the Lacerta FBC, including the `: I #`/`: P #`/`: V #` handshake with `D -` debug-line skipping, `--headless`, `--ready-file`, `--trace`, and `--firmware`. |
| `indigo_drivers/aux_flatmaster/aux_flatmaster_simulator/aux_flatmaster_simulator.c` | AUX | Implements the split runtime/state/protocol structure for the Pegasus Astro FlatMaster, including the `#`/`V` handshake and clamped `L:`/`E:` state, `--headless`, `--ready-file`, `--trace`, and `--firmware`. |
| `indigo_drivers/aux_flipflat/aux_flipflat_simulator/aux_flipflat_simulator.c` | AUX | Implements the split runtime/state/protocol structure for the Optec/Alnitak Flip-Flat, including the `>`/`*` command framing, monotonic-clock cover motion timing reported through `>SOOO` status, `--headless`, `--ready-file`, `--trace`, `--device-id`, and `--firmware`. |
| `indigo_drivers/ao_sx/ao_sx_simulator/ao_sx_simulator.c` | AO | Implements the split runtime/state/protocol structure for the StarlightXpress AO. Character-framed binary protocol (single-letter commands, fixed-length byte replies) rather than line based; supports the `X`/`V` handshake, `G` tip/tilt pulses, `M` guider pulses, `K`/`R` reset, `L` limit status, `--headless`, `--ready-file`, `--trace`, `--firmware`, and `--limit`. |
| `indigo_drivers/mount_synscan/mount_synscan_simulator/mount_synscan_simulator.c` | Mount | Implements the split runtime/state/protocol structure for SynScan/EQ8, `--headless`, `--ready-file`, `--trace`, and `--pcdirect`. |
| `indigo_drivers/wheel_quantum/wheel_quantum_simulator/wheel_quantum_simulator.c` | Wheel | Implements the split runtime/state/protocol structure for Brightstar Quantum filter wheels, `--headless`, `--ready-file`, `--trace`, `--serial`, and `--slots`. |
| `indigo_drivers/wheel_optec/wheel_optec_simulator/wheel_optec_simulator.c` | Wheel | Implements the split runtime/state/protocol structure for Optec filter wheels, `--headless`, `--ready-file`, `--trace`, `--goto-error`, and `--slots`. |
| `indigo_drivers/wheel_qhy/wheel_qhy_simulator/wheel_qhy_simulator.c` | Wheel | Implements the split runtime/state/protocol structure for QHY CFW1, CFW2, and CFW3 filter wheels, `--headless`, `--ready-file`, `--trace`, `--model`, `--firmware`, and `--slots`. |
| `indigo_drivers/wheel_trutek/wheel_trutek_simulator/wheel_trutek_simulator.c` | Wheel | Implements the split runtime/state/protocol structure for Trutek filter wheels, `--headless`, `--ready-file`, `--trace`, and `--slots`. |
| `indigo_drivers/wheel_xagyl/wheel_xagyl_simulator/wheel_xagyl_simulator.c` | Wheel | Implements the split runtime/state/protocol structure for Xagyl filter wheels, `--headless`, `--ready-file`, `--trace`, `--model`, `--firmware`, `--serial`, and `--slots`. |

## Candidates for refactoring

The following Arduino simulator sketches are firmware-side serial protocol references. They should be considered candidates for future host-side pseudo-terminal simulators using the contract above.

When a candidate is refactored into a host-side pseudo-terminal simulator, add the `.c` simulator to the implemented list above and remove the corresponding `.ino` row from this candidates list.

| Simulator | Device class | Notes |
| --- | --- | --- |
| `indigo_drivers/aux_ppb/aux_ppb_simulator/aux_ppb_simulator.ino` | AUX | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/aux_skyalert/aux_skyalert_simulator/aux_skyalert_simulator.ino` | AUX | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/aux_sqm/aux_sqm_simulator/aux_sqm_simulator.ino` | AUX | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/aux_upb/aux_upb_simulator/aux_upb_simulator.ino` | AUX | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/aux_usbdp/aux_usbdp_simulator/aux_usbdp_simulator.ino` | AUX | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/dome_baader/dome_baader_simulator/dome_baader_simulator.ino` | Dome | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/dome_nexdome/dome_nexdome_simulator/dome_nexdome_simulator.ino` | Dome | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/dome_nexdome3/dome_nexdome3_simulator/dome_nexdome3_simulator.ino` | Dome | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/dome_skyroof/dome_skyroof_simulator/dome_skyroof_simulator.ino` | Dome | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/dome_talon6ror/dome_talon6ror_simulator/dome_talon6ror_simulator.ino` | Dome | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_astromechanics/focuser_astromechanics_simulator/focuser_astromechanics_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_dmfc/focuser_dmfc_simulator/focuser_dmfc_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_efa/focuser_efa_simulator/focuser_efa_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_lakeside/focuser_lakeside_simulator/focuser_lakeside_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_mjkzz/focuser_mjkzz_simulator/focuser_mjkzz_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_moonlite/focuser_moonlite_simulator/focuser_moonlite_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_nfocus/focuser_nfocus_simulator/focuser_nfocus_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_nstep/focuser_nstep_simulator/focuser_nstep_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_optec/focuser_optec_simulator/focuser_optec_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_primaluce/focuser_primaluce_simulator/focuser_primaluce_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_prodigy/focuser_prodigy_simulator/focuser_prodigy_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_robofocus/focuser_robofocus_simulator/focuser_robofocus_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/focuser_steeldrive2/focuser_steeldrive2_simulator/focuser_steeldrive2_simulator.ino` | Focuser | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_1.0_simulator/mount_ioptron_1.0_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_2.0_simulator/mount_ioptron_2.0_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_2.5_simulator/mount_ioptron_2.5_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_3.0_simulator/mount_ioptron_3.0_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_8406_simulator/mount_ioptron_8406_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_ioptron/ioptron_simulator/mount_ioptron_8407_simulator/mount_ioptron_8407_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_lx200/mount_lx200_simulator/mount_lx200_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_nexstar/mount_nexstar_simulator/mount_nexstar_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_nexstaraux/mount_nexstaraux_simulator/mount_nexstaraux_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_rainbow/mount_rainbow_simulator/mount_rainbow_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/mount_synscan/mount_synscan_simulator/mount_synscan_simulator.ino` | Mount | Arduino sketch colocated with the refactored host-side SynScan simulator. |
| `indigo_drivers/mount_temma/mount_temma_simulator/mount_temma_simulator.ino` | Mount | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/rotator_optec/rotator_optec_simulator/rotator_optec_simulator.ino` | Rotator | Arduino sketch; candidate for future host-side serial simulator refactor. |
| `indigo_drivers/wheel_indigo/wheel_indigo_simulator/wheel_indigo_simulator.ino` | Wheel | Arduino sketch; candidate for future host-side serial simulator refactor. |

New serial simulator refactors should copy the host-side contract used by the implemented simulators above.

## Out of Scope

Not every directory named `*_simulator` should use this contract.

The `.ino` candidates above are firmware-side simulators. They are useful for hardware-in-the-loop testing, but they do not create a pseudo terminal by themselves. Automated tests can use them only through whatever real serial port the board exposes, so they need a different test setup.

INDIGO-native simulator drivers, such as `ccd_simulator`, `mount_simulator`, `dome_simulator`, `rotator_simulator`, `gps_simulator`, and `polaralign_simulator`, are not external serial protocol simulators. They are already INDIGO drivers and should be tested through normal INDIGO driver compliance tests.

Network simulators, such as `indigo_drivers/aux_dragonfly/relio_simulator/relio_simulator.pl` and `indigo_drivers/system_ascol/ascol_simulator/ascol_simulator.pl`, should use a similar ready-file idea, but with a network key instead of `INDIGO_SIMULATOR_PORT`, for example `INDIGO_SIMULATOR_HOST` and `INDIGO_SIMULATOR_TCP_PORT`.
