// iOptron mount serial simulator
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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
// Rewritten as a full-dialect, real-mechanics host simulator by AI (2026).

// Host-side iOptron mount protocol simulator, contract version 2.
//
// Reproduces the RS-232 command languages bundled with the driver: GOTONOVA
// HC 8406 (1.01), HC 8407 (1.4), 1.0, 2.0, 2.5 and 3.0/3.10. Every command is
// accepted only by the dialects that document it and only with the documented
// payload width and range. Unsupported commands are not answered and are logged
// as "?<command>"; malformed or out-of-range payloads are rejected with the
// documented failure reply (or no reply) and logged as "!<command>", so tests
// can assert protocol conformance of the driver.
//
// Mechanics use elapsed monotonic time: GOTO/park/home slews, sidereal drift of
// a stationary mount, tracking at sidereal/lunar/solar/King/custom rates, arrow
// motion at the selected speed, timed guide pulses at the guide rate, and a
// mount clock/site used for LST, side of pier and altitude/azimuth.
//
// Test control ("<ready-file>.control", consumed atomically):
//   <command>\t<reply>[\t<count>]   reply override (DROP = no reply, DELAY:<ms> =
//                                   delay then normal handling); count 0 = sticky,
//                                   reply CLEAR removes the rule
//   @status\t<digit|->              override the reported system status digit
//   @close\t1                       close the current TCP client connection
// Events ("<ready-file>.events"): "<monotonic seconds>\t<command>" per command.

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600
#define _DARWIN_C_SOURCE

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"

typedef enum {
	P8406,
	P8407,
	P0100,
	P0200,
	P0205,
	P0300
} dialect;

#define D8406 (1u << P8406)
#define D8407 (1u << P8407)
#define D0100 (1u << P0100)
#define D0200 (1u << P0200)
#define D0205 (1u << P0205)
#define D0300 (1u << P0300)
#define DALL (D8406 | D8407 | D0100 | D0200 | D0205 | D0300)

#define RA_FULL_MAS 1296000000.0
#define DEC_MAX_MAS 324000000.0
#define SIDEREAL_MAS_PER_S 15041.067
#define J2000_UNIX 946728000.0
#define MAX_RULES 16

typedef struct {
	bool headless;
	bool trace;
	bool tcp;
	const char *ready_file;
	dialect protocol;
	int product;
	char firmware[7];
	bool mountinfo;
	double slew_deg_per_s;
	double altitude_limit;
} simulator_options;

typedef struct {
	char command[64];
	char reply[128];
	int count;
} reply_rule;

typedef struct {
	bool tracking;
	bool track_after_slew;
	bool parked;
	bool parking;
	bool homed;
	bool homing;
	int tracking_mode;
	int moving_speed;
	double manual_ra;
	double manual_dec;
	int guide_ra;
	int guide_dec;
	int custom_rate;
	double rate_offset;
	int meridian_flip;
	int meridian_limit;
	bool pec;
	bool pec_recording;
	double latitude;
	double longitude;
	int utc_offset_minutes;
	int dst;
	double clock_delta;
	double park_alt;
	double park_az;
	bool park_position_set;
	double pulse_end[2];
	double pulse_rate[2];
	char status_override;
	bool has_target;
} simulator_state;

static simulator_options options = {
	.headless = false,
	.trace = true,
	.tcp = false,
	.ready_file = NULL,
	.protocol = P0300,
	.product = -1,
	.firmware = "",
	.mountinfo = true,
	.slew_deg_per_s = 100.0,
	.altitude_limit = -100.0
};

static simulator_state state = {
	.tracking = false,
	.homed = true,
	.tracking_mode = 0,
	.moving_speed = 5,
	.guide_ra = 50,
	.guide_dec = 50,
	.custom_rate = 10000,
	.meridian_flip = 0,
	.meridian_limit = 0,
	.latitude = 48.0,
	.longitude = 17.0,
	.status_override = 0
};

static const char *simulator_name = "mount_ioptron";
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static int listen_fd = -1;
static serial_motion ra_motion, dec_motion;
static double target_ra_mas, target_dec_mas;
static double last_update;
static bool slewing;
static reply_rule rules[MAX_RULES];
static FILE *events;
static const char *current_command;
static const int ARROW_SPEEDS[] = { 0, 1, 2, 8, 16, 64, 128, 256, 512, 1440 };
static const double HC8406_SPEEDS[] = { 16, 64, 256, 512 };

static void usage(const char *name) {
	printf("iOptron mount serial simulator\n");
	printf("Usage: %s [OPTIONS]\n", name);
	printf("  --headless                Disable terminal-oriented output\n");
	printf("  --protocol <dialect>      8406, 8407, 0100, 0200, 0205 or 0300\n");
	printf("  --product <nnnn>          :MountInfo# product code override\n");
	printf("  --firmware <YYMMDD>       main board firmware reported by :FW1#\n");
	printf("  --no-mountinfo            do not answer :MountInfo# and :FW1#\n");
	printf("  --parked | --tracking | --away   initial mechanical state\n");
	printf("  --track-mode <0-4>        initial tracking rate\n");
	printf("  --guide <rrdd>            initial RA/DEC guide rate percent\n");
	printf("  --slew-rate <deg/s>       simulated GOTO speed\n");
	printf("  --altitude-limit <deg>    reject GOTO targets below this altitude\n");
	printf("  --tcp                     serve an opt-in localhost TCP transport\n");
	printf("  --ready-file <path>       write INDIGO_SIMULATOR_PORT after setup\n");
	printf("  --trace                   log protocol requests and replies\n");
}

static bool parse_protocol(const char *value) {
	static const char *names[] = { "8406", "8407", "0100", "0200", "0205", "0300" };
	for (int i = 0; i < 6; i++) {
		if (!strcmp(value, names[i])) {
			options.protocol = (dialect)i;
			return true;
		}
	}
	return false;
}

