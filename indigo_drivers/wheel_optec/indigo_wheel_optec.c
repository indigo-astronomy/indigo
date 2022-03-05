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

/** INDIGO Optec filter wheel driver
 \file indigo_wheel_optec.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_wheel_optec"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>

#include <indigo/indigo_io.h>

#include "indigo_wheel_optec.h"

// -------------------------------------------------------------------------------- interface implementation

#define PRIVATE_DATA        ((optec_private_data *)device->private_data)

typedef struct {
	int handle;
	int slot;
} optec_private_data;

static bool optec_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
	if (PRIVATE_DATA->handle >= 0) {
		char reply;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (indigo_printf(PRIVATE_DATA->handle, "WSMODE\r\n") && indigo_scanf(PRIVATE_DATA->handle, "%c\r\n", &reply) == 1 && reply == '!') {
			indigo_printf(PRIVATE_DATA->handle, "WFILTR\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "%d\r\n", &PRIVATE_DATA->slot) == 1) {
				WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
				return true;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read current position");
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize");
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void optec_goto(indigo_device *device) {
	int slot = WHEEL_SLOT_ITEM->number.target;
	char reply;
	WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	if (indigo_printf(PRIVATE_DATA->handle, "WGOTO%d\r\n", slot) && indigo_scanf(PRIVATE_DATA->handle, "%c\r\n", &reply) == 1 && reply == '*') {
		WHEEL_SLOT_ITEM->number.value = slot;
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set position");
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void optec_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		indigo_printf(PRIVATE_DATA->handle, "WEXITS\r\n");
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		WHEEL_SLOT_ITEM->number.max = 8;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optec_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		optec_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
			indigo_set_timer(device, 0, optec_goto, NULL);
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
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

static indigo_device *wheel = NULL;

indigo_result indigo_wheel_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"Optec Filter Wheel",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	SET_DRIVER_INFO(info, "Optec Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		if (wheel == NULL) {
			wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			optec_private_data *private_data = indigo_safe_malloc(sizeof(optec_private_data));
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
