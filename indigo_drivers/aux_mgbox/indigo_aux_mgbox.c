// Copyright (c) 2020-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_mgbox.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <math.h>
#include <ctype.h>
#include <errno.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_gps_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_mgbox.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000A
#define DRIVER_NAME          "indigo_aux_mgbox"
#define DRIVER_LABEL         "Astromi.ch MGBox"
#define AUX_DEVICE_NAME      "MGBox Weather"
#define GPS_DEVICE_NAME      "MGBox GPS"
#define PRIVATE_DATA         ((mgbox_private_data *)device->private_data)

//+ define

#define SETTINGS_GROUP       "Settings"
#define WEATHER_GROUP        "Weather"
#define SWITCH_GROUP         "Switch Control"

//- define

#pragma mark - Property definitions

#define AUX_GPIO_OUTLET_PROPERTY       (PRIVATE_DATA->aux_gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM         (AUX_GPIO_OUTLET_PROPERTY->items + 0)

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY (PRIVATE_DATA->aux_outlet_pulse_lengths_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)

#define AUX_DEW_THRESHOLD_PROPERTY      (PRIVATE_DATA->aux_dew_threshold_property)
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM (AUX_DEW_THRESHOLD_PROPERTY->items + 0)

#define AUX_DEW_WARNING_PROPERTY       (PRIVATE_DATA->aux_dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM  (AUX_DEW_WARNING_PROPERTY->items + 0)

#define X_CALIBRATION_PROPERTY         (PRIVATE_DATA->x_calibration_property)
#define X_CALIBRATION_TEMPERATURE_ITEM (X_CALIBRATION_PROPERTY->items + 0)
#define X_CALIBRATION_HUMIDIDTY_ITEM   (X_CALIBRATION_PROPERTY->items + 1)
#define X_CALIBRATION_PRESSURE_ITEM    (X_CALIBRATION_PROPERTY->items + 2)

#define X_CALIBRATION_PROPERTY_NAME    "X_WEATHER_CALIBRATION"

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_PRESSURE_ITEM      (AUX_WEATHER_PROPERTY->items + 3)

#define X_SEND_WEATHER_MOUNT_PROPERTY      (PRIVATE_DATA->x_send_weather_mount_property)
#define X_SEND_WEATHER_MOUNT_ITEM          (X_SEND_WEATHER_MOUNT_PROPERTY->items + 0)

#define X_SEND_WEATHER_MOUNT_PROPERTY_NAME "X_SEND_WEATHER_DATA_TO_MOUNT"
#define X_SEND_WEATHER_MOUNT_ITEM_NAME     "ENABLED"

#define X_REBOOT_PROPERTY              (PRIVATE_DATA->x_reboot_property)
#define X_REBOOT_ITEM                  (X_REBOOT_PROPERTY->items + 0)

#define X_REBOOT_PROPERTY_NAME         "X_REBOOT_DEVICE"
#define X_REBOOT_ITEM_NAME             "REBOOT"

#define X_SEND_GPS_MOUNT_PROPERTY      (PRIVATE_DATA->x_send_gps_mount_property)
#define X_SEND_GPS_MOUNT_ITEM          (X_SEND_GPS_MOUNT_PROPERTY->items + 0)

#define X_SEND_GPS_MOUNT_PROPERTY_NAME "X_SEND_GPS_DATA_TO_MOUNT"
#define X_SEND_GPS_MOUNT_ITEM_NAME     "ENABLED"

#define X_REBOOT_GPS_PROPERTY          (PRIVATE_DATA->x_reboot_gps_property)
#define X_REBOOT_GPS_ITEM              (X_REBOOT_GPS_PROPERTY->items + 0)

#define X_REBOOT_GPS_PROPERTY_NAME     "X_REBOOT_GPS"
#define X_REBOOT_GPS_ITEM_NAME         "REBOOT"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *aux_gpio_outlet_property;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_outlet_pulse_lengths_property;
	indigo_property *aux_dew_threshold_property;
	indigo_property *aux_dew_warning_property;
	indigo_property *x_calibration_property;
	indigo_property *aux_weather_property;
	indigo_property *x_send_weather_mount_property;
	indigo_property *x_reboot_property;
	indigo_property *x_send_gps_mount_property;
	indigo_property *x_reboot_gps_property;
	//+ data
	indigo_device *aux_device;
	indigo_device *gps_device;
	char firmware[INDIGO_VALUE_SIZE];
	char device_type[INDIGO_VALUE_SIZE];
	char input[512];
	size_t input_length;
	bool input_overflow;
	bool transport_failed;
	int gps_fix;
	bool position_valid, time_valid;
	bool calibration_pending;
	bool weather_forwarding_pending, gps_forwarding_pending;
	bool weather_forwarding, gps_forwarding;
	bool rebooting, gps_rebooting;
	unsigned cal_sequence, weather_sequence, gps_sequence;
	unsigned cal_after, weather_after, gps_after;
	int cal_target[3];
	int cal_attempts, weather_attempts, gps_attempts;
	//- data
} mgbox_private_data;

