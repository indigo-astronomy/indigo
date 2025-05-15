// Copyright (c) 2023-2025 CloudMakers, s. r. o.
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

/** INDIGO Pegasus Indigo filter wheel driver
 \file indigo_ccd_indigo.c
 */

#define DRIVER_VERSION 0x03000002
#define DRIVER_NAME "indigo_wheel_indigo"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_uni_io.h>

#include "indigo_wheel_indigo.h"

// -------------------------------------------------------------------------------- interface implementation

#define PRIVATE_DATA        ((indigo_private_data *)device->private_data)

typedef struct {
	indigo_uni_handle *handle;
	pthread_mutex_t mutex;
} indigo_private_data;

static bool indigo_command(indigo_device *device, char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
			if (response != NULL) {
				if (indigo_uni_read_line(PRIVATE_DATA->handle, response, max) > 0) {
					return true;
				}
			} else {
				return true;
			}
		}
	}
	return false;
}

static void wheel_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			if (indigo_command(device, "W#", response, sizeof(response)) && !strcmp(response, "FW_OK")) {
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wheel not detected");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			if (indigo_command(device, "WI", response, sizeof(response)) && !strcmp(response, "WI:1")) {
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'WI' response");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			if (indigo_command(device, "WV", response, sizeof(response)) && !strncmp(response, "WV:", 3)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'WV' response");
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "undefined");
		if (PRIVATE_DATA->handle != NULL) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void wheel_goto_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	sprintf(command, "WM:%d", (int)WHEEL_SLOT_ITEM->number.target);
	if (indigo_command(device, command, response, sizeof(response)) && !strcmp(response, command)) {
		while (true) {
			if (indigo_command(device, "WF", response, sizeof(response)) && !strncmp(response, "WF:", 3)) {
				if (!strcmp(response, "WF:-1")) {
					indigo_sleep(1);
					continue;
				}
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				WHEEL_SLOT_ITEM->number.value = atoi(response + 3);
				break;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'WF' response");
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'WM' response");
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		WHEEL_SLOT_ITEM->number.max = 7;
		WHEEL_SLOT_NAME_PROPERTY->count = 7;
		WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Indigo Filter Wheel");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "undefined");
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, wheel_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 0, wheel_goto_handler, NULL);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

static indigo_device *wheel = NULL;

indigo_result indigo_wheel_indigo(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Indigo Filter Wheel",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	SET_DRIVER_INFO(info, "PegasusAstro Indigo Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			if (wheel == NULL) {
				wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
				indigo_private_data *private_data = indigo_safe_malloc(sizeof(indigo_private_data));
				wheel->private_data = private_data;
				indigo_attach_device(wheel);
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(wheel);
			last_action = action;
			if (wheel != NULL) {
				indigo_detach_device(wheel);
				free(wheel->private_data);
				free(wheel);
				wheel = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
