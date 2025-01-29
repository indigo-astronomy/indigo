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
#include <sys/termios.h>

#include <indigo/indigo_io.h>

#include "indigo_rotator_optec.h"

#define PRIVATE_DATA								((pyxis_private_data *)device->private_data)

#define X_HOME_PROPERTY     					(PRIVATE_DATA->home_property)
#define X_HOME_ITEM  									(X_HOME_PROPERTY->items + 0)

#define X_RATE_PROPERTY     					(PRIVATE_DATA->rate_property)
#define X_RATE_ITEM  									(X_RATE_PROPERTY->items + 0)

#define X_ROTATE_PROPERTY     				(PRIVATE_DATA->rotate_property)
#define X_ROTATE_ITEM  								(X_ROTATE_PROPERTY->items + 0)

typedef struct {
	int handle;
	indigo_property *home_property;
	indigo_property *rate_property;
	indigo_property *rotate_property;
	pthread_mutex_t mutex;
	indigo_timer *position_timer;
	indigo_timer *home_timer;
} pyxis_private_data;

static void optec_sleep(indigo_device *device) {
	indigo_printf(PRIVATE_DATA->handle, "CSLEEP");
}

static bool optec_wakeup(indigo_device *device) {
	char response;
	if (indigo_printf(PRIVATE_DATA->handle, "CWAKUP")) {
		if (indigo_select(PRIVATE_DATA->handle, 100000) > 0) {
			if (indigo_scanf(PRIVATE_DATA->handle, "%c", &response) != 1 || response != '!') {
				tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to wake up");
				return false;
			}
		}
	}
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	return true;
}

static bool optec_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
	if (PRIVATE_DATA->handle >= 0) {
		char response;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (optec_wakeup(device)) {
			if (indigo_printf(PRIVATE_DATA->handle, "CCLINK") && indigo_scanf(PRIVATE_DATA->handle, "%c", &response) == 1 && response == '!') {
				tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
				return true;
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

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);


static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_HOME
		X_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_HOME", ROTATOR_MAIN_GROUP, "Home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_HOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_HOME_ITEM, "HOME", "Find home", false);
		// -------------------------------------------------------------------------------- X_RATE
		X_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RATE", ROTATOR_MAIN_GROUP, "Rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_RATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_RATE_ITEM, "RATE", "Rotational rate", 0, 99, 1, 8);
		// -------------------------------------------------------------------------------- X_RATE
		X_ROTATE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_ROTATE", ROTATOR_MAIN_GROUP, "Rotate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_ROTATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_ROTATE_ITEM, "ROTATE", "Steps", -9, 9, 1, 0);
		// --------------------------------------------------------------------------------
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = true;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = true;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_POSITION_ITEM->number.min = -359;
		ROTATOR_POSITION_ITEM->number.max = 359;
		DEVICE_PORTS_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_HOME_PROPERTY);
		indigo_define_matching_property(X_RATE_PROPERTY);
		indigo_define_matching_property(X_ROTATE_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
}


