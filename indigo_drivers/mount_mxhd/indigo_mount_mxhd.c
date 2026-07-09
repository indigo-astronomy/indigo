// Copyright (c) 2026 Makoto Kasahara
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

// version history
// 3.0 by Makoto Kasahara

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_mount_mxhd"

#include "indigo_mount_mxhd.h"

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <indigo/indigo_align.h>
#include <indigo/indigo_uni_io.h>

#define PRIVATE_DATA ((mxhd_private_data *)device->private_data)
#define MXHD_DEFAULT_PORT "/dev/ttyUSB0"
#define MXHD_DEFAULT_BAUDRATE 9600
#define MXHD_POLL_SECONDS 4.0
#define MXHD_IO_TIMEOUT 2
#define MXHD_LONG_TIMEOUT 10

typedef struct {
	indigo_uni_handle *handle;
	int device_count;
	bool tracking_enabled;
	bool parked;
	bool parking;
	bool going_home;
	bool stop_drive_after_home;
	bool homing;
	bool slewing;
	bool stop_tracking_after_slew;
	double latitude;
	double longitude;
	bool has_site;
} mxhd_private_data;

static mxhd_private_data *private_data = NULL;
static indigo_device *mount = NULL;
static indigo_device *guider = NULL;

static bool set_tracking(indigo_device *device, bool enabled);

static void update_tracking_property(indigo_device *device, const char *message) {
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, PRIVATE_DATA->tracking_enabled ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, message);
}

static void update_track_rate_to_sidereal(indigo_device *device, const char *message) {
	indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, message);
}

static bool switch_item_selected(indigo_property *property, const char *name) {
	for (int i = 0; i < property->count; i++) {
		if (!strcmp(property->items[i].name, name)) {
			return property->items[i].sw.value;
		}
	}
	return false;
}

static void reject_if_parked(indigo_device *device, indigo_property *property) {
	property->state = INDIGO_ALERT_STATE;
	if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		indigo_update_coordinates(device, "Mount is parked!");
	} else {
		indigo_update_property(device, property, "Mount is parked!");
	}
}

static double clamp_dec(double value) {
	if (value > 90) {
		return 90;
	} else if (value < -90) {
		return -90;
	}
	return value;
}

static double normalize_hours(double value) {
	while (value < 0) {
		value += 24;
	}
	while (value >= 24) {
		value -= 24;
	}
	return value;
}

static double normalize_signed_hours(double value) {
	value = normalize_hours(value);
	if (value >= 12) {
		value -= 24;
	}
	return value;
}

static bool mxhd_read_exact(indigo_device *device, uint8_t *buffer, long size, int timeout) {
	long offset = 0;
	while (offset < size) {
		if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(timeout)) <= 0) {
			return false;
		}
		long count = indigo_uni_read(PRIVATE_DATA->handle, buffer + offset, size - offset);
		if (count <= 0) {
			return false;
		}
		offset += count;
	}
	return true;
}

static bool mxhd_read_hash_terminated(indigo_device *device, char *response, long response_size, int timeout) {
	if (response_size <= 0) {
		return false;
	}
	long offset = 0;
	while (offset + 1 < response_size) {
		uint8_t ch = 0;
		if (!mxhd_read_exact(device, &ch, 1, timeout)) {
			return false;
		}
		if (ch == '#') {
			break;
		}
		response[offset++] = (char)ch;
	}
	response[offset] = 0;
	return true;
}

static bool mxhd_send(indigo_device *device, const char *command) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!indigo_uni_is_valid(PRIVATE_DATA->handle)) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s <- %s", device->name, command);
	return indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0;
}

static bool mxhd_query_ack(indigo_device *device, const char *command, char *ack, int timeout) {
	bool result = false;
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		if (mxhd_send(device, command)) {
			uint8_t byte = 0;
			result = mxhd_read_exact(device, &byte, 1, timeout);
			*ack = (char)byte;
		}
	}
	return result;
}

static bool mxhd_query_hash(indigo_device *device, const char *command, char *response, long response_size, int timeout) {
	bool result = false;
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_discard(PRIVATE_DATA->handle);
		if (mxhd_send(device, command)) {
			result = mxhd_read_hash_terminated(device, response, response_size, timeout);
		}
	}
	return result;
}

static void mxhd_update_mount_info(indigo_device *device) {
	char response[128];
	if (mxhd_query_hash(device, ":GVP#", response, sizeof(response), MXHD_IO_TIMEOUT) && response[0] != 0) {
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, response);
	}
	if (mxhd_query_hash(device, ":GVF#", response, sizeof(response), MXHD_IO_TIMEOUT) && response[0] != 0) {
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}
	MOUNT_INFO_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_INFO_PROPERTY, NULL);
}

