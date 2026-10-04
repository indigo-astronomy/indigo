// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_mount_rainbow.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <time.h>
#include <pthread.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_rainbow.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000018
#define DRIVER_NAME          "indigo_mount_rainbow"
#define DRIVER_LABEL         "RainbowAstro Mount"
#define MOUNT_DEVICE_NAME    "RainbowAstro Mount"
#define GUIDER_DEVICE_NAME   "RainbowAstro Mount (guider)"
#define PRIVATE_DATA         ((rainbow_private_data *)device->private_data)

//+ define

#define RAINBOW_MESSAGE_SIZE 128
#define RAINBOW_MESSAGE_COUNT 64

//- define

#pragma mark - Property definitions

#define X_RAINBOW_POWER_PROPERTY          (PRIVATE_DATA->x_rainbow_power_property)
#define X_RAINBOW_POWER_VOLTAGE_ITEM      (X_RAINBOW_POWER_PROPERTY->items + 0)
#define X_RAINBOW_POWER_RA_ITEM           (X_RAINBOW_POWER_PROPERTY->items + 1)
#define X_RAINBOW_POWER_DEC_ITEM          (X_RAINBOW_POWER_PROPERTY->items + 2)

#define X_RAINBOW_POWER_PROPERTY_NAME     "X_RAINBOW_POWER"
#define X_RAINBOW_POWER_VOLTAGE_ITEM_NAME "VOLTAGE"
#define X_RAINBOW_POWER_RA_ITEM_NAME      "RA_MOTOR"
#define X_RAINBOW_POWER_DEC_ITEM_NAME     "DEC_MOTOR"

#define X_RAINBOW_TEMPERATURE_PROPERTY        (PRIVATE_DATA->x_rainbow_temperature_property)
#define X_RAINBOW_TEMPERATURE_BOARD_ITEM      (X_RAINBOW_TEMPERATURE_PROPERTY->items + 0)
#define X_RAINBOW_TEMPERATURE_RA_ITEM         (X_RAINBOW_TEMPERATURE_PROPERTY->items + 1)
#define X_RAINBOW_TEMPERATURE_DEC_ITEM        (X_RAINBOW_TEMPERATURE_PROPERTY->items + 2)

#define X_RAINBOW_TEMPERATURE_PROPERTY_NAME   "X_RAINBOW_TEMPERATURE"
#define X_RAINBOW_TEMPERATURE_BOARD_ITEM_NAME "BOARD"
#define X_RAINBOW_TEMPERATURE_RA_ITEM_NAME    "RA_MOTOR"
#define X_RAINBOW_TEMPERATURE_DEC_ITEM_NAME   "DEC_MOTOR"

#define X_RAINBOW_STATUS_PROPERTY            (PRIVATE_DATA->x_rainbow_status_property)
#define X_RAINBOW_STATUS_TCS_ITEM            (X_RAINBOW_STATUS_PROPERTY->items + 0)
#define X_RAINBOW_STATUS_RA_MOTOR_ITEM       (X_RAINBOW_STATUS_PROPERTY->items + 1)
#define X_RAINBOW_STATUS_DEC_MOTOR_ITEM      (X_RAINBOW_STATUS_PROPERTY->items + 2)
#define X_RAINBOW_STATUS_HOME_ITEM           (X_RAINBOW_STATUS_PROPERTY->items + 3)

#define X_RAINBOW_STATUS_PROPERTY_NAME       "X_RAINBOW_STATUS"
#define X_RAINBOW_STATUS_TCS_ITEM_NAME       "TCS"
#define X_RAINBOW_STATUS_RA_MOTOR_ITEM_NAME  "RA_MOTOR"
#define X_RAINBOW_STATUS_DEC_MOTOR_ITEM_NAME "DEC_MOTOR"
#define X_RAINBOW_STATUS_HOME_ITEM_NAME      "HOME"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_rainbow_power_property;
	indigo_property *x_rainbow_temperature_property;
	indigo_property *x_rainbow_status_property;
	//+ data
	indigo_timer *reader;
	struct tm utc;
	unsigned long version;
	bool reader_running;
	bool goto_active;
	// the status poll reported the running GOTO or park moving (:CL1#), so its :CL0# ends it if :MM0# is lost
	bool goto_moving;
	// a park is a slew to the park position that ends with :MM0#, tracking is switched off then
	bool parking;
	// shared with the guider, which refuses pulses on a parked mount
	bool parked;
	double goto_deadline;
	double park_deadline;
	double home_deadline;
	pthread_mutex_t message_mutex;
	// the mount and the guider write from their own queues
	pthread_mutex_t write_mutex;
	char messages[RAINBOW_MESSAGE_COUNT][RAINBOW_MESSAGE_SIZE], processed[RAINBOW_MESSAGE_COUNT][RAINBOW_MESSAGE_SIZE];
	int message_count;
	bool messages_scheduled;
	double ra;
	bool tracking;
	int track_rate;
	char utc_time[INDIGO_VALUE_SIZE], utc_offset[INDIGO_VALUE_SIZE];
	// shared with the guider, a pulse is refused while the mount searches for home or moves manually,
	// and a GOTO, park, home or manual move is refused while a pulse runs
	bool homing;
	bool manual_motion;
	bool pulse_ra;
	bool pulse_dec;
	// the DEC axis angle of the aligned mount (:CG3#), the side of pier is read from :CY# against it
	double dec_axis_alignment;
	bool dec_axis_alignment_known;
	// queries answered only by some mounts, set by their first reply and polled only then
	bool side_of_pier_supported;
	bool power_supported;
	bool temperature_supported;
	bool status_supported;
	bool home_sensor_supported;
	//- data
} rainbow_private_data;

#pragma mark - Low level code

//+ code

static void mount_goto_finalizer(indigo_device *device);
static void mount_park_finalizer(indigo_device *device);
static void mount_home_finalizer(indigo_device *device);
static void rainbow_process_messages(indigo_device *device);

static bool rainbow_write(indigo_device *device, const char *command) {
	pthread_mutex_lock(&PRIVATE_DATA->write_mutex);
	bool result = PRIVATE_DATA->handle != NULL && indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0;
	pthread_mutex_unlock(&PRIVATE_DATA->write_mutex);
	return result;
}