#pragma mark - Low level code

//+ code

static bool mgbox_number(const char *text, double min, double max, double *value) {
	if (!text || !*text) {
		return false;
	}
	char *end;
	errno = 0;
	*value = strtod(text, &end);
	return !errno && *end == 0 && isfinite(*value) && *value >= min && *value <= max;
}

static int mgbox_parse(char *buffer, char **tokens, int capacity) {
	int offset;
	if (!strncmp(buffer, "$GP", 3)) {
		offset = 3;
	} else if (!strncmp(buffer, "$P", 2)) {
		offset = 2;
	} else if (!strncmp(buffer, "$LOG:", 5)) {
		offset = 1;
	} else {
		return 0;
	}
	char *checksum = strchr(buffer, '*');
	if (checksum) {
		if (strlen(checksum + 1) != 2 || !isxdigit((unsigned char)checksum[1]) || !isxdigit((unsigned char)checksum[2])) {
			return 0;
		}
		unsigned long expected = strtoul(checksum + 1, NULL, 16), actual = 0;
		for (char *p = buffer + 1; p < checksum; p++) {
			actual ^= (unsigned char)*p;
		}
		if (actual != expected) {
			return 0;
		}
		*checksum = 0;
	}
	int count = 0;
	char *next = buffer + offset;
	while (next) {
		if (count == capacity) {
			return 0;
		}
		tokens[count++] = next;
		next = strchr(next, ',');
		if (next) {
			*next++ = 0;
		}
	}
	return count;
}

static void mgbox_update(indigo_device *device, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_update_property(device, property, NULL);
	}
}

