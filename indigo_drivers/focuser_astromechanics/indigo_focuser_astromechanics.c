// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_astromechanics.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_astromechanics.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000008
#define DRIVER_NAME          "indigo_focuser_astromechanics"
#define DRIVER_LABEL         "ASTROMECHANICS Focuser"
#define FOCUSER_DEVICE_NAME  "ASTROMECHANICS Focuser"
#define PRIVATE_DATA         ((astromechanics_private_data *)device->private_data)

//+ define

// A move whose position does not change for this many polls (0.1 s each) has stalled.
#define ASTROMECHANICS_STALL_POLLS 100
#define FOCUSER_IN_MOTION    (PRIVATE_DATA->moving || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)

//- define

#pragma mark - Property definitions

#define X_FOCUSER_APERTURE_PROPERTY      (PRIVATE_DATA->x_focuser_aperture_property)
#define X_FOCUSER_APERTURE_ITEM          (X_FOCUSER_APERTURE_PROPERTY->items + 0)

#define X_FOCUSER_APERTURE_PROPERTY_NAME "X_FOCUSER_APERTURE"
#define X_FOCUSER_APERTURE_ITEM_NAME     "APERURE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_focuser_aperture_property;
	//+ data
	char response[16];
	int motion_polls;
	int motion_position;
	bool moving;
	bool external_motion;
	//- data
} astromechanics_private_data;

#pragma mark - Low level code

//+ code

static bool astromechanics_command(indigo_device *device, char *command, int response, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, response);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
		if (result > 0 && response > 0) {
			// '#' terminates and is kept, so a reply cut short by a timeout can be told from a complete one
			result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "#", "\n", INDIGO_DELAY(1), INDIGO_DELAY(0.5));
		}
	}
	return result > 0;
}

static bool astromechanics_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 38400, INDIGO_LOG_DEBUG);
	return PRIVATE_DATA->handle != NULL;
}

static void astromechanics_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ focuser.code

static bool astromechanics_position(indigo_device *device, int *position) {
	if (!astromechanics_command(device, "P#", 16)) {
		return false;
	}
	char terminator = 0, tail;
	int digits = 0;
	if (sscanf(PRIVATE_DATA->response, "%d%n%c%c", position, &digits, &terminator, &tail) != 2 || terminator != '#' || digits < 1 || digits > 4 || *position < 0 || *position > 9999) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid position reply '%s'", PRIVATE_DATA->response);
		return false;
	}
	return true;
}

static void focuser_motion_finalizer(indigo_device *device) {
	int position;
	if (!astromechanics_position(device, &position)) {
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->external_motion = false;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Focuser position could not be read");
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return;
	}
	bool progress = position != PRIVATE_DATA->motion_position;
	PRIVATE_DATA->motion_position = position;
	FOCUSER_POSITION_ITEM->number.value = position;
	const char *message = NULL;
	if (PRIVATE_DATA->external_motion ? !progress : position == (int)FOCUSER_POSITION_ITEM->number.target) {
		/* motion the driver did not command ends where the lens settles */
		if (PRIVATE_DATA->external_motion) {
			FOCUSER_POSITION_ITEM->number.target = position;
		}
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->external_motion = false;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (progress) {
		PRIVATE_DATA->motion_polls = ASTROMECHANICS_STALL_POLLS;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	} else if (--PRIVATE_DATA->motion_polls <= 0) {
		PRIVATE_DATA->moving = false;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		message = "Focuser does not reach the target";
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	}
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	if (message != NULL) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void astromechanics_start_motion(indigo_device *device, int position) {
	indigo_cancel_pending_handler(device, focuser_motion_finalizer);
	FOCUSER_POSITION_ITEM->number.target = position;
	const char *message = NULL;
	if (astromechanics_command(device, "M%04d#", 0, position)) {
		PRIVATE_DATA->motion_polls = ASTROMECHANICS_STALL_POLLS;
		PRIVATE_DATA->motion_position = (int)FOCUSER_POSITION_ITEM->number.value;
		PRIVATE_DATA->moving = true;
		PRIVATE_DATA->external_motion = false;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		PRIVATE_DATA->moving = false;
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		message = "Move command could not be sent";
	}
	FOCUSER_STEPS_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	if (message != NULL) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

//- focuser.code

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = astromechanics_open(device);
		if (connection_result) {
			//+ focuser.on_connect
			int position, settled;
			connection_result = astromechanics_position(device, &position);
			if (connection_result) {
				/* a second reading tells a lens still moving from a previous session from a resting one */
				indigo_usleep(0.1 * ONE_SECOND_DELAY);
				connection_result = astromechanics_position(device, &settled);
			}
			if (connection_result) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = settled;
				PRIVATE_DATA->moving = PRIVATE_DATA->external_motion = settled != position;
				PRIVATE_DATA->motion_position = settled;
				FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				if (PRIVATE_DATA->moving) {
					indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
				}
			} else {
				astromechanics_close(device);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		PRIVATE_DATA->moving = false;
		PRIVATE_DATA->external_motion = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_FOCUSER_APERTURE_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
		astromechanics_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_x_focuser_aperture_handler(indigo_device *device) {
	X_FOCUSER_APERTURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_FOCUSER_APERTURE.on_change
	if (!astromechanics_command(device, "A%02d#", 0, (int)X_FOCUSER_APERTURE_ITEM->number.value)) {
		X_FOCUSER_APERTURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_FOCUSER_APERTURE.on_change
	indigo_update_property(device, X_FOCUSER_APERTURE_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int direction = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1;
	int position = (int)FOCUSER_POSITION_ITEM->number.value + direction * (int)FOCUSER_STEPS_ITEM->number.value;
	if (position < 0) {
		position = 0;
	} else if (position > 9999) {
		position = 9999;
	}
	astromechanics_start_motion(device, position);
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	}
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	astromechanics_start_motion(device, position);
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_execute_handler_in(device, 0.1, focuser_motion_finalizer);
	}
	//- focuser.FOCUSER_POSITION.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = true;
		//- focuser.on_attach
		X_FOCUSER_APERTURE_PROPERTY = indigo_init_number_property(NULL, device->name, X_FOCUSER_APERTURE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Aperture", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_FOCUSER_APERTURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_FOCUSER_APERTURE_ITEM, X_FOCUSER_APERTURE_ITEM_NAME, "Aperture", 0, 50, 1, 0);
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 9999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 9999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser.FOCUSER_POSITION.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_APERTURE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_APERTURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_APERTURE_PROPERTY, focuser_x_focuser_aperture_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_IN_MOTION, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_STEPS.on_change_request
		/* the accepted move is pending until its handler runs */
		PRIVATE_DATA->moving = true;
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_IN_MOTION, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_POSITION.on_change_request
		/* the accepted move is pending until its handler runs */
		PRIVATE_DATA->moving = true;
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_APERTURE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

#pragma mark - Main code

indigo_result indigo_focuser_astromechanics(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static astromechanics_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (astromechanics_private_data *)indigo_safe_malloc(sizeof(astromechanics_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
