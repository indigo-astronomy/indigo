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

#define DRIVER_VERSION 0x03000003
#define DRIVER_NAME	"indigo_rotator_wa"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_wa.h"

#define PRIVATE_DATA				((wa_private_data *)device->private_data)

#define X_SET_ZERO_POSITION_PROPERTY       (PRIVATE_DATA->set_zero_position_property)
#define X_SET_ZERO_POSITION_ITEM           (X_SET_ZERO_POSITION_PROPERTY->items+0)
#define X_SET_ZERO_POSITION_PROPERTY_NAME  "X_SET_ZERO_POSITION"
#define X_SET_ZERO_POSITION_ITEM_NAME      "SET_ZERO_POSITION"

typedef struct {
	indigo_uni_handle *handle;
	pthread_mutex_t mutex;
	indigo_timer *position_timer;
	int steps_degree;       /* steps per degree */
	double current_position;
	double pivot_position;
	indigo_property *set_zero_position_property;
} wa_private_data;

typedef struct {
	char model_id[50];
	char firmware[20];
	double position;
	double backlash;
	bool reverse;
	double last_move;
	bool has_power;
} wr_status_t;

bool wr_parse_status(char *response, wr_status_t *status) {
	char *buf;
	if (response == NULL || status == NULL) {
		return false;
	}
	status->has_power = true;
	if (!strncmp(response, "NP", strlen("NP"))) {
		memset(status, 0, sizeof(wr_status_t));
		status->has_power = false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "has_power = 0");
		return true;
	} else if (!strncmp(response, "WandererRotator", strlen("WandererRotator"))) {
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

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nposition = %.3f\nbacklash = %3f\nreverse = %d\nlast_move = %.2f\nhas_power = %d",
		status->model_id,
		status->firmware,
		status->position,
		status->backlash,
		status->reverse,
		status->last_move,
		status->has_power
	);

	return true;
}

static bool wr_command(indigo_device *device, const char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0) {
			indigo_uni_write(PRIVATE_DATA->handle, "\n", 1);
			if (response == NULL) {
				return true;
			}
			if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "\n", "", INDIGO_DELAY(1)) >= 0) {
				return true;
			}
		}
	}
	return false;
}

static void wr_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static void update_pivot_position(indigo_device *device) {
	PRIVATE_DATA->pivot_position = round((PRIVATE_DATA->current_position + ROTATOR_POSITION_OFFSET_ITEM->number.value)/360) * 360;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "new pivot_position = %.3f", PRIVATE_DATA->pivot_position);
}

static void rotator_update_status(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (wr_command(device, "1500001", response, sizeof(response))) {
		wr_status_t status;
		if (wr_parse_status(response, &status)) {
			if (PRIVATE_DATA->current_position != status.position) {
				ROTATOR_POSITION_ITEM->number.value = indigo_range360(status.position + ROTATOR_POSITION_OFFSET_ITEM->number.value);
				ROTATOR_RAW_POSITION_ITEM->number.value = status.position;
				PRIVATE_DATA->current_position = status.position;
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
				indigo_update_property(device, ROTATOR_RAW_POSITION_PROPERTY, NULL);
			}
			if (status.model_id[0] != '\0') {
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
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static bool rotator_handle_position(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	int result;

	while ((result = indigo_uni_read_section(PRIVATE_DATA->handle, response, sizeof(response), "\n", "", INDIGO_DELAY(0.1)) <= 0)) {
		if (ROTATOR_ABORT_MOTION_ITEM->sw.value) {
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			return false;
		}
	}

	if (result < 0)	{
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "READ -> no response");
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		return false;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "READ -> %s", response);

	wr_status_t status = {0};
	if (wr_parse_status(response, &status)) {
		if (!status.has_power) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			ROTATOR_RELATIVE_MOVE_ITEM->number.value = 0;
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "The rotator is not powered on");
			indigo_send_message(device, "Error: The rotator is not powered on");
			return false;
		}
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_POSITION_ITEM->number.value = indigo_range360(status.position + ROTATOR_POSITION_OFFSET_ITEM->number.value);
		ROTATOR_RAW_POSITION_ITEM->number.value = status.position;
		PRIVATE_DATA->current_position = status.position;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_RAW_POSITION_PROPERTY, NULL);
		ROTATOR_RELATIVE_MOVE_ITEM->number.value = 0;
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	}
	return true;
}

static void rotator_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle != NULL) {
			indigo_sleep(2); // wait for the rotator to initialize after opening the serial port
			if (wr_command(device, "1500001", response, sizeof(response))) {
				wr_status_t status;
				if (wr_parse_status(response, &status)) {
					if (!strncmp(response,"WandererRotatorMini", strlen("WandererRotatorMini"))) {
						PRIVATE_DATA->steps_degree = 1142;
					} else if (!strncmp(response,"WandererRotatorLite", strlen("WandererRotatorLite"))) {
						PRIVATE_DATA->steps_degree = 1199;
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
						wr_close(device);
					}
					ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = indigo_range360(status.position + ROTATOR_POSITION_OFFSET_ITEM->number.value);
					PRIVATE_DATA->current_position = status.position;
					update_pivot_position(device);
					ROTATOR_RAW_POSITION_ITEM->number.value = status.position;
					ROTATOR_BACKLASH_ITEM->number.value = status.backlash;
					ROTATOR_DIRECTION_NORMAL_ITEM->sw.value = !status.reverse;
					ROTATOR_DIRECTION_REVERSED_ITEM->sw.value = status.reverse;
					INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, status.model_id);
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, status.firmware);
					indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
					indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
					indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
					indigo_update_property(device, ROTATOR_RAW_POSITION_PROPERTY, NULL);
					indigo_define_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
					wr_close(device);
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotator not detected");
				wr_close(device);
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		if (PRIVATE_DATA->handle != NULL) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			wr_close(device);
		}
		PRIVATE_DATA->current_position = 0;
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
	wr_command(device, "stop", NULL, 0);
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	rotator_update_status(device);
}

