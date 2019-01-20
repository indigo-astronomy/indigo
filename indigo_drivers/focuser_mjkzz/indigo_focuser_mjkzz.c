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

/** INDIGO MJKZZ focuser driver
 \file indigo_focuser_mjkzz.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_mjkzz"

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
#include "indigo_focuser_mjkzz.h"

#include "mjkzz_def.h"

#define PRIVATE_DATA													((mjkzz_private_data *)device->private_data)

typedef struct {
	int handle;
	pthread_mutex_t port_mutex;
	indigo_timer *timer;
} mjkzz_private_data;

static int32_t mjkzz_get_int(mjkzz_message *message) {
	return ((((((int32_t)message->ucMSG[0] << 8) + (int32_t)message->ucMSG[1]) << 8) + (int32_t)message->ucMSG[2]) << 8) + (int32_t)message->ucMSG[3];
}

static void mjkzz_set_int(mjkzz_message *message, int32_t value) {
	message->ucMSG[0] = (uint8_t)(value >> 24);
	message->ucMSG[1] = (uint8_t)(value >> 16);
	message->ucMSG[2] = (uint8_t)(value >> 8);
	message->ucMSG[3] = (uint8_t)value;
}

static bool mjkzz_command(indigo_device *device, mjkzz_message *message) {
	message->ucSUM = message->ucADD + message->ucCMD + message->ucIDX + message->ucMSG[0] + message->ucMSG[1] + message->ucMSG[2] + message->ucMSG[3];
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %02x %02x %02x [%02x %02x %02x %02x] %02x (CMD = '%c' VAL = %d)", message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM, message->ucCMD, mjkzz_get_int(message));
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	indigo_write(PRIVATE_DATA->handle, (const char *)message, 8);
	if (indigo_read(PRIVATE_DATA->handle, (char *)message, 8) != 8) {
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command failed");
		return false;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %02x %02x %02x [%02x %02x %02x %02x] %02x (VAL = %d)", message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM, mjkzz_get_int(message));
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void timer_callback(indigo_device *device) {
	mjkzz_message message = { 0x01, CMD_GPOS };
	if (mjkzz_command(device, &message)) {
		int32_t position = mjkzz_get_int(&message);
		FOCUSER_POSITION_ITEM->number.value = position;
		if (FOCUSER_POSITION_ITEM->number.target == position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->timer = NULL;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_reschedule_timer(device, 0.1, &PRIVATE_DATA->timer);
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
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
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 3;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 1000;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_POSITION_ITEM->number.min = -32768;
		FOCUSER_POSITION_ITEM->number.max = 32767;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	mjkzz_message message = { 0x01 };
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->handle > 0) {
				message.ucCMD = CMD_GVER;
				if (mjkzz_command(device, &message)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "MJKZZ detected");
					message.ucCMD = CMD_SREG;
					message.ucIDX = reg_HPWR;
					mjkzz_set_int(&message, 16);
					mjkzz_command(device, &message);
					message.ucCMD = CMD_SREG;
					message.ucIDX = reg_LPWR;
					mjkzz_set_int(&message, 2);
					mjkzz_command(device, &message);
					message.ucCMD = CMD_SREG;
					message.ucIDX = reg_MSTEP;
					mjkzz_set_int(&message, 0);
					mjkzz_command(device, &message);
					message.ucCMD = CMD_GPOS;
					if (mjkzz_command(device, &message)) {
						FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = mjkzz_get_int(&message);
					}
					message.ucCMD = CMD_GSPD;
					if (mjkzz_command(device, &message)) {
						int speed = mjkzz_get_int(&message);
						if (speed > FOCUSER_SPEED_ITEM->number.max)
							speed = (int)FOCUSER_SPEED_ITEM->number.max;
						FOCUSER_SPEED_ITEM->number.target = FOCUSER_SPEED_ITEM->number.value = speed;
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "MJKZZ not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
			if (PRIVATE_DATA->handle > 0) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIVATE_DATA->handle > 0) {
				indigo_cancel_timer(device, &PRIVATE_DATA->timer);
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
			message.ucCMD = CMD_SSPD;
			mjkzz_set_int(&message, FOCUSER_SPEED_ITEM->number.value);
			if (mjkzz_command(device, &message)) {
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
		if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value)
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value + FOCUSER_STEPS_ITEM->number.value;
		else
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value - FOCUSER_STEPS_ITEM->number.value;
		message.ucCMD = CMD_SPOS;
		mjkzz_set_int(&message, FOCUSER_POSITION_ITEM->number.target);
		if (mjkzz_command(device, &message)) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->timer == NULL)
				PRIVATE_DATA->timer = indigo_set_timer(device, 0.0, timer_callback);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		message.ucCMD = CMD_SPOS;
		mjkzz_set_int(&message, FOCUSER_POSITION_ITEM->number.target);
		if (mjkzz_command(device, &message)) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->timer == NULL)
				PRIVATE_DATA->timer = indigo_set_timer(device, 0.0, timer_callback);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			message.ucCMD = CMD_STOP;
			if (mjkzz_command(device, &message)) {
				FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = mjkzz_get_int(&message);
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

indigo_result indigo_focuser_mjkzz(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static mjkzz_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_MJKZZ_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, FOCUSER_MJKZZ_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(mjkzz_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(mjkzz_private_data));
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