static void mgbox_gps_status(indigo_device *device, int fix) {
	PRIVATE_DATA->gps_fix = fix;
	GPS_STATUS_NO_FIX_ITEM->light.value = fix == 1 ? INDIGO_ALERT_STATE : INDIGO_IDLE_STATE;
	GPS_STATUS_2D_FIX_ITEM->light.value = fix == 2 ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
	GPS_STATUS_3D_FIX_ITEM->light.value = fix == 3 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
	if (fix == 1) {
		PRIVATE_DATA->position_valid = PRIVATE_DATA->time_valid = false;
	}
	GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = fix == 3 && PRIVATE_DATA->position_valid ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	GPS_UTC_TIME_PROPERTY->state = fix == 3 && PRIVATE_DATA->time_valid ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	mgbox_update(device, GPS_STATUS_PROPERTY);
	mgbox_update(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
	mgbox_update(device, GPS_UTC_TIME_PROPERTY);
}

static bool mgbox_coordinate(const char *text, const char *hemisphere, bool latitude, double *value) {
	double coordinate;
	if (!mgbox_number(text, 0, latitude ? 9000 : 18000, &coordinate) || strlen(hemisphere) != 1 || !strchr(latitude ? "NS" : "EW", *hemisphere) || fmod(coordinate, 100) >= 60) {
		return false;
	}
	*value = round((floor(coordinate / 100) + fmod(coordinate, 100) / 60) * 10000) / 10000;
	if (*hemisphere == 'S' || *hemisphere == 'W') {
		*value = -*value;
	}
	return true;
}

static void mgbox_process_line(indigo_device *device, char *buffer) {
	char *tokens[64];
	int count = mgbox_parse(buffer, tokens, 64);
	if (!count) {
		return;
	}
	device = PRIVATE_DATA->aux_device;
	if (!strncmp(tokens[0], "LOG: Device Type: ", 18)) {
		const char *model = tokens[0] + 18;
		if (!strcmp(model, "MBox") || !strcmp(model, "MGBox") || !strcmp(model, "MGPBox") || !strcmp(model, "PBox")) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->device_type, model);
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, model);
			mgbox_update(device, INFO_PROPERTY);
			device = PRIVATE_DATA->gps_device;
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, model);
			mgbox_update(device, INFO_PROPERTY);
		}
		return;
	}
	if (!strncmp(tokens[0], "LOG:", 4)) {
		if (IS_CONNECTED) {
			indigo_send_message(device, IDLE_PROPERTY, "%s", tokens[0] + 4);
		}
		return;
	}
	if (!strcmp(tokens[0], "XDR")) {
		double pressure, temperature, humidity, dewpoint;
		if (count < 18 || strcmp(tokens[1], "P") || strcmp(tokens[5], "C") || strcmp(tokens[9], "H") || strcmp(tokens[13], "C") || !*tokens[17] || !mgbox_number(tokens[2], 0, 1000000, &pressure) || !mgbox_number(tokens[6], -200, 80, &temperature) || !mgbox_number(tokens[10], 0, 100, &humidity) || !mgbox_number(tokens[14], -200, 80, &dewpoint)) {
			return;
		}
		AUX_WEATHER_PRESSURE_ITEM->number.value = pressure / 100;
		AUX_WEATHER_TEMPERATURE_ITEM->number.value = temperature;
		AUX_WEATHER_HUMIDITY_ITEM->number.value = humidity;
		AUX_WEATHER_DEWPOINT_ITEM->number.value = dewpoint;
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		mgbox_update(device, AUX_WEATHER_PROPERTY);
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = temperature - AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value <= dewpoint ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		mgbox_update(device, AUX_DEW_WARNING_PROPERTY);
		if (strcmp(PRIVATE_DATA->firmware, tokens[17])) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->firmware, tokens[17]);
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, tokens[17]);
			mgbox_update(device, INFO_PROPERTY);
			device = PRIVATE_DATA->gps_device;
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, tokens[17]);
			mgbox_update(device, INFO_PROPERTY);
		}
		return;
	}
	if (!strcmp(tokens[0], "CAL")) {
		double pressure, temperature, humidity, forwarding;
		if (count < 7 || count % 2 != 1 || strcmp(tokens[1], "P") || strcmp(tokens[3], "T") || strcmp(tokens[5], "H") || !mgbox_number(tokens[2], -9990, 9990, &pressure) || !mgbox_number(tokens[4], -2000, 2000, &temperature) || !mgbox_number(tokens[6], -990, 990, &humidity)) {
			return;
		}
		for (int i = 7; i + 1 < count; i += 2) {
			if ((!strcmp(tokens[i], "MM") || !strcmp(tokens[i], "MG")) && (!mgbox_number(tokens[i + 1], 0, 1, &forwarding) || floor(forwarding) != forwarding)) {
				return;
			}
		}
		X_CALIBRATION_PRESSURE_ITEM->number.value = pressure / 10;
		X_CALIBRATION_TEMPERATURE_ITEM->number.value = temperature / 10;
		X_CALIBRATION_HUMIDIDTY_ITEM->number.value = humidity / 10;
		PRIVATE_DATA->cal_sequence++;
		if (!PRIVATE_DATA->calibration_pending && X_CALIBRATION_PROPERTY->state != INDIGO_BUSY_STATE) {
			X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		}
		mgbox_update(device, X_CALIBRATION_PROPERTY);
		for (int i = 7; i + 1 < count; i += 2) {
			if (!strcmp(tokens[i], "MM")) {
				PRIVATE_DATA->weather_forwarding = atoi(tokens[i + 1]);
				PRIVATE_DATA->weather_sequence++;
				if (!PRIVATE_DATA->weather_forwarding_pending && X_SEND_WEATHER_MOUNT_PROPERTY->state != INDIGO_BUSY_STATE) {
					X_SEND_WEATHER_MOUNT_ITEM->sw.value = PRIVATE_DATA->weather_forwarding;
					X_SEND_WEATHER_MOUNT_PROPERTY->state = INDIGO_OK_STATE;
					mgbox_update(device, X_SEND_WEATHER_MOUNT_PROPERTY);
				}
			} else if (!strcmp(tokens[i], "MG")) {
				device = PRIVATE_DATA->gps_device;
				PRIVATE_DATA->gps_forwarding = atoi(tokens[i + 1]);
				PRIVATE_DATA->gps_sequence++;
				if (!PRIVATE_DATA->gps_forwarding_pending && X_SEND_GPS_MOUNT_PROPERTY->state != INDIGO_BUSY_STATE) {
					X_SEND_GPS_MOUNT_ITEM->sw.value = PRIVATE_DATA->gps_forwarding;
					X_SEND_GPS_MOUNT_PROPERTY->state = INDIGO_OK_STATE;
					mgbox_update(device, X_SEND_GPS_MOUNT_PROPERTY);
				}
				device = PRIVATE_DATA->aux_device;
			}
		}
		return;
	}
	device = PRIVATE_DATA->gps_device;
	if (!strcmp(tokens[0], "RMC")) {
		if (count >= 3 && !strcmp(tokens[2], "V")) {
			mgbox_gps_status(device, 1);
			return;
		}
		double lat, lon, utc, date_value;
		if (count < 10 || strcmp(tokens[2], "A") || !mgbox_coordinate(tokens[3], tokens[4], true, &lat) || !mgbox_coordinate(tokens[5], tokens[6], false, &lon) || !mgbox_number(tokens[1], 0, 235959.999, &utc) || !mgbox_number(tokens[9], 10100, 311299, &date_value) || floor(date_value) != date_value) {
			return;
		}
		int time = (int)utc, date = (int)date_value, month = date / 100 % 100, day = date / 10000;
		int year = 2000 + date % 100;
		int days[] = { 31, 28 + (year % 4 == 0), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		if (time / 100 % 100 >= 60 || time % 100 >= 60 || month < 1 || month > 12 || day < 1 || day > days[month - 1]) {
			return;
		}
		snprintf(GPS_UTC_ITEM->text.value, INDIGO_VALUE_SIZE, "%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, time / 10000, time / 100 % 100, time % 100);
		GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
		GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
		PRIVATE_DATA->position_valid = PRIVATE_DATA->time_valid = true;
		mgbox_gps_status(device, PRIVATE_DATA->gps_fix >= 2 ? PRIVATE_DATA->gps_fix : 3);
	} else if (!strcmp(tokens[0], "GGA")) {
		double quality, lat, lon, elevation, satellites;
		if (count < 7 || !mgbox_number(tokens[6], 0, 8, &quality) || floor(quality) != quality) {
			return;
		}
		if (quality == 0) {
			mgbox_gps_status(device, 1);
			return;
		}
		if (count < 11 || strcmp(tokens[10], "M") || !mgbox_coordinate(tokens[2], tokens[3], true, &lat) || !mgbox_coordinate(tokens[4], tokens[5], false, &lon) || !mgbox_number(tokens[9], -1000, 100000, &elevation) || !mgbox_number(tokens[7], 0, 99, &satellites) || floor(satellites) != satellites) {
			return;
		}
		GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
		GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
		GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = round(elevation);
		GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = satellites;
		GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->position_valid = true;
		mgbox_gps_status(device, PRIVATE_DATA->gps_fix >= 2 ? PRIVATE_DATA->gps_fix : 3);
	} else if (!strcmp(tokens[0], "GSA")) {
		double fix, pdop, hdop, vdop;
		if (count < 3 || !mgbox_number(tokens[2], 1, 3, &fix) || floor(fix) != fix) {
			return;
		}
		if (fix == 1) {
			mgbox_gps_status(device, 1);
			return;
		}
		if (count < 18 || !mgbox_number(tokens[15], 0, 999, &pdop) || !mgbox_number(tokens[16], 0, 999, &hdop) || !mgbox_number(tokens[17], 0, 999, &vdop)) {
			return;
		}
		GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = pdop;
		GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = hdop;
		GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = vdop;
		GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
		mgbox_gps_status(device, (int)fix);
	} else if (!strcmp(tokens[0], "GSV")) {
		double satellites;
		if (count < 4 || !mgbox_number(tokens[3], 0, 99, &satellites) || floor(satellites) != satellites) {
			return;
		}
		GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = satellites;
		GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
		mgbox_update(device, GPS_ADVANCED_STATUS_PROPERTY);
	}
}

static bool mgbox_read(indigo_device *device, double timeout) {
	int ready = indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(timeout));
	if (ready <= 0) {
		return ready == 0;
	}
	char input[512];
	long size = indigo_uni_read_available(PRIVATE_DATA->handle, input, sizeof(input));
	if (size <= 0) {
		return false;
	}
	for (long i = 0; i < size; i++) {
		char c = input[i];
		if (c == '$') {
			PRIVATE_DATA->input_length = 0;
			PRIVATE_DATA->input_overflow = false;
		}
		if (c == '\n') {
			if (!PRIVATE_DATA->input_overflow && PRIVATE_DATA->input_length) {
				PRIVATE_DATA->input[PRIVATE_DATA->input_length] = 0;
				mgbox_process_line(device, PRIVATE_DATA->input);
			}
			PRIVATE_DATA->input_length = 0;
			PRIVATE_DATA->input_overflow = false;
		} else if (c != '\r') {
			if (!c || PRIVATE_DATA->input_length + 1 >= sizeof(PRIVATE_DATA->input)) {
				PRIVATE_DATA->input_overflow = true;
			} else if (!PRIVATE_DATA->input_overflow) {
				PRIVATE_DATA->input[PRIVATE_DATA->input_length++] = c;
			}
		}
	}
	return true;
}