static bool mxhd_read_status(indigo_device *device, uint8_t *b1, uint8_t *b2, uint8_t *b3) {
	bool result = false;
	if (PRIVATE_DATA->handle != NULL) {
		uint8_t bytes[3] = { 0 };
		indigo_uni_discard(PRIVATE_DATA->handle);
		if (mxhd_send(device, "@ST#") && mxhd_read_exact(device, bytes, sizeof(bytes), MXHD_IO_TIMEOUT)) {
			*b1 = bytes[0];
			*b2 = bytes[1];
			*b3 = bytes[2];
			result = true;
		}
	}
	return result;
}

static bool parse_sexagesimal(const char *text, double *result, bool is_dec) {
	double value = indigo_stod(text);
	if (isnan(value)) {
		return false;
	}
	*result = is_dec ? clamp_dec(value) : normalize_hours(value);
	return true;
}

static void format_ra(double ra_hours, char *buffer, size_t size) {
	char *value = indigo_dtos(normalize_hours(ra_hours), "%02d:%02d:%02.0f");
	strncpy(buffer, value, size);
	buffer[size - 1] = 0;
}

static void format_dec(double dec_degrees, char *buffer, size_t size) {
	char *value = indigo_dtos(clamp_dec(dec_degrees), "%+03d*%02d:%02.0f");
	strncpy(buffer, value, size);
	buffer[size - 1] = 0;
}

static void format_latitude(double latitude, char *buffer, size_t size) {
	latitude = fmax(-90, fmin(90, latitude));
	char sign = latitude < 0 ? '-' : '+';
	double absolute = fabs(latitude);
	int degrees = (int)floor(absolute);
	int minutes = (int)round((absolute - degrees) * 60);
	if (minutes >= 60) {
		minutes -= 60;
		degrees++;
	}
	snprintf(buffer, size, "%c%02d*%02d", sign, degrees, minutes);
}

static void format_longitude_mount_west_positive(double longitude_east, char *buffer, size_t size) {
	double east = fmod(longitude_east, 360);
	if (east < 0) {
		east += 360;
	}
	double west = fmod(360 - east, 360);
	int total_minutes = (int)round(west * 60);
	total_minutes %= 360 * 60;
	int degrees = total_minutes / 60;
	int minutes = total_minutes % 60;
	snprintf(buffer, size, "+%03d*%02d", degrees, minutes);
}

static bool mxhd_set_target(indigo_device *device, double ra, double dec) {
	char ra_text[32], dec_text[32], command[64], ack = 0;
	format_ra(ra, ra_text, sizeof(ra_text));
	snprintf(command, sizeof(command), ":Sr%s#", ra_text);
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) || ack != '1') {
		return false;
	}
	format_dec(dec, dec_text, sizeof(dec_text));
	snprintf(command, sizeof(command), ":Sd%s#", dec_text);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) && ack == '1';
}

static bool mxhd_query_radec(indigo_device *device, double *ra, double *dec) {
	char response[64];
	if (!mxhd_query_hash(device, ":GR#", response, sizeof(response), MXHD_IO_TIMEOUT)) {
		return false;
	}
	if (!parse_sexagesimal(response, ra, false)) {
		return false;
	}
	if (!mxhd_query_hash(device, ":GD#", response, sizeof(response), MXHD_IO_TIMEOUT)) {
		return false;
	}
	return parse_sexagesimal(response, dec, true);
}

static bool mxhd_apply_site(indigo_device *device) {
	char lat[16], lon[16], command[32], ack = 0;
	format_latitude(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, lat, sizeof(lat));
	format_longitude_mount_west_positive(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, lon, sizeof(lon));
	snprintf(command, sizeof(command), ":St%s#", lat);
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) || ack != '1') {
		return false;
	}
	snprintf(command, sizeof(command), ":Sg%s#", lon);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) && ack == '1';
}

static bool mxhd_apply_utc(indigo_device *device) {
	int rounded_offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
	int mount_offset = -rounded_offset;
	if (mount_offset > 14) {
		mount_offset = 14;
	}
	if (mount_offset < -14) {
		mount_offset = -14;
	}
	char command[64], ack = 0;
	snprintf(command, sizeof(command), ":SG%c%02d#", mount_offset >= 0 ? '+' : '-', abs(mount_offset));
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) || ack != '1') {
		return false;
	}
	time_t utc = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (utc == (time_t)-1) {
		utc = time(NULL);
	}
	time_t local = utc + rounded_offset * 3600;
	struct tm local_tm;
	indigo_gmtime(&local, &local_tm);
	snprintf(command, sizeof(command), ":SC%02d/%02d/%02d#", local_tm.tm_mon + 1, local_tm.tm_mday, (local_tm.tm_year + 1900) % 100);
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) || ack != '1') {
		return false;
	}
	snprintf(command, sizeof(command), ":SL%02d:%02d:%02d#", local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT) && ack == '1';
}

