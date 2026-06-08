// Copyright (C) 2026 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** Askar-WAF USB CDC focuser driver
 \file indigo_focuser_askar.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_askar"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>

#include <indigo/indigo_io.h>

#include "indigo_focuser_askar.h"

// USB CDC ignores the baud rate, but DEVICE_PORT_PROPERTY exposes one.
#define SERIAL_BAUDRATE            "115200"

#define RESPONSE_TIMEOUT_MS        500

#define ASKAR_CMD_LEN              64

// Logical step range allowed by the protocol's set-max-travel command.
#define ASKAR_MAX_TRAVEL_MIN       100
#define ASKAR_MAX_TRAVEL_MAX       1000000

#define ASKAR_BACKLASH_MAX         10000

#define PRIVATE_DATA               ((askar_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected               gp_bits

typedef struct {
	int handle;
	int32_t current_position, target_position, max_position;
	indigo_timer *focuser_timer;
	pthread_mutex_t port_mutex;
} askar_private_data;

// -------------------------------------------------------------------------------- Low level protocol

// Send command, read reply (which ends with '#'). The reply is returned to the
// caller without the trailing '#' / "\r\n". Trailing CR/LF before/after the '#'
// is discarded so the caller can sscanf() the payload directly.
static bool askar_command(indigo_device *device, const char *command, char *response, int max) {
	long timeout_ms = RESPONSE_TIMEOUT_MS;
	struct timeval tv;
	char c;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush any stale bytes from a previous reply
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		long result = select(PRIVATE_DATA->handle + 1, &readout, NULL, NULL, &tv);
		if (result == 0) {
			break;
		}
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));

	// read response (terminated by '#'; \r and \n are ignored)
	if (response != NULL) {
		int index = 0;
		long t_sec = timeout_ms / 1000;
		long t_usec = (timeout_ms % 1000) * 1000;
		while (index < max - 1) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = t_sec;
			tv.tv_usec = t_usec;
			// only the first byte gets the full timeout - subsequent bytes are quick
			t_sec = 0;
			t_usec = 100000;
			long result = select(PRIVATE_DATA->handle + 1, &readout, NULL, NULL, &tv);
			if (result <= 0) {
				break;
			}
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}
			if (c == '\r' || c == '\n') {
				continue;
			}
			if (c == '#') {
				break;
			}
			response[index++] = c;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

// -------------------------------------------------------------------------------- Askar-WAF commands

// FV# -> FVv# (firmware version), e.g. "FV1.1.0"
static bool askar_get_firmware(indigo_device *device, char *firmware, int max) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "FV#", response, sizeof(response))) {
		return false;
	}
	// response starts with "FV"; everything after is the version
	if (strncmp(response, "FV", 2) != 0) {
		return false;
	}
	strncpy(firmware, response + 2, max - 1);
	firmware[max - 1] = 0;
	return true;
}

// FI# -> FImodel# (model name), e.g. "FIAskar-WAF"
static bool askar_get_model(indigo_device *device, char *model, int max) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "FI#", response, sizeof(response))) {
		return false;
	}
	if (strncmp(response, "FI", 2) != 0) {
		return false;
	}
	strncpy(model, response + 2, max - 1);
	model[max - 1] = 0;
	return true;
}

// Fp# -> Fpn# (read current logical position)
static bool askar_get_position(indigo_device *device, int32_t *position) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "Fp#", response, sizeof(response))) {
		return false;
	}
	int parsed = sscanf(response, "Fp%d", position);
	return parsed == 1;
}

// FQ# -> FQ0# (idle) / FQ1# (moving)
static bool askar_is_moving(indigo_device *device, bool *moving) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "FQ#", response, sizeof(response))) {
		return false;
	}
	int state = 0;
	int parsed = sscanf(response, "FQ%d", &state);
	if (parsed != 1) {
		return false;
	}
	*moving = (state != 0);
	return true;
}

// FM# -> FMn# (read max travel)
static bool askar_get_max_position(indigo_device *device, int32_t *max_position) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "FM#", response, sizeof(response))) {
		return false;
	}
	int parsed = sscanf(response, "FM%d", max_position);
	return parsed == 1;
}