static bool parse_args(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		const char *value = i + 1 < argc ? argv[i + 1] : NULL;
		if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
			usage(argv[0]);
			exit(0);
		} else if (!strcmp(arg, "--headless")) {
			options.headless = true;
			options.trace = false;
		} else if (!strcmp(arg, "--trace")) {
			options.trace = true;
		} else if (!strcmp(arg, "--tcp")) {
			options.tcp = true;
		} else if (!strcmp(arg, "--no-mountinfo")) {
			options.mountinfo = false;
		} else if (!strcmp(arg, "--parked")) {
			state.parked = true;
			state.homed = false;
		} else if (!strcmp(arg, "--tracking")) {
			state.tracking = true;
			state.homed = false;
		} else if (!strcmp(arg, "--away")) {
			state.homed = false;
		} else if (value == NULL) {
			fprintf(stderr, "Unknown or incomplete option '%s'\n", arg);
			return false;
		} else if (!strcmp(arg, "--protocol")) {
			if (!parse_protocol(value)) {
				fprintf(stderr, "--protocol requires 8406, 8407, 0100, 0200, 0205 or 0300\n");
				return false;
			}
			i++;
		} else if (!strcmp(arg, "--product")) {
			options.product = atoi(value);
			i++;
		} else if (!strcmp(arg, "--firmware")) {
			snprintf(options.firmware, sizeof(options.firmware), "%.6s", value);
			i++;
		} else if (!strcmp(arg, "--track-mode")) {
			state.tracking_mode = atoi(value);
			i++;
		} else if (!strcmp(arg, "--guide")) {
			state.guide_ra = atoi(value) / 100;
			state.guide_dec = atoi(value) % 100;
			i++;
		} else if (!strcmp(arg, "--slew-rate")) {
			options.slew_deg_per_s = atof(value);
			i++;
		} else if (!strcmp(arg, "--altitude-limit")) {
			options.altitude_limit = atof(value);
			i++;
		} else if (!strcmp(arg, "--ready-file")) {
			options.ready_file = value;
			i++;
		} else {
			fprintf(stderr, "Unknown option '%s'\n", arg);
			return false;
		}
	}
	return true;
}

static void signal_handler(int sig) {
	(void)sig;
	running = 0;
}

// ---------------------------------------------------------------- output and events

static void log_event(const char *prefix, const char *command) {
	if (events != NULL) {
		fprintf(events, "%.9f\t%s%s\n", serial_motion_time(), prefix, command);
		fflush(events);
	}
}

static void write_response(const char *response) {
	if (options.trace) {
		fprintf(stderr, "<- %s\n", response);
	}
	const char *cursor = response;
	size_t remaining = strlen(response);
	while (running && serial_fd >= 0 && remaining > 0) {
		ssize_t written = write(serial_fd, cursor, remaining);
		if (written > 0) {
			cursor += written;
			remaining -= (size_t)written;
		} else if (written < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
			usleep(1000);
		} else {
			break;
		}
	}
}

// Rejects an unsupported command: real firmware ignores it.
static void unsupported(void) {
	log_event("?", current_command);
}

// Acknowledges a documented set command with its "1"/"0" reply.
static void ack(bool valid) {
	if (!valid) {
		log_event("!", current_command);
	}
	write_response(valid ? "1" : "0");
}

// Malformed command documented without a reply.
static void malformed_silent(void) {
	log_event("!", current_command);
}

// ---------------------------------------------------------------- payload parsers

static bool parse_digits(const char *text, int count, long *value) {
	long result = 0;
	for (int i = 0; i < count; i++) {
		if (!isdigit((unsigned char)text[i])) {
			return false;
		}
		result = result * 10 + text[i] - '0';
	}
	if (text[count] != 0) {
		return false;
	}
	*value = result;
	return true;
}

static bool parse_signed_digits(const char *text, int count, long *value) {
	if (*text != '+' && *text != '-') {
		return false;
	}
	long magnitude;
	if (!parse_digits(text + 1, count, &magnitude)) {
		return false;
	}
	*value = *text == '-' ? -magnitude : magnitude;
	return true;
}

// " sDD*MM:SS" / " sDDD*MM:SS" with optional leading space and optional sign.
static bool parse_sexagesimal_degrees(const char *text, int degree_digits, bool sign_required, double *degrees) {
	if (*text == ' ') {
		text++;
	}
	int sign = 1;
	if (*text == '+' || *text == '-') {
		sign = *text == '-' ? -1 : 1;
		text++;
	} else if (sign_required) {
		return false;
	}
	for (int i = 0; i < degree_digits; i++) {
		if (!isdigit((unsigned char)text[i])) {
			return false;
		}
	}
	const char *rest = text + degree_digits;
	if (rest[0] != '*' || !isdigit((unsigned char)rest[1]) || !isdigit((unsigned char)rest[2]) || rest[3] != ':' || !isdigit((unsigned char)rest[4]) || !isdigit((unsigned char)rest[5]) || rest[6] != 0) {
		return false;
	}
	int d = atoi(text);
	int m = (rest[1] - '0') * 10 + rest[2] - '0';
	int s = (rest[4] - '0') * 10 + rest[5] - '0';
	if (m > 59 || s > 59) {
		return false;
	}
	*degrees = sign * (d + m / 60.0 + s / 3600.0);
	return true;
}

// " HH:MM:SS" or " HH:MM:SS.S".
static bool parse_sexagesimal_hours(const char *text, bool tenths, double *hours) {
	if (*text == ' ') {
		text++;
	}
	size_t length = tenths ? 10 : 8;
	if (strlen(text) != length || text[2] != ':' || text[5] != ':' || (tenths && text[8] != '.')) {
		return false;
	}
	for (size_t i = 0; i < length; i++) {
		if (i == 2 || i == 5 || (tenths && i == 8)) {
			continue;
		}
		if (!isdigit((unsigned char)text[i])) {
			return false;
		}
	}
	int h = atoi(text);
	int m = atoi(text + 3);
	double s = atof(text + 6);
	if (h > 23 || m > 59 || s >= 60) {
		return false;
	}
	*hours = h + m / 60.0 + s / 3600.0;
	return true;
}

// ---------------------------------------------------------------- clock, site and geometry

static double mount_unix_time(void) {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return now.tv_sec + now.tv_nsec / 1e9 + state.clock_delta;
}

static double local_sidereal_hours(void) {
	double jd = mount_unix_time() / 86400.0 + 2440587.5;
	double gmst = fmod(18.697374558 + 24.06570982441908 * (jd - 2451545.0), 24.0);
	double lst = fmod(gmst + state.longitude / 15.0, 24.0);
	return lst < 0 ? lst + 24 : lst;
}

static double wrap_ra(double mas) {
	mas = fmod(mas, RA_FULL_MAS);
	return mas < 0 ? mas + RA_FULL_MAS : mas;
}

static double hour_angle_hours(double ra_mas) {
	double ha = local_sidereal_hours() - ra_mas / 54000000.0;
	while (ha < -12) {
		ha += 24;
	}
	while (ha >= 12) {
		ha -= 24;
	}
	return ha;
}

