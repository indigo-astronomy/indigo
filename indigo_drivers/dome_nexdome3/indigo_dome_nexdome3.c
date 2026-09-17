// Copyright (c) 2019-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_dome_nexdome3.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <stdarg.h>
#include <pthread.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_nexdome3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000C
#define DRIVER_NAME          "indigo_dome_nexdome3"
#define DRIVER_LABEL         "NexDome3"
#define DOME_DEVICE_NAME     "NexDome3"
#define PRIVATE_DATA         ((nexdome3_private_data *)device->private_data)

//+ define

#define NEXDOME3_NETWORK_PORT 8080
#define NEXDOME3_RESET_DELAY 1
#define NEXDOME3_IDENTIFICATION_TIMEOUT 5
#define NEXDOME3_IDENTIFICATION_MESSAGES 30
#define NEXDOME3_READ_TIMEOUT 0.5
#define NEXDOME3_TRANSPORT_RETRY_DELAY 500000
#define NEXDOME3_STATUS_DELAY 3
#define NEXDOME3_CHECK_DELAY 1
#define NEXDOME3_STATUS_TIMEOUT 3
#define NEXDOME3_STALL_TIMEOUT 5
#define NEXDOME3_SHUTTER_TIMEOUT 30
#define NEXDOME3_DEFAULT_STEPS_PER_DEGREE 153
#define NEXDOME3_FIRMWARE_VERSION_3_2 0x0302
#define NEXDOME3_VOLTS_PER_ADU 0.01465
#define NEXDOME3_VOLT_THRESHOLD 7.5
#define NEXDOME3_MESSAGE_SIZE 100
#define NEXDOME3_MESSAGE_COUNT 64
#define NEXDOME3_PENDING_COUNT 16

//- define

#pragma mark - Property definitions

#define X_FIND_HOME_PROPERTY           (PRIVATE_DATA->x_find_home_property)
#define X_FIND_HOME_ITEM               (X_FIND_HOME_PROPERTY->items + 0)

#define X_FIND_HOME_PROPERTY_NAME      "X_FIND_HOME"
#define X_FIND_HOME_ITEM_NAME          "FIND_HOME"

#define X_HOME_POSITION_PROPERTY       (PRIVATE_DATA->x_home_position_property)
#define X_HOME_POSITION_ITEM           (X_HOME_POSITION_PROPERTY->items + 0)

#define X_HOME_POSITION_PROPERTY_NAME  "X_HOME_POSITION"
#define X_HOME_POSITION_ITEM_NAME      "POSITION"

#define X_MOVE_THRESHOLD_PROPERTY      (PRIVATE_DATA->x_move_threshold_property)
#define X_MOVE_THRESHOLD_ITEM          (X_MOVE_THRESHOLD_PROPERTY->items + 0)

#define X_MOVE_THRESHOLD_PROPERTY_NAME "X_MOVE_THRESHOLD"
#define X_MOVE_THRESHOLD_ITEM_NAME     "THRESHOLD"

#define X_BATTERY_POWER_PROPERTY          (PRIVATE_DATA->x_battery_power_property)
#define X_BATTERY_POWER_VOLTAGE_ITEM      (X_BATTERY_POWER_PROPERTY->items + 0)

#define X_BATTERY_POWER_PROPERTY_NAME     "X_BATTERY_POWER"
#define X_BATTERY_POWER_VOLTAGE_ITEM_NAME "VOLTAGE"

#define X_ACCELERATION_TIME_PROPERTY          (PRIVATE_DATA->x_acceleration_time_property)
#define X_ACCELERATION_TIME_ROTATOR_ITEM      (X_ACCELERATION_TIME_PROPERTY->items + 0)
#define X_ACCELERATION_TIME_SHUTTER_ITEM      (X_ACCELERATION_TIME_PROPERTY->items + 1)

#define X_ACCELERATION_TIME_PROPERTY_NAME     "X_ACCELERATION_TIME"
#define X_ACCELERATION_TIME_ROTATOR_ITEM_NAME "ROTATOR"
#define X_ACCELERATION_TIME_SHUTTER_ITEM_NAME "SHUTTER"

#define X_VELOCITY_PROPERTY            (PRIVATE_DATA->x_velocity_property)
#define X_VELOCITY_ROTATOR_ITEM        (X_VELOCITY_PROPERTY->items + 0)
#define X_VELOCITY_SHUTTER_ITEM        (X_VELOCITY_PROPERTY->items + 1)

#define X_VELOCITY_PROPERTY_NAME       "X_VELOCITY"
#define X_VELOCITY_ROTATOR_ITEM_NAME   "ROTATOR"
#define X_VELOCITY_SHUTTER_ITEM_NAME   "SHUTTER"

#define X_RANGE_PROPERTY               (PRIVATE_DATA->x_range_property)
#define X_RANGE_ROTATOR_ITEM           (X_RANGE_PROPERTY->items + 0)
#define X_RANGE_SHUTTER_ITEM           (X_RANGE_PROPERTY->items + 1)

#define X_RANGE_PROPERTY_NAME          "X_RANGE"
#define X_RANGE_ROTATOR_ITEM_NAME      "ROTATOR"
#define X_RANGE_SHUTTER_ITEM_NAME      "SHUTTER"

#define X_SETTINGS_PROPERTY            (PRIVATE_DATA->x_settings_property)
#define X_SETTINGS_LOAD_ITEM           (X_SETTINGS_PROPERTY->items + 0)
#define X_SETTINGS_SAVE_ITEM           (X_SETTINGS_PROPERTY->items + 1)
#define X_SETTINGS_DEFAULT_ITEM        (X_SETTINGS_PROPERTY->items + 2)

#define X_SETTINGS_PROPERTY_NAME       "X_SETTINGS"
#define X_SETTINGS_LOAD_ITEM_NAME      "LOAD_EEPROM"
#define X_SETTINGS_SAVE_ITEM_NAME      "SAVE_EEPROM"
#define X_SETTINGS_DEFAULT_ITEM_NAME   "LOAD_DEFAULT"

#define X_RAIN_SENSOR_PROPERTY         (PRIVATE_DATA->x_rain_sensor_property)
#define X_RAIN_SENSOR_ALERT_ITEM       (X_RAIN_SENSOR_PROPERTY->items + 0)

#define X_RAIN_SENSOR_PROPERTY_NAME    "X_RAIN_SENSOR"
#define X_RAIN_SENSOR_ALERT_ITEM_NAME  "RAIN_ALERT"

#define X_XB_STATE_PROPERTY            (PRIVATE_DATA->x_xb_state_property)
#define X_XB_STATE_ITEM                (X_XB_STATE_PROPERTY->items + 0)

