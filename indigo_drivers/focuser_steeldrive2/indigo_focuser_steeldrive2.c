// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_steeldrive2.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_steeldrive2.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000D
#define DRIVER_NAME          "indigo_focuser_steeldrive2"
#define DRIVER_LABEL         "Baader Planetarium SteelDriveII Focuser"
#define FOCUSER_DEVICE_NAME  "SteelDriveII (focuser)"
#define AUX_DEVICE_NAME      "SteelDriveII (aux)"
#define PRIVATE_DATA         ((steeldrive2_private_data *)device->private_data)

//+ define

#define STEELDRIVE2_MAX_POSITION 2147483647
#define STEELDRIVE2_LINE_BUDGET 8
#define STEELDRIVE2_MOTION_FAILURE_LIMIT 3
#define STEELDRIVE2_MOTION_STALL_LIMIT 30

//- define

#pragma mark - Property definitions

#define X_NAME_PROPERTY                (PRIVATE_DATA->x_name_property)
#define X_NAME_ITEM                    (X_NAME_PROPERTY->items + 0)

#define X_NAME_PROPERTY_NAME           "X_NAME"
#define X_NAME_ITEM_NAME               "NAME"

#define X_SAVED_VALUES_PROPERTY        (PRIVATE_DATA->x_saved_values_property)
#define X_SAVED_FOCUS_ITEM             (X_SAVED_VALUES_PROPERTY->items + 0)
#define X_SAVED_JOGSTEPS_ITEM          (X_SAVED_VALUES_PROPERTY->items + 1)
#define X_SAVED_SINGLESTEPS_ITEM       (X_SAVED_VALUES_PROPERTY->items + 2)
#define X_SAVED_BKLGT_ITEM             (X_SAVED_VALUES_PROPERTY->items + 3)
#define X_SAVED_TEMP0_OFS_ITEM         (X_SAVED_VALUES_PROPERTY->items + 4)
#define X_SAVED_TEMP1_OFS_ITEM         (X_SAVED_VALUES_PROPERTY->items + 5)

#define X_SAVED_VALUES_PROPERTY_NAME   "X_SAVED_VALUES"
#define X_SAVED_FOCUS_ITEM_NAME        "FOCUS"
#define X_SAVED_JOGSTEPS_ITEM_NAME     "JOGSTEPS"
#define X_SAVED_SINGLESTEPS_ITEM_NAME  "SINGLESTEPS"
#define X_SAVED_BKLGT_ITEM_NAME        "BKLGT"
#define X_SAVED_TEMP0_OFS_ITEM_NAME    "TEMP0_OFS"
#define X_SAVED_TEMP1_OFS_ITEM_NAME    "TEMP1_OFS"

#define X_STATUS_PROPERTY              (PRIVATE_DATA->x_status_property)
#define X_STATUS_SENSOR_0_ITEM         (X_STATUS_PROPERTY->items + 0)
#define X_STATUS_SENSOR_1_ITEM         (X_STATUS_PROPERTY->items + 1)

#define X_STATUS_PROPERTY_NAME         "X_STATUS"
#define X_STATUS_SENSOR_0_ITEM_NAME    "SENSOR_0"
#define X_STATUS_SENSOR_1_ITEM_NAME    "SENSOR_1"

#define X_SELECT_TC_SENSOR_PROPERTY      (PRIVATE_DATA->x_select_tc_sensor_property)
#define X_SELECT_TC_SENSOR_0_ITEM        (X_SELECT_TC_SENSOR_PROPERTY->items + 0)
#define X_SELECT_TC_SENSOR_1_ITEM        (X_SELECT_TC_SENSOR_PROPERTY->items + 1)
#define X_SELECT_TC_SENSOR_AVG_ITEM      (X_SELECT_TC_SENSOR_PROPERTY->items + 2)

#define X_SELECT_TC_SENSOR_PROPERTY_NAME "X_SELECT_TC_SENSOR"
#define X_SELECT_TC_SENSOR_0_ITEM_NAME   "SENSOR_0"
#define X_SELECT_TC_SENSOR_1_ITEM_NAME   "SENSOR_1"
#define X_SELECT_TC_SENSOR_AVG_ITEM_NAME "AVG"

#define X_RESET_PROPERTY               (PRIVATE_DATA->x_reset_property)
#define X_RESET_ITEM                   (X_RESET_PROPERTY->items + 0)
#define X_REBOOT_ITEM                  (X_RESET_PROPERTY->items + 1)

#define X_RESET_PROPERTY_NAME          "X_RESET"
#define X_RESET_ITEM_NAME              "RESET"
#define X_REBOOT_ITEM_NAME             "REBOOT"

#define X_USE_ENDSTOP_PROPERTY           (PRIVATE_DATA->x_use_endstop_property)
#define X_USE_ENDSTOP_DISABLED_ITEM      (X_USE_ENDSTOP_PROPERTY->items + 0)
#define X_USE_ENDSTOP_ENABLED_ITEM       (X_USE_ENDSTOP_PROPERTY->items + 1)

#define X_USE_ENDSTOP_PROPERTY_NAME      "X_USE_ENDSTOP"
#define X_USE_ENDSTOP_DISABLED_ITEM_NAME "DISABLED"
#define X_USE_ENDSTOP_ENABLED_ITEM_NAME  "ENABLED"

#define X_START_ZEROING_PROPERTY       (PRIVATE_DATA->x_start_zeroing_property)
#define X_START_ZEROING_ITEM           (X_START_ZEROING_PROPERTY->items + 0)

#define X_START_ZEROING_PROPERTY_NAME  "X_START_ZEROING"
#define X_START_ZEROING_ITEM_NAME      "START"

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)

#define X_USE_AUTO_DEW_PROPERTY        (PRIVATE_DATA->x_use_auto_dew_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (X_USE_AUTO_DEW_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (X_USE_AUTO_DEW_PROPERTY->items + 1)

#define X_USE_AUTO_DEW_PROPERTY_NAME   "X_USE_AUTO_DEW"

#define X_USE_PID_PROPERTY             (PRIVATE_DATA->x_use_pid_property)
#define X_USE_PID_DISABLED_ITEM        (X_USE_PID_PROPERTY->items + 0)
#define X_USE_PID_ENABLED_ITEM         (X_USE_PID_PROPERTY->items + 1)

#define X_USE_PID_PROPERTY_NAME        "X_USE_PID"
#define X_USE_PID_DISABLED_ITEM_NAME   "DISABLED"
#define X_USE_PID_ENABLED_ITEM_NAME    "ENABLED"

#define X_PID_SETTINGS_PROPERTY         (PRIVATE_DATA->x_pid_settings_property)
#define X_PID_SETTINGS_OFS_ITEM         (X_PID_SETTINGS_PROPERTY->items + 0)
#define X_PID_SETTINGS_TARGET_ITEM      (X_PID_SETTINGS_PROPERTY->items + 1)