// FXn# -> FXn# (set max travel; firmware clamps to [100, 1000000])
static bool askar_set_max_position(indigo_device *device, int32_t max_position) {
	if (max_position < ASKAR_MAX_TRAVEL_MIN || max_position > ASKAR_MAX_TRAVEL_MAX) {
		return false;
	}
	char command[ASKAR_CMD_LEN];
	char response[ASKAR_CMD_LEN] = {0};
	snprintf(command, sizeof(command), "FX%d#", max_position);
	if (!askar_command(device, command, response, sizeof(response))) {
		return false;
	}
	// "FE" reply means firmware rejected the value
	return strncmp(response, "FX", 2) == 0;
}

// FPn# -> FPn# (absolute move; async, poll Fp#/FQ# for completion)
static bool askar_goto_position(indigo_device *device, int32_t position) {
	char command[ASKAR_CMD_LEN];
	char response[ASKAR_CMD_LEN] = {0};
	snprintf(command, sizeof(command), "FP%d#", position);
	if (!askar_command(device, command, response, sizeof(response))) {
		return false;
	}
	return strncmp(response, "FP", 2) == 0;
}

// FYn# -> FYn# (set logical position without moving)
static bool askar_sync_position(indigo_device *device, int32_t position) {
	char command[ASKAR_CMD_LEN];
	char response[ASKAR_CMD_LEN] = {0};
	snprintf(command, sizeof(command), "FY%d#", position);
	if (!askar_command(device, command, response, sizeof(response))) {
		return false;
	}
	return strncmp(response, "FY", 2) == 0;
}

// FS# -> FS# (emergency stop)
static bool askar_stop(indigo_device *device) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "FS#", response, sizeof(response))) {
		return false;
	}
	return strncmp(response, "FS", 2) == 0;
}

// Fb# -> Fbn# (read user backlash offset)
static bool askar_get_backlash(indigo_device *device, int *backlash) {
	char response[ASKAR_CMD_LEN] = {0};
	if (!askar_command(device, "Fb#", response, sizeof(response))) {
		return false;
	}
	int parsed = sscanf(response, "Fb%d", backlash);
	return parsed == 1;
}

