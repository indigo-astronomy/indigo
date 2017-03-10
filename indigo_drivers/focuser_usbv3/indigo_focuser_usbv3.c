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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO StarlighXpress filter wheel driver
 \file indigo_ccd_sx.c
 */

#define DRIVER_VERSION 0x0001

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
#include "indigo_focuser_usbv3.h"

#define PRIVATE_DATA													((usbv3_private_data *)device->private_data)

typedef struct {
	int handle;
	pthread_mutex_t port_mutex;
	unsigned position;
	bool busy;
	indigo_timer *position_timer;
	indigo_timer *temperature_timer;
} usbv3_private_data;

static char *usbv3_command(indigo_device *device, char *format, ...) {
	static char response[128];
	char command[128];
	va_list args;
	va_start(args, format);
	vsnprintf(command, sizeof(command), format, args);
	va_end(args);
	
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	if (*format == '*') {
		indigo_write(PRIVATE_DATA->handle, command + 1, strlen(command) - 1);
		PRIVATE_DATA->busy = true;
		INDIGO_DEBUG_DRIVER(indigo_debug("usbv3: Command %s", command));
		INDIGO_DEBUG_DRIVER(indigo_debug("usbv3: Busy -> true"));
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return NULL;
	} else {
		int starCount = 1;
		indigo_write(PRIVATE_DATA->handle, command, strlen(command));
		int index = 0;
		int remains = sizeof(response);
		while (remains > 0) {
			long result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_ERROR(indigo_error("usbv3: Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno));
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return NULL;
			}
			if (c == '*') {
				if (PRIVATE_DATA->busy) {
					INDIGO_DEBUG_DRIVER(indigo_debug("usbv3: Busy -> false"));
					PRIVATE_DATA->busy = false;
					starCount = 2;
				}
				continue;
			}
			if (c == '\n')
				continue;
			if (c == '\r') {
				if (--starCount == 0)
					break;
				else
					continue;
			}
			response[index++] = c;
		}
		response[index] = 0;
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		INDIGO_DEBUG_DRIVER(indigo_debug("usbv3: Command %s -> %s", command, response != NULL ? response : "NULL"));
		return response;
	}
}


static bool usbv3_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle == 0) {
		INDIGO_ERROR(indigo_error("usbv3: failed to connect to %s", name));
		return false;
	}
	INDIGO_LOG(indigo_log("usbv3: connected to %s", name));
	usbv3_command(device, "FQUITx");
	usbv3_command(device, "FMANUA");
	usbv3_command(device, "FLX000");
	usbv3_command(device, "FZSIG0");
	unsigned position;
	sscanf(usbv3_command(device, "FPOSRO"), "P=%d", &position);
	FOCUSER_POSITION_ITEM->number.value = position;
	return true;
}

static void usbv3_close(indigo_device *device) {
	close(PRIVATE_DATA->handle);
	PRIVATE_DATA->handle = 0;
	INDIGO_LOG(indigo_log("usbv3: disconnected from %s", DEVICE_PORT_ITEM->text.value));
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_position_callback(indigo_device *device) {
	unsigned position;
	if (sscanf(usbv3_command(device, "FPOSRO"), "P=%d", &position) == 1)
		FOCUSER_POSITION_ITEM->number.value = position;
	if (PRIVATE_DATA->busy) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->position_timer);
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static void focuser_temperature_callback(indigo_device *device) {
	double temperature;
	if (sscanf(usbv3_command(device, "FTMPRO"), "T=%lf", &temperature) == 1)
		FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
	indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	indigo_reschedule_timer(device, 15, &PRIVATE_DATA->temperature_timer);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				strncpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name, INDIGO_VALUE_SIZE);
				break;
			}
		}
#endif
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.min = 2;
		FOCUSER_SPEED_ITEM->number.max = 9;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 1;
		FOCUSER_STEPS_ITEM->number.max = 65535;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (usbv3_open(device)) {
				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, focuser_temperature_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			usbv3_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		usbv3_command(device, "SMO00%d", (int)(FOCUSER_SPEED_ITEM->number.value));
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (PRIVATE_DATA->busy) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				usbv3_command(device, "*I%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
			} else {
				usbv3_command(device, "*O%05d", (int)(FOCUSER_STEPS_ITEM->number.value));
			}
			PRIVATE_DATA->position_timer = indigo_set_timer(device, 0.2, focuser_position_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (PRIVATE_DATA->busy) {
			if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
				usbv3_command(device, "FQUITx");
			}
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_focuser_detach(device);
}



indigo_result indigo_focuser_usbv3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbv3_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	
	static indigo_device focuser_template = {
		"USB_Focus v3", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		focuser_detach
	};
	
	SET_DRIVER_INFO(info, "USB_Focus v3 Focuser", __FUNCTION__, DRIVER_VERSION, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(usbv3_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(usbv3_private_data));
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
