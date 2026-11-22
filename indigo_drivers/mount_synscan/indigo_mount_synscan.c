// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_mount_synscan.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <indigo/indigo_align.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_uni_io.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_synscan.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000022
#define DRIVER_NAME          "indigo_mount_synscan"
#define DRIVER_LABEL         "SynScan Mount"
#define MOUNT_DEVICE_NAME    "Mount SynScan"
#define GUIDER_DEVICE_NAME   "Mount SynScan (guider)"
#define AUX_DEVICE_NAME      "Mount SynScan (aux)"
#define PRIVATE_DATA         ((synscan_private_data *)device->private_data)

//+ define

#define SIDEREAL_RATE        ((360.0 * 3600.0) / 86164.090530833)
#define LUNAR_RATE           14.511415
#define SOLAR_RATE           15.0

#define SYNSCAN_DEFAULT_UDP_PORT 11880
#define SYNSCAN_UDP_DISCOVERY_ATTEMPTS 8
#define SYNSCAN_UDP_DISCOVERY_RETRY_DELAY INDIGO_DELAY(1)
#define SYNSCAN_UDP_COMMAND_ATTEMPTS 3
#define SYNSCAN_UDP_REPLY_TIMEOUT INDIGO_DELAY(0.2)
#define SYNSCAN_COMMAND_TIMEOUT INDIGO_DELAY(1)
#define SYNSCAN_POLL_GUIDE_GUARD 0.2
#define SYNSCAN_POLL_AFTER_GUIDE 0.05
#define SYNSCAN_POLL_GUIDE_STALE 2.0
#define SYNSCAN_UDP_STRAY_WINDOW 0.5
#define SYNSCAN_MAX_STRAYS   16
#define SYNSCAN_UDP_MARKER   ":g1"
#define SYNSCAN_NEXT_BYTE_TIMEOUT INDIGO_DELAY(0.1)
// A motion in progress stays BUSY over this many consecutive polls whose position readback failed, the next poll
// retries; one more failure ends it ALERT.
#define POSITION_READ_RETRIES 2

#define SYNSCAN_STATUS_INITIALIZED 0x0100
#define SYNSCAN_STATUS_RUNNING 0x0001
#define SYNSCAN_HIGH_SPEED_GOTO_RAMP 3200
#define SYNSCAN_LOW_SPEED_GOTO_RAMP 200

#define SYNSCAN_SIDE_EAST    0
#define SYNSCAN_SIDE_WEST    1

#define MOUNT_POLARSCOPE_PROPERTY_NAME "X_POLARSCOPE"
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME "BRIGHTNESS"
#define MOUNT_USE_ENCODERS_PROPERTY_NAME "X_MOUNT_USE_ENCODERS"
#define MOUNT_USE_RA_ENCODER_ITEM_NAME "RA"
#define MOUNT_USE_DEC_ENCODER_ITEM_NAME "DEC"
#define MOUNT_AUTOHOME_PROPERTY_NAME "X_MOUNT_AUTOHOME"
#define MOUNT_AUTOHOME_ITEM_NAME "AUTOHOME"
#define MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME "X_MOUNT_AUTOHOME_SETTINGS"
#define MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME "DEC_OFFSET"
#define MOUNT_OPERATING_MODE_PROPERTY_NAME "X_MOUNT_OPERATING_MODE"
#define POLAR_MODE_ITEM_NAME "POLAR"
#define ALTAZ_MODE_ITEM_NAME "ALTAZ"
#define MOUNT_ENCODER_POSITION_PROPERTY_NAME "X_MOUNT_ENCODER_POSITION"
#define MOUNT_ENCODER_POSITION_RA_ITEM_NAME "RA"
#define MOUNT_ENCODER_POSITION_DEC_ITEM_NAME "DEC"

typedef enum {
	SYNSCAN_AXIS_RA = '1',
	SYNSCAN_AXIS_DEC = '2'
} synscan_axis;

typedef enum {
	SYNSCAN_GLOBAL_IDLE = 0,
	SYNSCAN_GLOBAL_SLEWING,
	SYNSCAN_GLOBAL_PARKING,
	SYNSCAN_GLOBAL_HOMING
} synscan_global_mode;

typedef enum {
	SYNSCAN_AXIS_IDLE = 0,
	SYNSCAN_AXIS_TRACKING,
	SYNSCAN_AXIS_GUIDING,
	SYNSCAN_AXIS_MANUAL_SLEWING,
	SYNSCAN_AXIS_GOTO
} synscan_axis_mode;

typedef enum {
	SYNSCAN_DIRECTION_FORWARD = 0,
	SYNSCAN_DIRECTION_REVERSE = 1
} synscan_direction;

typedef enum {
	HOME_IDLE,
	HOME_ENCODER_RA,
	HOME_ENCODER_DEC,
	HOME_INITIAL_STOP_RA,
	HOME_INITIAL_STOP_DEC,
	HOME_COUNTER_RA,
	HOME_COUNTER_DEC,
	HOME_PREP_RESET,
	HOME_PREP_INDEX,
	HOME_PREP_POSITION,
	HOME_PREP_WAIT,
	HOME_PREP_READ,
	HOME_PREP_SECOND_RESET,
	HOME_PREP_SECOND_INDEX,
	HOME_PREP_REVERSE,
	HOME_PREP_REVERSE_INDEX,
	HOME_PREP_DWELL,
	HOME_PREP_REVERSE_RESET,
	HOME_PREP_DONE,
	HOME_FORWARD_RA,
	HOME_FORWARD_DEC,
	HOME_FORWARD_INDEX_RA,
	HOME_FORWARD_INDEX_DEC,
	HOME_FORWARD_CHECK,
	HOME_FORWARD_WAIT_RA,
	HOME_FORWARD_WAIT_DEC,
	HOME_BACKOFF_RA,
	HOME_BACKOFF_DEC,
	HOME_BACKOFF_WAIT_RA,
	HOME_BACKOFF_WAIT_DEC,
	HOME_RETURN_RA,
	HOME_RETURN_DEC,
	HOME_RETURN_WAIT_RA,
	HOME_RETURN_WAIT_DEC,
	HOME_FINAL_COUNTER_RA,
	HOME_FINAL_COUNTER_DEC,
	HOME_COMPLETE,
	HOME_MOVE_STOP,
	HOME_WAIT,
	HOME_MOVE_POSITION,
	HOME_MOVE_MODE,
	HOME_MOVE_INCREMENT,
	HOME_MOVE_RAMP,
	HOME_MOVE_START,
	HOME_SEARCH_STATUS,
	HOME_SEARCH_MODE,
	HOME_SEARCH_PERIOD,
	HOME_SEARCH_START,
	HOME_SEARCH_STOP,
	HOME_INDEX_STOP
} synscan_home_stage;

enum {
	SYNSCAN_FEATURE_ENCODER = 0x0001,
	SYNSCAN_FEATURE_PPEC = 0x0002,
	SYNSCAN_FEATURE_HOME_INDEXER = 0x0004,
	SYNSCAN_FEATURE_AZEQ = 0x0008,
	SYNSCAN_FEATURE_IN_PPEC_TRAINING = 0x0010,
	SYNSCAN_FEATURE_IN_PPEC = 0x0020,
	SYNSCAN_FEATURE_POLAR_LED = 0x1000,
	SYNSCAN_FEATURE_COMMON_SLEW_START = 0x2000,
	SYNSCAN_FEATURE_HALF_CURRENT_TRACKING = 0x4000
};

enum {
	SYNSCAN_EXT_GET_HOME_INDEX = 0x0000,
	SYNSCAN_EXT_GET_FEATURES = 0x0001,
	SYNSCAN_EXT_START_PPEC_TRAINING = 0x0000,
	SYNSCAN_EXT_STOP_PPEC_TRAINING = 0x0001,
	SYNSCAN_EXT_TURN_PPEC_ON = 0x0002,
	SYNSCAN_EXT_TURN_PPEC_OFF = 0x0003,
	SYNSCAN_EXT_TURN_ENCODER_ON = 0x0004,
	SYNSCAN_EXT_TURN_ENCODER_OFF = 0x0005,
	SYNSCAN_EXT_RESET_HOME_INDEXER = 0x0008
};

//- define

#pragma mark - Property definitions

#define MOUNT_POLARSCOPE_PROPERTY        (PRIVATE_DATA->mount_polarscope_property)
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM (MOUNT_POLARSCOPE_PROPERTY->items + 0)

#define MOUNT_USE_ENCODERS_PROPERTY    (PRIVATE_DATA->mount_use_encoders_property)
#define MOUNT_USE_RA_ENCODER_ITEM      (MOUNT_USE_ENCODERS_PROPERTY->items + 0)
#define MOUNT_USE_DEC_ENCODER_ITEM     (MOUNT_USE_ENCODERS_PROPERTY->items + 1)

#define MOUNT_AUTOHOME_PROPERTY        (PRIVATE_DATA->mount_autohome_property)
#define MOUNT_AUTOHOME_ITEM            (MOUNT_AUTOHOME_PROPERTY->items + 0)

#define MOUNT_AUTOHOME_SETTINGS_PROPERTY (PRIVATE_DATA->mount_autohome_settings_property)
#define MOUNT_AUTOHOME_DEC_OFFSET_ITEM   (MOUNT_AUTOHOME_SETTINGS_PROPERTY->items + 0)

#define MOUNT_OPERATING_MODE_PROPERTY  (PRIVATE_DATA->mount_operating_mode_property)
#define POLAR_MODE_ITEM                (MOUNT_OPERATING_MODE_PROPERTY->items + 0)
#define ALTAZ_MODE_ITEM                (MOUNT_OPERATING_MODE_PROPERTY->items + 1)

#define MOUNT_ENCODER_POSITION_PROPERTY (PRIVATE_DATA->mount_encoder_position_property)
#define MOUNT_ENCODER_POSITION_RA_ITEM  (MOUNT_ENCODER_POSITION_PROPERTY->items + 0)
#define MOUNT_ENCODER_POSITION_DEC_ITEM (MOUNT_ENCODER_POSITION_PROPERTY->items + 1)

#define CCD_ABORT_EXPOSURE_PROPERTY    (PRIVATE_DATA->ccd_abort_exposure_property)
#define CCD_ABORT_EXPOSURE_ITEM        (CCD_ABORT_EXPOSURE_PROPERTY->items + 0)

#define CCD_EXPOSURE_PROPERTY          (PRIVATE_DATA->ccd_exposure_property)
#define CCD_EXPOSURE_ITEM              (CCD_EXPOSURE_PROPERTY->items + 0)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *mount_polarscope_property;
	indigo_property *mount_use_encoders_property;
	indigo_property *mount_autohome_property;
	indigo_property *mount_autohome_settings_property;
	indigo_property *mount_operating_mode_property;
	indigo_property *mount_encoder_position_property;
	indigo_property *ccd_abort_exposure_property;
	indigo_property *ccd_exposure_property;
	//+ data
	bool udp;
	bool configured;
	bool parked;
	bool homed;
	bool snap_port_supported;
	bool abort_motion;
	// a home, auto home or park request claimed SYNSCAN_GLOBAL_HOMING or SYNSCAN_GLOBAL_PARKING when it was admitted, its
	// handler has not run yet
	bool home_claimed;
	bool autohome_claimed;
	bool park_claimed;
	// UDP sends whose reply has not been read yet, in the order they were sent: the command, the payload length of its
	// reply and the time after which the reply is not expected any more
	// the time the last command was written, a guide pulse starts at its period change or start command
	double last_send_time;
	int stray_count;
	char stray_command[SYNSCAN_MAX_STRAYS][16];
	int stray_length[SYNSCAN_MAX_STRAYS];
	double stray_deadline[SYNSCAN_MAX_STRAYS];
	synscan_home_stage home_stage;
	synscan_home_stage home_resume;
	synscan_home_stage home_move_resume;
	synscan_home_stage home_search_resume;
	synscan_axis home_axis;
	synscan_axis home_prepare_axis;
	synscan_direction home_direction;
	bool home_slewing_up;
	bool home_extra_offset;
	bool home_found[2];
	long home_index[2];
	long home_target;
	long home_delta;
	double home_deadline;
	double home_index_deadline;
	int model_code;
	synscan_global_mode global_mode;
	// an admitted sync waits for its handler; the position poll keeps its BUSY instead of publishing OK over it
	bool sync_pending;
	// consecutive failed position readbacks of the poll, see POSITION_READ_RETRIES
	int position_failures;
	double motion_deadline;
	synscan_axis_mode ra_axis_mode;
	synscan_axis_mode dec_axis_mode;
	long ra_total_steps;
	long dec_total_steps;
	long ra_worm_steps;
	long dec_worm_steps;
	long ra_timer_frequency;
	long dec_timer_frequency;
	long ra_high_speed_ratio;
	long dec_high_speed_ratio;
	long ra_features;
	long dec_features;
	long ra_home_position;
	long dec_home_position;
	long ra_zero_position;
	long dec_zero_position;
	long ra_axis_position;
	long dec_axis_position;
	bool ra_axis_config_valid;
	bool dec_axis_config_valid;
	int ra_axis_config_mode;
	int dec_axis_config_mode;
	synscan_direction ra_axis_config_direction;
	synscan_direction dec_axis_config_direction;
	long ra_axis_config_period;
	long dec_axis_config_period;
	double ra_position;
	double dec_position;
	double ra_target_position;
	double dec_target_position;
	double guide_ra_deadline;
	double guide_dec_deadline;
	double guide_ra_resume_rate;
	int guide_ra_direction;
	int guide_dec_direction;
	double current_tracking_rate;
	bool ra_encoder;
	bool dec_encoder;
	int last_error;
	//- data
} synscan_private_data;

#pragma mark - Low level code

//+ code

static void aux_ccd_exposure_handler(indigo_device *device);
static void mount_equatorial_coordinates_handler(indigo_device *device);
static void mount_home_handler(indigo_device *device);
static void mount_autohome_handler(indigo_device *device);
static void mount_use_encoders_handler(indigo_device *device);
static void mount_track_rate_handler(indigo_device *device);
static void mount_tracking_handler(indigo_device *device);
static void mount_motion_dec_handler(indigo_device *device);
static void mount_motion_ra_handler(indigo_device *device);
static void mount_park_handler(indigo_device *device);

static const char hex_digits[] = "0123456789ABCDEF";

typedef enum {
	SYNSCAN_RESPONSE_OK,
	SYNSCAN_RESPONSE_COMMAND_ERROR,
	SYNSCAN_RESPONSE_IO_ERROR
} synscan_response_result;

static bool synscan_validate_handle(indigo_device *device);
static void synscan_handle_connection_loss(indigo_device *device);

static double synscan_now(void) {
	return indigo_monotonic_time();
}

// The payload length of the reply to a command: a setting is answered with "=" alone, a query with a fixed number
// of hex digits. A datagram of another length is a late reply to an earlier command.
static int synscan_reply_length(const char *command) {
	switch (command[1]) {
		case 'f':
			return 3;
		case 'g':
			return 2;
		case 'D':
			return 6;
	}
	return command[1] >= 'a' && command[1] <= 'z' ? 6 : 0;
}

static int synscan_hex_value(char value) {
	if (value >= '0' && value <= '9') {
		return value - '0';
	}
	if (value >= 'A' && value <= 'F') {
		return value - 'A' + 10;
	}
	if (value >= 'a' && value <= 'f') {
		return value - 'a' + 10;
	}
	return 0;
}

static const char *synscan_hex24(long value) {
	static INDIGO_THREAD_LOCAL char buffer[7];
	buffer[1] = hex_digits[value & 0xF];
	value >>= 4;
	buffer[0] = hex_digits[value & 0xF];
	value >>= 4;
	buffer[3] = hex_digits[value & 0xF];
	value >>= 4;
	buffer[2] = hex_digits[value & 0xF];
	value >>= 4;
	buffer[5] = hex_digits[value & 0xF];
	value >>= 4;
	buffer[4] = hex_digits[value & 0xF];
	buffer[6] = 0;
	return buffer;
}

static long synscan_hex24_to_long(const char *buffer) {
	long value = 0;
	value |= synscan_hex_value(buffer[4]);
	value <<= 4;
	value |= synscan_hex_value(buffer[5]);
	value <<= 4;
	value |= synscan_hex_value(buffer[2]);
	value <<= 4;
	value |= synscan_hex_value(buffer[3]);
	value <<= 4;
	value |= synscan_hex_value(buffer[0]);
	value <<= 4;
	value |= synscan_hex_value(buffer[1]);
	return value;
}

static long synscan_hex_to_long(const char *buffer) {
	long value = 0;
	while (*buffer != 0) {
		value <<= 4;
		value |= synscan_hex_value(*buffer++);
	}
	return value;
}

static long synscan_hex12_to_long(const char *buffer) {
	return (synscan_hex_value(buffer[2]) << 8) | (synscan_hex_value(buffer[0]) << 4) | synscan_hex_value(buffer[1]);
}