#define X_PID_SETTINGS_PROPERTY_NAME    "X_PID_SETTINGS"
#define X_PID_SETTINGS_OFS_ITEM_NAME    "PID_DEW_OFS"
#define X_PID_SETTINGS_TARGET_ITEM_NAME "PID TARGET"

#define X_SELECT_PID_SENSOR_PROPERTY      (PRIVATE_DATA->x_select_pid_sensor_property)
#define X_SELECT_PID_SENSOR_0_ITEM        (X_SELECT_PID_SENSOR_PROPERTY->items + 0)
#define X_SELECT_PID_SENSOR_1_ITEM        (X_SELECT_PID_SENSOR_PROPERTY->items + 1)
#define X_SELECT_PID_SENSOR_AVG_ITEM      (X_SELECT_PID_SENSOR_PROPERTY->items + 2)

#define X_SELECT_PID_SENSOR_PROPERTY_NAME "X_SELECT_PID_SENSOR"
#define X_SELECT_PID_SENSOR_0_ITEM_NAME   "SENSOR_0"
#define X_SELECT_PID_SENSOR_1_ITEM_NAME   "SENSOR_1"
#define X_SELECT_PID_SENSOR_AVG_ITEM_NAME "AVG"

#define X_SELECT_AMB_SENSOR_PROPERTY      (PRIVATE_DATA->x_select_amb_sensor_property)
#define X_SELECT_AMB_SENSOR_0_ITEM        (X_SELECT_AMB_SENSOR_PROPERTY->items + 0)
#define X_SELECT_AMB_SENSOR_1_ITEM        (X_SELECT_AMB_SENSOR_PROPERTY->items + 1)

#define X_SELECT_AMB_SENSOR_PROPERTY_NAME "X_SELECT_AMB_SENSOR"
#define X_SELECT_AMB_SENSOR_0_ITEM_NAME   "SENSOR_0"
#define X_SELECT_AMB_SENSOR_1_ITEM_NAME   "SENSOR_1"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_name_property;
	indigo_property *x_saved_values_property;
	indigo_property *x_status_property;
	indigo_property *x_select_tc_sensor_property;
	indigo_property *x_reset_property;
	indigo_property *x_use_endstop_property;
	indigo_property *x_start_zeroing_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *x_use_auto_dew_property;
	indigo_property *x_use_pid_property;
	indigo_property *x_pid_settings_property;
	indigo_property *x_select_pid_sensor_property;
	indigo_property *x_select_amb_sensor_property;
	//+ data
	char request[256];
	char response[512];
	char firmware[64];
	char name[20];
	int position, target, limit, focus, pwm, last_position, stalled, failures;
	double temperature_0, temperature_1, temperature_average;
	bool crc_enabled, moving, active, uncertain, zeroing;
	//- data
} steeldrive2_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void focuser_x_start_zeroing_handler(indigo_device *device);

static const uint8_t steeldrive2_crc_array[256] = {
	0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
	0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
	0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
	0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
	0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
	0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
	0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
	0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
	0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
	0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
	0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
	0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
	0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
	0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

static uint8_t steeldrive2_crc(const char *text, size_t length) {
	uint8_t crc = 0;
	for (size_t i = 0; i < length; i++) {
		crc = steeldrive2_crc_array[(uint8_t)text[i] ^ crc];
	}
	return crc;
}

static int steeldrive2_hex(char value) {
	if (value >= '0' && value <= '9') {
		return value - '0';
	}
	if (value >= 'A' && value <= 'F') {
		return value - 'A' + 10;
	}
	if (value >= 'a' && value <= 'f') {
		return value - 'a' + 10;
	}
	return -1;
}

static bool steeldrive2_read_line(indigo_device *device, bool crc_required) {
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || PRIVATE_DATA->response[count - 1] != '\n' || (long)strlen(PRIVATE_DATA->response) != count) {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	char *star = strrchr(PRIVATE_DATA->response, '*');
	if (star == NULL) {
		return !crc_required;
	}
	if (star[1] == 0 || star[2] == 0 || star[3] != 0) {
		return false;
	}
	int high = steeldrive2_hex(star[1]);
	int low = steeldrive2_hex(star[2]);
	if (high < 0 || low < 0 || steeldrive2_crc(PRIVATE_DATA->response, (size_t)(star - PRIVATE_DATA->response)) != (uint8_t)((high << 4) | low)) {
		return false;
	}
	*star = 0;
	return true;
}

static bool steeldrive2_vcommand(indigo_device *device, bool allow_unchecked_response, const char *format, va_list args) {
	int length = vsnprintf(PRIVATE_DATA->request, sizeof(PRIVATE_DATA->request), format, args);
	if (length <= 0 || length >= (int)sizeof(PRIVATE_DATA->request) || strncmp(PRIVATE_DATA->request, "$BS", 3)) {
		return false;
	}
	char wire[264];
	if (PRIVATE_DATA->crc_enabled) {
		int wire_length = snprintf(wire, sizeof(wire), "%s*%02X\r\n", PRIVATE_DATA->request, steeldrive2_crc(PRIVATE_DATA->request, (size_t)length));
		if (wire_length <= 0 || wire_length >= (int)sizeof(wire)) {
			return false;
		}
		length = wire_length;
	} else {
		int wire_length = snprintf(wire, sizeof(wire), "%s\r\n", PRIVATE_DATA->request);
		if (wire_length <= 0 || wire_length >= (int)sizeof(wire)) {
			return false;
		}
		length = wire_length;
	}
	if (indigo_uni_discard(PRIVATE_DATA->handle) < 0 || indigo_uni_write(PRIVATE_DATA->handle, wire, length) != length) {
		return false;
	}
	bool echoed = false;
	for (int i = 0; i < STEELDRIVE2_LINE_BUDGET; i++) {
		if (!steeldrive2_read_line(device, PRIVATE_DATA->crc_enabled)) {
			return false;
		}
		if (!strcmp(PRIVATE_DATA->response, PRIVATE_DATA->request)) {
			echoed = true;
			break;
		}
	}
	if (!echoed) {
		return false;
	}
	for (int i = 0; i < STEELDRIVE2_LINE_BUDGET; i++) {
		if (!steeldrive2_read_line(device, PRIVATE_DATA->crc_enabled && !allow_unchecked_response)) {
			return false;
		}
		if (!strncmp(PRIVATE_DATA->response, "$BS DEBUG:", 10)) {
			continue;
		}
		return !strncmp(PRIVATE_DATA->response, "$BS", 3);
	}
	return false;
}

static bool steeldrive2_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = steeldrive2_vcommand(device, false, format, args);
	va_end(args);
	return result;
}

static bool steeldrive2_unchecked_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = steeldrive2_vcommand(device, true, format, args);
	va_end(args);
	return result;
}

