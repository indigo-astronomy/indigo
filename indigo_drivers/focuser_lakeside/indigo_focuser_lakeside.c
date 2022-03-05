// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
// Lakeside focuser command set is extracted from INDI driver written
// by Phil Shepherd with help of Peter Chance.
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

/** INDIGO LakesideASTRO focuser driver
 \file indigo_focuser_lakeside.c
 */


#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_focuser_lakeside"

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

#include "indigo_focuser_lakeside.h"

#define PRIVATE_DATA													((lakeside_private_data *)device->private_data)

#define X_FOCUSER_ACTIVE_SLOPE_PROPERTY				(PRIVATE_DATA->active_slope_property)
#define X_FOCUSER_ACTIVE_SLOPE_1_ITEM					(X_FOCUSER_ACTIVE_SLOPE_PROPERTY->items+0)
#define X_FOCUSER_ACTIVE_SLOPE_2_ITEM					(X_FOCUSER_ACTIVE_SLOPE_PROPERTY->items+1)

#define X_FOCUSER_DEADBAND_ITEM								(FOCUSER_COMPENSATION_PROPERTY->items+1)
#define X_FOCUSER_PERIOD_ITEM									(FOCUSER_COMPENSATION_PROPERTY->items+2)

typedef struct {
	int handle;
	indigo_timer *timer;
	indigo_property *active_slope_property;
	pthread_mutex_t mutex;
} lakeside_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool lakeside_command(indigo_device *device, char *command, char *response, int timeout) {
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
			tv.tv_sec = timeout / 1000000;
			tv.tv_usec = timeout % 1000000;
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
		X_FOCUSER_ACTIVE_SLOPE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_ACTIVE_SLOPE", FOCUSER_MAIN_GROUP, "Active slope", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_ACTIVE_SLOPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_ACTIVE_SLOPE_1_ITEM, "1", "Slope #1", true);
		indigo_init_switch_item(X_FOCUSER_ACTIVE_SLOPE_2_ITEM, "2", "Slope #2", false);
		FOCUSER_COMPENSATION_PROPERTY = indigo_resize_property(FOCUSER_COMPENSATION_PROPERTY, 3);
		indigo_init_number_item(X_FOCUSER_DEADBAND_ITEM, "DEADBAND", "Deadband", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(X_FOCUSER_PERIOD_ITEM, "PERIOD", "Period", 0, 0xFFFF, 1, 1);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Lakeside Focuser");
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 0xFFFF;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_ITEM->number.min = -128;
		FOCUSER_COMPENSATION_ITEM->number.min = -127;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
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
		if (indigo_property_match(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	if (!IS_CONNECTED)
		return;
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		while (lakeside_command(device, NULL, response, 10000)) {
			if (!strcmp(response, "DONE")) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				break;
			} else if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
				FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
				if (lakeside_command(device, "CH#", NULL, 0)) {
					FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
					indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				} else {
					FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
				break;
			} else if (response[0] == 'P') {
				FOCUSER_POSITION_ITEM->number.value = atol(response + 1);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
	} else {
		if (lakeside_command(device, "?T#", response, 1000000) && *response == 'T') {
			FOCUSER_TEMPERATURE_ITEM->number.value = atol(response + 1) / 2.0;
			if (FOCUSER_TEMPERATURE_ITEM->number.target != FOCUSER_TEMPERATURE_ITEM->number.value) {
				FOCUSER_TEMPERATURE_ITEM->number.target = FOCUSER_TEMPERATURE_ITEM->number.value;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		} else if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_ALERT_STATE) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600);
		if (PRIVATE_DATA->handle > 0) {
			if (lakeside_command(device, "??#", response, 1000000) && !strcmp("OK", response)) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Lakeside focuser detected");
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Lakeside focuser not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			lakeside_command(device, "CTF#", NULL, 0);
			lakeside_command(device, "CRg1#", response, 1000000);
			if (lakeside_command(device, "?P#", response, 1000000) && *response == 'P') {
				FOCUSER_POSITION_ITEM->number.value = atol(response + 1);
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?B#", response, 1000000) && *response == 'B') {
				FOCUSER_BACKLASH_ITEM->number.target = FOCUSER_BACKLASH_ITEM->number.value = atol(response + 1);
			} else {
				FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?D#", response, 1000000) && *response == 'D') {
				if (atol(response + 1)) {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, FOCUSER_REVERSE_MOTION_DISABLED_ITEM, true);
				} else {
					indigo_set_switch(FOCUSER_REVERSE_MOTION_PROPERTY, FOCUSER_REVERSE_MOTION_ENABLED_ITEM, true);
				}
			} else {
				FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?T#", response, 1000000) && *response == 'T') {
				FOCUSER_TEMPERATURE_ITEM->number.target = FOCUSER_TEMPERATURE_ITEM->number.value = atol(response + 1) / 2.0;
			} else {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?1#", response, 100000) && *response == '1') {
				FOCUSER_COMPENSATION_ITEM->number.target =FOCUSER_COMPENSATION_ITEM->number.value = atol(response + 1);
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?a#", response, 100000) && *response == 'a') {
				if (atol(response + 1)) {
					FOCUSER_COMPENSATION_ITEM->number.target =FOCUSER_COMPENSATION_ITEM->number.value = -FOCUSER_COMPENSATION_ITEM->number.value;
				}
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?c#", response, 100000) && *response == 'c') {
				X_FOCUSER_DEADBAND_ITEM->number.target =X_FOCUSER_DEADBAND_ITEM->number.value = atol(response + 1);
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?e#", response, 100000) && *response == 'e') {
				X_FOCUSER_PERIOD_ITEM->number.target =X_FOCUSER_PERIOD_ITEM->number.value = atol(response + 1);
			} else {
				FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			indigo_delete_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
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
	char command[16];
	if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
		sprintf(command, "CI%d#", (unsigned)FOCUSER_STEPS_ITEM->number.value);
	} else {
		sprintf(command, "CO%d#", (unsigned)FOCUSER_STEPS_ITEM->number.value);
	}
	if (lakeside_command(device, command, NULL, 0)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_mode_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (lakeside_command(device, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? "CTN#" : "CTF#", NULL, 0)) {
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_backlash_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	if (IS_CONNECTED) {
		sprintf(command, "CRB%d#", (int)FOCUSER_BACKLASH_ITEM->number.value);
		if (lakeside_command(device, command, response, 100000) && !strcmp(response, "OK")) {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_compensation_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16], response[16];
	if (IS_CONNECTED) {
		bool result;
		if (X_FOCUSER_ACTIVE_SLOPE_1_ITEM->sw.value) {
			sprintf(command, "CR1%d#", (int)fabs(FOCUSER_COMPENSATION_ITEM->number.value));
			result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			if (result) {
				sprintf(command, "CRa%d#", FOCUSER_COMPENSATION_ITEM->number.value > 0 ? 0 : 1);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
			if (result) {
				sprintf(command, "CRc%d#", (int)X_FOCUSER_DEADBAND_ITEM->number.value);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
			if (result) {
				sprintf(command, "CRe%d#", (int)X_FOCUSER_PERIOD_ITEM->number.value);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
		} else {
			sprintf(command, "CR2%d#", (int)fabs(FOCUSER_COMPENSATION_ITEM->number.value));
			result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			if (result) {
				sprintf(command, "CRb%d#", FOCUSER_COMPENSATION_ITEM->number.value > 0 ? 0 : 1);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
			if (result) {
				sprintf(command, "CRd%d#", (int)X_FOCUSER_DEADBAND_ITEM->number.value);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
			if (result) {
				sprintf(command, "CRf%d#", (int)X_FOCUSER_PERIOD_ITEM->number.value);
				result = lakeside_command(device, command, response, 100000) && !strcmp(response, "OK");
			}
		}
		if (result) {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_active_slope_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[16];
	if (X_FOCUSER_ACTIVE_SLOPE_1_ITEM->sw.value) {
		if (lakeside_command(device, "CRg1#", response, 100000) && !strcmp(response, "OK")) {
			X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
			if (lakeside_command(device, "?1#", response, 100000) && *response == '1') {
				FOCUSER_COMPENSATION_ITEM->number.target =FOCUSER_COMPENSATION_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?a#", response, 100000) && *response == 'a') {
				if (atol(response + 1)) {
					FOCUSER_COMPENSATION_ITEM->number.target = FOCUSER_COMPENSATION_ITEM->number.value = -FOCUSER_COMPENSATION_ITEM->number.value;
				}
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?c#", response, 100000) && *response == 'c') {
				X_FOCUSER_DEADBAND_ITEM->number.target =X_FOCUSER_DEADBAND_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?e#", response, 100000) && *response == 'e') {
				X_FOCUSER_PERIOD_ITEM->number.target =X_FOCUSER_PERIOD_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	} else {
		if (lakeside_command(device, "CRg2#", response, 100000) && !strcmp(response, "OK")) {
			X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
			if (lakeside_command(device, "?2#", response, 100000) && *response == '2') {
				FOCUSER_COMPENSATION_ITEM->number.target =FOCUSER_COMPENSATION_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?b#", response, 100000) && *response == 'b') {
				if (atol(response + 1)) {
					FOCUSER_COMPENSATION_ITEM->number.target =FOCUSER_COMPENSATION_ITEM->number.value = -FOCUSER_COMPENSATION_ITEM->number.value;
				}
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?d#", response, 100000) && *response == 'd') {
				X_FOCUSER_DEADBAND_ITEM->number.target =X_FOCUSER_DEADBAND_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (lakeside_command(device, "?f#", response, 100000) && *response == 'f') {
				X_FOCUSER_PERIOD_ITEM->number.target =X_FOCUSER_PERIOD_ITEM->number.value = atol(response + 1);
			} else {
				X_FOCUSER_ACTIVE_SLOPE_PROPERTY->state = FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
	indigo_update_property(device, X_FOCUSER_ACTIVE_SLOPE_PROPERTY, NULL);
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
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_mode_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_backlash_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_compensation_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_ACTIVE_SLOPE
	} else if (indigo_property_match(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_ACTIVE_SLOPE_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_active_slope_handler, NULL);
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
	indigo_release_property(X_FOCUSER_ACTIVE_SLOPE_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_lakeside(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lakeside_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"LakesideAstro Focuser",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "LakesideAstro Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(lakeside_private_data));
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