// The payload length of a reply datagram, -1 for anything that is not a "=...\r" reply.
static int synscan_datagram_length(const char *buffer, long count) {
	return count >= 2 && buffer[0] == '=' && buffer[count - 1] == '\r' ? (int)count - 2 : -1;
}

// Removes outstanding send `index`.
static void synscan_remove_stray(indigo_device *device, int index) {
	for (int i = index + 1; i < PRIVATE_DATA->stray_count; i++) {
		memcpy(PRIVATE_DATA->stray_command[i - 1], PRIVATE_DATA->stray_command[i], sizeof(PRIVATE_DATA->stray_command[i]));
		PRIVATE_DATA->stray_length[i - 1] = PRIVATE_DATA->stray_length[i];
		PRIVATE_DATA->stray_deadline[i - 1] = PRIVATE_DATA->stray_deadline[i];
	}
	PRIVATE_DATA->stray_count--;
}

// Forgets the outstanding sends whose reply is not expected any more.
static void synscan_expire_strays(indigo_device *device) {
	double now = indigo_monotonic_time();
	for (int i = PRIVATE_DATA->stray_count - 1; i >= 0; i--) {
		if (PRIVATE_DATA->stray_deadline[i] <= now) {
			synscan_remove_stray(device, i);
		}
	}
}

// Remembers a send of `command`, whose reply may arrive until SYNSCAN_UDP_STRAY_WINDOW from now; with no room left the
// oldest send is forgotten.
static void synscan_add_stray(indigo_device *device, const char *command) {
	synscan_expire_strays(device);
	if (PRIVATE_DATA->stray_count == SYNSCAN_MAX_STRAYS) {
		synscan_remove_stray(device, 0);
	}
	int index = PRIVATE_DATA->stray_count++;
	snprintf(PRIVATE_DATA->stray_command[index], sizeof(PRIVATE_DATA->stray_command[index]), "%s", command);
	PRIVATE_DATA->stray_length[index] = synscan_reply_length(command);
	PRIVATE_DATA->stray_deadline[index] = indigo_monotonic_time() + SYNSCAN_UDP_STRAY_WINDOW;
}

// Accounts a reply datagram of `length`: it answers the oldest outstanding send of `command` with that reply length, or
// else the oldest outstanding send of any command with that length. Taking the oldest send keeps the later deadlines,
// which cover the send whose reply is really still on its way.
static void synscan_take_stray(indigo_device *device, int length, const char *command) {
	int oldest = -1;
	synscan_expire_strays(device);
	for (int i = 0; i < PRIVATE_DATA->stray_count; i++) {
		if (PRIVATE_DATA->stray_length[i] == length) {
			if (command != NULL && !strcmp(PRIVATE_DATA->stray_command[i], command)) {
				oldest = i;
				break;
			}
			if (oldest < 0) {
				oldest = i;
			}
		}
	}
	if (oldest >= 0) {
		synscan_remove_stray(device, oldest);
	}
}

// The time until which a reply of the same length to another command may still arrive, 0 when none is outstanding.
static double synscan_foreign_stray_deadline(indigo_device *device, const char *command) {
	int length = synscan_reply_length(command);
	double deadline = 0;
	synscan_expire_strays(device);
	for (int i = 0; i < PRIVATE_DATA->stray_count; i++) {
		if (PRIVATE_DATA->stray_length[i] == length && strcmp(PRIVATE_DATA->stray_command[i], command) && PRIVATE_DATA->stray_deadline[i] > deadline) {
			deadline = PRIVATE_DATA->stray_deadline[i];
		}
	}
	return deadline;
}

// Reads and drops the datagrams that arrived before a UDP command is sent. A query first waits until the outstanding
// replies of its own length to other commands, which it could not tell from its own reply, have arrived or their
// time is up. A late reply to the same query is a valid answer to it, a reply of another length is told apart by the
// length, so neither makes it wait. A setting does not wait either: its "=" carries no value.
static void synscan_drain_udp(indigo_device *device, const char *command) {
	char late[32];
	double deadline = synscan_reply_length(command) > 0 ? synscan_foreign_stray_deadline(device, command) : 0;
	while (true) {
		double now = indigo_monotonic_time();
		long wait = deadline > now ? (long)((deadline - now) * 1000000.0) : 0;
		if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, wait) <= 0) {
			break;
		}
		long count = indigo_uni_read(PRIVATE_DATA->handle, late, sizeof(late) - 1);
		if (count <= 0) {
			break;
		}
		synscan_take_stray(device, synscan_datagram_length(late, count), command);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Discarded late SynScan reply %.*s before %s", (int)count - 1, late, command);
		deadline = synscan_reply_length(command) > 0 ? synscan_foreign_stray_deadline(device, command) : 0;
	}
}

// After a UDP command was sent more than once, asks for the high speed ratio of RA, whose reply of two hex digits no
// other command of a session gets, and reads until it arrives. The controller answers requests in the order it
// receives them and the link keeps that order (no reply overtook another in 1500 pipelined pairs on the AZ-GTi), so
// once this reply is in, no reply to an earlier send can still come: those sends are forgotten and the next query of
// the same length does not wait for them. Without that reply they stay outstanding until their deadlines. A marker
// whose own earlier send is still outstanding could be answered by that send, so then none is sent.
static void synscan_resync_udp(indigo_device *device) {
	char buffer[32];
	synscan_expire_strays(device);
	for (int i = 0; i < PRIVATE_DATA->stray_count; i++) {
		if (!strcmp(PRIVATE_DATA->stray_command[i], SYNSCAN_UDP_MARKER)) {
			return;
		}
	}
	synscan_drain_udp(device, SYNSCAN_UDP_MARKER);
	if (indigo_uni_write(PRIVATE_DATA->handle, SYNSCAN_UDP_MARKER "\r", (long)strlen(SYNSCAN_UDP_MARKER "\r")) <= 0) {
		return;
	}
	synscan_add_stray(device, SYNSCAN_UDP_MARKER);
	double deadline = indigo_monotonic_time() + SYNSCAN_UDP_REPLY_TIMEOUT / 1000000.0;
	while (true) {
		long remaining = (long)((deadline - indigo_monotonic_time()) * 1000000.0);
		if (remaining <= 0 || indigo_uni_wait_for_data(PRIVATE_DATA->handle, remaining) <= 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No reply to the SynScan order marker, earlier sends stay outstanding");
			return;
		}
		long count = indigo_uni_read(PRIVATE_DATA->handle, buffer, sizeof(buffer) - 1);
		int length = synscan_datagram_length(buffer, count);
		if (length == synscan_reply_length(SYNSCAN_UDP_MARKER)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SynScan order marker answered, %d earlier send(s) forgotten", PRIVATE_DATA->stray_count - 1);
			PRIVATE_DATA->stray_count = 0;
			return;
		}
		synscan_take_stray(device, length, NULL);
	}
}

static synscan_response_result synscan_read_response(indigo_device *device, const char *command, char *response, size_t response_size) {
	char buffer[32] = { 0 };
	long count;
	if (PRIVATE_DATA->udp) {
		// Wait for the datagram before reading it. A bare read on a socket with a receive
		// timeout reports the timeout as a read error, which latches on the handle and
		// makes every later read and write fail without touching the socket, so a single
		// lost reply would silence the mount for the rest of the session and the retry
		// below could never recover it.
		// A lost datagram costs one SYNSCAN_UDP_REPLY_TIMEOUT: an AZ-GTi answers in 10 ms, rarely in up to 0.4 s, and
		// loses about one reply in a hundred. A datagram whose length does not fit this command is a late reply to an
		// earlier one and is skipped; a late reply of the right length answers a repetition of this same command.
		double deadline = indigo_monotonic_time() + SYNSCAN_UDP_REPLY_TIMEOUT / 1000000.0;
		while (true) {
			long remaining = (long)((deadline - indigo_monotonic_time()) * 1000000.0);
			if (remaining <= 0 || indigo_uni_wait_for_data(PRIVATE_DATA->handle, remaining) <= 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SynScan response before timeout");
				return SYNSCAN_RESPONSE_IO_ERROR;
			}
			count = indigo_uni_read(PRIVATE_DATA->handle, buffer, sizeof(buffer) - 1);
			int length = synscan_datagram_length(buffer, count);
			if (length >= 0 && length != synscan_reply_length(command)) {
				synscan_take_stray(device, length, NULL);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Ignoring late SynScan reply %.*s to %s", (int)count - 1, buffer, command);
				continue;
			}
			break;
		}
	} else {
		count = indigo_uni_read_section2(PRIVATE_DATA->handle, buffer, sizeof(buffer) - 1, "\r", "", SYNSCAN_COMMAND_TIMEOUT, SYNSCAN_NEXT_BYTE_TIMEOUT);
	}
	if (count < 2) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SynScan response");
		return SYNSCAN_RESPONSE_IO_ERROR;
	}
	buffer[count] = 0;
	if (buffer[0] == '!') {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SynScan command failed with error %.2s", buffer + 1);
		PRIVATE_DATA->last_error = buffer[1] >= '0' && buffer[1] <= '9' ? buffer[1] - '0' : -1;
		return SYNSCAN_RESPONSE_COMMAND_ERROR;
	}
	if (buffer[0] != '=' || buffer[count - 1] != '\r') {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Invalid SynScan response: %.*s", (int)count, buffer);
		return SYNSCAN_RESPONSE_IO_ERROR;
	}
	if (response != NULL && response_size > 0) {
		size_t payload_size = (size_t)count - 2;
		if (payload_size >= response_size) {
			payload_size = response_size - 1;
		}
		memcpy(response, buffer + 1, payload_size);
		response[payload_size] = 0;
	}
	return SYNSCAN_RESPONSE_OK;
}

static bool synscan_command(indigo_device *device, const char *command, char *response, size_t response_size) {
	if (!synscan_validate_handle(device)) {
		return false;
	}
	bool udp = PRIVATE_DATA->handle->type == INDIGO_UDP_HANDLE;
	int attempts = udp ? SYNSCAN_UDP_COMMAND_ATTEMPTS : 1;
	PRIVATE_DATA->last_error = -1;
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%s\r", command);
	// A failed discard, write or reply only ends the session when the connection is lost, see
	// synscan_validate_handle(); a missing or garbled reply fails just this command.
	// Every UDP send is remembered until its reply is read or SYNSCAN_UDP_STRAY_WINDOW has passed: the AZ-GTi
	// delivered late replies up to 0.42 s after their command (one outlier 1.6 s). The reply the command ends on is
	// accounted to its oldest outstanding send, the others stay outstanding as strays.
	synscan_response_result result = SYNSCAN_RESPONSE_IO_ERROR;
	int sent = 0;
	for (int attempt = 0; attempt < attempts; attempt++) {
		if (udp) {
			// Late datagrams are read whole; indigo_uni_discard() waits 10 ms for more data before every command and
			// reads a datagram byte by byte.
			synscan_drain_udp(device, command);
		} else if (indigo_uni_discard(PRIVATE_DATA->handle) < 0) {
			synscan_validate_handle(device);
			return false;
		}
		if (indigo_uni_write(PRIVATE_DATA->handle, buffer, (long)strlen(buffer)) <= 0) {
			if (!udp) {
				synscan_validate_handle(device);
				return false;
			}
			continue;
		}
		PRIVATE_DATA->last_send_time = indigo_monotonic_time();
		sent++;
		if (udp) {
			synscan_add_stray(device, command);
		}
		result = synscan_read_response(device, command, response, response_size);
		if (result != SYNSCAN_RESPONSE_IO_ERROR) {
			if (udp) {
				synscan_take_stray(device, synscan_reply_length(command), command);
			}
			break;
		}
		if (!udp) {
			synscan_validate_handle(device);
		}
	}
	if (udp && sent > 1) {
		synscan_resync_udp(device);
	}
	return result == SYNSCAN_RESPONSE_OK;
}

// A reply payload of exactly `length` hex digits; a short or garbled reply is refused instead of being decoded.
static bool synscan_valid_payload(const char *command, const char *response, size_t length) {
	if (strlen(response) != length || strspn(response, "0123456789ABCDEFabcdef") != length) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Malformed SynScan reply to %s: '%s'", command, response);
		return false;
	}
	return true;
}

// The reason of the last failed command, as the motor controller command set lists its error codes.
static const char *synscan_failure_reason(indigo_device *device) {
	switch (PRIVATE_DATA->last_error) {
		case 0:
			return "unknown command";
		case 1:
			return "command length error";
		case 2:
			return "motor not stopped";
		case 3:
			return "invalid character";
		case 4:
			return "not initialized";
		case 5:
			return "driver sleeping";
		case 7:
			return "PEC training is running";
		case 8:
			return "no valid PEC data";
	}
	return "no valid reply";
}

static bool synscan_command_with_long_result(indigo_device *device, const char *command, long *value) {
	char response[16] = { 0 };
	if (!synscan_command(device, command, response, sizeof(response)) || !synscan_valid_payload(command, response, 6)) {
		return false;
	}
	if (value != NULL) {
		*value = synscan_hex24_to_long(response);
	}
	return true;
}

static bool synscan_command_with_code_result(indigo_device *device, const char *command, long *value) {
	char response[16] = { 0 };
	if (!synscan_command(device, command, response, sizeof(response)) || !synscan_valid_payload(command, response, 2)) {
		return false;
	}
	if (value != NULL) {
		*value = synscan_hex_to_long(response);
	}
	return true;
}

static bool synscan_firmware_version(indigo_device *device, long *version) {
	return synscan_command_with_long_result(device, ":e1", version);
}

static bool synscan_probe_handle(indigo_device *device) {
	long version = 0;
	return synscan_firmware_version(device, &version);
}

static bool synscan_axis_long_query(indigo_device *device, char command, synscan_axis axis, long *value) {
	char buffer[8];
	snprintf(buffer, sizeof(buffer), ":%c%c", command, axis);
	return synscan_command_with_long_result(device, buffer, value);
}

static bool synscan_axis_code_query(indigo_device *device, char command, synscan_axis axis, long *value) {
	char buffer[8];
	snprintf(buffer, sizeof(buffer), ":%c%c", command, axis);
	return synscan_command_with_code_result(device, buffer, value);
}

static bool synscan_axis_status_query(indigo_device *device, synscan_axis axis, long *value) {
	char response[16] = { 0 };
	char buffer[8];
	snprintf(buffer, sizeof(buffer), ":f%c", axis);
	if (!synscan_command(device, buffer, response, sizeof(response)) || !synscan_valid_payload(buffer, response, 3)) {
		return false;
	}
	if (value != NULL) {
		*value = synscan_hex12_to_long(response);
	}
	return true;
}

static bool synscan_axis_command(indigo_device *device, char command, synscan_axis axis) {
	char buffer[8];
	snprintf(buffer, sizeof(buffer), ":%c%c", command, axis);
	return synscan_command(device, buffer, NULL, 0);
}

static bool synscan_axis_setting(indigo_device *device, char command, synscan_axis axis, long value) {
	char buffer[16];
	snprintf(buffer, sizeof(buffer), ":%c%c%s", command, axis, synscan_hex24(value));
	return synscan_command(device, buffer, NULL, 0);
}

static bool synscan_set_axis_mode(indigo_device *device, synscan_axis axis, int mode, synscan_direction direction) {
	char buffer[8];
	snprintf(buffer, sizeof(buffer), ":G%c%X%X", axis, mode & 0xF, direction == SYNSCAN_DIRECTION_REVERSE ? 1 : 0);
	return synscan_command(device, buffer, NULL, 0);
}

static bool *synscan_axis_config_valid(indigo_device *device, synscan_axis axis) {
	return axis == SYNSCAN_AXIS_RA ? &PRIVATE_DATA->ra_axis_config_valid : &PRIVATE_DATA->dec_axis_config_valid;
}

static int *synscan_axis_config_mode(indigo_device *device, synscan_axis axis) {
	return axis == SYNSCAN_AXIS_RA ? &PRIVATE_DATA->ra_axis_config_mode : &PRIVATE_DATA->dec_axis_config_mode;
}

static synscan_direction *synscan_axis_config_direction(indigo_device *device, synscan_axis axis) {
	return axis == SYNSCAN_AXIS_RA ? &PRIVATE_DATA->ra_axis_config_direction : &PRIVATE_DATA->dec_axis_config_direction;
}

static long *synscan_axis_config_period(indigo_device *device, synscan_axis axis) {
	return axis == SYNSCAN_AXIS_RA ? &PRIVATE_DATA->ra_axis_config_period : &PRIVATE_DATA->dec_axis_config_period;
}

static void synscan_invalidate_axis_config(indigo_device *device, synscan_axis axis) {
	*synscan_axis_config_valid(device, axis) = false;
}

static void synscan_invalidate_axis_configs(indigo_device *device) {
	synscan_invalidate_axis_config(device, SYNSCAN_AXIS_RA);
	synscan_invalidate_axis_config(device, SYNSCAN_AXIS_DEC);
}