#define X_XB_STATE_PROPERTY_NAME       "X_XB_STATE"
#define X_XB_STATE_ITEM_NAME           "XB_STATE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_find_home_property;
	indigo_property *x_home_position_property;
	indigo_property *x_move_threshold_property;
	indigo_property *x_battery_power_property;
	indigo_property *x_acceleration_time_property;
	indigo_property *x_velocity_property;
	indigo_property *x_range_property;
	indigo_property *x_settings_property;
	indigo_property *x_rain_sensor_property;
	indigo_property *x_xb_state_property;
	//+ data
	char command[64];
	char messages[NEXDOME3_MESSAGE_COUNT][NEXDOME3_MESSAGE_SIZE], processed[NEXDOME3_MESSAGE_COUNT][NEXDOME3_MESSAGE_SIZE];
	int message_count;
	bool messages_scheduled, reader_running;
	pthread_mutex_t message_mutex;
	indigo_timer *reader;
	char pending[NEXDOME3_PENDING_COUNT][4];
	double pending_time[NEXDOME3_PENDING_COUNT];
	int pending_count;
	int version;
	double steps_per_degree, position;
	int dead_zone;
	bool low_voltage, park_detection;
	bool rotation_active, rotation_moving, rotation_stop_requested, park_active, home_active, rotation_check_scheduled;
	double rotation_motion_time, rotation_status_time;
	bool shutter_active, shutter_moving, shutter_motion_seen, shutter_stop_requested, shutter_open_target, shutter_closed, shutter_check_scheduled;
	double shutter_request_time, shutter_motion_time;
	//- data
} nexdome3_private_data;

#pragma mark - Low level code

//+ code

// writes one command; the reply arrives through the reader like any other message
static bool nexdome3_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(PRIVATE_DATA->command, sizeof(PRIVATE_DATA->command), format, args);
	va_end(args);
	if (indigo_uni_printf(PRIVATE_DATA->handle, "@%s\n", PRIVATE_DATA->command) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write %s", PRIVATE_DATA->command);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command -> %s", PRIVATE_DATA->command);
	// commands are answered in order: remember the command code for reply matching
	if (PRIVATE_DATA->pending_count == NEXDOME3_PENDING_COUNT) {
		memmove(PRIVATE_DATA->pending, PRIVATE_DATA->pending + 1, sizeof(PRIVATE_DATA->pending[0]) * (NEXDOME3_PENDING_COUNT - 1));
		memmove(PRIVATE_DATA->pending_time, PRIVATE_DATA->pending_time + 1, sizeof(PRIVATE_DATA->pending_time[0]) * (NEXDOME3_PENDING_COUNT - 1));
		PRIVATE_DATA->pending_count--;
	}
	snprintf(PRIVATE_DATA->pending[PRIVATE_DATA->pending_count], sizeof(PRIVATE_DATA->pending[0]), "%.3s", PRIVATE_DATA->command);
	PRIVATE_DATA->pending_time[PRIVATE_DATA->pending_count++] = indigo_monotonic_time();
	return true;
}

static void nexdome3_drop_pending(indigo_device *device, int count) {
	memmove(PRIVATE_DATA->pending, PRIVATE_DATA->pending + count, sizeof(PRIVATE_DATA->pending[0]) * (PRIVATE_DATA->pending_count - count));
	memmove(PRIVATE_DATA->pending_time, PRIVATE_DATA->pending_time + count, sizeof(PRIVATE_DATA->pending_time[0]) * (PRIVATE_DATA->pending_count - count));
	PRIVATE_DATA->pending_count -= count;
}

// returns true when the reply code answers a pending command; unanswered older commands are dropped
static bool nexdome3_match_reply(indigo_device *device, const char *code) {
	double now = indigo_monotonic_time();
	while (PRIVATE_DATA->pending_count > 0 && now - PRIVATE_DATA->pending_time[0] > NEXDOME3_STATUS_TIMEOUT) {
		nexdome3_drop_pending(device, 1);
	}
	for (int i = 0; i < PRIVATE_DATA->pending_count; i++) {
		if (!strcmp(PRIVATE_DATA->pending[i], code)) {
			nexdome3_drop_pending(device, i + 1);
			return true;
		}
	}
	return false;
}

static void request_settings(indigo_device *device) {
	nexdome3_command(device, "ARR");
	nexdome3_command(device, "ARS");
	nexdome3_command(device, "DRR");
	nexdome3_command(device, "HRR");
	nexdome3_command(device, "PRR");
	nexdome3_command(device, "PRS");
	nexdome3_command(device, "VRR");
	nexdome3_command(device, "VRS");
	nexdome3_command(device, "RRR");
	nexdome3_command(device, "RRS");
}

static bool nexdome3_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "nexdome")) {
		PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			// to be on the safe side
			indigo_sleep(NEXDOME3_RESET_DELAY);
		}
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, NEXDOME3_NETWORK_PORT, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", name);
		indigo_global_unlock(device);
		return false;
	}
	char response[NEXDOME3_MESSAGE_SIZE], firmware[NEXDOME3_MESSAGE_SIZE] = "";
	PRIVATE_DATA->pending_count = 0;
	if (nexdome3_command(device, "FRR")) {
		// events can arrive before the version reply
		double deadline = indigo_monotonic_time() + NEXDOME3_IDENTIFICATION_TIMEOUT;
		for (int i = 0; i < NEXDOME3_IDENTIFICATION_MESSAGES && !*firmware; i++) {
			double remaining = deadline - indigo_monotonic_time();
			if (remaining <= 0) {
				break;
			}
			long length = indigo_uni_read_section2(PRIVATE_DATA->handle, response, sizeof(response) - 1, "#\n\r", "", INDIGO_DELAY(remaining), INDIGO_DELAY(NEXDOME3_READ_TIMEOUT));
			if (length <= 0) {
				break;
			}
			response[length] = 0;
			if (length == 1 && response[0] != '#') {
				// the undocumented line break after a reply
				i--;
				continue;
			}
			if (!strncmp(response, ":FR", 3) && response[length - 1] == '#' && length > 4) {
				snprintf(firmware, sizeof(firmware), "%.*s", (int)(length - 4), response + 3);
			}
		}
	}
	PRIVATE_DATA->pending_count = 0;
	int major = 0, minor = 0;
	if (*firmware && sscanf(firmware, "%d.%d", &major, &minor) >= 1) {
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "NexDome");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
		PRIVATE_DATA->version = (major << 8) + minor;
		indigo_update_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s with firmware V.%s (%04x) connected.", "NexDome", firmware, PRIVATE_DATA->version);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: NexDome did not respond. Are you using the correct firmware?");
	indigo_send_message(device, CONNECTION_PROPERTY, "NexDome did not respond. Are you using the correct firmware?");
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	return false;
}