static void equatorial_to_horizontal(double ra_mas, double dec_mas, double *alt, double *az) {
	double ha = hour_angle_hours(ra_mas) * M_PI / 12.0;
	double dec = dec_mas / 3600000.0 * M_PI / 180.0;
	double lat = state.latitude * M_PI / 180.0;
	double sin_alt = sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha);
	*alt = asin(fmax(-1, fmin(1, sin_alt))) * 180.0 / M_PI;
	double a = atan2(-cos(dec) * sin(ha), sin(dec) * cos(lat) - cos(dec) * sin(lat) * cos(ha)) * 180.0 / M_PI;
	*az = a < 0 ? a + 360 : a;
}

static void horizontal_to_equatorial(double alt_deg, double az_deg, double *ra_mas, double *dec_mas) {
	double alt = alt_deg * M_PI / 180.0;
	double az = az_deg * M_PI / 180.0;
	double lat = state.latitude * M_PI / 180.0;
	double sin_dec = sin(alt) * sin(lat) + cos(alt) * cos(lat) * cos(az);
	double dec = asin(fmax(-1, fmin(1, sin_dec)));
	double ha = atan2(-sin(az) * cos(alt), sin(alt) * cos(lat) - cos(alt) * sin(lat) * cos(az)) * 12.0 / M_PI;
	*dec_mas = dec * 180.0 / M_PI * 3600000.0;
	*ra_mas = wrap_ra((local_sidereal_hours() - ha) * 54000000.0);
}

static char hemisphere(void) {
	return state.latitude >= 0 ? '1' : '0';
}

// ---------------------------------------------------------------- mechanics

static double tracking_factor(void) {
	switch (state.tracking_mode) {
		case 1:
			return 0.96356;
		case 2:
			return 0.99727;
		case 3:
			return 0.99989;
		case 4:
			return options.protocol >= P0205 ? state.custom_rate / 10000.0 : 1.0 + state.rate_offset;
		default:
			return 1.0;
	}
}

static double pulse_overlap(int axis, double from, double to) {
	double end = state.pulse_end[axis];
	if (end <= from) {
		return 0;
	}
	return fmin(end, to) - from;
}

static void update_motion(void) {
	double now = serial_motion_time();
	double elapsed = now - last_update;
	double previous = last_update;
	last_update = now;
	serial_motion_update(&ra_motion);
	serial_motion_update(&dec_motion);
	bool active = ra_motion.duration > 0 || dec_motion.duration > 0;
	if (active) {
		slewing = true;
		return;
	}
	if (slewing) {
		slewing = false;
		serial_motion_sync(&ra_motion, wrap_ra(ra_motion.position));
		if (state.parking) {
			state.parking = false;
			state.parked = true;
			state.tracking = false;
		} else if (state.homing) {
			state.homing = false;
			state.homed = true;
			state.tracking = false;
		} else if (state.track_after_slew) {
			state.tracking = true;
		}
		state.track_after_slew = false;
	}
	double ra_rate = SIDEREAL_MAS_PER_S * (state.tracking && !state.parked ? 1.0 - tracking_factor() : 1.0);
	double dec_rate = 0;
	if (!state.parked) {
		ra_rate += SIDEREAL_MAS_PER_S * state.manual_ra;
		dec_rate += SIDEREAL_MAS_PER_S * state.manual_dec;
	}
	double ra = ra_motion.position + ra_rate * elapsed;
	double dec = dec_motion.position + dec_rate * elapsed;
	if (!state.parked) {
		ra += state.pulse_rate[0] * pulse_overlap(0, previous, now);
		dec += state.pulse_rate[1] * pulse_overlap(1, previous, now);
	}
	if (dec > DEC_MAX_MAS) {
		dec = DEC_MAX_MAS;
		state.manual_dec = 0;
	} else if (dec < -DEC_MAX_MAS) {
		dec = -DEC_MAX_MAS;
		state.manual_dec = 0;
	}
	serial_motion_sync(&ra_motion, wrap_ra(ra));
	serial_motion_sync(&dec_motion, dec);
}

static void start_slew(double ra_mas, double dec_mas) {
	double speed = options.slew_deg_per_s * 3600000.0;
	double position = ra_motion.position;
	double delta = ra_mas - position;
	if (delta > RA_FULL_MAS / 2) {
		ra_mas -= RA_FULL_MAS;
	} else if (delta < -RA_FULL_MAS / 2) {
		ra_mas += RA_FULL_MAS;
	}
	serial_motion_start(&ra_motion, ra_mas, speed);
	serial_motion_start(&dec_motion, dec_mas, speed);
	slewing = true;
	state.homed = false;
	state.parked = false;
	if (ra_motion.duration <= 0 && dec_motion.duration <= 0) {
		ra_motion.duration = 0.5;
		ra_motion.started = serial_motion_time();
		ra_motion.origin = ra_motion.target = ra_motion.position;
	}
}

static void stop_slew(void) {
	if (slewing) {
		serial_motion_stop(&ra_motion);
		serial_motion_stop(&dec_motion);
		serial_motion_sync(&ra_motion, wrap_ra(ra_motion.position));
		slewing = false;
	}
	state.parking = false;
	state.homing = false;
	state.track_after_slew = false;
}

static void stop_arrows(bool ra, bool dec) {
	if (ra) {
		state.manual_ra = 0;
	}
	if (dec) {
		state.manual_dec = 0;
	}
}

static void zero_position(double *ra_mas, double *dec_mas) {
	*ra_mas = wrap_ra(local_sidereal_hours() * 54000000.0);
	*dec_mas = state.latitude >= 0 ? DEC_MAX_MAS : -DEC_MAX_MAS;
}

static bool pulse_active(void) {
	double now = serial_motion_time();
	return state.pulse_end[0] > now || state.pulse_end[1] > now;
}

static char status_digit(void) {
	if (state.status_override) {
		return state.status_override;
	}
	if (state.parked) {
		return '6';
	}
	if (slewing) {
		return '2';
	}
	if (state.homed && options.protocol != P0100) {
		return '7';
	}
	if (state.tracking) {
		if (pulse_active()) {
			return '3';
		}
		return state.pec ? '5' : '1';
	}
	return '0';
}

static bool target_above_limit(double ra_mas, double dec_mas) {
	if (options.altitude_limit < -90) {
		return true;
	}
	double alt, az;
	equatorial_to_horizontal(ra_mas, dec_mas, &alt, &az);
	return alt >= options.altitude_limit;
}