static bool synscan_axis_config_matches(indigo_device *device, synscan_axis axis, int mode, synscan_direction direction, long period) {
	return *synscan_axis_config_valid(device, axis)
		&& *synscan_axis_config_mode(device, axis) == mode
		&& *synscan_axis_config_direction(device, axis) == direction
		&& *synscan_axis_config_period(device, axis) == period;
}

static void synscan_cache_axis_config(indigo_device *device, synscan_axis axis, int mode, synscan_direction direction, long period) {
	*synscan_axis_config_valid(device, axis) = true;
	*synscan_axis_config_mode(device, axis) = mode;
	*synscan_axis_config_direction(device, axis) = direction;
	*synscan_axis_config_period(device, axis) = period;
}

static bool synscan_wait_axis_stopped(indigo_device *device, synscan_axis axis, bool *abort);

static bool synscan_stop_axis_and_wait(indigo_device *device, synscan_axis axis, bool *abort) {
	synscan_invalidate_axis_config(device, axis);
	return synscan_axis_command(device, 'K', axis) && synscan_wait_axis_stopped(device, axis, abort);
}

static double synscan_tracking_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
		return SOLAR_RATE;
	}
	if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
		return LUNAR_RATE;
	}
	return SIDEREAL_RATE;
}

// Read from the site at every use: a latitude set after connecting must not leave RA turning the northern way.
static bool synscan_southern_hemisphere(indigo_device *device) {
	device = device->master_device;
	return MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;
}

static long synscan_rate_to_period(indigo_device *device, synscan_axis axis, double rate, bool *high_speed, synscan_direction *direction) {
	if (synscan_southern_hemisphere(device) && axis == SYNSCAN_AXIS_RA) {
		rate = -rate;
	}
	*direction = rate < 0 ? SYNSCAN_DIRECTION_REVERSE : SYNSCAN_DIRECTION_FORWARD;
	rate = fabs(rate);
	*high_speed = rate > 128 * SIDEREAL_RATE;
	if (*high_speed) {
		long ratio = axis == SYNSCAN_AXIS_RA ? PRIVATE_DATA->ra_high_speed_ratio : PRIVATE_DATA->dec_high_speed_ratio;
		if (ratio > 0) {
			rate /= ratio;
		}
	}
	if (rate <= 0) {
		return 0;
	}
	double steps = axis == SYNSCAN_AXIS_RA ? PRIVATE_DATA->ra_total_steps : PRIVATE_DATA->dec_total_steps;
	double frequency = axis == SYNSCAN_AXIS_RA ? PRIVATE_DATA->ra_timer_frequency : PRIVATE_DATA->dec_timer_frequency;
	double counts_per_second = rate * steps / (360.0 * 3600.0);
	long period = counts_per_second > 0 ? lrint(frequency / counts_per_second) : 0;
	return period > 0 && period < 6 ? 6 : period;
}

static bool synscan_slew_axis_at_rate(indigo_device *device, synscan_axis axis, double rate) {
	if (rate == 0) {
		return synscan_stop_axis_and_wait(device, axis, NULL);
	}
	bool high_speed = false;
	synscan_direction direction = SYNSCAN_DIRECTION_FORWARD;
	long period = synscan_rate_to_period(device, axis, rate, &high_speed, &direction);
	if (period <= 0) {
		return false;
	}
	int mode = high_speed ? 3 : 1;
	long status = 0;
	bool running = false;
	if (!synscan_axis_status_query(device, axis, &status)) {
		return false;
	}
	running = (status & SYNSCAN_STATUS_RUNNING) != 0;
	// A running low-speed axis takes a new step period without stopping; only high speed or
	// a change of mode or direction needs the stop and the full :G, :I, :J sequence.
	if (running && mode == 1 && synscan_axis_config_matches(device, axis, mode, direction, *synscan_axis_config_period(device, axis))) {
		if (*synscan_axis_config_period(device, axis) == period) {
			return true;
		}
		if (!synscan_axis_setting(device, 'I', axis, period)) {
			synscan_invalidate_axis_config(device, axis);
			return false;
		}
		synscan_cache_axis_config(device, axis, mode, direction, period);
		return true;
	}
	if (!synscan_axis_config_matches(device, axis, mode, direction, period)) {
		if (running) {
			if (!synscan_stop_axis_and_wait(device, axis, NULL)) {
				return false;
			}
		}
		if (!synscan_set_axis_mode(device, axis, mode, direction) || !synscan_axis_setting(device, 'I', axis, period)) {
			synscan_invalidate_axis_config(device, axis);
			return false;
		}
		synscan_cache_axis_config(device, axis, mode, direction, period);
		running = false;
	}
	return running || synscan_axis_command(device, 'J', axis);
}

static bool synscan_update_mount_coordinates(indigo_device *device);
static bool synscan_ext_inquiry(indigo_device *device, synscan_axis axis, long inquiry, long *value);
static bool synscan_ext_setting(indigo_device *device, synscan_axis axis, long setting);
static void synscan_clear_tracking_state(indigo_device *device);
static long synscan_low_speed_goto_margin(indigo_device *device, synscan_axis axis);

static bool synscan_wait_axis_stopped(indigo_device *device, synscan_axis axis, bool *abort) {
	for (int i = 0; i < 300; i++) {
		long status = 0;
		if (abort != NULL && *abort) {
			return false;
		}
		if (!synscan_axis_status_query(device, axis, &status)) {
			return false;
		}
		if ((status & SYNSCAN_STATUS_RUNNING) == 0) {
			return true;
		}
		if (i % 5 == 0) {
			synscan_update_mount_coordinates(device);
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool synscan_slew_axis_to_steps(indigo_device *device, synscan_axis axis, long target) {
	long current = 0;
	// the slew owns the axis from now on, a guide pulse running on it ends without touching it
	if (axis == SYNSCAN_AXIS_RA) {
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_GOTO;
	} else {
		PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_GOTO;
	}
	if (!synscan_stop_axis_and_wait(device, axis, NULL) || !synscan_axis_long_query(device, 'j', axis, &current)) {
		return false;
	}
	long delta = target - current;
	if (delta == 0) {
		return true;
	}
	synscan_direction direction = delta < 0 ? SYNSCAN_DIRECTION_REVERSE : SYNSCAN_DIRECTION_FORWARD;
	delta = labs(delta);
	bool high_speed = delta > synscan_low_speed_goto_margin(device, axis);
	long ramp = high_speed ? SYNSCAN_HIGH_SPEED_GOTO_RAMP : SYNSCAN_LOW_SPEED_GOTO_RAMP;
	long slowdown = delta > ramp ? ramp : delta;
	return synscan_set_axis_mode(device, axis, high_speed ? 0 : 2, direction)
		&& synscan_axis_setting(device, 'H', axis, delta)
		&& synscan_axis_setting(device, 'M', axis, slowdown)
		&& synscan_axis_command(device, 'J', axis);
}

// The SLEW light is ALERT whenever MOUNT_EQUATORIAL_COORDINATES is, the ALERT that indigo_update_coordinates() keeps after
// a refused or failed request included, so it is derived from the state that function published: call it after
// indigo_update_coordinates() or indigo_reject_coordinates_change(), never between writing the state and publishing it.
static void synscan_update_mount_state(indigo_device *device) {
	MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
	MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
	MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
	MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_ALERT_STATE) {
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_SLEWING || PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_MANUAL_SLEWING || PRIVATE_DATA->dec_axis_mode == SYNSCAN_AXIS_MANUAL_SLEWING) {
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
	}
	if (MOUNT_PARK_PROPERTY->state == INDIGO_ALERT_STATE) {
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_PARKING) {
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->parked) {
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_ALERT_STATE) {
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_HOMING) {
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->homed) {
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
	}
	if (MOUNT_TRACKING_PROPERTY->state == INDIGO_ALERT_STATE) {
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
	}
	MOUNT_STATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
}

static void synscan_clear_tracking_state(indigo_device *device) {
	PRIVATE_DATA->current_tracking_rate = 0;
	if (PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_TRACKING || PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_GUIDING || PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_GOTO) {
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
	}
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	// A pending request owns the state, its handler reads the target and publishes the result
	if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	}
}

// Puts RA back on the tracking rate after a failed command left the axis in an unknown state: the cached axis
// configuration is forgotten, so the axis is stopped and gets the full mode, period and start sequence.
static bool synscan_restore_tracking(indigo_device *device, double rate) {
	synscan_invalidate_axis_config(device, SYNSCAN_AXIS_RA);
	if (!synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, rate)) {
		return false;
	}
	PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_TRACKING;
	return true;
}

// RA could not be put back on the tracking rate, the switch shows that the mount does not track; device is the mount.
static void synscan_tracking_lost(indigo_device *device, const char *message) {
	PRIVATE_DATA->current_tracking_rate = 0;
	PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	// A pending request owns the state, its handler reads the target and publishes the result
	if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, message);
	}
}

static long synscan_position_to_steps(long zero, long total, double position) {
	if (position > 0.75) {
		position -= 1.0;
	}
	return lrint(zero + position * total);
}

static long synscan_low_speed_goto_margin(indigo_device *device, synscan_axis axis) {
	long total_steps = axis == SYNSCAN_AXIS_RA ? PRIVATE_DATA->ra_total_steps : PRIVATE_DATA->dec_total_steps;
	return lrint(5 * 128 * SIDEREAL_RATE * total_steps / (360.0 * 3600.0));
}

static bool synscan_update_axis_positions(indigo_device *device) {
	// Always the step counters: ':d' counts in the resolution of the axis encoder (an AZ-GTi 1068 per revolution against
	// 2073600 steps), which the controller does not report. With X_MOUNT_USE_ENCODERS on, the firmware corrects the step
	// counters from the encoders itself.
	bool ok = synscan_axis_long_query(device, 'j', SYNSCAN_AXIS_DEC, &PRIVATE_DATA->dec_axis_position);
	ok = ok && synscan_axis_long_query(device, 'j', SYNSCAN_AXIS_RA, &PRIVATE_DATA->ra_axis_position);
	if (PRIVATE_DATA->ra_total_steps > 0) {
		PRIVATE_DATA->ra_position = (double)(PRIVATE_DATA->ra_axis_position - PRIVATE_DATA->ra_zero_position) / (double)PRIVATE_DATA->ra_total_steps;
	}
	if (PRIVATE_DATA->dec_total_steps > 0) {
		PRIVATE_DATA->dec_position = (double)(PRIVATE_DATA->dec_axis_position - PRIVATE_DATA->dec_zero_position) / (double)PRIVATE_DATA->dec_total_steps;
	}
	return ok;
}

static void synscan_encoder_to_eq(indigo_device *device, double ra_encoder, double dec_encoder, double *ha, double *dec, int *side_of_pier) {
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;
	if (dec_encoder < 0) {
		dec_encoder += 1.0;
	}
	bool west = false;
	if (dec_encoder < 0.25) {
		*dec = dec_encoder;
	} else if (dec_encoder < 0.75) {
		*dec = 0.5 - dec_encoder;
		west = true;
	} else {
		*dec = dec_encoder - 1.0;
	}
	if (south) {
		west = !west;
		*dec = -*dec;
	}
	*dec *= 2.0 * M_PI;
	// west means the OTA points west of the meridian, which puts it east of the pier
	*side_of_pier = west ? SYNSCAN_SIDE_EAST : SYNSCAN_SIDE_WEST;
	if (!south) {
		if (!west) {
			*ha = ra_encoder - 0.5;
		} else if (ra_encoder < 0.5) {
			*ha = ra_encoder;
		} else {
			*ha = ra_encoder - 1.0;
		}
	} else {
		if (west) {
			*ha = 0.5 - ra_encoder;
		} else if (ra_encoder < 0.5) {
			*ha = -ra_encoder;
		} else {
			*ha = 1.0 - ra_encoder;
		}
	}
	if (*ha < -0.5) {
		*ha += 1.0;
	}
	if (*ha >= 0.5) {
		*ha -= 1.0;
	}
	*ha *= 2.0 * M_PI;
}

static void synscan_eq_to_encoder(indigo_device *device, double ha, double dec, double ra_positions[2], double dec_positions[2]) {
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;
	double west_dec = M_PI - dec;
	if (south) {
		west_dec = dec < 0 ? -dec : 2.0 * M_PI - dec;
	}
	double east_dec = M_PI + dec;
	if (!south) {
		east_dec = dec < 0 ? 2.0 * M_PI + dec : dec;
	}
	if (west_dec > M_PI + M_PI / 2.0) {
		west_dec -= 2.0 * M_PI;
	}
	if (east_dec > M_PI + M_PI / 2.0) {
		east_dec -= 2.0 * M_PI;
	}
	while (ha > M_PI) {
		ha -= 2.0 * M_PI;
	}
	while (ha < -M_PI) {
		ha += 2.0 * M_PI;
	}
	double west_ra = ha >= 0 ? ha : ha + 2.0 * M_PI;
	if (south) {
		west_ra = M_PI - ha;
	}
	double east_ra = ha + M_PI;
	if (south) {
		east_ra = ha >= 0 ? 2.0 * M_PI - ha : -ha;
	}
	ra_positions[0] = west_ra / (2.0 * M_PI);
	dec_positions[0] = west_dec / (2.0 * M_PI);
	ra_positions[1] = east_ra / (2.0 * M_PI);
	dec_positions[1] = east_dec / (2.0 * M_PI);
}

static int synscan_select_encoder_solution(double ra_positions[2]) {
	return ra_positions[0] <= 0.5 ? 0 : 1;
}

static double synscan_range24(double hours) {
	double range = fmod(hours, 24.0);
	if (range < 0) {
		range += 24.0;
	}
	return range;
}

// MOUNT_PARK_SET and MOUNT_HOME_SET: the position is the mechanical hour angle and declination of the axes;
// DEFAULT is the home of an equatorial mount, CURRENT comes from the axis counters, which know the hour angle of the
// date, not from the J2000 coordinates of the bus the mount base driver would take it from.
static void synscan_set_position(indigo_device *device, indigo_property *set, indigo_item *default_item, indigo_item *current_item, indigo_property *position, indigo_item *ha_item, indigo_item *dec_item) {
	if (default_item->sw.value) {
		ha_item->number.value = 6;
		dec_item->number.value = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value > 0 ? 90 : -90;
	} else if (current_item->sw.value) {
		double ha = 0;
		double dec = 0;
		int side_of_pier = MOUNT_SIDE_EAST;
		// read the counters now rather than take the last poll's, which can be a second old or, after a failed
		// first reading at connect, those of a previous session
		if (!synscan_update_axis_positions(device)) {
			default_item->sw.value = false;
			current_item->sw.value = false;
			set->state = INDIGO_ALERT_STATE;
			INDIGO_UPDATE_PROPERTY_STATE(position, INDIGO_ALERT_STATE, "Failed to read the axis positions");
			return;
		}
		synscan_encoder_to_eq(device, PRIVATE_DATA->ra_position, PRIVATE_DATA->dec_position, &ha, &dec, &side_of_pier);
		ha_item->number.value = ha * 12.0 / M_PI;
		dec_item->number.value = dec * 180.0 / M_PI;
	} else {
		return;
	}
	default_item->sw.value = false;
	current_item->sw.value = false;
	position->state = INDIGO_OK_STATE;
	indigo_update_property(device, position, NULL);
}

