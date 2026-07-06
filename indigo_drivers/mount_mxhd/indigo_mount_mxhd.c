#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_mount_mxhd"

#include "indigo_mount_mxhd.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_io.h>

#define PRIVATE_DATA ((mxhd_private_data *)device->private_data)
#define MXHD_DEFAULT_PORT "/dev/ttyUSB0"
#define MXHD_DEFAULT_BAUDRATE 9600
#define MXHD_POLL_SECONDS 4.0
#define MXHD_IO_TIMEOUT_MS 2000
#define MXHD_LONG_TIMEOUT_MS 10000

typedef struct {
	int handle;
	int connected_devices;
	pthread_mutex_t port_mutex;
	indigo_timer *position_timer;
	indigo_timer *guide_ra_timer;
	indigo_timer *guide_dec_timer;
	bool tracking_enabled;
	bool parked;
	bool parking;
	bool going_home;
	bool homing;
	bool slewing;
	double latitude;
	double longitude;
	bool has_site;
} mxhd_private_data;

static double clamp_dec(double value) {
	if (value > 90)
		return 90;
	if (value < -90)
		return -90;
	return value;
}

static double normalize_hours(double value) {
	while (value < 0)
		value += 24;
	while (value >= 24)
		value -= 24;
	return value;
}

static double normalize_signed_hours(double value) {
	value = normalize_hours(value);
	if (value >= 12)
		value -= 24;
	return value;
}

static bool wait_for_fd(int fd, bool write_mode, int timeout_ms) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	struct timeval timeout;
	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;
	int rc = select(fd + 1, write_mode ? NULL : &fds, write_mode ? &fds : NULL, NULL, &timeout);
	return rc > 0 && FD_ISSET(fd, &fds);
}

static bool write_all(int fd, const char *buffer, size_t size, int timeout_ms) {
	size_t offset = 0;
	while (offset < size) {
		if (!wait_for_fd(fd, true, timeout_ms))
			return false;
		ssize_t written = write(fd, buffer + offset, size - offset);
		if (written < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return false;
		}
		offset += (size_t)written;
	}
	return true;
}

static bool read_exact(int fd, uint8_t *buffer, size_t size, int timeout_ms) {
	size_t offset = 0;
	while (offset < size) {
		if (!wait_for_fd(fd, false, timeout_ms))
			return false;
		ssize_t count = read(fd, buffer + offset, size - offset);
		if (count < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return false;
		}
		if (count == 0)
			return false;
		offset += (size_t)count;
	}
	return true;
}

static bool read_hash_terminated(int fd, char *response, size_t response_size, int timeout_ms) {
	if (response_size == 0)
		return false;
	size_t offset = 0;
	while (offset + 1 < response_size) {
		uint8_t ch = 0;
		if (!read_exact(fd, &ch, 1, timeout_ms))
			return false;
		if (ch == '#')
			break;
		response[offset++] = (char)ch;
	}
	response[offset] = 0;
	return true;
}

static bool mxhd_send_locked(indigo_device *device, const char *command) {
	if (PRIVATE_DATA->handle < 0)
		return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s <- %s", device->name, command);
	return write_all(PRIVATE_DATA->handle, command, strlen(command), MXHD_IO_TIMEOUT_MS);
}

