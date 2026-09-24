// MLAstro Robotic Polar Alignment (RPA) simulator
//
// Copyright (c) 2026 by Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Protocol: https://github.com/MLAstroRPA/MLAstroRPA.NINA.Plugin/blob/main/Documentation/Serial-protocol.md
// Behaviour the document leaves open was taken from the MLAstro NINA plugin
// (Services/SerialConnectionService.cs) in the same repository.
//
// Behaviour marked [1.8.1] was observed on MLAstro firmware 1.8.1 running on a
// bare ESP32 board (TTGO, no TMC2209 drivers) on 2026-09-24; the rest follows
// the protocol document.
//
// Modelled behaviour:
// - Until the "[MLAstroRPA-TC]" handshake is accepted every command is refused
//   with "error: Not connected. Send [MLAstroRPA-TC] to take control." and the
//   "?" poll with "error: Not connected. System is idle or controlled by
//   Web/PC-Wireless." [1.8.1]. "Disconnect" (sent by the NINA plugin before it
//   closes the port) is acknowledged with "ok" and hands control back; so does
//   "disconnect" written to the control file, which models a Web UI reload: the
//   simulator then pushes "DISCONNECTED" and waits for a new handshake.
// - Communication watchdog [1.8.1]: with control taken, a gap of more than
//   --heartbeat seconds (default 1.15) between two commands of any kind, "?"
//   included, stops the motors, pushes "error: Serial heartbeat timeout ->
//   ESTOP" and drops control. "heartbeat" in the control file expires it at
//   once; "release" drops control without any push, so only the "Not
//   connected" replies tell.
// - Every command line gets one "ok"/"error: ..." reply, including the
//   comma-chained ones (ReDe:D,ReAM:M,ReAS:S,MAzR:1 and AzED:...,AAll:1); a
//   chained line whose motion command is refused gets the error and then an
//   extra "ok" [1.8.1]. SetH:1 is followed by a "SetH:COMPLETED" push and
//   STOP:1 while idle by a "SetH:STOPPED" push [1.8.1].
// - --no-motor-drivers models the bench board without TMC2209 drivers [1.8.1]:
//   the post-start driver check locks the controller in ERROR and pushes
//   "ERROR:Sys:2,AzNC:2,AlNC:2,..." after the handshake reply; ReER:1 clears it
//   (READY, "ERROR:Sys:0,..." push) until the next motion command, which is
//   refused with "error: Driver Not Responding" after the axis has crept
//   0.13 deg, and locks the controller again.
// - --log-noise pushes the firmware's boot banner after the handshake reply and
//   a WiFi log line every two seconds, as firmware 1.8.1 does on its serial
//   port [1.8.1].
// - Telemetry reports AzPH/AlPH, the angle from the SetH:1 reference, and
//   Mpos, the angle moved since the last motion started.
// - Relative moves report MOVING, alignment moves ALIGNING and end in
//   ALIGN_COMPLETED plus an asynchronous "AzAN/AlAN/AAll:COMPLETED" push; AAll
//   moves azimuth first, then altitude. RetH:1 reports HOMING and ends in
//   HOME_COMPLETED plus a "HOME_COMPLETED" push.
// - STOP:1 and the ":0" release of a move command decelerate, ESTOP:1 stops
//   at once; STOP:0/ESTOP:0 are ignored.
// - In jog mode (JoRe:0, the power-on default) a move command drives towards
//   the soft limit and the 500 ms watchdog stops it unless it is repeated.
// - RstH:1 forgets the home reference but keeps AzPH/AlPH and Mpos [1.8.1].
// - --hard-limit-az trips a StallGuard hard limit: the controller stops in
//   ERROR, pushes an "ERROR:Sys:...,AzHL:1,..." line and refuses motion with "error: System
//   Locked" until ReER:1; afterwards a move further in the blocked direction
//   is refused with "error: Hard Limit".
// Motor tuning (AzIR/AzIH/AzMS/AzAc/AzDec/AzSB/AzSC/AzRM and their Al*
// siblings), altitude overshoot (Over/OvUp/OvDn/OvD/OvM/OvS) and WiFi
// provisioning (STAs/STAp/APss/APpa/APip) are acknowledged without simulated
// effect: indigo_polaralign_mlastro does not use them either.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

// ----------------------------------------------------------------- options