static void rotator_connect_callback(indigo_device *device) {
	char response[16] = { 0 };
	int value;
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optec_open(device)) {
			if (indigo_printf(PRIVATE_DATA->handle, "CMREAD") && indigo_scanf(PRIVATE_DATA->handle, "%d", &value) == 1) {
				indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, value == 0 ? ROTATOR_DIRECTION_NORMAL_ITEM : ROTATOR_DIRECTION_REVERSED_ITEM, 1);
				ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (indigo_printf(PRIVATE_DATA->handle, "CGETPA") && indigo_scanf(PRIVATE_DATA->handle, "%d", &value) == 1) {
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = value;
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_define_property(device, X_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_RATE_PROPERTY, NULL);
			if (indigo_printf(PRIVATE_DATA->handle, "CTxx%02d", (int)X_RATE_ITEM->number.target) && read(PRIVATE_DATA->handle, response, 15) == 1 && *response == '!')
				X_RATE_PROPERTY->state = INDIGO_OK_STATE;
			else
				X_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", PRIVATE_DATA->handle, response));
			tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
			optec_sleep(device);
			indigo_define_property(device, X_ROTATE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->home_timer);
		indigo_delete_property(device, X_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_RATE_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATE_PROPERTY, NULL);
		optec_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_direction_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (optec_wakeup(device) && indigo_printf(PRIVATE_DATA->handle, "CD%dxxx", ROTATOR_DIRECTION_NORMAL_ITEM->sw.value ? 0 : 1))
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	else
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	optec_sleep(device);
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_position_callback(indigo_device *device) {
	char response[16] = { 0 };
	int steps = 0;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (optec_wakeup(device)) {
		indigo_printf(PRIVATE_DATA->handle, "CPA%03d", (int)ROTATOR_POSITION_ITEM->number.target);
		while (true) {
			if (indigo_select(PRIVATE_DATA->handle, 1000000) > 0) {
				if (indigo_read(PRIVATE_DATA->handle, response, 1) == 1) {
					if (*response == '!') {
						steps++;
						continue;
					}
					if (*response == 'F') {
						INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %d!F", PRIVATE_DATA->handle, steps));
						ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
						break;
					}
					if (indigo_select(PRIVATE_DATA->handle, 10000) > 0) {
						read(PRIVATE_DATA->handle, response + 1, 10);
						INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", PRIVATE_DATA->handle, response));
					}
				}
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
	}
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	optec_sleep(device);
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_home_callback(indigo_device *device) {
	char response[16] = { 0 };
	int steps = 0;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (optec_wakeup(device)) {
		indigo_printf(PRIVATE_DATA->handle, "CHOMES");
		while (true) {
			if (indigo_select(PRIVATE_DATA->handle, 1000000) > 0) {
				if (indigo_read(PRIVATE_DATA->handle, response, 1) == 1) {
					if (*response == '!') {
						steps++;
						continue;
					}
					if (*response == 'F') {
						INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %d!F", PRIVATE_DATA->handle, steps));
						ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = 0;
						ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
						X_HOME_PROPERTY->state = INDIGO_OK_STATE;
						break;
					}
					if (indigo_select(PRIVATE_DATA->handle, 10000) > 0) {
						read(PRIVATE_DATA->handle, response + 1, 10);
						INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", PRIVATE_DATA->handle, response));
					}
				}
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			X_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
	}
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	optec_sleep(device);
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	indigo_update_property(device, X_HOME_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_rate_callback(indigo_device *device) {
	char response[16] = { 0 };
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (optec_wakeup(device) && indigo_printf(PRIVATE_DATA->handle, "CTxx%02d", (int)X_RATE_ITEM->number.target) && read(PRIVATE_DATA->handle, response, 15) == 1 && *response == '!')
		X_RATE_PROPERTY->state = INDIGO_OK_STATE;
	else
		X_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", PRIVATE_DATA->handle, response));
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	optec_sleep(device);
	indigo_update_property(device, X_RATE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_rotate_callback(indigo_device *device) {
	char response[16] = { 0 };
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int value = X_ROTATE_ITEM->number.target > 0 ? (int)X_ROTATE_ITEM->number.target : 10 - (int)X_ROTATE_ITEM->number.target;
	if (optec_wakeup(device) && indigo_printf(PRIVATE_DATA->handle, "CXxx%02d", value) && read(PRIVATE_DATA->handle, response, 15) == 1 && *response == '!')
		X_ROTATE_PROPERTY->state = INDIGO_OK_STATE;
	else
		X_ROTATE_PROPERTY->state = INDIGO_ALERT_STATE;
	INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", PRIVATE_DATA->handle, response));
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	optec_sleep(device);
	X_ROTATE_ITEM->number.target = 0;
	indigo_update_property(device, X_ROTATE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, rotator_connect_callback, NULL);
		return INDIGO_OK;		
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_DIRECTION
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_direction_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		int current = ROTATOR_POSITION_ITEM->number.value;
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		if (ROTATOR_POSITION_ITEM->number.target != current) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_position_callback, &PRIVATE_DATA->position_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_HOME
		indigo_property_copy_values(X_HOME_PROPERTY, property, false);
		if (X_HOME_ITEM->sw.value) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			X_HOME_ITEM->sw.value = false;
			X_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_HOME_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_home_callback, &PRIVATE_DATA->home_timer);
		}
	} else if (indigo_property_match_changeable(X_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RATE
		indigo_property_copy_values(X_RATE_PROPERTY, property, false);
		X_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_rate_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_ROTATE
		indigo_property_copy_values(X_ROTATE_PROPERTY, property, false);
		if (X_ROTATE_ITEM->number.value) {
			X_ROTATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_ROTATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_rotate_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_RATE_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->home_timer);
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connect_callback(device);
	}
	indigo_release_property(X_HOME_PROPERTY);
	indigo_release_property(X_RATE_PROPERTY);
	indigo_release_property(X_ROTATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// --------------------------------------------------------------------------------

static pyxis_private_data *private_data = NULL;
static indigo_device *rotator = NULL;

indigo_result indigo_rotator_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		"Optec Pyxis",
		rotator_attach,
		rotator_enumerate_properties,
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
			rotator = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			indigo_attach_device(rotator);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				free(rotator);
				rotator = NULL;
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