// FBn# -> FBn# (set user backlash offset; firmware clamps to [0, 10000])
static bool askar_set_backlash(indigo_device *device, int backlash) {
	char command[ASKAR_CMD_LEN];
	char response[ASKAR_CMD_LEN] = {0};
	if (backlash < 0) backlash = 0;
	if (backlash > ASKAR_BACKLASH_MAX) backlash = ASKAR_BACKLASH_MAX;
	snprintf(command, sizeof(command), "FB%d#", backlash);
	if (!askar_command(device, command, response, sizeof(response))) {
		return false;
	}
	return strncmp(response, "FB", 2) == 0;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	bool moving = false;
	int32_t position;

	if (!askar_is_moving(device, &moving)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_is_moving(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if (!askar_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_get_position(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->current_position = position;
	}

	FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
	if (!moving || PRIVATE_DATA->current_position == PRIVATE_DATA->target_position) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		PRIVATE_DATA->handle = -1;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, SERIAL_BAUDRATE);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = ASKAR_MAX_TRAVEL_MIN;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 100;

		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 100;
		FOCUSER_POSITION_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.max = ASKAR_MAX_TRAVEL_MAX;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = ASKAR_BACKLASH_MAX;

		// Features the Askar-WAF CDC protocol does not expose.
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		FOCUSER_COMPENSATION_PROPERTY->hidden = true;
		FOCUSER_MODE_PROPERTY->hidden = true;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;

		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->master_device != NULL;

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
	int32_t position;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				char *name = DEVICE_PORT_ITEM->text.value;
				if (!indigo_is_device_url(name, "askar")) {
					PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
				} else {
					indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
					PRIVATE_DATA->handle = indigo_open_network_device(name, 8080, &proto);
				}

				if (PRIVATE_DATA->handle < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					return;
				} else if (!askar_get_position(device, &position)) {
					int res = close(PRIVATE_DATA->handle);
					if (res < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					}
					indigo_global_unlock(device);
					device->is_connected = false;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: Askar-WAF did not respond");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, "Askar-WAF did not respond");
					return;
				} else {
					char model[ASKAR_CMD_LEN] = "Askar-WAF";
					char firmware[ASKAR_CMD_LEN] = "N/A";
					askar_get_model(device, model, sizeof(model));
					askar_get_firmware(device, firmware, sizeof(firmware));
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, model);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);

					PRIVATE_DATA->current_position = position;
					PRIVATE_DATA->target_position = position;
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = (double)position;

					if (askar_get_max_position(device, &PRIVATE_DATA->max_position)) {
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = (double)PRIVATE_DATA->max_position;
						FOCUSER_POSITION_ITEM->number.max = (double)PRIVATE_DATA->max_position;
						FOCUSER_STEPS_ITEM->number.max = (double)PRIVATE_DATA->max_position;
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_get_max_position(%d) failed", PRIVATE_DATA->handle);
					}

					int backlash = 0;
					if (askar_get_backlash(device, &backlash)) {
						FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = (double)backlash;
					}

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);

			askar_stop(device);

			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			int res = close(PRIVATE_DATA->handle);
			if (res < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			}
			PRIVATE_DATA->handle = -1;
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.target < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value ||
		    FOCUSER_POSITION_ITEM->number.target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if ((int32_t)FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			PRIVATE_DATA->target_position = (int32_t)FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				if (!askar_goto_position(device, PRIVATE_DATA->target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_goto_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* SYNC */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				if (!askar_sync_position(device, PRIVATE_DATA->target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_sync_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				int32_t position;
				if (askar_get_position(device, &position)) {
					PRIVATE_DATA->current_position = position;
					FOCUSER_POSITION_ITEM->number.value = (double)position;
				}
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);

			int32_t position;
			if (askar_get_position(device, &position)) {
				PRIVATE_DATA->current_position = position;
			}

			int32_t steps = (int32_t)FOCUSER_STEPS_ITEM->number.value;
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - steps;
			} else {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + steps;
			}
			// firmware clamps but mirror the limits here so the property values stay in sync
			if (PRIVATE_DATA->target_position > FOCUSER_POSITION_ITEM->number.max) {
				PRIVATE_DATA->target_position = (int32_t)FOCUSER_POSITION_ITEM->number.max;
			} else if (PRIVATE_DATA->target_position < FOCUSER_POSITION_ITEM->number.min) {
				PRIVATE_DATA->target_position = (int32_t)FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
			if (!askar_goto_position(device, PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_goto_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);

		if (!askar_stop(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_stop(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		int32_t position;
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = position;
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		int32_t requested = (int32_t)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		if (!askar_set_max_position(device, requested)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_set_max_position(%d, %d) failed", PRIVATE_DATA->handle, requested);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (askar_get_max_position(device, &PRIVATE_DATA->max_position)) {
			FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
			FOCUSER_POSITION_ITEM->number.max = (double)PRIVATE_DATA->max_position;
			FOCUSER_STEPS_ITEM->number.max = (double)PRIVATE_DATA->max_position;
		}
		// FXn# clamps the position, so re-read it as well.
		int32_t position;
		if (askar_get_position(device, &position)) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
			FOCUSER_POSITION_ITEM->number.value = (double)position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		int requested = (int)FOCUSER_BACKLASH_ITEM->number.target;
		if (!askar_set_backlash(device, requested)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "askar_set_backlash(%d, %d) failed", PRIVATE_DATA->handle, requested);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		}
		int backlash = 0;
		if (askar_get_backlash(device, &backlash)) {
			FOCUSER_BACKLASH_ITEM->number.value = (double)backlash;
		}
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------
indigo_result indigo_focuser_askar(indigo_driver_action action, indigo_driver_info *info) {
	static askar_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_ASKAR_NAME,
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Askar-WAF Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = indigo_safe_malloc(sizeof(askar_private_data));
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
