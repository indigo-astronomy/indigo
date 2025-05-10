// Copyright (c) 2024-2025 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_focuser_fc3.driver

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_fc3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_focuser_fc3"
#define DRIVER_LABEL         "PegasusAstro FocusCube v3 Focuser"
#define FOCUSER_DEVICE_NAME  "Pegasus FocusCube3"
#define PRIVATE_DATA         ((fc3_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_timer *focuser_timer;
	indigo_timer *focuser_connection_handler_timer;
	indigo_timer *focuser_backlash_handler_timer;
	indigo_timer *focuser_reverse_motion_handler_timer;
	indigo_timer *focuser_temperature_handler_timer;
	indigo_timer *focuser_speed_handler_timer;
	indigo_timer *focuser_steps_handler_timer;
	indigo_timer *focuser_on_position_set_handler_timer;
	indigo_timer *focuser_position_handler_timer;
	indigo_timer *focuser_abort_motion_handler_timer;
	indigo_timer *focuser_direction_handler_timer;
	indigo_timer *focuser_limits_handler_timer;
	//+ data
	char response[128];
	//- data
} fc3_private_data;

#pragma mark - Low level code

//+ code

static bool fc3_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool fc3_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (fc3_command(device, "##") && !strncmp(PRIVATE_DATA->response, "FC3_", 4)) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "FocusCube v3");
			if (fc3_command(device, "FV") && !strncmp(PRIVATE_DATA->response, "FV:", 3)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response + 3);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void fc3_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ focuser.on_timer
	bool update = false;
	if (fc3_command(device, "FA") && !strncmp(PRIVATE_DATA->response, "FC3:", 4)) {
		char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
		token = strtok_r(NULL, ":", &pnt); // position
		if (token) {
			int pos = atoi(token);
			if (FOCUSER_POSITION_ITEM->number.value != pos) {
				FOCUSER_POSITION_ITEM->number.value = pos;
				update = true;
			}
		}
		token = strtok_r(NULL, ":", &pnt); // is_running
		if (token) {
			if (*token == '0') {
				if (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE) {
					FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					update = true;
				}
			} else {
				if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE) {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
					update = true;
				}
			}
		}
		token = strtok_r(NULL, ":", &pnt); // temperature
		if (token) {
			double temp = indigo_atod(token);
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		}
	}
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->focuser_timer);
	//- focuser.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = fc3_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			if (fc3_command(device, "FA") && !strncmp(PRIVATE_DATA->response, "FC3:", 4)) {
				char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
				token = strtok_r(NULL, ":", &pnt); // position
				if (token) {
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(token);
				}
				token = strtok_r(NULL, ":", &pnt); // is_running
				if (token) {
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				token = strtok_r(NULL, ":", &pnt); // temperature
				if (token) {
					FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = indigo_atod(token);
				}
				token = strtok_r(NULL, ":", &pnt); // direction
				if (token) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, *token == '1' ? FOCUSER_REVERSE_MOTION_ENABLED_ITEM : FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // backlash
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
				}
			}
			if (fc3_command(device, "SP") && !strncmp(PRIVATE_DATA->response, "SP:", 3)) {
				FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = atoi(PRIVATE_DATA->response + 3);
			}
			//- focuser.on_connect
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_backlash_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_reverse_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_temperature_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_speed_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_steps_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_on_position_set_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_position_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_abort_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_direction_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_limits_handler_timer);
		//+ focuser.on_disconnect
		fc3_command(device, "FH");
		//- focuser.on_disconnect
		fc3_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!fc3_command(device, "BL:%d", (int)FOCUSER_BACKLASH_ITEM->number.value)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_reverse_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_REVERSE_MOTION.on_change
	if (!fc3_command(device, "FD:%d", (int)FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value? 0 : 1)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_temperature_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!fc3_command(device, "SP:%d", (int)FOCUSER_SPEED_ITEM->number.value)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
		if (position + FOCUSER_STEPS_ITEM->number.target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
			FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value - position;
		}
	} else {
		if (position - FOCUSER_STEPS_ITEM->number.target < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value) {
			FOCUSER_STEPS_ITEM->number.value = FOCUSER_STEPS_ITEM->number.target = position - FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		}
	}
	if (fc3_command(device, "FG:%d", (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1))) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_on_position_set_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_POSITION.on_change
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		int position = (int)FOCUSER_POSITION_ITEM->number.target;
		if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value) {
			position = (int)FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
		}
		if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value) {
			position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
		}
		FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
		if (fc3_command(device, "FM:%d", (int)FOCUSER_POSITION_ITEM->number.value)) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		if (!fc3_command(device, "FN:%d", (int)FOCUSER_POSITION_ITEM->number.target)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (fc3_command(device, "FH")) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_direction_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_limits_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- focuser.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		//- focuser.FOCUSER_BACKLASH.on_attach
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_DIRECTION_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -9999999;
		strcpy(FOCUSER_LIMITS_MIN_POSITION_ITEM->number.format, "%.0f");
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 9999999;
		strcpy(FOCUSER_LIMITS_MAX_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_LIMITS.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_connection_handler, &PRIVATE_DATA->focuser_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler, focuser_backlash_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_reverse_motion_handler, focuser_reverse_motion_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_TEMPERATURE_PROPERTY, focuser_temperature_handler, focuser_temperature_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler, focuser_speed_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler, focuser_steps_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ON_POSITION_SET_PROPERTY, focuser_on_position_set_handler, focuser_on_position_set_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler, focuser_position_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler, focuser_abort_motion_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_DIRECTION_PROPERTY, focuser_direction_handler, focuser_direction_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_limits_handler, focuser_limits_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_fc3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static fc3_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			patterns[0].vendor_id = 0x303a;
			patterns[0].product_id = 0x9000;
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(fc3_private_data));
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