static bool mgbox_command(indigo_device *device, const char *command) {
	if (!PRIVATE_DATA->handle || PRIVATE_DATA->transport_failed) {
		return false;
	}
	// The device requires at least half a second between commands.
	indigo_usleep(500000);
	return indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) == (long)strlen(command);
}

static bool mgbox_open(indigo_device *device) {
	PRIVATE_DATA->input_length = 0;
	PRIVATE_DATA->input_overflow = false;
	PRIVATE_DATA->transport_failed = false;
	PRIVATE_DATA->firmware[0] = PRIVATE_DATA->device_type[0] = 0;
	PRIVATE_DATA->gps_fix = 1;
	PRIVATE_DATA->position_valid = PRIVATE_DATA->time_valid = false;
	PRIVATE_DATA->calibration_pending = false;
	PRIVATE_DATA->weather_forwarding_pending = PRIVATE_DATA->gps_forwarding_pending = false;
	PRIVATE_DATA->rebooting = PRIVATE_DATA->gps_rebooting = false;
	PRIVATE_DATA->cal_sequence = PRIVATE_DATA->weather_sequence = PRIVATE_DATA->gps_sequence = 0;
	if (indigo_uni_is_url(DEVICE_PORT_ITEM->text.value, "mgbox")) {
		PRIVATE_DATA->handle = indigo_uni_client_tcp_socket(DEVICE_PORT_ITEM->text.value, 9999, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle) {
			indigo_uni_set_socket_write_timeout(PRIVATE_DATA->handle, INDIGO_DELAY(2));
		}
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	}
	if (!PRIVATE_DATA->handle) {
		return false;
	}
	indigo_usleep(1000000);
	for (int retry = 0; retry < 3; retry++) {
		if (!mgbox_command(device, ":devicetype*")) {
			break;
		}
		for (int i = 0; i < 25; i++) {
			if (!mgbox_read(device, 0.1)) {
				PRIVATE_DATA->transport_failed = true;
				break;
			}
			if (*PRIVATE_DATA->device_type) {
				return true;
			}
		}
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	PRIVATE_DATA->input_length = 0;
	PRIVATE_DATA->firmware[0] = PRIVATE_DATA->device_type[0] = 0;
	return false;
}

static void mgbox_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	PRIVATE_DATA->input_length = 0;
	PRIVATE_DATA->input_overflow = false;
	PRIVATE_DATA->firmware[0] = PRIVATE_DATA->device_type[0] = 0;
}