static bool mxhd_send(indigo_device *device, const char *command) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = mxhd_send_locked(device, command);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static bool mxhd_query_ack(indigo_device *device, const char *command, char *ack, int timeout_ms) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = false;
	if (PRIVATE_DATA->handle >= 0) {
		tcflush(PRIVATE_DATA->handle, TCIFLUSH);
		if (mxhd_send_locked(device, command)) {
			uint8_t byte = 0;
			result = read_exact(PRIVATE_DATA->handle, &byte, 1, timeout_ms);
			*ack = (char)byte;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static bool mxhd_query_hash(indigo_device *device, const char *command, char *response, size_t response_size, int timeout_ms) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = false;
	if (PRIVATE_DATA->handle >= 0) {
		tcflush(PRIVATE_DATA->handle, TCIFLUSH);
		if (mxhd_send_locked(device, command))
			result = read_hash_terminated(PRIVATE_DATA->handle, response, response_size, timeout_ms);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static bool mxhd_read_status(indigo_device *device, uint8_t *b1, uint8_t *b2, uint8_t *b3) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = false;
	if (PRIVATE_DATA->handle >= 0) {
		uint8_t bytes[3] = { 0 };
		tcflush(PRIVATE_DATA->handle, TCIFLUSH);
		if (mxhd_send_locked(device, "@ST#") && read_exact(PRIVATE_DATA->handle, bytes, sizeof(bytes), MXHD_IO_TIMEOUT_MS)) {
			*b1 = bytes[0];
			*b2 = bytes[1];
			*b3 = bytes[2];
			result = true;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static bool parse_sexagesimal(const char *text, double *result, bool is_dec) {
	double values[3] = { 0 };
	int count = 0;
	bool negative = false;
	const char *cursor = text;
	while (*cursor && count < 3) {
		while (*cursor && !isdigit((unsigned char)*cursor) && *cursor != '-' && *cursor != '+')
			cursor++;
		if (!*cursor)
			break;
		char *end = NULL;
		double value = strtod(cursor, &end);
		if (end == cursor)
			break;
		if (count == 0 && cursor[0] == '-')
			negative = true;
		values[count++] = value;
		cursor = end;
	}
	if (count == 0)
		return false;
	if (count == 1) {
		*result = is_dec ? clamp_dec(values[0]) : normalize_hours(values[0]);
		return true;
	}
	double sign = (values[0] < 0 || negative) ? -1 : 1;
	double absolute = fabs(values[0]) + fabs(values[1]) / 60.0 + (count >= 3 ? fabs(values[2]) / 3600.0 : 0);
	*result = is_dec ? clamp_dec(sign * absolute) : normalize_hours(absolute);
	return true;
}

static void format_ra(double ra_hours, char *buffer, size_t size) {
	ra_hours = normalize_hours(ra_hours);
	int hours = (int)floor(ra_hours);
	double minutes_float = (ra_hours - hours) * 60;
	int minutes = (int)floor(minutes_float);
	int seconds = (int)round((minutes_float - minutes) * 60);
	if (seconds >= 60) {
		seconds -= 60;
		minutes++;
	}
	if (minutes >= 60) {
		minutes -= 60;
		hours++;
	}
	if (hours >= 24)
		hours -= 24;
	snprintf(buffer, size, "%02d:%02d:%02d", hours, minutes, seconds);
}

static void format_dec(double dec_degrees, char *buffer, size_t size) {
	dec_degrees = clamp_dec(dec_degrees);
	char sign = dec_degrees < 0 ? '-' : '+';
	double absolute = fabs(dec_degrees);
	int degrees = (int)floor(absolute);
	double minutes_float = (absolute - degrees) * 60;
	int minutes = (int)floor(minutes_float);
	int seconds = (int)round((minutes_float - minutes) * 60);
	if (seconds >= 60) {
		seconds -= 60;
		minutes++;
	}
	if (minutes >= 60) {
		minutes -= 60;
		degrees++;
	}
	if (degrees > 90) {
		degrees = 90;
		minutes = 0;
		seconds = 0;
	}
	snprintf(buffer, size, "%c%02d*%02d:%02d", sign, degrees, minutes, seconds);
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
	if (east < 0)
		east += 360;
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
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) || ack != '1')
		return false;
	format_dec(dec, dec_text, sizeof(dec_text));
	snprintf(command, sizeof(command), ":Sd%s#", dec_text);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) && ack == '1';
}

static bool mxhd_query_radec(indigo_device *device, double *ra, double *dec) {
	char response[64];
	if (!mxhd_query_hash(device, ":GR#", response, sizeof(response), MXHD_IO_TIMEOUT_MS))
		return false;
	if (!parse_sexagesimal(response, ra, false))
		return false;
	if (!mxhd_query_hash(device, ":GD#", response, sizeof(response), MXHD_IO_TIMEOUT_MS))
		return false;
	return parse_sexagesimal(response, dec, true);
}

static bool mxhd_apply_site(indigo_device *device) {
	char lat[16], lon[16], command[32], ack = 0;
	format_latitude(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, lat, sizeof(lat));
	format_longitude_mount_west_positive(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, lon, sizeof(lon));
	snprintf(command, sizeof(command), ":St%s#", lat);
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) || ack != '1')
		return false;
	snprintf(command, sizeof(command), ":Sg%s#", lon);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) && ack == '1';
}

static bool mxhd_apply_utc(indigo_device *device) {
	double offset = MOUNT_UTC_OFFSET_ITEM->number.value;
	int rounded_offset = (int)round(offset);
	int mount_offset = -rounded_offset;
	if (mount_offset > 14)
		mount_offset = 14;
	if (mount_offset < -14)
		mount_offset = -14;
	char command[64], ack = 0;
	snprintf(command, sizeof(command), ":SG%c%02d#", mount_offset >= 0 ? '+' : '-', abs(mount_offset));
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) || ack != '1')
		return false;
	time_t utc = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (utc == (time_t)-1)
		utc = time(NULL);
	time_t local = utc + rounded_offset * 3600;
	struct tm local_tm;
	gmtime_r(&local, &local_tm);
	snprintf(command, sizeof(command), ":SC%02d/%02d/%02d#", local_tm.tm_mon + 1, local_tm.tm_mday, (local_tm.tm_year + 1900) % 100);
	if (!mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) || ack != '1')
		return false;
	snprintf(command, sizeof(command), ":SL%02d:%02d:%02d#", local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
	return mxhd_query_ack(device, command, &ack, MXHD_LONG_TIMEOUT_MS) && ack == '1';
}

