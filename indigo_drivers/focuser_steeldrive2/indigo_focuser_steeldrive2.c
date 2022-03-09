// Copyright (c) 2019 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Baader Planetarium SteelDriverII focuser driver
 \file indigo_focuser_steeldrive2.c
 */

#define DRIVER_VERSION 0x000B
#define DRIVER_NAME "indigo_focuser_steeldrive2"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_steeldrive2.h"

#define PRIVATE_DATA									((steeldrive2_private_data *)device->private_data)

#define USE_CRC

#define X_NAME_PROPERTY								(PRIVATE_DATA->x_name_property)
#define X_NAME_ITEM										(X_NAME_PROPERTY->items + 0)

#define X_SAVED_VALUES_PROPERTY				(PRIVATE_DATA->x_saved_values_property)
#define X_SAVED_FOCUS_ITEM						(X_SAVED_VALUES_PROPERTY->items + 0)
#define X_SAVED_JOGSTEPS_ITEM					(X_SAVED_VALUES_PROPERTY->items + 1)
#define X_SAVED_SINGLESTEPS_ITEM			(X_SAVED_VALUES_PROPERTY->items + 2)
#define X_SAVED_BKLGT_ITEM						(X_SAVED_VALUES_PROPERTY->items + 3)
#define X_SAVED_TEMP0_OFS_ITEM				(X_SAVED_VALUES_PROPERTY->items + 4)
#define X_SAVED_TEMP1_OFS_ITEM				(X_SAVED_VALUES_PROPERTY->items + 5)

#define X_STATUS_PROPERTY							(PRIVATE_DATA->x_status_property)
#define X_STATUS_SENSOR_0_ITEM				(X_STATUS_PROPERTY->items + 0)
#define X_STATUS_SENSOR_1_ITEM				(X_STATUS_PROPERTY->items + 1)

#define X_SELECT_TC_SENSOR_PROPERTY		(PRIVATE_DATA->x_select_tc_sensor_property)
#define X_SELECT_TC_SENSOR_0_ITEM			(X_SELECT_TC_SENSOR_PROPERTY->items + 0)
#define X_SELECT_TC_SENSOR_1_ITEM			(X_SELECT_TC_SENSOR_PROPERTY->items + 1)
#define X_SELECT_TC_SENSOR_AVG_ITEM		(X_SELECT_TC_SENSOR_PROPERTY->items + 2)

#define X_RESET_PROPERTY							(PRIVATE_DATA->x_reset_property)
#define X_RESET_ITEM									(X_RESET_PROPERTY->items + 0)
#define X_REBOOT_ITEM									(X_RESET_PROPERTY->items + 1)

#define X_USE_ENDSTOP_PROPERTY				(PRIVATE_DATA->x_use_endstop_property)
#define X_USE_ENDSTOP_DISABLED_ITEM		(X_USE_ENDSTOP_PROPERTY->items + 0)
#define X_USE_ENDSTOP_ENABLED_ITEM		(X_USE_ENDSTOP_PROPERTY->items + 1)

#define X_START_ZEROING_PROPERTY			(PRIVATE_DATA->x_start_zeroing_property)
#define X_START_ZEROING_ITEM					(X_START_ZEROING_PROPERTY->items + 0)

#define AUX_HEATER_OUTLET_PROPERTY		(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM			(AUX_HEATER_OUTLET_PROPERTY->items + 0)

#define AUX_DEW_CONTROL_PROPERTY			(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM		(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM	(AUX_DEW_CONTROL_PROPERTY->items + 1)

#define X_USE_PID_PROPERTY						(PRIVATE_DATA->x_use_pid_property)
#define X_USE_PID_DISABLED_ITEM				(X_USE_PID_PROPERTY->items + 0)
#define X_USE_PID_ENABLED_ITEM				(X_USE_PID_PROPERTY->items + 1)

#define X_PID_SETTINGS_PROPERTY				(PRIVATE_DATA->x_pid_settings_property)
#define X_PID_SETTINGS_TARGET_ITEM		(X_PID_SETTINGS_PROPERTY->items + 0)
#define X_PID_SETTINGS_OFS_ITEM				(X_PID_SETTINGS_PROPERTY->items + 1)

#define X_SELECT_PID_SENSOR_PROPERTY	(PRIVATE_DATA->x_select_pid_sensor_property)
#define X_SELECT_PID_SENSOR_0_ITEM		(X_SELECT_PID_SENSOR_PROPERTY->items + 0)
#define X_SELECT_PID_SENSOR_1_ITEM		(X_SELECT_PID_SENSOR_PROPERTY->items + 1)
#define X_SELECT_PID_SENSOR_AVG_ITEM	(X_SELECT_PID_SENSOR_PROPERTY->items + 2)