typedef struct {
	bool headless;
	bool trace;
	const char *ready_file;
	const char *firmware;
	const char *serial_number;
	bool legacy_handshake;
	int boot_noise;
	bool defer_push;
	bool hard_limit_az_set;
	double hard_limit_az;
	double heartbeat;
	bool no_motor_drivers;
	bool log_noise;
	char control_file[PATH_MAX];
	char event_file[PATH_MAX];
} simulator_options;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.ready_file = NULL,
	.firmware = "firmware 1.2.43",
	.serial_number = "AA:BB:CC:DD:EE:F0",
	.heartbeat = 1.15
};

static const char *simulator_name = "polaralign_mlastro";

static void usage(const char *name) {
	printf("MLAstro Robotic Polar Alignment (RPA) simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless              Disable terminal-oriented output\n");
	printf("  --ready-file <path>     Write INDIGO_SIMULATOR_PORT after PTY setup\n");
	printf("  --trace                 Log protocol requests and replies\n");
	printf("  --firmware <version>    Set the handshake firmware string\n");
	printf("  --legacy-handshake      Reply a bare \"ok\" to the handshake, like older firmware\n");
	printf("  --boot-noise <count>    Answer the first <count> handshakes with ESP32 boot log lines only\n");
	printf("  --defer-push            Hold asynchronous pushes until just before the next command reply\n");
	printf("  --hard-limit-az <deg>   Trip a StallGuard hard limit when azimuth crosses <deg>\n");
	printf("  --heartbeat <seconds>   Communication watchdog timeout, 0 disables it (default 1.15)\n");
	printf("  --no-motor-drivers      Model a board without TMC2209 drivers (locked in ERROR)\n");
	printf("  --log-noise             Push the boot banner and periodic WiFi log lines\n");
	printf("  -h, --help              Show this help and exit\n");
	printf("  Runtime control is read from <ready-file>.control (\"disconnect\" hands control\n");
	printf("  back to the Web UI, \"heartbeat\" expires the communication watchdog, \"release\"\n");
	printf("  drops control silently) and every complete command is recorded in <ready-file>.events.\n");
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
		} else if (!strcmp(argv[i], "--firmware")) {
			if (++i == argc) {
				fprintf(stderr, "--firmware requires a version\n");
				return false;
			}
			options.firmware = argv[i];
		} else if (!strcmp(argv[i], "--legacy-handshake")) {
			options.legacy_handshake = true;
		} else if (!strcmp(argv[i], "--boot-noise")) {
			if (++i == argc) {
				fprintf(stderr, "--boot-noise requires a count\n");
				return false;
			}
			options.boot_noise = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--defer-push")) {
			options.defer_push = true;
		} else if (!strcmp(argv[i], "--hard-limit-az")) {
			if (++i == argc) {
				fprintf(stderr, "--hard-limit-az requires an angle\n");
				return false;
			}
			options.hard_limit_az_set = true;
			options.hard_limit_az = atof(argv[i]);
		} else if (!strcmp(argv[i], "--heartbeat")) {
			if (++i >= argc) {
				fprintf(stderr, "Missing value for --heartbeat\n");
				return false;
			}
			options.heartbeat = atof(argv[i]);
		} else if (!strcmp(argv[i], "--no-motor-drivers")) {
			options.no_motor_drivers = true;
		} else if (!strcmp(argv[i], "--log-noise")) {
			options.log_noise = true;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", argv[i]);
			return false;
		}
	}
	if (options.ready_file != NULL) {
		snprintf(options.control_file, sizeof(options.control_file), "%s.control", options.ready_file);
		snprintf(options.event_file, sizeof(options.event_file), "%s.events", options.ready_file);
	}
	return true;
}

// ----------------------------------------------------------------- state

static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef enum {
	MOTION_NONE,
	MOTION_MOVE,
	MOTION_ALIGN,
	MOTION_HOME
} motion_kind;

// Everything below is guarded by state_mutex.

static bool controlled = false;
static int handshakes_to_ignore = 0;
// serial_motion_time() of the last command line, for the communication watchdog
static double last_command_time = 0;
static double next_log_noise = 0;

// degrees, relative to the last SetH:1 reference (AzPH/AlPH)
static serial_motion az_motion, alt_motion;
// where the last motion started, for Mpos
static double mpos_origin_az = 0, mpos_origin_alt = 0;