static void rotator_direction_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[8] = "1700000";
	if (ROTATOR_DIRECTION_NORMAL_ITEM->sw.value) {
		command[6] = '0';
	} else if (ROTATOR_DIRECTION_REVERSED_ITEM->sw.value) {
		command[6] = '1';
	}
	if (wr_command(device, command, NULL, 0)) {
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
	int move = (int)round(ROTATOR_RELATIVE_MOVE_ITEM->number.target * PRIVATE_DATA->steps_degree);
	if (abs(move) < 1) {
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_RELATIVE_MOVE_ITEM->number.value = 0;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	snprintf(command, sizeof(command), "%d", move);
	if (wr_command(device, command, NULL, 0)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else {
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	rotator_handle_position(device);
}

static double adjust_move(double base_angle, double pivot_position, double move_deg) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "base_angle = %.4f\npivot_position = %.4f\nmove_deg = %.4f\n", base_angle, pivot_position, move_deg);
	if ((move_deg < 0) && (base_angle + move_deg < pivot_position - 180)) {
		return move_deg + 360;
	}
	if ((move_deg > 0) && (base_angle + move_deg > pivot_position + 180)) {
		return move_deg - 360;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "move_deg corrected = %.4f\n", move_deg);
	return move_deg;
}

static void rotator_absolute_move_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	char response[64];
	if (wr_command(device, "1500001", response, sizeof(response))) {
		wr_status_t status = {0};
		if (wr_parse_status(response, &status)) {
			double base_angle = status.position + ROTATOR_POSITION_OFFSET_ITEM->number.value;
			ROTATOR_POSITION_ITEM->number.value = indigo_range360(base_angle);
			double move_deg = ROTATOR_POSITION_ITEM->number.target - indigo_range360(base_angle);
			move_deg = adjust_move(base_angle, PRIVATE_DATA->pivot_position, move_deg);
			int move_steps = (int)round(move_deg * PRIVATE_DATA->steps_degree);
			if (abs(move_steps) < 1) {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
				pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				return;
			}
			/* use fast speed for goto (+1000000 is fast speed) */
			snprintf(command, sizeof(command), "%d", move_steps + 1000000);
			if (wr_command(device, command, NULL, 0)) {
				ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			} else {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "wr_parse_status(): failed to get current position");
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	rotator_handle_position(device);
}

static void rotator_handle_zero_position(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	X_SET_ZERO_POSITION_ITEM->sw.value = false;
	if (wr_command(device, "1500002", NULL, 0)) {
		X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_POSITION_ITEM->number.value =
		ROTATOR_POSITION_ITEM->number.target =
		ROTATOR_RAW_POSITION_ITEM->number.value =
		ROTATOR_POSITION_OFFSET_ITEM->number.value =
		ROTATOR_POSITION_OFFSET_ITEM->number.target = 0;
		PRIVATE_DATA->current_position = 0;
		update_pivot_position(device);
		indigo_rotator_save_calibration(device);
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_POSITION_OFFSET_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_RAW_POSITION_PROPERTY, NULL);
	} else {
		X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	snprintf(command, sizeof(command), "1600%03d", (int)(ROTATOR_BACKLASH_ITEM->number.target * 10));
	if (wr_command(device, command, NULL, 0)) {
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
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
		ROTATOR_POSITION_ITEM->number.max = 360;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_ITEM->number.min = 0;
		ROTATOR_BACKLASH_ITEM->number.max = 5;
		strncpy(ROTATOR_BACKLASH_ITEM->label, "Backlash [°]", INDIGO_VALUE_SIZE);
		strncpy(ROTATOR_BACKLASH_ITEM->number.format, "%g", INDIGO_VALUE_SIZE);
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_PORT_PROPERTY->hidden = false;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "WandederAstro Rotator");
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		X_SET_ZERO_POSITION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SET_ZERO_POSITION_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Set current position as mechanical zero", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_SET_ZERO_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}

		indigo_init_switch_item(X_SET_ZERO_POSITION_ITEM, X_SET_ZERO_POSITION_ITEM_NAME, "Set mechanical zero", false);
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
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SET_ZERO_POSITION_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
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
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SET_ZERO_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SET_ZERO_POSITION
		indigo_property_copy_values(X_SET_ZERO_POSITION_PROPERTY, property, false);
		X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_handle_zero_position, NULL);
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
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(ROTATOR_RELATIVE_MOVE_PROPERTY, property, false);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_relative_move_handler, &PRIVATE_DATA->position_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_absolute_move_handler, &PRIVATE_DATA->position_timer);
			return INDIGO_OK;
		} else {
			update_pivot_position(device);
			/* The rest of ROTATOR_ON_POSITION_SET_SYNC is handled by the base class */
		}
	} else if (indigo_property_match_changeable(ROTATOR_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_BACKLASH
		indigo_property_copy_values(ROTATOR_BACKLASH_PROPERTY, property, false);
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_backlash_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION_OFFSET
		indigo_property_copy_values(ROTATOR_ON_POSITION_SET_PROPERTY, property, false);
		update_pivot_position(device);
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
	indigo_release_property(X_SET_ZERO_POSITION_PROPERTY);
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

	if (action == last_action) {
		return INDIGO_OK;
	}

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