static bool steeldrive2_number(const char *text, double minimum, double maximum, bool integer, double *value) {
	if (text == NULL || *text == 0 || isspace((unsigned char)*text)) {
		return false;
	}
	char *end;
	errno = 0;
	double parsed = strtod(text, &end);
	if (errno || *end || !isfinite(parsed) || parsed < minimum || parsed > maximum || (integer && floor(parsed) != parsed)) {
		return false;
	}
	*value = parsed;
	return true;
}

static const char *steeldrive2_status_value(indigo_device *device, const char *name) {
	char prefix[80];
	int length = snprintf(prefix, sizeof(prefix), "$BS STATUS %s:", name);
	if (length <= 0 || length >= (int)sizeof(prefix) || strncmp(PRIVATE_DATA->response, prefix, (size_t)length)) {
		return NULL;
	}
	return PRIVATE_DATA->response + length;
}

static bool steeldrive2_get_number(indigo_device *device, const char *command, const char *name, double minimum, double maximum, bool integer, double *value) {
	return steeldrive2_command(device, "$BS GET %s", command) && steeldrive2_number(steeldrive2_status_value(device, name), minimum, maximum, integer, value);
}

static bool steeldrive2_get_switch(indigo_device *device, const char *command, const char *name, int maximum, int *value) {
	double parsed;
	if (!steeldrive2_get_number(device, command, name, 0, maximum, true, &parsed)) {
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool steeldrive2_ok(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = steeldrive2_vcommand(device, false, format, args);
	va_end(args);
	return result && !strcmp(PRIVATE_DATA->response, "$BS OK");
}

static bool steeldrive2_summary(indigo_device *device) {
	if (!steeldrive2_command(device, "$BS SUMMARY")) {
		return false;
	}
	const char *prefix = "$BS STATUS NAME:";
	if (strncmp(PRIVATE_DATA->response, prefix, strlen(prefix))) {
		return false;
	}
	char *fields[10];
	char *cursor = PRIVATE_DATA->response + strlen(prefix);
	for (int i = 0; i < 10; i++) {
		fields[i] = cursor;
		char *separator = strchr(cursor, ';');
		if (i == 9) {
			if (separator != NULL) {
				return false;
			}
		} else {
			if (separator == NULL) {
				return false;
			}
			*separator = 0;
			cursor = separator + 1;
		}
	}
	const char *keys[] = { "", "POS:", "STATE:", "LIMIT:", "FOCUS:", "TEMP0:", "TEMP1:", "TEMP_AVG:", "TCOMP:", "PWM:" };
	for (int i = 1; i < 10; i++) {
		size_t key_length = strlen(keys[i]);
		if (strncmp(fields[i], keys[i], key_length)) {
			return false;
		}
		fields[i] += key_length;
	}
	double position, limit, focus, temperature_0, temperature_1, temperature_average, tcomp, pwm;
	if (*fields[0] == 0 || strlen(fields[0]) > 19 || !steeldrive2_number(fields[1], 0, STEELDRIVE2_MAX_POSITION, true, &position) || (strcmp(fields[2], "GOING_UP") && strcmp(fields[2], "GOING_DOWN") && strcmp(fields[2], "STOPPED") && strcmp(fields[2], "ZEROED")) || !steeldrive2_number(fields[3], 0, STEELDRIVE2_MAX_POSITION, true, &limit) || !steeldrive2_number(fields[4], 0, STEELDRIVE2_MAX_POSITION, true, &focus) || !steeldrive2_number(fields[5], -128, 150, false, &temperature_0) || !steeldrive2_number(fields[6], -128, 150, false, &temperature_1) || !steeldrive2_number(fields[7], -128, 150, false, &temperature_average) || !steeldrive2_number(fields[8], 0, 1, true, &tcomp) || !steeldrive2_number(fields[9], 0, 100, true, &pwm)) {
		return false;
	}
	PRIVATE_DATA->position = (int)position;
	PRIVATE_DATA->limit = (int)limit;
	PRIVATE_DATA->focus = (int)focus;
	PRIVATE_DATA->temperature_0 = temperature_0;
	PRIVATE_DATA->temperature_1 = temperature_1;
	PRIVATE_DATA->temperature_average = temperature_average;
	PRIVATE_DATA->pwm = (int)pwm;
	PRIVATE_DATA->moving = !strcmp(fields[2], "GOING_UP") || !strcmp(fields[2], "GOING_DOWN");
	return true;
}

static bool steeldrive2_read_name(indigo_device *device) {
	if (!steeldrive2_command(device, "$BS GET NAME")) {
		return false;
	}
	const char *value = steeldrive2_status_value(device, "NAME");
	if (value == NULL || *value == 0 || strlen(value) > 19 || strpbrk(value, ";:*\r\n") != NULL) {
		return false;
	}
	snprintf(PRIVATE_DATA->name, sizeof(PRIVATE_DATA->name), "%s", value);
	INDIGO_COPY_VALUE(X_NAME_ITEM->text.value, value);
	return true;
}

static bool steeldrive2_read_saved(indigo_device *device) {
	double values[6];
	if (!steeldrive2_get_number(device, "FOCUS", "FOCUS", 0, STEELDRIVE2_MAX_POSITION, true, values) || !steeldrive2_get_number(device, "JOGSTEPS", "JOGSTEPS", 1, STEELDRIVE2_MAX_POSITION, true, values + 1) || !steeldrive2_get_number(device, "SINGLESTEPS", "SINGLESTEPS", 1, STEELDRIVE2_MAX_POSITION, true, values + 2) || !steeldrive2_get_number(device, "BKLGT", "BKLGT", 0, 100, true, values + 3) || !steeldrive2_get_number(device, "TEMP0_OFS", "TEMP0_OFS", -50, 50, false, values + 4) || !steeldrive2_get_number(device, "TEMP1_OFS", "TEMP1_OFS", -50, 50, false, values + 5) || values[2] > values[1]) {
		return false;
	}
	for (int i = 0; i < 6; i++) {
		X_SAVED_VALUES_PROPERTY->items[i].number.value = X_SAVED_VALUES_PROPERTY->items[i].number.target = values[i];
	}
	return true;
}

static bool steeldrive2_read_compensation(indigo_device *device) {
	double factor, period, threshold;
	if (!steeldrive2_get_number(device, "TCOMP_FACTOR", "TCOMP_FACTOR", -100000, 100000, false, &factor) || !steeldrive2_get_number(device, "TCOMP_PERIOD", "TCOMP_PERIOD", 0, UINT_MAX, true, &period) || !steeldrive2_get_number(device, "TCOMP_DELTA", "TCOMP_DELTA", 0, 100, false, &threshold)) {
		return false;
	}
	FOCUSER_COMPENSATION_ITEM->number.value = FOCUSER_COMPENSATION_ITEM->number.target = factor;
	FOCUSER_COMPENSATION_PERIOD_ITEM->number.value = FOCUSER_COMPENSATION_PERIOD_ITEM->number.target = period / 1000;
	FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target = threshold;
	return true;
}

static void steeldrive2_set_switch(indigo_property *property, int index) {
	for (int i = 0; i < property->count; i++) {
		property->items[i].sw.value = i == index;
	}
}

static bool steeldrive2_load_focuser(indigo_device *device) {
	int tcomp, sensor, endstop;
	if (!steeldrive2_read_name(device) || !steeldrive2_read_saved(device) || !steeldrive2_get_switch(device, "TCOMP", "TCOMP", 1, &tcomp) || !steeldrive2_read_compensation(device) || !steeldrive2_get_switch(device, "TCOMP_SENSOR", "TCOMP_SENSOR", 2, &sensor) || !steeldrive2_get_switch(device, "USE_ENDSTOP", "USE_ENDSTOP", 1, &endstop) || !steeldrive2_summary(device)) {
		return false;
	}
	steeldrive2_set_switch(FOCUSER_MODE_PROPERTY, tcomp ? 1 : 0);
	steeldrive2_set_switch(X_SELECT_TC_SENSOR_PROPERTY, sensor);
	steeldrive2_set_switch(X_USE_ENDSTOP_PROPERTY, endstop ? 1 : 0);
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
	FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->limit;
	FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->limit;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->limit;
	FOCUSER_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->temperature_average;
	X_STATUS_SENSOR_0_ITEM->number.value = PRIVATE_DATA->temperature_0;
	X_STATUS_SENSOR_1_ITEM->number.value = PRIVATE_DATA->temperature_1;
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Baader Planetarium SteelDriveII");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
	PRIVATE_DATA->target = PRIVATE_DATA->position;
	PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->zeroing = false;
	return true;
}

static bool steeldrive2_read_aux(indigo_device *device, bool complete) {
	double pwm, target = 0, offset = 0;
	int pid, auto_dew, pid_sensor = 0, ambient_sensor = 0;
	if (!steeldrive2_get_switch(device, "PID_CTRL", "PID_CTRL", 1, &pid) || !steeldrive2_get_number(device, "PWM", "PWM", 0, 100, true, &pwm) || !steeldrive2_get_switch(device, "AUTO_DEW", "AUTO_DEW", 1, &auto_dew)) {
		return false;
	}
	if (complete && (!steeldrive2_get_switch(device, "PID_SENSOR", "PID_SENSOR", 2, &pid_sensor) || !steeldrive2_get_number(device, "PID_DEW_OFS", "PID_DEW_OFS", -50, 50, false, &offset) || !steeldrive2_get_number(device, "PID_TARGET", "PID_TARGET", -50, 50, false, &target) || !steeldrive2_get_switch(device, "AMBIENT_SENSOR", "AMBIENT_SENSOR", 1, &ambient_sensor))) {
		return false;
	}
	PRIVATE_DATA->pwm = (int)pwm;
	AUX_HEATER_OUTLET_1_ITEM->number.value = AUX_HEATER_OUTLET_1_ITEM->number.target = pwm;
	steeldrive2_set_switch(X_USE_PID_PROPERTY, pid ? 1 : 0);
	steeldrive2_set_switch(X_USE_AUTO_DEW_PROPERTY, auto_dew ? 1 : 0);
	if (complete) {
		steeldrive2_set_switch(X_SELECT_PID_SENSOR_PROPERTY, pid_sensor);
		steeldrive2_set_switch(X_SELECT_AMB_SENSOR_PROPERTY, ambient_sensor);
		X_PID_SETTINGS_OFS_ITEM->number.value = X_PID_SETTINGS_OFS_ITEM->number.target = offset;
		X_PID_SETTINGS_TARGET_ITEM->number.value = X_PID_SETTINGS_TARGET_ITEM->number.target = target;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Baader Planetarium SteelDriveII");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
	}
	return true;
}

static bool steeldrive2_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	PRIVATE_DATA->crc_enabled = false;
	bool result = steeldrive2_command(device, "$BS CRC_DISABLE") && !strcmp(PRIVATE_DATA->response, "$BS OK") && steeldrive2_command(device, "$BS GET VERSION");
	const char *version = result ? steeldrive2_status_value(device, "VERSION") : NULL;
	if (version == NULL || *version == 0 || strlen(version) >= sizeof(PRIVATE_DATA->firmware)) {
		result = false;
	} else {
		snprintf(PRIVATE_DATA->firmware, sizeof(PRIVATE_DATA->firmware), "%s", version);
	}
	if (result) {
		result = steeldrive2_command(device, "$BS CRC_ENABLE") && !strcmp(PRIVATE_DATA->response, "$BS OK");
	}
	if (result) {
		PRIVATE_DATA->crc_enabled = true;
		return true;
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	PRIVATE_DATA->crc_enabled = false;
	return false;
}

static void steeldrive2_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	PRIVATE_DATA->crc_enabled = PRIVATE_DATA->active = PRIVATE_DATA->moving = PRIVATE_DATA->uncertain = PRIVATE_DATA->zeroing = false;
}

//- code

//+ focuser.code

static void steeldrive2_publish_focuser(indigo_device *device, indigo_property_state motion_state) {
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->position;
	FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->limit;
	FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->limit;
	FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->limit;
	FOCUSER_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->temperature_average;
	X_STATUS_SENSOR_0_ITEM->number.value = PRIVATE_DATA->temperature_0;
	X_STATUS_SENSOR_1_ITEM->number.value = PRIVATE_DATA->temperature_1;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = motion_state;
	FOCUSER_TEMPERATURE_PROPERTY->state = X_STATUS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	indigo_update_property(device, X_STATUS_PROPERTY, NULL);
}

static void steeldrive2_finish_motion(indigo_device *device, indigo_property_state state) {
	PRIVATE_DATA->active = false;
	FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
	steeldrive2_publish_focuser(device, state);
	if (PRIVATE_DATA->zeroing) {
		PRIVATE_DATA->zeroing = false;
		X_START_ZEROING_ITEM->sw.value = false;
		X_START_ZEROING_PROPERTY->state = state;
		indigo_update_property(device, X_START_ZEROING_PROPERTY, NULL);
	}
}

static void motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->active) {
		return;
	}
	if (!steeldrive2_summary(device)) {
		if (++PRIVATE_DATA->failures < STEELDRIVE2_MOTION_FAILURE_LIMIT) {
			indigo_execute_handler_in(device, 0.1, motion_finalizer);
			return;
		}
		PRIVATE_DATA->uncertain = true;
		steeldrive2_ok(device, "$BS STOP");
		steeldrive2_finish_motion(device, INDIGO_ALERT_STATE);
		return;
	}
	PRIVATE_DATA->failures = 0;
	if (PRIVATE_DATA->moving) {
		if (PRIVATE_DATA->position == PRIVATE_DATA->last_position) {
			PRIVATE_DATA->stalled++;
		} else {
			PRIVATE_DATA->last_position = PRIVATE_DATA->position;
			PRIVATE_DATA->stalled = 0;
		}
		if (PRIVATE_DATA->stalled >= STEELDRIVE2_MOTION_STALL_LIMIT) {
			PRIVATE_DATA->uncertain = true;
			steeldrive2_ok(device, "$BS STOP");
			steeldrive2_finish_motion(device, INDIGO_ALERT_STATE);
			return;
		}
		steeldrive2_publish_focuser(device, INDIGO_BUSY_STATE);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
		return;
	}
	PRIVATE_DATA->uncertain = PRIVATE_DATA->position != PRIVATE_DATA->target;
	steeldrive2_finish_motion(device, PRIVATE_DATA->uncertain ? INDIGO_ALERT_STATE : INDIGO_OK_STATE);
}