static motion_kind kind = MOTION_NONE;
static char align_command[8];
static bool pending_alt = false;
static double pending_alt_target = 0;
static bool stopping = false;
static double jog_deadline = 0;
static const char *status = "READY";
static bool homed = false;

static int speed_level = 3;
static bool relative_mode = false;

// staged by ReDe/ReAM/ReAS, consumed by MAzL:1/MAzR:1/MAlU:1/MAlD:1
static int staged_re_d = 0, staged_re_m = 0, staged_re_s = 0;

// staged by AzED/AzEM/AzES/AzDi and AlED/AlEM/AlES/AlDi, consumed by AzAN:1/AlAN:1/AAll:1
static int staged_az_d = 0, staged_az_m = 0, staged_az_s = 0;
static bool staged_az_positive = true;
static int staged_alt_d = 0, staged_alt_m = 0, staged_alt_s = 0;
static bool staged_alt_positive = true;

// soft limits, degrees
static double az_limit_min = -9.0, az_limit_max = 9.0;
static double alt_limit_min = -9.0, alt_limit_max = 9.0;

static bool az_reversed = false, alt_reversed = false;
static double az_steps_per_deg = 3200, alt_steps_per_deg = 3200;

static bool backlash_enabled = false;
static int az_backlash_steps = 0, alt_backlash_steps = 0;

// StallGuard latch: locked until ReER:1, then motion further in the
// tripped azimuth direction (+1/-1) is refused until the axis backs off
static bool locked = false;
static int blocked_az_direction = 0;

// the "ERROR:Sys:..." report: Sys 2 while locked, AzNC/AlNC 2 for a motor
// driver that does not answer, AzHL for a tripped azimuth hard limit
static int error_az_nc = 0, error_alt_nc = 0, error_az_hl = 0;

// a push to send right after the reply to the command being dispatched
static char post_reply_push[256];

// Guarded by write_mutex: pushes held back by --defer-push.
static char deferred_pushes[8][256];
static int deferred_push_count = 0;

// slow enough that a move of a degree or more stays observably BUSY across
// several of the driver's once-a-second telemetry polls
#define SPEED_PER_LEVEL (0.2)   // deg/s per SLvl step, 0.6 deg/s at the default level 3
#define DECEL_DEG       (0.25)  // distance a soft stop takes to come to rest

static double speed(void) {
	return SPEED_PER_LEVEL * speed_level;
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
}

// ----------------------------------------------------------------- output

static void write_line_locked(int fd, const char *line) {
	serial_simulator_trace_line(options.trace, "<-", line);
	serial_simulator_write_all(fd, line, strlen(line));
}

// A reply to a command. Pushes held back by --defer-push go out first, so
// they land between the command and its reply, as a push that fires while
// the command is in flight would on the real device.
static void send_reply(int fd, bool flush_pushes, const char *format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	pthread_mutex_lock(&write_mutex);
	if (flush_pushes) {
		for (int i = 0; i < deferred_push_count; i++) {
			write_line_locked(fd, deferred_pushes[i]);
		}
		deferred_push_count = 0;
	}
	write_line_locked(fd, buffer);
	pthread_mutex_unlock(&write_mutex);
}

static void send_push(int fd, const char *line) {
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%s\n", line);
	pthread_mutex_lock(&write_mutex);
	if (options.defer_push) {
		if (deferred_push_count < (int)(sizeof(deferred_pushes) / sizeof(deferred_pushes[0]))) {
			snprintf(deferred_pushes[deferred_push_count++], sizeof(deferred_pushes[0]), "%s", buffer);
		}
	} else {
		write_line_locked(fd, buffer);
	}
	pthread_mutex_unlock(&write_mutex);
}

static void record_event(const char *command) {
	if (*options.event_file == '\0') {
		return;
	}
	FILE *file = fopen(options.event_file, "a");
	if (file == NULL) {
		return;
	}
	fprintf(file, "%s\n", command);
	fclose(file);
}

// One action at a time in <ready-file>.control, consumed when read.
static bool take_control_action(char *action, size_t size) {
	if (*options.control_file == '\0') {
		return false;
	}
	FILE *file = fopen(options.control_file, "r");
	if (file == NULL) {
		return false;
	}
	char buffer[32] = { 0 };
	int count = fscanf(file, "%31s", buffer);
	fclose(file);
	unlink(options.control_file);
	if (count != 1) {
		return false;
	}
	snprintf(action, size, "%s", buffer);
	return true;
}