static void update_pier_side(indigo_device *device, double ra) {
	double longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	time_t utc = indigo_get_mount_utc(device);
	double lst = indigo_lst(&utc, longitude);
	double ha = normalize_signed_hours(lst - ra);
	if (fabs(ha) < (1.0 / 60.0))
		return;
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
	if (!IS_CONNECTED)
		return;
	uint8_t b1 = 0, b2 = 0, b3 = 0;
	if (mxhd_read_status(device, &b1, &b2, &b3))
		decode_status(device, b1, b2, b3);
	if (PRIVATE_DATA->parking && !PRIVATE_DATA->slewing && !PRIVATE_DATA->homing) {
		PRIVATE_DATA->parking = false;
		PRIVATE_DATA->parked = true;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
	}
	if (PRIVATE_DATA->going_home && !PRIVATE_DATA->slewing && !PRIVATE_DATA->homing) {
		PRIVATE_DATA->going_home = false;
		PRIVATE_DATA->parked = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, "Home reached");
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
	}
	if (!PRIVATE_DATA->slewing && !PRIVATE_DATA->homing) {
		double ra = 0, dec = 0;
		if (mxhd_query_radec(device, &ra, &dec)) {
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target = ra;
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = dec;
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = dec;
			MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			update_pier_side(device, ra);
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, "Mount is moving");
	}
	indigo_reschedule_timer(device, MXHD_POLL_SECONDS, &PRIVATE_DATA->position_timer);
}

static bool mxhd_open(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0)
		return true;
	char *port = DEVICE_PORT_ITEM->text.value;
	int baudrate = (int)DEVICE_BAUDRATE_ITEM->number.value;
	if (baudrate <= 0)
		baudrate = MXHD_DEFAULT_BAUDRATE;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(port, baudrate);
	if (PRIVATE_DATA->handle < 0)
		return false;
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
	if (PRIVATE_DATA->handle >= 0) {
		(void)mxhd_send(device, ":Q#");
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
	}
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (mxhd_open(device)) {
			PRIVATE_DATA->connected_devices++;
			PRIVATE_DATA->tracking_enabled = true;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			if (!MOUNT_UTC_TIME_PROPERTY->hidden)
				(void)mxhd_apply_utc(device);
			(void)mxhd_apply_site(device);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		if (PRIVATE_DATA->connected_devices > 0)
			PRIVATE_DATA->connected_devices--;
		if (PRIVATE_DATA->connected_devices == 0)
			mxhd_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CONNECTION_PROPERTY, NULL);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_PORT_ITEM->text.value, MXHD_DEFAULT_PORT);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		DEVICE_BAUDRATE_ITEM->number.value = DEVICE_BAUDRATE_ITEM->number.target = MXHD_DEFAULT_BAUDRATE;
		MOUNT_INFO_PROPERTY->hidden = false;
		indigo_copy_value(MOUNT_INFO_VENDOR_ITEM->text.value, "MX-HD");
		indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, "MX-HD");
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_TRACK_RATE_PROPERTY->count = 3;
		indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static bool set_track_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value)
		return mxhd_send(device, "@LP1#");
	if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value)
		return mxhd_send(device, "@LP4#");
	return mxhd_send(device, "@CE0#");
}

static bool set_tracking(indigo_device *device, bool enabled) {
	const char *command = enabled ? "@FD1#" : "@FD0#";
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s tracking %s -> %s", device->name, enabled ? "ON" : "OFF", command);
	if (!enabled)
		(void)mxhd_send(device, ":Q#");
	return mxhd_send(device, command);
}

static bool set_slew_rate(indigo_device *device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		return mxhd_send(device, ":RG#");
	if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		return mxhd_send(device, ":RC#");
	if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		return mxhd_send(device, ":RM#");
	return mxhd_send(device, ":RS#");
}