static bool rainbow_response(indigo_device *device, char *response, int length) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	long result = indigo_uni_read_section2(PRIVATE_DATA->handle, response, length - 1, "#", "", INDIGO_DELAY(0.2), INDIGO_DELAY(0.2));
	if (result <= 0) {
		response[0] = 0;
		return false;
	}
	response[result] = 0;
	// Sr/Sd acknowledge with an unterminated '1'; it may prefix an asynchronous frame.
	char *frame = response;
	while (*frame == '1') {
		frame++;
	}
	if (frame != response && *frame == ':') {
		memmove(response, frame, strlen(frame) + 1);
	}
	return true;
}

static bool rainbow_sync_command(indigo_device *device, const char *command, indigo_property *property) {
	property->state = INDIGO_ALERT_STATE;
	if (rainbow_write(device, command)) {
		for (int i = 0; i < 100; i++) {
			indigo_usleep(10000);
			// runs on the device queue, so it takes the replies over from the reader itself
			rainbow_process_messages(device);
			if (property->state == INDIGO_OK_STATE || (property == MOUNT_EQUATORIAL_COORDINATES_PROPERTY && property->state == INDIGO_BUSY_STATE)) {
				if (IS_CONNECTED) {
					indigo_update_property(device, property, NULL);
				}
				return true;
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Failed to set %s", property->name);
	return false;
}

// true if the request switches the named item on, or any item for a NULL name
static bool rainbow_requested(indigo_property *property, const char *name) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value && (name == NULL || !strcmp(property->items[i].name, name))) {
			return true;
		}
	}
	return false;
}

static void rainbow_set_number(indigo_item *item, double value, bool *changed) {
	if (item->number.value != value) {
		item->number.value = value;
		*changed = true;
	}
}

static void rainbow_set_light(indigo_item *item, indigo_property_state state, bool *changed) {
	if (item->light.value != state) {
		item->light.value = state;
		*changed = true;
	}
}

static void rainbow_publish_status(indigo_device *device, indigo_property *property, bool changed) {
	if (changed || property->state != INDIGO_OK_STATE) {
		property->state = INDIGO_OK_STATE;
		if (IS_CONNECTED && !property->hidden) {
			indigo_update_property(device, property, NULL);
		}
	}
}