static void update_pier_side(indigo_device *device, double ra) {
	double longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	time_t utc = indigo_get_mount_utc(device);
	double lst = indigo_lst(&utc, longitude);
	double ha = normalize_signed_hours(lst - ra);
	if (fabs(ha) < (1.0 / 60.0)) {
		return;
	}
	indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, ha > 0 ? MOUNT_SIDE_OF_PIER_EAST_ITEM : MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
	MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
}

static void decode_status(indigo_device *device, uint8_t b1, uint8_t b2, uint8_t b3) {
	(void)b1;
	(void)b2;
	PRIVATE_DATA->homing = (b3 & 0x80) != 0;
	PRIVATE_DATA->slewing = PRIVATE_DATA->homing || ((b3 & 0x40) != 0);
}

static void position_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	uint8_t b1 = 0, b2 = 0, b3 = 0;
	if (mxhd_read_status(device, &b1, &b2, &b3)) {
		decode_status(device, b1, b2, b3);
	}
	if (PRIVATE_DATA->parking && !PRIVATE_DATA->slewing && !PRIVATE_DATA->homing) {
		PRIVATE_DATA->parking = false;
		PRIVATE_DATA->parked = true;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
		PRIVATE_DATA->tracking_enabled = false;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Parked, tracking stopped");
	}
	if (PRIVATE_DATA->going_home && !PRIVATE_DATA->slewing && !PRIVATE_DATA->homing) {
		bool stop_drive_after_home = PRIVATE_DATA->stop_drive_after_home;
		PRIVATE_DATA->going_home = false;
		PRIVATE_DATA->stop_drive_after_home = false;
		PRIVATE_DATA->parked = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, "Home reached");
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
		update_track_rate_to_sidereal(device, "Home operation selected sidereal rate");
		if (stop_drive_after_home) {
			PRIVATE_DATA->tracking_enabled = false;
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			if (mxhd_send(device, "@FD0#")) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Home reached, tracking stopped");
			} else {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Home reached, tracking stop failed");
			}
		} else {
			PRIVATE_DATA->tracking_enabled = true;
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Unparked, tracking active");
		}
	}
	bool moving = PRIVATE_DATA->slewing || PRIVATE_DATA->homing;
	if (PRIVATE_DATA->stop_tracking_after_slew && !moving) {
		PRIVATE_DATA->stop_tracking_after_slew = false;
		PRIVATE_DATA->tracking_enabled = false;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		if (set_tracking(device, false)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Slew complete, tracking stopped");
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Slew complete, tracking stop failed");
		}
	}
	double ra = 0, dec = 0;
	if (mxhd_query_radec(device, &ra, &dec)) {
		MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target = ra;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = dec;
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = dec;
		MOUNT_RAW_COORDINATES_PROPERTY->state = moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_coordinates(device, moving ? "Mount is moving" : NULL);
		indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
		update_pier_side(device, ra);
	} else if (moving) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, "Mount is moving");
	}
	indigo_execute_handler_in(device, MXHD_POLL_SECONDS, position_timer_callback);
}

static bool mxhd_open(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		return true;
	}
	char *port = DEVICE_PORT_ITEM->text.value;
	int baudrate = (int)DEVICE_BAUDRATE_ITEM->number.value;
	if (baudrate <= 0) {
		baudrate = MXHD_DEFAULT_BAUDRATE;
	}
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(port, baudrate, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	uint8_t b1 = 0, b2 = 0, b3 = 0;
	for (int attempt = 0; attempt < 3; attempt++) {
		if (mxhd_read_status(device, &b1, &b2, &b3)) {
			decode_status(device, b1, b2, b3);
			return true;
		}
		indigo_usleep(200000);
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Handshake status read failed; keeping %s open for client retries", port);
	return true;
}

static void mxhd_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		(void)mxhd_send(device, ":Q#");
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = mxhd_open(device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			update_tracking_property(device, NULL);
			mxhd_update_mount_info(device);
			indigo_execute_handler(device, position_timer_callback);
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (PRIVATE_DATA->device_count > 0 && --PRIVATE_DATA->device_count == 0) {
			mxhd_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, MXHD_DEFAULT_PORT);
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		DEVICE_BAUDRATE_ITEM->number.value = DEVICE_BAUDRATE_ITEM->number.target = MXHD_DEFAULT_BAUDRATE;
		MOUNT_INFO_PROPERTY->hidden = false;
		INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "GOTO Telescope");
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, "MX-HD");
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, "Unknown");
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_TRACK_RATE_PROPERTY->count = 3;
		indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static bool set_track_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
		return mxhd_send(device, "@LP1#");
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
		return mxhd_send(device, "@LP4#");
	}
	return mxhd_send(device, "@CE0#");
}

