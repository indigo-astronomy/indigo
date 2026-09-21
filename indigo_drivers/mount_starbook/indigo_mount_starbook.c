// Copyright (c) 2017-2026 Koji Tsunoda
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

// This file generated from indigo_mount_starbook.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <time.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_starbook.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_mount_starbook"
#define DRIVER_LABEL         "Vixen StarBook Mount"
#define MOUNT_DEVICE_NAME    "Mount Vixen StarBook"
#define GUIDER_DEVICE_NAME   "Mount Vixen StarBook (guider)"
#define PRIVATE_DATA         ((starbook_private_data *)device->private_data)

//+ define

#define STARBOOK_DEFAULT_ADDRESS "169.254.0.1"
#define STARBOOK_STATE_INIT  1
#define STARBOOK_STATE_SCOPE 2
#define STARBOOK_STATE_CHART 3
#define STARBOOK_STATE_STOP  4
#define STARBOOK_STATE_TRACK 5
#define STARBOOK_TRACK_STATE_STOP 0
#define STARBOOK_PIERSIDE_EAST 0
#define STARBOOK_PIERSIDE_WEST 1
#define STARBOOK_ERROR_NONE  0
#define STARBOOK_ERROR_ILLEGAL_STATE 1
#define STARBOOK_ERROR_FORMAT 2
#define STARBOOK_ERROR_BELOW_HORIZON 3
#define STARBOOK_WARNING_NEAR_SUN 4
#define STARBOOK_ERROR_UNKNOWN 5

//- define

#pragma mark - Property definitions

#define TIMEZONE_PROPERTY              (PRIVATE_DATA->timezone_property)
#define TIMEZONE_VALUE_ITEM            (TIMEZONE_PROPERTY->items + 0)

#define TIMEZONE_PROPERTY_NAME         "X_STARBOOK_TIMEZONE"
#define TIMEZONE_VALUE_ITEM_NAME       "VALUE"

#define RESET_PROPERTY                 (PRIVATE_DATA->reset_property)
#define RESET_CTRL_ITEM                (RESET_PROPERTY->items + 0)

#define RESET_PROPERTY_NAME            "X_STARBOOK_RESET"
#define RESET_CTRL_ITEM_NAME           "RESET"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *timezone_property;
	indigo_property *reset_property;
	//+ data
	char endpoint[INDIGO_VALUE_SIZE + 8];
	char host_header[INDIGO_VALUE_SIZE];
	char response[4096];
	size_t response_size;
	double version;
	double current_ra, current_dec;
	double goto_deadline;
	int current_state, current_speed;
	bool move_north, move_south, move_east, move_west;
	//- data
} starbook_private_data;

#pragma mark - Low level code

//+ code

static void mount_goto_finalizer(indigo_device *device);
static void guider_guide_ra_finalizer(indigo_device *device);
static void guider_guide_dec_finalizer(indigo_device *device);

