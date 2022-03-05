// Copyright (c) 2020 CloudMakers, s. r. o.
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

/** INDIGO RoboFocus focuser driver
 \file indigo_focuser_robofocus.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_robofocus"

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

#include "indigo_focuser_robofocus.h"

#define PRIVATE_DATA													((robofocus_private_data *)device->private_data)

#define X_FOCUSER_POWER_CHANNELS_PROPERTY			(PRIVATE_DATA->power_channels_property)
#define X_FOCUSER_POWER_CHANNEL_1_ITEM				(X_FOCUSER_POWER_CHANNELS_PROPERTY->items+0)
#define X_FOCUSER_POWER_CHANNEL_2_ITEM				(X_FOCUSER_POWER_CHANNELS_PROPERTY->items+1)
#define X_FOCUSER_POWER_CHANNEL_3_ITEM				(X_FOCUSER_POWER_CHANNELS_PROPERTY->items+2)
#define X_FOCUSER_POWER_CHANNEL_4_ITEM				(X_FOCUSER_POWER_CHANNELS_PROPERTY->items+3)

#define X_FOCUSER_CONFIG_PROPERTY							(PRIVATE_DATA->config_property)
#define X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM			(X_FOCUSER_CONFIG_PROPERTY->items+0)
#define X_FOCUSER_CONFIG_STEP_DELAY_ITEM			(X_FOCUSER_CONFIG_PROPERTY->items+1)
#define X_FOCUSER_CONFIG_STEP_SIZE_ITEM				(X_FOCUSER_CONFIG_PROPERTY->items+2)
#define X_FOCUSER_CONFIG_BACKLASH_ITEM				(X_FOCUSER_CONFIG_PROPERTY->items+3)

typedef struct {
	int handle;
	indigo_timer *timer;
	indigo_property *power_channels_property;
	indigo_property *config_property;
	pthread_mutex_t mutex;
} robofocus_private_data;

static bool robofocus_command(indigo_device *device, char *command, char *response) {
	char c;
	struct timeval tv;
	unsigned sum = 0, i_count = 0, o_count = 0;
	char buffer[9];
	for (int i = 0; i < 8; i++)
		sum += buffer[i] = command[i];
	buffer[8] = sum & 0xFF;
	indigo_write(PRIVATE_DATA->handle, buffer, 9);
	bool done = false;
	while (!done) {
		fd_set readout;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result <= 0)
			return false;
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result <= 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
			return false;
		}
		switch (c) {
			case 'I':
				i_count++;
				continue;
			case 'O':
				o_count++;
				continue;
			case 'F':
				response[0] = c;
				result = read(PRIVATE_DATA->handle, response + 1, 8);
				if (result < 8) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				done = true;
				break;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command '%c%c %02x %02x %02x %02x %02x %02x %02x' -> '%c%c %02x %02x %02x %02x %02x %02x %02x'", buffer[0], buffer[1], buffer[2] & 0xFF, buffer[3] & 0xFF, buffer[4] & 0xFF, buffer[5] & 0xFF, buffer[6] & 0xFF, buffer[7] & 0xFF, buffer[8] & 0xFF, response[0], response[1], response[2] & 0xFF, response[3] & 0xFF, response[4] & 0xFF, response[5] & 0xFF, response[6] & 0xFF, response[7] & 0xFF, response[8] & 0xFF);
	response[8] = 0; // TODO checksum
	return true;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_FOCUSER_POWER_CHANNELS
		X_FOCUSER_POWER_CHANNELS_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_POWER_CHANNELS", FOCUSER_MAIN_GROUP, "Power channels", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (X_FOCUSER_POWER_CHANNELS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_1_ITEM, "1", "Channel #1", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_2_ITEM, "2", "Channel #2", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_3_ITEM, "3", "Channel #3", false);
		indigo_init_switch_item(X_FOCUSER_POWER_CHANNEL_4_ITEM, "4", "Channel #4", false);
		// -------------------------------------------------------------------------------- X_FOCUSER_POWER_CHANNELS
		X_FOCUSER_CONFIG_PROPERTY = indigo_init_number_property(NULL, device->name, "X_FOCUSER_CONFIG", FOCUSER_MAIN_GROUP, "Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (X_FOCUSER_CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM, "DUTY_CYCLE", "Duty cycle", 0, 250, 1, 0);
		indigo_init_number_item(X_FOCUSER_CONFIG_STEP_DELAY_ITEM, "STEP_DELAY", "Step delay", 1, 64, 1, 1);
		indigo_init_number_item(X_FOCUSER_CONFIG_STEP_SIZE_ITEM, "STEP_SIZE", "Step size", 1, 64, 1, 1);
		indigo_init_number_item(X_FOCUSER_CONFIG_BACKLASH_ITEM, "BACKLASH", "Backlash", -0xFFFF, 0xFFFF, 1, 20);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "RoboFocus Focuser");
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 0xFFFF;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 1;
		FOCUSER_POSITION_ITEM->number.max = 0xFFFF;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 1;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0xFFFF;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 0xFFFF;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 0xFFFF;
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
		if (indigo_property_match(X_FOCUSER_POWER_CHANNELS_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
		if (indigo_property_match(X_FOCUSER_CONFIG_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[9];
	if (robofocus_command(device, "FT000000", response)) {
		double temp = round(atol(response + 4) - 546.30) / 2;
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 5.0, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char response[9];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 9600);
		if (PRIVATE_DATA->handle > 0) {
			for (int i = 0; true; i++) {
				if (robofocus_command(device, "FV000000", response) && !strncmp(response, "FV", 2)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "RoboFocus focuser %s", response + 2);
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, response + 2);
					break;
				} else if (i < 5) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "No reply from RoboFocus focuser - retrying");
					indigo_usleep(2 * ONE_SECOND_DELAY);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "RoboFocus focuser not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
					break;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (robofocus_command(device, "FG000000", response) && !strncmp(response, "FD", 2)) {
				FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = atol(response + 2);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (robofocus_command(device, "FP000000", response) && !strncmp(response, "FP", 2)) {
				X_FOCUSER_POWER_CHANNEL_1_ITEM->sw.value = response[4] == '2';
				X_FOCUSER_POWER_CHANNEL_2_ITEM->sw.value = response[5] == '2';
				X_FOCUSER_POWER_CHANNEL_3_ITEM->sw.value = response[6] == '2';
				X_FOCUSER_POWER_CHANNEL_4_ITEM->sw.value = response[7] == '2';
				X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (robofocus_command(device, "FC000000", response) && !strncmp(response, "FC", 2)) {
				X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.target = X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.value = response[5];
				X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.target = X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.value = response[6];
				X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.target = X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.value = response[7];
				if (robofocus_command(device, "FB000000", response) && !strncmp(response, "FB", 2)) {
					X_FOCUSER_CONFIG_BACKLASH_ITEM->number.target = X_FOCUSER_CONFIG_BACKLASH_ITEM->number.value = (response[2] == '3' ? -1 : 1) * atol(response + 3);
					X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else {
				X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
			}

			indigo_define_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
			indigo_update_property(device, INFO_PROPERTY, NULL);
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
			indigo_delete_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
			indigo_delete_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
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
	char command[9], response[9];
	int position = FOCUSER_POSITION_ITEM->number.value + (int)FOCUSER_STEPS_ITEM->number.value * (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1) * (FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -1 : 1);
	if (position < 1)
		position = 1;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	if (position > 0xFFFF)
		position = 0xFFFF;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	snprintf(command, sizeof(command), "FG%06d", position);
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	if (robofocus_command(device, command, response)) {
		FOCUSER_POSITION_ITEM->number.value = atol(response + 3);
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE)
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[9], response[9];
	int position = (int)FOCUSER_POSITION_ITEM->number.value;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
	snprintf(command, sizeof(command), "FG%06d", position);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	if (robofocus_command(device, command, response)) {
		FOCUSER_POSITION_ITEM->number.value = atol(response + 3);
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE)
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_write(PRIVATE_DATA->handle, "\r", 1);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			}
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_power_channels_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[9], response[9];
	sprintf(command, "FP00%c%c%c%c", X_FOCUSER_POWER_CHANNEL_1_ITEM->sw.value ? '2' : '1', X_FOCUSER_POWER_CHANNEL_2_ITEM->sw.value ? '2' : '1', X_FOCUSER_POWER_CHANNEL_3_ITEM->sw.value ? '2' : '1', X_FOCUSER_POWER_CHANNEL_4_ITEM->sw.value ? '2' : '1');
	if (robofocus_command(device, command, response) && !strncmp(response, "FP", 2)) {
		X_FOCUSER_POWER_CHANNEL_1_ITEM->sw.value = response[4] == '2';
		X_FOCUSER_POWER_CHANNEL_2_ITEM->sw.value = response[5] == '2';
		X_FOCUSER_POWER_CHANNEL_3_ITEM->sw.value = response[6] == '2';
		X_FOCUSER_POWER_CHANNEL_4_ITEM->sw.value = response[7] == '2';
		X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_FOCUSER_POWER_CHANNELS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_POWER_CHANNELS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_config_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[9], response[9];
	sprintf(command, "FC000%c%c%c", (int)X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.target, (int)X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.target, (int)X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.target);
	if (robofocus_command(device, command, response) && !strncmp(response, "FC", 2)) {
		X_FOCUSER_CONFIG_DUTY_CYCLE_ITEM->number.value = response[5];
		X_FOCUSER_CONFIG_STEP_DELAY_ITEM->number.value = response[6];
		X_FOCUSER_CONFIG_STEP_SIZE_ITEM->number.value = response[7];
		sprintf(command, "FB%c%05d", X_FOCUSER_CONFIG_BACKLASH_ITEM->number.value < 0 ? '3' : '2', (int)fabs(X_FOCUSER_CONFIG_BACKLASH_ITEM->number.target));
		if (robofocus_command(device, command, response) && !strncmp(response, "FB", 2)) {
			X_FOCUSER_CONFIG_BACKLASH_ITEM->number.target = X_FOCUSER_CONFIG_BACKLASH_ITEM->number.value = (response[2] == '3' ? -1 : 1) * atol(response + 3);
			X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		X_FOCUSER_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_CONFIG_PROPERTY, NULL);
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
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_POWER_CHANNELS
	} else if (indigo_property_match(X_FOCUSER_POWER_CHANNELS_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_POWER_CHANNELS_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_power_channels_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_FOCUSER_CONFIG
	} else if (indigo_property_match(X_FOCUSER_CONFIG_PROPERTY, property)) {
		indigo_property_copy_values(X_FOCUSER_CONFIG_PROPERTY, property, false);
		indigo_set_timer(device, 0, focuser_config_handler, NULL);
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
	indigo_release_property(X_FOCUSER_POWER_CHANNELS_PROPERTY);
	indigo_release_property(X_FOCUSER_CONFIG_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_robofocus(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static robofocus_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"RoboFocus",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "RoboFocus Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(robofocus_private_data));
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