static bool set_tracking(indigo_device *device, bool enabled) {
	const char *command = enabled ? "@FD1#" : "@FD0#";
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s tracking %s -> %s", device->name, enabled ? "ON" : "OFF", command);
	if (!enabled) {
		(void)mxhd_send(device, ":Q#");
	}
	return mxhd_send(device, command);
}

static bool enable_motion_and_send(indigo_device *device, const char *command) {
	return mxhd_send(device, "@ME1#") && mxhd_send(device, command);
}

static bool set_slew_rate(indigo_device *device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		return mxhd_send(device, ":RG#");
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		return mxhd_send(device, ":RC#");
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		return mxhd_send(device, ":RM#");
	}
	return mxhd_send(device, ":RS#");
}

static bool send_motion(indigo_device *device, indigo_property *property) {
	if (property == MOUNT_MOTION_DEC_PROPERTY) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			return mxhd_send(device, ":Mn#");
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			return mxhd_send(device, ":Ms#");
		}
		return mxhd_send(device, ":Qn#") && mxhd_send(device, ":Qs#");
	}
	if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		return mxhd_send(device, ":Me#");
	} else if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		return mxhd_send(device, ":Mw#");
	}
	return mxhd_send(device, ":Qe#") && mxhd_send(device, ":Qw#");
}

