// Copyright (C) 2024 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO WandererCover V4-EC aux driver
 \file indigo_aux_wcv4ec.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_aux_wcv4ec"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include "indigo_aux_wcv4ec.h"

#define PRIVATE_DATA												((wcv4ec_private_data *)device->private_data)

#define AUX_LIGHT_SWITCH_PROPERTY          	(PRIVATE_DATA->light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM            (AUX_LIGHT_SWITCH_PROPERTY->items+0)
#define AUX_LIGHT_SWITCH_OFF_ITEM           (AUX_LIGHT_SWITCH_PROPERTY->items+1)

#define AUX_LIGHT_INTENSITY_PROPERTY				(PRIVATE_DATA->light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM						(AUX_LIGHT_INTENSITY_PROPERTY->items+0)

#define AUX_COVER_PROPERTY          				(PRIVATE_DATA->cover_property)
#define AUX_COVER_CLOSE_ITEM            		(AUX_COVER_PROPERTY->items+0)
#define AUX_COVER_OPEN_ITEM           			(AUX_COVER_PROPERTY->items+1)

#define AUX_DETECT_OPEN_CLOSE_PROPERTY          				(PRIVATE_DATA->detect_open_close_property)
#define AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM            		(AUX_DETECT_OPEN_CLOSE_PROPERTY->items+0)
#define AUX_DETECT_OPEN_CLOSE_OPEN_ITEM           			(AUX_DETECT_OPEN_CLOSE_PROPERTY->items+1)

#define AUX_SET_OPEN_CLOSE_PROPERTY          				(PRIVATE_DATA->set_open_close_property)
#define AUX_SET_OPEN_CLOSE_CLOSE_ITEM            		(AUX_SET_OPEN_CLOSE_PROPERTY->items+0)
#define AUX_SET_OPEN_CLOSE_OPEN_ITEM           			(AUX_SET_OPEN_CLOSE_PROPERTY->items+1)

#define AUX_HEATER_PROPERTY          	(PRIVATE_DATA->heater_property)
#define AUX_HEATER_OFF_ITEM            (AUX_HEATER_PROPERTY->items+0)
#define AUX_HEATER_LOW_ITEM           (AUX_HEATER_PROPERTY->items+1)
#define AUX_HEATER_HIGH_ITEM            (AUX_HEATER_PROPERTY->items+2)
#define AUX_HEATER_MAX_ITEM           (AUX_HEATER_PROPERTY->items+3)

#define DEVICE_ID "WandererCoverV4"

typedef struct {
	int handle;
	indigo_property *light_switch_property;
	indigo_property *light_intensity_property;
	indigo_property *cover_property;
	indigo_property *detect_open_close_property;
	indigo_property *set_open_close_property;
	indigo_property *heater_property;
	indigo_timer *aux_timer;
	pthread_mutex_t mutex;
	time_t operation_start_time;
	bool operation_running;
} wcv4ec_private_data;

typedef struct {
	char model_id[1550];
	char firmware[20];
	float close_position;
	float open_position;
	float current_position;
	float input_voltage;
	int brightness;
	bool ready;
} wcv4ec_status_t;

static bool wcv4ec_parse_status(char *status_line, wcv4ec_status_t *status) {
	char *buf;
	
	char* token = strtok_r(status_line, "A", &buf);
	if (token == NULL) {
		return false;
	}
	strncpy(status->model_id, token, sizeof(status->model_id));
	if (strcmp(status->model_id, DEVICE_ID)) {
		return false;
	}
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	strncpy(status->firmware, token, sizeof(status->firmware));
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->close_position = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->open_position = atof(token);

	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->current_position = atof(token);

	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->input_voltage = atof(token);

	/* inctroduced with firmware 20240618 */
	token = strtok_r(NULL, "A", &buf);
	if (token != NULL) {
		status->brightness = atoi(token);
	} else {
		status->brightness = 0;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nclose_position = %.2f\nopen_position = %.2f\ncurrent_position = %.2f\ninput_voltage = %.2fV\nbrightness = %d/255\ndone = %d\n",
	status->model_id, status->firmware, status->close_position, status->open_position, status->current_position, status->input_voltage, status->brightness, status->ready);
 
	return true;
}