// ----------------------------------------------------------------- motion

static void update_positions(void) {
	serial_motion_update(&az_motion);
	serial_motion_update(&alt_motion);
}

static bool axis_moving(const serial_motion *motion) {
	return motion->duration > 0;
}

static void soft_stop_axis(serial_motion *motion) {
	serial_motion_update(motion);
	if (axis_moving(motion)) {
		double remaining = motion->target - motion->position;
		serial_motion_start(motion, motion->position + copysign(fmin(fabs(remaining), DECEL_DEG), remaining), speed());
	}
}

static void soft_stop(void) {
	soft_stop_axis(&az_motion);
	soft_stop_axis(&alt_motion);
	pending_alt = false;
	jog_deadline = 0;
	if (kind != MOTION_NONE) {
		stopping = true;
	}
}

static void hard_stop(void) {
	serial_motion_stop(&az_motion);
	serial_motion_stop(&alt_motion);
	pending_alt = false;
	jog_deadline = 0;
	stopping = false;
	kind = MOTION_NONE;
}

static void format_error_line(char *line, size_t size) {
	snprintf(line, size, "ERROR:Sys:%d,AzNC:%d,AlNC:%d,AzOT:0,AlOT:0,AzPW:0,AlPW:0,AzSA:0,AzSB:0,AlSA:0,AlSB:0,AzOL:0,AlOL:0,AzHL:%d,AlHL:0,AzSL:0,AlSL:0,Esc:0,CmdRf:0", locked ? 2 : 0, error_az_nc, error_alt_nc, error_az_hl);
}

// Without TMC2209 drivers a motion command gets as far as a few steps before
// the controller notices that the driver does not answer [1.8.1].
static const char *driver_not_responding(double new_az, double new_alt) {
	bool azimuth = new_az != az_motion.position || new_alt == alt_motion.position;
	serial_motion *motion = azimuth ? &az_motion : &alt_motion;
	double target = azimuth ? new_az : new_alt;
	mpos_origin_az = az_motion.position;
	mpos_origin_alt = alt_motion.position;
	serial_motion_sync(motion, motion->position + (target >= motion->position ? 0.13 : -0.13));
	locked = true;
	status = "ERROR";
	if (azimuth) {
		error_az_nc = 2;
	} else {
		error_alt_nc = 2;
	}
	format_error_line(post_reply_push, sizeof(post_reply_push));
	return "error: Driver Not Responding\n";
}

// Starts a motion to an absolute (az, alt) target in degrees from home;
// returns the reply to send. AAll sequences azimuth before altitude.
static const char *begin_motion(motion_kind new_kind, double new_az, double new_alt, bool sequence_alt) {
	update_positions();
	if (locked) {
		return "error: System Locked\n";
	}
	if (new_az < az_limit_min || new_az > az_limit_max || new_alt < alt_limit_min || new_alt > alt_limit_max) {
		return "error: Soft Limit\n";
	}
	if (options.no_motor_drivers) {
		return driver_not_responding(new_az, new_alt);
	}
	int direction = new_az > az_motion.position ? 1 : new_az < az_motion.position ? -1 : 0;
	if (blocked_az_direction != 0 && direction == blocked_az_direction) {
		return "error: Hard Limit\n";
	}
	if (direction != 0) {
		blocked_az_direction = 0;
	}
	mpos_origin_az = az_motion.position;
	mpos_origin_alt = alt_motion.position;
	serial_motion_start(&az_motion, new_az, speed());
	if (sequence_alt) {
		pending_alt = true;
		pending_alt_target = new_alt;
	} else {
		pending_alt = false;
		serial_motion_start(&alt_motion, new_alt, speed());
	}
	kind = new_kind;
	stopping = false;
	jog_deadline = 0;
	status = new_kind == MOTION_MOVE ? "MOVING" : new_kind == MOTION_ALIGN ? "ALIGNING" : "HOMING";
	return "ok\n";
}

