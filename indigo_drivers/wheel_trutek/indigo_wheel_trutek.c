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

/** INDIGO Trutek filter wheel driver
 \file indigo_ccd_trutek.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_wheel_trutek"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>

#include "indigo_io.h"
#include "indigo_wheel_trutek.h"

// -------------------------------------------------------------------------------- interface implementation

#define PRIVATE_DATA        ((trutek_private_data *)device->private_data)

typedef struct {
	int handle;
	int slot;
} trutek_private_data;

static bool trutek_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
		unsigned char buffer[4] = { 0xA5, 0x03, 0x20, 0xA5 + 0x03 + 0x20 };
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d ← %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
		if (indigo_write(PRIVATE_DATA->handle, (char *)buffer, 4)) {
			if (indigo_read(PRIVATE_DATA->handle, (char *)buffer, 4)) {
				INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d → %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
				if (buffer[0] == 0xA5 && buffer[1] == 0x83) {
					WHEEL_SLOT_ITEM->number.max =  buffer[2] - 0x30;
					return true;
				}
			}
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
	}
	return false;
}

static void trutek_query(indigo_device *device) {
	unsigned char buffer[4] = { 0xA5, 0x02, 0x20, 0xA5 + 0x02 + 0x20 };
	WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d ← %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
	if (indigo_write(PRIVATE_DATA->handle, (char *)buffer, 4)) {
		if (indigo_read(PRIVATE_DATA->handle, (char *)buffer, 4)) {
			INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d → %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
			if (buffer[0] == 0xA5 && buffer[1] == 0x82) {
				WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->slot = buffer[2] - 0x30;
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	if (IS_CONNECTED)
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void trutek_goto(indigo_device *device) {
	int slot = WHEEL_SLOT_ITEM->number.target;
	unsigned char buffer[4] = { 0xA5, 0x01, slot, 0xA5 + 0x01 + slot };
	WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d ← %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
	if (indigo_write(PRIVATE_DATA->handle, (char *)buffer, 4)) {
		if (indigo_read(PRIVATE_DATA->handle, (char *)buffer, 4)) {
			INDIGO_DRIVER_TRACE(DRIVER_NAME, "%d → %02x %02x %02x %02x", PRIVATE_DATA->handle, buffer[0], buffer[1], buffer[2], buffer[3])
			if (buffer[0] == 0xA5 && buffer[1] == 0x81) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static void trutek_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (trutek_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0, trutek_query);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			trutek_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
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
			indigo_set_timer(device, 0, trutek_goto);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

static indigo_device *device = NULL;

indigo_result indigo_wheel_trutek(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"Trutek Filter Wheel",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	SET_DRIVER_INFO(info, "Trutek Filter Wheel", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		if (device == NULL) {
			device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &wheel_template, sizeof(indigo_device));
			trutek_private_data *private_data = malloc(sizeof(trutek_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(trutek_private_data));
			device->private_data = private_data;
			indigo_attach_device(device);
		}
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		if (device != NULL) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
			device = NULL;
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