static bool starbook_http_get(indigo_device *device, const char *path) {
	indigo_uni_handle *handle = indigo_uni_open_url(PRIVATE_DATA->endpoint, 80, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	if (handle == NULL) {
		return false;
	}
	PRIVATE_DATA->response_size = 0;
	PRIVATE_DATA->response[0] = 0;
	bool ok = indigo_uni_printf(handle, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, PRIVATE_DATA->host_header) > 0;
	double deadline = indigo_monotonic_time() + 3;
	while (ok && PRIVATE_DATA->response_size < sizeof(PRIVATE_DATA->response) - 1 && indigo_monotonic_time() < deadline) {
		int available = indigo_uni_wait_for_data(handle, INDIGO_DELAY(0.1));
		if (available > 0) {
			long count = indigo_uni_read_available(handle, PRIVATE_DATA->response + PRIVATE_DATA->response_size, sizeof(PRIVATE_DATA->response) - PRIVATE_DATA->response_size - 1);
			if (count < 0) {
				ok = false;
			} else {
				PRIVATE_DATA->response_size += count;
				PRIVATE_DATA->response[PRIVATE_DATA->response_size] = 0;
				if (strstr(PRIVATE_DATA->response, "</html>") != NULL) break;
			}
		} else if (available < 0) {
			break;
		}
	}
	indigo_uni_close(&handle);
	ok = ok && (strstr(PRIVATE_DATA->response, "HTTP/1.0 200") == PRIVATE_DATA->response || strstr(PRIVATE_DATA->response, "HTTP/1.1 200") == PRIVATE_DATA->response) && strstr(PRIVATE_DATA->response, "</html>") != NULL;
	if (!ok) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GET %s failed", path);
	return ok;
}

static bool starbook_get(indigo_device *device, const char *path) {
	if (!starbook_http_get(device, path)) {
		return false;
	}
	char *start = strstr(PRIVATE_DATA->response, "<!--");
	char *end = start == NULL ? NULL : strstr(start + 4, "-->");
	if (start != NULL && end != NULL) {
		start += 4;
	} else {
		start = strstr(PRIVATE_DATA->response, "</HEAD>");
		end = start == NULL ? NULL : strstr(start + 7, "</html>");
		if (start != NULL && end != NULL) {
			start += 7;
		} else {
			start = PRIVATE_DATA->response;
			end = strstr(start, "</html>");
		}
	}
	if (end == NULL || end < start) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Malformed response to %s", path);
		return false;
	}
	size_t length = end - start;
	memmove(PRIVATE_DATA->response, start, length);
	PRIVATE_DATA->response[length] = 0;
	PRIVATE_DATA->response_size = length;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s", path, PRIVATE_DATA->response);
	return true;
}

static bool starbook_query_value(const char *query, const char *key, char *value, size_t size) {
	size_t key_length = strlen(key);
	const char *cursor = query;
	while (*cursor) {
		if (!strncasecmp(cursor, key, key_length)) {
			const char *end = strchr(cursor + key_length, '&');
			size_t length = end == NULL ? strlen(cursor + key_length) : (size_t)(end - cursor - key_length);
			if (length == 0 || length >= size) {
				return false;
			}
			memcpy(value, cursor + key_length, length);
			value[length] = 0;
			return true;
		}
		cursor = strchr(cursor, '&');
		if (cursor == NULL) {
			break;
		}
		cursor++;
	}
	return false;
}

static bool starbook_query_double(const char *query, const char *key, double *value) {
	char text[64], *end;
	if (!starbook_query_value(query, key, text, sizeof(text))) {
		return false;
	}
	errno = 0;
	double parsed = strtod(text, &end);
	if (errno != 0 || *end != 0 || !isfinite(parsed)) {
		return false;
	}
	*value = parsed;
	return true;
}