static void nexdome3_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
}

//- code

//+ dome.code

static void nexdome3_process_messages(indigo_device *device);

// reads notifications and replies on its own thread and hands them over to the device queue
static void nexdome3_reader(indigo_device *device) {
	indigo_rename_thread("NexDome3 reader");
	char message[NEXDOME3_MESSAGE_SIZE];
	bool discard = false, failed = false;
	while (PRIVATE_DATA->reader_running) {
		double started = indigo_monotonic_time();
		long length = indigo_uni_read_section2(PRIVATE_DATA->handle, message, sizeof(message) - 1, "#\n\r", "", INDIGO_DELAY(NEXDOME3_READ_TIMEOUT), INDIGO_DELAY(NEXDOME3_READ_TIMEOUT));
		if (length <= 0) {
			// an immediate empty read is a closed or failed transport, not a timeout
			if (length < 0 || indigo_monotonic_time() - started < NEXDOME3_READ_TIMEOUT / 2) {
				if (!failed) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s", DEVICE_PORT_ITEM->text.value);
				}
				failed = true;
				indigo_usleep(NEXDOME3_TRANSPORT_RETRY_DELAY);
			}
			continue;
		}
		failed = false;
		message[length] = 0;
		bool terminated = strchr("#\n\r", message[length - 1]) != NULL;
		if (discard || !terminated) {
			// the rest of an overlong line is not a message
			if (!discard) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Overlong message ignored");
			}
			discard = !terminated;
			continue;
		}
		if (message[length - 1] != '#') {
			message[--length] = 0;
		}
		if (length == 0) {
			continue;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Response -> %s", message);
		pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
		bool schedule = false;
		if (PRIVATE_DATA->message_count < NEXDOME3_MESSAGE_COUNT) {
			strcpy(PRIVATE_DATA->messages[PRIVATE_DATA->message_count++], message);
			schedule = !PRIVATE_DATA->messages_scheduled;
			PRIVATE_DATA->messages_scheduled = true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Message queue full, '%s' ignored", message);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
		if (schedule) {
			indigo_execute_handler(device, nexdome3_process_messages);
		}
	}
}

static void nexdome3_clear_messages(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	PRIVATE_DATA->message_count = 0;
	PRIVATE_DATA->messages_scheduled = false;
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static double nexdome3_azimuth(indigo_device *device, double position) {
	double azimuth = fmod(position / PRIVATE_DATA->steps_per_degree, 360);
	return azimuth < 0 ? azimuth + 360 : azimuth;
}

// a park request cannot get closer to the park position than the dead zone
static bool nexdome3_in_park_position(indigo_device *device, bool dead_zone) {
	double tolerance = dead_zone ? PRIVATE_DATA->dead_zone / PRIVATE_DATA->steps_per_degree : 1;
	return indigo_azimuth_distance(0, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value) <= (tolerance > 1 ? tolerance : 1);
}

static void nexdome3_update_shutter_switches(indigo_device *device) {
	indigo_set_switch(DOME_SHUTTER_PROPERTY, PRIVATE_DATA->shutter_closed ? DOME_SHUTTER_CLOSED_ITEM : DOME_SHUTTER_OPENED_ITEM, true);
}

static void nexdome3_rotation_failed(indigo_device *device) {
	PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_moving = PRIVATE_DATA->rotation_stop_requested = false;
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	if (DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->park_active) {
		PRIVATE_DATA->park_active = false;
		DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->home_active) {
		PRIVATE_DATA->home_active = false;
		indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
		X_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_FIND_HOME_PROPERTY, NULL);
	}
}

static void nexdome3_shutter_failed(indigo_device *device, const char *message) {
	PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_moving = false;
	nexdome3_update_shutter_switches(device);
	DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, message);
}

// watches a rotation request until the controller reports that the motor stopped
static void rotation_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->rotation_active) {
		PRIVATE_DATA->rotation_check_scheduled = false;
		return;
	}
	double now = indigo_monotonic_time();
	if (PRIVATE_DATA->rotation_moving) {
		if (now - PRIVATE_DATA->rotation_motion_time > NEXDOME3_STALL_TIMEOUT) {
			// no position report: the next status report ends the motion
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotation reports stopped, requesting status");
			PRIVATE_DATA->rotation_moving = false;
			PRIVATE_DATA->rotation_status_time = now;
			nexdome3_command(device, "SRR");
		}
	} else if (PRIVATE_DATA->rotation_status_time > 0 && now - PRIVATE_DATA->rotation_status_time > NEXDOME3_STATUS_TIMEOUT) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No rotator status report");
		PRIVATE_DATA->rotation_check_scheduled = false;
		nexdome3_rotation_failed(device);
		return;
	}
	indigo_execute_handler_in(device, NEXDOME3_CHECK_DELAY, rotation_finalizer);
}

// the status is requested 3 s after every rotation request
static void rotator_status_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	if (PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_moving) {
		PRIVATE_DATA->rotation_status_time = indigo_monotonic_time();
	}
	nexdome3_command(device, "SRR");
	if (PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_check_scheduled) {
		PRIVATE_DATA->rotation_check_scheduled = true;
		indigo_execute_handler_in(device, NEXDOME3_CHECK_DELAY, rotation_finalizer);
	}
}

static void nexdome3_start_rotation(indigo_device *device) {
	PRIVATE_DATA->rotation_active = true;
	PRIVATE_DATA->rotation_moving = false;
	PRIVATE_DATA->rotation_status_time = 0;
	indigo_execute_handler_in(device, NEXDOME3_STATUS_DELAY, rotator_status_finalizer);
}

// watches a shutter request until the shutter reports the requested end position
static void shutter_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->shutter_active) {
		PRIVATE_DATA->shutter_check_scheduled = false;
		return;
	}
	double now = indigo_monotonic_time();
	if (!PRIVATE_DATA->shutter_motion_seen && now - PRIVATE_DATA->shutter_request_time > NEXDOME3_SHUTTER_TIMEOUT) {
		PRIVATE_DATA->shutter_check_scheduled = false;
		nexdome3_shutter_failed(device, "Shutter did not respond");
		return;
	}
	if (PRIVATE_DATA->shutter_moving && now - PRIVATE_DATA->shutter_motion_time > NEXDOME3_STALL_TIMEOUT) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Shutter reports stopped, requesting status");
		PRIVATE_DATA->shutter_moving = false;
		PRIVATE_DATA->shutter_motion_time = now;
		nexdome3_command(device, "SRS");
	}
	indigo_execute_handler_in(device, NEXDOME3_CHECK_DELAY, shutter_finalizer);
}

