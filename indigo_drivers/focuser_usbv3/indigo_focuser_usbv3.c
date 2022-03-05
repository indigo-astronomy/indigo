// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO USBV3 focuser driver
 \file indigo_focuser_usbv3.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_focuser_usbv3"

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

#include "indigo_focuser_usbv3.h"

#define PRIVATE_DATA													((usbv3_private_data *)device->private_data)

#define X_FOCUSER_STEP_SIZE_PROPERTY					(PRIVATE_DATA->step_size_property)
#define X_FOCUSER_FULL_STEP_ITEM							(X_FOCUSER_STEP_SIZE_PROPERTY->items+0)
#define X_FOCUSER_HALF_STEP_ITEM							(X_FOCUSER_STEP_SIZE_PROPERTY->items+1)


typedef struct {
	int handle;
	pthread_mutex_t port_mutex;
	indigo_timer *position_timer;
	indigo_timer *temperature_timer;
	pthread_t thread;
	indigo_property *step_size_property;
} usbv3_private_data;

static void usbv3_command(indigo_device *device, char *format, ...) {
	char command[128];
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	va_list args;
	va_start(args, format);
	vsnprintf(command, sizeof(command), format, args);
	va_end(args);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s", command);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
}

static char *usbv3_response(indigo_device *device) {
	static char response[128];
	char c;
	int index = 0;
	while (index < sizeof(response)) {
		long result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return "";
		}
		if (c == '\n')
			continue;
		if (c == '\r') {
			break;
		}
		response[index++] = c;
	}
	response[index] = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Response %s", response != NULL ? response : "");
	return response;
}

static char *usbv3_reader(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "usbv3_reader started");
	while (PRIVATE_DATA->handle > 0) {
		char *response = usbv3_response(device);
		if (*response == '*') {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (*response == 'P') {
			unsigned position;
			if (sscanf(response, "P=%d", &position) == 1) {
				FOCUSER_POSITION_ITEM->number.value = position;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else if (*response == 'T') {
			double temperature;
			if (sscanf(response, "T=%lf", &temperature) == 1) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ignored response: %s", response);
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "usbv3_reader finished");
	return NULL;
}

static void usbv3_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static bool usbv3_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s (%s)", name, strerror(errno));
		return false;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
	usbv3_command(device, "SGETAL");
	if (*usbv3_response(device) != 'C') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "invalid response");
		usbv3_close(device);
		return false;
	}
	usbv3_command(device, "FMANUA");
	usbv3_response(device);
	usbv3_command(device, "M65535");
	usbv3_response(device);
	usbv3_command(device, "SMROTH");
	usbv3_command(device, "SMSTPF");
	usbv3_command(device, "FTMPRO");
	sscanf(usbv3_response(device), "T=%lf", &FOCUSER_TEMPERATURE_ITEM->number.value);
	usbv3_command(device, "FPOSRO");
	unsigned position;
	if (sscanf(usbv3_response(device), "P=%d", &position) == 1) {
		FOCUSER_POSITION_ITEM->number.value = position;
	}
	//	unsigned sign, compensation;
	//	usbv3_command(device, "FREADA");
	//	if (sscanf(usbv3_response(device), "A=%d", &compensation) == 1) {
	//		usbv3_command(device, "FTxxxA");
	//		if (sscanf(usbv3_response(device), "A=%d", &sign) == 1) {
	//			if (sign == 0)
	//				compensation = -compensation;
	//			FOCUSER_COMPENSATION_ITEM->number.value = compensation;
	//		}
	//	}
	indigo_async((void * (*)(void*))usbv3_reader, device);
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation


static void focuser_position_callback(indigo_device *device) {
	usbv3_command(device, "FPOSRO");
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
}

static void focuser_temperature_callback(indigo_device *device) {
	usbv3_command(device, "FTMPRO");
	indigo_reschedule_timer(device, 15, &PRIVATE_DATA->temperature_timer);
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_FOCUSER_STEP_SIZE
		X_FOCUSER_STEP_SIZE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_STEP_SIZE", FOCUSER_MAIN_GROUP, "Step size", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEP_SIZE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_FULL_STEP_ITEM, "FULL_STEP", "Full step", true);
		indigo_init_switch_item(X_FOCUSER_HALF_STEP_ITEM, "HALF_STEP", "Half step", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = 4;
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 8;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 65535;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -999;
		FOCUSER_COMPENSATION_ITEM->number.max = 999;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_STEP_SIZE_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (usbv3_open(device)) {
			indigo_define_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_temperature_callback, &PRIVATE_DATA->temperature_timer);
			usbv3_command(device, "SMO00%d", (int)(FOCUSER_SPEED_ITEM->number.value));
			usbv3_command(device, "FLX%03d", abs((int)FOCUSER_COMPENSATION_ITEM->number.value));
			usbv3_command(device, "FZSIG%d", FOCUSER_COMPENSATION_ITEM->number.value < 0 ? 0 : 1);
			if (X_FOCUSER_FULL_STEP_ITEM->sw.value) {
				usbv3_command(device, "SMSTPF");
			} else {
				usbv3_command(device, "SMSTPD");
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
		usbv3_command(device, "FQUITx");
		indigo_delete_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
		usbv3_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
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
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_FOCUSER_STEP_SIZE_PROPERTY);
		}
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
			usbv3_command(device, "SMO00%d", 10 - (int)(FOCUSER_SPEED_ITEM->number.value));
			FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				if (FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)
					usbv3_command(device, "I%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
				else
					usbv3_command(device, "O%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
			} else {
				if (FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)
					usbv3_command(device, "O%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
				else
					usbv3_command(device, "I%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
			}
			indigo_set_timer(device, 0.2, focuser_position_callback, &PRIVATE_DATA->position_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
			if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
				usbv3_command(device, "FQUITx");
			}
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
			if (IS_CONNECTED) {
				usbv3_command(device, "FLX%03d", abs((int)FOCUSER_COMPENSATION_ITEM->number.value));
				usbv3_command(device, "FZSIG%d", FOCUSER_COMPENSATION_ITEM->number.value < 0 ? 0 : 1);
			}
			FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			usbv3_command(device, "FAUTOM");
		} else {
			usbv3_command(device, "FMANUA");
			indigo_set_timer(device, 0, focuser_temperature_callback, &PRIVATE_DATA->temperature_timer);
		}
		// -------------------------------------------------------------------------------- X_FOCUSER_STEP_SIZE
	} else if (indigo_property_match(X_FOCUSER_STEP_SIZE_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_STEP_SIZE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			if (X_FOCUSER_FULL_STEP_ITEM->sw.value) {
				usbv3_command(device, "SMSTPF");
			} else {
				usbv3_command(device, "SMSTPD");
			}
		}
		X_FOCUSER_STEP_SIZE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_FOCUSER_STEP_SIZE_PROPERTY, NULL);
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(X_FOCUSER_STEP_SIZE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

indigo_result indigo_focuser_usbv3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbv3_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"USB_Focus v3",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "USB_Focus v3 Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(usbv3_private_data));
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
