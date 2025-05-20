// Copyright (c) 2023 CloudMakers, s. r. o.
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

/** INDIGO PegasusAstro Falcon field rotator driver
 \file indigo_rotator_falcon.c
 */

#define DRIVER_VERSION 0x0004
#define DRIVER_NAME	"indigo_rotator_falcon"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/termios.h>

#include <indigo/indigo_io.h>

#include "indigo_rotator_falcon.h"

#define PRIVATE_DATA								((falcon_private_data *)device->private_data)

typedef struct {
	int handle;
	pthread_mutex_t mutex;
	indigo_timer *position_handler_timer, *direction_handler_timer, *abort_motion_handler_timer, *relative_move_handler_timer;
	int version;
} falcon_private_data;

static bool falcon_command(indigo_device *device, char *command, char *response, int max) {
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	indigo_write(PRIVATE_DATA->handle, "\n", 1);
	if (response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) <= 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> no response", command);
			return false;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static void rotator_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600);
		if (PRIVATE_DATA->handle > 0) {
			if (falcon_command(device, "F#", response, sizeof(response)) && !strcmp(response, "FR_OK")) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value ,"Falcon Rotator");
			} else {
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200);
				if (PRIVATE_DATA->handle > 0) {
					if (falcon_command(device, "F#", response, sizeof(response)) && !strncmp(response, "F2R_", 4)) {
						strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Falcon Rotator v2");
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (falcon_command(device, "FV", response, sizeof(response)) && !strncmp(response, "FV:", 3)) {
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 3);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'FV' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (!(falcon_command(device, "FH", response, sizeof(response)) && !strcmp(response, "FH:1"))) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'FH' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (!(falcon_command(device, "DR:0", response, sizeof(response)) && !strncmp(response, "DR:", 3))) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'DR' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (falcon_command(device, "FA", response, sizeof(response))) {
				if (!strncmp(response, "FR_OK", 5)) {
					char *pnt, *token = strtok_r(response, ":", &pnt);
					token = strtok_r(NULL, ":", &pnt); // position_in_deg
					if (token) {
						ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value = indigo_atod(token);
					}
					token = strtok_r(NULL, ":", &pnt); // is_running
					token = strtok_r(NULL, ":", &pnt); // limit_detect
					token = strtok_r(NULL, ":", &pnt); // do_derotation
					token = strtok_r(NULL, ":", &pnt); // motor_reverse
					if (token) {
						if (*token == '0')
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_NORMAL_ITEM, true);
						else
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_REVERSED_ITEM, true);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'FA' response");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				} else if (!strncmp(response, "F2R:", 4)) {
					char *pnt, *token = strtok_r(response, ":", &pnt); // position_in_deg
					token = strtok_r(NULL, ":", &pnt); // position_in_deg
					if (token) {
						ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value = indigo_atod(token);
					}
					token = strtok_r(NULL, ":", &pnt); // is_running
					token = strtok_r(NULL, ":", &pnt); // speed
					token = strtok_r(NULL, ":", &pnt); // microsteps
					token = strtok_r(NULL, ":", &pnt); // direction
					if (token) {
						if (*token == '0')
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_NORMAL_ITEM, true);
						else
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_REVERSED_ITEM, true);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'FA' response");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'FA' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->direction_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->abort_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->relative_move_handler_timer);
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void falcon_move(indigo_device *device) {
	char command[16], response[64];
	sprintf(command, "MD:%0.2f", ROTATOR_POSITION_ITEM->number.target);
	if (falcon_command(device, command, response, sizeof(response)) && !strncmp(response, "MD:", 3)) {
		while (true) {
			if (falcon_command(device, "FD", response, sizeof(response)) && !strncmp(response, "FD:", 3)) {
				ROTATOR_POSITION_ITEM->number.value = indigo_atod(response + 3);
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			} else {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			if (falcon_command(device, "FR", response, sizeof(response)) && !strncmp(response, "FR:", 3)) {
				if (!strcmp(response, "FR:1")) {
					pthread_mutex_unlock(&PRIVATE_DATA->mutex);
					indigo_usleep(0.1 * ONE_SECOND_DELAY);
					pthread_mutex_lock(&PRIVATE_DATA->mutex);
					continue;
				}
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			break;
		}
		if (falcon_command(device, "FD", response, sizeof(response)) && !strncmp(response, "FD:", 3)) {
			ROTATOR_POSITION_ITEM->number.value = indigo_atod(response + 3);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

static void falcon_sync(indigo_device *device) {
	char command[16], response[64];
	sprintf(command, "SD:%0.2f", ROTATOR_POSITION_ITEM->number.target);
	if (falcon_command(device, command, response, sizeof(response)) && !strncmp(response, "SD:", 3)) {
		if (falcon_command(device, "FD", response, sizeof(response)) && !strncmp(response, "FD:", 3)) {
			ROTATOR_POSITION_ITEM->number.value = indigo_atod(response + 3);
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

static void rotator_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		falcon_move(device);
	} else {
		falcon_sync(device);
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_relative_move_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	double position = ROTATOR_POSITION_ITEM->number.value + ROTATOR_RELATIVE_MOVE_ITEM->number.value;
	if (position < 0) {
		position += 360.0;
	} else if (position >= 360.0) {
		position -= 360.0;
	}
	ROTATOR_POSITION_ITEM->number.target = position;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	falcon_move(device);
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	ROTATOR_RELATIVE_MOVE_PROPERTY->state = ROTATOR_POSITION_PROPERTY->state;
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_abort_handler(indigo_device *device) {
	char response[64];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (!falcon_command(device, "FH", response, sizeof(response)) && !strncmp(response, "FH:", 3)) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_direction_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (falcon_command(device, ROTATOR_DIRECTION_NORMAL_ITEM->sw.value ? "FN:0" : "FN:1", response, sizeof(response)) && !strncmp(response, "FN:", 3)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'FN' response");
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// -------------------------------------------------------------------------------- INDIGO rotator device implementation

static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 359.99;
		DEVICE_PORTS_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->hidden = false;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
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
		indigo_set_timer(device, 0, rotator_connection_handler, NULL);
		return INDIGO_OK;		
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_DIRECTION
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_direction_handler, &PRIVATE_DATA->direction_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_position_handler, &PRIVATE_DATA->position_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_RELATIVE_MOVE
		if (ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_RELATIVE_MOVE_PROPERTY, property, false);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_relative_move_handler, &PRIVATE_DATA->relative_move_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_abort_handler, &PRIVATE_DATA->abort_motion_handler_timer);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// --------------------------------------------------------------------------------

static falcon_private_data *private_data = NULL;
static indigo_device *rotator = NULL;

indigo_result indigo_rotator_falcon(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Falcon rotator",
		rotator_attach,
		indigo_rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	static indigo_device_match_pattern patterns[1] = { 0 };
	strcpy(patterns[0].vendor_string, "Pegasus Astro");
	strcpy(patterns[0].product_string, "FalconRotator");
	INDIGO_REGISER_MATCH_PATTERNS(rotator_template, patterns, 1);

	SET_DRIVER_INFO(info, "PegasusAstro Falcon rotator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(falcon_private_data));
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
