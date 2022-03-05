// Copyright (c) 2020 CloudMakers, s. r. o.
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

/** INDIGO QHY CFW filter wheel driver
 \file indigo_ccd_qhy.c
 */

#define DRIVER_VERSION 0x0007
#define DRIVER_NAME "indigo_wheel_qhy"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#include <indigo/indigo_io.h>

#include "indigo_wheel_qhy.h"

// -------------------------------------------------------------------------------- interface implementation

#define X_MODEL_PROPERTY                (PRIVATE_DATA->model)
#define X_MODEL_1_ITEM                  (X_MODEL_PROPERTY->items+0)
#define X_MODEL_2_ITEM                  (X_MODEL_PROPERTY->items+1)
#define X_MODEL_3_ITEM                  (X_MODEL_PROPERTY->items+2)

#define X_MODEL_PROPERTY_NAME           "X_MODEL"
#define X_MODEL_1_ITEM_NAME             "1"
#define X_MODEL_2_ITEM_NAME             "2"
#define X_MODEL_3_ITEM_NAME             "3"

#define PRIVATE_DATA        ((qhy_private_data *)device->private_data)

typedef struct {
	int handle;
	int current_slot;
	indigo_property *model;
	pthread_mutex_t mutex;
} qhy_private_data;

static bool qhy_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static bool qhy_command(indigo_device *device, char *command, char *reply, int reply_length, int read_timeout) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	size_t result = indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (result > 0 && reply) {
		for (int i = 0; i < read_timeout; i++) {
			result = indigo_read(PRIVATE_DATA->handle, reply, reply_length);
			if (result > 0) {
				break;
			} else {
				indigo_usleep(ONE_SECOND_DELAY);
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "command: %s -> %.*s (%s)", command, reply_length, reply, result > 0 ? "OK" : strerror(errno));
	return result > 0;
}

static void qhy_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (qhy_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			if (X_MODEL_3_ITEM->sw.value) {
				char reply[8];
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW3");
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				bool result = qhy_command(device, "VRS", INFO_DEVICE_FW_REVISION_ITEM->text.value, 8, 1);
				if (!result) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, retrying...");
					result = qhy_command(device, "VRS", INFO_DEVICE_FW_REVISION_ITEM->text.value, 8, 1);
				}
				if (result) {
					if (qhy_command(device, "MXP", reply, 1, 1)) {
						WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = isdigit(reply[0]) ? reply[0] - '0' : reply[0] - 'A' + 10;
					}
					if (qhy_command(device, "NOW", reply, 1, 1)) {
						WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = 	isdigit(reply[0]) ? reply[0] - '0' + 1 : reply[0] - 'A' + 11;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed, fallback to CFW1");
					indigo_set_switch(X_MODEL_PROPERTY, X_MODEL_1_ITEM, true);
					indigo_update_property(device, X_MODEL_PROPERTY, "Failed to connect to CFW3, trying to fallback to CFW1");
				}
			}
			if (X_MODEL_2_ITEM->sw.value) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW2");
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				if (!qhy_command(device, "0", NULL, 0, 0)) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
			}
			if (X_MODEL_1_ITEM->sw.value) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "QHY CFW1");
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
				if (!qhy_command(device, "0", NULL, 0, 0)) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 7;
				WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		qhy_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_goto_handler(indigo_device *device) {
	char command[2] = { '0' + WHEEL_SLOT_ITEM->number.target - 1, 0 };
	char reply[2];
	if (qhy_command(device, command, reply, 1, 15)) {
		if (X_MODEL_1_ITEM->sw.value) {
			WHEEL_SLOT_PROPERTY->state = reply[0] == '-';
		} else if (X_MODEL_2_ITEM->sw.value || X_MODEL_3_ITEM->sw.value) {
			WHEEL_SLOT_PROPERTY->state = reply[0] == command[0];
		}
		if (X_MODEL_1_ITEM->sw.value) {
			int slots_to_go = (int)(WHEEL_SLOT_ITEM->number.target - PRIVATE_DATA->current_slot);
			if (slots_to_go < 0) slots_to_go = slots_to_go + WHEEL_SLOT_ITEM->number.max;
			int timeout = (int)(ONE_SECOND_DELAY * (3 + 2 * (slots_to_go - 1)));
			indigo_usleep(timeout);
			/*
			INDIGO_DRIVER_ERROR(
				DRIVER_NAME,
				"current = %d, target = %f, slots_to_go = %d (%d us), %f",
				PRIVATE_DATA->current_slot,
				WHEEL_SLOT_ITEM->number.target,
				slots_to_go,
				timeout,
				WHEEL_SLOT_ITEM->number.max
			);
			*/
		}
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->current_slot = WHEEL_SLOT_ITEM->number.target;
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_MODEL
		X_MODEL_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MODEL_PROPERTY_NAME, MAIN_GROUP, "Device type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_MODEL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_MODEL_1_ITEM, X_MODEL_1_ITEM_NAME, "CFW 1", false);
		indigo_init_switch_item(X_MODEL_2_ITEM, X_MODEL_2_ITEM_NAME, "CFW 2", false);
		indigo_init_switch_item(X_MODEL_3_ITEM, X_MODEL_3_ITEM_NAME, "CFW 3", true);
		// --------------------------------------------------------------------------------
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		INFO_PROPERTY->count = 6;
		WHEEL_SLOT_ITEM->number.value = 1;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(X_MODEL_PROPERTY, property))
		indigo_define_property(device, X_MODEL_PROPERTY, NULL);
	return indigo_wheel_enumerate_properties(device, NULL, NULL);
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
	} else if (indigo_property_match(X_MODEL_PROPERTY, property)) {
		indigo_property_copy_values(X_MODEL_PROPERTY, property, false);
		indigo_update_property(device, X_MODEL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		int slot = WHEEL_SLOT_ITEM->number.value;
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.target == slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_set_timer(device, 0, wheel_goto_handler, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_MODEL_PROPERTY);
		}
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
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	indigo_release_property(X_MODEL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

static indigo_device *wheel = NULL;

indigo_result indigo_wheel_qhy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"CFW Filter Wheel",
		wheel_attach,
		wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
		);

	SET_DRIVER_INFO(info, "QHY CFW Filter Wheel", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		if (wheel == NULL) {
			wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			qhy_private_data *private_data = indigo_safe_malloc(sizeof(qhy_private_data));
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
