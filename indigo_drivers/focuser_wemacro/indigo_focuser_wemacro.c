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
 \file indigo_focuser_wemacro.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_focuser_wemacro"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_wemacro.h"

#define PRIVATE_DATA													((wemacro_private_data *)device->private_data)

#define X_RAIL_BATCH													"Batch"

#define X_RAIL_CONFIG_PROPERTY								(PRIVATE_DATA->config_property)
#define X_RAIL_CONFIG_BACK_ITEM								(X_RAIL_CONFIG_PROPERTY->items+0)
#define X_RAIL_CONFIG_BEEP_ITEM								(X_RAIL_CONFIG_PROPERTY->items+1)

#define X_RAIL_SHUTTER_PROPERTY								(PRIVATE_DATA->shutter_property)
#define X_RAIL_SHUTTER_ITEM										(X_RAIL_SHUTTER_PROPERTY->items+0)

#define X_RAIL_EXECUTE_PROPERTY								(PRIVATE_DATA->move_steps_property)
#define X_RAIL_EXECUTE_SETTLE_TIME_ITEM				(X_RAIL_EXECUTE_PROPERTY->items+0)
#define X_RAIL_EXECUTE_PER_STEP_ITEM					(X_RAIL_EXECUTE_PROPERTY->items+1)
#define X_RAIL_EXECUTE_INTERVAL_ITEM					(X_RAIL_EXECUTE_PROPERTY->items+2)
#define X_RAIL_EXECUTE_LENGTH_ITEM						(X_RAIL_EXECUTE_PROPERTY->items+3)
#define X_RAIL_EXECUTE_COUNT_ITEM							(X_RAIL_EXECUTE_PROPERTY->items+4)


typedef struct {
	int handle;
	int device_count;
	pthread_mutex_t port_mutex;
	indigo_property *shutter_property;
	indigo_property *config_property;
	indigo_property *move_steps_property;
} wemacro_private_data;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static uint8_t wemacro_read(indigo_device *device) {
	uint8_t in[3] = { 0, 0, 0 };
	struct timeval tv;
	fd_set readout;
	FD_ZERO(&readout);
	FD_SET(PRIVATE_DATA->handle, &readout);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "select %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
		return 0;
	}
	if (result == 0) {
		return 0;
	}
	result = indigo_read(PRIVATE_DATA->handle, (char *)in, sizeof(in));
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "read %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
		return 0;
	}
	if (result == sizeof(in)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%02x %02x %02x", in[0], in[1], in[2]);
		return in[2];
	}
	return 0;
}

static bool wemacro_write(indigo_device *device, uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint32_t d) {
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
	}
	out[10] = crc & 0xFF;
	out[11] = (crc >> 8) &0xFF;
	result = indigo_write(PRIVATE_DATA->handle, (char *)out, sizeof(out));
	if (result) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11]);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "write %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return result;
}