static void mgbox_poll(indigo_device *device) {
	if (!PRIVATE_DATA->transport_failed && !mgbox_read(device, 0)) {
		PRIVATE_DATA->transport_failed = true;
		device = PRIVATE_DATA->aux_device;
		AUX_WEATHER_PROPERTY->state = AUX_DEW_WARNING_PROPERTY->state = INDIGO_ALERT_STATE;
		mgbox_update(device, AUX_WEATHER_PROPERTY);
		mgbox_update(device, AUX_DEW_WARNING_PROPERTY);
		device = PRIVATE_DATA->gps_device;
		GPS_STATUS_PROPERTY->state = GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		mgbox_update(device, GPS_STATUS_PROPERTY);
		mgbox_update(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
		mgbox_update(device, GPS_UTC_TIME_PROPERTY);
	}
}

static void pulse_finalizer(indigo_device *device) {
	AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
	AUX_GPIO_OUTLET_PROPERTY->state = PRIVATE_DATA->transport_failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
}

static void calibration_finalizer(indigo_device *device) {
	bool confirmed = PRIVATE_DATA->cal_sequence > PRIVATE_DATA->cal_after;
	for (int i = 0; i < 3; i++) {
		confirmed = confirmed && fabs(X_CALIBRATION_PROPERTY->items[i].number.value * 10 - PRIVATE_DATA->cal_target[i]) < 0.01;
	}
	if (!confirmed && !PRIVATE_DATA->transport_failed && --PRIVATE_DATA->cal_attempts > 0) {
		indigo_execute_handler_in(device, 0.1, calibration_finalizer);
		return;
	}
	PRIVATE_DATA->calibration_pending = false;
	X_CALIBRATION_PROPERTY->state = confirmed && !PRIVATE_DATA->transport_failed ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
}

static void weather_forwarding_finalizer(indigo_device *device) {
	bool confirmed = PRIVATE_DATA->weather_sequence > PRIVATE_DATA->weather_after && X_SEND_WEATHER_MOUNT_ITEM->sw.value == PRIVATE_DATA->weather_forwarding;
	if (!confirmed && !PRIVATE_DATA->transport_failed && --PRIVATE_DATA->weather_attempts > 0) {
		indigo_execute_handler_in(device, 0.1, weather_forwarding_finalizer);
		return;
	}
	PRIVATE_DATA->weather_forwarding_pending = false;
	X_SEND_WEATHER_MOUNT_ITEM->sw.value = PRIVATE_DATA->weather_forwarding;
	X_SEND_WEATHER_MOUNT_PROPERTY->state = confirmed && !PRIVATE_DATA->transport_failed ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
}

static void gps_forwarding_finalizer(indigo_device *device) {
	bool confirmed = PRIVATE_DATA->gps_sequence > PRIVATE_DATA->gps_after && X_SEND_GPS_MOUNT_ITEM->sw.value == PRIVATE_DATA->gps_forwarding;
	if (!confirmed && !PRIVATE_DATA->transport_failed && --PRIVATE_DATA->gps_attempts > 0) {
		indigo_execute_handler_in(device, 0.1, gps_forwarding_finalizer);
		return;
	}
	PRIVATE_DATA->gps_forwarding_pending = false;
	X_SEND_GPS_MOUNT_ITEM->sw.value = PRIVATE_DATA->gps_forwarding;
	X_SEND_GPS_MOUNT_PROPERTY->state = confirmed && !PRIVATE_DATA->transport_failed ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
}

static void reboot_finalizer(indigo_device *device) {
	PRIVATE_DATA->rebooting = false;
	X_REBOOT_ITEM->sw.value = false;
	X_REBOOT_PROPERTY->state = PRIVATE_DATA->transport_failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, X_REBOOT_PROPERTY, NULL);
}

static void gps_reboot_finalizer(indigo_device *device) {
	PRIVATE_DATA->gps_rebooting = false;
	X_REBOOT_GPS_ITEM->sw.value = false;
	X_REBOOT_GPS_PROPERTY->state = PRIVATE_DATA->transport_failed ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, X_REBOOT_GPS_PROPERTY, NULL);
}

