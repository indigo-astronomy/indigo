// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO MoonLite focuser driver
 \file indigo_ccd_sx.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_moonlite"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_focuser_moonlite.h"

#define PRIVATE_DATA													((moonlite_private_data *)device->private_data)

#define X_FOCUSER_STEPPING_MODE_PROPERTY			(PRIVATE_DATA->stepping_mode_property)
#define X_FOCUSER_STEPPING_MODE_HALF_ITEM			(X_FOCUSER_STEPPING_MODE_PROPERTY->items+0)
#define X_FOCUSER_STEPPING_MODE_FULL_ITEM			(X_FOCUSER_STEPPING_MODE_PROPERTY->items+1)


typedef struct {
	int handle;
	pthread_mutex_t port_mutex;
	indigo_timer *timer;
	indigo_property *stepping_mode_property;
} moonlite_private_data;

static bool moonlite_command(indigo_device *device, char *command, char *response, int max) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (response != NULL) {
		int index = 0;
		while (index < max) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			if (c < 0)
				c = ':';
			if (c == '#')
				break;
			response[index++] = c;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command '%s' -> '%s'", command, response != NULL ? response : "NULL");
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void timer_callback(indigo_device *device) {
	static bool read_temperature = false;
	char response[16];
	if (read_temperature) {
		if (moonlite_command(device, ":GT#", response, sizeof(response))) {
			double temp = ((int)strtol(response, NULL, 16)) / 2.0;
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		}
		read_temperature = false;
	} else {
		moonlite_command(device, ":C#", NULL, 0);
		read_temperature = true;
	}
	bool update = false;
	if (moonlite_command(device, ":GP#", response, sizeof(response))) {
		long pos = strtol(response, NULL, 16);
		if (FOCUSER_POSITION_ITEM->number.value != pos) {
			FOCUSER_POSITION_ITEM->number.value = pos;
			update = true;
		}
	}
	if (moonlite_command(device, ":GI#", response, sizeof(response))) {
		if (strcmp(response, "00") == 0) {
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
	indigo_reschedule_timer(device, FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE ? 0.2 : 1.0, &PRIVATE_DATA->timer);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		X_FOCUSER_STEPPING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_STEPPING_MODE", FOCUSER_MAIN_GROUP, "Stepping mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEPPING_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_HALF_ITEM, "HALF", "Half", false);
		indigo_init_switch_item(X_FOCUSER_STEPPING_MODE_FULL_ITEM, "FULL", "Full", true);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "MoonLite Focuser");
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		FOCUSER_SPEED_ITEM->number.max = 5;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 0xFFFF;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 0xFFFF;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_ITEM->number.min = -128;
		FOCUSER_COMPENSATION_ITEM->number.min = -127;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_STEPPING_MODE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[16], response[64];
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600);
			if (PRIVATE_DATA->handle > 0) {
				for (int i = 0; true; i++) {
					if (moonlite_command(device, ":GV#", response, sizeof(response)) && strlen(response) == 2) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "MoonLite focuser %c.%c", response[0], response[1]);
						break;
					} else if (i < 5) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "No reply from MoonLite focuser - retrying");
						sleep(2);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "MoonLite focuser not detected");
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
						break;
					}
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				moonlite_command(device, ":C#", NULL, 0);
				moonlite_command(device, ":FQ#", NULL, 0);
				moonlite_command(device, ":SF#", NULL, 0);
				moonlite_command(device, ":-#", NULL, 0);
				moonlite_command(device, ":SD02#", NULL, 0);
				usleep(750000);
				if (moonlite_command(device, ":GT#", response, sizeof(response))) {
					FOCUSER_TEMPERATURE_ITEM->number.value = ((int)strtol(response, NULL, 16)) / 2.0;
				}
				if (moonlite_command(device, ":GP#", response, sizeof(response))) {
					FOCUSER_POSITION_ITEM->number.value = strtol(response, NULL, 16);
				}
				if (moonlite_command(device, ":GC#", response, sizeof(response))) {
					FOCUSER_COMPENSATION_ITEM->number.value = (char)strtol(response, NULL, 16);
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				indigo_define_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
				PRIVATE_DATA->timer = indigo_set_timer(device, 0, timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIVATE_DATA->handle > 0) {
				indigo_cancel_timer(device, &PRIVATE_DATA->timer);
				indigo_delete_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
			snprintf(command, sizeof(command), ":SD%02X#", 0x01 << (int)FOCUSER_SPEED_ITEM->number.value);
			if (moonlite_command(device, command, NULL, 0)) {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		int position = FOCUSER_POSITION_ITEM->number.value + (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? 1 : -1);
		if (position < 0)
			position = 0;
		if (position > 0xFFFF)
			position = 0xFFFF;
		snprintf(command, sizeof(command), ":SN%04X#:FG#", position);
		if (moonlite_command(device, command, NULL, 0)) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		snprintf(command, sizeof(command), ":SN%04X#:FG#", (int)FOCUSER_POSITION_ITEM->number.value);
		if (moonlite_command(device, command, NULL, 0)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			if (moonlite_command(device, ":FQ#", NULL, 0)) {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (moonlite_command(device, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? ":+#" : ":-#", NULL, 0)) {
			FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		sprintf(command, ":SC%02X#", ((char)FOCUSER_COMPENSATION_ITEM->number.value) & 0xFF);
		if (moonlite_command(device, command, NULL, 0)) {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		// -------------------------------------------------------------------------------- X_FOCUSER_STEPPING_MODE
	} else if (indigo_property_match(X_FOCUSER_STEPPING_MODE_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_STEPPING_MODE_PROPERTY, property, false);
		if (moonlite_command(device, X_FOCUSER_STEPPING_MODE_FULL_ITEM->sw.value ? ":SF#" : ":SH#", NULL, 0)) {
			X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			X_FOCUSER_STEPPING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_FOCUSER_STEPPING_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(X_FOCUSER_STEPPING_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

indigo_result indigo_focuser_moonlite(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static moonlite_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"MoonLite",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "MoonLite Focuser", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(moonlite_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(moonlite_private_data));
			focuser = malloc(sizeof(indigo_device));
			assert(focuser != NULL);
			memcpy(focuser, &focuser_template, sizeof(indigo_device));
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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