static bool synscan_read_mount_coordinates(indigo_device *device) {
	// Reached from the guider's finalizers too, whose device_context is an indigo_guider_context,
	// so resolve the master device before touching any MOUNT_* property.
	device = device->master_device;
	if (!synscan_update_axis_positions(device)) {
		return false;
	}
	double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	double raw_ha = 0;
	double raw_dec = 0;
	int side_of_pier = MOUNT_SIDE_EAST;
	synscan_encoder_to_eq(device, PRIVATE_DATA->ra_position, PRIVATE_DATA->dec_position, &raw_ha, &raw_dec, &side_of_pier);
	// The axes know the hour angle of the date; the bus carries J2000 (MOUNT_EPOCH), so the raw and the
	// aligned coordinates and the alignment points are all J2000.
	double raw_ra_hours = synscan_range24(lst - raw_ha * 12.0 / M_PI);
	double raw_dec_degrees = raw_dec * 180.0 / M_PI;
	indigo_jnow_to_j2k(&raw_ra_hours, &raw_dec_degrees);
	MOUNT_RAW_COORDINATES_RA_ITEM->number.value = synscan_range24(raw_ra_hours);
	MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = raw_dec_degrees;
	indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, side_of_pier == MOUNT_SIDE_EAST ? MOUNT_SIDE_OF_PIER_EAST_ITEM : MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
	indigo_raw_to_translated_with_lst(device, lst, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, side_of_pier, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	PRIVATE_DATA->position_failures = 0;
	return true;
}

static bool synscan_update_mount_coordinates(indigo_device *device) {
	// Reached from the guider's finalizers too, whose device_context is an indigo_guider_context,
	// so resolve the master device before touching any MOUNT_* property.
	device = device->master_device;
	// A guider-only session or a connection in progress has no mount properties to publish
	if (!IS_CONNECTED || !synscan_read_mount_coordinates(device)) {
		return false;
	}
	indigo_property_state coordinates_state = PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE && !PRIVATE_DATA->sync_pending ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = coordinates_state;
	indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
	// the mount base driver derives MOUNT_HORIZONTAL_COORDINATES and MOUNT_LST_TIME from the equatorial
	// coordinates and publishes them with them
	indigo_update_coordinates(device, NULL);
	// after the publication, which turns the OK of the reading into a kept ALERT
	synscan_update_mount_state(device);
	return true;
}

// X_MOUNT_ENCODER_POSITION: the signed count of the encoder of each axis exactly as ':d' reports it (the 24-bit reply minus
// 0x800000), without scaling or a zero position; the resolution is model dependent, so the driver does not interpret
// it. An axis without an encoder shows 0.
static bool synscan_read_encoder_position(indigo_device *device) {
	long ra_count = 0x800000;
	long dec_count = 0x800000;
	if ((PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_ENCODER) && !synscan_axis_long_query(device, 'd', SYNSCAN_AXIS_DEC, &dec_count)) {
		return false;
	}
	if ((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_ENCODER) && !synscan_axis_long_query(device, 'd', SYNSCAN_AXIS_RA, &ra_count)) {
		return false;
	}
	MOUNT_ENCODER_POSITION_RA_ITEM->number.value = ra_count - 0x800000;
	MOUNT_ENCODER_POSITION_DEC_ITEM->number.value = dec_count - 0x800000;
	return true;
}

// Called by the poll after a successful synscan_update_mount_coordinates(); a failed query keeps the last values
// with ALERT, the next good poll restores OK. The bus publishes the property only when its values or state change.
static void synscan_update_encoder_position(indigo_device *device) {
	if (MOUNT_ENCODER_POSITION_PROPERTY->hidden) {
		return;
	}
	if (synscan_read_encoder_position(device)) {
		MOUNT_ENCODER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->handle != NULL && IS_CONNECTED) {
		MOUNT_ENCODER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		return;
	}
	indigo_update_property(device, MOUNT_ENCODER_POSITION_PROPERTY, NULL);
}

// The sign of a DEC axis rate that moves north. The axis counts declination the other way on the other side of the
// pier (synscan_encoder_to_eq), in both hemispheres: north is a reverse step with the OTA east of the pier, a
// forward step with the OTA west of the pier. Manual motion and guide pulses use the same mapping.
static double synscan_dec_north_sign(indigo_device *device) {
	device = device->master_device;
	return MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value ? 1 : -1;
}

// Published by indigo_update_coordinates(), which publishes MOUNT_HORIZONTAL_COORDINATES in the same state with them and
// keeps an ALERT that ends a BUSY motion until the next BUSY.
static void synscan_set_coordinate_state(indigo_device *device, indigo_property_state state) {
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = state;
	indigo_update_coordinates(device, NULL);
}

static void synscan_ha_dec_to_steps(indigo_device *device, double ha_hours, double dec_degrees, long *ra_steps, long *dec_steps) {
	double ra_positions[2];
	double dec_positions[2];
	synscan_eq_to_encoder(device, ha_hours * M_PI / 12.0, dec_degrees * M_PI / 180.0, ra_positions, dec_positions);
	int index = synscan_select_encoder_solution(ra_positions);
	if (ra_steps != NULL) {
		*ra_steps = synscan_position_to_steps(PRIVATE_DATA->ra_zero_position, PRIVATE_DATA->ra_total_steps, ra_positions[index]);
	}
	if (dec_steps != NULL) {
		*dec_steps = synscan_position_to_steps(PRIVATE_DATA->dec_zero_position, PRIVATE_DATA->dec_total_steps, dec_positions[index]);
	}
}

// The J2000 coordinates of the bus, through the alignment model, to the axis steps of their hour angle of the date
static void synscan_radec_to_steps(indigo_device *device, double ra_hours, double dec_degrees, long *ra_steps, long *dec_steps) {
	double raw_ra = ra_hours;
	double raw_dec = dec_degrees;
	indigo_translated_to_raw(device, ra_hours, dec_degrees, &raw_ra, &raw_dec);
	indigo_j2k_to_jnow(&raw_ra, &raw_dec);
	double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	synscan_ha_dec_to_steps(device, lst - raw_ra, raw_dec, ra_steps, dec_steps);
}

static bool synscan_ext_inquiry(indigo_device *device, synscan_axis axis, long inquiry, long *value) {
	char buffer[16];
	snprintf(buffer, sizeof(buffer), ":q%c%s", axis, synscan_hex24(inquiry));
	return synscan_command_with_long_result(device, buffer, value);
}

static bool synscan_ext_setting(indigo_device *device, synscan_axis axis, long setting) {
	char buffer[16];
	snprintf(buffer, sizeof(buffer), ":W%c%s", axis, synscan_hex24(setting));
	return synscan_command(device, buffer, NULL, 0);
}

// X_MOUNT_USE_ENCODERS switches the dual encoders of the firmware and nothing else. Turning them on makes the
// controller re-base the step counter of an axis from its encoder at once (an AZ-GTi snaps it to the encoder grid);
// the coordinates keep coming from the step counters. The items show what the controller accepted.
static bool synscan_set_encoders(indigo_device *device, bool ra, bool dec) {
	bool ok = synscan_ext_setting(device, SYNSCAN_AXIS_RA, ra ? SYNSCAN_EXT_TURN_ENCODER_ON : SYNSCAN_EXT_TURN_ENCODER_OFF);
	if (ok) {
		PRIVATE_DATA->ra_encoder = ra;
	}
	ok = ok && synscan_ext_setting(device, SYNSCAN_AXIS_DEC, dec ? SYNSCAN_EXT_TURN_ENCODER_ON : SYNSCAN_EXT_TURN_ENCODER_OFF);
	if (ok) {
		PRIVATE_DATA->dec_encoder = dec;
	}
	MOUNT_USE_RA_ENCODER_ITEM->sw.value = PRIVATE_DATA->ra_encoder;
	MOUNT_USE_DEC_ENCODER_ITEM->sw.value = PRIVATE_DATA->dec_encoder;
	return ok;
}

static void synscan_ppec_training_timer(indigo_device *device) {
	bool ok = synscan_ext_inquiry(device, SYNSCAN_AXIS_RA, SYNSCAN_EXT_GET_FEATURES, &PRIVATE_DATA->ra_features);
	ok = ok && synscan_ext_inquiry(device, SYNSCAN_AXIS_DEC, SYNSCAN_EXT_GET_FEATURES, &PRIVATE_DATA->dec_features);
	bool training = ok && ((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING));
	if (training) {
		indigo_execute_handler_in(device, 1, synscan_ppec_training_timer);
	} else if (ok) {
		indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
	}
	// A request copied while training ended owns the state, its handler reads the target and publishes the result
	if (!training && MOUNT_PEC_TRAINING_PROPERTY->state != INDIGO_BUSY_STATE) {
		MOUNT_PEC_TRAINING_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, ok ? "Cleared PPEC training state." : "Failed to read PPEC training state.");
	}
}

static bool synscan_set_polarscope_brightness(indigo_device *device, int brightness) {
	char buffer[8];
	if (brightness < 0) {
		brightness = 0;
	} else if (brightness > 255) {
		brightness = 255;
	}
	snprintf(buffer, sizeof(buffer), ":V1%02X", brightness);
	return synscan_command(device, buffer, NULL, 0);
}

static bool synscan_set_st4_guide_rate(indigo_device *device, synscan_axis axis, double rate) {
	char buffer[8];
	rate /= 100.0;
	int code = 2;
	if (rate >= 1.0) {
		code = 0;
	} else if (rate >= 0.75) {
		code = 1;
	} else if (rate >= 0.5) {
		code = 2;
	} else if (rate >= 0.25) {
		code = 3;
	} else {
		code = 4;
	}
	snprintf(buffer, sizeof(buffer), ":P%c%d", axis, code);
	return synscan_command(device, buffer, NULL, 0);
}

static void synscan_make_park_file_path(indigo_device *device, char *path, size_t path_size) {
	const char *config_folder = indigo_uni_config_folder();
	const char *model = *MOUNT_INFO_MODEL_ITEM->text.value ? MOUNT_INFO_MODEL_ITEM->text.value : "unknown";
	snprintf(path, path_size, "%s/synscan-%s.park", config_folder, model);
}

static bool synscan_save_position(indigo_device *device) {
	char path[INDIGO_VALUE_SIZE];
	char buffer[INDIGO_VALUE_SIZE];
	synscan_make_park_file_path(device, path, sizeof(path));
	indigo_uni_mkdir(indigo_uni_config_folder());
	indigo_uni_handle *file = indigo_uni_create_file(path, INDIGO_LOG_DEBUG);
	if (file == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't create %s", path);
		return false;
	}
	snprintf(buffer, sizeof(buffer), "%06lx %06lx\n", PRIVATE_DATA->ra_axis_position, PRIVATE_DATA->dec_axis_position);
	bool ok = indigo_uni_write(file, buffer, (long)strlen(buffer)) == (long)strlen(buffer);
	indigo_uni_close(&file);
	if (!ok) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't write %s", path);
	}
	return ok;
}

static bool synscan_restore_position(indigo_device *device, synscan_axis axis, bool remove_file) {
	char path[INDIGO_VALUE_SIZE];
	char buffer[INDIGO_VALUE_SIZE] = { 0 };
	long ra_position = PRIVATE_DATA->ra_home_position;
	long dec_position = PRIVATE_DATA->dec_home_position;
	synscan_make_park_file_path(device, path, sizeof(path));
	indigo_uni_handle *file = indigo_uni_open_file(path, INDIGO_LOG_DEBUG);
	if (file != NULL) {
		long count = indigo_uni_read(file, buffer, sizeof(buffer) - 1);
		if (count > 0) {
			buffer[count] = 0;
			if (sscanf(buffer, "%lx %lx", &ra_position, &dec_position) != 2) {
				ra_position = PRIVATE_DATA->ra_home_position;
				dec_position = PRIVATE_DATA->dec_home_position;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse saved SynScan park position: %s", buffer);
			}
		}
		indigo_uni_close(&file);
		if (remove_file) {
			indigo_uni_remove(path);
		}
	}
	char command[16];
	snprintf(command, sizeof(command), ":E%c%s", axis, synscan_hex24(axis == SYNSCAN_AXIS_RA ? ra_position : dec_position));
	return synscan_command(device, command, NULL, 0);
}

static bool synscan_has_snap_port(int model_code) {
	switch (model_code) {
	case 0x04: // EQ8
	case 0x05: // AZEQ6
	case 0x06: // AZEQ5
	case 0x0A: // Star Adventurer
	case 0x23: // EQ6-R Pro
	case 0xA5: // AZGTi
		return true;
	}
	return false;
}

static bool synscan_set_snap_port(indigo_device *device, bool enabled) {
	char command[8];
	snprintf(command, sizeof(command), ":O%c%c", SYNSCAN_AXIS_RA, enabled ? '1' : '0');
	return synscan_command(device, command, NULL, 0);
}

static void synscan_set_model_info(indigo_device *device, long version) {
	int release = (int)(version & 0xFF);
	int revision = (int)((version >> 8) & 0xFF);
	int model = (int)((version >> 16) & 0xFF);
	PRIVATE_DATA->model_code = model;
	PRIVATE_DATA->snap_port_supported = synscan_has_snap_port(model);
	snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%d.%02d", release, revision);
	snprintf(MOUNT_INFO_VENDOR_ITEM->text.value, INDIGO_VALUE_SIZE, "Sky-Watcher SynScan");
	switch (model) {
	case 0x00:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ6");
		break;
	case 0x01:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "HEQ5");
		break;
	case 0x02:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ5");
		break;
	case 0x03:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ3");
		break;
	case 0x04:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ8");
		break;
	case 0x05:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZEQ6");
		break;
	case 0x06:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZEQ5");
		break;
	case 0x0A:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "Star Adventurer");
		break;
	case 0x0C:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "Star Adventurer GTi");
		break;
	case 0x20:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ8-R Pro");
		break;
	case 0x22:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZEQ6 Pro");
		break;
	case 0x23:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ6-R Pro");
		break;
	case 0x24:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ6 Pro");
		break;
	case 0x25:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "CQ350 Pro");
		break;
	case 0x31:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ5 Pro");
		break;
	case 0x80:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "GT");
		break;
	case 0x81:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "MF");
		break;
	case 0x82:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "114GT");
		break;
	case 0x83:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "StarSeek");
		break;
	case 0x90:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "DOB");
		break;
	case 0xA2:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZ-GTe");
		break;
	case 0xA5:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZGTi");
		break;
	case 0x44:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "Wave 100i");
		break;
	case 0x45:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "Wave 150i");
		break;
	default:
		snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "CUSTOM (%02x)", model);
		break;
	}
}