static bool starbook_query_int(const char *query, const char *key, int *value) {
	char text[64], *end;
	if (!starbook_query_value(query, key, text, sizeof(text))) {
		return false;
	}
	errno = 0;
	long parsed = strtol(text, &end, 10);
	if (errno != 0 || *end != 0 || parsed < INT_MIN || parsed > INT_MAX) {
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool starbook_set(indigo_device *device, const char *path, int *error) {
	if (error != NULL) {
		*error = STARBOOK_ERROR_NONE;
	}
	if (!starbook_get(device, path)) {
		return false;
	}
	if (!strcmp(PRIVATE_DATA->response, "OK")) {
		return true;
	}
	if (error != NULL) {
		if (!strcmp(PRIVATE_DATA->response, "ERROR:ILLEGAL STATE")) *error = STARBOOK_ERROR_ILLEGAL_STATE;
		else if (!strcmp(PRIVATE_DATA->response, "ERROR:FORMAT")) *error = STARBOOK_ERROR_FORMAT;
		else if (!strcmp(PRIVATE_DATA->response, "ERROR:BELOW HORIZON") || !strcmp(PRIVATE_DATA->response, "ERROR:BELOW HORIZONE")) *error = STARBOOK_ERROR_BELOW_HORIZON;
		else if (!strcmp(PRIVATE_DATA->response, "WARNING:NEAR SUN")) *error = STARBOOK_WARNING_NEAR_SUN;
		else *error = STARBOOK_ERROR_UNKNOWN;
	}
	return false;
}

static const char *starbook_error_text(int error) {
	switch (error) {
		case STARBOOK_ERROR_ILLEGAL_STATE: return "ILLEGAL STATE";
		case STARBOOK_ERROR_FORMAT: return "FORMAT";
		case STARBOOK_ERROR_BELOW_HORIZON: return "BELOW HORIZON";
		case STARBOOK_WARNING_NEAR_SUN: return "NEAR SUN";
		case STARBOOK_ERROR_UNKNOWN: return "UNKNOWN";
		default: return NULL;
	}
}

static bool starbook_get_version(indigo_device *device, double *version) {
	return starbook_get(device, "/VERSION") && starbook_query_double(PRIVATE_DATA->response, "VERSION=", version) && *version > 0;
}

static bool starbook_parse_degree_minute(const char *query, const char *key, double *value) {
	char text[64], *separator, *end;
	if (!starbook_query_value(query, key, text, sizeof(text))) return false;
	int sign = text[0] == '-' ? -1 : 1;
	char *start = text + (text[0] == '-' || text[0] == '+' ? 1 : 0);
	separator = strchr(start, '+');
	if (separator == NULL) return false;
	*separator = 0;
	errno = 0;
	long degrees = strtol(start, &end, 10);
	if (errno != 0 || *end != 0) return false;
	double minutes = strtod(separator + 1, &end);
	if (errno != 0 || *end != 0 || minutes < 0 || minutes >= 60) return false;
	*value = sign * (degrees + minutes / 60.0);
	return true;
}

static bool starbook_get_status(indigo_device *device, double *ra, double *dec, int *going, int *state) {
	bool high_precision = PRIVATE_DATA->version >= 4.20;
	if (!starbook_get(device, high_precision ? "/GETSTATUS2" : "/GETSTATUS")) return false;
	if (high_precision) {
		if (!starbook_query_double(PRIVATE_DATA->response, "RA=", ra) || !starbook_query_double(PRIVATE_DATA->response, "DEC=", dec)) return false;
	} else if (!starbook_parse_degree_minute(PRIVATE_DATA->response, "RA=", ra) || !starbook_parse_degree_minute(PRIVATE_DATA->response, "DEC=", dec)) return false;
	char text[32];
	if (!starbook_query_int(PRIVATE_DATA->response, "GOTO=", going) || (*going != 0 && *going != 1) || !starbook_query_value(PRIVATE_DATA->response, "STATE=", text, sizeof(text))) return false;
	if (!strcmp(text, "INIT")) *state = STARBOOK_STATE_INIT;
	else if (!strcmp(text, "SCOPE")) *state = STARBOOK_STATE_SCOPE;
	else if (!strcmp(text, "CHART")) *state = STARBOOK_STATE_CHART;
	else if (!strcmp(text, "STOP")) *state = STARBOOK_STATE_STOP;
	else if (!strcmp(text, "TRACK")) *state = STARBOOK_STATE_TRACK;
	else return false;
	return *ra >= 0 && *ra <= 24 && *dec >= -90 && *dec <= 90;
}

static bool starbook_get_track_status(indigo_device *device, int *state) {
	return starbook_get(device, "/GETTRACKSTATUS") && starbook_query_int(PRIVATE_DATA->response, "TRACK=", state) && *state >= 0 && *state <= 2;
}

static bool starbook_get_pierside(indigo_device *device, int *side) {
	return starbook_get(device, "/GET_PIERSIDE") && starbook_query_int(PRIVATE_DATA->response, "PIERSIDE=", side) && (*side == STARBOOK_PIERSIDE_EAST || *side == STARBOOK_PIERSIDE_WEST);
}

static bool starbook_get_place(indigo_device *device, double *longitude, double *latitude, int *timezone) {
	char text[64], *separator, *end;
	if (!starbook_get(device, "/GETPLACE") || !starbook_query_value(PRIVATE_DATA->response, "LONGITUDE=", text, sizeof(text)) || (text[0] != 'E' && text[0] != 'W') || (separator = strchr(text + 1, '+')) == NULL) return false;
	*separator = 0;
	double degrees = strtod(text + 1, &end);
	if (*end != 0) return false;
	double minutes = strtod(separator + 1, &end);
	if (*end != 0 || degrees > 180 || minutes < 0 || minutes >= 60) return false;
	*longitude = (text[0] == 'W' ? -1 : 1) * (degrees + minutes / 60.0);
	if (!starbook_query_value(PRIVATE_DATA->response, "LATITUDE=", text, sizeof(text)) || (text[0] != 'N' && text[0] != 'S') || (separator = strchr(text + 1, '+')) == NULL) return false;
	*separator = 0;
	degrees = strtod(text + 1, &end);
	if (*end != 0) return false;
	minutes = strtod(separator + 1, &end);
	if (*end != 0 || degrees > 90 || minutes < 0 || minutes >= 60) return false;
	*latitude = (text[0] == 'S' ? -1 : 1) * (degrees + minutes / 60.0);
	return starbook_query_int(PRIVATE_DATA->response, "TIMEZONE=", timezone) && *timezone >= -12 && *timezone <= 12;
}

static bool starbook_get_utc(indigo_device *device, time_t *utc, int *offset) {
	char text[64];
	int year, month, day, hour, minute, second;
	if (!starbook_get(device, "/GETTIME") || !starbook_query_value(PRIVATE_DATA->response, "TIME=", text, sizeof(text)) || sscanf(text, "%d+%d+%d+%d+%d+%d", &year, &month, &day, &hour, &minute, &second) != 6 || year < 2000 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) return false;
	struct tm value = { 0 };
	value.tm_year = year - 1900;
	value.tm_mon = month - 1;
	value.tm_mday = day;
	value.tm_hour = hour;
	value.tm_min = minute;
	value.tm_sec = second;
	*offset = (int)TIMEZONE_VALUE_ITEM->number.value;
	*utc = indigo_timegm(&value) - *offset * 3600;
	return *utc != (time_t)-1;
}

static bool starbook_set_utc(indigo_device *device, time_t utc, int offset) {
	time_t local = utc + offset * 3600;
	struct tm value;
	indigo_gmtime(&local, &value);
	char path[128];
	snprintf(path, sizeof(path), "/SETTIME?TIME=%d+%02d+%02d+%02d+%02d+%02d", value.tm_year + 1900, value.tm_mon + 1, value.tm_mday, value.tm_hour, value.tm_min, value.tm_sec);
	return starbook_set(device, path, NULL);
}

static bool starbook_set_place(indigo_device *device, double longitude, double latitude, int timezone) {
	double longitude_degrees, latitude_degrees;
	double longitude_fraction = modf(fabs(longitude), &longitude_degrees);
	double latitude_fraction = modf(fabs(latitude), &latitude_degrees);
	char path[192];
	snprintf(path, sizeof(path), "/SETPLACE?LONGITUDE=%c%d+%d&LATITUDE=%c%d+%d&TIMEZONE=%d", longitude < 0 ? 'W' : 'E', (int)longitude_degrees, (int)(longitude_fraction * 60), latitude < 0 ? 'S' : 'N', (int)latitude_degrees, (int)(latitude_fraction * 60), timezone);
	return starbook_set(device, path, NULL);
}

static bool starbook_set_coordinates(indigo_device *device, bool sync, double ra, double dec, int *error) {
	double ra_degrees, dec_degrees;
	double ra_fraction = modf(fabs(ra), &ra_degrees);
	double dec_fraction = modf(fabs(dec), &dec_degrees);
	char path[192];
	if (PRIVATE_DATA->version >= 4.20) snprintf(path, sizeof(path), "/%s?ra=%d+%04.3f&dec=%c%d+%05.2f", sync ? "ALIGN" : "GOTORADEC", (int)ra_degrees, ra_fraction * 60, dec < 0 ? '-' : '+', (int)dec_degrees, dec_fraction * 60);
	else snprintf(path, sizeof(path), "/%s?ra=%d+%02.1f&dec=%c%d+%02d", sync ? "ALIGN" : "GOTORADEC", (int)ra_degrees, ra_fraction * 60, dec < 0 ? '-' : '+', (int)dec_degrees, (int)(dec_fraction * 60));
	return starbook_set(device, path, error);
}

static bool starbook_start(indigo_device *device) {
	return starbook_set(device, PRIVATE_DATA->version <= 2.7 ? "/START" : "/START?INIT=OFF", NULL);
}

static bool starbook_move(indigo_device *device, bool north, bool south, bool east, bool west) {
	if (PRIVATE_DATA->move_north == north && PRIVATE_DATA->move_south == south && PRIVATE_DATA->move_east == east && PRIVATE_DATA->move_west == west) return true;
	char path[128];
	snprintf(path, sizeof(path), "/MOVE?NORTH=%d&SOUTH=%d&EAST=%d&WEST=%d", north, south, east, west);
	if (!starbook_set(device, path, NULL)) return false;
	PRIVATE_DATA->move_north = north;
	PRIVATE_DATA->move_south = south;
	PRIVATE_DATA->move_east = east;
	PRIVATE_DATA->move_west = west;
	return true;
}

static bool starbook_set_speed(indigo_device *device, int speed) {
	if (PRIVATE_DATA->current_speed == speed) return true;
	char path[64];
	snprintf(path, sizeof(path), "/SETSPEED?speed=%d", speed);
	if (!starbook_set(device, path, NULL)) return false;
	PRIVATE_DATA->current_speed = speed;
	return true;
}

static bool starbook_pulse(indigo_device *device, int direction, int duration) {
	char path[96];
	snprintf(path, sizeof(path), "/MOVEPULSE?DIRECT=%d&DURATION=%d", direction, duration);
	return starbook_set(device, path, NULL);
}

static bool starbook_open(indigo_device *device) {
	const char *address = DEVICE_PORT_ITEM->text.value;
	if (!strncmp(address, "http://", 7)) address += 7;
	if (snprintf(PRIVATE_DATA->endpoint, sizeof(PRIVATE_DATA->endpoint), "tcp://%s", address) >= (int)sizeof(PRIVATE_DATA->endpoint) || snprintf(PRIVATE_DATA->host_header, sizeof(PRIVATE_DATA->host_header), "%s", address) >= (int)sizeof(PRIVATE_DATA->host_header)) return false;
	PRIVATE_DATA->current_speed = -1;
	PRIVATE_DATA->move_north = PRIVATE_DATA->move_south = PRIVATE_DATA->move_east = PRIVATE_DATA->move_west = false;
	return starbook_get_version(device, &PRIVATE_DATA->version);
}

static void starbook_close(indigo_device *device) {
	(void)device;
}

static bool starbook_update_position(indigo_device *device) {
	int going, state;
	double ra, dec;
	if (!starbook_get_status(device, &ra, &dec, &going, &state)) return false;
	PRIVATE_DATA->current_ra = ra;
	PRIVATE_DATA->current_dec = dec;
	PRIVATE_DATA->current_state = state;
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	return true;
}

static void mount_goto_finalizer(indigo_device *device) {
	int going, state;
	double ra, dec;
	if (!IS_CONNECTED || indigo_monotonic_time() > PRIVATE_DATA->goto_deadline || !starbook_get_status(device, &ra, &dec, &going, &state)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		if (IS_CONNECTED) starbook_set(device, "/STOP", NULL);
	} else {
		PRIVATE_DATA->current_ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		PRIVATE_DATA->current_dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		PRIVATE_DATA->current_state = state;
		if (going) {
			indigo_update_coordinates(device, NULL);
			indigo_execute_handler_in(device, 0.25, mount_goto_finalizer);
			return;
		}
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_coordinates(device, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	bool ok = starbook_update_position(device);
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_coordinates(device, NULL);
	if (PRIVATE_DATA->version > 2.7) {
		int tracking;
		MOUNT_TRACKING_PROPERTY->state = starbook_get_track_status(device, &tracking) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (MOUNT_TRACKING_PROPERTY->state == INDIGO_OK_STATE) indigo_set_switch(MOUNT_TRACKING_PROPERTY, tracking == STARBOOK_TRACK_STATE_STOP ? MOUNT_TRACKING_OFF_ITEM : MOUNT_TRACKING_ON_ITEM, true);
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		int side;
		MOUNT_SIDE_OF_PIER_PROPERTY->state = starbook_get_pierside(device, &side) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		if (MOUNT_SIDE_OF_PIER_PROPERTY->state == INDIGO_OK_STATE) indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, side == STARBOOK_PIERSIDE_EAST ? MOUNT_SIDE_OF_PIER_EAST_ITEM : MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
	}
	time_t utc;
	int offset;
	if (starbook_get_utc(device, &utc, &offset)) {
		indigo_timetoisogm(utc, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", offset);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	} else MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	indigo_execute_handler_in(device, 0.5, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = starbook_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "Vixen");
			INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->version <= 2.7 ? "StarBook" : "StarBook TEN");
			snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "v%.2f", PRIVATE_DATA->version);
			MOUNT_TRACKING_PROPERTY->hidden = PRIVATE_DATA->version <= 2.7;
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = PRIVATE_DATA->version <= 2.7;
			connection_result = starbook_update_position(device);
			if (connection_result) {
				double longitude, latitude;
				int timezone;
				connection_result = starbook_get_place(device, &longitude, &latitude, &timezone);
				if (connection_result) {
					MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
					TIMEZONE_VALUE_ITEM->number.value = timezone;
				}
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, TIMEZONE_PROPERTY, NULL);
			indigo_define_property(device, RESET_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				starbook_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		indigo_cancel_pending_handlers(device);
		starbook_move(device, false, false, false, false);
		starbook_set(device, "/STOP", NULL);
		//- mount.on_disconnect
		indigo_delete_property(device, TIMEZONE_PROPERTY, NULL);
		indigo_delete_property(device, RESET_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			starbook_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, mount_timer_callback);
	}
}

static void mount_timezone_handler(indigo_device *device) {
	//+ mount.TIMEZONE.on_change
	TIMEZONE_PROPERTY->state = starbook_set_place(device, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, (int)TIMEZONE_VALUE_ITEM->number.value) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.TIMEZONE.on_change
	indigo_update_property(device, TIMEZONE_PROPERTY, NULL);
}

static void mount_reset_handler(indigo_device *device) {
	RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.RESET.on_change
	bool ok = !RESET_CTRL_ITEM->sw.value || starbook_set(device, "/RESET", NULL);
	RESET_CTRL_ITEM->sw.value = false;
	RESET_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.RESET.on_change
	indigo_update_property(device, RESET_PROPERTY, NULL);
}

static void mount_set_host_time_handler(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SET_HOST_TIME.on_change
	bool ok = true;
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		time_t utc = time(NULL);
		ok = (PRIVATE_DATA->version > 2.7 || PRIVATE_DATA->current_state == STARBOOK_STATE_INIT) && starbook_set_utc(device, utc, indigo_get_utc_offset());
	}
	MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
	MOUNT_SET_HOST_TIME_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_SET_HOST_TIME.on_change
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_handler(indigo_device *device) {
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_UTC_TIME.on_change
	time_t utc = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	int offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
	MOUNT_UTC_TIME_PROPERTY->state = utc != (time_t)-1 && (PRIVATE_DATA->version > 2.7 || PRIVATE_DATA->current_state == STARBOOK_STATE_INIT) && starbook_set_utc(device, utc, offset) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_UTC_TIME.on_change
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK.on_change
	bool ok = !MOUNT_PARK_PARKED_ITEM->sw.value || starbook_set(device, "/STOP", NULL);
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_PARK_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	bool settable = PRIVATE_DATA->version > 2.7 || PRIVATE_DATA->current_state == STARBOOK_STATE_INIT;
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = settable && starbook_set_place(device, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, (int)TIMEZONE_VALUE_ITEM->number.value) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
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
	if (PRIVATE_DATA->current_state == STARBOOK_STATE_INIT && !starbook_start(device)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
	} else {
		int error = STARBOOK_ERROR_NONE;
		bool sync = MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value;
		bool ok = starbook_set_coordinates(device, sync, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &error);
		if (!sync && !ok && error == STARBOOK_WARNING_NEAR_SUN) ok = starbook_set_coordinates(device, false, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &error);
		if (sync || !ok) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, starbook_error_text(error));
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->goto_deadline = indigo_monotonic_time() + 600;
			indigo_update_coordinates(device, NULL);
			indigo_execute_handler_in(device, 0.1, mount_goto_finalizer);
		}
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, mount_goto_finalizer);
	bool ok = starbook_set(device, "/STOP", NULL) && starbook_move(device, false, false, false, false);
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = MOUNT_MOTION_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_update_coordinates(device, ok ? "Aborted" : "Abort failed");
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
	if (PRIVATE_DATA->current_state == STARBOOK_STATE_INIT && !starbook_start(device)) MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	else MOUNT_MOTION_DEC_PROPERTY->state = starbook_move(device, MOUNT_MOTION_NORTH_ITEM->sw.value, MOUNT_MOTION_SOUTH_ITEM->sw.value, MOUNT_MOTION_EAST_ITEM->sw.value, MOUNT_MOTION_WEST_ITEM->sw.value) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
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
	if (PRIVATE_DATA->current_state == STARBOOK_STATE_INIT && !starbook_start(device)) MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	else MOUNT_MOTION_RA_PROPERTY->state = starbook_move(device, MOUNT_MOTION_NORTH_ITEM->sw.value, MOUNT_MOTION_SOUTH_ITEM->sw.value, MOUNT_MOTION_EAST_ITEM->sw.value, MOUNT_MOTION_WEST_ITEM->sw.value) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_slew_rate_handler(indigo_device *device) {
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SLEW_RATE.on_change
	int speed = MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value ? 0 : MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value ? 3 : MOUNT_SLEW_RATE_FIND_ITEM->sw.value ? 5 : 8;
	MOUNT_SLEW_RATE_PROPERTY->state = starbook_set_speed(device, speed) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_SLEW_RATE.on_change
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ mount.on_attach
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		MOUNT_TRACKING_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_EPOCH_PROPERTY->perm = INDIGO_RO_PERM;
		DEVICE_PORT_PROPERTY->hidden = false;
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, STARBOOK_DEFAULT_ADDRESS);
		//- mount.on_attach
		TIMEZONE_PROPERTY = indigo_init_number_property(NULL, device->name, TIMEZONE_PROPERTY_NAME, MOUNT_SITE_GROUP, "Timezone", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (TIMEZONE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(TIMEZONE_VALUE_ITEM, TIMEZONE_VALUE_ITEM_NAME, "Timezone", -12, 12, 1, 0);
		RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, RESET_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(RESET_CTRL_ITEM, RESET_CTRL_ITEM_NAME, "Reset", false);
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(TIMEZONE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(RESET_PROPERTY);
	}
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
	} else if (indigo_property_match_changeable(TIMEZONE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(TIMEZONE_PROPERTY, mount_timezone_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(RESET_PROPERTY, mount_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_EQUATORIAL_COORDINATES_PROPERTY->count; i++) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items[i].do_update = true;
			}
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_MOTION_DEC_PROPERTY->count; i++) {
				MOUNT_MOTION_DEC_PROPERTY->items[i].do_update = true;
			}
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_MOTION_RA_PROPERTY->count; i++) {
				MOUNT_MOTION_RA_PROPERTY->items[i].do_update = true;
			}
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SLEW_RATE_PROPERTY, mount_slew_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, DEVICE_PORT_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(TIMEZONE_PROPERTY);
	indigo_release_property(RESET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = starbook_open(device->master_device);
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
				starbook_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			starbook_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	int duration = 0, direction = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		direction = 0;
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		direction = 1;
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	}
	bool ok = duration == 0 || starbook_pulse(device, direction, duration);
	if (ok && duration > 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_dec_finalizer);
	} else {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	int duration = 0, direction = 2;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		direction = 2;
		duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		direction = 3;
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
	}
	bool ok = duration == 0 || starbook_pulse(device, direction, duration);
	if (ok && duration > 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_ra_finalizer);
	} else {
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
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
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, guider_connection_handler);
		}
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

indigo_result indigo_mount_starbook(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static starbook_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (starbook_private_data *)indigo_safe_malloc(sizeof(starbook_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
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