static bool wcv4ec_read_status(indigo_device *device, wcv4ec_status_t *wc_stat) {
	char status[256] = {0};
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	wc_stat->ready = false;
	int res = indigo_read_line(PRIVATE_DATA->handle, status, 256);
	if (strncmp(status, DEVICE_ID, strlen(DEVICE_ID))) {   // first part of the message is cleared by tcflush() or "done";
		if (status[0] == '\0') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BRANCH: no id, status= '%s'", status);
			res = indigo_read_line(PRIVATE_DATA->handle, status, 256);
		}
		if (!strncmp(status, "done", strlen("done"))) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BRANCH: done");
			wc_stat->ready = true;
			res = indigo_read_line(PRIVATE_DATA->handle, status, 256);
		}
	}
	if (res == -1) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No status report");
		return false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Read: \"%s\" %d", status, res);
		return wcv4ec_parse_status(status, wc_stat);
	}
}

static void aux_update_states(indigo_device *device) {
	wcv4ec_status_t wc_stat = {0};
	if (wcv4ec_read_status(device, &wc_stat)) {
		bool update = false;
		if (fabs(wc_stat.close_position - wc_stat.current_position) < 6 && PRIVATE_DATA->operation_running) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Close");
			AUX_COVER_CLOSE_ITEM->sw.value = true;
			AUX_COVER_OPEN_ITEM->sw.value = false;
			AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->operation_running = false;
			PRIVATE_DATA->operation_start_time = 0;
			update = true;
		} else if (fabs(wc_stat.open_position - wc_stat.current_position) < 6 && PRIVATE_DATA->operation_running) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Open");
			AUX_COVER_CLOSE_ITEM->sw.value = false;
			AUX_COVER_OPEN_ITEM->sw.value = true;
			AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->operation_running = false;
			PRIVATE_DATA->operation_start_time = 0;
			update = true;
		} else if (PRIVATE_DATA->operation_running && AUX_COVER_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_COVER_CLOSE_ITEM->sw.value = false;
			AUX_COVER_OPEN_ITEM->sw.value = false;
			PRIVATE_DATA->operation_running = false;
			PRIVATE_DATA->operation_start_time = 0;
			update = true;
		}
		if (update) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Update");
			indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
		}

		update = false;
		if (fabs(AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.value - wc_stat.open_position) > 0.01) {
			AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.value = wc_stat.open_position;
			update = true;
		}
		if (fabs(AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.value - wc_stat.close_position) > 0.01) {
			AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.value = wc_stat.close_position;
			update = true;
		}
		if (update) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Update open close positions");
			if (AUX_SET_OPEN_CLOSE_PROPERTY->state == INDIGO_BUSY_STATE) {
				AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
		}
	}

	// timeout if open or close get stuck somewhere
	if(time(NULL) - PRIVATE_DATA->operation_start_time > 60 && PRIVATE_DATA->operation_start_time > 0) {
		AUX_COVER_CLOSE_ITEM->sw.value = false;
		AUX_COVER_OPEN_ITEM->sw.value = false;
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
		PRIVATE_DATA->operation_running = false;
		PRIVATE_DATA->operation_start_time = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME,"Open/close operation timeout");
		indigo_update_property(device, AUX_COVER_PROPERTY, "Open/close operation timeout");
	}
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	aux_update_states(device);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->aux_timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// -------------------------------------------------------------------------------- Low level communication routines
static bool wcv4ec_command(indigo_device *device, char *command) {
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	int res = indigo_write(PRIVATE_DATA->handle, "\n", 1);
	if (res < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command %s failed", command);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s", command);
	return true;
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity", 0, 255, 1, 50);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		// -------------------------------------------------------------------------------- AUX_COVER
		AUX_COVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_COVER_PROPERTY_NAME, AUX_MAIN_GROUP, "Cover (open/close)", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_COVER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_COVER_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(AUX_COVER_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Close", false);
		// -------------------------------------------------------------------------------- X_COVER_DETECT_OPEN_CLOSE
		AUX_DETECT_OPEN_CLOSE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_COVER_DETECT_OPEN_CLOSE", AUX_ADVANCED_GROUP, "Detect cover open/close position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DETECT_OPEN_CLOSE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DETECT_OPEN_CLOSE_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Detect Open", false);
		indigo_init_switch_item(AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Detect Close", false);
		// -------------------------------------------------------------------------------- X_COVER_SET_OPEN_CLOSE
		AUX_SET_OPEN_CLOSE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_COVER_SET_OPEN_CLOSE", AUX_ADVANCED_GROUP, "Set cover open/close position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_SET_OPEN_CLOSE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_SET_OPEN_CLOSE_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Set Open [°]", 0, 290, 1, 110);
		indigo_init_number_item(AUX_SET_OPEN_CLOSE_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Set Close [°]", 0, 290, 1, 22);
		// -------------------------------------------------------------------------------- X_HEATER
		AUX_HEATER_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_HEATER", AUX_MAIN_GROUP, "Heater", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AUX_HEATER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_HEATER_OFF_ITEM, "OFF", "Off", true);
		indigo_init_switch_item(AUX_HEATER_LOW_ITEM, "LOW", "Low", false);
		indigo_init_switch_item(AUX_HEATER_HIGH_ITEM, "HIGH", "High", false);
		indigo_init_switch_item(AUX_HEATER_MAX_ITEM, "MAX", "Max", false);
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
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_LIGHT_SWITCH_PROPERTY);
		indigo_define_matching_property(AUX_LIGHT_INTENSITY_PROPERTY);
		indigo_define_matching_property(AUX_COVER_PROPERTY);
		indigo_define_matching_property(AUX_DETECT_OPEN_CLOSE_PROPERTY);
		indigo_define_matching_property(AUX_SET_OPEN_CLOSE_PROPERTY);
		indigo_define_matching_property(AUX_HEATER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		wcv4ec_status_t wc_stat = {0};
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			indigo_usleep(ONE_SECOND_DELAY);
			if (wcv4ec_read_status(device, &wc_stat)) {
				if (!strcmp(wc_stat.model_id, DEVICE_ID)) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, wc_stat.model_id);
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, wc_stat.firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device is not WandererCover V4-EC");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device is not WandererCover V4-EC");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			PRIVATE_DATA->operation_start_time = 0;
			if (wc_stat.brightness > 0) {
				AUX_LIGHT_SWITCH_ON_ITEM->sw.value = true;
				AUX_LIGHT_SWITCH_OFF_ITEM->sw.value = false;
				AUX_LIGHT_INTENSITY_ITEM->number.value = wc_stat.brightness;
			} else {
				wcv4ec_command(device, "9999"); // turn light off as the state is not known with older firmware
				AUX_LIGHT_SWITCH_ON_ITEM->sw.value = false;
				AUX_LIGHT_SWITCH_OFF_ITEM->sw.value = true;
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
			}

			wcv4ec_command(device, "2000"); // turn the heater off as the state is not known
			AUX_HEATER_OFF_ITEM->sw.value = true;
			AUX_HEATER_LOW_ITEM->sw.value = false;
			AUX_HEATER_HIGH_ITEM->sw.value = false;
			AUX_HEATER_MAX_ITEM->sw.value = false;
			AUX_HEATER_PROPERTY->state = INDIGO_OK_STATE;

			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_define_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
			indigo_define_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_PROPERTY, NULL);
			PRIVATE_DATA->operation_running = true; //force open close status update;
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_COVER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_PROPERTY, NULL);
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		wcv4ec_command(device, "9999"); // turn light off
		wcv4ec_command(device, "2000"); // turn the heater off
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_switch_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
		sprintf(command, "%d", (int)(AUX_LIGHT_INTENSITY_ITEM->number.value));
	} else {
		strcpy(command, "9999");
	}
	if (wcv4ec_command(device, command)) {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_intensity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
		sprintf(command, "%d", (int)(AUX_LIGHT_INTENSITY_ITEM->number.value));
		if (wcv4ec_command(device, command)) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_cover_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (PRIVATE_DATA->operation_running) {
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	char command[16];
	strcpy(command, AUX_COVER_OPEN_ITEM->sw.value ? "1001" : "1000");
	if (wcv4ec_command(device, command)) {
		PRIVATE_DATA->operation_start_time = time(NULL);
		PRIVATE_DATA->operation_running = true;
		AUX_COVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_usleep(ONE_SECOND_DELAY);
	} else {
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_detect_open_close_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (PRIVATE_DATA->operation_running) {
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	bool success = false;
	if (AUX_DETECT_OPEN_CLOSE_OPEN_ITEM->sw.value) {
		success = wcv4ec_command(device, "100001");
	} else if (AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM->sw.value) {
		success = wcv4ec_command(device, "100000");
	}
	if (success) {
		PRIVATE_DATA->operation_running = true; // let the status callback set correct open/close when we are done
		char status_line[128] = {0};
		tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
		do {
			indigo_read_line(PRIVATE_DATA->handle, status_line, 128);
		} while (strncmp(status_line, "OpenSet", strlen("OpenSet")) && strncmp(status_line, "CloseSet", strlen("CloseSet")));
		AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Autodetect open/close failed");
	}
	AUX_DETECT_OPEN_CLOSE_OPEN_ITEM->sw.value =false;
	AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM->sw.value =false;
	indigo_update_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_set_open_close_handler(indigo_device *device) {
	char command[32];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (PRIVATE_DATA->operation_running) {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Operation in progress");
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	if (AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.target <= AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.target + 45) {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Open position can not be smaller than Close + 45");
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return;
	}
	bool success = false;
	sprintf(command, "%d", 40000 + (int)(AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.target * 100));
	success = wcv4ec_command(device, command);
	sprintf(command, "%d", 10000 + (int)(AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.target * 100));
	success = wcv4ec_command(device, command);
	if (success) {
		indigo_usleep(ONE_SECOND_DELAY);
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Set open/close failed");
	}
	indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	bool success = true;
	if (AUX_HEATER_OFF_ITEM->sw.value) {
		success = wcv4ec_command(device, "2000");
	} else if (AUX_HEATER_LOW_ITEM->sw.value) {
		success = wcv4ec_command(device, "2050");
	} else if (AUX_HEATER_HIGH_ITEM->sw.value) {
		success = wcv4ec_command(device, "2100");
	} else if (AUX_HEATER_MAX_ITEM->sw.value) {
		success = wcv4ec_command(device, "2150");
	}
	if (success) {
		AUX_HEATER_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_HEATER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_HEATER_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_SWITCH_PROPERTY, property, false);
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_switch_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_INTENSITY_PROPERTY, property, false);
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_intensity_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_COVER
	} else if (indigo_property_match_changeable(AUX_COVER_PROPERTY, property)) {
		indigo_property_copy_values(AUX_COVER_PROPERTY, property, false);
		if (AUX_DETECT_OPEN_CLOSE_PROPERTY->state == INDIGO_BUSY_STATE) {
			AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_COVER_PROPERTY, "Operation in progress");
			return INDIGO_OK;
		}
		AUX_COVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_cover_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_DETECT_OPEN_CLOSE
	} else if (indigo_property_match_changeable(AUX_DETECT_OPEN_CLOSE_PROPERTY, property)) {
		indigo_property_copy_values(AUX_DETECT_OPEN_CLOSE_PROPERTY, property, false);
		if (AUX_DETECT_OPEN_CLOSE_PROPERTY->state == INDIGO_BUSY_STATE || AUX_COVER_PROPERTY->state == INDIGO_BUSY_STATE) {
			AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
			AUX_DETECT_OPEN_CLOSE_OPEN_ITEM->sw.value =false;
			AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM->sw.value =false;
			indigo_update_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, "Operation in progress");
			return INDIGO_OK;
		}
		AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_detect_open_close_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_SET_OPEN_CLOSE
	} else if (indigo_property_match_changeable(AUX_SET_OPEN_CLOSE_PROPERTY, property)) {
		indigo_property_copy_values(AUX_SET_OPEN_CLOSE_PROPERTY, property, false);
		if (AUX_DETECT_OPEN_CLOSE_PROPERTY->state == INDIGO_BUSY_STATE || AUX_COVER_PROPERTY->state == INDIGO_BUSY_STATE) {
			AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Operation in progress");
			return INDIGO_OK;
		}
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_set_open_close_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_HEATER
	} else if (indigo_property_match_changeable(AUX_HEATER_PROPERTY, property)) {
		indigo_property_copy_values(AUX_HEATER_PROPERTY, property, false);
		AUX_HEATER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_HEATER_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_heater_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
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
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(AUX_COVER_PROPERTY);
	indigo_release_property(AUX_DETECT_OPEN_CLOSE_PROPERTY);
	indigo_release_property(AUX_SET_OPEN_CLOSE_PROPERTY);
	indigo_release_property(AUX_HEATER_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation
indigo_result indigo_aux_wcv4ec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wcv4ec_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"WandererCover V4-EC",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, "WandererCover V4-EC Cover", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(wcv4ec_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
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