static bool synscan_configure(indigo_device *device) {
	long version = 0;
	long ra_status = 0;
	long dec_status = 0;
	if (!synscan_firmware_version(device, &version)) {
		return false;
	}
	synscan_set_model_info(device, version);
	if (!synscan_axis_status_query(device, SYNSCAN_AXIS_RA, &ra_status)) {
		return false;
	}
	if (!synscan_axis_status_query(device, SYNSCAN_AXIS_DEC, &dec_status)) {
		return false;
	}
	if ((ra_status & SYNSCAN_STATUS_RUNNING) != 0) {
		synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_RA, NULL);
	}
	if ((dec_status & SYNSCAN_STATUS_RUNNING) != 0) {
		synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_DEC, NULL);
	}
	if (!synscan_axis_long_query(device, 'a', SYNSCAN_AXIS_RA, &PRIVATE_DATA->ra_total_steps)) {
		return false;
	}
	if (!synscan_axis_long_query(device, 'a', SYNSCAN_AXIS_DEC, &PRIVATE_DATA->dec_total_steps)) {
		return false;
	}
	synscan_axis_long_query(device, 's', SYNSCAN_AXIS_RA, &PRIVATE_DATA->ra_worm_steps);
	synscan_axis_long_query(device, 's', SYNSCAN_AXIS_DEC, &PRIVATE_DATA->dec_worm_steps);
	if (!synscan_axis_long_query(device, 'b', SYNSCAN_AXIS_RA, &PRIVATE_DATA->ra_timer_frequency)) {
		return false;
	}
	if (!synscan_axis_long_query(device, 'b', SYNSCAN_AXIS_DEC, &PRIVATE_DATA->dec_timer_frequency)) {
		return false;
	}
	if (!synscan_axis_code_query(device, 'g', SYNSCAN_AXIS_RA, &PRIVATE_DATA->ra_high_speed_ratio)) {
		return false;
	}
	if (!synscan_axis_code_query(device, 'g', SYNSCAN_AXIS_DEC, &PRIVATE_DATA->dec_high_speed_ratio)) {
		return false;
	}
	if (!synscan_ext_inquiry(device, SYNSCAN_AXIS_RA, SYNSCAN_EXT_GET_FEATURES, &PRIVATE_DATA->ra_features)) {
		PRIVATE_DATA->ra_features = 0;
	}
	if (!synscan_ext_inquiry(device, SYNSCAN_AXIS_DEC, SYNSCAN_EXT_GET_FEATURES, &PRIVATE_DATA->dec_features)) {
		PRIVATE_DATA->dec_features = 0;
	}
	MOUNT_OPERATING_MODE_PROPERTY->hidden = !((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_AZEQ) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_AZEQ));
	MOUNT_USE_ENCODERS_PROPERTY->hidden = !((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_ENCODER) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_ENCODER));
	MOUNT_ENCODER_POSITION_PROPERTY->hidden = MOUNT_USE_ENCODERS_PROPERTY->hidden;
	MOUNT_PEC_PROPERTY->hidden = MOUNT_PEC_TRAINING_PROPERTY->hidden = !((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_PPEC) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_PPEC));
	MOUNT_AUTOHOME_PROPERTY->hidden = MOUNT_AUTOHOME_SETTINGS_PROPERTY->hidden = !((PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_HOME_INDEXER) && (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_HOME_INDEXER));
	// The probe sets the brightness, the property shows the value the LED now has
	MOUNT_POLARSCOPE_PROPERTY->hidden = !synscan_set_polarscope_brightness(device, 0);
	if (!MOUNT_POLARSCOPE_PROPERTY->hidden) {
		MOUNT_POLARSCOPE_BRIGHTNESS_ITEM->number.value = MOUNT_POLARSCOPE_BRIGHTNESS_ITEM->number.target = 0;
	}
	if (!MOUNT_PEC_PROPERTY->hidden) {
		indigo_set_switch(MOUNT_PEC_PROPERTY, PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_IN_PPEC ? MOUNT_PEC_ENABLED_ITEM : MOUNT_PEC_DISABLED_ITEM, true);
	}
	if (!MOUNT_PEC_TRAINING_PROPERTY->hidden) {
		indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, (PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING) ? MOUNT_PEC_TRAINIG_STARTED_ITEM : MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
	}
	PRIVATE_DATA->ra_home_position = 0x800000;
	PRIVATE_DATA->dec_home_position = 0x800000 + (PRIVATE_DATA->dec_total_steps / 4);
	PRIVATE_DATA->ra_zero_position = PRIVATE_DATA->ra_home_position - (PRIVATE_DATA->ra_total_steps / 4);
	PRIVATE_DATA->dec_zero_position = PRIVATE_DATA->dec_home_position - (PRIVATE_DATA->dec_total_steps / 4);
	if ((ra_status & SYNSCAN_STATUS_INITIALIZED) == 0) {
		if (!synscan_axis_command(device, 'F', SYNSCAN_AXIS_RA)) {
			return false;
		}
		if (!synscan_restore_position(device, SYNSCAN_AXIS_RA, false)) {
			return false;
		}
	}
	if ((dec_status & SYNSCAN_STATUS_INITIALIZED) == 0) {
		if (!synscan_axis_command(device, 'F', SYNSCAN_AXIS_DEC)) {
			return false;
		}
		if (!synscan_restore_position(device, SYNSCAN_AXIS_DEC, true)) {
			return false;
		}
	}
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	PRIVATE_DATA->home_claimed = PRIVATE_DATA->autohome_claimed = PRIVATE_DATA->park_claimed = false;
	PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
	PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
	PRIVATE_DATA->position_failures = 0;
	PRIVATE_DATA->configured = true;
	PRIVATE_DATA->parked = false;
	PRIVATE_DATA->homed = false;
	PRIVATE_DATA->current_tracking_rate = 0;
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	// The controller keeps the dual encoder setting of the last command it got, which no query reports, so the item
	// (off in a new driver, the last selection after a reconnect, a saved selection only after a configuration load) is
	// sent before the first reading: the controller and the item agree and the coordinates carry any re-base.
	if (!MOUNT_USE_ENCODERS_PROPERTY->hidden) {
		MOUNT_USE_ENCODERS_PROPERTY->state = synscan_set_encoders(device, MOUNT_USE_RA_ENCODER_ITEM->sw.value, MOUNT_USE_DEC_ENCODER_ITEM->sw.value) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (PRIVATE_DATA->handle == NULL) {
			return false;
		}
	}
	// A bad first reading marks only the coordinates, the first poll reads them again; a lost port fails the connection
	bool coordinates_read = synscan_read_mount_coordinates(device);
	if (!coordinates_read) {
		if (PRIVATE_DATA->handle == NULL) {
			return false;
		}
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	// MOUNT_STATE is defined after this, with the SLEW light of the coordinates it is defined with
	MOUNT_STATE_SLEW_ITEM->light.value = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_ALERT_STATE ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE;
	// The encoder position is defined OK only with a reading of this session, otherwise with ALERT
	if (!MOUNT_ENCODER_POSITION_PROPERTY->hidden) {
		MOUNT_ENCODER_POSITION_PROPERTY->state = coordinates_read && synscan_read_encoder_position(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (PRIVATE_DATA->handle == NULL) {
			return false;
		}
	}
	return true;
}

static bool synscan_udp_discovery(char *host, size_t host_size) {
	char command[] = ":e1\r";
	char response[16] = { 0 };
	for (int attempt = 0; attempt < SYNSCAN_UDP_DISCOVERY_ATTEMPTS; attempt++) {
		host[0] = 0;
		response[0] = 0;
		if (indigo_perform_active_discovery("255.255.255.255", SYNSCAN_DEFAULT_UDP_PORT, 1, command, (int)strlen(command), host, (int)host_size, response, sizeof(response))) {
			size_t response_length = strlen(response);
			if (response_length >= 2 && response[0] == '=' && response[response_length - 1] == '\r') {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "SynScan mount detected by UDP broadcast at %s", host);
				return true;
			}
		}
		if (attempt + 1 < SYNSCAN_UDP_DISCOVERY_ATTEMPTS) {
			indigo_usleep(SYNSCAN_UDP_DISCOVERY_RETRY_DELAY);
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SynScan mount detected by UDP broadcast");
	return false;
}

static bool synscan_autodetect_udp(indigo_device *device) {
	char host[INDIGO_VALUE_SIZE] = { 0 };
	if (!synscan_udp_discovery(host, sizeof(host))) {
		return false;
	}
	char url[INDIGO_VALUE_SIZE];
	snprintf(url, sizeof(url), "udp://%s:%d", host, SYNSCAN_DEFAULT_UDP_PORT);
	PRIVATE_DATA->udp = true;
	PRIVATE_DATA->handle = indigo_uni_open_url(url, SYNSCAN_DEFAULT_UDP_PORT, INDIGO_UDP_HANDLE, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	indigo_uni_set_socket_read_timeout(PRIVATE_DATA->handle, SYNSCAN_COMMAND_TIMEOUT);
	indigo_uni_set_socket_write_timeout(PRIVATE_DATA->handle, SYNSCAN_COMMAND_TIMEOUT);
	if (!synscan_probe_handle(device)) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	snprintf(url, sizeof(url), "synscan://%s:%d", host, SYNSCAN_DEFAULT_UDP_PORT);
	INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, url);
	indigo_update_property(device, DEVICE_PORT_PROPERTY, "Mount detected by UDP broadcast");
	return true;
}

static bool synscan_open(indigo_device *device, const char **error_message) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (PRIVATE_DATA->handle != NULL) {
		return true;
	}
	if (!strncmp(name, "synscan://", 10)) {
		PRIVATE_DATA->udp = true;
		if (name[10] == 0) {
			return synscan_autodetect_udp(device);
		}
		char url[INDIGO_VALUE_SIZE];
		snprintf(url, sizeof(url), "udp://%s", name + 10);
		PRIVATE_DATA->handle = indigo_uni_open_url(url, SYNSCAN_DEFAULT_UDP_PORT, INDIGO_UDP_HANDLE, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			indigo_uni_set_socket_read_timeout(PRIVATE_DATA->handle, SYNSCAN_COMMAND_TIMEOUT);
			indigo_uni_set_socket_write_timeout(PRIVATE_DATA->handle, SYNSCAN_COMMAND_TIMEOUT);
		}
	} else {
		PRIVATE_DATA->udp = false;
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_config2(name, DEVICE_BAUDRATE_ITEM->text.value, INDIGO_LOG_DEBUG, error_message);
	}
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
	if (!synscan_probe_handle(device)) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No SynScan response from %s", name);
		return false;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s @ %s", name, PRIVATE_DATA->udp ? "UDP" : DEVICE_BAUDRATE_ITEM->text.value);
	return true;
}

static void synscan_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	PRIVATE_DATA->udp = false;
	PRIVATE_DATA->stray_count = 0;
}

static bool synscan_validate_handle(indigo_device *device) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (PRIVATE_DATA->handle->type == INDIGO_UDP_HANDLE) {
		return true;
	}
	// a device that is gone, an error left on the port, a connection reset or closed by the mount
	if (indigo_uni_is_connection_lost(PRIVATE_DATA->handle)) {
		synscan_handle_connection_loss(device);
		return false;
	}
	return true;
}

static void synscan_handle_connection_loss(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		if (PRIVATE_DATA->handle->type == INDIGO_UDP_HANDLE) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Temporary SynScan UDP communication failure");
			return;
		}
		bool was_connected = CONNECTION_CONNECTED_ITEM->sw.value && CONNECTION_PROPERTY->state == INDIGO_OK_STATE;
		indigo_device *master = device->master_device;
		synscan_close(master);
		if (was_connected) {
			indigo_device_disconnect(NULL, master->name);
			indigo_execute_handler(master, indigo_disconnect_slave_devices);
		}
	}
}

//- code

//+ mount.code