static bool rainbow_set_utc(indigo_device *device, time_t seconds, int utc_offset) {
	if (PRIVATE_DATA->version < 200625 || utc_offset < -12 || utc_offset > 12) {
		return false;
	}
	seconds += utc_offset * 3600;
	struct tm tm;
	indigo_gmtime(&seconds, &tm);
	char command[128];
	snprintf(command, sizeof(command), ":SC%02d/%02d/%02d#:SG%+03d#:SL%02d:%02d:%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100, -utc_offset, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return rainbow_write(device, command);
}

static bool rainbow_open(indigo_device *device) {
	const char *name = DEVICE_PORT_ITEM->text.value;
	// the WiFi module of the mount listens on port 7100
	bool network = indigo_uni_is_url(name, "rainbow");
	if (network) {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, 7100, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	// The mount answers on the interface it was told to use (:AU# USB, :AW# WiFi) and speaks the Rainbow
	// protocol only after :AR#; a mount left in the LX200 protocol (:AL#) does not answer :AV#.
	char response[128];
	bool result = rainbow_write(device, network ? ":AW#:AR#" : ":AU#:AR#");
	indigo_usleep(20000);
	// the probe is repeated after a missing reply, a wrong one is refused
	bool replied = false;
	for (int attempt = 0; result && !replied && attempt < 3; attempt++) {
		result = indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && rainbow_write(device, ":AV#");
		replied = result && rainbow_response(device, response, sizeof(response));
	}
	result = result && replied && !strncmp(response, ":AV", 3);
	if (result) {
		// a guider connected without the mount needs the version for its axis stops
		PRIVATE_DATA->version = atol(response + 3);
	} else {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return result;
}

static void rainbow_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

// reads replies on its own thread and hands them over to the device queue, where the properties are written
static void rainbow_reader(indigo_device *device) {
	indigo_rename_thread("Rainbow reader");
	char response[RAINBOW_MESSAGE_SIZE];
	while (PRIVATE_DATA->reader_running && PRIVATE_DATA->handle != NULL) {
		if (!rainbow_response(device, response, sizeof(response))) {
			continue;
		}
		pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
		bool schedule = false;
		if (PRIVATE_DATA->message_count < RAINBOW_MESSAGE_COUNT) {
			strcpy(PRIVATE_DATA->messages[PRIVATE_DATA->message_count++], response);
			schedule = !PRIVATE_DATA->messages_scheduled;
			PRIVATE_DATA->messages_scheduled = true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Message queue full, '%s' ignored", response);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
		if (schedule) {
			indigo_execute_handler(device, rainbow_process_messages);
		}
	}
}

static void rainbow_clear_messages(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	PRIVATE_DATA->message_count = 0;
	PRIVATE_DATA->messages_scheduled = false;
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

// :MM0#, or :CL0# after :CL1#, ends the running GOTO or park
static void rainbow_slew_completed(indigo_device *device) {
	PRIVATE_DATA->goto_active = PRIVATE_DATA->goto_moving = false;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	if (IS_CONNECTED) {
		indigo_update_coordinates(device, NULL);
	}
	if (PRIVATE_DATA->parking) {
		// the park position is reached, a parked mount does not track
		PRIVATE_DATA->parking = false;
		bool stopped = rainbow_write(device, ":CtL#");
		PRIVATE_DATA->parked = MOUNT_PARK_PARKED_ITEM->sw.value = stopped;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = !stopped;
		MOUNT_PARK_PROPERTY->state = stopped ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, stopped ? "Parked" : "Tracking could not be stopped");
		}
	}
}

static void rainbow_process_message(indigo_device *device, const char *response) {
	if (!strncmp(response, ":GR", 3)) {
		// the mount may report the right ascension beyond 24 h or below 0 h
		double ra = fmod(indigo_stod(response + 3), 24);
		PRIVATE_DATA->ra = ra < 0 ? ra + 24 : ra;
	} else if (!strncmp(response, ":GD", 3)) {
		double ra = PRIVATE_DATA->ra, dec = indigo_stod(response + 3);
		// a declination reported beyond a pole is mirrored back over it, the right ascension stays as reported
		if (dec > 90) {
			dec = 180 - dec;
		} else if (dec < -90) {
			dec = -180 - dec;
		}
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	} else if (!strcmp(response, ":CL0#") && !PRIVATE_DATA->goto_active) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_coordinates(device, NULL);
		}
	} else if (!strcmp(response, ":CL0#")) {
		// a :CL0# to a poll sent before the slew started comes before any :CL1# and is ignored
		if (PRIVATE_DATA->goto_moving) {
			rainbow_slew_completed(device);
		}
	} else if (!strcmp(response, ":MM0#")) {
		rainbow_slew_completed(device);
	} else if (PRIVATE_DATA->goto_active && (!strcmp(response, ":MML#") || !strcmp(response, ":MMU#") || !strcmp(response, ":MME#"))) {
		PRIVATE_DATA->goto_active = PRIVATE_DATA->goto_moving = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		if (IS_CONNECTED) {
			indigo_update_coordinates(device, response[3] == 'L' ? "Target below the altitude limit" : response[3] == 'U' ? "Target above the altitude limit" : "Slew canceled at the mount");
		}
		if (PRIVATE_DATA->parking) {
			PRIVATE_DATA->parking = false;
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park position not reached");
			}
		}
	} else if (!strcmp(response, ":CL1#")) {
		PRIVATE_DATA->goto_moving = PRIVATE_DATA->goto_active;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		if (IS_CONNECTED) {
			indigo_update_coordinates(device, NULL);
		}
	} else if (!strncmp(response, ":CH", 3)) {
		// :CHO# the mechanical origin was found, :CH0# the RA and :CH<# the DEC axis failed to find it
		bool found = !strcmp(response, ":CHO#");
		PRIVATE_DATA->homing = false;
		MOUNT_HOME_ITEM->sw.value = found;
		MOUNT_HOME_PROPERTY->state = found ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_HOME_PROPERTY, found ? "At home" : response[3] == '<' ? "DEC axis homing failed" : "RA axis homing failed");
		}
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_coordinates(device, NULL);
		}
	} else if (!strncmp(response, ":GC", 3)) {
		char separator;
		sscanf(response + 3, "%d%c%d%c%d", &PRIVATE_DATA->utc.tm_mon, &separator, &PRIVATE_DATA->utc.tm_mday, &separator, &PRIVATE_DATA->utc.tm_year);
		PRIVATE_DATA->utc.tm_year += 100;
		PRIVATE_DATA->utc.tm_mon--;
	} else if (!strncmp(response, ":GG", 3)) {
		int offset = -atoi(response + 3);
		#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		PRIVATE_DATA->utc.tm_gmtoff = offset * 3600;
		#endif
		snprintf(PRIVATE_DATA->utc_offset, INDIGO_VALUE_SIZE, "%d", offset);
		// a pending MOUNT_UTC_TIME request owns its items, its handler sets the requested time
		if (MOUNT_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
			INDIGO_COPY_VALUE(MOUNT_UTC_OFFSET_ITEM->text.value, PRIVATE_DATA->utc_offset);
		}
	} else if (!strncmp(response, ":GL", 3)) {
		char separator;
		if (PRIVATE_DATA->version < 200625) {
			time_t now = time(NULL);
			PRIVATE_DATA->utc = *localtime(&now);
		}
		sscanf(response + 3, "%d%c%d%c%d", &PRIVATE_DATA->utc.tm_hour, &separator, &PRIVATE_DATA->utc.tm_min, &separator, &PRIVATE_DATA->utc.tm_sec);
		PRIVATE_DATA->utc.tm_isdst = -1;
		time_t seconds = mktime(&PRIVATE_DATA->utc);
		indigo_timetoisogm(seconds, PRIVATE_DATA->utc_time, INDIGO_VALUE_SIZE);
		if (MOUNT_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
			INDIGO_COPY_VALUE(MOUNT_UTC_ITEM->text.value, PRIVATE_DATA->utc_time);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			}
		}
	} else if (!strncmp(response, ":Gt", 3)) {
		MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response + 3);
	} else if (!strncmp(response, ":Gg", 3)) {
		double longitude = indigo_stod(response + 3);
		if (longitude < 0) {
			longitude += 360;
		}
		// the prime meridian is 0, not 360
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude == 0 ? 0 : 360 - longitude;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
	} else if (!strncmp(response, ":AV", 3)) {
		INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "RainbowAstro");
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
		strncpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response + 3, 6);
		MOUNT_INFO_FIRMWARE_ITEM->text.value[6] = 0;
		PRIVATE_DATA->version = atol(response + 3);
		MOUNT_INFO_PROPERTY->state = INDIGO_OK_STATE;
	} else if (!strcmp(response, ":AT0#") || !strcmp(response, ":AT1#")) {
		PRIVATE_DATA->tracking = !strcmp(response, ":AT1#");
		// a pending MOUNT_TRACKING request is shown by its handler, an unchanged state is not republished
		if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE && (MOUNT_TRACKING_PROPERTY->state != INDIGO_OK_STATE || MOUNT_TRACKING_ON_ITEM->sw.value != PRIVATE_DATA->tracking)) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, PRIVATE_DATA->tracking ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}
		}
	} else if (!strncmp(response, ":CT", 3) && strchr(response, '|') != NULL) {
		// :CT<board>|<RA motor>|<DEC motor># are the temperatures, not a tracking rate
		double board, ra, dec;
		if (sscanf(response + 3, "%lf|%lf|%lf", &board, &ra, &dec) == 3) {
			bool changed = false;
			PRIVATE_DATA->temperature_supported = true;
			rainbow_set_number(X_RAINBOW_TEMPERATURE_BOARD_ITEM, board, &changed);
			rainbow_set_number(X_RAINBOW_TEMPERATURE_RA_ITEM, ra, &changed);
			rainbow_set_number(X_RAINBOW_TEMPERATURE_DEC_ITEM, dec, &changed);
			rainbow_publish_status(device, X_RAINBOW_TEMPERATURE_PROPERTY, changed);
		}
	} else if (!strncmp(response, ":CT", 3) && response[3] >= '0' && response[3] <= '2' && response[4] == '#') {
		// :CT3# is the guide speed used as the tracking rate, which has no item
		PRIVATE_DATA->track_rate = response[3] - '0';
		// a pending MOUNT_TRACK_RATE request is shown by its handler, an unchanged rate is not republished
		if (MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_BUSY_STATE && (MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_TRACK_RATE_PROPERTY->items[PRIVATE_DATA->track_rate].sw.value)) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_PROPERTY->items + PRIVATE_DATA->track_rate, true);
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
		}
	} else if (!strncmp(response, ":Cv", 3)) {
		bool changed = false;
		PRIVATE_DATA->power_supported = true;
		rainbow_set_number(X_RAINBOW_POWER_VOLTAGE_ITEM, atof(response + 3), &changed);
		rainbow_publish_status(device, X_RAINBOW_POWER_PROPERTY, changed);
	} else if (!strncmp(response, ":CP", 3)) {
		// :CP<DEC motor>|<RA motor># is the motor power in percent
		double dec, ra;
		if (sscanf(response + 3, "%lf|%lf", &dec, &ra) == 2) {
			bool changed = false;
			PRIVATE_DATA->power_supported = true;
			rainbow_set_number(X_RAINBOW_POWER_RA_ITEM, ra, &changed);
			rainbow_set_number(X_RAINBOW_POWER_DEC_ITEM, dec, &changed);
			rainbow_publish_status(device, X_RAINBOW_POWER_PROPERTY, changed);
		}
	} else if (!strncmp(response, ":GY", 3) && strlen(response) >= 8) {
		// :GY<TCS>?<DEC motor><RA motor>#, 'O' is fine, anything else needs a check
		bool changed = false;
		PRIVATE_DATA->status_supported = true;
		rainbow_set_light(X_RAINBOW_STATUS_TCS_ITEM, response[3] == 'O' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, &changed);
		rainbow_set_light(X_RAINBOW_STATUS_DEC_MOTOR_ITEM, response[5] == 'O' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, &changed);
		rainbow_set_light(X_RAINBOW_STATUS_RA_MOTOR_ITEM, response[6] == 'O' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE, &changed);
		rainbow_publish_status(device, X_RAINBOW_STATUS_PROPERTY, changed);
	} else if (!strncmp(response, ":GH", 3) && strlen(response) >= 5) {
		// :GHO# the home sensor was found
		bool changed = false;
		PRIVATE_DATA->home_sensor_supported = true;
		rainbow_set_light(X_RAINBOW_STATUS_HOME_ITEM, response[3] == 'O' ? INDIGO_OK_STATE : INDIGO_IDLE_STATE, &changed);
		if (PRIVATE_DATA->status_supported) {
			rainbow_publish_status(device, X_RAINBOW_STATUS_PROPERTY, changed);
		}
	} else if (!strncmp(response, ":CG3", 4)) {
		PRIVATE_DATA->dec_axis_alignment = atof(response[4] == '=' ? response + 5 : response + 4);
		PRIVATE_DATA->dec_axis_alignment_known = true;
	} else if (!strncmp(response, ":CY", 3) && PRIVATE_DATA->dec_axis_alignment_known && strlen(response) >= 11) {
		// :CY<DEC axis angle, 7 characters><separator><RA axis angle, 7 characters>#, the OTA is west of the pier
		// when the DEC axis is turned more than 90 degrees from its aligned position
		char angle[8];
		memcpy(angle, response + 3, 7);
		angle[7] = 0;
		indigo_item *side = atof(angle) - PRIVATE_DATA->dec_axis_alignment > 90 ? MOUNT_SIDE_OF_PIER_WEST_ITEM : MOUNT_SIDE_OF_PIER_EAST_ITEM;
		PRIVATE_DATA->side_of_pier_supported = true;
		if (!side->sw.value || MOUNT_SIDE_OF_PIER_PROPERTY->state != INDIGO_OK_STATE) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, side, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED && !MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		}
	} else if (!strncmp(response, ":CU0=", 5)) {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = round(100 * atof(response + 5));
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		}
	}
}