// A move command in either movement mode: relative mode moves by the staged
// ReDe/ReAM/ReAS angle, jog mode drives towards the soft limit until the
// watchdog runs out or the command is released.
static const char *move_command(bool azimuth, int sign) {
	update_positions();
	if (!relative_mode) {
		bool same_jog = jog_deadline > 0 && kind == MOTION_MOVE && axis_moving(azimuth ? &az_motion : &alt_motion);
		if (!same_jog) {
			double target = azimuth ? (sign > 0 ? az_limit_max : az_limit_min) : (sign > 0 ? alt_limit_max : alt_limit_min);
			const char *reply = azimuth ? begin_motion(MOTION_MOVE, target, alt_motion.position, false) : begin_motion(MOTION_MOVE, az_motion.position, target, false);
			if (strcmp(reply, "ok\n")) {
				return reply;
			}
		}
		jog_deadline = serial_motion_time() + 0.5;
		return "ok\n";
	}
	double delta = sign * (staged_re_d + staged_re_m / 60.0 + staged_re_s / 3600.0);
	if (azimuth) {
		return begin_motion(MOTION_MOVE, az_motion.position + delta, alt_motion.position, false);
	}
	return begin_motion(MOTION_MOVE, az_motion.position, alt_motion.position + delta, false);
}

// One background tick; returns the push line to send, if any.
static void tick(char *push, size_t size) {
	update_positions();
	if (options.hard_limit_az_set && axis_moving(&az_motion)) {
		double limit = options.hard_limit_az;
		if ((az_motion.origin < limit && az_motion.position >= limit) || (az_motion.origin > limit && az_motion.position <= limit)) {
			blocked_az_direction = az_motion.origin < limit ? 1 : -1;
			hard_stop();
			serial_motion_sync(&az_motion, limit);
			locked = true;
			error_az_hl = 1;
			status = "ERROR";
			format_error_line(push, size);
			return;
		}
	}
	if (jog_deadline > 0 && serial_motion_time() > jog_deadline) {
		soft_stop();
	}
	if (kind != MOTION_NONE && !axis_moving(&az_motion) && !axis_moving(&alt_motion)) {
		if (pending_alt) {
			pending_alt = false;
			serial_motion_start(&alt_motion, pending_alt_target, speed());
			return;
		}
		if (stopping || kind == MOTION_MOVE) {
			status = "READY";
		} else if (kind == MOTION_ALIGN) {
			status = "ALIGN_COMPLETED";
			snprintf(push, size, "%s:COMPLETED", align_command);
		} else {
			status = "HOME_COMPLETED";
			snprintf(push, size, "HOME_COMPLETED");
		}
		kind = MOTION_NONE;
		stopping = false;
	}
}

static void *background(void *arg) {
	(void)arg;
	while (running) {
		usleep(50000);
		char push[256] = { 0 };
		char action[32] = { 0 };
		bool acted = take_control_action(action, sizeof(action));
		bool web_ui = acted && !strcmp(action, "disconnect");
		bool release = acted && !strcmp(action, "release");
		pthread_mutex_lock(&state_mutex);
		tick(push, sizeof(push));
		bool was_controlled = controlled;
		bool heartbeat_expired = was_controlled && ((acted && !strcmp(action, "heartbeat")) || (options.heartbeat > 0 && serial_motion_time() - last_command_time > options.heartbeat));
		if (heartbeat_expired) {
			hard_stop();
		}
		if (web_ui || release || heartbeat_expired) {
			controlled = false;
		}
		bool log_noise = options.log_noise && serial_motion_time() > next_log_noise;
		if (log_noise) {
			next_log_noise = serial_motion_time() + 2;
		}
		pthread_mutex_unlock(&state_mutex);
		if (serial_fd < 0) {
			continue;
		}
		if (*push) {
			send_push(serial_fd, push);
		}
		if (web_ui && was_controlled) {
			send_push(serial_fd, "DISCONNECTED");
		}
		if (heartbeat_expired) {
			send_push(serial_fd, "error: Serial heartbeat timeout -> ESTOP");
		}
		if (log_noise) {
			send_push(serial_fd, "STA disconnected/failed - keep AP alive (reason: 201 - NO_AP_FOUND) | attempt: 0 | status: 1 | AP: 192.168.4.1");
		}
	}
	return NULL;
}

// ----------------------------------------------------------------- reader

static int sim_read_byte(int fd, char *byte) {
	while (running) {
		ssize_t count = read(fd, byte, 1);
		if (count == 1) {
			return 0;
		}
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
				usleep(500);
				continue;
			}
			return -1;
		}
		usleep(500);
	}
	return -1;
}