static void mount_autohome_finalizer(indigo_device *device);
static void mount_autohome_wait(indigo_device *device, synscan_axis axis, synscan_home_stage next) {
	PRIVATE_DATA->home_axis = axis;
	PRIVATE_DATA->home_resume = next;
	PRIVATE_DATA->home_deadline = indigo_monotonic_time() + 30;
	PRIVATE_DATA->home_stage = HOME_WAIT;
}
static void mount_autohome_move(indigo_device *device, synscan_axis axis, long target, synscan_home_stage next) {
	PRIVATE_DATA->home_axis = axis;
	PRIVATE_DATA->home_target = target;
	PRIVATE_DATA->home_move_resume = next;
	PRIVATE_DATA->home_stage = HOME_MOVE_STOP;
}
static void mount_autohome_search(indigo_device *device, synscan_axis axis, synscan_direction direction, synscan_home_stage next) {
	PRIVATE_DATA->home_axis = axis;
	PRIVATE_DATA->home_direction = direction;
	PRIVATE_DATA->home_search_resume = next;
	PRIVATE_DATA->home_stage = HOME_SEARCH_STATUS;
}
static void mount_autohome_finish(indigo_device *device, bool ok) {
	PRIVATE_DATA->home_stage = HOME_IDLE;
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	PRIVATE_DATA->homed = ok;
	PRIVATE_DATA->ra_axis_mode = PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
	if (!ok && PRIVATE_DATA->handle != NULL) {
		synscan_axis_command(device, 'L', SYNSCAN_AXIS_RA);
		synscan_axis_command(device, 'L', SYNSCAN_AXIS_DEC);
		synscan_invalidate_axis_configs(device);
	}
	if (!ok) {
		synscan_clear_tracking_state(device);
	}
	MOUNT_AUTOHOME_ITEM->sw.value = false;
	// The result is published once through indigo_update_coordinates(): a successful reading in OK, otherwise
	// ALERT, which follows the BUSY of the auto home and is kept until the next BUSY
	if (ok) {
		ok = synscan_update_mount_coordinates(device);
	}
	PRIVATE_DATA->homed = ok;
	if (!ok) {
		synscan_set_coordinate_state(device, INDIGO_ALERT_STATE);
	}
	synscan_update_mount_state(device);
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_AUTOHOME_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, ok ? NULL : "Auto home failed or timed out; stop requested on both axes.");
}
// Progress dispatches perform at most one protocol transaction. Motion/index polls and the
// sensor clearance dwell yield the master queue so abort/disconnect can own cancellation.
static void mount_autohome_finalizer(indigo_device *device) {
	if (PRIVATE_DATA->home_stage == HOME_IDLE) {
		return;
	}
	if (!IS_CONNECTED || PRIVATE_DATA->handle == NULL || PRIVATE_DATA->abort_motion) {
		mount_autohome_finish(device, false);
		return;
	}
	bool ok = true;
	double delay = 0;
	long value = 0;
	synscan_axis axis = PRIVATE_DATA->home_axis;
	synscan_home_stage stage = PRIVATE_DATA->home_stage++;
	switch (stage) {
		case HOME_ENCODER_RA:
			ok = synscan_ext_setting(device, SYNSCAN_AXIS_RA, SYNSCAN_EXT_TURN_ENCODER_OFF);
			break;
		case HOME_ENCODER_DEC:
			ok = synscan_ext_setting(device, SYNSCAN_AXIS_DEC, SYNSCAN_EXT_TURN_ENCODER_OFF);
			break;
		case HOME_INITIAL_STOP_RA:
		case HOME_INITIAL_STOP_DEC:
			axis = stage == HOME_INITIAL_STOP_RA ? SYNSCAN_AXIS_RA : SYNSCAN_AXIS_DEC;
			ok = synscan_axis_command(device, 'K', axis);
			synscan_invalidate_axis_config(device, axis);
			mount_autohome_wait(device, axis, stage + 1);
			break;
		case HOME_COUNTER_RA:
			synscan_clear_tracking_state(device);
			ok = synscan_axis_setting(device, 'E', SYNSCAN_AXIS_RA, PRIVATE_DATA->ra_home_position);
			break;
		case HOME_COUNTER_DEC:
			ok = synscan_axis_setting(device, 'E', SYNSCAN_AXIS_DEC, PRIVATE_DATA->dec_home_position);
			PRIVATE_DATA->home_prepare_axis = SYNSCAN_AXIS_RA;
			break;
		case HOME_PREP_RESET:
			ok = synscan_ext_setting(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_RESET_HOME_INDEXER);
			PRIVATE_DATA->home_extra_offset = false;
			break;
		case HOME_PREP_INDEX:
		case HOME_PREP_SECOND_INDEX:
			ok = synscan_ext_inquiry(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_GET_HOME_INDEX, &value);
			PRIVATE_DATA->home_slewing_up = value == 0;
			break;
		case HOME_PREP_POSITION:
			axis = PRIVATE_DATA->home_prepare_axis;
			ok = synscan_axis_long_query(device, 'j', axis, &value);
			long steps = axis == SYNSCAN_AXIS_RA ? PRIVATE_DATA->ra_total_steps : PRIVATE_DATA->dec_total_steps;
			mount_autohome_move(device, axis, value + (PRIVATE_DATA->home_slewing_up ? -1 : 1) * lrint(5.0 / 360.0 * steps), HOME_PREP_WAIT);
			break;
		case HOME_PREP_WAIT:
			mount_autohome_wait(device, PRIVATE_DATA->home_prepare_axis, PRIVATE_DATA->home_extra_offset ? HOME_PREP_SECOND_RESET : HOME_PREP_READ);
			break;
		case HOME_PREP_READ:
			ok = synscan_ext_inquiry(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_GET_HOME_INDEX, &value);
			if (value != 0 && value != 0xFFFFFF) {
				PRIVATE_DATA->home_extra_offset = true;
				PRIVATE_DATA->home_stage = HOME_PREP_POSITION;
			} else {
				PRIVATE_DATA->home_stage = HOME_PREP_REVERSE;
			}
			break;
		case HOME_PREP_SECOND_RESET:
			ok = synscan_ext_setting(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_RESET_HOME_INDEXER);
			break;
		case HOME_PREP_REVERSE:
			if (PRIVATE_DATA->home_slewing_up) {
				PRIVATE_DATA->home_stage = HOME_PREP_DONE;
			} else {
				PRIVATE_DATA->home_index_deadline = indigo_monotonic_time() + 120;
				mount_autohome_search(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_DIRECTION_REVERSE, HOME_PREP_REVERSE_INDEX);
			}
			break;
		case HOME_PREP_REVERSE_INDEX:
			ok = indigo_monotonic_time() < PRIVATE_DATA->home_index_deadline && synscan_ext_inquiry(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_GET_HOME_INDEX, &value);
			if (value == 0xFFFFFF) {
				PRIVATE_DATA->home_stage = HOME_PREP_REVERSE_INDEX;
				delay = 0.1;
			} else {
				delay = 3;
			}
			break;
		case HOME_PREP_DWELL:
			ok = synscan_axis_command(device, 'K', PRIVATE_DATA->home_prepare_axis);
			synscan_invalidate_axis_config(device, PRIVATE_DATA->home_prepare_axis);
			mount_autohome_wait(device, PRIVATE_DATA->home_prepare_axis, HOME_PREP_REVERSE_RESET);
			break;
		case HOME_PREP_REVERSE_RESET:
			ok = synscan_ext_setting(device, PRIVATE_DATA->home_prepare_axis, SYNSCAN_EXT_RESET_HOME_INDEXER);
			break;
		case HOME_PREP_DONE:
			if (PRIVATE_DATA->home_prepare_axis == SYNSCAN_AXIS_RA) {
				PRIVATE_DATA->home_prepare_axis = SYNSCAN_AXIS_DEC;
				PRIVATE_DATA->home_stage = HOME_PREP_RESET;
			}
			break;
		case HOME_FORWARD_RA:
			mount_autohome_search(device, SYNSCAN_AXIS_RA, SYNSCAN_DIRECTION_FORWARD, HOME_FORWARD_DEC);
			break;
		case HOME_FORWARD_DEC:
			mount_autohome_search(device, SYNSCAN_AXIS_DEC, SYNSCAN_DIRECTION_FORWARD, HOME_FORWARD_INDEX_RA);
			PRIVATE_DATA->home_index_deadline = indigo_monotonic_time() + 120;
			break;
		case HOME_FORWARD_INDEX_RA:
		case HOME_FORWARD_INDEX_DEC: {
			int index = stage == HOME_FORWARD_INDEX_RA ? 0 : 1;
			axis = index == 0 ? SYNSCAN_AXIS_RA : SYNSCAN_AXIS_DEC;
			ok = indigo_monotonic_time() < PRIVATE_DATA->home_index_deadline;
			if (!PRIVATE_DATA->home_found[index]) {
				ok = ok && synscan_ext_inquiry(device, axis, SYNSCAN_EXT_GET_HOME_INDEX, &value);
				if (value != 0) {
					PRIVATE_DATA->home_index[index] = value;
					PRIVATE_DATA->home_found[index] = true;
					PRIVATE_DATA->home_axis = axis;
					PRIVATE_DATA->home_search_resume = stage == HOME_FORWARD_INDEX_RA ? HOME_FORWARD_INDEX_DEC : HOME_FORWARD_CHECK;
					PRIVATE_DATA->home_stage = HOME_INDEX_STOP;
				}
			}
			break;
		}
		case HOME_INDEX_STOP:
			ok = synscan_axis_command(device, 'K', axis);
			synscan_invalidate_axis_config(device, axis);
			PRIVATE_DATA->home_stage = PRIVATE_DATA->home_search_resume;
			break;
		case HOME_FORWARD_CHECK:
			if (!PRIVATE_DATA->home_found[0] || !PRIVATE_DATA->home_found[1]) {
				PRIVATE_DATA->home_stage = HOME_FORWARD_INDEX_RA;
				delay = 0.1;
			}
			break;
		case HOME_FORWARD_WAIT_RA:
		case HOME_BACKOFF_WAIT_RA:
		case HOME_RETURN_WAIT_RA:
			mount_autohome_wait(device, SYNSCAN_AXIS_RA, stage + 1);
			break;
		case HOME_FORWARD_WAIT_DEC:
		case HOME_BACKOFF_WAIT_DEC:
		case HOME_RETURN_WAIT_DEC:
			mount_autohome_wait(device, SYNSCAN_AXIS_DEC, stage + 1);
			break;
		case HOME_BACKOFF_RA:
		case HOME_BACKOFF_DEC:
		case HOME_RETURN_RA:
		case HOME_RETURN_DEC: {
			bool ra = stage == HOME_BACKOFF_RA || stage == HOME_RETURN_RA;
			long offset = stage == HOME_BACKOFF_RA || stage == HOME_BACKOFF_DEC ? lrint(10.0 / 360.0 * (ra ? PRIVATE_DATA->ra_total_steps : PRIVATE_DATA->dec_total_steps)) : 0;
			mount_autohome_move(device, ra ? SYNSCAN_AXIS_RA : SYNSCAN_AXIS_DEC, PRIVATE_DATA->home_index[ra ? 0 : 1] - offset, stage + 1);
			break;
		}
		case HOME_FINAL_COUNTER_RA:
			ok = synscan_axis_setting(device, 'E', SYNSCAN_AXIS_RA, PRIVATE_DATA->ra_home_position);
			break;
		case HOME_FINAL_COUNTER_DEC:
			ok = synscan_axis_setting(device, 'E', SYNSCAN_AXIS_DEC, PRIVATE_DATA->dec_home_position + lrint(MOUNT_AUTOHOME_DEC_OFFSET_ITEM->number.value / 360.0 * PRIVATE_DATA->dec_total_steps));
			break;
		case HOME_COMPLETE:
			mount_autohome_finish(device, true);
			return;
		case HOME_MOVE_STOP:
			ok = synscan_axis_command(device, 'K', axis);
			synscan_invalidate_axis_config(device, axis);
			mount_autohome_wait(device, axis, HOME_MOVE_POSITION);
			break;
		case HOME_WAIT:
			ok = indigo_monotonic_time() < PRIVATE_DATA->home_deadline && synscan_axis_status_query(device, axis, &value);
			if (value & SYNSCAN_STATUS_RUNNING) {
				PRIVATE_DATA->home_stage = HOME_WAIT;
				delay = 0.1;
			} else {
				PRIVATE_DATA->home_stage = PRIVATE_DATA->home_resume;
			}
			break;
		case HOME_MOVE_POSITION:
			ok = synscan_axis_long_query(device, 'j', axis, &value);
			PRIVATE_DATA->home_delta = PRIVATE_DATA->home_target - value;
			if (PRIVATE_DATA->home_delta == 0) {
				PRIVATE_DATA->home_stage = PRIVATE_DATA->home_move_resume;
			}
			break;
		case HOME_MOVE_MODE:
			ok = synscan_set_axis_mode(device, axis, labs(PRIVATE_DATA->home_delta) > synscan_low_speed_goto_margin(device, axis) ? 0 : 2, PRIVATE_DATA->home_delta < 0 ? SYNSCAN_DIRECTION_REVERSE : SYNSCAN_DIRECTION_FORWARD);
			if (axis == SYNSCAN_AXIS_RA) {
				PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_GOTO;
			} else {
				PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_GOTO;
			}
			break;
		case HOME_MOVE_INCREMENT:
			ok = synscan_axis_setting(device, 'H', axis, labs(PRIVATE_DATA->home_delta));
			break;
		case HOME_MOVE_RAMP: {
			long delta = labs(PRIVATE_DATA->home_delta);
			long ramp = delta > synscan_low_speed_goto_margin(device, axis) ? SYNSCAN_HIGH_SPEED_GOTO_RAMP : SYNSCAN_LOW_SPEED_GOTO_RAMP;
			ok = synscan_axis_setting(device, 'M', axis, delta > ramp ? ramp : delta);
			break;
		}
		case HOME_MOVE_START:
			ok = synscan_axis_command(device, 'J', axis);
			PRIVATE_DATA->home_stage = PRIVATE_DATA->home_move_resume;
			break;
		case HOME_SEARCH_STATUS:
			ok = synscan_axis_status_query(device, axis, &value);
			if (synscan_axis_config_matches(device, axis, 3, PRIVATE_DATA->home_direction, 6)) {
				PRIVATE_DATA->home_stage = value & SYNSCAN_STATUS_RUNNING ? PRIVATE_DATA->home_search_resume : HOME_SEARCH_START;
			} else if (value & SYNSCAN_STATUS_RUNNING) {
				PRIVATE_DATA->home_stage = HOME_SEARCH_STOP;
			}
			break;
		case HOME_SEARCH_MODE:
			ok = synscan_set_axis_mode(device, axis, 3, PRIVATE_DATA->home_direction);
			break;
		case HOME_SEARCH_PERIOD:
			ok = synscan_axis_setting(device, 'I', axis, 6);
			if (ok) {
				synscan_cache_axis_config(device, axis, 3, PRIVATE_DATA->home_direction, 6);
			}
			break;
		case HOME_SEARCH_START:
			ok = synscan_axis_command(device, 'J', axis);
			PRIVATE_DATA->home_stage = PRIVATE_DATA->home_search_resume;
			break;
		case HOME_SEARCH_STOP:
			ok = synscan_axis_command(device, 'K', axis);
			synscan_invalidate_axis_config(device, axis);
			mount_autohome_wait(device, axis, HOME_SEARCH_MODE);
			break;
		default:
			ok = false;
			break;
	}
	if (!ok) {
		mount_autohome_finish(device, false);
		return;
	}
	indigo_execute_handler_in(device, delay, mount_autohome_finalizer);
}
static int mount_manual_slew_rate_index(indigo_device *device) {
	if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		return 4;
	}
	if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		return 6;
	}
	if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		return 9;
	}
	return 1;
}
static int mount_motion_progress(indigo_device *device) {
	long ra_status = 0;
	long dec_status = 0;
	if (PRIVATE_DATA->handle == NULL || PRIVATE_DATA->abort_motion || synscan_now() >= PRIVATE_DATA->motion_deadline || !synscan_axis_status_query(device, SYNSCAN_AXIS_RA, &ra_status) || !synscan_axis_status_query(device, SYNSCAN_AXIS_DEC, &dec_status)) {
		return -1;
	}
	return (ra_status & SYNSCAN_STATUS_RUNNING) || (dec_status & SYNSCAN_STATUS_RUNNING) ? 0 : 1;
}
static void mount_motion_failed(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		synscan_axis_command(device, 'L', SYNSCAN_AXIS_RA);
		synscan_axis_command(device, 'L', SYNSCAN_AXIS_DEC);
	}
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	synscan_invalidate_axis_configs(device);
	synscan_clear_tracking_state(device);
	synscan_update_mount_state(device);
}
static void mount_equatorial_coordinates_finalizer(indigo_device *device) {
	if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_SLEWING) {
		return;
	}
	int progress = mount_motion_progress(device);
	if (progress == 0) {
		indigo_execute_handler_in(device, 0.2, mount_equatorial_coordinates_finalizer);
		return;
	}
	bool ok = progress > 0 && synscan_update_axis_positions(device);
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	if (ok && MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		PRIVATE_DATA->current_tracking_rate = synscan_tracking_rate(device);
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, PRIVATE_DATA->current_tracking_rate);
		PRIVATE_DATA->ra_axis_mode = ok ? SYNSCAN_AXIS_TRACKING : SYNSCAN_AXIS_IDLE;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, ok ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
		// A pending request owns the state, its handler reads the target and publishes the result
		if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) {
			MOUNT_TRACKING_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
	} else if (ok) {
		synscan_clear_tracking_state(device);
	}
	if (ok) {
		ok = synscan_update_mount_coordinates(device);
	}
	if (!ok) {
		mount_motion_failed(device);
		synscan_set_coordinate_state(device, INDIGO_ALERT_STATE);
	}
	synscan_update_mount_state(device);
}
static void mount_park_finalizer(indigo_device *device) {
	if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_PARKING) {
		return;
	}
	int progress = mount_motion_progress(device);
	if (progress == 0) {
		indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
		return;
	}
	bool ok = progress > 0 && synscan_update_axis_positions(device) && synscan_save_position(device);
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	PRIVATE_DATA->parked = ok;
	if (ok) {
		synscan_clear_tracking_state(device);
		ok = synscan_update_mount_coordinates(device);
	}
	if (!ok) {
		PRIVATE_DATA->parked = false;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		mount_motion_failed(device);
		synscan_set_coordinate_state(device, INDIGO_ALERT_STATE);
	}
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
	synscan_update_mount_state(device);
}
static void mount_home_finalizer(indigo_device *device) {
	if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_HOMING) {
		return;
	}
	int progress = mount_motion_progress(device);
	if (progress == 0) {
		indigo_execute_handler_in(device, 0.2, mount_home_finalizer);
		return;
	}
	bool ok = progress > 0 && synscan_update_axis_positions(device);
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	PRIVATE_DATA->homed = ok;
	if (ok) {
		synscan_clear_tracking_state(device);
		ok = synscan_update_mount_coordinates(device);
	}
	if (!ok) {
		PRIVATE_DATA->homed = false;
		mount_motion_failed(device);
		synscan_set_coordinate_state(device, INDIGO_ALERT_STATE);
	}
	MOUNT_HOME_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, NULL);
	synscan_update_mount_state(device);
}

//- mount.code

//+ guider.code

static void guider_guide_ra_finalizer(indigo_device *device) {
	double remaining = PRIVATE_DATA->guide_ra_deadline - synscan_now();
	if (remaining > 0.01) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, remaining, guider_guide_ra_finalizer);
		return;
	}
	// A GOTO, abort or manual motion that took the axis over since the pulse started owns it now
	bool guiding = PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_GUIDING;
	bool ok = !guiding || synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, PRIVATE_DATA->guide_ra_resume_rate);
	PRIVATE_DATA->guide_ra_deadline = 0;
	PRIVATE_DATA->guide_ra_direction = 0;
	if (guiding && ok) {
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_TRACKING;
	} else if (guiding) {
		// the pulse ran its time, it failed only if the axis cannot be put back on the tracking rate
		ok = synscan_restore_tracking(device, PRIVATE_DATA->guide_ra_resume_rate);
		if (!ok) {
			synscan_tracking_lost(device->master_device, "Failed to resume tracking after RA guide pulse.");
		}
		synscan_update_mount_state(device->master_device);
	}
	// only the values, the target of a pulse requested while this one ends is read by its handler
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, ok ? NULL : "Failed to finish RA guide pulse.");
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	double remaining = PRIVATE_DATA->guide_dec_deadline - synscan_now();
	if (remaining > 0.01) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, remaining, guider_guide_dec_finalizer);
		return;
	}
	// A GOTO, abort or manual motion that took the axis over since the pulse started owns it now
	bool ok = true;
	if (PRIVATE_DATA->dec_axis_mode == SYNSCAN_AXIS_GUIDING) {
		ok = synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_DEC, NULL);
		PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
	}
	PRIVATE_DATA->guide_dec_deadline = 0;
	PRIVATE_DATA->guide_dec_direction = 0;
	// only the values, the target of a pulse requested while this one ends is read by its handler
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, ok ? NULL : "Failed to finish DEC guide pulse.");
}

//- guider.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	if (!IS_CONNECTED || PRIVATE_DATA->handle == NULL) {
		return;
	}
	// A poll in progress holds the shared queue, so a guide pulse coming due meanwhile ends late, by a whole reply
	// timeout when a reply is lost. Close to the end of a pulse the poll waits until just after it. The finalizer
	// runs a little after the deadline; a deadline long past is one whose finalizer a guider disconnect cancelled.
	double now = synscan_now();
	double guide_remaining = 0;
	bool guide_due = false;
	if (PRIVATE_DATA->guide_ra_deadline > 0 && PRIVATE_DATA->guide_ra_deadline - now > -SYNSCAN_POLL_GUIDE_STALE) {
		guide_remaining = PRIVATE_DATA->guide_ra_deadline - now;
		guide_due = true;
	}
	if (PRIVATE_DATA->guide_dec_deadline > 0 && PRIVATE_DATA->guide_dec_deadline - now > -SYNSCAN_POLL_GUIDE_STALE && (!guide_due || PRIVATE_DATA->guide_dec_deadline - now < guide_remaining)) {
		guide_remaining = PRIVATE_DATA->guide_dec_deadline - now;
		guide_due = true;
	}
	if (guide_due && guide_remaining < SYNSCAN_POLL_GUIDE_GUARD) {
		indigo_execute_handler_in(device, guide_remaining > 0 ? guide_remaining + SYNSCAN_POLL_AFTER_GUIDE : SYNSCAN_POLL_AFTER_GUIDE, mount_timer_callback);
		return;
	}
	// A failed reading keeps the last valid coordinates with ALERT, the next good poll restores the state. During a
	// motion or a request the reading is retried by the next poll and the coordinates stay BUSY: an ALERT published
	// after BUSY is kept and would turn the successful end of the motion into ALERT. A reading that keeps failing,
	// or one failing while nothing runs, is an ALERT.
	if (synscan_update_mount_coordinates(device)) {
		synscan_update_encoder_position(device);
	} else if (PRIVATE_DATA->handle != NULL && IS_CONNECTED) {
		bool running = PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_IDLE || MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
		PRIVATE_DATA->position_failures++;
		synscan_set_coordinate_state(device, running && PRIVATE_DATA->position_failures <= POSITION_READ_RETRIES ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE);
		synscan_update_mount_state(device);
		if (!MOUNT_ENCODER_POSITION_PROPERTY->hidden) {
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_ENCODER_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
	}
	indigo_execute_handler_in(device, PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE ? 1 : 0.5, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		if (PRIVATE_DATA->count == 0) {
			connection_result = synscan_open(device, &error_message);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			connection_result = synscan_configure(device);
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ENCODER_POSITION_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				synscan_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		PRIVATE_DATA->home_stage = HOME_IDLE;
		if (MOUNT_AUTOHOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			MOUNT_AUTOHOME_ITEM->sw.value = false;
			MOUNT_AUTOHOME_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->homed = false;
		}
		// Request stops for session motion before closing the port; report a refusal, and start
		// the next session with the motion switches off.
		if (PRIVATE_DATA->handle != NULL && (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_IDLE || PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_MANUAL_SLEWING || PRIVATE_DATA->dec_axis_mode == SYNSCAN_AXIS_MANUAL_SLEWING)) {
			PRIVATE_DATA->abort_motion = true;
			bool stopped = synscan_axis_command(device, 'L', SYNSCAN_AXIS_RA);
			stopped = synscan_axis_command(device, 'L', SYNSCAN_AXIS_DEC) && stopped;
			if (!stopped) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to stop one or both axes before disconnect.");
			}
			synscan_invalidate_axis_configs(device);
			PRIVATE_DATA->ra_axis_mode = PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
		}
		PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
		PRIVATE_DATA->sync_pending = false;
		MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
		MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
		MOUNT_HOME_ITEM->sw.value = false;
		//- mount.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			DEVICE_BAUDRATE_PROPERTY,
			MOUNT_EPOCH_PROPERTY,
			MOUNT_STATE_PROPERTY,
			MOUNT_PARK_SET_PROPERTY,
			MOUNT_PARK_POSITION_PROPERTY,
			MOUNT_HOME_SET_PROPERTY,
			MOUNT_HOME_POSITION_PROPERTY,
			MOUNT_RAW_COORDINATES_PROPERTY,
			MOUNT_SIDE_OF_PIER_PROPERTY,
			MOUNT_ALIGNMENT_MODE_PROPERTY,
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY,
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY,
			MOUNT_PARK_PROPERTY,
			MOUNT_HOME_PROPERTY,
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY,
			MOUNT_TRACKING_PROPERTY,
			MOUNT_TRACK_RATE_PROPERTY,
			MOUNT_MOTION_RA_PROPERTY,
			MOUNT_MOTION_DEC_PROPERTY,
			MOUNT_ABORT_MOTION_PROPERTY,
			MOUNT_STATE_PROPERTY,
			MOUNT_POLARSCOPE_PROPERTY,
			MOUNT_USE_ENCODERS_PROPERTY,
			MOUNT_GUIDE_RATE_PROPERTY,
			MOUNT_PEC_PROPERTY,
			MOUNT_PEC_TRAINING_PROPERTY,
			MOUNT_AUTOHOME_PROPERTY,
			MOUNT_AUTOHOME_SETTINGS_PROPERTY,
			MOUNT_OPERATING_MODE_PROPERTY,
			MOUNT_ENCODER_POSITION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ENCODER_POSITION_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			synscan_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, mount_timer_callback);
	}
}