static void mgbox_reset_gps(indigo_device *device) {
	PRIVATE_DATA->gps_fix = 1;
	PRIVATE_DATA->position_valid = PRIVATE_DATA->time_valid = false;
	GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = GPS_UTC_TIME_PROPERTY->state = GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
	GPS_STATUS_NO_FIX_ITEM->light.value = GPS_STATUS_2D_FIX_ITEM->light.value = GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	INDIGO_COPY_VALUE(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00");
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	mgbox_poll(device);
	if (!PRIVATE_DATA->transport_failed) {
		indigo_execute_handler_in(device, 0.1, aux_timer_callback);
	}
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = mgbox_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			AUX_WEATHER_PROPERTY->state = AUX_DEW_WARNING_PROPERTY->state = INDIGO_BUSY_STATE;
			X_CALIBRATION_PROPERTY->state = X_SEND_WEATHER_MOUNT_PROPERTY->state = X_REBOOT_PROPERTY->state = AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
			if (strchr(PRIVATE_DATA->device_type, 'M')) {
				connection_result = mgbox_command(device, ":calget*");
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATION_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
			indigo_define_property(device, X_REBOOT_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				mgbox_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		PRIVATE_DATA->calibration_pending = PRIVATE_DATA->weather_forwarding_pending = PRIVATE_DATA->rebooting = false;
		X_CALIBRATION_PROPERTY->state = X_SEND_WEATHER_MOUNT_PROPERTY->state = X_REBOOT_PROPERTY->state = AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		AUX_GPIO_OUTLET_1_ITEM->sw.value = X_REBOOT_ITEM->sw.value = false;
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
		indigo_delete_property(device, X_REBOOT_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			mgbox_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_gpio_outlet_handler(indigo_device *device) {
	//+ aux.AUX_GPIO_OUTLET.on_change
	if (!IS_CONNECTED || !strchr(PRIVATE_DATA->device_type, 'P') || PRIVATE_DATA->rebooting) {
		AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
		AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, "Pulse unavailable for this model or during reboot");
		return;
	}
	if (AUX_GPIO_OUTLET_1_ITEM->sw.value) {
		char command[64];
		snprintf(command, sizeof(command), ":pulse,%d*", (int)AUX_OUTLET_PULSE_LENGTHS_1_ITEM->number.target);
		if (!mgbox_command(device, command)) {
			AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			return;
		}
		indigo_execute_handler_in(device, AUX_OUTLET_PULSE_LENGTHS_1_ITEM->number.target / 1000, pulse_finalizer);
	} else {
		pulse_finalizer(device);
	}
	//- aux.AUX_GPIO_OUTLET.on_change
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
	}
	snprintf(AUX_GPIO_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_x_calibration_handler(indigo_device *device) {
	//+ aux.X_CALIBRATION.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->rebooting || !strchr(PRIVATE_DATA->device_type, 'M')) {
		X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->calibration_pending = true;
	PRIVATE_DATA->cal_after = PRIVATE_DATA->cal_sequence;
	PRIVATE_DATA->cal_attempts = 50;
	const char *commands[] = { ":calt,%d*", ":calh,%d*", ":calp,%d*" };
	for (int i = 0; i < 3; i++) {
		PRIVATE_DATA->cal_target[i] = (int)(X_CALIBRATION_PROPERTY->items[i].number.target * 10);
	}
	for (int i = 0; i < 3; i++) {
		char command[64];
		snprintf(command, sizeof(command), commands[i], PRIVATE_DATA->cal_target[i]);
		if (!mgbox_command(device, command)) {
			PRIVATE_DATA->calibration_pending = false;
			X_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
			return;
		}
	}
	indigo_execute_handler_in(device, 0.1, calibration_finalizer);
	//- aux.X_CALIBRATION.on_change
}

static void aux_x_send_weather_mount_handler(indigo_device *device) {
	//+ aux.X_SEND_WEATHER_MOUNT.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->rebooting || !strchr(PRIVATE_DATA->device_type, 'M')) {
		X_SEND_WEATHER_MOUNT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->weather_after = PRIVATE_DATA->weather_sequence;
	PRIVATE_DATA->weather_attempts = 50;
	PRIVATE_DATA->weather_forwarding_pending = true;
	if (mgbox_command(device, X_SEND_WEATHER_MOUNT_ITEM->sw.value ? ":mm,1*" : ":mm,0*")) {
		indigo_execute_handler_in(device, 0.1, weather_forwarding_finalizer);
	} else {
		PRIVATE_DATA->weather_forwarding_pending = false;
		X_SEND_WEATHER_MOUNT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
	}
	//- aux.X_SEND_WEATHER_MOUNT.on_change
}

static void aux_x_reboot_handler(indigo_device *device) {
	//+ aux.X_REBOOT.on_change
	if (!IS_CONNECTED || AUX_GPIO_OUTLET_PROPERTY->state == INDIGO_BUSY_STATE || X_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE || X_SEND_WEATHER_MOUNT_PROPERTY->state == INDIGO_BUSY_STATE || X_REBOOT_GPS_PROPERTY->state == INDIGO_BUSY_STATE || X_SEND_GPS_MOUNT_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_REBOOT_ITEM->sw.value = false;
		X_REBOOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_REBOOT_PROPERTY, "Another device operation is unfinished");
		return;
	}
	if (!X_REBOOT_ITEM->sw.value) {
		reboot_finalizer(device);
	} else if (mgbox_command(device, ":reboot*")) {
		PRIVATE_DATA->rebooting = true;
		indigo_execute_handler_in(device, 2, reboot_finalizer);
	} else {
		X_REBOOT_ITEM->sw.value = false;
		X_REBOOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_REBOOT_PROPERTY, NULL);
	}
	//- aux.X_REBOOT.on_change
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_WEATHER | INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ aux.on_attach
		PRIVATE_DATA->aux_device = device;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, "38400");
		//- aux.on_attach
		AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, SWITCH_GROUP, "Switch outlet", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AUX_GPIO_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Pulse switch", false);
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, SETTINGS_GROUP, "Switch name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Switch name", "Pulse switch");
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_OUTLET_PULSE_LENGTHS_PROPERTY_NAME, SWITCH_GROUP, "Switch pulse length (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Pulse switch", 1, 10000, 100, 1000);
		AUX_DEW_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_DEW_THRESHOLD_PROPERTY_NAME, SETTINGS_GROUP, "Dew warning threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_DEW_THRESHOLD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_1_ITEM, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME, "Temperature difference (°C)", 0, 9, 0, 2);
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_OK_STATE, 1);
		if (AUX_DEW_WARNING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning", INDIGO_IDLE_STATE);
		X_CALIBRATION_PROPERTY = indigo_init_number_property(NULL, device->name, X_CALIBRATION_PROPERTY_NAME, SETTINGS_GROUP, "Weather calibration factors", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (X_CALIBRATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_CALIBRATION_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature (°C)", -200, 200, 0, 0);
		indigo_init_number_item(X_CALIBRATION_HUMIDIDTY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Relative Humidity (%)", -99, 99, 0, 0);
		indigo_init_number_item(X_CALIBRATION_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Atmospheric Pressure (Pa)", -999, 999, 0, 0);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather conditions", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient temperature (°C)", -200, 80, 0, 0);
		strcpy(AUX_WEATHER_TEMPERATURE_ITEM->number.format, "%.1f");
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint (°C)", -200, 80, 1, 0);
		strcpy(AUX_WEATHER_DEWPOINT_ITEM->number.format, "%.1f");
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Relative humidity (%)", 0, 100, 0, 0);
		strcpy(AUX_WEATHER_HUMIDITY_ITEM->number.format, "%.1f");
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Atmospheric Pressure (hPa)", 0, 10000, 0, 0);
		strcpy(AUX_WEATHER_PRESSURE_ITEM->number.format, "%.2f");
		X_SEND_WEATHER_MOUNT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SEND_WEATHER_MOUNT_PROPERTY_NAME, SETTINGS_GROUP, "Send weather data to mount", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_SEND_WEATHER_MOUNT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SEND_WEATHER_MOUNT_ITEM, X_SEND_WEATHER_MOUNT_ITEM_NAME, "Enable", false);
		X_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REBOOT_PROPERTY_NAME, SETTINGS_GROUP, "Reboot device", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_REBOOT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_REBOOT_ITEM, X_REBOOT_ITEM_NAME, "Reboot!", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_WARNING_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SEND_WEATHER_MOUNT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_REBOOT_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_THRESHOLD_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_GPIO_OUTLET_PROPERTY, aux_gpio_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_THRESHOLD_PROPERTY, property)) {
		indigo_property_copy_values(AUX_DEW_THRESHOLD_PROPERTY, property, false);
		AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_CALIBRATION_PROPERTY, aux_x_calibration_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SEND_WEATHER_MOUNT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SEND_WEATHER_MOUNT_PROPERTY, aux_x_send_weather_mount_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_REBOOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_REBOOT_PROPERTY, aux_x_reboot_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_DEW_THRESHOLD_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_DEW_THRESHOLD_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(X_CALIBRATION_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(X_SEND_WEATHER_MOUNT_PROPERTY);
	indigo_release_property(X_REBOOT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (gps)

static void gps_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ gps.on_timer
	mgbox_poll(device);
	if (!PRIVATE_DATA->transport_failed) {
		indigo_execute_handler_in(device, 0.1, gps_timer_callback);
	}
	//- gps.on_timer
}

static void gps_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = mgbox_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ gps.on_connect
			connection_result = strchr(PRIVATE_DATA->device_type, 'G') != NULL;
			if (connection_result) {
				mgbox_reset_gps(device);
				X_SEND_GPS_MOUNT_PROPERTY->state = X_REBOOT_GPS_PROPERTY->state = INDIGO_OK_STATE;
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
				connection_result = mgbox_command(device, ":calget*");
			}
			//- gps.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
			indigo_define_property(device, X_REBOOT_GPS_PROPERTY, NULL);
			indigo_execute_handler(device, gps_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				mgbox_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ gps.on_disconnect
		PRIVATE_DATA->gps_forwarding_pending = PRIVATE_DATA->gps_rebooting = false;
		X_SEND_GPS_MOUNT_PROPERTY->state = X_REBOOT_GPS_PROPERTY->state = INDIGO_OK_STATE;
		X_REBOOT_GPS_ITEM->sw.value = false;
		//- gps.on_disconnect
		indigo_delete_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
		indigo_delete_property(device, X_REBOOT_GPS_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			mgbox_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void gps_x_send_gps_mount_handler(indigo_device *device) {
	//+ gps.X_SEND_GPS_MOUNT.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->rebooting || PRIVATE_DATA->gps_rebooting) {
		X_SEND_GPS_MOUNT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->gps_after = PRIVATE_DATA->gps_sequence;
	PRIVATE_DATA->gps_attempts = 50;
	PRIVATE_DATA->gps_forwarding_pending = true;
	if (mgbox_command(device, X_SEND_GPS_MOUNT_ITEM->sw.value ? ":mg,1*" : ":mg,0*")) {
		indigo_execute_handler_in(device, 0.1, gps_forwarding_finalizer);
	} else {
		PRIVATE_DATA->gps_forwarding_pending = false;
		X_SEND_GPS_MOUNT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
	}
	//- gps.X_SEND_GPS_MOUNT.on_change
}

static void gps_x_reboot_gps_handler(indigo_device *device) {
	//+ gps.X_REBOOT_GPS.on_change
	if (!IS_CONNECTED || PRIVATE_DATA->rebooting || PRIVATE_DATA->gps_forwarding_pending) {
		X_REBOOT_GPS_ITEM->sw.value = false;
		X_REBOOT_GPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_REBOOT_GPS_PROPERTY, "Another GPS operation is unfinished");
		return;
	}
	if (!X_REBOOT_GPS_ITEM->sw.value) {
		gps_reboot_finalizer(device);
	} else if (mgbox_command(device, ":rebootgps*")) {
		PRIVATE_DATA->gps_rebooting = true;
		mgbox_reset_gps(device);
		indigo_execute_handler_in(device, 2, gps_reboot_finalizer);
	} else {
		X_REBOOT_GPS_ITEM->sw.value = false;
		X_REBOOT_GPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_REBOOT_GPS_PROPERTY, NULL);
	}
	//- gps.X_REBOOT_GPS.on_change
}

#pragma mark - Device API (gps)

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result gps_attach(indigo_device *device) {
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ gps.on_attach
		PRIVATE_DATA->gps_device = device;
		INFO_PROPERTY->count = 6;
		//- gps.on_attach
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		//+ gps.GPS_GEOGRAPHIC_COORDINATES.on_attach
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		//- gps.GPS_GEOGRAPHIC_COORDINATES.on_attach
		GPS_UTC_TIME_PROPERTY->hidden = false;
		//+ gps.GPS_UTC_TIME.on_attach
		GPS_UTC_TIME_PROPERTY->count = 1;
		//- gps.GPS_UTC_TIME.on_attach
		X_SEND_GPS_MOUNT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SEND_GPS_MOUNT_PROPERTY_NAME, SETTINGS_GROUP, "Send GPS data to mount", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_SEND_GPS_MOUNT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SEND_GPS_MOUNT_ITEM, X_SEND_GPS_MOUNT_ITEM_NAME, "Enable", false);
		X_REBOOT_GPS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REBOOT_GPS_PROPERTY_NAME, SETTINGS_GROUP, "Reboot GPS", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_REBOOT_GPS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_REBOOT_GPS_ITEM, X_REBOOT_GPS_ITEM_NAME, "Reboot!", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SEND_GPS_MOUNT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_REBOOT_GPS_PROPERTY);
	}
	return indigo_gps_enumerate_properties(device, client, property);
}

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, gps_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SEND_GPS_MOUNT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SEND_GPS_MOUNT_PROPERTY, gps_x_send_gps_mount_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_REBOOT_GPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_REBOOT_GPS_PROPERTY, gps_x_reboot_gps_handler);
		return INDIGO_OK;
	}
	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connection_handler(device);
	}
	indigo_release_property(X_SEND_GPS_MOUNT_PROPERTY);
	indigo_release_property(X_REBOOT_GPS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(GPS_DEVICE_NAME, gps_attach, gps_enumerate_properties, gps_change_property, NULL, gps_detach);

#pragma mark - Main code

indigo_result indigo_aux_mgbox(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static mgbox_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	static indigo_device *gps = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (mgbox_private_data *)indigo_safe_malloc(sizeof(mgbox_private_data));
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			gps = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
			gps->private_data = private_data;
			gps->master_device = aux;
			indigo_attach_device(gps);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(aux);
			VERIFY_NOT_CONNECTED(gps);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (gps != NULL) {
				indigo_detach_device(gps);
				indigo_safe_free(gps);
				gps = NULL;
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