#define X_SELECT_AMB_SENSOR_PROPERTY	(PRIVATE_DATA->x_select_amb_sensor_property)
#define X_SELECT_AMB_SENSOR_0_ITEM		(X_SELECT_AMB_SENSOR_PROPERTY->items + 0)
#define X_SELECT_AMB_SENSOR_1_ITEM		(X_SELECT_AMB_SENSOR_PROPERTY->items + 1)

typedef struct {
	int handle;
	indigo_property *x_name_property;
	indigo_property *x_saved_values_property;
	indigo_property *x_status_property;
	indigo_property *x_select_tc_sensor_property;
	indigo_property *x_reset_property;
	indigo_property *x_use_endstop_property;
	indigo_property *x_start_zeroing_property;
	indigo_property *heating_mode_property;
	indigo_property *heater_outlet_property;
	indigo_property *x_use_pid_property;
	indigo_property *x_pid_settings_property;
	indigo_property *x_select_pid_sensor_property;
	indigo_property *x_select_amb_sensor_property;
	pthread_mutex_t mutex;
	indigo_timer *timer;
	bool moving;
	bool use_crc;
	int count;
	indigo_device *focuser;
	indigo_device *aux;
} steeldrive2_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

#ifdef USE_CRC

const uint8_t crc_array[256] = {
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
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

#endif

static bool steeldrive2_command(indigo_device *device, char *command, char *response, int length) {
	char tmp[1024], *crc;
#ifdef USE_CRC
	unsigned char c, remote = 0, local = 0;
	if (PRIVATE_DATA->use_crc) {
		crc = command;
		while (*crc) {
			local = crc_array[*crc++ ^ local];
		}
		indigo_printf(PRIVATE_DATA->handle, "%s*%02x\r\n", command, local);
	} else {
		indigo_printf(PRIVATE_DATA->handle, "%s\r\n", command);
	}
#else
	indigo_printf(PRIVATE_DATA->handle, "%s\r\n", command);
#endif
	int len = (int)strlen(command);
	while (true) {
		if (indigo_read_line(PRIVATE_DATA->handle, tmp, sizeof(tmp)) < 0) {
			return false;
		}
		if (!strncmp(command, tmp, len)) {
			break;
		}
	}
	while (true) {
		if (indigo_read_line(PRIVATE_DATA->handle, tmp, sizeof(tmp)) < 0) {
			return false;
		}
		if (!strncmp("$BS DEBUG:", tmp, 10)) {
			continue;
		}
		if (strncmp("$BS", tmp, 3)) {
			continue;
		}
		if ((crc = strchr(tmp, '*'))) {
			*crc++ = 0;
#ifdef USE_CRC
			while ((c = *crc++)) {
				if (c >= 'a') {
					remote = remote * 16 + c - 'a' + 10;
				} else if (c >= 'A') {
					remote = remote * 16 + c - 'A' + 10;
				} else {
					remote = remote * 16 + c - '0';
				}
			}
			crc = tmp;
			local = 0;
			while (*crc) {
				local = crc_array[*crc++ ^ local];
			}
			if (local != remote) {
				return false;
			}
#endif
		}
		strncpy(response, tmp, length);
		break;
	}
	return true;
}

static void steeldrive2_connect(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
	if (PRIVATE_DATA->handle > 0) {
		PRIVATE_DATA->use_crc = false;
		char response[256], *colon;
		for (int i = 0; i < 3; i++) {
			if (indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && !strcmp(response, "$BS Hello World!")) {
				if (steeldrive2_command(device, "$BS GET VERSION", response, sizeof(response)) && (colon = strchr(response, ':'))) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Baader Planetarium SteelDriveII");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, colon + 1);
					indigo_update_property(device, INFO_PROPERTY, NULL);
#ifdef USE_CRC
					if (steeldrive2_command(device, "$BS CRC_ENABLE", response, sizeof(response))) {
						PRIVATE_DATA->use_crc = true;
					}
#endif
					return;
				}
			}
			indigo_usleep(100000);
		}
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
	}
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbserial", 17)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// -------------------------------------------------------------------------------- X_NAME
		X_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, "X_NAME", "Advanced", "Device name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_NAME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(X_NAME_ITEM, "NAME", "Name", "");
		// -------------------------------------------------------------------------------- X_SAVED_VALUES
		X_SAVED_VALUES_PROPERTY = indigo_init_number_property(NULL, device->name, "X_SAVED_VALUES", "Advanced", "Saved values", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
		if (X_SAVED_VALUES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_SAVED_FOCUS_ITEM, "FOCUS", "Saved focus", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(X_SAVED_JOGSTEPS_ITEM, "JOGSTEPS", "Jogging mode steps", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(X_SAVED_SINGLESTEPS_ITEM, "SINGLESTEPS", "Single mode steps", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(X_SAVED_BKLGT_ITEM, "BKLGT", "Backlight brightness", 0, 100, 10, 50);
		indigo_init_number_item(X_SAVED_TEMP0_OFS_ITEM, "TEMP0_OFS", "Sensor #0 offset", -50, 50, 1, 0);
		indigo_init_number_item(X_SAVED_TEMP1_OFS_ITEM, "TEMP1_OFS", "Sensor #1 offset", -50, 50, 1, 0);
		// -------------------------------------------------------------------------------- X_STATUS
		X_STATUS_PROPERTY = indigo_init_number_property(NULL, device->name, "X_STATUS", "Advanced", "Status", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_STATUS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_STATUS_SENSOR_0_ITEM, "SENSOR_0", "Sensor #0", -100, 100, 0, 0);
		indigo_init_number_item(X_STATUS_SENSOR_1_ITEM, "SENSOR_1", "Sensor #1", -100, 100, 0, 0);
		// -------------------------------------------------------------------------------- X_SELECT_TC_SENSOR
		X_SELECT_TC_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_SELECT_TC_SENSOR", "Advanced", "TCOMP sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_SELECT_TC_SENSOR_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_SELECT_TC_SENSOR_0_ITEM, "SENSOR_0", "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_TC_SENSOR_1_ITEM, "SENSOR_1", "Sensor #1", false);
		indigo_init_switch_item(X_SELECT_TC_SENSOR_AVG_ITEM, "AVG", "Average", true);
		// -------------------------------------------------------------------------------- X_RESET
		X_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RESET", "Advanced", "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (X_RESET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_RESET_ITEM, "RESET", "Reset", false);
		indigo_init_switch_item(X_REBOOT_ITEM, "REBOOT", "Reboot", false);
		// -------------------------------------------------------------------------------- X_USE_ENDSTOP
		X_USE_ENDSTOP_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_USE_ENDSTOP", "Advanced", "Use end-stop sensor", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_USE_ENDSTOP_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_USE_ENDSTOP_DISABLED_ITEM, "DISABLED", "Disabled", true);
		indigo_init_switch_item(X_USE_ENDSTOP_ENABLED_ITEM, "ENABLED", "Enabled", false);
		// -------------------------------------------------------------------------------- X_START_ZEROING
		X_START_ZEROING_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_START_ZEROING", "Advanced", "Start zeroing", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_START_ZEROING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_START_ZEROING_ITEM, "START", "Start", false);
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 0xFFFF;
		FOCUSER_STEPS_ITEM->number.step = 1;
			// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 0xFFFF;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 0xFFFF;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->count = 3;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_NAME_PROPERTY, property))
			indigo_define_property(device, X_NAME_PROPERTY, NULL);
		if (indigo_property_match(X_SAVED_VALUES_PROPERTY, property))
			indigo_define_property(device, X_SAVED_VALUES_PROPERTY, NULL);
		if (indigo_property_match(X_STATUS_PROPERTY, property))
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);
		if (indigo_property_match(X_SELECT_TC_SENSOR_PROPERTY, property))
			indigo_define_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
		if (indigo_property_match(X_RESET_PROPERTY, property))
			indigo_define_property(device, X_RESET_PROPERTY, NULL);
		if (indigo_property_match(X_USE_ENDSTOP_PROPERTY, property))
			indigo_define_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
		if (indigo_property_match(X_START_ZEROING_PROPERTY, property))
			indigo_define_property(device, X_START_ZEROING_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle == 0)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	device = PRIVATE_DATA->focuser;
	char response[256], *value;
	bool status_update = false;
	if (steeldrive2_command(device, "$BS SUMMARY", response, sizeof(response))) {
		char *pnt, *token = strtok_r(response, ";", &pnt);
		while (token) {
			if ((value = strchr(token, ':'))) {
				*value++ = 0;
				if (!strcmp(token, "STATE")) {
					if (!strcmp(value, "STOPPED")) {
						PRIVATE_DATA->moving = false;
					} else {
						PRIVATE_DATA->moving = true;
					}
				} else if (!strcmp(token, "POS")) {
					int tmp = atoi(value);
					if (FOCUSER_POSITION_ITEM->number.value != tmp) {
						FOCUSER_POSITION_ITEM->number.value = tmp;
						indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
					}
				} else if (!strcmp(token, "LIMIT")) {
					int tmp = atoi(value);
					if (FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value != tmp) {
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max = tmp;
						indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
					}
				} else if (!strcmp(token, "TEMP0")) {
					double tmp = indigo_atod(value);
					if (X_STATUS_SENSOR_0_ITEM->number.value != tmp) {
						X_STATUS_SENSOR_0_ITEM->number.value = tmp;
						status_update = true;
					}
				} else if (!strcmp(token, "TEMP1")) {
					double tmp = indigo_atod(value);
					if (X_STATUS_SENSOR_1_ITEM->number.value != tmp) {
						X_STATUS_SENSOR_1_ITEM->number.value = tmp;
						status_update = true;
					}
				} else if (!strcmp(token, "PWM")) {
					indigo_device *aux = PRIVATE_DATA->focuser;
					indigo_device *device = aux;
					double tmp = indigo_atod(value);
					if (AUX_HEATER_OUTLET_1_ITEM->number.value != tmp) {
						AUX_HEATER_OUTLET_1_ITEM->number.value = tmp;
						indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
					}
				} else if (!strcmp(token, "TEMP_AVG")) {
					double tmp = indigo_atod(value);
					if (FOCUSER_TEMPERATURE_ITEM->number.value != tmp) {
						FOCUSER_TEMPERATURE_ITEM->number.value = tmp;
						indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
					}
				}
			}
			token = strtok_r(NULL, ";", &pnt);
		}
		if (status_update) {
			indigo_update_property(device, X_STATUS_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->moving && FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else if (!PRIVATE_DATA->moving && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
	indigo_set_timer(device, PRIVATE_DATA->moving ? 0.1 : 0.5, focuser_timer_callback, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[256];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0)
			steeldrive2_connect(device);
		if (PRIVATE_DATA->handle > 0) {
			int value;
			if (steeldrive2_command(device, "$BS GET NAME", response, sizeof(response)) && sscanf(response, "$BS STATUS NAME:%s", X_NAME_ITEM->text.value) == 1) {
				X_NAME_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				X_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_NAME_PROPERTY, NULL);
			X_SAVED_VALUES_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET FOCUS", response, sizeof(response)) && sscanf(response, "$BS STATUS FOCUS:%lg", &X_SAVED_FOCUS_ITEM->number.value) == 1) {
				X_SAVED_FOCUS_ITEM->number.target = X_SAVED_FOCUS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET JOGSTEPS", response, sizeof(response)) && sscanf(response, "$BS STATUS JOGSTEPS:%lg", &X_SAVED_JOGSTEPS_ITEM->number.value) == 1) {
				X_SAVED_JOGSTEPS_ITEM->number.target = X_SAVED_JOGSTEPS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET SINGLESTEPS", response, sizeof(response)) && sscanf(response, "$BS STATUS SINGLESTEPS:%lg", &X_SAVED_SINGLESTEPS_ITEM->number.value) == 1) {
				X_SAVED_SINGLESTEPS_ITEM->number.target = X_SAVED_SINGLESTEPS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET BKLGT", response, sizeof(response)) && sscanf(response, "$BS STATUS BKLGT:%lg", &X_SAVED_BKLGT_ITEM->number.value) == 1) {
				X_SAVED_BKLGT_ITEM->number.target = X_SAVED_BKLGT_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET TEMP0_OFS", response, sizeof(response)) && sscanf(response, "$BS STATUS TEMP0_OFS:%lg", &X_SAVED_TEMP0_OFS_ITEM->number.value) == 1) {
				X_SAVED_TEMP0_OFS_ITEM->number.target = X_SAVED_TEMP0_OFS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET TEMP1_OFS", response, sizeof(response)) && sscanf(response, "$BS STATUS TEMP1_OFS:%lg", &X_SAVED_TEMP1_OFS_ITEM->number.value) == 1) {
				X_SAVED_TEMP1_OFS_ITEM->number.target = X_SAVED_TEMP1_OFS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_SAVED_VALUES_PROPERTY, NULL);
			FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET TCOMP", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP:%d", &value) == 1) {
				if (value)
					indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_AUTOMATIC_ITEM, true);
				else
					indigo_set_switch(FOCUSER_MODE_PROPERTY, FOCUSER_MODE_MANUAL_ITEM, true);
			} else {
				FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET TCOMP_FACTOR", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP_FACTOR:%lg", &FOCUSER_COMPENSATION_ITEM->number.value) == 1) {
				FOCUSER_COMPENSATION_ITEM->number.target = FOCUSER_COMPENSATION_ITEM->number.value;
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET TCOMP_PERIOD", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP_PERIOD:%lg", &FOCUSER_COMPENSATION_PERIOD_ITEM->number.value) == 1) {
				FOCUSER_COMPENSATION_PERIOD_ITEM->number.target = FOCUSER_COMPENSATION_PERIOD_ITEM->number.value = FOCUSER_COMPENSATION_PERIOD_ITEM->number.value / 1000;
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET TCOMP_DELTA", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP_DELTA:%lg", &FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) == 1) {
				FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value;
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET TCOMP_DELTA", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP_DELTA:%lg", &FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) == 1) {
				FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.target = FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value;
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET TCOMP_SENSOR", response, sizeof(response)) && sscanf(response, "$BS STATUS TCOMP_SENSOR:%d", &value) == 1) {
				if (value == 0)
					indigo_set_switch(X_SELECT_TC_SENSOR_PROPERTY, X_SELECT_TC_SENSOR_0_ITEM, true);
				else if (value == 1)
					indigo_set_switch(X_SELECT_TC_SENSOR_PROPERTY, X_SELECT_TC_SENSOR_1_ITEM, true);
				else
					indigo_set_switch(X_SELECT_TC_SENSOR_PROPERTY, X_SELECT_TC_SENSOR_AVG_ITEM, true);
			} else {
				X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
			X_USE_ENDSTOP_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET USE_ENDSTOP", response, sizeof(response)) && sscanf(response, "$BS STATUS USE_ENDSTOP:%d", &value) == 1) {
				if (value)
					indigo_set_switch(X_USE_ENDSTOP_PROPERTY, X_USE_ENDSTOP_ENABLED_ITEM, true);
				else
					indigo_set_switch(X_USE_ENDSTOP_PROPERTY, X_USE_ENDSTOP_DISABLED_ITEM, true);
			} else {
				X_USE_ENDSTOP_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
			indigo_define_property(device, X_STATUS_PROPERTY, NULL);
			indigo_define_property(device, X_RESET_PROPERTY, NULL);
			indigo_define_property(device, X_START_ZEROING_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count == 1)
				indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count = 0;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			steeldrive2_command(device, "$BS STOP", response, sizeof(response));
			indigo_delete_property(device, X_NAME_PROPERTY, NULL);
			indigo_delete_property(device, X_SAVED_VALUES_PROPERTY, NULL);
			indigo_delete_property(device, X_STATUS_PROPERTY, NULL);
			indigo_delete_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
			indigo_delete_property(device, X_RESET_PROPERTY, NULL);
			indigo_delete_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
			indigo_delete_property(device, X_START_ZEROING_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			if (--PRIVATE_DATA->count == 0) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	int position = FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		if (position < 0)
			position = 0;
		else if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
			position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		sprintf(command, "$BS GO %d", position);
	} else {
		sprintf(command, "$BS SET POS:%d", position);
	}
	if (steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	else
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	int position = FOCUSER_POSITION_ITEM->number.value + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1) * (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -1 : 1) * FOCUSER_STEPS_ITEM->number.target;
	if (position < 0)
		position = 0;
	else if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	sprintf(command, "$BS GO %d", position);
	if (steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	else
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[256];
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		if (steeldrive2_command(device, "$BS STOP", response, sizeof(response)) && !strcmp(response, "$BS OK"))
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		else
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_name_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	sprintf(command, "$BS SET NAME:%s", X_NAME_ITEM->text.value);
	if (steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_NAME_PROPERTY->state = INDIGO_OK_STATE;
	else
		X_NAME_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_NAME_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_saved_values_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_SAVED_VALUES_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET FOCUS:%d", (int)X_SAVED_FOCUS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET JOGSTEPS:%d", (int)X_SAVED_JOGSTEPS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET SINGLESTEPS:%d", (int)X_SAVED_SINGLESTEPS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET BKLGT:%d", (int)X_SAVED_BKLGT_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET TEMP0_OFS:%d", (int)X_SAVED_TEMP0_OFS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET TEMP1_OFS:%d", (int)X_SAVED_TEMP1_OFS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET PID_DEW_OFS:%d", (int)X_PID_SETTINGS_OFS_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET PID_TARGET:%d", (int)X_PID_SETTINGS_TARGET_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SAVED_VALUES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_mode_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET TCOMP:%d", FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? 1 : 0);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_compensation_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET TCOMP_FACTOR:%.2f", FOCUSER_COMPENSATION_ITEM->number.value);
	indigo_fix_locale(command);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET TCOMP_PERIOD:%d", (int)(FOCUSER_COMPENSATION_PERIOD_ITEM->number.value * 1000));
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	sprintf(command, "$BS SET TCOMP_DELTA:%.1f", FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value);
	indigo_fix_locale(command);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_limits_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET LIMIT:%d", (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_tc_sensor_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET TCOMP_SENSOR:%d", X_SELECT_TC_SENSOR_0_ITEM->sw.value ? 0 : X_SELECT_TC_SENSOR_1_ITEM->sw.value ? 1 : 2);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_reset_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[256];
	X_RESET_PROPERTY->state = INDIGO_OK_STATE;
	if (X_RESET_ITEM->sw.value) {
		X_RESET_ITEM->sw.value = false;
		if (steeldrive2_command(device, "$BS RESET", response, sizeof(response)) && !strcmp(response, "$BS OK")) {
			indigo_device_disconnect(NULL, device->name);
			return;
		}
	} else if (X_REBOOT_ITEM->sw.value) {
		X_REBOOT_ITEM->sw.value = false;
		indigo_printf(PRIVATE_DATA->handle, "$BS REBOOT\r\n");
		indigo_device_disconnect(NULL, device->name);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	X_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_RESET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_use_endstop_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_USE_ENDSTOP_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET USE_ENDSTOP:%d", X_USE_ENDSTOP_ENABLED_ITEM->sw.value ? 1 : 0);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_USE_ENDSTOP_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_start_zeroing_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[256];
	X_START_ZEROING_PROPERTY->state = INDIGO_OK_STATE;
	if (X_START_ZEROING_ITEM->sw.value) {
		X_START_ZEROING_ITEM->sw.value = false;
		if (!steeldrive2_command(device, "$BS ZEROING", response, sizeof(response)) && !strcmp(response, "$BS OK"))
			X_START_ZEROING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_START_ZEROING_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		int value = FOCUSER_POSITION_ITEM->number.value;
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_ITEM->number.value = value;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_NAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_NAME
		indigo_property_copy_values(X_NAME_PROPERTY, property, false);
		X_NAME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_NAME_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_name_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_SAVED_VALUES_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- X_SAVED_VALUES
		indigo_property_copy_values(X_SAVED_VALUES_PROPERTY, property, false);
		X_SAVED_VALUES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SAVED_VALUES_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_saved_values_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_mode_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		if (IS_CONNECTED) {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_compensation_handler, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
	} else if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		if (IS_CONNECTED) {
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_limits_handler, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_SELECT_TC_SENSOR
	} else if (indigo_property_match(X_SELECT_TC_SENSOR_PROPERTY, property)) {
		indigo_property_copy_values(X_SELECT_TC_SENSOR_PROPERTY, property, false);
		X_SELECT_TC_SENSOR_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SELECT_TC_SENSOR_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_tc_sensor_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_RESET
	} else if (indigo_property_match(X_RESET_PROPERTY, property)) {
		indigo_property_copy_values(X_RESET_PROPERTY, property, false);
		X_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RESET_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_reset_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_USE_ENDSTOP
	} else if (indigo_property_match(X_USE_ENDSTOP_PROPERTY, property)) {
		indigo_property_copy_values(X_USE_ENDSTOP_PROPERTY, property, false);
		X_USE_ENDSTOP_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_USE_ENDSTOP_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_use_endstop_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_START_ZEROING
	} else if (indigo_property_match(X_START_ZEROING_PROPERTY, property)) {
		indigo_property_copy_values(X_START_ZEROING_PROPERTY, property, false);
		X_START_ZEROING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_START_ZEROING_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_start_zeroing_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
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
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbserial", 17)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// -------------------------------------------------------------------------------- HEATER OUTLETS
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, "Heating", "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater outlet [%]", 0, 100, 5, 0);
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_USE_AUTO_DEW", "Heating", "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		// -------------------------------------------------------------------------------- X_USE_PID
		X_USE_PID_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_USE_PID", "Heating", "PID control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_USE_PID_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_USE_PID_DISABLED_ITEM, "DISABLED", "Disabled", true);
		indigo_init_switch_item(X_USE_PID_ENABLED_ITEM, "ENABLED", "Enabled", false);
		// -------------------------------------------------------------------------------- X_PID_SETTINGS
		X_PID_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, "X_PID_SETTINGS", "Heating", "Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_SAVED_VALUES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_PID_SETTINGS_OFS_ITEM, "PID_DEW_OFS", "PID offset", -50, 50, 1, 0);
		indigo_init_number_item(X_PID_SETTINGS_TARGET_ITEM, "PID TARGET", "PID target", -50, 50, 1, 0);
		// -------------------------------------------------------------------------------- X_SELECT_PID_SENSOR
		X_SELECT_PID_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_SELECT_PID_SENSOR", "Heating", "PID sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_SELECT_PID_SENSOR_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_SELECT_PID_SENSOR_0_ITEM, "SENSOR_0", "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_PID_SENSOR_1_ITEM, "SENSOR_1", "Sensor #1", false);
		indigo_init_switch_item(X_SELECT_PID_SENSOR_AVG_ITEM, "AVG", "Average", true);
		// -------------------------------------------------------------------------------- X_SELECT_AMB_SENSOR
		X_SELECT_AMB_SENSOR_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_SELECT_AMB_SENSOR", "Heating", "Ambient sensor selection", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_SELECT_AMB_SENSOR_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_SELECT_AMB_SENSOR_0_ITEM, "SENSOR_0", "Sensor #0", false);
		indigo_init_switch_item(X_SELECT_AMB_SENSOR_1_ITEM, "SENSOR_1", "Sensor #1", true);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(X_USE_PID_PROPERTY, property))
			indigo_define_property(device, X_USE_PID_PROPERTY, NULL);
		if (indigo_property_match(X_PID_SETTINGS_PROPERTY, property))
			indigo_define_property(device, X_PID_SETTINGS_PROPERTY, NULL);
		if (indigo_property_match(X_SELECT_PID_SENSOR_PROPERTY, property))
			indigo_define_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
		if (indigo_property_match(X_SELECT_AMB_SENSOR_PROPERTY, property))
			indigo_define_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[256];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->count++ == 0)
			steeldrive2_connect(device);
		if (PRIVATE_DATA->handle > 0) {
			int value;
			X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET PID_CTRL", response, sizeof(response)) && sscanf(response, "$BS STATUS PID_CTRL:%d", &value) == 1) {
				if (value)
					indigo_set_switch(X_USE_PID_PROPERTY, X_USE_PID_ENABLED_ITEM, true);
				else
					indigo_set_switch(X_USE_PID_PROPERTY, X_USE_PID_DISABLED_ITEM, true);
			} else {
				X_USE_PID_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_USE_PID_PROPERTY, NULL);
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET PWM", response, sizeof(response)) && sscanf(response, "$BS STATUS PWM:%lg", &AUX_HEATER_OUTLET_1_ITEM->number.value) == 1) {
				AUX_HEATER_OUTLET_1_ITEM->number.target = AUX_HEATER_OUTLET_1_ITEM->number.value;
			} else {
				AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET AUTO_DEW", response, sizeof(response)) && sscanf(response, "$BS STATUS AUTO_DEW:%d", &value) == 1) {
				if (value)
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
				else
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
			} else {
				AUX_DEW_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET PID_SENSOR", response, sizeof(response)) && sscanf(response, "$BS STATUS PID_SENSOR:%d", &value) == 1) {
				if (value == 0)
					indigo_set_switch(X_SELECT_PID_SENSOR_PROPERTY, X_SELECT_PID_SENSOR_0_ITEM, true);
				else if (value == 1)
					indigo_set_switch(X_SELECT_PID_SENSOR_PROPERTY, X_SELECT_PID_SENSOR_1_ITEM, true);
				else
					indigo_set_switch(X_SELECT_PID_SENSOR_PROPERTY, X_SELECT_PID_SENSOR_AVG_ITEM, true);
			} else {
				X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
			X_PID_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET PID_DEW_OFS", response, sizeof(response)) && sscanf(response, "$BS STATUS PID_DEW_OFS:%lg", &X_PID_SETTINGS_OFS_ITEM->number.value) == 1) {
				X_PID_SETTINGS_OFS_ITEM->number.target = X_PID_SETTINGS_OFS_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (steeldrive2_command(device, "$BS GET PID_TARGET", response, sizeof(response)) && sscanf(response, "$BS STATUS PID_TARGET:%lg", &X_PID_SETTINGS_TARGET_ITEM->number.value) == 1) {
				X_PID_SETTINGS_TARGET_ITEM->number.target = X_PID_SETTINGS_TARGET_ITEM->number.value;
			} else {
				X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_PID_SETTINGS_PROPERTY, NULL);
			X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
			if (steeldrive2_command(device, "$BS GET AMBIENT_SENSOR", response, sizeof(response)) && sscanf(response, "$BS STATUS AMBIENT_SENSOR:%d", &value) == 1) {
				if (value == 0)
					indigo_set_switch(X_SELECT_AMB_SENSOR_PROPERTY, X_SELECT_AMB_SENSOR_0_ITEM, true);
				else
					indigo_set_switch(X_SELECT_AMB_SENSOR_PROPERTY, X_SELECT_AMB_SENSOR_1_ITEM, true);
			} else {
				X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count == 1)
				indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count = 0;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_delete_property(device, X_USE_PID_PROPERTY, NULL);
			indigo_delete_property(device, X_PID_SETTINGS_PROPERTY, NULL);
			indigo_delete_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
			indigo_delete_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			if (--PRIVATE_DATA->count == 0) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}