// Commands end with LF or CR, as the protocol allows either.
static int sim_read_command(int fd, char *buffer, size_t length) {
	char byte = '\0';
	size_t used = 0;
	while (running && used + 1 < length) {
		if (sim_read_byte(fd, &byte) < 0) {
			return -1;
		}
		if (byte == '\r' || byte == '\n') {
			if (used == 0) {
				continue;
			}
			buffer[used] = '\0';
			serial_simulator_trace_line(options.trace, "->", buffer);
			return (int)used;
		}
		buffer[used++] = byte;
	}
	buffer[0] = '\0';
	return -1;
}

// ----------------------------------------------------------------- protocol

static void send_telemetry(int fd) {
	pthread_mutex_lock(&state_mutex);
	update_positions();
	char line[1024];
	snprintf(
		line, sizeof(line),
		"<%s|Mpos:%.5f,%.5f|>Scal:1,WSta:1,SLvl:%d,Home:%d,JoRe:%d,ReDe:%d,ReAM:%d,ReAS:%d,"
		"AzED:%d,AzEM:%d,AzES:%d,AzDi:%d,AlED:%d,AlEM:%d,AlES:%d,AlDi:%d,"
		"AzPH:%.5f,AzL1:%.2f,AzL2:%.2f,AzRD:%d,AzSD:%.0f,"
		"AlPH:%.5f,AlL1:%.2f,AlL2:%.2f,AlRD:%d,AlSD:%.0f,"
		"Back:%d,Over:0,OvD:0,OvM:30,OvS:0,OvUp:0,OvDn:1,AzBl:%d,AlBl:%d\n",
		status, az_motion.position - mpos_origin_az, alt_motion.position - mpos_origin_alt,
		speed_level, homed ? 1 : 0, relative_mode ? 1 : 0, staged_re_d, staged_re_m, staged_re_s,
		staged_az_d, staged_az_m, staged_az_s, staged_az_positive ? 1 : 0, staged_alt_d, staged_alt_m, staged_alt_s, staged_alt_positive ? 1 : 0,
		az_motion.position, az_limit_min, az_limit_max, az_reversed ? 1 : 0, az_steps_per_deg,
		alt_motion.position, alt_limit_min, alt_limit_max, alt_reversed ? 1 : 0, alt_steps_per_deg,
		backlash_enabled ? 1 : 0, az_backlash_steps, alt_backlash_steps
	);
	pthread_mutex_unlock(&state_mutex);
	send_reply(fd, false, "%s", line);
}

