// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO WeMacro Rail driver
 \file indigo_ccd_sx.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_ccd_wemacro"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_focuser_wemacro.h"

#define PRIVATE_DATA													((wemacro_private_data *)device->private_data)

#define X_RAIL_BATCH												"Batch"

#define X_RAIL_CONFIG_PROPERTY							(PRIVATE_DATA->config_property)
#define X_RAIL_CONFIG_BACK_ITEM							(X_RAIL_CONFIG_PROPERTY->items+0)
#define X_RAIL_CONFIG_BEEP_ITEM							(X_RAIL_CONFIG_PROPERTY->items+1)
#define X_RAIL_CONFIG_MM_ITEM								(X_RAIL_CONFIG_PROPERTY->items+2)

#define X_RAIL_SHUTTER_PROPERTY							(PRIVATE_DATA->shutter_property)
#define X_RAIL_SHUTTER_ITEM									(X_RAIL_SHUTTER_PROPERTY->items+0)

#define X_RAIL_MOVE_AHEAD_PROPERTY					(PRIVATE_DATA->move_ahead_property)
#define X_RAIL_MOVE_AHEAD_SETTLE_TIME_ITEM	(X_RAIL_MOVE_AHEAD_PROPERTY->items+0)
#define X_RAIL_MOVE_AHEAD_PER_STEP_ITEM			(X_RAIL_MOVE_AHEAD_PROPERTY->items+1)
#define X_RAIL_MOVE_AHEAD_INTERVAL_ITEM			(X_RAIL_MOVE_AHEAD_PROPERTY->items+2)
#define X_RAIL_MOVE_AHEAD_LENGTH_ITEM				(X_RAIL_MOVE_AHEAD_PROPERTY->items+3)

#define X_RAIL_MOVE_BACK_PROPERTY						(PRIVATE_DATA->move_back_property)
#define X_RAIL_MOVE_BACK_SETTLE_TIME_ITEM		(X_RAIL_MOVE_BACK_PROPERTY->items+0)
#define X_RAIL_MOVE_BACK_PER_STEP_ITEM			(X_RAIL_MOVE_BACK_PROPERTY->items+1)
#define X_RAIL_MOVE_BACK_INTERVAL_ITEM			(X_RAIL_MOVE_BACK_PROPERTY->items+2)
#define X_RAIL_MOVE_BACK_LENGTH_ITEM				(X_RAIL_MOVE_BACK_PROPERTY->items+3)

#define X_RAIL_MOVE_STEPS_PROPERTY					(PRIVATE_DATA->move_steps_property)
#define X_RAIL_MOVE_STEPS_SETTLE_TIME_ITEM	(X_RAIL_MOVE_STEPS_PROPERTY->items+0)
#define X_RAIL_MOVE_STEPS_PER_STEP_ITEM			(X_RAIL_MOVE_STEPS_PROPERTY->items+1)
#define X_RAIL_MOVE_STEPS_INTERVAL_ITEM			(X_RAIL_MOVE_STEPS_PROPERTY->items+2)
#define X_RAIL_MOVE_STEPS_COUNT_ITEM				(X_RAIL_MOVE_STEPS_PROPERTY->items+3)


typedef struct {
	int handle;
	int device_count;
	pthread_t reader;
	pthread_mutex_t port_mutex;
	indigo_property *shutter_property;
	indigo_property *config_property;
	indigo_property *move_ahead_property;
	indigo_property *move_back_property;
	indigo_property *move_steps_property;
} wemacro_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static char *wemacro_reader(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "wemacro_reader started");
	while (PRIVATE_DATA->handle > 0) {
		uint8_t in[3] = { 0, 0, 0 };
		int result = indigo_read(PRIVATE_DATA->handle, (char *)in, sizeof(in));
		if (result == sizeof(in)) {
			INDIGO_DEBUG_DRIVER(indigo_debug("WeMacro > %02x %02x %02x", in[0], in[1], in[2]));
			if (in[2] == 0xf5 || in[2] == 0xf6) {
				if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				}
				if (X_RAIL_MOVE_AHEAD_PROPERTY->state == INDIGO_BUSY_STATE) {
					X_RAIL_MOVE_AHEAD_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, X_RAIL_MOVE_AHEAD_PROPERTY, NULL);
				}
				if (X_RAIL_MOVE_BACK_PROPERTY->state == INDIGO_BUSY_STATE) {
					X_RAIL_MOVE_BACK_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, X_RAIL_MOVE_BACK_PROPERTY, NULL);
				}
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "wemacro_reader finished");
	return NULL;
}