// runs on the device queue, so the replies are applied in turn with the change handlers
static void rainbow_process_messages(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	int count = PRIVATE_DATA->message_count;
	memcpy(PRIVATE_DATA->processed, PRIVATE_DATA->messages, sizeof(PRIVATE_DATA->messages[0]) * count);
	PRIVATE_DATA->message_count = 0;
	PRIVATE_DATA->messages_scheduled = false;
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
	for (int i = 0; i < count; i++) {
		rainbow_process_message(device, PRIVATE_DATA->processed[i]);
	}
}

static void mount_goto_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (indigo_monotonic_time() >= PRIVATE_DATA->goto_deadline) {
		PRIVATE_DATA->goto_active = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, "Slew timed out");
		return;
	}
	indigo_execute_handler_in(device, 0.2, mount_goto_finalizer);
}

static void mount_park_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (indigo_monotonic_time() >= PRIVATE_DATA->park_deadline) {
		PRIVATE_DATA->parking = PRIVATE_DATA->goto_active = false;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park timed out");
		return;
	}
	indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
}

static void mount_home_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (indigo_monotonic_time() >= PRIVATE_DATA->home_deadline) {
		PRIVATE_DATA->homing = false;
		MOUNT_HOME_ITEM->sw.value = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, "Homing timed out");
		return;
	}
	indigo_execute_handler_in(device, 0.2, mount_home_finalizer);
}

// Converts the park position (hour angle and declination) to the altitude and azimuth (north 0, east 90) that
// :Sa# and :Sz# take.
static void rainbow_park_horizontal(indigo_device *device, double *alt, double *az) {
	double latitude = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value * M_PI / 180;
	double ha = MOUNT_PARK_POSITION_HA_ITEM->number.value * M_PI / 12;
	double dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value * M_PI / 180;
	*alt = asin(sin(latitude) * sin(dec) + cos(latitude) * cos(dec) * cos(ha)) * 180 / M_PI;
	*az = atan2(-cos(dec) * sin(ha), sin(dec) * cos(latitude) - cos(dec) * cos(ha) * sin(latitude)) * 180 / M_PI;
	if (*az < 0) {
		*az += 360;
	}
	// due north computes as a hair below 360, which would be written as 360*00:00.0
	if (*az >= 360 - 0.05 / 3600) {
		*az = 0;
	}
}

