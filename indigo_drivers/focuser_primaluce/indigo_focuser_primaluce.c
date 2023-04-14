// Copyright (c) 2023 CloudMakers, s. r. o.
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

/** INDIGO PrimaluceLab focuser/rotator/flatfield driver
 \file indigo_focuser_primaluce.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_primaluce"

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

#include "indigo_focuser_primaluce.h"

#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"

#define PRIVATE_DATA													((primaluce_private_data *)device->private_data)

typedef struct {
	int handle;
	indigo_timer *timer;
	pthread_mutex_t mutex;
	jsmn_parser parser;
} primaluce_private_data;

static char *MODNAME[] = { "res", "get", "MODNAME", NULL };
static char *SN[] = { "res", "get", "SN", NULL };
static char *SWAPP[] = { "res", "get", "SWVERS", "SWAPP", NULL };
static char *SWWEB[] = { "res", "get", "SWVERS", "SWWEB", NULL };
static char *MOT1_ABS_POS[] = { "res", "get", "MOT1", "ABS_POS", NULL };
static char *MOT1_GET_SPEED[] = { "res", "get", "MOT1", "SPEED", NULL };
static char *MOT1_SET_SPEED[] = { "res", "set", "MOT1", "SPEED", NULL };
static char *MOT1_MIN_SPEED[] = { "res", "get", "MOT1", "MIN_SPEED", NULL };
static char *MOT1_MAX_SPEED[] = { "res", "get", "MOT1", "MAX_SPEED", NULL };
static char *MOT1_BUSY[] = { "res", "get", "MOT1", "STATUS", "BUSY", NULL };
static char *MOT1_MOVE_ABS_STEP[] = { "res", "cmd", "MOT1", "MOVE_ABS", "STEP", NULL };
static char *MOT1_MOT_STOP[] = { "res", "get", "MOT1", "MOT_STOP", NULL };
static char *MOT1_NTC_T[] = { "res", "get", "MOT1", "NTC_T", NULL };

// -------------------------------------------------------------------------------- Low level communication routines

static bool primaluce_command(indigo_device *device, char *command, char *response, int size, jsmntok_t *tokens, int count) {
	long result;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (!indigo_write(PRIVATE_DATA->handle, command, strlen(command))) {
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
//	struct timeval tv;
//	fd_set readout;
//	tv.tv_sec = 5;
//	tv.tv_usec = 0;
//	FD_ZERO(&readout);
//	FD_SET(PRIVATE_DATA->handle, &readout);
//	result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
//	if (result <= 0)
//		return false;
	result = indigo_read_line(PRIVATE_DATA->handle, response, size);
	if (result < 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
	jsmn_init(&PRIVATE_DATA->parser);
	if (*response == '"' || jsmn_parse(&PRIVATE_DATA->parser, response, size, tokens, count) <= 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to parse '%s' -> '%s'", command, response);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Parsed '%s' -> '%s'", command, response);
	for (int i = 0; i < count; i++) {
		if (tokens[i].type == JSMN_UNDEFINED)
			break;
		if (tokens[i].type == JSMN_STRING)
			response[tokens[i].end] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return true;
}

static int getToken(char *response, jsmntok_t *tokens, int start, char *path[]) {
	if (tokens[start].type != JSMN_OBJECT) {
		return -1;
	}
	int count = tokens[start].size;
	int index = start + 1;
	char *name = *path;
	for (int i = 0; i < count; i++) {
		if (tokens[index].type != JSMN_STRING)
			return -1;
		char *n = response + tokens[index].start;
		if (!strcmp(n, name)) {
			index++;
			if (*++path == NULL) {
				return index;
			}
			return getToken(response, tokens, index, path);
		} else {
			while (true) {
				index++;
				if (tokens[index].type == JSMN_UNDEFINED)
					return -1;
				if (tokens[index].parent == start)
					break;
			}
		}
	}
	return -1;
}

static char *getString(char *response, jsmntok_t *tokens, char *path[]) {
	int index = getToken(response, tokens, 0, path);
	if (index == -1 || tokens[index].type != JSMN_STRING)
		return NULL;
	return response + tokens[index].start;
}

static double getNumber(char *response, jsmntok_t *tokens, char *path[]) {
	int index = getToken(response, tokens, 0, path);
	if (index == -1)
		return 0;
	if  (tokens[index].type == JSMN_PRIMITIVE || tokens[index].type == JSMN_STRING)
		return atof(response + tokens[index].start);
	return 0;
}


static bool primaluce_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		char response[8 * 1024];
		jsmntok_t tokens[1024];
		char *text;
		if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}}", response, sizeof(response), tokens, 1024)) {
			if ((text = getString(response, tokens, MODNAME))) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Model: %s", text);
				indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, text);
			}
			if ((text = getString(response, tokens, SN))) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SN: %s", text);
				indigo_copy_value(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
			}
			if ((text = getString(response, tokens, SWAPP))) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWAPP: %s", text);
				indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
				if ((text = getString(response, tokens, SWWEB))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWWEB: %s", text);
					strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, " / ");
					strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
				}
			}
			indigo_delete_property(device, INFO_PROPERTY, NULL);
			indigo_define_property(device, INFO_PROPERTY, NULL);
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = getNumber(response, tokens, MOT1_ABS_POS);
			FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = getNumber(response, tokens, MOT1_GET_SPEED);
			FOCUSER_SPEED_ITEM->number.min = getNumber(response, tokens, MOT1_MIN_SPEED);
			FOCUSER_SPEED_ITEM->number.max = getNumber(response, tokens, MOT1_MAX_SPEED);
			FOCUSER_POSITION_PROPERTY->state = getNumber(response, tokens, MOT1_BUSY) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize");
		}
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void primaluce_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, "N/A");
		indigo_copy_value(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "N/A");
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
		indigo_delete_property(device, INFO_PROPERTY, NULL);
		indigo_define_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 8;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_timer_callback(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	if (!IS_CONNECTED)
		return;
	if (primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"NTC_T\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		double temp = getNumber(response, tokens, MOT1_NTC_T);
		if (temp != FOCUSER_TEMPERATURE_ITEM->number.value) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (primaluce_open(device)) {
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			primaluce_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}", (int)FOCUSER_POSITION_ITEM->number.target);
	if (primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		char *state = getString(response, tokens, MOT1_MOVE_ABS_STEP);
		if (state == NULL || strcmp(state, "done")) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			return;
		}
	}
	while (true) {
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":{\"BUSY\":\"\"}}}}}", response, sizeof(response), tokens, 128)) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = getNumber(response, tokens, MOT1_ABS_POS);
			if (getNumber(response, tokens, MOT1_BUSY) == 0) {
				break;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	int steps = (int)FOCUSER_STEPS_ITEM->number.target;
	position = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? (position + steps) : (position - steps);
	if (position < FOCUSER_POSITION_ITEM->number.min)
		position = FOCUSER_POSITION_ITEM->number.min;
	else if (position > FOCUSER_POSITION_ITEM->number.max)
		position = FOCUSER_POSITION_ITEM->number.max;
	snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}", position);
	if (primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		char *state = getString(response, tokens, MOT1_MOVE_ABS_STEP);
		if (state == NULL || strcmp(state, "done")) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			return;
		}
	}
	while (true) {
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":{\"BUSY\":\"\"}}}}}", response, sizeof(response), tokens, 128)) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = getNumber(response, tokens, MOT1_ABS_POS);
			if (getNumber(response, tokens, MOT1_BUSY) == 0) {
				break;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"MOT1\":{\"SPEED\":%d}}}}", (int)FOCUSER_SPEED_ITEM->number.target);
	if (primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		char *state = getString(response, tokens, MOT1_SET_SPEED);
		if (state == NULL || strcmp(state, "done")) {
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			return;
		}
	}
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_abort_handler(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	if (primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		char *state = getString(response, tokens, MOT1_MOT_STOP);
		if (state == NULL || strcmp(state, "done")) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			return;
		}
	}
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
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
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
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
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_primaluce(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static primaluce_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"PrimaluceLab Focuser",
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "PrimaluceLab Focuser/Rotator/Flatfield", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(primaluce_private_data));
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