static char *wemacro_reader(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "started");
	uint8_t state = wemacro_read(device);
	if (state == 0xf0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "initialised");
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "failed, trying reset");
		wemacro_write(device, 0x40, 0, 0, 0, 0);
		wemacro_write(device, 0x20, 0, 0, 0, 0);
		wemacro_write(device, 0x40, 0, 0, 0, 0);
		state = wemacro_read(device);
		if (state == 0xf5) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "initialised");
		} else {
			indigo_device_disconnect(NULL, device->name);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "WeMacro initialisation failed, no reply from the controller");
			return NULL;
		}
	}
	wemacro_write(device, 0x80 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0), FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0, 0, 0, 0);
	while (PRIVATE_DATA->handle > 0) {
		state = wemacro_read(device);
		if (state) {
			if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
				if (state == 0xf5 || state == 0xf6) {
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				}
			} else if (X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE) {
				if (state == 0xf7) {
					X_RAIL_EXECUTE_COUNT_ITEM->number.value--;
				}
				if (X_RAIL_CONFIG_BACK_ITEM->sw.value) {
					if (state == 0xf6)
						X_RAIL_EXECUTE_PROPERTY->state = INDIGO_OK_STATE;
				} else if (X_RAIL_EXECUTE_COUNT_ITEM->number.value == 0) {
					X_RAIL_EXECUTE_PROPERTY->state = INDIGO_OK_STATE;
				}
				indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "finished");
	return NULL;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		FOCUSER_SPEED_ITEM->number.max = 2;
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		X_RAIL_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_CONFIG", X_RAIL_BATCH, "Set configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (X_RAIL_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RAIL_CONFIG_BACK_ITEM, "BACK", "Return back when done", false);
		indigo_init_switch_item(X_RAIL_CONFIG_BEEP_ITEM, "BEEP", "Beep when done", false);
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		X_RAIL_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_SHUTTER", X_RAIL_BATCH, "Fire shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_RAIL_SHUTTER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RAIL_SHUTTER_ITEM, "SHUTTER", "Fire shutter", false);
		// -------------------------------------------------------------------------------- X_RAIL_EXECUTE
		X_RAIL_EXECUTE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RAIL_EXECUTE", X_RAIL_BATCH, "Execute batch", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (X_RAIL_EXECUTE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RAIL_EXECUTE_SETTLE_TIME_ITEM, "SETTLE_TIME", "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_PER_STEP_ITEM, "SHUTTER_PER_STEP", "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_INTERVAL_ITEM, "SHUTTER_INTERVAL", "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_LENGTH_ITEM, "LENGTH", "Step size", 1, 0xFFFFFF, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_COUNT_ITEM, "COUNT", "Step count", 0, 0xFFFFFF, 1, 1);
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_RAIL_CONFIG_PROPERTY);
		indigo_define_matching_property(X_RAIL_SHUTTER_PROPERTY);
		indigo_define_matching_property(X_RAIL_EXECUTE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		char *name = DEVICE_PORT_ITEM->text.value;
		if (PRIVATE_DATA->device_count++ == 0) {
			PRIVATE_DATA->handle = indigo_open_serial(name);
		}
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
			indigo_async((void * (*)(void*))wemacro_reader, device);
			indigo_define_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s -> %s (%d)", name, strerror(errno), errno);
			PRIVATE_DATA->device_count--;
			indigo_delete_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
			indigo_delete_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
			indigo_delete_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				wemacro_write(device, FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0x41 : 0x40, 0, 0, 0, FOCUSER_STEPS_ITEM->number.value);
			} else {
				wemacro_write(device, FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0x40 : 0x41, 0, 0, 0, FOCUSER_STEPS_ITEM->number.value);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			wemacro_write(device, 0x20, 0, 0, 0, 0);
			X_RAIL_EXECUTE_COUNT_ITEM->number.value = 0;
			X_RAIL_EXECUTE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		wemacro_write(device, 0x80 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0), FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0, 0, 0, 0);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		indigo_property_copy_values(X_RAIL_CONFIG_PROPERTY, property, false);
		X_RAIL_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		indigo_property_copy_values(X_RAIL_SHUTTER_PROPERTY, property, false);
		if (X_RAIL_SHUTTER_ITEM->sw.value) {
			wemacro_write(device, 0x04, 0, 0, 0, 0);
			X_RAIL_SHUTTER_ITEM->sw.value = false;
		}
		X_RAIL_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RAIL_EXECUTE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_EXECUTE
		indigo_property_copy_values(X_RAIL_EXECUTE_PROPERTY, property, false);
		wemacro_write(device, 0x80, FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0, 0, 0, (uint32_t)X_RAIL_EXECUTE_LENGTH_ITEM->number.value);
		indigo_usleep(100000);
		wemacro_write(device, 0x10 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0), (uint8_t)X_RAIL_EXECUTE_SETTLE_TIME_ITEM->number.value, (uint8_t)X_RAIL_EXECUTE_PER_STEP_ITEM->number.value, (uint8_t)X_RAIL_EXECUTE_INTERVAL_ITEM->number.value, (uint32_t)X_RAIL_EXECUTE_COUNT_ITEM->number.value - 1);
		X_RAIL_EXECUTE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_RAIL_CONFIG_PROPERTY);
		}
	// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(X_RAIL_CONFIG_PROPERTY);
	indigo_release_property(X_RAIL_SHUTTER_PROPERTY);
	indigo_release_property(X_RAIL_EXECUTE_PROPERTY);
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

	static indigo_device_match_pattern patterns[1] = { 0 };
	patterns[0].vendor_id = 0x1A86;
	patterns[0].product_id = 0x7523;
	strcpy(patterns[0].product_string, "USB2.0-Serial");
	INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);

	SET_DRIVER_INFO(info, FOCUSER_WEMACRO_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(wemacro_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser);
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
