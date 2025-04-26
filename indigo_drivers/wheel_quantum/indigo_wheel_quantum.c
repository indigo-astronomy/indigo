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

/** INDIGO Brightstar Quantum filter wheel driver
 \file indigo_ccd_quantum.c
 */

#define DRIVER_VERSION 0x03000004
#define DRIVER_NAME "indigo_wheel_quantum"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_uni_io.h>

#include "indigo_wheel_quantum.h"

// -------------------------------------------------------------------------------- interface implementation

#define PRIVATE_DATA        ((quantum_private_data *)device->private_data)

typedef struct {
	indigo_uni_handle *handle;
	int slot;
} quantum_private_data;

static bool quantum_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void quantum_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
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
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_goto_callback(indigo_device *device) {
	indigo_uni_printf(PRIVATE_DATA->handle, "G%d\r\n", (int)WHEEL_SLOT_ITEM->number.target - 1);
	for (int repeat = 0; repeat < 30; repeat++) {
		int slot;
		if (indigo_uni_scanf_line(PRIVATE_DATA->handle, "P%d", &slot) == 1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "slot = %d", slot);
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot = slot + 1;
			if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				return;
			} else {
				WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			}
		}
		indigo_usleep(500000);
	}
	WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (quantum_open(device)) {
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
			WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 1;
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			wheel_goto_callback(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		quantum_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
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
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		if (WHEEL_SLOT_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
			if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->slot) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
				WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				indigo_set_timer(device, 0, wheel_goto_callback, NULL);
			}
			return INDIGO_OK;
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

indigo_result indigo_wheel_quantum(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"Quantum Filter Wheel",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	SET_DRIVER_INFO(info, "Brightstar Quantum Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			if (wheel == NULL) {
				wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
				quantum_private_data *private_data = indigo_safe_malloc(sizeof(quantum_private_data));
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