// the status is requested 3 s after every shutter request
static void shutter_status_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	nexdome3_command(device, "SRS");
	if (PRIVATE_DATA->shutter_active && !PRIVATE_DATA->shutter_check_scheduled) {
		PRIVATE_DATA->shutter_check_scheduled = true;
		indigo_execute_handler_in(device, NEXDOME3_CHECK_DELAY, shutter_finalizer);
	}
}

static void handle_rotator_position(indigo_device *device, const char *message) {
	int position;
	char end;
	if (sscanf(message, ":PRR%d%c", &position, &end) != 2 && sscanf(message, "P%d%c", &position, &end) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	PRIVATE_DATA->position = position;
	if (*message == 'P') {
		PRIVATE_DATA->rotation_motion_time = indigo_monotonic_time();
	}
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = nexdome3_azimuth(device, position);
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
}

static void handle_rotator_move(indigo_device *device, const char *message) {
	PRIVATE_DATA->rotation_moving = true;
	PRIVATE_DATA->rotation_motion_time = indigo_monotonic_time();
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	if (PRIVATE_DATA->home_active) {
		X_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FIND_HOME_PROPERTY, "Going home...");
	}
	if (PRIVATE_DATA->park_active) {
		DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_PARK_PROPERTY, "Going to park position...");
	}
}

static void handle_shutter_move(indigo_device *device, const char *message) {
	PRIVATE_DATA->shutter_moving = PRIVATE_DATA->shutter_motion_seen = true;
	PRIVATE_DATA->shutter_motion_time = indigo_monotonic_time();
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, message[1] == 'c' ? "Shutter is closing..." : "Shutter is opening...");
}

static void handle_rotator_status(indigo_device *device, const char *message, bool reply) {
	int position, at_home, max_position, home_position, dead_zone;
	char end;
	if (sscanf(message, ":SER,%d,%d,%d,%d,%d%c", &position, &at_home, &max_position, &home_position, &dead_zone, &end) != 6 || end != '#' || max_position <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	PRIVATE_DATA->steps_per_degree = max_position / 360.0;
	PRIVATE_DATA->position = position;
	PRIVATE_DATA->dead_zone = dead_zone;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = nexdome3_azimuth(device, position);
	if (reply && PRIVATE_DATA->rotation_moving) {
		// a requested status report while the motor moves
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		return;
	}
	bool stopped = PRIVATE_DATA->rotation_moving || PRIVATE_DATA->rotation_active;
	// a requested status report does not hide a failed request
	bool failed = reply && !stopped;
	PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_moving = false;
	if (!failed || DOME_HORIZONTAL_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (!failed || DOME_STEPS_PROPERTY->state != INDIGO_ALERT_STATE) {
		DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (PRIVATE_DATA->home_active && at_home) {
		X_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
		indigo_update_property(device, X_FIND_HOME_PROPERTY, "Dome is at home.");
		PRIVATE_DATA->home_active = false;
	}
	if ((PRIVATE_DATA->park_active && nexdome3_in_park_position(device, true)) || (PRIVATE_DATA->park_detection && nexdome3_in_park_position(device, false))) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		PRIVATE_DATA->park_active = false;
	}
	PRIVATE_DATA->park_detection = false;
	if (PRIVATE_DATA->rotation_stop_requested || stopped) {
		PRIVATE_DATA->rotation_stop_requested = false;
		// a find home or park that stopped elsewhere has failed
		if (PRIVATE_DATA->home_active) {
			PRIVATE_DATA->home_active = false;
			indigo_set_switch(X_FIND_HOME_PROPERTY, X_FIND_HOME_ITEM, false);
			X_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_FIND_HOME_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->park_active) {
			PRIVATE_DATA->park_active = false;
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
		}
	}
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}

static void handle_shutter_status(indigo_device *device, const char *message, bool reply) {
	int position, max_position, open_switch, close_switch;
	char end;
	if (sscanf(message, ":SES,%d,%d,%d,%d%c", &position, &max_position, &open_switch, &close_switch, &end) != 5 || end != '#' || max_position <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	if (!reply) {
		PRIVATE_DATA->shutter_moving = false;
	}
	bool closed = close_switch || position <= 0, open = !closed && (open_switch || position >= max_position);
	if (PRIVATE_DATA->shutter_active && !PRIVATE_DATA->shutter_motion_seen && ((PRIVATE_DATA->shutter_open_target && closed) || (!PRIVATE_DATA->shutter_open_target && open))) {
		// the shutter has not started to move over the wireless link yet
		PRIVATE_DATA->shutter_closed = closed;
		return;
	}
	if (closed) {
		PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_stop_requested = false;
		PRIVATE_DATA->shutter_closed = true;
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, close_switch ? "Shutter is closed." : "Shutter is closed, but end swich is not activated.");
	} else if (open) {
		PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_stop_requested = false;
		PRIVATE_DATA->shutter_closed = false;
		DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, open_switch ? "Shutter is open." : "Shutter is open, but end swich is not activated.");
	} else if (PRIVATE_DATA->shutter_stop_requested) {
		PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_stop_requested = PRIVATE_DATA->shutter_moving = false;
		PRIVATE_DATA->shutter_closed = false;
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter stopped.");
	} else if (PRIVATE_DATA->shutter_moving) {
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	} else {
		// stopped between the end positions
		PRIVATE_DATA->shutter_active = false;
		PRIVATE_DATA->shutter_closed = false;
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	}
}

static void handle_battery_status(indigo_device *device, const char *message) {
	int adc_value;
	char end;
	if (sscanf(message, ":BV%d%c", &adc_value, &end) != 2 || end != '#') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	/* 15 / 1024 =  0.01465 V/ADU */
	double volts = NEXDOME3_VOLTS_PER_ADU * adc_value;
	if (volts < NEXDOME3_VOLT_THRESHOLD) {
		if (!PRIVATE_DATA->low_voltage) {
			indigo_send_message(device, ALERT_PROPERTY, "Dome power is low! (U = %.2fV)", volts);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Dome power is low! (U = %.2fV", volts);
		}
		PRIVATE_DATA->low_voltage = true;
	} else {
		if (PRIVATE_DATA->low_voltage) {
			indigo_send_message(device, IDLE_PROPERTY, "Dome power is normal! (U = %.2fV)", volts);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Dome power is normal! (U = %.2fV)", volts);
		}
		PRIVATE_DATA->low_voltage = false;
	}
	if (fabs((volts - X_BATTERY_POWER_VOLTAGE_ITEM->number.value) * 100) >= 1) {
		X_BATTERY_POWER_VOLTAGE_ITEM->number.value = volts;
		indigo_update_property(device, X_BATTERY_POWER_PROPERTY, NULL);
	}
}

static void handle_home_position(indigo_device *device, const char *message) {
	int home_position;
	char end;
	if (sscanf(message, ":HRR%d%c", &home_position, &end) != 2 || end != '#') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	if (fabs(home_position - X_HOME_POSITION_ITEM->number.value) >= 1) {
		X_HOME_POSITION_ITEM->number.value = home_position;
		indigo_update_property(device, X_HOME_POSITION_PROPERTY, NULL);
	}
}

static void handle_move_threshold(indigo_device *device, const char *message) {
	int dead_zone;
	char end;
	if (sscanf(message, ":DRR%d%c", &dead_zone, &end) != 2 || end != '#') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	PRIVATE_DATA->dead_zone = dead_zone;
	if (fabs(dead_zone - X_MOVE_THRESHOLD_ITEM->number.value) >= 1) {
		X_MOVE_THRESHOLD_ITEM->number.value = dead_zone;
		indigo_update_property(device, X_MOVE_THRESHOLD_PROPERTY, NULL);
	}
	if (dead_zone > (DOME_SLAVING_THRESHOLD_ITEM->number.value * PRIVATE_DATA->steps_per_degree)) {
		DOME_SLAVING_THRESHOLD_ITEM->number.value = dead_zone / PRIVATE_DATA->steps_per_degree;
		DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, "Dome synchrinization threshold updated as it can not be less than the dome minimal move.");
	}
}

// :ARR, :ARS, :VRR, :VRS, :RRR and :RRS update the rotator or shutter item of a property
static void handle_pair(indigo_device *device, const char *message, const char *format, indigo_property *property) {
	int value;
	char target, end;
	if (sscanf(message, format, &target, &value, &end) != 3 || end != '#' || (target != 'R' && target != 'S')) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	property->items[target == 'R' ? 0 : 1].number.value = value;
	indigo_update_property(device, property, NULL);
	if (property == X_RANGE_PROPERTY && target == 'R' && value > 0) {
		// the circumference defines the step conversion
		PRIVATE_DATA->steps_per_degree = value / 360.0;
	}
}

static void handle_xb(indigo_device *device, const char *message) {
	char state[20];
	if (sscanf(message, "XB->%19s", state) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing message = '%s' error!", message);
		return;
	}
	if (strcmp(state, X_XB_STATE_ITEM->text.value)) {
		indigo_set_text_item_value(X_XB_STATE_ITEM, state);
		X_XB_STATE_PROPERTY->state = strcmp(state, "Online") ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, X_XB_STATE_PROPERTY, NULL);
	}
}

