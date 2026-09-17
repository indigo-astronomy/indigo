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

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_rainbow.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000010
#define DRIVER_NAME          "indigo_mount_rainbow"
#define DRIVER_LABEL         "RainbowAstro Mount"
#define MOUNT_DEVICE_NAME    "RainbowAstro Mount"
#define PRIVATE_DATA         ((rainbow_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	indigo_timer *reader;
	struct tm utc;
	unsigned long version;
	bool reader_running;
	bool goto_active;
	double goto_deadline;
	double park_deadline;
	//- data
} rainbow_private_data;

#pragma mark - Low level code

//+ code

static void mount_goto_finalizer(indigo_device *device);
static void mount_park_finalizer(indigo_device *device);

static bool rainbow_write(indigo_device *device, const char *command) {
	return PRIVATE_DATA->handle != NULL && indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0;
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
	return true;
}

static bool rainbow_sync_command(indigo_device *device, const char *command, indigo_property *property) {
	property->state = INDIGO_ALERT_STATE;
	if (rainbow_write(device, command)) {
		for (int i = 0; i < 100; i++) {
			indigo_usleep(10000);
			if (property->state == INDIGO_OK_STATE) {
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
	if (indigo_uni_is_url(name, "rainbow")) {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, 4030, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	char response[128];
	bool result = indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && rainbow_write(device, ":AV#") && rainbow_response(device, response, sizeof(response)) && !strncmp(response, ":AV", 3);
	if (!result) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return result;
}

static void rainbow_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void rainbow_reader(indigo_device *device) {
	indigo_rename_thread("Rainbow reader");
	char response[128];
	double ra = 0, dec = 0;
	while (PRIVATE_DATA->reader_running && PRIVATE_DATA->handle != NULL) {
		if (!rainbow_response(device, response, sizeof(response))) {
			continue;
		}
		if (!strncmp(response, ":GR", 3)) {
			ra = indigo_stod(response + 3);
		} else if (!strncmp(response, ":GD", 3)) {
			dec = indigo_stod(response + 3);
			indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		} else if (!strcmp(response, ":CL0#") && !PRIVATE_DATA->goto_active) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
		} else if (!strcmp(response, ":MM0#")) {
			PRIVATE_DATA->goto_active = false;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
		} else if (!strcmp(response, ":CL1#")) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, NULL);
		} else if (!strcmp(response, ":CHO#")) {
			MOUNT_PARK_PARKED_ITEM->sw.value = true;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
		} else if (!strncmp(response, ":CH", 3)) {
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
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
			snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", offset);
		} else if (!strncmp(response, ":GL", 3)) {
			char separator;
			if (PRIVATE_DATA->version < 200625) {
				time_t now = time(NULL);
				PRIVATE_DATA->utc = *localtime(&now);
			}
			sscanf(response + 3, "%d%c%d%c%d", &PRIVATE_DATA->utc.tm_hour, &separator, &PRIVATE_DATA->utc.tm_min, &separator, &PRIVATE_DATA->utc.tm_sec);
			PRIVATE_DATA->utc.tm_isdst = -1;
			time_t seconds = mktime(&PRIVATE_DATA->utc);
			indigo_timetoisogm(seconds, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		} else if (!strncmp(response, ":Gt", 3)) {
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response + 3);
		} else if (!strncmp(response, ":Gg", 3)) {
			double longitude = indigo_stod(response + 3);
			if (longitude < 0) {
				longitude += 360;
			}
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 360 - longitude;
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
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, !strcmp(response, ":AT1#") ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}
		} else if (!strncmp(response, ":CT", 3) && response[3] >= '0' && response[3] <= '2') {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_PROPERTY->items + response[3] - '0', true);
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
		} else if (!strncmp(response, ":CU0=", 5)) {
			MOUNT_GUIDE_RATE_RA_ITEM->number.value = round(100 * atof(response + 5));
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			}
		}
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
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park timed out");
		return;
	}
	indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
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
	indigo_execute_handler_in(device, 1, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = rainbow_open(device);
		if (connection_result) {
			//+ mount.on_connect
			PRIVATE_DATA->goto_active = false;
			PRIVATE_DATA->reader_running = true;
			indigo_set_timer(device, 0, rainbow_reader, &PRIVATE_DATA->reader);
			connection_result = rainbow_sync_command(device, ":AV#", MOUNT_INFO_PROPERTY);
			connection_result = rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":CU0#", MOUNT_GUIDE_RATE_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":Gt#:Gg#", MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":GR#:GD#:CL#", MOUNT_EQUATORIAL_COORDINATES_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY) && connection_result;
			connection_result = rainbow_sync_command(device, PRIVATE_DATA->version >= 200625 ? ":GC#:GG#:GL#" : ":GL#", MOUNT_UTC_TIME_PROPERTY) && connection_result;
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
			if (!connection_result) {
				PRIVATE_DATA->reader_running = false;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
				rainbow_close(device);
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, mount_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		PRIVATE_DATA->goto_active = false;
		PRIVATE_DATA->reader_running = false;
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
		//- mount.on_disconnect
		rainbow_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		if (rainbow_write(device, ":Ch#")) {
			PRIVATE_DATA->park_deadline = indigo_monotonic_time() + 600;
			indigo_execute_handler_in(device, 0.2, mount_park_finalizer);
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
	}
	//- mount.MOUNT_PARK.on_change
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	char latitude[32], longitude[32], command[128];
	if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0) {
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
	}
	double longitude_value = (360 - MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - 360;
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
	if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		snprintf(command, sizeof(command), ":Ck%07.3f%+7.3f#", ra * 15, dec);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = rainbow_write(device, command) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
	} else {
		const char *rate_command = MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value ? ":CtS#" : MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value ? ":CtM#" : ":CtR#";
		bool ok = rainbow_write(device, rate_command);
		snprintf(command, sizeof(command), ":CtA#:Sr%s#:Sd%s#:MS#", indigo_dtos_r(ra, "%02d:%02d:%04.1f", ra_string, sizeof(ra_string)), indigo_dtos_r(dec, "%+03d*%02d:%04.1f", dec_string, sizeof(dec_string)));
		PRIVATE_DATA->goto_active = true;
		ok = rainbow_write(device, command) && ok;
		if (!ok) {
			PRIVATE_DATA->goto_active = false;
		}
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
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
	bool ok = rainbow_write(device, ":Q#");
	PRIVATE_DATA->goto_active = false;
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = MOUNT_MOTION_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_coordinates(device, ok ? "Aborted" : "Abort failed");
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	//- mount.MOUNT_ABORT_MOTION.on_change
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	const char *rate = MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value ? ":RG#" : MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value ? ":RC#" : MOUNT_SLEW_RATE_FIND_ITEM->sw.value ? ":RM#" : ":RS#";
	char command[32];
	if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		snprintf(command, sizeof(command), "%s:%s#", rate, MOUNT_MOTION_NORTH_ITEM->sw.value ? "Mn" : "Ms");
	} else {
		snprintf(command, sizeof(command), "%s", PRIVATE_DATA->version >= 200625 ? ":Qn#:Qs#" : ":Q#");
	}
	if (!rainbow_write(device, command)) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
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
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
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
	time_t seconds = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (seconds == (time_t)-1) {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		int utc_offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
		MOUNT_UTC_TIME_PROPERTY->state = rainbow_set_utc(device, seconds, utc_offset) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
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
	if (!rainbow_write(device, MOUNT_TRACKING_ON_ITEM->sw.value ? ":CtA#" : ":CtL#")) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	const char *command = MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value ? ":CtS#" : MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value ? ":CtM#" : ":CtR#";
	if (!rainbow_write(device, command)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	char command[32];
	snprintf(command, sizeof(command), ":CU0=%3.1f#", MOUNT_GUIDE_RATE_RA_ITEM->number.value / 100.0);
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
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		//- mount.on_attach
		MOUNT_PARK_PROPERTY->hidden = false;
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
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, mount_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

#pragma mark - Main code

indigo_result indigo_mount_rainbow(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static rainbow_private_data *private_data = NULL;
	static indigo_device *mount = NULL;

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
			indigo_attach_device(mount);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			last_action = action;
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
