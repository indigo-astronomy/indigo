// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASTROMECHANICS focuser driver
 \file indigo_focuser_astromechanics.c
 */


#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_focuser_astromechanics"

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

#include "indigo_focuser_astromechanics.h"

#define PRIVATE_DATA													((astromechanics_private_data *)device->private_data)

#define X_FOCUSER_APERTURE_PROPERTY						(PRIVATE_DATA->aperture_property)
#define X_FOCUSER_APERTURE_ITEM								(X_FOCUSER_APERTURE_PROPERTY->items+0)

typedef struct {
	int handle;
	indigo_timer *timer;
	indigo_property *aperture_property;
	pthread_mutex_t mutex;
} astromechanics_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool astromechanics_command(indigo_device *device, char *command, char *response) {
	char c;
	struct timeval tv;
	if (command != NULL) {
		if (!indigo_write(PRIVATE_DATA->handle, command, strlen(command)))
			return false;
	}
	if (response != NULL) {
		int index = 0;
		while (index < 10) {
			fd_set readout;
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}
			if (c <= ' ')
				continue;
			if (c < 0 || c == '#')
				break;
			response[index++] = c;
		}
		response[index] = 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command '%s' -> '%s'", command, response != NULL ? response : "NULL");
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		X_FOCUSER_APERTURE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_FOCUSER_APERTURE", "Advanced", "Aperture", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_FOCUSER_APERTURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_FOCUSER_APERTURE_ITEM, "APERURE", "Aperture", 0, 50, 1, 0);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_LINUX
		if (DEVICE_PORTS_PROPERTY->count > 1) {
			/* 0 is refresh button */
			indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
		} else {
			strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
		}
#endif
		// -------------------------------------------------------------------------------- INFO
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "ASTROMECHANICS Focuser");
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 9999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 9999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_APERTURE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 38400);
		if (PRIVATE_DATA->handle > 0) {
			if (astromechanics_command(device, "P#", response)) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(response);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "ASTROMECHANICS focuser detected");
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "ASTROMECHANICS focuser not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_delete_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	int position;
	if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
		position = FOCUSER_POSITION_ITEM->number.value - FOCUSER_STEPS_ITEM->number.value;
		if (position < 0)
			position = 0;
		sprintf(command, "M%04d#", position);
	} else {
		position = FOCUSER_POSITION_ITEM->number.value + FOCUSER_STEPS_ITEM->number.value;
		if (position > 9999)
			position = 9999;
		sprintf(command, "M%04d#", position);
	}
	if (astromechanics_command(device, command, NULL)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		for (int i = 0; i < 50 && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE; i++) {
			if (astromechanics_command(device, "P#", response)) {
				position = atoi(response);
				if (FOCUSER_POSITION_ITEM->number.value == position) {
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				} else {
					FOCUSER_POSITION_ITEM->number.value = position;
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				}
			} else {
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			indigo_usleep(100000);
		}
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	int position;
	position = FOCUSER_POSITION_ITEM->number.target;
	if (position < 0)
		position = 0;
	else if (position > 9999)
		position = 9999;
	FOCUSER_POSITION_ITEM->number.target = position;
	sprintf(command, "M%04d#", position);
	if (astromechanics_command(device, command, NULL)) {
		for (int i = 0; i < 50 && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE; i++) {
			if (astromechanics_command(device, "P#", response)) {
				position = atoi(response);
				if (FOCUSER_POSITION_ITEM->number.value == position) {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				} else {
					FOCUSER_POSITION_ITEM->number.value = position;
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				}
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			indigo_usleep(100000);
		}
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_aperture_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	sprintf(command, "A%02d#", (int)X_FOCUSER_APERTURE_ITEM->number.value);
	if (astromechanics_command(device, command, NULL)) {
		X_FOCUSER_APERTURE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_FOCUSER_APERTURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
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
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		long position = FOCUSER_POSITION_ITEM->number.value;
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_ITEM->number.value = position;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value &&  FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_FOCUSER_APERTURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_APERTURE
		indigo_property_copy_values(X_FOCUSER_APERTURE_PROPERTY, property, false);
		X_FOCUSER_APERTURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_aperture_handler, NULL);
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
	indigo_release_property(X_FOCUSER_APERTURE_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_astromechanics(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static astromechanics_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"ASTROMECHANICS Focuser",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "ASTROMECHANICS Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(astromechanics_private_data));
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