// Applies one "KEY:VALUE" token, with state_mutex held. *response is left
// untouched for a plain setting, or set to the reply for the whole line when
// this token triggers one (a motion or home command). Returns false for an
// unknown key.
static bool apply_token(const char *key, const char *value, const char **response, bool *reboot) {
	bool press = !strcmp(value, "1");
	if (!strcmp(key, "ReDe")) {
		staged_re_d = atoi(value);
	} else if (!strcmp(key, "ReAM")) {
		staged_re_m = atoi(value);
	} else if (!strcmp(key, "ReAS")) {
		staged_re_s = atoi(value);
	} else if (!strcmp(key, "AzED")) {
		staged_az_d = atoi(value);
	} else if (!strcmp(key, "AzEM")) {
		staged_az_m = atoi(value);
	} else if (!strcmp(key, "AzES")) {
		staged_az_s = atoi(value);
	} else if (!strcmp(key, "AzDi")) {
		staged_az_positive = atoi(value) != 0;
	} else if (!strcmp(key, "AlED")) {
		staged_alt_d = atoi(value);
	} else if (!strcmp(key, "AlEM")) {
		staged_alt_m = atoi(value);
	} else if (!strcmp(key, "AlES")) {
		staged_alt_s = atoi(value);
	} else if (!strcmp(key, "AlDi")) {
		staged_alt_positive = atoi(value) != 0;
	} else if (!strcmp(key, "SLvl")) {
		int level = atoi(value);
		speed_level = level < 1 ? 1 : level > 5 ? 5 : level;
	} else if (!strcmp(key, "JoRe")) {
		relative_mode = atoi(value) != 0;
	} else if (!strcmp(key, "AzL1")) {
		az_limit_min = atof(value);
	} else if (!strcmp(key, "AzL2")) {
		az_limit_max = atof(value);
	} else if (!strcmp(key, "AlL1")) {
		alt_limit_min = atof(value);
	} else if (!strcmp(key, "AlL2")) {
		alt_limit_max = atof(value);
	} else if (!strcmp(key, "AzRD")) {
		az_reversed = atoi(value) != 0;
	} else if (!strcmp(key, "AlRD")) {
		alt_reversed = atoi(value) != 0;
	} else if (!strcmp(key, "AzSD")) {
		az_steps_per_deg = atof(value);
	} else if (!strcmp(key, "AlSD")) {
		alt_steps_per_deg = atof(value);
	} else if (!strcmp(key, "Back")) {
		backlash_enabled = atoi(value) != 0;
	} else if (!strcmp(key, "AzBl")) {
		az_backlash_steps = atoi(value);
	} else if (!strcmp(key, "AlBl")) {
		alt_backlash_steps = atoi(value);
	} else if (!strcmp(key, "MAzR") || !strcmp(key, "MAzL") || !strcmp(key, "MAlU") || !strcmp(key, "MAlD")) {
		bool azimuth = key[1] == 'A' && key[2] == 'z';
		if (press) {
			*response = move_command(azimuth, key[3] == 'R' || key[3] == 'U' ? 1 : -1);
		} else {
			soft_stop_axis(azimuth ? &az_motion : &alt_motion);
			if (kind != MOTION_NONE) {
				stopping = true;
			}
			jog_deadline = 0;
		}
	} else if (!strcmp(key, "AzAN") || !strcmp(key, "AlAN") || !strcmp(key, "AAll")) {
		if (press) {
			update_positions();
			double az_delta = (staged_az_d + staged_az_m / 60.0 + staged_az_s / 3600.0) * (staged_az_positive ? 1 : -1);
			double alt_delta = (staged_alt_d + staged_alt_m / 60.0 + staged_alt_s / 3600.0) * (staged_alt_positive ? 1 : -1);
			bool move_az = strcmp(key, "AlAN") != 0;
			bool move_alt = strcmp(key, "AzAN") != 0;
			*response = begin_motion(MOTION_ALIGN, az_motion.position + (move_az ? az_delta : 0), alt_motion.position + (move_alt ? alt_delta : 0), move_az && move_alt);
			snprintf(align_command, sizeof(align_command), "%s", key);
		}
	} else if (!strcmp(key, "STOP")) {
		if (press) {
			update_positions();
			if (kind == MOTION_NONE) {
				snprintf(post_reply_push, sizeof(post_reply_push), "SetH:STOPPED");
			}
			soft_stop();
		}
	} else if (!strcmp(key, "ESTOP")) {
		if (press) {
			hard_stop();
			status = "READY";
		}
	} else if (!strcmp(key, "ReER")) {
		hard_stop();
		locked = false;
		error_az_nc = error_alt_nc = error_az_hl = 0;
		status = "READY";
		format_error_line(post_reply_push, sizeof(post_reply_push));
	} else if (!strcmp(key, "SetH")) {
		hard_stop();
		serial_motion_sync(&az_motion, 0);
		serial_motion_sync(&alt_motion, 0);
		mpos_origin_az = mpos_origin_alt = 0;
		homed = true;
		status = locked ? "ERROR" : "READY";
		snprintf(post_reply_push, sizeof(post_reply_push), "SetH:COMPLETED");
	} else if (!strcmp(key, "RetH")) {
		*response = homed ? begin_motion(MOTION_HOME, 0, 0, false) : "error: Not homed\n";
	} else if (!strcmp(key, "RstH")) {
		// The document says "Clears the home status and coordinates", but
		// firmware 1.8.1 keeps reporting AzPH/AlPH and Mpos unchanged.
		homed = false;
	} else if (!strcmp(key, "Save&Reboot")) {
		*reboot = press;
	} else if (!strncmp(key, "Az", 2) || !strncmp(key, "Al", 2) || !strncmp(key, "Ov", 2) || !strcmp(key, "Over") || !strncmp(key, "STA", 3) || !strncmp(key, "AP", 2)) {
		// motor tuning / overshoot / WiFi fields not modelled; see file header
	} else {
		return false;
	}
	return true;
}

static const char *boot_banner[] = {
	"",
	"",
	"====== MLAstro Robotic Polar Alignment ======",
	"|               (MLAstroRPA)                |",
	"| firmware 1.8.1",
	"|              [ System Ready ]             |",
	"============================================="
};