static bool steeldrive2_start_motion(indigo_device *device, int target, bool zeroing) {
	if (PRIVATE_DATA->active || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
		return false;
	}
	target = (int)fmax(0, fmin(PRIVATE_DATA->limit, target));
	PRIVATE_DATA->target = target;
	if (!zeroing && target == PRIVATE_DATA->position) {
		FOCUSER_POSITION_ITEM->number.target = target;
		steeldrive2_publish_focuser(device, INDIGO_OK_STATE);
		return true;
	}
	bool accepted = zeroing ? steeldrive2_ok(device, "$BS ZEROING") : steeldrive2_ok(device, "$BS GO %d", target);
	if (!accepted) {
		return false;
	}
	if (!zeroing) {
		FOCUSER_POSITION_ITEM->number.target = target;
	}
	PRIVATE_DATA->active = true;
	PRIVATE_DATA->zeroing = zeroing;
	PRIVATE_DATA->stalled = PRIVATE_DATA->failures = 0;
	PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	steeldrive2_publish_focuser(device, INDIGO_BUSY_STATE);
	return true;
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (!PRIVATE_DATA->active && !PRIVATE_DATA->uncertain) {
		if (steeldrive2_summary(device)) {
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			steeldrive2_publish_focuser(device, PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
		} else {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = FOCUSER_TEMPERATURE_PROPERTY->state = X_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			indigo_update_property(device, X_STATUS_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, PRIVATE_DATA->moving ? 0.1 : 0.5, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = steeldrive2_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser.on_connect
			connection_result = steeldrive2_load_focuser(device);
			if (connection_result) {
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_NAME_PROPERTY, NULL);
			indigo_define_property(device, X_SAVED_VALUES_PROPERTY, NULL);
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);
			indigo_define_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
			indigo_define_property(device, X_RESET_PROPERTY, NULL);
			indigo_define_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
			indigo_define_property(device, X_START_ZEROING_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				steeldrive2_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		if (PRIVATE_DATA->active || PRIVATE_DATA->moving || PRIVATE_DATA->uncertain) {
			steeldrive2_ok(device, "$BS STOP");
			steeldrive2_summary(device);
		}
		PRIVATE_DATA->active = PRIVATE_DATA->moving = PRIVATE_DATA->uncertain = PRIVATE_DATA->zeroing = false;
		//- focuser.on_disconnect
		indigo_delete_property(device, X_NAME_PROPERTY, NULL);
		indigo_delete_property(device, X_SAVED_VALUES_PROPERTY, NULL);
		indigo_delete_property(device, X_STATUS_PROPERTY, NULL);
		indigo_delete_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
		indigo_delete_property(device, X_RESET_PROPERTY, NULL);
		indigo_delete_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
		indigo_delete_property(device, X_START_ZEROING_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			steeldrive2_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_x_name_handler(indigo_device *device) {
	X_NAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_NAME.on_change
	const char *requested = X_NAME_ITEM->text.value;
	if (*requested == 0 || strlen(requested) > 19 || strpbrk(requested, ";:*\r\n") != NULL || !steeldrive2_ok(device, "$BS SET NAME:%s", requested)) {
		X_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(X_NAME_ITEM->text.value, PRIVATE_DATA->name);
	} else {
		snprintf(PRIVATE_DATA->name, sizeof(PRIVATE_DATA->name), "%s", requested);
	}
	//- focuser.X_NAME.on_change
	indigo_update_property(device, X_NAME_PROPERTY, NULL);
}

static void focuser_x_saved_values_handler(indigo_device *device) {
	X_SAVED_VALUES_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_SAVED_VALUES.on_change
	bool accepted = X_SAVED_SINGLESTEPS_ITEM->number.value <= X_SAVED_JOGSTEPS_ITEM->number.value;
	accepted = accepted && steeldrive2_ok(device, "$BS SET FOCUS:%d", (int)X_SAVED_FOCUS_ITEM->number.value);
	accepted = accepted && steeldrive2_ok(device, "$BS SET JOGSTEPS:%d", (int)X_SAVED_JOGSTEPS_ITEM->number.value);
	accepted = accepted && steeldrive2_ok(device, "$BS SET SINGLESTEPS:%d", (int)X_SAVED_SINGLESTEPS_ITEM->number.value);
	accepted = accepted && steeldrive2_ok(device, "$BS SET BKLGT:%d", (int)X_SAVED_BKLGT_ITEM->number.value);
	accepted = accepted && steeldrive2_ok(device, "$BS SET TEMP0_OFS:%.2f", X_SAVED_TEMP0_OFS_ITEM->number.value);
	accepted = accepted && steeldrive2_ok(device, "$BS SET TEMP1_OFS:%.2f", X_SAVED_TEMP1_OFS_ITEM->number.value);
	if (!steeldrive2_read_saved(device) || !accepted) {
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_SAVED_VALUES.on_change
	indigo_update_property(device, X_SAVED_VALUES_PROPERTY, NULL);
}

static void focuser_x_select_tc_sensor_handler(indigo_device *device) {
	X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_SELECT_TC_SENSOR.on_change
	int requested = X_SELECT_TC_SENSOR_0_ITEM->sw.value ? 0 : X_SELECT_TC_SENSOR_1_ITEM->sw.value ? 1 : 2;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET TCOMP_SENSOR:%d", requested) || !steeldrive2_get_switch(device, "TCOMP_SENSOR", "TCOMP_SENSOR", 2, &actual) || actual != requested) {
		X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "TCOMP_SENSOR", "TCOMP_SENSOR", 2, &actual)) {
		steeldrive2_set_switch(X_SELECT_TC_SENSOR_PROPERTY, actual);
	}
	//- focuser.X_SELECT_TC_SENSOR.on_change
	indigo_update_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
}

static void focuser_x_reset_handler(indigo_device *device) {
	X_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RESET.on_change
	bool reset = X_RESET_ITEM->sw.value;
	bool reboot = X_REBOOT_ITEM->sw.value;
	X_RESET_ITEM->sw.value = X_REBOOT_ITEM->sw.value = false;
	if ((!reset && !reboot) || PRIVATE_DATA->count != 1 || PRIVATE_DATA->active || PRIVATE_DATA->moving || (reset && !steeldrive2_ok(device, "$BS RESET")) || (reboot && (!steeldrive2_unchecked_command(device, "$BS REBOOT") || strcmp(PRIVATE_DATA->response, "$BS Hello World!")))) {
		X_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		indigo_device_disconnect(NULL, device->name);
	}
	//- focuser.X_RESET.on_change
	indigo_update_property(device, X_RESET_PROPERTY, NULL);
}

static void focuser_x_use_endstop_handler(indigo_device *device) {
	X_USE_ENDSTOP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_USE_ENDSTOP.on_change
	int requested = X_USE_ENDSTOP_ENABLED_ITEM->sw.value ? 1 : 0;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET USE_ENDSTOP:%d", requested) || !steeldrive2_get_switch(device, "USE_ENDSTOP", "USE_ENDSTOP", 1, &actual) || actual != requested) {
		X_USE_ENDSTOP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "USE_ENDSTOP", "USE_ENDSTOP", 1, &actual)) {
		steeldrive2_set_switch(X_USE_ENDSTOP_PROPERTY, actual ? 1 : 0);
	}
	//- focuser.X_USE_ENDSTOP.on_change
	indigo_update_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
}

static void focuser_x_start_zeroing_handler(indigo_device *device) {
	//+ focuser.X_START_ZEROING.on_change
	bool requested = X_START_ZEROING_ITEM->sw.value;
	X_START_ZEROING_ITEM->sw.value = false;
	if (!requested || !steeldrive2_start_motion(device, 0, true)) {
		X_START_ZEROING_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_START_ZEROING_PROPERTY, NULL);
	} else {
		X_START_ZEROING_ITEM->sw.value = true;
		X_START_ZEROING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_START_ZEROING_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.X_START_ZEROING.on_change
}

static void focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_LIMITS.on_change
	int requested = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
	double actual;
	if (PRIVATE_DATA->active || !steeldrive2_ok(device, "$BS SET LIMIT:%d", requested) || !steeldrive2_get_number(device, "LIMIT", "LIMIT", 0, STEELDRIVE2_MAX_POSITION, true, &actual) || (int)actual != requested) {
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_number(device, "LIMIT", "LIMIT", 0, STEELDRIVE2_MAX_POSITION, true, &actual)) {
		PRIVATE_DATA->limit = (int)actual;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = actual;
		FOCUSER_POSITION_ITEM->number.max = FOCUSER_STEPS_ITEM->number.max = actual;
	}
	//- focuser.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_MODE.on_change
	int requested = FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? 1 : 0;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET TCOMP:%d", requested) || !steeldrive2_get_switch(device, "TCOMP", "TCOMP", 1, &actual) || actual != requested) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "TCOMP", "TCOMP", 1, &actual)) {
		steeldrive2_set_switch(FOCUSER_MODE_PROPERTY, actual ? 1 : 0);
	}
	//- focuser.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_compensation_handler(indigo_device *device) {
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_COMPENSATION.on_change
	double factor = FOCUSER_COMPENSATION_ITEM->number.target;
	double period = FOCUSER_COMPENSATION_PERIOD_ITEM->number.target;
	double threshold = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target;
	bool accepted = steeldrive2_ok(device, "$BS SET TCOMP_FACTOR:%.2f", factor) && steeldrive2_ok(device, "$BS SET TCOMP_PERIOD:%u", (unsigned)(period * 1000)) && steeldrive2_ok(device, "$BS SET TCOMP_DELTA:%.2f", threshold);
	if (!steeldrive2_read_compensation(device) || !accepted || FOCUSER_COMPENSATION_ITEM->number.value != factor || FOCUSER_COMPENSATION_PERIOD_ITEM->number.value != period || FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value != threshold) {
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_COMPENSATION.on_change
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int requested = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		bool accepted = !PRIVATE_DATA->active && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain && steeldrive2_ok(device, "$BS SET POS:%d", requested) && steeldrive2_summary(device) && PRIVATE_DATA->position == requested && !PRIVATE_DATA->moving;
		PRIVATE_DATA->uncertain = !accepted;
		if (accepted) {
			PRIVATE_DATA->target = PRIVATE_DATA->position;
		}
		steeldrive2_finish_motion(device, accepted ? INDIGO_OK_STATE : INDIGO_ALERT_STATE);
	} else if (!steeldrive2_start_motion(device, requested, false)) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1;
	if (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value) {
		direction = -direction;
	}
	long long requested = (long long)PRIVATE_DATA->position + direction * (long long)FOCUSER_STEPS_ITEM->number.value;
	int target = requested < 0 ? 0 : requested > PRIVATE_DATA->limit ? PRIVATE_DATA->limit : (int)requested;
	if (!steeldrive2_start_motion(device, target, false)) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (PRIVATE_DATA->active) {
		indigo_execute_handler_in(device, 0.1, motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, focuser_x_start_zeroing_handler);
		bool stopped = (!PRIVATE_DATA->active && !PRIVATE_DATA->moving && !PRIVATE_DATA->uncertain) || (steeldrive2_ok(device, "$BS STOP") && steeldrive2_summary(device) && !PRIVATE_DATA->moving);
		indigo_cancel_pending_handler(device, motion_finalizer);
		if (stopped) {
			if (PRIVATE_DATA->zeroing) {
				X_START_ZEROING_ITEM->sw.value = false;
				X_START_ZEROING_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, X_START_ZEROING_PROPERTY, "Zeroing aborted");
			}
			PRIVATE_DATA->active = PRIVATE_DATA->uncertain = PRIVATE_DATA->zeroing = false;
			PRIVATE_DATA->target = PRIVATE_DATA->position;
			FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position;
			steeldrive2_publish_focuser(device, INDIGO_OK_STATE);
		} else {
			PRIVATE_DATA->active = false;
			PRIVATE_DATA->uncertain = true;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		X_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, X_NAME_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Device name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_NAME_ITEM, X_NAME_ITEM_NAME, "Name", "");
		X_SAVED_VALUES_PROPERTY = indigo_init_number_property(NULL, device->name, X_SAVED_VALUES_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Saved values", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
		if (X_SAVED_VALUES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_SAVED_FOCUS_ITEM, X_SAVED_FOCUS_ITEM_NAME, "Saved focus", 0, 2147483647, 1, 0);
		indigo_init_number_item(X_SAVED_JOGSTEPS_ITEM, X_SAVED_JOGSTEPS_ITEM_NAME, "Jogging mode steps", 1, 2147483647, 1, 50);
		indigo_init_number_item(X_SAVED_SINGLESTEPS_ITEM, X_SAVED_SINGLESTEPS_ITEM_NAME, "Single mode steps", 1, 2147483647, 1, 1);
		indigo_init_number_item(X_SAVED_BKLGT_ITEM, X_SAVED_BKLGT_ITEM_NAME, "Backlight brightness", 0, 100, 1, 50);
		indigo_init_number_item(X_SAVED_TEMP0_OFS_ITEM, X_SAVED_TEMP0_OFS_ITEM_NAME, "Sensor #0 offset", -50, 50, 0.01, 0);
		indigo_init_number_item(X_SAVED_TEMP1_OFS_ITEM, X_SAVED_TEMP1_OFS_ITEM_NAME, "Sensor #1 offset", -50, 50, 0.01, 0);
		X_STATUS_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATUS_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Status", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_STATUS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATUS_SENSOR_0_ITEM, X_STATUS_SENSOR_0_ITEM_NAME, "Sensor #0", -128, 150, 0, 0);
		indigo_init_number_item(X_STATUS_SENSOR_1_ITEM, X_STATUS_SENSOR_1_ITEM_NAME, "Sensor #1", -128, 150, 0, 0);
		X_SELECT_TC_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SELECT_TC_SENSOR_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "TCOMP sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_SELECT_TC_SENSOR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SELECT_TC_SENSOR_0_ITEM, X_SELECT_TC_SENSOR_0_ITEM_NAME, "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_TC_SENSOR_1_ITEM, X_SELECT_TC_SENSOR_1_ITEM_NAME, "Sensor #1", false);
		indigo_init_switch_item(X_SELECT_TC_SENSOR_AVG_ITEM, X_SELECT_TC_SENSOR_AVG_ITEM_NAME, "Average", true);
		X_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RESET_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (X_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RESET_ITEM, X_RESET_ITEM_NAME, "Reset", false);
		indigo_init_switch_item(X_REBOOT_ITEM, X_REBOOT_ITEM_NAME, "Reboot", false);
		X_USE_ENDSTOP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_USE_ENDSTOP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Use end-stop sensor", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_USE_ENDSTOP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_USE_ENDSTOP_DISABLED_ITEM, X_USE_ENDSTOP_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(X_USE_ENDSTOP_ENABLED_ITEM, X_USE_ENDSTOP_ENABLED_ITEM_NAME, "Enabled", false);
		X_START_ZEROING_PROPERTY = indigo_init_switch_property(NULL, device->name, X_START_ZEROING_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Start zeroing", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_START_ZEROING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_START_ZEROING_ITEM, X_START_ZEROING_ITEM_NAME, "Start", false);
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = STEELDRIVE2_MAX_POSITION;
		//- focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_PROPERTY->count = 3;
		//- focuser.FOCUSER_COMPENSATION.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0; FOCUSER_POSITION_ITEM->number.max = STEELDRIVE2_MAX_POSITION; FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0; FOCUSER_STEPS_ITEM->number.max = STEELDRIVE2_MAX_POSITION; FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_NAME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SAVED_VALUES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATUS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SELECT_TC_SENSOR_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_USE_ENDSTOP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_START_ZEROING_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_NAME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_NAME_PROPERTY, focuser_x_name_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SAVED_VALUES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SAVED_VALUES_PROPERTY, focuser_x_saved_values_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SELECT_TC_SENSOR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SELECT_TC_SENSOR_PROPERTY, focuser_x_select_tc_sensor_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RESET_PROPERTY, focuser_x_reset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_USE_ENDSTOP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_USE_ENDSTOP_PROPERTY, focuser_x_use_endstop_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_START_ZEROING_PROPERTY, property)) {
		//+ focuser.X_START_ZEROING.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, X_START_ZEROING_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.X_START_ZEROING.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_START_ZEROING_PROPERTY, focuser_x_start_zeroing_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ON_POSITION_SET_PROPERTY, property, false);
		FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_COMPENSATION_PROPERTY, focuser_compensation_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		//+ focuser.FOCUSER_POSITION.on_change_request
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || X_START_ZEROING_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		//+ focuser.FOCUSER_STEPS.on_change_request
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || X_START_ZEROING_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Another motion is pending");
			return INDIGO_OK;
		}
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_NAME_PROPERTY);
	indigo_release_property(X_SAVED_VALUES_PROPERTY);
	indigo_release_property(X_STATUS_PROPERTY);
	indigo_release_property(X_SELECT_TC_SENSOR_PROPERTY);
	indigo_release_property(X_RESET_PROPERTY);
	indigo_release_property(X_USE_ENDSTOP_PROPERTY);
	indigo_release_property(X_START_ZEROING_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	if (steeldrive2_read_aux(device, false)) {
		AUX_HEATER_OUTLET_PROPERTY->state = X_USE_PID_PROPERTY->state = X_USE_AUTO_DEW_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_HEATER_OUTLET_PROPERTY->state = X_USE_PID_PROPERTY->state = X_USE_AUTO_DEW_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
	indigo_update_property(device, X_USE_AUTO_DEW_PROPERTY, NULL);
	indigo_execute_handler_in(device, 1, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = steeldrive2_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux.on_connect
			connection_result = steeldrive2_read_aux(device, true);
			if (connection_result) {
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, X_USE_AUTO_DEW_PROPERTY, NULL);
			indigo_define_property(device, X_USE_PID_PROPERTY, NULL);
			indigo_define_property(device, X_PID_SETTINGS_PROPERTY, NULL);
			indigo_define_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
			indigo_define_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				steeldrive2_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, X_USE_AUTO_DEW_PROPERTY, NULL);
		indigo_delete_property(device, X_USE_PID_PROPERTY, NULL);
		indigo_delete_property(device, X_PID_SETTINGS_PROPERTY, NULL);
		indigo_delete_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
		indigo_delete_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			steeldrive2_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	int requested = (int)AUX_HEATER_OUTLET_1_ITEM->number.value;
	bool accepted = steeldrive2_ok(device, "$BS SET PWM:%d", requested);
	if (!steeldrive2_read_aux(device, false) || !accepted || PRIVATE_DATA->pwm != requested) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
	indigo_update_property(device, X_USE_AUTO_DEW_PROPERTY, NULL);
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_x_use_auto_dew_handler(indigo_device *device) {
	X_USE_AUTO_DEW_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_USE_AUTO_DEW.on_change
	int requested = AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET AUTO_DEW:%d", requested) || !steeldrive2_get_switch(device, "AUTO_DEW", "AUTO_DEW", 1, &actual) || actual != requested) {
		X_USE_AUTO_DEW_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "AUTO_DEW", "AUTO_DEW", 1, &actual)) {
		steeldrive2_set_switch(X_USE_AUTO_DEW_PROPERTY, actual ? 1 : 0);
	}
	//- aux.X_USE_AUTO_DEW.on_change
	indigo_update_property(device, X_USE_AUTO_DEW_PROPERTY, NULL);
}

static void aux_x_use_pid_handler(indigo_device *device) {
	X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_USE_PID.on_change
	int requested = X_USE_PID_ENABLED_ITEM->sw.value ? 1 : 0;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET PID_CTRL:%d", requested) || !steeldrive2_get_switch(device, "PID_CTRL", "PID_CTRL", 1, &actual) || actual != requested) {
		X_USE_PID_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "PID_CTRL", "PID_CTRL", 1, &actual)) {
		steeldrive2_set_switch(X_USE_PID_PROPERTY, actual ? 1 : 0);
	}
	//- aux.X_USE_PID.on_change
	indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
}