// ---------------------------------------------------------------- formatting

static void format_sexagesimal(char *buffer, size_t size, double value, int degree_digits, bool tenths) {
	double magnitude = fabs(value);
	if (tenths) {
		long total = lround(magnitude * 36000.0) % 864000L;
		snprintf(buffer, size, "%02ld:%02ld:%02ld.%ld", total / 36000, (total / 600) % 60, (total / 10) % 60, total % 10);
	} else {
		long total = lround(magnitude * 3600.0);
		snprintf(buffer, size, "%c%0*ld*%02ld:%02ld", value < 0 ? '-' : '+', degree_digits, total / 3600, (total / 60) % 60, total % 60);
	}
}

static void format_signed(char *buffer, size_t size, long value, int digits) {
	snprintf(buffer, size, "%c%0*ld", value < 0 ? '-' : '+', digits, labs(value));
}

// ---------------------------------------------------------------- control file

static void add_rule(const char *command, const char *reply, int count) {
	size_t command_length = strlen(command), reply_length = strlen(reply);
	// A shortened rule would match the wrong command or answer the wrong bytes, and the test would
	// fail somewhere else, so refuse it and say so.
	if (command_length >= sizeof(rules[0].command) || reply_length >= sizeof(rules[0].reply)) {
		fprintf(stderr, "control: rule too long, command %zu of %zu bytes, reply %zu of %zu bytes\n", command_length, sizeof(rules[0].command), reply_length, sizeof(rules[0].reply));
		return;
	}
	for (int i = 0; i < MAX_RULES; i++) {
		if (rules[i].command[0] && !strcmp(rules[i].command, command)) {
			rules[i].command[0] = 0;
		}
	}
	if (!strcmp(reply, "CLEAR")) {
		return;
	}
	for (int i = 0; i < MAX_RULES; i++) {
		if (!rules[i].command[0]) {
			memcpy(rules[i].command, command, command_length + 1);
			memcpy(rules[i].reply, reply, reply_length + 1);
			rules[i].count = count;
			return;
		}
	}
}

// Atomic control-file rename in the test prevents partial injection requests.
static void read_control(void) {
	if (options.ready_file == NULL) {
		return;
	}
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s.control", options.ready_file);
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	char line[512];
	while (fgets(line, sizeof(line), file)) {
		line[strcspn(line, "\r\n")] = 0;
		char *reply = strchr(line, '\t');
		if (reply == NULL) {
			continue;
		}
		*reply++ = 0;
		char *count_text = strchr(reply, '\t');
		int count = 1;
		if (count_text != NULL) {
			*count_text++ = 0;
			count = atoi(count_text);
		}
		if (!strcmp(line, "@status")) {
			state.status_override = *reply == '-' ? 0 : *reply;
		} else if (!strcmp(line, "@close")) {
			if (options.tcp && serial_fd >= 0) {
				close(serial_fd);
				serial_fd = -1;
				log_event("", "CLOSE");
			}
		} else {
			add_rule(line, reply, count);
		}
	}
	fclose(file);
	unlink(path);
}

// Returns true when the command was fully handled by an injected rule.
static bool apply_rule(const char *command) {
	for (int i = 0; i < MAX_RULES; i++) {
		reply_rule *rule = rules + i;
		if (!rule->command[0] || strcmp(rule->command, command)) {
			continue;
		}
		char reply[128];
		snprintf(reply, sizeof(reply), "%s", rule->reply);
		if (rule->count > 0 && --rule->count == 0) {
			rule->command[0] = 0;
		}
		if (!strncmp(reply, "DELAY:", 6)) {
			usleep((useconds_t)atoi(reply + 6) * 1000);
			return false;
		}
		if (strcmp(reply, "DROP")) {
			write_response(reply);
		}
		return true;
	}
	return false;
}

// ---------------------------------------------------------------- command groups

static bool is(unsigned mask) {
	return (mask & (1u << options.protocol)) != 0;
}

static const char *default_firmware(void) {
	static const char *firmware[] = { "000000", "200115", "120807", "140807", "181018", "210605" };
	return *options.firmware ? options.firmware : firmware[options.protocol];
}

static int default_product(void) {
	static const int products[] = { 0, 8407, 60, 60, 40, 120 };
	return options.product >= 0 ? options.product : products[options.protocol];
}

static bool handle_identity(const char *command) {
	char response[64];
	if (!strcmp(command, "V")) {
		if (is(D8406 | D8407 | D0100 | D0200 | D0205)) {
			write_response("V1.00#");
		}
		return true;
	}
	if (!strcmp(command, "MountInfo")) {
		if (is(D8406) || !options.mountinfo) {
			unsupported();
		} else {
			snprintf(response, sizeof(response), "%04d", default_product());
			write_response(response);
		}
		return true;
	}
	if (!strcmp(command, "FW1") || !strcmp(command, "FW2")) {
		if (is(D8406) || !options.mountinfo) {
			unsupported();
		} else {
			snprintf(response, sizeof(response), "%s%s#", default_firmware(), default_firmware());
			write_response(response);
		}
		return true;
	}
	return false;
}

