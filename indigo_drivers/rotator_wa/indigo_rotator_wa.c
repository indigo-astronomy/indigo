// Copyright (C) 2024 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO WandererAstro Rotator driver
 \file indigo_rotator_wa.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_rotator_wa"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/termios.h>

#include <indigo/indigo_io.h>

#include "indigo_rotator_wa.h"

#define PRIVATE_DATA				((wa_private_data *)device->private_data)

typedef struct {
	int handle;
	pthread_mutex_t mutex;
	indigo_timer *position_timer;
	int steps_degree;       /* steps per degree */
	double current_position;
} wa_private_data;

typedef struct {
	char model_id[50];
	char firmware[20];
	double position;
	double backlash;
	bool reverse;
	double last_move;
} wr_status_t;

int wr_parse_status(char *response, wr_status_t *status) {
	char *buf;
	if (!strncmp(response, "WandererRotator", strlen("WandererRotator"))) {
		status->last_move = 0;

		char* token = strtok_r(response, "A", &buf);
		if (token == NULL) {
			return false;
		}
		strncpy(status->model_id, token, sizeof(status->model_id));

		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		strncpy(status->firmware, token, sizeof(status->firmware));

		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		status->position = atof(token)/1000.0;

		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		status->backlash = atof(token);

		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		status->reverse = atoi(token);
	} else {
		status->model_id[0] = '\0';

		status->firmware[0] = '\0';

		char *token = strtok_r(response, "A", &buf);
		if (token == NULL) {
			return false;
		}
		status->last_move = atof(token);

		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		status->position = atof(token)/1000.0;
	}

	INDIGO_DRIVER_ERROR(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nposition = %.3f\nlast_move = %.2f\n",
		status->model_id,
		status->firmware,
		status->position,
		status->last_move
	);
	return true;
}

static bool wa_command(indigo_device *device, char *command, char *response, int max) {
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	indigo_write(PRIVATE_DATA->handle, "\n", 1);
	if (response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) == 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> no response", command);
			return false;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static void rotator_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (wa_command(device, "1500001", response, sizeof(response))) {
		wr_status_t status;
		if (wr_parse_status(response, &status)) {
			if (PRIVATE_DATA->current_position != status.position) {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				ROTATOR_POSITION_ITEM->number.value = indigo_range360(status.position);
				PRIVATE_DATA->current_position == status.position;
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
			if (ROTATOR_BACKLASH_ITEM->number.value != status.backlash) {
				ROTATOR_BACKLASH_ITEM->number.value = status.backlash;
				indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
			}
			if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value && status.reverse) {
				ROTATOR_DIRECTION_NORMAL_ITEM->sw.value = false;
				ROTATOR_DIRECTION_REVERSED_ITEM->sw.value = true;
				indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
			} else if (ROTATOR_DIRECTION_REVERSED_ITEM->sw.value && !status.reverse) {
				ROTATOR_DIRECTION_NORMAL_ITEM->sw.value = true;
				ROTATOR_DIRECTION_REVERSED_ITEM->sw.value = false;
				indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->position_timer);
}

static void rotator_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			if (wa_command(device, "1500001", response, sizeof(response))) {
				wr_status_t status;
				if (wr_parse_status(response, &status)) {
					if (!strncmp(response,"WandererRotatorMini", strlen("WandererRotatorMini"))) {
						PRIVATE_DATA->steps_degree = 1142;
					} else if (!strncmp(response,"WandererRotatorLite", strlen("WandererRotatorLite"))) {
						PRIVATE_DATA->steps_degree = 1199;
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
					}
					ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.value = status.position;
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, status.model_id);
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, status.firmware);
					indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_timer(device, 1, rotator_position_handler, &PRIVATE_DATA->position_timer);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
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

static void rotator_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_direction_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[8] = "1700000";
	if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
		command[6] = '0';
	} else if(ROTATOR_DIRECTION_REVERSED_ITEM->sw.value) {
		command[6] = '1';
	}
	if(wa_command(device, command, NULL, 0)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_relative_move_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	snprintf(command, sizeof(command), "%d", (int)(ROTATOR_RELATIVE_MOVE_ITEM->number.target * PRIVATE_DATA->steps_degree));
	if(wa_command(device, command, NULL, 0)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else {
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// -------------------------------------------------------------------------------- INDIGO rotator device implementation

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);


static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;
		ROTATOR_RAW_POSITION_PROPERTY->hidden = false;
		ROTATOR_POSITION_OFFSET_PROPERTY->hidden = false;
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 359.999;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_ITEM->number.min = 0;
		ROTATOR_BACKLASH_ITEM->number.max = 5;
		strncpy(ROTATOR_BACKLASH_ITEM->label, "Backlash (°)", INDIGO_VALUE_SIZE);
		strncpy(ROTATOR_BACKLASH_ITEM->number.format, "%g", INDIGO_VALUE_SIZE);
		DEVICE_PORTS_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->hidden = false;
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "WandederAstro Rotator");
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
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
		indigo_set_timer(device, 0, rotator_direction_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_RELATIVE_MOVE
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_RELATIVE_MOVE_PROPERTY, property, false);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_relative_move_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_position_handler, &PRIVATE_DATA->position_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_abort_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// --------------------------------------------------------------------------------

static wa_private_data *private_data = NULL;
static indigo_device *rotator = NULL;

indigo_result indigo_rotator_wa(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		"WandererAstro rotator",
		rotator_attach,
		rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "WandererAstro rotator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(wa_private_data));
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