static void mount_geo_coords_callback(indigo_device *device) {
	PRIVATE_DATA->latitude = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
	PRIVATE_DATA->longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	PRIVATE_DATA->has_site = true;
	if (!IS_CONNECTED || mxhd_apply_site(device)) {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_set_utc_time_callback(indigo_device *device) {
	if (!IS_CONNECTED || mxhd_apply_utc(device)) {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_set_host_time_callback(indigo_device *device) {
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		time_t now = time(NULL);
		indigo_timetoisogm(now, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", indigo_get_utc_offset());
		mount_set_utc_time_callback(device);
	}
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_eq_coords_callback(indigo_device *device) {
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		PRIVATE_DATA->stop_tracking_after_slew = false;
		char response[128];
		if (mxhd_set_target(device, ra, dec) && mxhd_query_hash(device, ":CM#", response, sizeof(response), MXHD_LONG_TIMEOUT)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_coordinates(device, NULL);
	} else {
		char ack = 0;
		bool stop_after_slew = MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value;
		bool was_tracking = PRIVATE_DATA->tracking_enabled;
		bool target_set = mxhd_set_target(device, ra, dec);
		bool tracking_started = target_set && set_tracking(device, true);
		if (tracking_started) {
			PRIVATE_DATA->tracking_enabled = true;
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Tracking enabled before slew");
		}
		if (tracking_started && mxhd_query_ack(device, ":MS#", &ack, MXHD_LONG_TIMEOUT) && ack == '0') {
			PRIVATE_DATA->slewing = true;
			PRIVATE_DATA->stop_tracking_after_slew = stop_after_slew;
			update_track_rate_to_sidereal(device, "Slew selected sidereal rate");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, "Slewing");
		} else {
			PRIVATE_DATA->stop_tracking_after_slew = false;
			if (tracking_started && (stop_after_slew || !was_tracking)) {
				PRIVATE_DATA->tracking_enabled = was_tracking;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, was_tracking ? MOUNT_TRACKING_ON_ITEM : MOUNT_TRACKING_OFF_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = set_tracking(device, was_tracking) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Slew failed, tracking restored");
			}
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, "Slew failed");
		}
	}
}

static void mount_park_callback(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (enable_motion_and_send(device, "@Hm#")) {
			PRIVATE_DATA->parking = true;
			PRIVATE_DATA->parked = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park failed");
		}
	} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
		if (enable_motion_and_send(device, "@OG#")) {
			PRIVATE_DATA->going_home = true;
			PRIVATE_DATA->stop_drive_after_home = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking via HOME");
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unpark failed");
		}
	}
}

static void mount_home_callback(indigo_device *device) {
	if (MOUNT_HOME_ITEM->sw.value) {
		MOUNT_HOME_ITEM->sw.value = false;
		if (enable_motion_and_send(device, "@OG#")) {
			PRIVATE_DATA->going_home = true;
			PRIVATE_DATA->stop_drive_after_home = true;
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home");
		} else {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Home failed");
		}
	}
}

static void mount_slew_rate_callback(indigo_device *device) {
	if (set_slew_rate(device)) {
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}

static void mount_tracking_callback(indigo_device *device) {
	PRIVATE_DATA->tracking_enabled = MOUNT_TRACKING_ON_ITEM->sw.value;
	if (set_tracking(device, PRIVATE_DATA->tracking_enabled)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_callback(indigo_device *device) {
	if (set_track_rate(device)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_motion_dec_callback(indigo_device *device) {
	if (send_motion(device, MOUNT_MOTION_DEC_PROPERTY)) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_callback(indigo_device *device) {
	if (send_motion(device, MOUNT_MOTION_RA_PROPERTY)) {
		if (MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_abort_callback(indigo_device *device) {
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		bool stop_tracking_after_slew = PRIVATE_DATA->stop_tracking_after_slew;
		PRIVATE_DATA->stop_tracking_after_slew = false;
		PRIVATE_DATA->stop_drive_after_home = false;
		bool ok = PRIVATE_DATA->homing ? mxhd_send(device, "@ME0#") : mxhd_send(device, ":Q#");
		const char *message = ok ? "Aborted" : "Abort failed";
		MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (ok) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			if (stop_tracking_after_slew) {
				PRIVATE_DATA->tracking_enabled = false;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				if (mxhd_send(device, "@FD0#")) {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Aborted, tracking stopped");
					message = "Aborted, tracking stopped";
				} else {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Abort succeeded, tracking stop failed");
					MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
					message = "Abort succeeded, tracking stop failed";
				}
			}
		}
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, message);
		indigo_update_coordinates(device, NULL);
	}
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_execute_handler(device, mount_connect_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_execute_handler(device, mount_geo_coords_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_execute_handler(device, mount_set_utc_time_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		indigo_execute_handler(device, mount_set_host_time_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		indigo_execute_handler(device, mount_eq_coords_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		if (PRIVATE_DATA->parked && !switch_item_selected(property, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Already parked");
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		indigo_execute_handler(device, mount_park_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_HOME_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
			indigo_execute_handler(device, mount_home_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
		indigo_execute_handler(device, mount_slew_rate_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_TRACKING_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		indigo_execute_handler(device, mount_tracking_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_TRACK_RATE_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		indigo_execute_handler(device, mount_track_rate_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_MOTION_DEC_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_execute_handler(device, mount_motion_dec_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, MOUNT_MOTION_RA_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_execute_handler(device, mount_motion_ra_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, mount_abort_callback);
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

static void stop_guide_callback(indigo_device *device) {
	(void)mxhd_send(device, ":Q#");
}

static void guider_guide_ra_finish_callback(indigo_device *device) {
	stop_guide_callback(device);
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finish_callback(indigo_device *device) {
	stop_guide_callback(device);
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->count = 2;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = mxhd_open(device->master_device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->device_count > 0 && --PRIVATE_DATA->device_count == 0) {
			mxhd_close(device->master_device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_callback(indigo_device *device) {
	const char *command = NULL;
	double duration = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		command = "@mE#";
		duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		command = "@mW#";
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
	}
	if (command == NULL) {
		guider_guide_ra_finish_callback(device);
	} else if (mxhd_send(device, command)) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, duration / 1000.0, guider_guide_ra_finish_callback);
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static void guider_guide_dec_callback(indigo_device *device) {
	const char *command = NULL;
	double duration = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		command = "@mN#";
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		command = "@mS#";
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	}
	if (command == NULL) {
		guider_guide_dec_finish_callback(device);
	} else if (mxhd_send(device, command)) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, duration / 1000.0, guider_guide_dec_finish_callback);
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_execute_handler(device, guider_connect_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, GUIDER_GUIDE_RA_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, guider_guide_ra_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		if (PRIVATE_DATA->parked) {
			reject_if_parked(device, GUIDER_GUIDE_DEC_PROPERTY);
			return INDIGO_OK;
		}
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, guider_guide_dec_callback);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

indigo_result indigo_mount_mxhd(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_MXHD_NAME,
		mount_attach,
		indigo_mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_MXHD_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	SET_DRIVER_INFO(info, "MX-HD Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);
	if (action == last_action) {
		return INDIGO_OK;
	}
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(mxhd_private_data));
			private_data->handle = NULL;
			private_data->tracking_enabled = false;
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;
		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