static void dispatch_command(int fd, char *line) {
	record_event(line);
	if (!strcmp(line, "[MLAstroRPA-TC]")) {
		char error_line[256];
		pthread_mutex_lock(&state_mutex);
		bool booting = handshakes_to_ignore > 0;
		if (booting) {
			handshakes_to_ignore--;
		} else {
			controlled = true;
			last_command_time = serial_motion_time();
			format_error_line(error_line, sizeof(error_line));
		}
		pthread_mutex_unlock(&state_mutex);
		if (booting) {
			send_reply(fd, false, "ets Jul 29 2019 12:21:46\r\n");
			send_reply(fd, false, "rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\r\n");
		} else if (options.legacy_handshake) {
			send_reply(fd, true, "ok\n");
		} else {
			send_reply(fd, true, "ok,%s,SN:%s\n", options.firmware, options.serial_number);
			// firmware 1.8.1 reports its error state right after taking control
			send_push(fd, error_line);
			if (options.log_noise) {
				for (size_t i = 0; i < sizeof(boot_banner) / sizeof(boot_banner[0]); i++) {
					send_push(fd, boot_banner[i]);
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&state_mutex);
	bool accepted = controlled;
	if (accepted) {
		last_command_time = serial_motion_time();
		if (!strcmp(line, "Disconnect")) {
			controlled = false;
		}
	}
	pthread_mutex_unlock(&state_mutex);
	if (!accepted) {
		send_reply(fd, false, "%s", !strcmp(line, "?") ? "error: Not connected. System is idle or controlled by Web/PC-Wireless.\n" : "error: Not connected. Send [MLAstroRPA-TC] to take control.\n");
		return;
	}
	if (!strcmp(line, "Disconnect")) {
		send_reply(fd, true, "ok\n");
		return;
	}
	if (!strcmp(line, "?")) {
		send_telemetry(fd);
		return;
	}
	const char *response = "ok\n";
	bool reboot = false;
	char *save = NULL;
	char push[256];
	int tokens = 0;
	pthread_mutex_lock(&state_mutex);
	post_reply_push[0] = '\0';
	for (char *token = strtok_r(line, ",", &save); token; token = strtok_r(NULL, ",", &save)) {
		tokens++;
		char key[16] = { 0 }, value[32] = { 0 };
		if (sscanf(token, "%15[^:]:%31s", key, value) != 2 || !apply_token(key, value, &response, &reboot)) {
			response = "error: Unknown command\n";
			reboot = false;
			break;
		}
	}
	if (reboot) {
		controlled = false;
	}
	// a chained line whose motion command was refused still ends in "ok" [1.8.1]
	bool chained_refusal = tokens > 1 && !strncmp(response, "error:", 6) && strcmp(response, "error: Unknown command\n");
	snprintf(push, sizeof(push), "%s", post_reply_push);
	pthread_mutex_unlock(&state_mutex);
	send_reply(fd, true, "%s", response);
	if (chained_refusal) {
		send_reply(fd, false, "ok\n");
	}
	if (*push) {
		send_push(fd, push);
	}
	if (reboot) {
		send_reply(fd, false, "REBOOTING...\n");
	}
}

// ----------------------------------------------------------------- main

int main(int argc, char *argv[]) {
	pthread_t thread;
	char line[256];
	char port[128];

	if (!parse_args(argc, argv)) {
		usage(argv[0]);
		return 1;
	}
	handshakes_to_ignore = options.boot_noise;
	if (options.no_motor_drivers) {
		// the post-start driver connectivity check finds neither driver [1.8.1]
		locked = true;
		error_az_nc = error_alt_nc = 2;
		status = "ERROR";
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

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
		printf("MLAstro RPA simulator is running on %s\n", port);
		fflush(stdout);
	}

	if (pthread_create(&thread, NULL, background, NULL) != 0) {
		perror("pthread_create");
		close(serial_fd);
		serial_fd = -1;
		return 1;
	}

	while (running) {
		int count = sim_read_command(serial_fd, line, sizeof(line));
		if (count > 0) {
			dispatch_command(serial_fd, line);
		} else {
			usleep(1000);
		}
	}

	if (serial_fd >= 0) {
		close(serial_fd);
		serial_fd = -1;
	}
	pthread_join(thread, NULL);
	return 0;
}