static void mount_park_set_handler(indigo_device *device) {
	MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK_SET.on_change
	synscan_set_position(device, MOUNT_PARK_SET_PROPERTY, MOUNT_PARK_SET_DEFAULT_ITEM, MOUNT_PARK_SET_CURRENT_ITEM, MOUNT_PARK_POSITION_PROPERTY, MOUNT_PARK_POSITION_HA_ITEM, MOUNT_PARK_POSITION_DEC_ITEM);
	//- mount.MOUNT_PARK_SET.on_change
	indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
}

static void mount_home_set_handler(indigo_device *device) {
	MOUNT_HOME_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_HOME_SET.on_change
	synscan_set_position(device, MOUNT_HOME_SET_PROPERTY, MOUNT_HOME_SET_DEFAULT_ITEM, MOUNT_HOME_SET_CURRENT_ITEM, MOUNT_HOME_POSITION_PROPERTY, MOUNT_HOME_POSITION_HA_ITEM, MOUNT_HOME_POSITION_DEC_ITEM);
	//- mount.MOUNT_HOME_SET.on_change
	indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	bool claimed = PRIVATE_DATA->park_claimed;
	PRIVATE_DATA->park_claimed = false;
	// A sync admitted just before this park request never reaches its handler body, the parked-mount guard of
	// the handler refuses it; it must not keep the coordinates BUSY.
	PRIVATE_DATA->sync_pending = false;
	if (!MOUNT_PARK_PARKED_ITEM->sw.value) {
		PRIVATE_DATA->parked = false;
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_OK_STATE, NULL);
		synscan_update_mount_state(device);
	} else if (!claimed) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, PRIVATE_DATA->parked ? MOUNT_PARK_PARKED_ITEM : MOUNT_PARK_UNPARKED_ITEM, true);
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
	} else {
		long ra_target = 0;
		long dec_target = 0;
		PRIVATE_DATA->abort_motion = false;
		PRIVATE_DATA->homed = false;
		synscan_update_mount_state(device);
		synscan_ha_dec_to_steps(device, MOUNT_PARK_POSITION_HA_ITEM->number.value, MOUNT_PARK_POSITION_DEC_ITEM->number.value, &ra_target, &dec_target);
		bool ok = synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_RA, ra_target) && synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_DEC, dec_target);
		if (ok) {
			PRIVATE_DATA->motion_deadline = synscan_now() + 300;
			indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
		} else {
			// the reason is read before the stop commands replace it
			const char *reason = synscan_failure_reason(device);
			mount_motion_failed(device);
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_ALERT_STATE, "Park failed, %s.", reason);
			synscan_update_mount_state(device);
		}
	}
	//- mount.MOUNT_PARK.on_change
}

static void mount_home_handler(indigo_device *device) {
	//+ mount.MOUNT_HOME.on_change
	bool claimed = PRIVATE_DATA->home_claimed;
	PRIVATE_DATA->home_claimed = false;
	if (!MOUNT_HOME_ITEM->sw.value) {
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, INDIGO_OK_STATE, NULL);
	} else if (!claimed) {
		MOUNT_HOME_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, INDIGO_ALERT_STATE, NULL);
	} else {
		long ra_target = 0;
		long dec_target = 0;
		PRIVATE_DATA->abort_motion = false;
		PRIVATE_DATA->homed = false;
		synscan_update_mount_state(device);
		synscan_ha_dec_to_steps(device, MOUNT_HOME_POSITION_HA_ITEM->number.value, MOUNT_HOME_POSITION_DEC_ITEM->number.value, &ra_target, &dec_target);
		bool ok = synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_RA, ra_target) && synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_DEC, dec_target);
		if (ok) {
			PRIVATE_DATA->motion_deadline = synscan_now() + 300;
			indigo_execute_handler_in(device, 0.2, mount_home_finalizer);
		} else {
			const char *reason = synscan_failure_reason(device);
			mount_motion_failed(device);
			MOUNT_HOME_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, INDIGO_ALERT_STATE, "Home failed, %s.", reason);
			synscan_update_mount_state(device);
		}
	}
	//- mount.MOUNT_HOME.on_change
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_reject_coordinates_change(device, "Mount is parked!");
		return;
	}
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	// The request claimed the sync or the slew, a request that claimed neither is refused. The result of a sync is
	// published by the generated indigo_update_coordinates() below, a slew publishes its own start or failure.
	if (PRIVATE_DATA->sync_pending) {
		PRIVATE_DATA->sync_pending = false;
		indigo_mount_sync(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		// the result follows the BUSY of the request, indigo_update_coordinates() publishes it unchanged
		synscan_update_mount_state(device);
	} else {
		if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_SLEWING) {
			indigo_reject_coordinates_change(device, "Mount is moving");
			synscan_update_mount_state(device);
		} else {
			long ra_target = 0;
			long dec_target = 0;
			synscan_radec_to_steps(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra_target, &dec_target);
			PRIVATE_DATA->abort_motion = false;
			PRIVATE_DATA->homed = false;
			synscan_update_mount_state(device);
			bool ok = synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_RA, ra_target) && synscan_slew_axis_to_steps(device, SYNSCAN_AXIS_DEC, dec_target);
			if (ok) {
				PRIVATE_DATA->motion_deadline = synscan_now() + 300;
				indigo_execute_handler_in(device, 0.2, mount_equatorial_coordinates_finalizer);
			} else {
				const char *reason = synscan_failure_reason(device);
				mount_motion_failed(device);
				indigo_reject_coordinates_change(device, "Slew failed, %s.", reason);
				synscan_update_mount_state(device);
			}
		}
		return;
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	if (PRIVATE_DATA->home_stage != HOME_IDLE) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.2, mount_tracking_handler);
		return;
	}
	// The slew, park and home finalizers, the failed motion path, abort and auto home write the tracking state
	// between the copy of the request and this handler, the target keeps the requested value. On failure the
	// switch shows the state of the RA axis.
	bool on = indigo_get_switch_target(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM_NAME);
	bool ok = true;
	if (on) {
		PRIVATE_DATA->current_tracking_rate = synscan_tracking_rate(device);
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, PRIVATE_DATA->current_tracking_rate);
		PRIVATE_DATA->ra_axis_mode = ok ? SYNSCAN_AXIS_TRACKING : SYNSCAN_AXIS_IDLE;
	} else {
		ok = synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_RA, NULL);
		PRIVATE_DATA->current_tracking_rate = 0;
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
	}
	if (ok) {
		indigo_apply_switch_targets(MOUNT_TRACKING_PROPERTY);
	} else {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, PRIVATE_DATA->ra_axis_mode == SYNSCAN_AXIS_TRACKING ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	synscan_update_mount_state(device);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	if (PRIVATE_DATA->home_stage != HOME_IDLE) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.2, mount_track_rate_handler);
		return;
	}
	bool ok = true;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		PRIVATE_DATA->current_tracking_rate = synscan_tracking_rate(device);
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, PRIVATE_DATA->current_tracking_rate);
	}
	if (!ok) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	synscan_update_mount_state(device);
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
		return;
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_RA.on_change
	if (PRIVATE_DATA->home_stage != HOME_IDLE) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.2, mount_motion_ra_handler);
		return;
	}
	static const double rates[] = { 1.25, 2, 8, 16, 32, 70, 100, 625, 725, 825 };
	bool ok = true;
	double rate = rates[mount_manual_slew_rate_index(device)] * SIDEREAL_RATE;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, rate);
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_MANUAL_SLEWING;
		PRIVATE_DATA->homed = false;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, -rate);
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_MANUAL_SLEWING;
		PRIVATE_DATA->homed = false;
	} else {
		ok = synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_RA, NULL);
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
		// Manual motion only interrupts tracking, the switch stays on, so RA goes back to the tracking rate.
		// If it cannot, the switch is turned off to show the axis is not tracking.
		if (MOUNT_TRACKING_ON_ITEM->sw.value && !PRIVATE_DATA->parked) {
			PRIVATE_DATA->current_tracking_rate = synscan_tracking_rate(device);
			if (ok && synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, PRIVATE_DATA->current_tracking_rate)) {
				PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_TRACKING;
			} else {
				ok = synscan_restore_tracking(device, PRIVATE_DATA->current_tracking_rate);
				if (!ok) {
					synscan_tracking_lost(device, "Failed to resume tracking after manual motion.");
				}
			}
		}
	}
	if (!ok) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	synscan_update_mount_state(device);
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	if (PRIVATE_DATA->home_stage != HOME_IDLE) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.2, mount_motion_dec_handler);
		return;
	}
	static const double rates[] = { 0.5, 1, 8, 16, 32, 70, 100, 625, 725, 825 };
	bool ok = true;
	double rate = rates[mount_manual_slew_rate_index(device)] * SIDEREAL_RATE;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_DEC, synscan_dec_north_sign(device) * rate);
		PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_MANUAL_SLEWING;
		PRIVATE_DATA->homed = false;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_DEC, -synscan_dec_north_sign(device) * rate);
		PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_MANUAL_SLEWING;
		PRIVATE_DATA->homed = false;
	} else {
		ok = synscan_stop_axis_and_wait(device, SYNSCAN_AXIS_DEC, NULL);
		PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
	}
	if (!ok) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	synscan_update_mount_state(device);
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
	bool autohome_active = MOUNT_AUTOHOME_PROPERTY->state == INDIGO_BUSY_STATE;
	indigo_cancel_pending_handler(device, mount_autohome_handler);
	indigo_cancel_pending_handler(device, mount_autohome_finalizer);
	PRIVATE_DATA->home_stage = HOME_IDLE;
	if (MOUNT_AUTOHOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_AUTOHOME_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_AUTOHOME_PROPERTY, INDIGO_ALERT_STATE, "Auto home aborted.");
	}
	indigo_cancel_pending_handler(device, mount_equatorial_coordinates_handler);
	// a sync whose handler was cancelled never ends its claim, the poll must not keep it BUSY
	PRIVATE_DATA->sync_pending = false;
	indigo_cancel_pending_handler(device, mount_equatorial_coordinates_finalizer);
	indigo_cancel_pending_handler(device, mount_motion_ra_handler);
	indigo_cancel_pending_handler(device, mount_motion_dec_handler);
	indigo_cancel_pending_handler(device, mount_park_handler);
	indigo_cancel_pending_handler(device, mount_park_finalizer);
	if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, PRIVATE_DATA->parked ? MOUNT_PARK_PARKED_ITEM : MOUNT_PARK_UNPARKED_ITEM, true);
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	indigo_cancel_pending_handler(device, mount_home_handler);
	indigo_cancel_pending_handler(device, mount_home_finalizer);
	// the claims of cancelled home and park handlers end with the global mode below
	PRIVATE_DATA->home_claimed = PRIVATE_DATA->autohome_claimed = PRIVATE_DATA->park_claimed = false;
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_HOME_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_HOME_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	// A tracking mount stays tracking: the abort stops slews and manual motion, then RA goes back to the
	// tracking rate. A pending tracking request is carried out by its own handler.
	bool tracking = !autohome_active && MOUNT_TRACKING_ON_ITEM->sw.value && MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE && !PRIVATE_DATA->parked;
	PRIVATE_DATA->abort_motion = true;
	bool stopped = synscan_axis_command(device, 'L', SYNSCAN_AXIS_RA);
	stopped = synscan_axis_command(device, 'L', SYNSCAN_AXIS_DEC) && stopped;
	synscan_invalidate_axis_configs(device);
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
	PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_IDLE;
	PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_IDLE;
	PRIVATE_DATA->homed = false;
	if (tracking && stopped) {
		PRIVATE_DATA->current_tracking_rate = synscan_tracking_rate(device);
		if (!synscan_restore_tracking(device, PRIVATE_DATA->current_tracking_rate)) {
			synscan_tracking_lost(device, "Failed to resume tracking after abort.");
		}
	} else {
		synscan_clear_tracking_state(device);
	}
	if (autohome_active || MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		synscan_set_coordinate_state(device, INDIGO_ALERT_STATE);
	}
	// The manual motion switches show the stopped axes
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, MOUNT_MOTION_RA_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_ALERT_STATE : INDIGO_OK_STATE, NULL);
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, MOUNT_MOTION_DEC_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_ALERT_STATE : INDIGO_OK_STATE, NULL);
	synscan_update_mount_state(device);
	INDIGO_UPDATE_PROPERTY_STATE(MOUNT_ABORT_MOTION_PROPERTY, stopped ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, stopped ? NULL : "Failed to stop one or both axes.");
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_mount_commit_motion_client(device, MOUNT_ABORT_MOTION_PROPERTY);
}

