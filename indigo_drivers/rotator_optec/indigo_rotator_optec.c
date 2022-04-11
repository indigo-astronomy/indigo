// Copyright (c) 2022 CloudMakers, s. r. o.
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

/** INDIGO Optec Pyxis camera field rotator driver
 \file indigo_rotator_optec.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_rotator_optec"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_io.h>

#include "indigo_rotator_optec.h"

#define PRIVATE_DATA								((pyxis_private_data *)device->private_data)

typedef struct {
	int handle;
	bool sleeping;
	pthread_mutex_t mutex;
	indigo_timer *rotator_timer;
} pyxis_private_data;

static bool optec_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
	if (PRIVATE_DATA->handle >= 0) {
		char reply;
		int value;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (indigo_printf(PRIVATE_DATA->handle, "CWAKUP\r\n")) {
			if (indigo_select(PRIVATE_DATA->handle, 100000) > 0) {
				if (indigo_scanf(PRIVATE_DATA->handle, "%c\r\n", &reply) != 1 || reply != '!') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to wake up");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
					return false;
				}
			}
		}
		if (indigo_printf(PRIVATE_DATA->handle, "CCLINK\r\n") && indigo_scanf(PRIVATE_DATA->handle, "%c\r\n", &reply) == 1 && reply == '!') {
			indigo_printf(PRIVATE_DATA->handle, "CMREAD\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "%d\r\n", &value) == 1) {
				indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, value == 0 ? ROTATOR_DIRECTION_NORMAL_ITEM : ROTATOR_DIRECTION_REVERSED_ITEM, 1);
				indigo_printf(PRIVATE_DATA->handle, "CGETPA\r\n");
				if (indigo_scanf(PRIVATE_DATA->handle, "%d\r\n", &value) == 1) {
					ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = value;
					PRIVATE_DATA->sleeping = false;
					return true;
				}
			}
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize");
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void optec_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}


// -------------------------------------------------------------------------------- INDIGO rotator device implementation

static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = true;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = true;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 360;
		DEVICE_PORTS_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void rotator_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optec_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_timer);
		optec_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_direction_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (indigo_printf(PRIVATE_DATA->handle, "CD%dxxx\r\n", ROTATOR_DIRECTION_NORMAL_ITEM->sw.value ? 0 : 1))
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	else
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_position_callback(indigo_device *device) {
	char response;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	indigo_printf(PRIVATE_DATA->handle, "CPA%3d\r\n", (int)ROTATOR_POSITION_ITEM->number.target);
	while (true) {
		if (indigo_select(PRIVATE_DATA->handle, 1000000) > 0) {
			if (indigo_read(PRIVATE_DATA->handle, &response, 1) == 1) {
				if (response == '!')
					continue;
				if (response == 'F') {
					ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				}
			}
		}
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		break;
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, rotator_connect_callback, NULL);
		return INDIGO_OK;		
	} else if (indigo_property_match(ROTATOR_DIRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_DIRECTION
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_direction_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		int current = ROTATOR_POSITION_ITEM->number.value;
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		if (ROTATOR_POSITION_ITEM->number.target != current) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_position_callback, &PRIVATE_DATA->rotator_timer);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// --------------------------------------------------------------------------------

static pyxis_private_data *private_data = NULL;
static indigo_device *imager_focuser = NULL;

indigo_result indigo_rotator_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_rotator_template = INDIGO_DEVICE_INITIALIZER(
		"Optec Pyxis",
		rotator_attach,
		indigo_rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Optec Pyxis Rotator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(pyxis_private_data));
			imager_focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &imager_rotator_template);
			imager_focuser->private_data = private_data;
			indigo_attach_device(imager_focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(imager_focuser);
			last_action = action;
			if (imager_focuser != NULL) {
				indigo_detach_device(imager_focuser);
				free(imager_focuser);
				imager_focuser = NULL;
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