static bool wemacro_command(indigo_device *device, uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint32_t d) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = true;
	uint8_t out[12] = { 0xA5, 0x5A, cmd, a, b, c, (d >> 24 & 0xFF), (d >> 16 & 0xFF), (d >> 8 & 0xFF), (d & 0xFF) };
	uint16_t crc = 0xFFFF;
	for (int i = 0; i < 10; i++) {
		crc = crc ^ out[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 0x0001)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = crc >> 1;
		}
		out[10] = crc & 0xFF;
		out[11] = (crc >> 8) &0xFF;
		result = indigo_write(PRIVATE_DATA->handle, (char *)out, sizeof(out));
		INDIGO_DEBUG_DRIVER(indigo_debug("WeMacro < %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %s", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11], result ? "OK" : "Failed"));
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		FOCUSER_ROTATION_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_POSITION_PROPERTY->hidden = true;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "CH341")) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/wemacro_rail");
#endif
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		X_RAIL_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_CONFIG", X_RAIL_BATCH, "Configuration", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (X_RAIL_CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_RAIL_CONFIG_BACK_ITEM, "BACK", "Return back when done", false);
		indigo_init_switch_item(X_RAIL_CONFIG_BEEP_ITEM, "BEEP", "Beep when done", false);
		indigo_init_switch_item(X_RAIL_CONFIG_MM_ITEM, "MM", "Use milimeters (instead of microns)", false);
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		X_RAIL_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_SHUTTER", X_RAIL_BATCH, "Shutter", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_RAIL_SHUTTER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_RAIL_SHUTTER_ITEM, "SHUTTER", "Shutter", false);
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_AHEAD
		X_RAIL_MOVE_AHEAD_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RAIL_MOVE_AHEAD", X_RAIL_BATCH, "Move ahead", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
		if (X_RAIL_MOVE_AHEAD_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_RAIL_MOVE_AHEAD_SETTLE_TIME_ITEM, "SETTLE_TIME", "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_AHEAD_PER_STEP_ITEM, "SHUTTER_PER_STEP", "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_AHEAD_INTERVAL_ITEM, "SHUTTER_INTERVAL", "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_AHEAD_LENGTH_ITEM, "LENTH", "Length", 1, 0xFFFFFF, 1, 1);
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_BACK
		X_RAIL_MOVE_BACK_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RAIL_MOVE_BACK", X_RAIL_BATCH, "Move back", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
		if (X_RAIL_MOVE_BACK_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_RAIL_MOVE_BACK_SETTLE_TIME_ITEM, "SETTLE_TIME", "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_BACK_PER_STEP_ITEM, "SHUTTER_PER_STEP", "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_BACK_INTERVAL_ITEM, "SHUTTER_INTERVAL", "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_BACK_LENGTH_ITEM, "LENTH", "Length", 1, 0xFFFFFF, 1, 1);
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_STEPS
		X_RAIL_MOVE_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RAIL_MOVE_STEPS", X_RAIL_BATCH, "Move steps", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
		if (X_RAIL_MOVE_STEPS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_RAIL_MOVE_STEPS_SETTLE_TIME_ITEM, "SETTLE_TIME", "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_STEPS_PER_STEP_ITEM, "SHUTTER_PER_STEP", "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_STEPS_INTERVAL_ITEM, "SHUTTER_INTERVAL", "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_MOVE_STEPS_COUNT_ITEM, "COUNT", "Count", 1, 0xFFFFFF, 1, 1);
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_RAIL_CONFIG_PROPERTY, property))
			indigo_define_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_SHUTTER_PROPERTY, property))
			indigo_define_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_MOVE_AHEAD_PROPERTY, property))
			indigo_define_property(device, X_RAIL_MOVE_AHEAD_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_MOVE_BACK_PROPERTY, property))
			indigo_define_property(device, X_RAIL_MOVE_BACK_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_MOVE_STEPS_PROPERTY, property))
			indigo_define_property(device, X_RAIL_MOVE_STEPS_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			char *name = DEVICE_PORT_ITEM->text.value;
			if (PRIVATE_DATA->device_count++ == 0) {
				PRIVATE_DATA->handle = indigo_open_serial(name);
				wemacro_command(device, 0x20, 0, 0, 0, 0);
			}
			if (PRIVATE_DATA->handle) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
				pthread_create(&PRIVATE_DATA->reader, NULL, (void * (*)(void*))wemacro_reader, device);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				wemacro_command(device, FOCUSER_ROTATION_CLOCKWISE_ITEM->sw.value ? 0x40 : 0x41, 0, 0, 0, FOCUSER_STEPS_ITEM->number.value);
			} else {
				wemacro_command(device, FOCUSER_ROTATION_CLOCKWISE_ITEM->sw.value ? 0x41 : 0x40, 0, 0, 0, FOCUSER_STEPS_ITEM->number.value);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			wemacro_command(device, 0x20, 0, 0, 0, 0);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		indigo_property_copy_values(X_RAIL_CONFIG_PROPERTY, property, false);
		wemacro_command(device, 0x80 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0), X_RAIL_CONFIG_MM_ITEM->sw.value ? 0xFF : 0, 0, 0, 0xFFFFFF);
		X_RAIL_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		indigo_property_copy_values(X_RAIL_SHUTTER_PROPERTY, property, false);
		wemacro_command(device, 0x04, 0, 0, 0, 0);
		X_RAIL_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_MOVE_AHEAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_AHEAD
		indigo_property_copy_values(X_RAIL_MOVE_AHEAD_PROPERTY, property, false);
		wemacro_command(device, FOCUSER_ROTATION_CLOCKWISE_ITEM->sw.value ? 0x40 : 0x41, (uint8_t)X_RAIL_MOVE_AHEAD_SETTLE_TIME_ITEM->number.value, (uint8_t)X_RAIL_MOVE_AHEAD_PER_STEP_ITEM->number.value, (uint8_t)X_RAIL_MOVE_AHEAD_INTERVAL_ITEM->number.value, (uint32_t)X_RAIL_MOVE_AHEAD_LENGTH_ITEM->number.value);
		X_RAIL_MOVE_AHEAD_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RAIL_MOVE_AHEAD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_MOVE_BACK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_BACK
		indigo_property_copy_values(X_RAIL_MOVE_BACK_PROPERTY, property, false);
		wemacro_command(device, FOCUSER_ROTATION_CLOCKWISE_ITEM->sw.value ? 0x41 : 0x40, (uint8_t)X_RAIL_MOVE_BACK_SETTLE_TIME_ITEM->number.value, (uint8_t)X_RAIL_MOVE_BACK_PER_STEP_ITEM->number.value, (uint8_t)X_RAIL_MOVE_BACK_INTERVAL_ITEM->number.value, (uint32_t)X_RAIL_MOVE_BACK_LENGTH_ITEM->number.value);
		X_RAIL_MOVE_BACK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RAIL_MOVE_BACK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_MOVE_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_MOVE_STEPS
		indigo_property_copy_values(X_RAIL_MOVE_STEPS_PROPERTY, property, false);
		wemacro_command(device, 0x10 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0), (uint8_t)X_RAIL_MOVE_STEPS_SETTLE_TIME_ITEM->number.value, (uint8_t)X_RAIL_MOVE_STEPS_PER_STEP_ITEM->number.value, (uint8_t)X_RAIL_MOVE_STEPS_INTERVAL_ITEM->number.value, (uint32_t)X_RAIL_MOVE_STEPS_COUNT_ITEM->number.value);
		X_RAIL_MOVE_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RAIL_MOVE_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(X_RAIL_CONFIG_PROPERTY);
	indigo_release_property(X_RAIL_SHUTTER_PROPERTY);
	indigo_release_property(X_RAIL_MOVE_AHEAD_PROPERTY);
	indigo_release_property(X_RAIL_MOVE_BACK_PROPERTY);
	indigo_release_property(X_RAIL_MOVE_STEPS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

static wemacro_private_data *private_data = NULL;
static indigo_device *focuser = NULL;

indigo_result indigo_focuser_wemacro(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_WEMACRO_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
		);
	

	SET_DRIVER_INFO(info, "WeMacro Rail", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(wemacro_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(wemacro_private_data));
			focuser = malloc(sizeof(indigo_device));
			assert(focuser != NULL);
			memcpy(focuser, &focuser_template, sizeof(indigo_device));
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
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