// A pulse ends with the stop of its own axis; firmware older than 200625 has only the global :Q#.
static bool rainbow_stop_pulse(indigo_device *device, const char *axis_stop) {
	return rainbow_write(device, PRIVATE_DATA->version >= 200625 ? axis_stop : ":Q#");
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	bool ok = rainbow_stop_pulse(device, GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? ":Qs#" : ":Qn#");
	PRIVATE_DATA->pulse_dec = false;
	// only the values, the target of a pulse requested while this one ends is read by its handler
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	bool ok = rainbow_stop_pulse(device, GUIDER_GUIDE_WEST_ITEM->number.value > 0 ? ":Qw#" : ":Qe#");
	PRIVATE_DATA->pulse_ra = false;
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	rainbow_write(device, ":GR#:GD#:CL#");
	rainbow_write(device, PRIVATE_DATA->version >= 200625 ? ":GC#:GG#:GL#" : ":GL#");
	rainbow_write(device, ":AT#");
	rainbow_write(device, ":Ct?#");
	if (PRIVATE_DATA->side_of_pier_supported) {
		rainbow_write(device, ":CY#");
	}
	if (PRIVATE_DATA->power_supported) {
		rainbow_write(device, ":Cv#:CP#");
	}
	if (PRIVATE_DATA->temperature_supported) {
		rainbow_write(device, ":CT#");
	}
	if (PRIVATE_DATA->status_supported) {
		rainbow_write(device, ":GY#");
	}
	if (PRIVATE_DATA->home_sensor_supported) {
		rainbow_write(device, ":GH#");
	}
	indigo_execute_handler_in(device, 1, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = rainbow_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			PRIVATE_DATA->goto_active = PRIVATE_DATA->goto_moving = PRIVATE_DATA->parking = false;
			PRIVATE_DATA->homing = PRIVATE_DATA->manual_motion = false;
			PRIVATE_DATA->dec_axis_alignment_known = false;
			PRIVATE_DATA->side_of_pier_supported = PRIVATE_DATA->power_supported = PRIVATE_DATA->temperature_supported = PRIVATE_DATA->status_supported = PRIVATE_DATA->home_sensor_supported = false;
			INDIGO_COPY_VALUE(PRIVATE_DATA->utc_time, MOUNT_UTC_ITEM->text.value);
			INDIGO_COPY_VALUE(PRIVATE_DATA->utc_offset, MOUNT_UTC_OFFSET_ITEM->text.value);
			rainbow_clear_messages(device);
			PRIVATE_DATA->reader_running = true;
			indigo_set_timer(device, 0, rainbow_reader, &PRIVATE_DATA->reader);
			// only the identity is required, a lost reply to another query leaves just its property in ALERT
			connection_result = rainbow_sync_command(device, ":AV#", MOUNT_INFO_PROPERTY);
			if (connection_result) {
				rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY);
				rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY);
				rainbow_sync_command(device, ":CU0#", MOUNT_GUIDE_RATE_PROPERTY);
				rainbow_sync_command(device, ":Gt#:Gg#", MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
				rainbow_sync_command(device, ":GR#:GD#:CL#", MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
				rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY);
				rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY);
				rainbow_sync_command(device, PRIVATE_DATA->version >= 200625 ? ":GC#:GG#:GL#" : ":GL#", MOUNT_UTC_TIME_PROPERTY);
				// side of pier, power, temperatures and motor status are answered only by some mounts, the
				// properties of the unanswered queries stay hidden
				if (rainbow_write(device, ":CG3#:CY#:Cv#:CP#:CT#:GY#:GH#")) {
					for (int i = 0; i < 100 && !(PRIVATE_DATA->side_of_pier_supported && PRIVATE_DATA->power_supported && PRIVATE_DATA->temperature_supported && PRIVATE_DATA->status_supported && PRIVATE_DATA->home_sensor_supported); i++) {
						indigo_usleep(10000);
						rainbow_process_messages(device);
					}
				}
			}
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = !PRIVATE_DATA->side_of_pier_supported;
			X_RAINBOW_POWER_PROPERTY->hidden = !PRIVATE_DATA->power_supported;
			X_RAINBOW_TEMPERATURE_PROPERTY->hidden = !PRIVATE_DATA->temperature_supported;
			X_RAINBOW_STATUS_PROPERTY->hidden = !PRIVATE_DATA->status_supported;
			X_RAINBOW_STATUS_PROPERTY->count = PRIVATE_DATA->home_sensor_supported ? 4 : 3;
			// the mount does not report a park, it is known only from a park made in this session
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			PRIVATE_DATA->parked = false;
			MOUNT_HOME_ITEM->sw.value = false;
			if (!connection_result) {
				PRIVATE_DATA->reader_running = false;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
				rainbow_clear_messages(device);
				rainbow_close(device);
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_RAINBOW_POWER_PROPERTY, NULL);
			indigo_define_property(device, X_RAINBOW_TEMPERATURE_PROPERTY, NULL);
			indigo_define_property(device, X_RAINBOW_STATUS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				rainbow_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		// a running GOTO, park, homing or manual motion is stopped before the port closes
		bool manual = MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value;
		if (PRIVATE_DATA->goto_active || manual || MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE || MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			rainbow_write(device, ":Q#");
		}
		MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
		PRIVATE_DATA->goto_active = PRIVATE_DATA->parking = false;
		PRIVATE_DATA->homing = PRIVATE_DATA->manual_motion = false;
		PRIVATE_DATA->reader_running = false;
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
		rainbow_clear_messages(device);
		//- mount.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			MOUNT_PARK_PROPERTY,
			MOUNT_PARK_POSITION_PROPERTY,
			MOUNT_PARK_SET_PROPERTY,
			MOUNT_HOME_PROPERTY,
			MOUNT_EPOCH_PROPERTY,
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY,
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY,
			MOUNT_ABORT_MOTION_PROPERTY,
			MOUNT_MOTION_DEC_PROPERTY,
			MOUNT_MOTION_RA_PROPERTY,
			MOUNT_SET_HOST_TIME_PROPERTY,
			MOUNT_UTC_TIME_PROPERTY,
			MOUNT_TRACKING_PROPERTY,
			MOUNT_TRACK_RATE_PROPERTY,
			MOUNT_SIDE_OF_PIER_PROPERTY,
			X_RAINBOW_POWER_PROPERTY,
			X_RAINBOW_TEMPERATURE_PROPERTY,
			X_RAINBOW_STATUS_PROPERTY,
			MOUNT_GUIDE_RATE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_RAINBOW_POWER_PROPERTY, NULL);
		indigo_delete_property(device, X_RAINBOW_TEMPERATURE_PROPERTY, NULL);
		indigo_delete_property(device, X_RAINBOW_STATUS_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			rainbow_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, mount_timer_callback);
	}
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		// The park position is reached by a slew to its altitude and azimuth, the mount stops tracking there.
		double alt, az;
		char alt_string[32], az_string[32], command[128];
		rainbow_park_horizontal(device, &alt, &az);
		snprintf(command, sizeof(command), ":Sa%s#:Sz%s#:MA#", indigo_dtos_r(alt, "%+03d*%02d:%04.1f", alt_string, sizeof(alt_string)), indigo_dtos_r(az, "%03d*%02d:%04.1f", az_string, sizeof(az_string)));
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		MOUNT_HOME_ITEM->sw.value = false;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		PRIVATE_DATA->goto_active = PRIVATE_DATA->parking = true;
		PRIVATE_DATA->goto_moving = false;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		if (rainbow_write(device, command)) {
			PRIVATE_DATA->park_deadline = indigo_monotonic_time() + 600;
			indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
		} else {
			PRIVATE_DATA->goto_active = PRIVATE_DATA->parking = false;
			MOUNT_PARK_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, NULL);
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
	} else {
		// :CtA# unparks and starts tracking
		if (rainbow_write(device, ":CtA#")) {
			PRIVATE_DATA->parked = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		// the generated handler does not publish MOUNT_PARK
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	}
	//- mount.MOUNT_PARK.on_change
}

