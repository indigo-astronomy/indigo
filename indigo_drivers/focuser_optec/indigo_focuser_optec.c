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

/** INDIGO Optec TCF-S focuser driver
 \file indigo_focuser_optec.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_focuser_optec"

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

#include "indigo_focuser_optec.h"

#define PRIVATE_DATA													((optec_private_data *)device->private_data)

typedef struct {
	int handle;
	indigo_timer *timer;
	pthread_mutex_t mutex;
} optec_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool optec_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
	if (PRIVATE_DATA->handle >= 0) {
		char reply;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (indigo_printf(PRIVATE_DATA->handle, "FMMODE\r\n") && indigo_scanf(PRIVATE_DATA->handle, "%c\r\n", &reply) == 1 && reply == '!') {
			double value;
			indigo_printf(PRIVATE_DATA->handle, "FPOSRO\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "P=%lf\r\n", &value) == 1) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = value;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read current position");
			}
			indigo_printf(PRIVATE_DATA->handle, "FTMPRO\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "T=%lf\r\n", &value) == 1) {
				FOCUSER_TEMPERATURE_ITEM->number.value = value;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read current temperature");
			}
			indigo_printf(PRIVATE_DATA->handle, "FREADA\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "A=%lf\r\n", &value) == 1) {
				FOCUSER_COMPENSATION_ITEM->number.value = value;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read current compensation");
			}
			indigo_printf(PRIVATE_DATA->handle, "FTxxxA\r\n");
			if (indigo_scanf(PRIVATE_DATA->handle, "A=%lf\r\n", &value) == 1) {
				if (value == 1)
					FOCUSER_COMPENSATION_ITEM->number.value = -FOCUSER_COMPENSATION_ITEM->number.value;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read current compensation");
			}
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize");
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void optec_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		indigo_printf(PRIVATE_DATA->handle, "FFMODE\r\n");
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 9999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -999;
		FOCUSER_COMPENSATION_ITEM->number.max = 999;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	double value;
	if (FOCUSER_MODE_PROPERTY->state == INDIGO_OK_STATE && FOCUSER_MODE_MANUAL_ITEM->sw.value) {
		indigo_printf(PRIVATE_DATA->handle, "FPOSRO\r\n");
		if (indigo_scanf(PRIVATE_DATA->handle, "P=%lf\r\n", &value) == 1) {
			FOCUSER_POSITION_ITEM->number.value = value;
			FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = FOCUSER_POSITION_ITEM->number.value == FOCUSER_POSITION_ITEM->number.target ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		indigo_printf(PRIVATE_DATA->handle, "FTMPRO\r\n");
		if (indigo_scanf(PRIVATE_DATA->handle, "T=%lf\r\n", &value) == 1) {
			FOCUSER_TEMPERATURE_ITEM->number.value = value;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optec_open(device)) {
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			optec_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? 'I' : 'O';
	if (indigo_printf(PRIVATE_DATA->handle, "F%c%04d\r\n", direction, (int)FOCUSER_STEPS_ITEM->number.value) && indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && strcmp(response, "*") == 0) {
		FOCUSER_POSITION_ITEM->number.target += (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value);
		FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_mode_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		if (indigo_printf(PRIVATE_DATA->handle, "FQUIT1\r\n") && indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && strcmp(response, "DONE") == 0) {
			if (indigo_printf(PRIVATE_DATA->handle, "FAMODE\r\n")) {
				FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			}
		}
	} else {
		for (int i = 0; i < 10; i++) {
			if (indigo_printf(PRIVATE_DATA->handle, "FMMODE\r\n") && indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && strcmp(response, "!") == 0) {
				FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				break;
			}
			indigo_usleep(ONE_SECOND_DELAY);
		}
	}
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_compensation_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	if (indigo_printf(PRIVATE_DATA->handle, "FLA%04d\r\n", (int)fabs(FOCUSER_COMPENSATION_ITEM->number.value)) && indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && strcmp(response, "DONE") == 0) {
		if (indigo_printf(PRIVATE_DATA->handle, "FZAxx%c\r\n", FOCUSER_COMPENSATION_ITEM->number.value >= 0 ? '0' : '1') && indigo_read_line(PRIVATE_DATA->handle, response, sizeof(response)) > 0 && strcmp(response, "DONE") == 0) {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
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
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		if (IS_CONNECTED) {
			FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_compensation_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_mode_handler, NULL);
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
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_optec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static optec_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"Optec TCF-S",
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "Optec TCF-S Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(optec_private_data));
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