static bool handle_status(const char *command) {
	char response[128], a[32], b[32];
	double ra = ra_motion.position, dec = dec_motion.position;
	if (!strcmp(command, "SE?")) {
		if (is(D8406 | D8407)) {
			write_response(slewing ? "1" : "0");
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "AP") || !strcmp(command, "AT") || !strcmp(command, "QT")) {
		if (!is(D8407)) {
			unsupported();
		} else if (command[0] == 'A' && command[1] == 'P') {
			write_response(state.parked || state.parking ? "1" : "0");
		} else if (command[0] == 'A') {
			write_response(state.tracking ? "1" : "0");
		} else {
			snprintf(response, sizeof(response), "%d", state.tracking_mode);
			write_response(response);
		}
		return true;
	}
	if (!strcmp(command, "AH")) {
		if (is(D8407 | D0100)) {
			write_response(state.homed ? "1" : "0");
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "GAS")) {
		if (!is(D0100 | D0200)) {
			unsupported();
			return true;
		}
		snprintf(response, sizeof(response), "0%c%d%d1%c#", status_digit(), state.tracking_mode, state.moving_speed, hemisphere());
		write_response(response);
		return true;
	}
	if (!strcmp(command, "GLS")) {
		if (is(D0205)) {
			snprintf(response, sizeof(response), "%c%06ld%06ld0%c%d%d1%c#", state.longitude < 0 ? '-' : '+', lround(fabs(state.longitude) * 3600), lround((state.latitude + 90) * 3600), status_digit(), state.tracking_mode, state.moving_speed, hemisphere());
		} else if (is(D0300)) {
			snprintf(response, sizeof(response), "%c%08ld%08ld0%c%d%d1%c#", state.longitude < 0 ? '-' : '+', lround(fabs(state.longitude) * 360000), lround((state.latitude + 90) * 360000), status_digit(), state.tracking_mode, state.moving_speed, hemisphere());
		} else {
			unsupported();
			return true;
		}
		write_response(response);
		return true;
	}
	if (!strcmp(command, "GR") || !strcmp(command, "GD")) {
		if (!is(D8406 | D8407)) {
			unsupported();
			return true;
		}
		if (command[1] == 'R') {
			if (is(D8406)) {
				format_sexagesimal(a, sizeof(a), ra / 54000000.0, 2, true);
			} else {
				long total = lround(ra / 54000000.0 * 3600.0) % 86400;
				snprintf(a, sizeof(a), "%02ld:%02ld:%02ld", total / 3600, (total / 60) % 60, total % 60);
			}
		} else {
			format_sexagesimal(a, sizeof(a), dec / 3600000.0, 2, false);
		}
		snprintf(response, sizeof(response), "%s#", a);
		write_response(response);
		return true;
	}
	if (!strcmp(command, "pS")) {
		bool east = hour_angle_hours(ra) >= 0;
		if (is(D8406)) {
			write_response(east ? "East#" : "West#");
		} else if (is(D8407)) {
			write_response(east ? "0" : "1");
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "GEC")) {
		if (is(D0100)) {
			format_signed(a, sizeof(a), lround(dec / 10.0), 9);
			snprintf(b, sizeof(b), "%09ld", lround(ra / 15.0 / 10.0) % 864000000L);
		} else if (is(D0200 | D0205)) {
			format_signed(a, sizeof(a), lround(dec / 10.0), 8);
			snprintf(b, sizeof(b), "%08ld", lround(ra / 15.0) % 86400000L);
		} else {
			unsupported();
			return true;
		}
		snprintf(response, sizeof(response), "%s%s#", a, b);
		write_response(response);
		return true;
	}
	if (!strcmp(command, "GEP")) {
		if (!is(D0300)) {
			unsupported();
			return true;
		}
		format_signed(a, sizeof(a), lround(dec / 10.0), 8);
		snprintf(b, sizeof(b), "%09ld", lround(ra / 10.0) % 129600000L);
		snprintf(response, sizeof(response), "%s%s%c1#", a, b, hour_angle_hours(ra) >= 0 ? '0' : '1');
		write_response(response);
		return true;
	}
	if (!strcmp(command, "GAC")) {
		if (!is(D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		double alt, az;
		equatorial_to_horizontal(ra, dec, &alt, &az);
		format_signed(a, sizeof(a), lround(alt * 360000), is(D0100) ? 9 : 8);
		snprintf(b, sizeof(b), "%09ld", lround(az * 360000) % 129600000L);
		snprintf(response, sizeof(response), "%s%s#", a, b);
		write_response(response);
		return true;
	}
	if (!strcmp(command, "AG")) {
		if (is(D8407 | D0100)) {
			snprintf(response, sizeof(response), "%d.%02d#", state.guide_ra / 100, state.guide_ra % 100);
		} else if (is(D0200)) {
			snprintf(response, sizeof(response), "%03d#", state.guide_ra);
		} else if (is(D0205 | D0300)) {
			snprintf(response, sizeof(response), "%02d%02d#", state.guide_ra, state.guide_dec);
		} else {
			unsupported();
			return true;
		}
		write_response(response);
		return true;
	}
	if (!strcmp(command, "GTR")) {
		if (is(D0300)) {
			snprintf(response, sizeof(response), "%05d#", state.custom_rate);
			write_response(response);
		} else if (is(D8406)) {
			write_response(state.tracking_mode == 2 ? "1" : state.tracking_mode == 1 ? "2" : "0");
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "GMT")) {
		if (is(D0205 | D0300)) {
			snprintf(response, sizeof(response), "%d%02d#", state.meridian_flip, state.meridian_limit);
			write_response(response);
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "GPE") || !strcmp(command, "GPR")) {
		if (is(D0300)) {
			write_response(command[2] == 'E' ? "1" : state.pec_recording ? "1" : "0");
		} else {
			unsupported();
		}
		return true;
	}
	if (!strcmp(command, "GUT")) {
		if (!is(D0300)) {
			unsupported();
			return true;
		}
		snprintf(response, sizeof(response), "%c%03d%d%013lld#", state.utc_offset_minutes < 0 ? '-' : '+', abs(state.utc_offset_minutes), state.dst, (long long)llround((mount_unix_time() - J2000_UNIX) * 1000.0));
		write_response(response);
		return true;
	}
	return false;
}

static void set_target(double ra_mas, double dec_mas) {
	target_ra_mas = ra_mas;
	target_dec_mas = dec_mas;
	state.has_target = true;
}

static bool handle_target(const char *command) {
	long value;
	double number;
	if (!strncmp(command, "SRA", 3)) {
		if (!is(D0300)) {
			unsupported();
		} else if (parse_digits(command + 3, 9, &value) && value < 129600000L) {
			target_ra_mas = value * 10.0;
			ack(true);
		} else {
			ack(false);
		}
		return true;
	}
	if (!strncmp(command, "Sr", 2)) {
		bool valid;
		if (is(D8406 | D8407 | D0100)) {
			valid = parse_sexagesimal_hours(command + 2, is(D8406), &number) && command[2] == ' ';
			if (valid) {
				target_ra_mas = number * 54000000.0;
			}
		} else if (is(D0200 | D0205)) {
			valid = parse_digits(command + 2, 8, &value) && value < 86400000L;
			if (valid) {
				target_ra_mas = value * 15.0;
			}
		} else {
			unsupported();
			return true;
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "Sd", 2)) {
		bool valid;
		if (is(D8406 | D8407 | D0100)) {
			valid = command[2] == ' ' && parse_sexagesimal_degrees(command + 2, 2, false, &number) && fabs(number) <= 90;
			if (valid) {
				target_dec_mas = number * 3600000.0;
			}
		} else {
			valid = parse_signed_digits(command + 2, 8, &value) && labs(value) <= 32400000L;
			if (valid) {
				target_dec_mas = value * 10.0;
			}
		}
		if (valid) {
			state.has_target = true;
		}
		ack(valid);
		return true;
	}
	return false;
}

static void sync_to_target(void) {
	serial_motion_sync(&ra_motion, wrap_ra(target_ra_mas));
	serial_motion_sync(&dec_motion, target_dec_mas);
	state.homed = false;
}

static bool handle_motion(const char *command) {
	long value;
	if (!strcmp(command, "MS") || !strcmp(command, "MS1")) {
		bool supported = command[2] ? is(D0300) : is(D8406 | D8407 | D0100 | D0200 | D0205);
		if (!supported) {
			unsupported();
			return true;
		}
		bool accepted = !state.parked && target_above_limit(target_ra_mas, target_dec_mas);
		if (accepted) {
			stop_slew();
			start_slew(target_ra_mas, target_dec_mas);
			state.track_after_slew = true;
		}
		if (is(D8406)) {
			write_response(accepted ? "0" : "1Object is below horizon        #");
		} else {
			write_response(accepted ? "1" : "0");
		}
		return true;
	}
	if (!strcmp(command, "CM") || !strcmp(command, "CMR")) {
		if (command[2] && !is(D8406)) {
			unsupported();
			return true;
		}
		if (!slewing || is(D8406)) {
			sync_to_target();
		}
		write_response(is(D8406) ? "Coordinates     matched.        #" : "1");
		return true;
	}
	if (!strcmp(command, "Q")) {
		stop_slew();
		if (is(D8406 | D0300)) {
			stop_arrows(true, true);
		}
		if (!is(D8406)) {
			write_response("1");
		}
		return true;
	}
	if (!strcmp(command, "q") || !strcmp(command, "qR") || !strcmp(command, "qD")) {
		bool supported = command[1] ? is(D0100 | D0200 | D0205 | D0300) : is(D8407 | D0100 | D0200 | D0205);
		if (!supported) {
			unsupported();
			return true;
		}
		stop_arrows(command[1] != 'D', command[1] != 'R');
		if (is(D0200 | D0205 | D0300)) {
			write_response("1");
		}
		return true;
	}
	if (command[0] == 'Q' && command[1] && strchr("nsew", command[1]) && command[2] == 0) {
		if (is(D8406)) {
			stop_arrows(command[1] == 'e' || command[1] == 'w', command[1] == 'n' || command[1] == 's');
		} else {
			unsupported();
		}
		return true;
	}
	if (command[0] == 'm' && command[1] && strchr("nsew", command[1]) && command[2] == 0) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		double speed = ARROW_SPEEDS[state.moving_speed];
		if (command[1] == 'n' || command[1] == 's') {
			state.manual_dec = command[1] == 'n' ? speed : -speed;
		} else {
			state.manual_ra = command[1] == 'e' ? speed : -speed;
		}
		return true;
	}
	if (command[0] == 'M' && command[1] && strchr("nsew", command[1])) {
		int axis = command[1] == 'n' || command[1] == 's' ? 1 : 0;
		int sign = command[1] == 'n' || command[1] == 'e' ? 1 : -1;
		if (is(D8406)) {
			if (command[2] != 0) {
				unsupported();
				return true;
			}
			double rate = state.moving_speed == 0 ? state.guide_ra / 100.0 : HC8406_SPEEDS[(state.moving_speed - 1) % 4];
			if (axis) {
				state.manual_dec = sign * rate;
			} else {
				state.manual_ra = sign * rate;
			}
			return true;
		}
		long limit = is(D8407 | D0100) ? 32767 : 99999;
		if (!parse_digits(command + 2, 5, &value) || value > limit) {
			malformed_silent();
			return true;
		}
		double rate = (axis ? state.guide_dec : state.guide_ra) / 100.0;
		if (!is(D0205 | D0300)) {
			rate = state.guide_ra / 100.0;
		}
		state.pulse_rate[axis] = sign * SIDEREAL_MAS_PER_S * rate;
		state.pulse_end[axis] = serial_motion_time() + value / 1000.0;
		return true;
	}
	return false;
}

static bool handle_park_home(const char *command) {
	long value;
	if (!strcmp(command, "MP1")) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		if (!state.parked && !state.parking) {
			double ra, dec;
			if (is(D0100 | D0200)) {
				ra = target_ra_mas;
				dec = target_dec_mas;
			} else if (is(D0205 | D0300)) {
				double alt = state.park_position_set ? state.park_alt : fabs(state.latitude);
				double az = state.park_position_set ? state.park_az : (state.latitude >= 0 ? 0 : 180);
				horizontal_to_equatorial(alt, az, &ra, &dec);
			} else {
				zero_position(&ra, &dec);
			}
			stop_slew();
			stop_arrows(true, true);
			start_slew(ra, dec);
			state.parking = true;
		}
		write_response("1");
		return true;
	}
	if (!strcmp(command, "PK")) {
		if (!is(D8406)) {
			unsupported();
			return true;
		}
		double ra, dec;
		zero_position(&ra, &dec);
		stop_slew();
		start_slew(ra, dec);
		state.parking = true;
		write_response("1");
		return true;
	}
	if (!strcmp(command, "MP0")) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		state.parked = false;
		write_response("1");
		return true;
	}
	if (!strcmp(command, "MH") || !strcmp(command, "MSH")) {
		bool supported = command[1] == 'S' ? is(D0200 | D0205 | D0300) : is(D8407 | D0100 | D0200 | D0205 | D0300);
		if (!supported) {
			unsupported();
			return true;
		}
		if (!state.parked) {
			double ra, dec;
			zero_position(&ra, &dec);
			stop_slew();
			stop_arrows(true, true);
			start_slew(ra, dec);
			state.homing = true;
			state.tracking = false;
		}
		write_response("1");
		return true;
	}
	if (!strncmp(command, "SPA", 3) || !strncmp(command, "SPH", 3)) {
		if (!is(D0205 | D0300)) {
			unsupported();
			return true;
		}
		bool azimuth = command[2] == 'A';
		bool valid = parse_digits(command + 3, azimuth ? 9 : 8, &value) && value <= (azimuth ? 129599999L : 32400000L);
		if (valid) {
			if (azimuth) {
				state.park_az = value / 360000.0;
			} else {
				state.park_alt = value / 360000.0;
			}
			state.park_position_set = true;
		}
		ack(valid);
		return true;
	}
	return false;
}