static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	sprintf(command, "$BS SET PWM:%d", (int)AUX_HEATER_OUTLET_1_ITEM->number.value);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SAVED_VALUES_PROPERTY->state = INDIGO_ALERT_STATE;
	X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
	int value;
	if (steeldrive2_command(device, "$BS GET PID_CTRL", response, sizeof(response)) && sscanf(response, "$BS STATUS PID_CTRL:%d", &value) == 1) {
		if (value)
			indigo_set_switch(X_USE_PID_PROPERTY, X_USE_PID_ENABLED_ITEM, true);
		else
			indigo_set_switch(X_USE_PID_PROPERTY, X_USE_PID_DISABLED_ITEM, true);
	} else {
		X_USE_PID_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	if (steeldrive2_command(device, "$BS GET AUTO_DEW", response, sizeof(response)) && sscanf(response, "$BS STATUS AUTO_DEW:%d", &value) == 1) {
		if (value)
			indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
		else
			indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
	} else {
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_dew_control_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET PID_CTRL:%d", AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_use_pid_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET PID_CTRL:%d", X_USE_PID_ENABLED_ITEM->sw.value ? 1 : 0);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_USE_PID_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_pid_settings_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_USE_PID_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET PID_TARGET:%.2f", X_PID_SETTINGS_TARGET_ITEM->number.value);
	indigo_fix_locale(command);
	if (steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK")) {
		sprintf(command, "$BS SET PID_DEV_OFSL:%.2f",  X_PID_SETTINGS_OFS_ITEM->number.value);
		indigo_fix_locale(command);
		if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
			X_PID_SETTINGS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		X_PID_SETTINGS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_PID_SETTINGS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_select_pid_sensor_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET PID_SENSOR:%d", X_SELECT_PID_SENSOR_0_ITEM->sw.value ? 0 : X_SELECT_PID_SENSOR_1_ITEM->sw.value ? 1 : 2);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);

	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_select_amb_sensor_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[64], response[256];
	X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, "$BS SET AMBIENT_SENSOR:%d", X_SELECT_AMB_SENSOR_0_ITEM->sw.value ? 0 : 1);
	if (!steeldrive2_command(device, command, response, sizeof(response)) && !strcmp(response, "$BS OK"))
		X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
	} else if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_heater_outlet_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
	} else if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property)) {
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_dew_control_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_USE_PID
	} else if (indigo_property_match(X_USE_PID_PROPERTY, property)) {
		indigo_property_copy_values(X_USE_PID_PROPERTY, property, false);
		X_USE_PID_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_USE_PID_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_use_pid_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_PID_SETTINGS
	} else if (indigo_property_match(X_PID_SETTINGS_PROPERTY, property)) {
		indigo_property_copy_values(X_PID_SETTINGS_PROPERTY, property, false);
		X_PID_SETTINGS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_PID_SETTINGS_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_pid_settings_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_SELECT_PID_SENSOR
	} else if (indigo_property_match(X_SELECT_PID_SENSOR_PROPERTY, property)) {
		indigo_property_copy_values(X_SELECT_PID_SENSOR_PROPERTY, property, false);
		X_SELECT_PID_SENSOR_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SELECT_PID_SENSOR_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_select_pid_sensor_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_SELECT_AMB_SENSOR
	} else if (indigo_property_match(X_SELECT_AMB_SENSOR_PROPERTY, property)) {
		indigo_property_copy_values(X_SELECT_AMB_SENSOR_PROPERTY, property, false);
		X_SELECT_AMB_SENSOR_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SELECT_AMB_SENSOR_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_select_amb_sensor_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(X_USE_PID_PROPERTY);
	indigo_release_property(X_PID_SETTINGS_PROPERTY);
	indigo_release_property(X_SELECT_PID_SENSOR_PROPERTY);
	indigo_release_property(X_SELECT_AMB_SENSOR_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_steeldrive2(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static steeldrive2_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *aux = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"SteelDriveII (focuser)",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"SteelDriveII (aux)",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	
	SET_DRIVER_INFO(info, "Baader Planetarium SteelDriveII Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(steeldrive2_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			private_data->focuser = focuser;
			indigo_attach_device(focuser);
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = focuser;
			private_data->aux = aux;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
			}
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
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