static void aux_x_pid_settings_handler(indigo_device *device) {
	X_PID_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_PID_SETTINGS.on_change
	double target = X_PID_SETTINGS_TARGET_ITEM->number.value;
	double offset = X_PID_SETTINGS_OFS_ITEM->number.value;
	bool accepted = steeldrive2_ok(device, "$BS SET PID_TARGET:%.2f", target) && steeldrive2_ok(device, "$BS SET PID_DEW_OFS:%.2f", offset);
	double actual_target, actual_offset;
	bool readback = steeldrive2_get_number(device, "PID_TARGET", "PID_TARGET", -50, 50, false, &actual_target) && steeldrive2_get_number(device, "PID_DEW_OFS", "PID_DEW_OFS", -50, 50, false, &actual_offset);
	if (readback) {
		X_PID_SETTINGS_TARGET_ITEM->number.value = X_PID_SETTINGS_TARGET_ITEM->number.target = actual_target;
		X_PID_SETTINGS_OFS_ITEM->number.value = X_PID_SETTINGS_OFS_ITEM->number.target = actual_offset;
	}
	if (!accepted || !readback || actual_target != target || actual_offset != offset) {
		X_PID_SETTINGS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.X_PID_SETTINGS.on_change
	indigo_update_property(device, X_PID_SETTINGS_PROPERTY, NULL);
}

static void aux_x_select_pid_sensor_handler(indigo_device *device) {
	X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_SELECT_PID_SENSOR.on_change
	int requested = X_SELECT_PID_SENSOR_0_ITEM->sw.value ? 0 : X_SELECT_PID_SENSOR_1_ITEM->sw.value ? 1 : 2;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET PID_SENSOR:%d", requested) || !steeldrive2_get_switch(device, "PID_SENSOR", "PID_SENSOR", 2, &actual) || actual != requested) {
		X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "PID_SENSOR", "PID_SENSOR", 2, &actual)) {
		steeldrive2_set_switch(X_SELECT_PID_SENSOR_PROPERTY, actual);
	}
	//- aux.X_SELECT_PID_SENSOR.on_change
	indigo_update_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
}