static bool handle_rates(const char *command) {
	long value;
	if (!strncmp(command, "RT", 2)) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 2, 1, &value) && value <= 4;
		if (valid) {
			state.tracking_mode = (int)value;
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "STR", 3)) {
		if (!is(D8406)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 3, 1, &value) && value <= 2;
		if (valid) {
			state.tracking_mode = value == 1 ? 2 : value == 2 ? 1 : 0;
		}
		ack(valid);
		return true;
	}
	if (!strcmp(command, "ST0") || !strcmp(command, "ST1")) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		if (!state.parked) {
			state.tracking = command[2] == '1';
			if (state.tracking) {
				state.homed = false;
			}
		}
		write_response("1");
		return true;
	}
	if (!strncmp(command, "SR", 2)) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 2, 1, &value) && value >= 1 && value <= 9;
		if (valid) {
			state.moving_speed = (int)value;
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "RC", 2) || !strcmp(command, "RG")) {
		if (!is(D8406)) {
			unsupported();
			return true;
		}
		if (!strcmp(command, "RG")) {
			state.moving_speed = 0;
			state.tracking = true;
		} else if (!strcmp(command, "RC")) {
			state.moving_speed = 2;
		} else if (parse_digits(command + 2, 1, &value) && value <= 3) {
			state.moving_speed = (int)value + 1;
		} else {
			malformed_silent();
		}
		return true;
	}
	if (!strncmp(command, "RG", 2)) {
		if (is(D8407 | D0100 | D0200)) {
			long maximum = is(D0100) ? 80 : 90;
			bool valid = parse_digits(command + 2, 3, &value) && ((value >= 10 && value <= maximum) || (is(D8407) && value == 100));
			if (valid) {
				state.guide_ra = state.guide_dec = (int)value;
			}
			ack(valid);
		} else if (is(D0205 | D0300)) {
			bool valid = parse_digits(command + 2, 4, &value) && value / 100 >= 1 && value / 100 <= 90 && value % 100 >= 10;
			if (valid) {
				state.guide_ra = (int)(value / 100);
				state.guide_dec = (int)(value % 100);
			}
			ack(valid);
		} else {
			unsupported();
		}
		return true;
	}
	if (!strncmp(command, "RR", 2)) {
		if (is(D8407 | D0100 | D0200)) {
			const char *payload = command + 2;
			bool valid = true;
			if (is(D8407 | D0100)) {
				valid = *payload++ == ' ';
			}
			valid = valid && strlen(payload) == 8 && (payload[0] == '+' || payload[0] == '-') && isdigit((unsigned char)payload[1]) && isdigit((unsigned char)payload[2]) && payload[3] == '.';
			for (int i = 4; valid && i < 8; i++) {
				valid = isdigit((unsigned char)payload[i]);
			}
			double offset = valid ? atof(payload) : 0;
			valid = valid && fabs(offset) <= 0.01 + 1e-9;
			if (valid) {
				state.rate_offset = offset;
			}
			ack(valid);
		} else if (is(D0205 | D0300)) {
			long minimum = is(D0205) ? 5000 : 1000;
			long maximum = is(D0205) ? 15000 : 19000;
			bool valid = parse_digits(command + 2, 5, &value) && value >= minimum && value <= maximum;
			if (valid) {
				state.custom_rate = (int)value;
			}
			ack(valid);
		} else {
			unsupported();
		}
		return true;
	}
	if (!strncmp(command, "SMT", 3)) {
		if (!is(D0205 | D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 3, 3, &value) && value / 100 <= 1;
		if (valid) {
			state.meridian_flip = (int)(value / 100);
			state.meridian_limit = (int)(value % 100);
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SPP", 3) || !strncmp(command, "SPR", 3)) {
		if (!is(D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 3, 1, &value) && value <= 1;
		if (valid) {
			if (command[2] == 'P') {
				state.pec = value == 1;
			} else {
				state.pec_recording = value == 1;
			}
		}
		ack(valid);
		return true;
	}
	return false;
}

static int days_in_month(int year, int month) {
	static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	return month == 2 && year % 4 == 0 ? 29 : days[month - 1];
}

static bool handle_time_site(const char *command) {
	long value;
	double number;
	if (!strncmp(command, "SUT", 3)) {
		if (!is(D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 3, 13, &value);
		if (valid) {
			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			state.clock_delta = J2000_UNIX + value / 1000.0 - (now.tv_sec + now.tv_nsec / 1e9);
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SDS", 3)) {
		if (!is(D8407 | D0100 | D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		bool valid = parse_digits(command + 3, 1, &value) && value <= 1;
		if (valid) {
			state.dst = (int)value;
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SG", 2)) {
		const char *payload = command + 2;
		bool valid;
		if (is(D8406)) {
			valid = *payload++ == ' ';
			int sign = 1;
			if (*payload == '+' || *payload == '-') {
				sign = *payload++ == '-' ? -1 : 1;
			}
			valid = valid && parse_digits(payload, 2, &value) && value <= 12;
			if (valid) {
				state.utc_offset_minutes = sign * (int)value * 60;
			}
		} else if (is(D8407 | D0100)) {
			valid = payload[0] == ' ' && (payload[1] == '+' || payload[1] == '-') && isdigit((unsigned char)payload[2]) && isdigit((unsigned char)payload[3]) && payload[4] == ':' && isdigit((unsigned char)payload[5]) && isdigit((unsigned char)payload[6]) && payload[7] == 0;
			int hours = valid ? atoi(payload + 2) : 0;
			valid = valid && hours <= (is(D8407) ? 12 : 13);
			if (valid) {
				state.utc_offset_minutes = (payload[1] == '-' ? -1 : 1) * (hours * 60 + atoi(payload + 5));
			}
		} else {
			valid = parse_signed_digits(payload, 3, &value) && value >= -720 && value <= 780;
			if (valid) {
				state.utc_offset_minutes = (int)value;
			}
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SC", 2)) {
		int year = 0, month = 0, day = 0;
		bool valid;
		if (is(D8406 | D8407 | D0100)) {
			const char *p = command + 2;
			valid = strlen(p) == 9 && p[0] == ' ' && p[3] == '/' && p[6] == '/' && isdigit((unsigned char)p[1]) && isdigit((unsigned char)p[2]) && isdigit((unsigned char)p[4]) && isdigit((unsigned char)p[5]) && isdigit((unsigned char)p[7]) && isdigit((unsigned char)p[8]);
			if (valid) {
				month = atoi(p + 1);
				day = atoi(p + 4);
				year = atoi(p + 7);
			}
		} else if (is(D0200 | D0205)) {
			valid = parse_digits(command + 2, 6, &value);
			year = (int)(value / 10000);
			month = (int)(value / 100 % 100);
			day = (int)(value % 100);
		} else {
			unsupported();
			return true;
		}
		valid = valid && month >= 1 && month <= 12 && day >= 1 && day <= days_in_month(year, month);
		if (!valid) {
			log_event("!", command);
		}
		if (is(D8406)) {
			write_response("                                #                                #");
		} else {
			write_response(valid ? "1" : "0");
		}
		return true;
	}
	if (!strncmp(command, "SL", 2) && strncmp(command, "SLA", 3) && strncmp(command, "SLO", 3)) {
		bool valid;
		if (is(D8406 | D8407 | D0100)) {
			valid = command[2] == ' ' && parse_sexagesimal_hours(command + 2, false, &number);
		} else if (is(D0200 | D0205)) {
			valid = parse_digits(command + 2, 6, &value) && value / 10000 <= 23 && value / 100 % 100 <= 59 && value % 100 <= 59;
		} else {
			unsupported();
			return true;
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SLA", 3) || !strncmp(command, "SLO", 3)) {
		if (!is(D0300)) {
			unsupported();
			return true;
		}
		bool latitude = command[2] == 'A';
		bool valid = parse_signed_digits(command + 3, 8, &value) && labs(value) <= (latitude ? 32400000L : 64800000L);
		if (valid) {
			if (latitude) {
				state.latitude = value / 360000.0;
			} else {
				state.longitude = value / 360000.0;
			}
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "St", 2) || !strncmp(command, "Sg", 2)) {
		bool latitude = command[1] == 't';
		bool valid;
		if (is(D8406 | D8407 | D0100)) {
			valid = command[2] == ' ' && parse_sexagesimal_degrees(command + 2, latitude ? 2 : 3, false, &number) && fabs(number) <= (latitude ? 90 : 180);
		} else if (is(D0200 | D0205)) {
			valid = parse_signed_digits(command + 2, 6, &value) && labs(value) <= (latitude ? 324000L : 648000L);
			number = value / 3600.0;
		} else {
			unsupported();
			return true;
		}
		if (valid) {
			if (latitude) {
				state.latitude = number;
			} else {
				state.longitude = number;
			}
		}
		ack(valid);
		return true;
	}
	if (!strncmp(command, "SHE", 3)) {
		if (!is(D0200 | D0205 | D0300)) {
			unsupported();
			return true;
		}
		ack(parse_digits(command + 3, 1, &value) && value <= 1);
		return true;
	}
	return false;
}

static void handle_command(const char *command) {
	update_motion();
	read_control();
	log_event("", command);
	if (apply_rule(command)) {
		update_motion();
		return;
	}
	if (options.trace) {
		fprintf(stderr, "-> :%s#\n", command);
	}
	current_command = command;
	if (handle_identity(command) || handle_status(command) || handle_target(command) || handle_motion(command) || handle_park_home(command) || handle_rates(command) || handle_time_site(command)) {
		return;
	}
	unsupported();
}

static void handle_byte(char ch, char *buffer, size_t *length) {
	// A leading ':' frames a new command; ':' inside a command (sexagesimal
	// coordinates and times) is a payload character and must be kept.
	if (ch == ':' && *length == 0) {
		return;
	}
	if (ch == '#') {
		buffer[*length] = 0;
		handle_command(buffer);
		*length = 0;
		return;
	}
	if (*length + 1 < 128) {
		buffer[(*length)++] = ch;
	}
}

int main(int argc, char *argv[]) {
	char port[PATH_MAX] = { 0 };
	if (!parse_args(argc, argv)) {
		return 2;
	}
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);
	if (options.tcp) {
		listen_fd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in address = { 0 };
		address.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
		if (listen_fd < 0 || bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(listen_fd, 4) < 0) {
			perror("iOptron simulator TCP listen");
			return 1;
		}
		socklen_t size = sizeof(address);
		if (getsockname(listen_fd, (struct sockaddr *)&address, &size) < 0) {
			return 1;
		}
		snprintf(port, sizeof(port), "ieq://127.0.0.1:%u", ntohs(address.sin_port));
	} else {
		serial_fd = serial_simulator_open_pty(port, sizeof(port));
		if (serial_fd < 0) {
			return 1;
		}
	}
	if (options.ready_file != NULL) {
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s.events", options.ready_file);
		events = fopen(path, "w");
	}
	double ra, dec;
	if (state.homed) {
		zero_position(&ra, &dec);
	} else {
		ra = wrap_ra(local_sidereal_hours() * 54000000.0 - 2 * 54000000.0);
		dec = 20 * 3600000.0;
	}
	serial_motion_sync(&ra_motion, ra);
	serial_motion_sync(&dec_motion, dec);
	target_ra_mas = ra;
	target_dec_mas = dec;
	last_update = serial_motion_time();
	if (options.ready_file != NULL && !serial_simulator_write_ready_file(options.ready_file, simulator_name, port)) {
		return 1;
	}
	if (!options.headless) {
		printf("iOptron mount simulator ready on %s\n", port);
	}
	char command[128] = { 0 };
	size_t command_length = 0;
	while (running) {
		int active_fd = serial_fd >= 0 ? serial_fd : listen_fd;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(active_fd, &readfds);
		struct timeval timeout = { 0, 100000 };
		int selected = select(active_fd + 1, &readfds, NULL, NULL, &timeout);
		if (selected < 0) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		if (selected == 0 || !FD_ISSET(active_fd, &readfds)) {
			update_motion();
			read_control();
			continue;
		}
		if (serial_fd < 0) {
			serial_fd = accept(listen_fd, NULL, NULL);
			command_length = 0;
			if (serial_fd >= 0) {
				log_event("", "CONNECT");
			}
			continue;
		}
		char buffer[128];
		ssize_t count = read(serial_fd, buffer, sizeof(buffer));
		if (count > 0) {
			for (ssize_t i = 0; i < count; i++) {
				handle_byte(buffer[i], command, &command_length);
			}
		} else if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO)) {
			// No client on the pseudo terminal: keep mechanics and test control alive.
			update_motion();
			read_control();
			usleep(10000);
		} else if (count == 0) {
			if (options.tcp) {
				close(serial_fd);
				serial_fd = -1;
				log_event("", "CLOSE");
			} else {
				update_motion();
				read_control();
				usleep(10000);
			}
		} else {
			break;
		}
	}
	if (listen_fd >= 0) {
		close(listen_fd);
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events != NULL) {
		fclose(events);
	}
	return 0;
}