static void mount_home_handler(indigo_device *device) {
	//+ mount.MOUNT_HOME.on_change
	if (MOUNT_HOME_ITEM->sw.value) {
		// :Ch# finds the mechanical origin and reports the result with :CH<result>#
		MOUNT_HOME_ITEM->sw.value = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		if (rainbow_write(device, ":Ch#")) {
			PRIVATE_DATA->homing = true;
			PRIVATE_DATA->home_deadline = indigo_monotonic_time() + 600;
			indigo_execute_handler_in(device, 0.2, mount_home_finalizer);
		} else {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		}
	}
	//- mount.MOUNT_HOME.on_change
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	char latitude[32], longitude[32], command[128];
	if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0) {
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
	}
	double longitude_value = -MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	if (longitude_value < -180) {
		longitude_value += 360;
	}
	snprintf(command, sizeof(command), ":St%s#:Sg%s#", indigo_dtos_r(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, "%+03d*%02d'%02d", latitude, sizeof(latitude)), indigo_dtos_r(longitude_value, "%+04d*%02d'%02d", longitude, sizeof(longitude)));
	if (!rainbow_write(device, command)) {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	char ra_string[32], dec_string[32], command[128];
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	// a value just below 24 h would be rounded up to 24 h, or 360 degrees
	if (ra >= 24 - 0.05 / 3600) {
		ra = 0;
	}
	if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		snprintf(command, sizeof(command), ":Ck%07.3f%+07.3f#", ra * 15 >= 360 - 0.0005 ? 0 : ra * 15, dec);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = rainbow_write(device, command) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
	} else {
		const char *rate_command = MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value ? ":CtS#" : MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value ? ":CtM#" : ":CtR#";
		bool ok = rainbow_write(device, rate_command);
		snprintf(command, sizeof(command), ":CtA#:Sr%s#:Sd%s#:MS#", indigo_dtos_r(ra, "%02d:%02d:%04.1f", ra_string, sizeof(ra_string)), indigo_dtos_r(dec, "%+03d*%02d:%04.1f", dec_string, sizeof(dec_string)));
		PRIVATE_DATA->goto_active = true;
		PRIVATE_DATA->goto_moving = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		}
		ok = rainbow_write(device, command) && ok;
		if (!ok) {
			PRIVATE_DATA->goto_active = false;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_coordinates(device, NULL);
		if (ok) {
			PRIVATE_DATA->goto_deadline = indigo_monotonic_time() + 600;
			indigo_execute_handler_in(device, 0.2, mount_goto_finalizer);
		}
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, mount_goto_finalizer);
	indigo_cancel_pending_handler(device, mount_park_finalizer);
	indigo_cancel_pending_handler(device, mount_home_finalizer);
	bool ok = rainbow_write(device, ":Q#");
	// an aborted GOTO or park did not reach its target
	bool slewing = PRIVATE_DATA->goto_active;
	if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park aborted");
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_HOME_ITEM->sw.value = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, "Homing aborted");
	}
	PRIVATE_DATA->goto_active = PRIVATE_DATA->parking = false;
	PRIVATE_DATA->homing = PRIVATE_DATA->manual_motion = false;
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = MOUNT_MOTION_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok && !slewing ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_coordinates(device, ok ? "Aborted" : "Abort failed");
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_mount_commit_motion_client(device, MOUNT_ABORT_MOTION_PROPERTY);
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
	const char *rate = MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value ? ":RG#" : MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value ? ":RC#" : MOUNT_SLEW_RATE_FIND_ITEM->sw.value ? ":RM#" : ":RS#";
	char command[32];
	if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		// :Ms# moves the declination up (north), :Mn# down (south)
		snprintf(command, sizeof(command), "%s:%s#", rate, MOUNT_MOTION_NORTH_ITEM->sw.value ? "Ms" : "Mn");
	} else {
		snprintf(command, sizeof(command), "%s", PRIVATE_DATA->version >= 200625 ? ":Qs#:Qn#" : ":Q#");
	}
	if (!rainbow_write(device, command)) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->manual_motion = MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value;
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
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
	const char *rate = MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value ? ":RG#" : MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value ? ":RC#" : MOUNT_SLEW_RATE_FIND_ITEM->sw.value ? ":RM#" : ":RS#";
	char command[32];
	if (MOUNT_MOTION_WEST_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value) {
		snprintf(command, sizeof(command), "%s:%s#", rate, MOUNT_MOTION_WEST_ITEM->sw.value ? "Mw" : "Me");
	} else {
		snprintf(command, sizeof(command), "%s", PRIVATE_DATA->version >= 200625 ? ":Qw#:Qe#" : ":Q#");
	}
	if (!rainbow_write(device, command)) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->manual_motion = MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value;
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
}