static void handle_rain(indigo_device *device, const char *message) {
	if (!strcmp(message, ":Rain#")) {
		X_RAIN_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
		X_RAIN_SENSOR_ALERT_ITEM->light.value = INDIGO_ALERT_STATE;
	} else if (!strcmp(message, ":RainStopped#")) {
		X_RAIN_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
		X_RAIN_SENSOR_ALERT_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, X_RAIN_SENSOR_PROPERTY, NULL);
}

// a command rejected by the controller fails the operation it started
static void handle_error(indigo_device *device) {
	char code[4] = "";
	if (PRIVATE_DATA->pending_count > 0) {
		strcpy(code, PRIVATE_DATA->pending[0]);
		nexdome3_drop_pending(device, 1);
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command %s rejected", *code ? code : "(unknown)");
	if ((!strcmp(code, "GAR") || !strcmp(code, "GSR") || !strcmp(code, "GHR") || !strcmp(code, "PWR")) && (PRIVATE_DATA->rotation_active || PRIVATE_DATA->home_active)) {
		nexdome3_rotation_failed(device);
	} else if ((!strcmp(code, "OPS") || !strcmp(code, "CLS")) && PRIVATE_DATA->shutter_active) {
		nexdome3_shutter_failed(device, NULL);
	}
}

static void nexdome3_process_message(indigo_device *device, const char *message) {
	if (!strncmp(message, "P", 1) || !strncmp(message, ":PRR", 4)) {
		if (*message == ':') {
			nexdome3_match_reply(device, "PRR");
		}
		handle_rotator_position(device, message);
	} else if (!strncmp(message, "S", 1) || !strncmp(message, ":PRS", 4)) {
		if (*message == ':') {
			nexdome3_match_reply(device, "PRS");
		} else if (PRIVATE_DATA->shutter_moving) {
			PRIVATE_DATA->shutter_motion_time = indigo_monotonic_time();
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
	} else if (!strcmp(message, ":left#") || !strcmp(message, ":right#")) {
		handle_rotator_move(device, message);
	} else if (!strcmp(message, ":open#") || !strcmp(message, ":close#")) {
		handle_shutter_move(device, message);
	} else if (!strncmp(message, ":SER", 4)) {
		handle_rotator_status(device, message, nexdome3_match_reply(device, "SRR"));
	} else if (!strncmp(message, ":SES", 4)) {
		handle_shutter_status(device, message, nexdome3_match_reply(device, "SRS"));
	} else if (!strncmp(message, ":BV", 3)) {
		handle_battery_status(device, message);
	} else if (!strncmp(message, ":HRR", 4)) {
		nexdome3_match_reply(device, "HRR");
		handle_home_position(device, message);
	} else if (!strncmp(message, ":DRR", 4)) {
		nexdome3_match_reply(device, "DRR");
		handle_move_threshold(device, message);
	} else if (!strncmp(message, ":AR", 3)) {
		nexdome3_match_reply(device, message[3] == 'S' ? "ARS" : "ARR");
		handle_pair(device, message, ":AR%c%d%c", X_ACCELERATION_TIME_PROPERTY);
	} else if (!strncmp(message, ":VR", 3)) {
		nexdome3_match_reply(device, message[3] == 'S' ? "VRS" : "VRR");
		handle_pair(device, message, ":VR%c%d%c", X_VELOCITY_PROPERTY);
	} else if (!strncmp(message, ":RR", 3)) {
		nexdome3_match_reply(device, message[3] == 'S' ? "RRS" : "RRR");
		handle_pair(device, message, ":RR%c%d%c", X_RANGE_PROPERTY);
	} else if (!strncmp(message, "XB->", 4)) {
		handle_xb(device, message);
	} else if (!strncmp(message, ":Rain", 5)) {
		handle_rain(device, message);
	} else if (!strcmp(message, ":Err#")) {
		handle_error(device);
	} else if (*message == ':' && strlen(message) == 5 && message[4] == '#') {
		char code[4];
		snprintf(code, sizeof(code), "%.3s", message + 1);
		nexdome3_match_reply(device, code);
	}
}

static void nexdome3_process_messages(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	int count = PRIVATE_DATA->message_count;
	memcpy(PRIVATE_DATA->processed, PRIVATE_DATA->messages, sizeof(PRIVATE_DATA->messages[0]) * count);
	PRIVATE_DATA->message_count = 0;
	PRIVATE_DATA->messages_scheduled = false;
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
	if (!IS_CONNECTED) {
		return;
	}
	for (int i = 0; i < count; i++) {
		nexdome3_process_message(device, PRIVATE_DATA->processed[i]);
	}
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = nexdome3_open(device);
		if (connection_result) {
			//+ dome.on_connect
			PRIVATE_DATA->steps_per_degree = NEXDOME3_DEFAULT_STEPS_PER_DEGREE;
			PRIVATE_DATA->dead_zone = 0;
			PRIVATE_DATA->low_voltage = false;
			// operations interrupted by a disconnection are not resumed
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_moving = PRIVATE_DATA->rotation_stop_requested = PRIVATE_DATA->park_active = PRIVATE_DATA->home_active = PRIVATE_DATA->rotation_check_scheduled = false;
			PRIVATE_DATA->shutter_active = PRIVATE_DATA->shutter_moving = PRIVATE_DATA->shutter_motion_seen = PRIVATE_DATA->shutter_stop_requested = PRIVATE_DATA->shutter_check_scheduled = false;
			X_FIND_HOME_PROPERTY->state = X_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
			X_FIND_HOME_ITEM->sw.value = false;
			// the dome is parked when the first status report is in the park position
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_detection = true;
			nexdome3_clear_messages(device);
			PRIVATE_DATA->reader_running = true;
			indigo_set_timer(device, 0, nexdome3_reader, &PRIVATE_DATA->reader);
			/* Request Rotator and Shutter report to set the current values */
			nexdome3_command(device, "SRR");
			nexdome3_command(device, "SRS");
			request_settings(device);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FIND_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_HOME_POSITION_PROPERTY, NULL);
			indigo_define_property(device, X_MOVE_THRESHOLD_PROPERTY, NULL);
			indigo_define_property(device, X_BATTERY_POWER_PROPERTY, NULL);
			indigo_define_property(device, X_ACCELERATION_TIME_PROPERTY, NULL);
			indigo_define_property(device, X_VELOCITY_PROPERTY, NULL);
			indigo_define_property(device, X_RANGE_PROPERTY, NULL);
			indigo_define_property(device, X_SETTINGS_PROPERTY, NULL);
			indigo_define_property(device, X_RAIN_SENSOR_PROPERTY, NULL);
			indigo_define_property(device, X_XB_STATE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ dome.on_disconnect
		PRIVATE_DATA->reader_running = false;
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
		nexdome3_clear_messages(device);
		PRIVATE_DATA->pending_count = 0;
		//- dome.on_disconnect
		indigo_delete_property(device, X_FIND_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_HOME_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, X_MOVE_THRESHOLD_PROPERTY, NULL);
		indigo_delete_property(device, X_BATTERY_POWER_PROPERTY, NULL);
		indigo_delete_property(device, X_ACCELERATION_TIME_PROPERTY, NULL);
		indigo_delete_property(device, X_VELOCITY_PROPERTY, NULL);
		indigo_delete_property(device, X_RANGE_PROPERTY, NULL);
		indigo_delete_property(device, X_SETTINGS_PROPERTY, NULL);
		indigo_delete_property(device, X_RAIN_SENSOR_PROPERTY, NULL);
		indigo_delete_property(device, X_XB_STATE_PROPERTY, NULL);
		nexdome3_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_slaving_parameters_handler(indigo_device *device) {
	DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_SLAVING_PARAMETERS.on_change
	if (X_MOVE_THRESHOLD_ITEM->number.value > (DOME_SLAVING_THRESHOLD_ITEM->number.value * PRIVATE_DATA->steps_per_degree)) {
		DOME_SLAVING_THRESHOLD_ITEM->number.value = X_MOVE_THRESHOLD_ITEM->number.value / PRIVATE_DATA->steps_per_degree;
		DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, "Requested synchronization threshold is less than the dome minimal move. Minimal move is used.");
		return;
	}
	//- dome.DOME_SLAVING_PARAMETERS.on_change
	indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
}

static void dome_horizontal_coordinates_handler(indigo_device *device) {
	//+ dome.DOME_HORIZONTAL_COORDINATES.on_change
	double target_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked.");
		nexdome3_command(device, "PRR");
		return;
	}
	PRIVATE_DATA->park_active = PRIVATE_DATA->home_active = PRIVATE_DATA->park_detection = false;
	bool result;
	if (DOME_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		result = nexdome3_command(device, "PWR,%.0f", target_position * PRIVATE_DATA->steps_per_degree);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = result ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (result) {
			indigo_execute_handler_in(device, NEXDOME3_STATUS_DELAY, rotator_status_finalizer);
		}
	} else {
		if (PRIVATE_DATA->version < NEXDOME3_FIRMWARE_VERSION_3_2) {
			result = nexdome3_command(device, "GAR,%.0f", target_position);
		} else {
			result = nexdome3_command(device, "GSR,%.0f", target_position * PRIVATE_DATA->steps_per_degree);
		}
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = result ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
		if (result) {
			nexdome3_start_rotation(device);
		}
	}
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	//- dome.DOME_HORIZONTAL_COORDINATES.on_change
}

static void dome_steps_handler(indigo_device *device) {
	DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_STEPS.on_change
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked.");
		return;
	}
	double current_position = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, target_position = current_position;
	// the step is a whole number of degrees, the current heading keeps its fraction
	DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
	if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
		target_position = fmod(current_position - DOME_STEPS_ITEM->number.value + 360, 360);
	} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
		target_position = fmod(current_position + DOME_STEPS_ITEM->number.value + 360, 360);
	}
	PRIVATE_DATA->park_active = PRIVATE_DATA->home_active = PRIVATE_DATA->park_detection = false;
	if (nexdome3_command(device, "GAR,%.0f", target_position)) {
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		nexdome3_start_rotation(device);
	} else {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- dome.DOME_STEPS.on_change
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	if (DOME_ABORT_MOTION_ITEM->sw.value) {
		nexdome3_command(device, "SWR");
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || PRIVATE_DATA->rotation_active || PRIVATE_DATA->home_active) {
			PRIVATE_DATA->rotation_stop_requested = true;
		}
		nexdome3_command(device, "SWS");
		if (DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
			PRIVATE_DATA->shutter_stop_requested = true;
		}
	}
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	bool open = DOME_SHUTTER_OPENED_ITEM->sw.value;
	if (!nexdome3_command(device, open ? "OPS" : "CLS")) {
		nexdome3_update_shutter_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->shutter_active = true;
		PRIVATE_DATA->shutter_open_target = open;
		PRIVATE_DATA->shutter_motion_seen = PRIVATE_DATA->shutter_stop_requested = false;
		PRIVATE_DATA->shutter_request_time = indigo_monotonic_time();
		DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, NEXDOME3_STATUS_DELAY, shutter_status_finalizer);
	}
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	//- dome.DOME_SHUTTER.on_change
}

