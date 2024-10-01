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

/** INDIGO PegasusAstro Prodigy Microfocuser driver
 \file indigo_focuser_prodigy.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_focuser_prodigy"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/termios.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_prodigy.h"

#define PRIVATE_DATA	((prodigy_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY							(PRIVATE_DATA->outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM					(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM					(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_USB_PORT_NAME_1_ITEM							(AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_USB_PORT_NAME_2_ITEM							(AUX_OUTLET_NAMES_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_PROPERTY							(PRIVATE_DATA->power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM								(AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM								(AUX_POWER_OUTLET_PROPERTY->items + 1)

#define AUX_USB_PORT_PROPERTY									(PRIVATE_DATA->usb_port_property)
#define AUX_USB_PORT_1_ITEM										(AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM										(AUX_USB_PORT_PROPERTY->items + 1)

#define X_AUX_REBOOT_PROPERTY									(PRIVATE_DATA->reboot_property)
#define X_AUX_REBOOT_ITEM											(X_AUX_REBOOT_PROPERTY->items + 0)

#define X_FOCUSER_PARK_PROPERTY								(PRIVATE_DATA->park_property)
#define X_FOCUSER_PARK_ITEM										(X_FOCUSER_PARK_PROPERTY->items + 0)

#define AUX_GROUP															"Powerbox"

typedef struct {
	int handle;
	int count;
	indigo_property *outlet_names_property;
	indigo_property *power_outlet_property;
	indigo_property *usb_port_property;
	indigo_property *reboot_property;
	indigo_property *park_property;
	indigo_timer *timer;
	pthread_mutex_t mutex;
} prodigy_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool prodigy_command(indigo_device *device, char *command, char *response, int max) {
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

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 9999;
		FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = 100;
		// -------------------------------------------------------------------------------- X_FOCUSER_PARK
		X_FOCUSER_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_PARK", FOCUSER_MAIN_GROUP, "Park", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_FOCUSER_PARK_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_PARK_ITEM, "PARK", "Park", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyPRDG");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Undefined");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Undefined");
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		//FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 400;
		FOCUSER_SPEED_ITEM->number.min = 100;
		FOCUSER_SPEED_ITEM->number.max = 1000;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 9999999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = -999999;
		FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 999999;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_PARK_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	char response[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (prodigy_command(device, "T", response, sizeof(response))) {
		double temp = indigo_atod(response);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	bool update = false;
	if (prodigy_command(device, "P", response, sizeof(response))) {
		int pos = atoi(response);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (prodigy_command(device, "I", response, sizeof(response))) {
		if (*response == '0') {
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
	if (update) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->count++ == 0) {
			if (PRIVATE_DATA->handle > 0) {
				if (prodigy_command(device, "#", response, sizeof(response)) && !strncmp(response, "OK_", 3)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%s OK", response + 3);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (prodigy_command(device, "A", response, sizeof(response)) && !strncmp(response, "OK_", 3)) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, token + 3);
				token = strtok_r(NULL, ":", &pnt); // status
				if (token) { // version
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, token);
				}
				token = strtok_r(NULL, ":", &pnt); // motor
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // temperature
					FOCUSER_TEMPERATURE_ITEM->number.value = FOCUSER_TEMPERATURE_ITEM->number.target = indigo_atod(token);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // position
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = atoi(token);
				}
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // moving status
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = *token == '1' ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
				}
				token = strtok_r(NULL, ":", &pnt); // led status
				token = strtok_r(NULL, ":", &pnt); // reverse
				token = strtok_r(NULL, ":", &pnt); // encoder
				token = strtok_r(NULL, ":", &pnt);
				if (token) { // backlash
					FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = atoi(token);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'A' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'A' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Undefined");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Undefined");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			if (PRIVATE_DATA->handle > 0) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
				prodigy_command(device, "H", response, sizeof(response));
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	snprintf(command, sizeof(command), "S:%d", (int)FOCUSER_SPEED_ITEM->number.value);
	if (prodigy_command(device, command, response, sizeof(response))) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	snprintf(command, sizeof(command), "G:%d", (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1));
	if (prodigy_command(device, command, NULL, 0)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		snprintf(command, sizeof(command), "M:%d", (int)FOCUSER_POSITION_ITEM->number.value);
		if (prodigy_command(device, command, response, sizeof(response))) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		snprintf(command, sizeof(command), "W:%d", (int)FOCUSER_POSITION_ITEM->number.value);
		if (prodigy_command(device, command, response, sizeof(response))) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (prodigy_command(device, "H", response, sizeof(response))) {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

//static void focuser_reverse_motion_handler(indigo_device *device) {
//	pthread_mutex_lock(&PRIVATE_DATA->mutex);
//	char command[16], response[64];
//	snprintf(command, sizeof(command), "N:%d", (int)FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value? 0 : 1);
//	if (prodigy_command(device, command, response, sizeof(response))) {
//		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
//	} else {
//		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
//	}
//	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
//	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
//}

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[64];
	snprintf(command, sizeof(command), "C:%d", (int)FOCUSER_BACKLASH_ITEM->number.value);
	if (prodigy_command(device, command, response, sizeof(response))) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_park_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_FOCUSER_PARK_ITEM->sw.value) {
		prodigy_command(device, "Z", response, sizeof(response));
		X_FOCUSER_PARK_ITEM->sw.value = false;
		X_FOCUSER_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
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
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
//		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
//	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
//		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
//		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
//		indigo_set_timer(device, 0, focuser_reverse_motion_handler, NULL);
//		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_PARK
	} else if (indigo_property_match_changeable(X_FOCUSER_PARK_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_PARK_PROPERTY, property, false);
		X_FOCUSER_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FOCUSER_PARK_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_park_handler, NULL);
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
	indigo_release_property(X_FOCUSER_PARK_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		INFO_PROPERTY->count = 4;
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Outlet #1", "Outlet #1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Outlet #2", "Outlet #2");
		indigo_init_text_item(AUX_USB_PORT_NAME_1_ITEM, AUX_USB_PORT_NAME_1_ITEM_NAME, "Port #1", "Port #1");
		indigo_init_text_item(AUX_USB_PORT_NAME_2_ITEM, AUX_USB_PORT_NAME_2_ITEM_NAME, "Port #2", "Port #2");
		// -------------------------------------------------------------------------------- POWER OUTLETS
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_POWER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Outlet #1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Outlet #2", true);
		// -------------------------------------------------------------------------------- USB PORTS
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_USB_PORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "Port #1", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "Port #2", true);
		// -------------------------------------------------------------------------------- X_AUX_REBOOT
		X_AUX_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_AUX_REBOOT", AUX_GROUP, "Reboot", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_REBOOT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_AUX_REBOOT_ITEM, "REBOOT", "Reboot", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyPRDG");
#endif
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_USB_PORT_PROPERTY, property))
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		if (indigo_property_match(X_AUX_REBOOT_PROPERTY, property))
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[64];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->count++ == 0) {
			if (PRIVATE_DATA->handle > 0) {
				if (prodigy_command(device, "#", response, sizeof(response)) && !strncmp(response, "OK_", 3)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%s OK", response + 3);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Focuser not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (prodigy_command(device, "D", response, sizeof(response)) && !strncmp(response, "D:",2)) {
				char *pnt, *token = strtok_r(response, ":", &pnt);
				token = strtok_r(NULL, ":", &pnt); // power #1
				if (token) {
					indigo_set_switch(AUX_POWER_OUTLET_PROPERTY, AUX_POWER_OUTLET_1_ITEM, *token == '1');
				}
				token = strtok_r(NULL, ":", &pnt); // power #2
				if (token) {
					indigo_set_switch(AUX_POWER_OUTLET_PROPERTY, AUX_POWER_OUTLET_2_ITEM, *token == '2');
				}
				token = strtok_r(NULL, ":", &pnt); // usb #1
				if (token) {
					indigo_set_switch(AUX_USB_PORT_PROPERTY, AUX_USB_PORT_1_ITEM, *token == '1');
				}
				token = strtok_r(NULL, ":", &pnt); // usb #2
				if (token) {
					indigo_set_switch(AUX_USB_PORT_PROPERTY, AUX_USB_PORT_2_ITEM, *token == '1');
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'D' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'D' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_REBOOT_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			if (PRIVATE_DATA->handle > 0) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_power_outlet_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	prodigy_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "X:1" : "X:0", response, sizeof(response));
	prodigy_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "Y:1" : "Y:0", response, sizeof(response));
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_usb_port_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	prodigy_command(device, AUX_USB_PORT_1_ITEM->sw.value ? "U:1" : "U:0", response, sizeof(response));
	prodigy_command(device, AUX_USB_PORT_2_ITEM->sw.value ? "J:1" : "J:0", response, sizeof(response));
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_reboot_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_AUX_REBOOT_ITEM->sw.value) {
		prodigy_command(device, "Q", NULL, 0);
		X_AUX_REBOOT_ITEM->sw.value = false;
		X_AUX_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_USB_PORT_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_1_ITEM->text.value);
		snprintf(AUX_USB_PORT_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_USB_PORT_NAME_2_ITEM->text.value);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_USB_PORT
		indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
		AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_usb_port_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_REBOOT
		indigo_property_copy_values(X_AUX_REBOOT_PROPERTY, property, false);
		X_AUX_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_AUX_REBOOT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_reboot_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(X_AUX_REBOOT_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_prodigy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static prodigy_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *aux = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Prodigy Focuser",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Prodigy Powerbox",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);


	SET_DRIVER_INFO(info, "PegasusAstro Prodigy Microfocuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(prodigy_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
			}
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
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