static void mount_set_host_time_handler(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SET_HOST_TIME.on_change
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		time_t seconds = time(NULL);
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		int utc_offset = indigo_get_utc_offset();
		MOUNT_SET_HOST_TIME_PROPERTY->state = rainbow_set_utc(device, seconds, utc_offset) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (MOUNT_SET_HOST_TIME_PROPERTY->state == INDIGO_OK_STATE) {
			indigo_timetoisogm(seconds, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", utc_offset);
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		}
	}
	//- mount.MOUNT_SET_HOST_TIME.on_change
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_handler(indigo_device *device) {
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_UTC_TIME.on_change
	int utc_offset = 0;
	time_t seconds = indigo_mount_get_utc_target(device, &utc_offset);
	if (seconds != (time_t)-1 && rainbow_set_utc(device, seconds, utc_offset)) {
		indigo_timetoisogm(seconds, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", utc_offset);
	} else {
		INDIGO_COPY_VALUE(MOUNT_UTC_ITEM->text.value, PRIVATE_DATA->utc_time);
		INDIGO_COPY_VALUE(MOUNT_UTC_OFFSET_ITEM->text.value, PRIVATE_DATA->utc_offset);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_UTC_TIME.on_change
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	if (rainbow_write(device, indigo_get_switch_target(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM_NAME) ? ":CtA#" : ":CtL#")) {
		indigo_apply_switch_targets(MOUNT_TRACKING_PROPERTY);
	} else {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, PRIVATE_DATA->tracking ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	const char *command = indigo_get_switch_target(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME) ? ":CtS#" : indigo_get_switch_target(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME) ? ":CtM#" : ":CtR#";
	if (rainbow_write(device, command)) {
		indigo_apply_switch_targets(MOUNT_TRACK_RATE_PROPERTY);
	} else {
		indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_PROPERTY->items + PRIVATE_DATA->track_rate, true);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	char command[32];
	snprintf(command, sizeof(command), ":Cu0=%3.1f#", MOUNT_GUIDE_RATE_RA_ITEM->number.value / 100.0);
	if (!rainbow_write(device, command)) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ mount.on_attach
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_GUIDE_RATE_PROPERTY->count = 1;
		// the guide speed is 0.1x to 1.0x sidereal
		MOUNT_GUIDE_RATE_RA_ITEM->number.min = 10;
		MOUNT_GUIDE_RATE_RA_ITEM->number.max = 100;
		// sidereal, solar and lunar (:CtR#, :CtS#, :CtM#)
		MOUNT_TRACK_RATE_PROPERTY->count = 3;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->message_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->write_mutex, NULL);
		//- mount.on_attach
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_EPOCH_PROPERTY->hidden = false;
		//+ mount.MOUNT_EPOCH.on_attach
		MOUNT_EPOCH_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_EPOCH_ITEM->number.value = MOUNT_EPOCH_ITEM->number.target = 2000.0;
		//- mount.MOUNT_EPOCH.on_attach
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
		//+ mount.MOUNT_SIDE_OF_PIER.on_attach
		// read from the axis angles, the forced meridian flip is not supported
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
		//- mount.MOUNT_SIDE_OF_PIER.on_attach
		X_RAINBOW_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, X_RAINBOW_POWER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Power", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_RAINBOW_POWER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RAINBOW_POWER_VOLTAGE_ITEM, X_RAINBOW_POWER_VOLTAGE_ITEM_NAME, "Input voltage [V]", 0, 50, 0, 0);
		strcpy(X_RAINBOW_POWER_VOLTAGE_ITEM->number.format, "%.1f");
		indigo_init_number_item(X_RAINBOW_POWER_RA_ITEM, X_RAINBOW_POWER_RA_ITEM_NAME, "RA motor power [%]", 0, 100, 0, 0);
		strcpy(X_RAINBOW_POWER_RA_ITEM->number.format, "%.0f");
		indigo_init_number_item(X_RAINBOW_POWER_DEC_ITEM, X_RAINBOW_POWER_DEC_ITEM_NAME, "DEC motor power [%]", 0, 100, 0, 0);
		strcpy(X_RAINBOW_POWER_DEC_ITEM->number.format, "%.0f");
		X_RAINBOW_POWER_PROPERTY->hidden = true;
		X_RAINBOW_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, X_RAINBOW_TEMPERATURE_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Temperature", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_RAINBOW_TEMPERATURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RAINBOW_TEMPERATURE_BOARD_ITEM, X_RAINBOW_TEMPERATURE_BOARD_ITEM_NAME, "Main board [\u00B0C]", -50, 150, 0, 0);
		strcpy(X_RAINBOW_TEMPERATURE_BOARD_ITEM->number.format, "%.1f");
		indigo_init_number_item(X_RAINBOW_TEMPERATURE_RA_ITEM, X_RAINBOW_TEMPERATURE_RA_ITEM_NAME, "RA motor [\u00B0C]", -50, 150, 0, 0);
		strcpy(X_RAINBOW_TEMPERATURE_RA_ITEM->number.format, "%.1f");
		indigo_init_number_item(X_RAINBOW_TEMPERATURE_DEC_ITEM, X_RAINBOW_TEMPERATURE_DEC_ITEM_NAME, "DEC motor [\u00B0C]", -50, 150, 0, 0);
		strcpy(X_RAINBOW_TEMPERATURE_DEC_ITEM->number.format, "%.1f");
		X_RAINBOW_TEMPERATURE_PROPERTY->hidden = true;
		X_RAINBOW_STATUS_PROPERTY = indigo_init_light_property(NULL, device->name, X_RAINBOW_STATUS_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Mount status", INDIGO_OK_STATE, 4);
		if (X_RAINBOW_STATUS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_RAINBOW_STATUS_TCS_ITEM, X_RAINBOW_STATUS_TCS_ITEM_NAME, "Telescope control system", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_RAINBOW_STATUS_RA_MOTOR_ITEM, X_RAINBOW_STATUS_RA_MOTOR_ITEM_NAME, "RA motor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_RAINBOW_STATUS_DEC_MOTOR_ITEM, X_RAINBOW_STATUS_DEC_MOTOR_ITEM_NAME, "DEC motor", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_RAINBOW_STATUS_HOME_ITEM, X_RAINBOW_STATUS_HOME_ITEM_NAME, "Home found", INDIGO_IDLE_STATE);
		X_RAINBOW_STATUS_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAINBOW_POWER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAINBOW_TEMPERATURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RAINBOW_STATUS_PROPERTY);
	}
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->homing, MOUNT_PARK_PROPERTY, "Mount is searching for home");
		INDIGO_REJECT_CHANGE_IF((PRIVATE_DATA->pulse_ra || PRIVATE_DATA->pulse_dec) && rainbow_requested(property, MOUNT_PARK_PARKED_ITEM_NAME), MOUNT_PARK_PROPERTY, "Guiding pulse in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->goto_active, MOUNT_HOME_PROPERTY, "Mount is slewing");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->manual_motion, MOUNT_HOME_PROPERTY, "Mount is moving");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->pulse_ra || PRIVATE_DATA->pulse_dec, MOUNT_HOME_PROPERTY, "Guiding pulse in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->homing, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is searching for home");
		INDIGO_REJECT_CHANGE_IF((PRIVATE_DATA->pulse_ra || PRIVATE_DATA->pulse_dec) && !MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Guiding pulse in progress");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->homing && rainbow_requested(property, NULL), MOUNT_MOTION_DEC_PROPERTY, "Mount is searching for home");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->goto_active && rainbow_requested(property, NULL), MOUNT_MOTION_DEC_PROPERTY, "Mount is slewing");
		INDIGO_REJECT_CHANGE_IF((PRIVATE_DATA->pulse_ra || PRIVATE_DATA->pulse_dec) && rainbow_requested(property, NULL), MOUNT_MOTION_DEC_PROPERTY, "Guiding pulse in progress");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->homing && rainbow_requested(property, NULL), MOUNT_MOTION_RA_PROPERTY, "Mount is searching for home");
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->goto_active && rainbow_requested(property, NULL), MOUNT_MOTION_RA_PROPERTY, "Mount is slewing");
		INDIGO_REJECT_CHANGE_IF((PRIVATE_DATA->pulse_ra || PRIVATE_DATA->pulse_dec) && rainbow_requested(property, NULL), MOUNT_MOTION_RA_PROPERTY, "Guiding pulse in progress");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		//+ mount.MOUNT_UTC_TIME.on_change_request
		// replies to the status poll refresh the items from the mount clock, so the handler sends the
		// requested time recorded here instead of the copied items
		indigo_mount_set_utc_target(device, property);
		//- mount.MOUNT_UTC_TIME.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	//+ mount.on_detach
	pthread_mutex_destroy(&PRIVATE_DATA->message_mutex);
	pthread_mutex_destroy(&PRIVATE_DATA->write_mutex);
	//- mount.on_detach
	indigo_release_property(X_RAINBOW_POWER_PROPERTY);
	indigo_release_property(X_RAINBOW_TEMPERATURE_PROPERTY);
	indigo_release_property(X_RAINBOW_STATUS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = rainbow_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				rainbow_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		if (GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_BUSY_STATE) {
			rainbow_stop_pulse(device, GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? ":Qs#" : ":Qn#");
		}
		if (GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
			rainbow_stop_pulse(device, GUIDER_GUIDE_WEST_ITEM->number.value > 0 ? ":Qw#" : ":Qe#");
		}
		PRIVATE_DATA->pulse_ra = PRIVATE_DATA->pulse_dec = false;
		//- guider.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			GUIDER_GUIDE_DEC_PROPERTY,
			GUIDER_GUIDE_RA_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		if (--PRIVATE_DATA->count == 0) {
			rainbow_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	// the previous pulse's finalizer may have zeroed the values after the request was copied, the targets keep it
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target;
	double north = GUIDER_GUIDE_NORTH_ITEM->number.value, south = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	PRIVATE_DATA->pulse_dec = false;
	if (PRIVATE_DATA->goto_active || PRIVATE_DATA->parked || PRIVATE_DATA->homing || PRIVATE_DATA->manual_motion) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, PRIVATE_DATA->parked ? "No guiding while parked" : PRIVATE_DATA->homing ? "No guiding while searching for home" : PRIVATE_DATA->manual_motion ? "No guiding during a manual move" : "No guiding during a slew");
	} else if (north > 0 || south > 0) {
		if (rainbow_write(device, north > 0 ? ":RG#:Ms#" : ":RG#:Mn#")) {
			PRIVATE_DATA->pulse_dec = true;
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, (north > 0 ? north : south) / 1000.0, guider_guide_dec_finalizer);
		} else {
			GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target;
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target;
	double west = GUIDER_GUIDE_WEST_ITEM->number.value, east = GUIDER_GUIDE_EAST_ITEM->number.value;
	PRIVATE_DATA->pulse_ra = false;
	if (PRIVATE_DATA->goto_active || PRIVATE_DATA->parked || PRIVATE_DATA->homing || PRIVATE_DATA->manual_motion) {
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, PRIVATE_DATA->parked ? "No guiding while parked" : PRIVATE_DATA->homing ? "No guiding while searching for home" : PRIVATE_DATA->manual_motion ? "No guiding during a manual move" : "No guiding during a slew");
	} else if (west > 0 || east > 0) {
		if (rainbow_write(device, west > 0 ? ":RG#:Mw#" : ":RG#:Me#")) {
			PRIVATE_DATA->pulse_ra = true;
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, (west > 0 ? west : east) / 1000.0, guider_guide_ra_finalizer);
		} else {
			GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
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
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
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

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_rainbow(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static rainbow_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (rainbow_private_data *)indigo_safe_malloc(sizeof(rainbow_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
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