static bool send_motion(indigo_device *device, indigo_property *property) {
	if (property == MOUNT_MOTION_DEC_PROPERTY) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value)
			return mxhd_send(device, ":Mn#");
		if (MOUNT_MOTION_SOUTH_ITEM->sw.value)
			return mxhd_send(device, ":Ms#");
		return mxhd_send(device, ":Qn#") && mxhd_send(device, ":Qs#");
	}
	if (MOUNT_MOTION_EAST_ITEM->sw.value)
		return mxhd_send(device, ":Me#");
	if (MOUNT_MOTION_WEST_ITEM->sw.value)
		return mxhd_send(device, ":Mw#");
	return mxhd_send(device, ":Qe#") && mxhd_send(device, ":Qw#");
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		PRIVATE_DATA->latitude = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
		PRIVATE_DATA->longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		PRIVATE_DATA->has_site = true;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = (!IS_CONNECTED || mxhd_apply_site(device)) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		MOUNT_UTC_TIME_PROPERTY->state = (!IS_CONNECTED || mxhd_apply_utc(device)) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
			time_t now = time(NULL);
			indigo_timetoisogm(now, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			MOUNT_UTC_OFFSET_ITEM->number.value = MOUNT_UTC_OFFSET_ITEM->number.target = indigo_get_utc_offset();
			MOUNT_UTC_TIME_PROPERTY->state = (!IS_CONNECTED || mxhd_apply_utc(device)) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		}
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			char response[128];
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (mxhd_set_target(device, ra, dec) && mxhd_query_hash(device, ":CM#", response, sizeof(response), MXHD_LONG_TIMEOUT_MS)) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_OK_STATE) {
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
			}
			indigo_update_coordinates(device, NULL);
		} else {
			char ack = 0;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, "Slewing");
			if (mxhd_set_target(device, ra, dec) && mxhd_query_ack(device, ":MS#", &ack, MXHD_LONG_TIMEOUT_MS) && ack == '0') {
				PRIVATE_DATA->slewing = true;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, "Slew failed");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			if (mxhd_send(device, "@Hm#")) {
				PRIVATE_DATA->parking = true;
				PRIVATE_DATA->parked = false;
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park failed");
			}
		} else {
			if (mxhd_send(device, "@OG#")) {
				PRIVATE_DATA->going_home = true;
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking via HOME");
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unpark failed");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_ITEM->sw.value = false;
			if (mxhd_send(device, "@OG#")) {
				PRIVATE_DATA->going_home = true;
				MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home");
			} else {
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, "Home failed");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = set_slew_rate(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		PRIVATE_DATA->tracking_enabled = MOUNT_TRACKING_ON_ITEM->sw.value;
		MOUNT_TRACKING_PROPERTY->state = set_tracking(device, PRIVATE_DATA->tracking_enabled) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = set_track_rate(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property) || indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		indigo_property *motion_property = indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property) ? MOUNT_MOTION_DEC_PROPERTY : MOUNT_MOTION_RA_PROPERTY;
		indigo_property_copy_values(motion_property, property, false);
		motion_property->state = send_motion(device, motion_property) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, motion_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		bool ok = PRIVATE_DATA->homing ? mxhd_send(device, "@ME0#") : mxhd_send(device, ":Q#");
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_ALERT_STATE : MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, ok ? "Aborted" : "Abort failed");
		indigo_update_coordinates(device, NULL);
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

static void guider_ra_timer_callback(indigo_device *device) {
	stop_guide_callback(device);
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_timer_callback(indigo_device *device) {
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
		if (PRIVATE_DATA->handle >= 0) {
			PRIVATE_DATA->connected_devices++;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guide_ra_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guide_dec_timer);
		if (PRIVATE_DATA->connected_devices > 0)
			PRIVATE_DATA->connected_devices--;
		if (PRIVATE_DATA->connected_devices == 0)
			mxhd_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CONNECTION_PROPERTY, CONNECTION_PROPERTY->state == INDIGO_ALERT_STATE ? "Connect the MX-HD Mount device first" : NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		indigo_cancel_timer(device, &PRIVATE_DATA->guide_ra_timer);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		const char *command = NULL;
		double duration = 0;
		if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			command = "@mE#";
			duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			command = "@mW#";
			duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		}
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		if (command != NULL) {
			if (mxhd_send(device, command)) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration / 1000.0, guider_ra_timer_callback, &PRIVATE_DATA->guide_ra_timer);
			} else {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		indigo_cancel_timer(device, &PRIVATE_DATA->guide_dec_timer);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		const char *command = NULL;
		double duration = 0;
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			command = "@mN#";
			duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			command = "@mS#";
			duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		}
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		if (command != NULL) {
			if (mxhd_send(device, command)) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration / 1000.0, guider_dec_timer_callback, &PRIVATE_DATA->guide_dec_timer);
			} else {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
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

static mxhd_private_data *private_data = NULL;
static indigo_device *mount = NULL;
static indigo_device *guider = NULL;

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
	if (action == last_action)
		return INDIGO_OK;
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(mxhd_private_data));
			private_data->handle = -1;
			private_data->tracking_enabled = true;
			pthread_mutex_init(&private_data->port_mutex, NULL);
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
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
				pthread_mutex_destroy(&private_data->port_mutex);
				free(private_data);
				private_data = NULL;
			}
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
