// Copyright (c) 2023-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_rotator_falcon.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_falcon.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_rotator_falcon"
#define DRIVER_LABEL         "PegasusAstro Falcon rotator"
#define ROTATOR_DEVICE_NAME  "Pegasus Falcon rotator"
#define PRIVATE_DATA         ((falcon_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_timer *rotator_connection_handler_timer;
	indigo_timer *rotator_position_handler_timer;
	indigo_timer *rotator_direction_handler_timer;
	indigo_timer *rotator_abort_motion_handler_timer;
	indigo_timer *rotator_relative_move_handler_timer;
	//+ data
	char response[128];
	//- data
} falcon_private_data;

#pragma mark - Low level code

//+ code

static bool falcon_command(indigo_device *device, char *command, ...) {
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

static bool falcon_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (falcon_command(device, "F#") && !strncmp(PRIVATE_DATA->response, "FR_OK", 5)) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value ,"Falcon Rotator");
			if (falcon_command(device, "FV") && !strncmp(PRIVATE_DATA->response, "FV:", 3)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response + 3);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		} else {
			indigo_uni_close(&PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200, INDIGO_LOG_DEBUG);
			if (falcon_command(device, "F#") && !strncmp(PRIVATE_DATA->response, "F2R_", 4)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Falcon Rotator v2");
				if (falcon_command(device, "FV") && !strncmp(PRIVATE_DATA->response, "FV:", 3)) {
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response + 3);
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void falcon_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = falcon_open(device);
		if (connection_result) {
			//+ rotator.on_connect
			falcon_command(device, "FH");
			falcon_command(device, "DR:0");
			if (falcon_command(device, "FA")) {
				if (!strncmp(PRIVATE_DATA->response, "FR_OK", 5)) {
					char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt);
					token = strtok_r(NULL, ":", &pnt); // position_in_deg
					if (token) {
						ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value = indigo_atod(token);
					}
					token = strtok_r(NULL, ":", &pnt); // is_running
					token = strtok_r(NULL, ":", &pnt); // limit_detect
					token = strtok_r(NULL, ":", &pnt); // do_derotation
					token = strtok_r(NULL, ":", &pnt); // motor_reverse
					if (token) {
						if (*token == '0') {
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_NORMAL_ITEM, true);
						} else {
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_REVERSED_ITEM, true);
						}
					}
				} else if (!strncmp(PRIVATE_DATA->response, "F2R:", 4)) {
					char *pnt, *token = strtok_r(PRIVATE_DATA->response, ":", &pnt); // position_in_deg
					if (token) {
						ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value = indigo_atod(token);
					}
					token = strtok_r(NULL, ":", &pnt); // is_running
					token = strtok_r(NULL, ":", &pnt); // speed
					token = strtok_r(NULL, ":", &pnt); // microsteps
					token = strtok_r(NULL, ":", &pnt); // direction
					if (token) {
						if (*token == '0') {
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_NORMAL_ITEM, true);
						} else {
							indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, ROTATOR_DIRECTION_REVERSED_ITEM, true);
						}
					}
				}
			}
			//- rotator.on_connect
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_position_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_direction_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_abort_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_relative_move_handler_timer);
		falcon_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void rotator_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ rotator.ROTATOR_POSITION.on_change
	ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		if (falcon_command(device, "MD:%0.2f", ROTATOR_POSITION_ITEM->number.target) && !strncmp(PRIVATE_DATA->response, "MD:", 3)) {
			while (true) {
				if (falcon_command(device, "FD") && !strncmp(PRIVATE_DATA->response, "FD:", 3)) {
					ROTATOR_POSITION_ITEM->number.value = indigo_atod(PRIVATE_DATA->response + 3);
					indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
				} else {
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				if (falcon_command(device, "FR") && !strncmp(PRIVATE_DATA->response, "FR:", 3)) {
					if (!strcmp(PRIVATE_DATA->response, "FR:1")) {
						pthread_mutex_unlock(&PRIVATE_DATA->mutex);
						indigo_sleep(0.5);
						pthread_mutex_lock(&PRIVATE_DATA->mutex);
						continue;
					}
				}
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
			if (falcon_command(device, "FD") && !strncmp(PRIVATE_DATA->response, "FD:", 3)) {
				ROTATOR_POSITION_ITEM->number.value = indigo_atod(PRIVATE_DATA->response + 3);
			} else {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (falcon_command(device, "SD:%0.2f", ROTATOR_POSITION_ITEM->number.target) && !strncmp(PRIVATE_DATA->response, "SD:", 3)) {
			if (falcon_command(device, "FD") && !strncmp(PRIVATE_DATA->response, "FD:", 3)) {
				ROTATOR_POSITION_ITEM->number.value = indigo_atod(PRIVATE_DATA->response + 3);
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- rotator.ROTATOR_POSITION.on_change
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_direction_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_DIRECTION.on_change
	if (!falcon_command(device, "FN:%d", ROTATOR_DIRECTION_NORMAL_ITEM->sw.value ? 0 : 1) && !strncmp(PRIVATE_DATA->response, "FN:", 3)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (!falcon_command(device, "FH") && !strncmp(PRIVATE_DATA->response, "FH:", 3)) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_ABORT_MOTION.on_change
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_relative_move_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ rotator.ROTATOR_RELATIVE_MOVE.on_change
	ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value + ROTATOR_RELATIVE_MOVE_ITEM->number.value;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	rotator_position_handler(device);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_RELATIVE_MOVE_PROPERTY->state = ROTATOR_POSITION_PROPERTY->state;
	//- rotator.ROTATOR_RELATIVE_MOVE.on_change
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ rotator.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- rotator.on_attach
		ROTATOR_POSITION_PROPERTY->hidden = false;
		//+ rotator.ROTATOR_POSITION.on_attach
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 359.9;
		//- rotator.ROTATOR_POSITION.on_attach
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_connection_handler, &PRIVATE_DATA->rotator_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler, rotator_position_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_direction_handler, rotator_direction_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler, rotator_abort_motion_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_RELATIVE_MOVE_PROPERTY, rotator_relative_move_handler, rotator_relative_move_handler_timer);
		return INDIGO_OK;
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Main code

indigo_result indigo_rotator_falcon(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static falcon_private_data *private_data = NULL;
	static indigo_device *rotator = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "FalconRotator");
			strcpy(patterns[0].vendor_string, "Pegasus Astro");
			INDIGO_REGISER_MATCH_PATTERNS(rotator_template, patterns, 1);
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