static void mount_polarscope_handler(indigo_device *device) {
	MOUNT_POLARSCOPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_POLARSCOPE.on_change
	bool ok = synscan_set_polarscope_brightness(device, (int)MOUNT_POLARSCOPE_BRIGHTNESS_ITEM->number.value);
	if (!ok) {
		MOUNT_POLARSCOPE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_POLARSCOPE.on_change
	indigo_update_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
}

static void mount_use_encoders_handler(indigo_device *device) {
	MOUNT_USE_ENCODERS_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_USE_ENCODERS.on_change
	if (PRIVATE_DATA->home_stage != HOME_IDLE) {
		MOUNT_USE_ENCODERS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.2, mount_use_encoders_handler);
		return;
	}
	// Auto home turns the encoders off and writes the values between the copy of a request and this handler,
	// the targets keep the request. The values show what the mount accepted.
	bool ok = synscan_set_encoders(device, indigo_get_switch_target(MOUNT_USE_ENCODERS_PROPERTY, MOUNT_USE_RA_ENCODER_ITEM_NAME), indigo_get_switch_target(MOUNT_USE_ENCODERS_PROPERTY, MOUNT_USE_DEC_ENCODER_ITEM_NAME));
	if (!ok) {
		MOUNT_USE_ENCODERS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	// the client sees a re-base of the step counters at once, not with the next poll; a failed reading is left to it
	synscan_update_mount_coordinates(device);
	//- mount.MOUNT_USE_ENCODERS.on_change
	indigo_update_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	bool ok = true;
	ok = ok && synscan_set_st4_guide_rate(device, SYNSCAN_AXIS_RA, MOUNT_GUIDE_RATE_RA_ITEM->number.value);
	ok = ok && synscan_set_st4_guide_rate(device, SYNSCAN_AXIS_DEC, MOUNT_GUIDE_RATE_DEC_ITEM->number.value);
	if (!ok) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void mount_pec_handler(indigo_device *device) {
	MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PEC.on_change
	bool ok = synscan_ext_setting(device, SYNSCAN_AXIS_RA, MOUNT_PEC_ENABLED_ITEM->sw.value ? SYNSCAN_EXT_TURN_PPEC_ON : SYNSCAN_EXT_TURN_PPEC_OFF);
	if (!ok) {
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_PEC.on_change
	indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
}

static void mount_pec_training_handler(indigo_device *device) {
	MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PEC_TRAINING.on_change
	// The training timer writes STOPPED when training ends between the copy of a request and this handler,
	// the target keeps the request. On failure the switch shows the training state the mount last reported.
	bool start = indigo_get_switch_target(MOUNT_PEC_TRAINING_PROPERTY, MOUNT_PEC_TRAINIG_STARTED_ITEM_NAME);
	if (synscan_ext_setting(device, SYNSCAN_AXIS_RA, start ? SYNSCAN_EXT_START_PPEC_TRAINING : SYNSCAN_EXT_STOP_PPEC_TRAINING)) {
		indigo_apply_switch_targets(MOUNT_PEC_TRAINING_PROPERTY);
		if (start) {
			indigo_execute_handler_in(device, 1, synscan_ppec_training_timer);
		}
	} else {
		indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, (PRIVATE_DATA->ra_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING) || (PRIVATE_DATA->dec_features & SYNSCAN_FEATURE_IN_PPEC_TRAINING) ? MOUNT_PEC_TRAINIG_STARTED_ITEM : MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
		MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_PEC_TRAINING.on_change
	indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
}

static void mount_autohome_handler(indigo_device *device) {
	//+ mount.MOUNT_AUTOHOME.on_change
	bool claimed = PRIVATE_DATA->autohome_claimed;
	PRIVATE_DATA->autohome_claimed = false;
	if (!MOUNT_AUTOHOME_ITEM->sw.value) {
		MOUNT_AUTOHOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
		return;
	}
	if (PRIVATE_DATA->parked) {
		// a claim made before the mount was parked is given up
		if (claimed) {
			PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_IDLE;
		}
		MOUNT_AUTOHOME_ITEM->sw.value = false;
		MOUNT_AUTOHOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Mount is parked.");
		return;
	}
	if (!claimed) {
		MOUNT_AUTOHOME_ITEM->sw.value = false;
		MOUNT_AUTOHOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Auto home not started - mount is busy.");
		return;
	}
	PRIVATE_DATA->homed = false;
	// Homing owns both axes immediately; an older guide finalizer must not restore a rate.
	PRIVATE_DATA->ra_axis_mode = PRIVATE_DATA->dec_axis_mode = SYNSCAN_AXIS_GOTO;
	MOUNT_AUTOHOME_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Starting auto home procedure...");
	synscan_set_coordinate_state(device, INDIGO_BUSY_STATE);
	synscan_update_mount_state(device);
	PRIVATE_DATA->abort_motion = false;
	PRIVATE_DATA->home_stage = HOME_ENCODER_RA;
	PRIVATE_DATA->home_found[0] = PRIVATE_DATA->home_found[1] = false;
	MOUNT_USE_RA_ENCODER_ITEM->sw.value = MOUNT_USE_DEC_ENCODER_ITEM->sw.value = false;
	PRIVATE_DATA->ra_encoder = PRIVATE_DATA->dec_encoder = false;
	indigo_execute_handler(device, mount_autohome_finalizer);
	//- mount.MOUNT_AUTOHOME.on_change
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ mount.on_attach
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		//- mount.on_attach
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		MOUNT_EPOCH_PROPERTY->hidden = false;
		//+ mount.MOUNT_EPOCH.on_attach
		MOUNT_EPOCH_PROPERTY->perm = INDIGO_RO_PERM;
		//- mount.MOUNT_EPOCH.on_attach
		MOUNT_STATE_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_HOME_SET_PROPERTY->hidden = false;
		MOUNT_HOME_POSITION_PROPERTY->hidden = false;
		MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = false;
		//+ mount.MOUNT_ALIGNMENT_MODE.on_attach
		// Select before hiding: the switch clears only the visible items, and a CONTROLLER item left on
		// behind the count bypasses the alignment model.
		indigo_set_switch(MOUNT_ALIGNMENT_MODE_PROPERTY, MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM, true);
		MOUNT_ALIGNMENT_MODE_PROPERTY->count = 2;
		//- mount.MOUNT_ALIGNMENT_MODE.on_attach
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
		//+ mount.MOUNT_ALIGNMENT_SELECT_POINTS.on_attach
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
		//- mount.MOUNT_ALIGNMENT_SELECT_POINTS.on_attach
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		//+ mount.MOUNT_PARK.on_attach
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		//- mount.MOUNT_PARK.on_attach
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		//+ mount.MOUNT_TRACKING.on_attach
		MOUNT_TRACKING_ON_ITEM->sw.value = false;
		MOUNT_TRACKING_OFF_ITEM->sw.value = true;
		//- mount.MOUNT_TRACKING.on_attach
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_STATE_PROPERTY->hidden = false;
		MOUNT_POLARSCOPE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Polarscope", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_POLARSCOPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(MOUNT_POLARSCOPE_BRIGHTNESS_ITEM, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, "Polarscope Brightness", 0, 255, 0, 0);
		MOUNT_POLARSCOPE_PROPERTY->hidden = true;
		MOUNT_USE_ENCODERS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_USE_ENCODERS_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Use encoders", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (MOUNT_USE_ENCODERS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_USE_RA_ENCODER_ITEM, MOUNT_USE_RA_ENCODER_ITEM_NAME, "Use RA encoder", false);
		indigo_init_switch_item(MOUNT_USE_DEC_ENCODER_ITEM, MOUNT_USE_DEC_ENCODER_ITEM_NAME, "Use Dec encoder", false);
		MOUNT_USE_ENCODERS_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		//+ mount.MOUNT_GUIDE_RATE.on_attach
		strncpy(MOUNT_GUIDE_RATE_PROPERTY->label, "ST4 guide rate", INDIGO_VALUE_SIZE);
		//- mount.MOUNT_GUIDE_RATE.on_attach
		MOUNT_PEC_PROPERTY->hidden = false;
		MOUNT_PEC_TRAINING_PROPERTY->hidden = false;
		MOUNT_AUTOHOME_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_AUTOHOME_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Auto home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (MOUNT_AUTOHOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_AUTOHOME_ITEM, MOUNT_AUTOHOME_ITEM_NAME, "Start auto home procedure", false);
		MOUNT_AUTOHOME_PROPERTY->hidden = true;
		MOUNT_AUTOHOME_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Auto home settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_AUTOHOME_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(MOUNT_AUTOHOME_DEC_OFFSET_ITEM, MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME, "Dec offset", -90, 90, 0, 0);
		MOUNT_AUTOHOME_SETTINGS_PROPERTY->hidden = true;
		MOUNT_OPERATING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_OPERATING_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Operating mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_OPERATING_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", true);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		MOUNT_OPERATING_MODE_PROPERTY->hidden = true;
		MOUNT_ENCODER_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_ENCODER_POSITION_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Encoder position", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (MOUNT_ENCODER_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(MOUNT_ENCODER_POSITION_RA_ITEM, MOUNT_ENCODER_POSITION_RA_ITEM_NAME, "RA encoder (counts)", -8388608, 8388607, 1, 0);
		strcpy(MOUNT_ENCODER_POSITION_RA_ITEM->number.format, "%.0f");
		indigo_init_number_item(MOUNT_ENCODER_POSITION_DEC_ITEM, MOUNT_ENCODER_POSITION_DEC_ITEM_NAME, "Dec encoder (counts)", -8388608, 8388607, 1, 0);
		strcpy(MOUNT_ENCODER_POSITION_DEC_ITEM->number.format, "%.0f");
		MOUNT_ENCODER_POSITION_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_POLARSCOPE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_USE_ENCODERS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_AUTOHOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_AUTOHOME_SETTINGS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_OPERATING_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_ENCODER_POSITION_PROPERTY);
	}
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_SET_PROPERTY, mount_park_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_SET_PROPERTY, mount_home_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		//+ mount.MOUNT_PARK.on_change_request
		// Claimed when admitted, see MOUNT_HOME: a home request admitted after this one finds the mount parking.
		if (MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE && indigo_get_switch(property, MOUNT_PARK_PARKED_ITEM_NAME) && PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE) {
			PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_PARKING;
			PRIVATE_DATA->park_claimed = true;
		}
		//- mount.MOUNT_PARK.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		//+ mount.MOUNT_HOME.on_change_request
		// Claim the homing when the request is admitted, as a coordinates request claims its slew: a coordinates request
		// sent after this one, before its handler runs, is then refused as during the homing instead of claiming the
		// axes itself and making this handler refuse the home.
		if (MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE && indigo_get_switch(property, MOUNT_HOME_ITEM_NAME) && PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE) {
			PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_HOMING;
			PRIVATE_DATA->home_claimed = true;
		}
		//- mount.MOUNT_HOME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_REJECT_COORDINATES_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, "Mount is parked!");
		INDIGO_REJECT_COORDINATES_CHANGE_IF(PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_HOMING, "Mount is homing");
		//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change_request
		// Claim the slew or the sync before the poll can republish OK over the accepted BUSY state. The controller
		// has no sync of its own, a sync is an alignment point of the framework's model; the parked mount guard above
		// already refused it on a parked mount, and during a park, whose request sets the parked switch. A request while
		// a home or an auto home runs is refused above, before its BUSY: it would claim nothing, and the end of the
		// homing could publish OK over its BUSY before its handler refused it.
		if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				PRIVATE_DATA->sync_pending = !MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value;
			} else {
				PRIVATE_DATA->sync_pending = false;
				if (PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE) {
					PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_SLEWING;
				}
			}
		}
		//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_COORDINATES_CHANGE(mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_POLARSCOPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_POLARSCOPE_PROPERTY, mount_polarscope_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_USE_ENCODERS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_USE_ENCODERS_PROPERTY, mount_use_encoders_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PEC_PROPERTY, mount_pec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_TRAINING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PEC_TRAINING_PROPERTY, mount_pec_training_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_AUTOHOME_PROPERTY, property)) {
		//+ mount.MOUNT_AUTOHOME.on_change_request
		// Claimed when admitted, see MOUNT_HOME
		if (MOUNT_AUTOHOME_PROPERTY->state != INDIGO_BUSY_STATE && indigo_get_switch(property, MOUNT_AUTOHOME_ITEM_NAME) && !PRIVATE_DATA->parked && PRIVATE_DATA->global_mode == SYNSCAN_GLOBAL_IDLE) {
			PRIVATE_DATA->global_mode = SYNSCAN_GLOBAL_HOMING;
			PRIVATE_DATA->autohome_claimed = true;
		}
		//- mount.MOUNT_AUTOHOME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_AUTOHOME_PROPERTY, mount_autohome_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_AUTOHOME_SETTINGS_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_AUTOHOME_SETTINGS_PROPERTY, property, false);
		MOUNT_AUTOHOME_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_POLARSCOPE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_USE_ENCODERS_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_AUTOHOME_SETTINGS_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(MOUNT_POLARSCOPE_PROPERTY);
	indigo_release_property(MOUNT_USE_ENCODERS_PROPERTY);
	indigo_release_property(MOUNT_AUTOHOME_PROPERTY);
	indigo_release_property(MOUNT_AUTOHOME_SETTINGS_PROPERTY);
	indigo_release_property(MOUNT_OPERATING_MODE_PROPERTY);
	indigo_release_property(MOUNT_ENCODER_POSITION_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		if (PRIVATE_DATA->count == 0) {
			connection_result = synscan_open(device->master_device, &error_message);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			if (!PRIVATE_DATA->configured) {
				indigo_device *master = device->master_device;
				connection_result = synscan_configure(master);
			}
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				synscan_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			GUIDER_GUIDE_RA_PROPERTY,
			GUIDER_GUIDE_DEC_PROPERTY,
			GUIDER_RATE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			synscan_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	// the previous pulse's finalizer may have zeroed the values after the request was copied, the targets keep it
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target;
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target;
	double duration = 0;
	int direction = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		duration = GUIDER_GUIDE_EAST_ITEM->number.value / 1000.0;
		direction = -1;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		duration = GUIDER_GUIDE_WEST_ITEM->number.value / 1000.0;
		direction = 1;
	}
	if (duration <= 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return;
	}
	if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_IDLE) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "Ignoring RA guide pulse - mount is slewing.");
		return;
	}
	double tracking_rate = PRIVATE_DATA->current_tracking_rate;
	if (tracking_rate == 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "Ignoring RA guide pulse - mount is not tracking.");
		return;
	}
	double guide_rate = tracking_rate + direction * GUIDER_RATE_ITEM->number.value * tracking_rate / 100.0;
	// A pulse arriving while another one is still running replaces it, in either
	// direction, so the deadline is always the new one and the finalizer of the
	// replaced pulse must not fire on the old schedule.
	if (PRIVATE_DATA->guide_ra_deadline > 0) {
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	}
	PRIVATE_DATA->guide_ra_deadline = synscan_now() + duration;
	PRIVATE_DATA->guide_ra_direction = direction;
	PRIVATE_DATA->guide_ra_resume_rate = tracking_rate;
	bool ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_RA, guide_rate);
	if (ok) {
		PRIVATE_DATA->ra_axis_mode = SYNSCAN_AXIS_GUIDING;
	} else {
		PRIVATE_DATA->guide_ra_deadline = 0;
		PRIVATE_DATA->guide_ra_direction = 0;
		if (!synscan_restore_tracking(device, tracking_rate)) {
			synscan_tracking_lost(device->master_device, "Failed to resume tracking after RA guide pulse.");
		}
		synscan_update_mount_state(device->master_device);
	}
	if (ok) {
		// the pulse runs from the command that changed the period, not from the acknowledgement of it
		PRIVATE_DATA->guide_ra_deadline = PRIVATE_DATA->last_send_time + duration;
	}
	GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, ok ? NULL : "Failed to start RA guide pulse.");
	if (ok) {
		double remaining = PRIVATE_DATA->guide_ra_deadline - synscan_now();
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, remaining > 0 ? remaining : 0, guider_guide_ra_finalizer);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	// the previous pulse's finalizer may have zeroed the values after the request was copied, the targets keep it
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target;
	double duration = 0;
	int direction = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value / 1000.0;
		direction = -1;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value / 1000.0;
		direction = 1;
	}
	if (duration <= 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return;
	}
	if (PRIVATE_DATA->global_mode != SYNSCAN_GLOBAL_IDLE) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, "Ignoring DEC guide pulse - mount is slewing.");
		return;
	}
	double tracking_rate = PRIVATE_DATA->current_tracking_rate != 0 ? PRIVATE_DATA->current_tracking_rate : SIDEREAL_RATE;
	// direction is -1 for north, the axis direction that moves north depends on the side of the pier
	double guide_rate = -direction * synscan_dec_north_sign(device) * GUIDER_DEC_RATE_ITEM->number.value * tracking_rate / 100.0;
	// A pulse arriving while another one is still running replaces it, in either
	// direction, so the deadline is always the new one and the finalizer of the
	// replaced pulse must not fire on the old schedule.
	if (PRIVATE_DATA->guide_dec_deadline > 0) {
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	}
	PRIVATE_DATA->guide_dec_deadline = synscan_now() + duration;
	PRIVATE_DATA->guide_dec_direction = direction;
	bool ok = synscan_slew_axis_at_rate(device, SYNSCAN_AXIS_DEC, guide_rate);
	PRIVATE_DATA->dec_axis_mode = ok ? SYNSCAN_AXIS_GUIDING : SYNSCAN_AXIS_IDLE;
	if (ok) {
		// the pulse runs from the command that started the axis, not from the acknowledgement of it
		PRIVATE_DATA->guide_dec_deadline = PRIVATE_DATA->last_send_time + duration;
	}
	GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, ok ? NULL : "Failed to start DEC guide pulse.");
	if (ok) {
		double remaining = PRIVATE_DATA->guide_dec_deadline - synscan_now();
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, remaining > 0 ? remaining : 0, guider_guide_dec_finalizer);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->count = 2;
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, GUIDER_RATE_PROPERTY);
		}
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value--;
		if (CCD_EXPOSURE_ITEM->number.value <= 0) {
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_EXPOSURE_PROPERTY->state = synscan_set_snap_port(device, false) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_EXPOSURE_ITEM->number.value > 0) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.value < 1 ? CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback);
		}
	}
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		if (PRIVATE_DATA->count == 0) {
			connection_result = synscan_open(device->master_device, &error_message);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			if (!PRIVATE_DATA->configured) {
				indigo_device *master = device->master_device;
				connection_result = synscan_configure(master);
			}
			if (connection_result && !PRIVATE_DATA->snap_port_supported) {
				connection_result = false;
			}
			CCD_ABORT_EXPOSURE_PROPERTY->hidden = !connection_result;
			CCD_EXPOSURE_PROPERTY->hidden = !connection_result;
			CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				synscan_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		if (PRIVATE_DATA->snap_port_supported) {
			synscan_set_snap_port(device, false);
		}
		//- aux.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			CCD_ABORT_EXPOSURE_PROPERTY,
			CCD_EXPOSURE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			synscan_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_ccd_abort_exposure_handler(indigo_device *device) {
	CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.CCD_ABORT_EXPOSURE.on_change
	if (CCD_ABORT_EXPOSURE_ITEM->sw.value && CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, aux_ccd_exposure_handler);
		indigo_cancel_pending_handler(device, aux_timer_callback);
		synscan_set_snap_port(device, false);
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//- aux.CCD_ABORT_EXPOSURE.on_change
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
}

static void aux_ccd_exposure_handler(indigo_device *device) {
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.CCD_EXPOSURE.on_change
	if (!PRIVATE_DATA->snap_port_supported) {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		return;
	}
	bool ok = synscan_set_snap_port(device, true);
	if (ok) {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.value < 1 ? CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback);
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.CCD_EXPOSURE.on_change
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SHUTTER) == INDIGO_OK) {
		CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (CCD_ABORT_EXPOSURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, CCD_ABORT_EXPOSURE_ITEM_NAME, "Abort exposure", false);
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = true;
		CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (CCD_EXPOSURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Start exposure", 0, 1000, 1, 0);
		strcpy(CCD_EXPOSURE_ITEM->number.format, "%g");
		CCD_EXPOSURE_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(CCD_ABORT_EXPOSURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(CCD_EXPOSURE_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, aux_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, aux_ccd_exposure_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(CCD_ABORT_EXPOSURE_PROPERTY);
	indigo_release_property(CCD_EXPOSURE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_mount_synscan(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static synscan_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (synscan_private_data *)indigo_safe_malloc(sizeof(synscan_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = mount;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				indigo_safe_free(guider);
				guider = NULL;
			}
			if (mount != NULL) {
				indigo_detach_device(mount);
				indigo_safe_free(mount);
				mount = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