static void aux_x_select_amb_sensor_handler(indigo_device *device) {
	X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_SELECT_AMB_SENSOR.on_change
	int requested = X_SELECT_AMB_SENSOR_0_ITEM->sw.value ? 0 : 1;
	int actual;
	if (!steeldrive2_ok(device, "$BS SET AMBIENT_SENSOR:%d", requested) || !steeldrive2_get_switch(device, "AMBIENT_SENSOR", "AMBIENT_SENSOR", 1, &actual) || actual != requested) {
		X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (steeldrive2_get_switch(device, "AMBIENT_SENSOR", "AMBIENT_SENSOR", 1, &actual)) {
		steeldrive2_set_switch(X_SELECT_AMB_SENSOR_PROPERTY, actual);
	}
	//- aux.X_SELECT_AMB_SENSOR.on_change
	indigo_update_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, "Heating", "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater outlet [%]", 0, 100, 1, 0);
		X_USE_AUTO_DEW_PROPERTY = indigo_init_switch_property(NULL, device->name, X_USE_AUTO_DEW_PROPERTY_NAME, "Heating", "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_USE_AUTO_DEW_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		X_USE_PID_PROPERTY = indigo_init_switch_property(NULL, device->name, X_USE_PID_PROPERTY_NAME, "Heating", "PID control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_USE_PID_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_USE_PID_DISABLED_ITEM, X_USE_PID_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(X_USE_PID_ENABLED_ITEM, X_USE_PID_ENABLED_ITEM_NAME, "Enabled", false);
		X_PID_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, X_PID_SETTINGS_PROPERTY_NAME, "Heating", "Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_PID_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_PID_SETTINGS_OFS_ITEM, X_PID_SETTINGS_OFS_ITEM_NAME, "PID offset", -50, 50, 0.01, 0);
		indigo_init_number_item(X_PID_SETTINGS_TARGET_ITEM, X_PID_SETTINGS_TARGET_ITEM_NAME, "PID target", -50, 50, 0.01, 0);
		X_SELECT_PID_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SELECT_PID_SENSOR_PROPERTY_NAME, "Heating", "PID sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_SELECT_PID_SENSOR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SELECT_PID_SENSOR_0_ITEM, X_SELECT_PID_SENSOR_0_ITEM_NAME, "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_PID_SENSOR_1_ITEM, X_SELECT_PID_SENSOR_1_ITEM_NAME, "Sensor #1", false);
		indigo_init_switch_item(X_SELECT_PID_SENSOR_AVG_ITEM, X_SELECT_PID_SENSOR_AVG_ITEM_NAME, "Average", true);
		X_SELECT_AMB_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SELECT_AMB_SENSOR_PROPERTY_NAME, "Heating", "Ambient sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_SELECT_AMB_SENSOR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SELECT_AMB_SENSOR_0_ITEM, X_SELECT_AMB_SENSOR_0_ITEM_NAME, "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_AMB_SENSOR_1_ITEM, X_SELECT_AMB_SENSOR_1_ITEM_NAME, "Sensor #1", true);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_USE_AUTO_DEW_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_USE_PID_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PID_SETTINGS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SELECT_PID_SENSOR_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SELECT_AMB_SENSOR_PROPERTY);
	}
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
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_USE_AUTO_DEW_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_USE_AUTO_DEW_PROPERTY, aux_x_use_auto_dew_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_USE_PID_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_USE_PID_PROPERTY, aux_x_use_pid_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PID_SETTINGS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PID_SETTINGS_PROPERTY, aux_x_pid_settings_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SELECT_PID_SENSOR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SELECT_PID_SENSOR_PROPERTY, aux_x_select_pid_sensor_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SELECT_AMB_SENSOR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SELECT_AMB_SENSOR_PROPERTY, aux_x_select_amb_sensor_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(X_USE_AUTO_DEW_PROPERTY);
	indigo_release_property(X_USE_PID_PROPERTY);
	indigo_release_property(X_PID_SETTINGS_PROPERTY);
	indigo_release_property(X_SELECT_PID_SENSOR_PROPERTY);
	indigo_release_property(X_SELECT_AMB_SENSOR_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_focuser_steeldrive2(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static steeldrive2_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (steeldrive2_private_data *)indigo_safe_malloc(sizeof(steeldrive2_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = focuser;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
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