static void dome_park_handler(indigo_device *device) {
	DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_PARK.on_change
	if (DOME_PARK_UNPARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_active = PRIVATE_DATA->park_detection = false;
	} else if (nexdome3_in_park_position(device, true)) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		PRIVATE_DATA->home_active = false;
		if (nexdome3_command(device, "GAR,0")) {
			PRIVATE_DATA->park_active = true;
			nexdome3_command(device, "PRR");
			DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			nexdome3_start_rotation(device);
		} else {
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- dome.DOME_PARK.on_change
	indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
}

static void dome_x_find_home_handler(indigo_device *device) {
	X_FIND_HOME_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_FIND_HOME.on_change
	if (X_FIND_HOME_ITEM->sw.value) {
		PRIVATE_DATA->park_active = PRIVATE_DATA->park_detection = false;
		if (nexdome3_command(device, "GHR")) {
			PRIVATE_DATA->home_active = true;
			X_FIND_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			X_FIND_HOME_ITEM->sw.value = false;
			X_FIND_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- dome.X_FIND_HOME.on_change
	indigo_update_property(device, X_FIND_HOME_PROPERTY, NULL);
}

static void dome_x_home_position_handler(indigo_device *device) {
	X_HOME_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_HOME_POSITION.on_change
	nexdome3_command(device, "HWR,%.0f", X_HOME_POSITION_ITEM->number.value);
	nexdome3_command(device, "HRR");
	//- dome.X_HOME_POSITION.on_change
	indigo_update_property(device, X_HOME_POSITION_PROPERTY, NULL);
}

static void dome_x_move_threshold_handler(indigo_device *device) {
	X_MOVE_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_MOVE_THRESHOLD.on_change
	nexdome3_command(device, "DWR,%.0f", X_MOVE_THRESHOLD_ITEM->number.value);
	nexdome3_command(device, "DRR");
	//- dome.X_MOVE_THRESHOLD.on_change
	indigo_update_property(device, X_MOVE_THRESHOLD_PROPERTY, NULL);
}

static void dome_x_acceleration_time_handler(indigo_device *device) {
	X_ACCELERATION_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_ACCELERATION_TIME.on_change
	nexdome3_command(device, "AWR,%.0f", X_ACCELERATION_TIME_ROTATOR_ITEM->number.value);
	nexdome3_command(device, "AWS,%.0f", X_ACCELERATION_TIME_SHUTTER_ITEM->number.value);
	nexdome3_command(device, "ARR");
	nexdome3_command(device, "ARS");
	//- dome.X_ACCELERATION_TIME.on_change
	indigo_update_property(device, X_ACCELERATION_TIME_PROPERTY, NULL);
}

static void dome_x_velocity_handler(indigo_device *device) {
	X_VELOCITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_VELOCITY.on_change
	nexdome3_command(device, "VWR,%.0f", X_VELOCITY_ROTATOR_ITEM->number.value);
	nexdome3_command(device, "VWS,%.0f", X_VELOCITY_SHUTTER_ITEM->number.value);
	nexdome3_command(device, "VRR");
	nexdome3_command(device, "VRS");
	//- dome.X_VELOCITY.on_change
	indigo_update_property(device, X_VELOCITY_PROPERTY, NULL);
}

static void dome_x_range_handler(indigo_device *device) {
	X_RANGE_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_RANGE.on_change
	nexdome3_command(device, "RWR,%.0f", X_RANGE_ROTATOR_ITEM->number.value);
	nexdome3_command(device, "RWS,%.0f", X_RANGE_SHUTTER_ITEM->number.value);
	nexdome3_command(device, "RRR");
	nexdome3_command(device, "RRS");
	//- dome.X_RANGE.on_change
	indigo_update_property(device, X_RANGE_PROPERTY, NULL);
}

static void dome_x_settings_handler(indigo_device *device) {
	X_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.X_SETTINGS.on_change
	if (X_SETTINGS_LOAD_ITEM->sw.value) {
		X_SETTINGS_LOAD_ITEM->sw.value = false;
		// Load settings
		nexdome3_command(device, "ZRR");
		nexdome3_command(device, "ZRS");
		request_settings(device);
	} else if (X_SETTINGS_SAVE_ITEM->sw.value) {
		X_SETTINGS_SAVE_ITEM->sw.value = false;
		// Save current settings
		nexdome3_command(device, "ZWR");
		nexdome3_command(device, "ZWS");
	} else if (X_SETTINGS_DEFAULT_ITEM->sw.value) {
		X_SETTINGS_DEFAULT_ITEM->sw.value = false;
		// Load default settings
		nexdome3_command(device, "ZDR");
		nexdome3_command(device, "ZDS");
		request_settings(device);
	}
	//- dome.X_SETTINGS.on_change
	indigo_update_property(device, X_SETTINGS_PROPERTY, NULL);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ dome.on_attach
		INFO_PROPERTY->count = 6;
		pthread_mutex_init(&PRIVATE_DATA->message_mutex, NULL);
		//- dome.on_attach
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_ON_COORDINATES_SET_PROPERTY->hidden = false;
		//+ dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_ON_COORDINATES_SET_PROPERTY->count = 2;
		//- dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = false;
		//+ dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		//- dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_STEPS_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SHUTTER_PROPERTY->hidden = false;
		DOME_PARK_PROPERTY->hidden = false;
		X_FIND_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FIND_HOME_PROPERTY_NAME, "Settings", "Find home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_FIND_HOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FIND_HOME_ITEM, X_FIND_HOME_ITEM_NAME, "Find home sensor", false);
		X_HOME_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, X_HOME_POSITION_PROPERTY_NAME, "Settings", "Home position", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_HOME_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_HOME_POSITION_ITEM, X_HOME_POSITION_ITEM_NAME, "Position (steps, ~153 steps/°)", 0, 100000, 1, 0);
		strcpy(X_HOME_POSITION_ITEM->number.format, "%.0f");
		X_MOVE_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, X_MOVE_THRESHOLD_PROPERTY_NAME, "Settings", "Move threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_MOVE_THRESHOLD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_MOVE_THRESHOLD_ITEM, X_MOVE_THRESHOLD_ITEM_NAME, "Minimal move (steps, ~153 steps/°)", 0, 10000, 1, 300);
		strcpy(X_MOVE_THRESHOLD_ITEM->number.format, "%.0f");
		X_BATTERY_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, X_BATTERY_POWER_PROPERTY_NAME, "Settings", "Power status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_BATTERY_POWER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_BATTERY_POWER_VOLTAGE_ITEM, X_BATTERY_POWER_VOLTAGE_ITEM_NAME, "Battery charge (Volts)", 0, 500, 1, 0);
		strcpy(X_BATTERY_POWER_VOLTAGE_ITEM->number.format, "%.2f");
		X_ACCELERATION_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, X_ACCELERATION_TIME_PROPERTY_NAME, "Settings", "Acceleration time", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_ACCELERATION_TIME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_ACCELERATION_TIME_ROTATOR_ITEM, X_ACCELERATION_TIME_ROTATOR_ITEM_NAME, "Rotator (ms)", 100, 10000, 1, 1500);
		strcpy(X_ACCELERATION_TIME_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_ACCELERATION_TIME_SHUTTER_ITEM, X_ACCELERATION_TIME_SHUTTER_ITEM_NAME, "Shutter (ms)", 100, 10000, 1, 1500);
		strcpy(X_ACCELERATION_TIME_SHUTTER_ITEM->number.format, "%.0f");
		X_VELOCITY_PROPERTY = indigo_init_number_property(NULL, device->name, X_VELOCITY_PROPERTY_NAME, "Settings", "Movement velocity", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_VELOCITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_VELOCITY_ROTATOR_ITEM, X_VELOCITY_ROTATOR_ITEM_NAME, "Rotator (steps/s)", 32, 5000, 1, 600);
		strcpy(X_VELOCITY_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_VELOCITY_SHUTTER_ITEM, X_VELOCITY_SHUTTER_ITEM_NAME, "Shutter (steps/s)", 32, 5000, 1, 800);
		strcpy(X_VELOCITY_SHUTTER_ITEM->number.format, "%.0f");
		X_RANGE_PROPERTY = indigo_init_number_property(NULL, device->name, X_RANGE_PROPERTY_NAME, "Settings", "Movement range", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_RANGE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RANGE_ROTATOR_ITEM, X_RANGE_ROTATOR_ITEM_NAME, "Dome circumference (steps)", 30000, 100000, 1, 55080);
		strcpy(X_RANGE_ROTATOR_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_RANGE_SHUTTER_ITEM, X_RANGE_SHUTTER_ITEM_NAME, "Shutter travel (steps)", 20000, 90000, 1, 46000);
		strcpy(X_RANGE_SHUTTER_ITEM->number.format, "%.0f");
		X_SETTINGS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SETTINGS_PROPERTY_NAME, "Settings", "Settings management", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (X_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SETTINGS_LOAD_ITEM, X_SETTINGS_LOAD_ITEM_NAME, "Load from EEPROM", false);
		indigo_init_switch_item(X_SETTINGS_SAVE_ITEM, X_SETTINGS_SAVE_ITEM_NAME, "Save to EEPROM", false);
		indigo_init_switch_item(X_SETTINGS_DEFAULT_ITEM, X_SETTINGS_DEFAULT_ITEM_NAME, "Load factory defaults", false);
		X_RAIN_SENSOR_PROPERTY = indigo_init_light_property(NULL, device->name, X_RAIN_SENSOR_PROPERTY_NAME, "Settings", "Rain sensor", INDIGO_OK_STATE, 1);
		if (X_RAIN_SENSOR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_RAIN_SENSOR_ALERT_ITEM, X_RAIN_SENSOR_ALERT_ITEM_NAME, "Rain alert", INDIGO_IDLE_STATE);
		X_XB_STATE_PROPERTY = indigo_init_text_property(NULL, device->name, X_XB_STATE_PROPERTY_NAME, "Settings", "Shutter state", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_XB_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_XB_STATE_ITEM, X_XB_STATE_ITEM_NAME, "Shutter state", "");
		//+ dome.X_XB_STATE.on_attach
		X_XB_STATE_PROPERTY->state = INDIGO_IDLE_STATE;
		//- dome.X_XB_STATE.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FIND_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_HOME_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MOVE_THRESHOLD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_BATTERY_POWER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ACCELERATION_TIME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_VELOCITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RANGE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SETTINGS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAIN_SENSOR_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_XB_STATE_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SLAVING_PARAMETERS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SLAVING_PARAMETERS_PROPERTY, dome_slaving_parameters_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_HORIZONTAL_COORDINATES_PROPERTY, dome_horizontal_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_STEPS_PROPERTY, dome_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_PROPERTY, dome_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FIND_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FIND_HOME_PROPERTY, dome_x_find_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HOME_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_HOME_POSITION_PROPERTY, dome_x_home_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MOVE_THRESHOLD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MOVE_THRESHOLD_PROPERTY, dome_x_move_threshold_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ACCELERATION_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ACCELERATION_TIME_PROPERTY, dome_x_acceleration_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_VELOCITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_VELOCITY_PROPERTY, dome_x_velocity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RANGE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RANGE_PROPERTY, dome_x_range_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SETTINGS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SETTINGS_PROPERTY, dome_x_settings_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	//+ dome.on_detach
	pthread_mutex_destroy(&PRIVATE_DATA->message_mutex);
	//- dome.on_detach
	indigo_release_property(X_FIND_HOME_PROPERTY);
	indigo_release_property(X_HOME_POSITION_PROPERTY);
	indigo_release_property(X_MOVE_THRESHOLD_PROPERTY);
	indigo_release_property(X_BATTERY_POWER_PROPERTY);
	indigo_release_property(X_ACCELERATION_TIME_PROPERTY);
	indigo_release_property(X_VELOCITY_PROPERTY);
	indigo_release_property(X_RANGE_PROPERTY);
	indigo_release_property(X_SETTINGS_PROPERTY);
	indigo_release_property(X_RAIN_SENSOR_PROPERTY);
	indigo_release_property(X_XB_STATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_nexdome3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nexdome3_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (nexdome3_private_data *)indigo_safe_malloc(sizeof(nexdome3_private_data));
			dome = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				indigo_safe_free(dome);
				dome = NULL;
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
