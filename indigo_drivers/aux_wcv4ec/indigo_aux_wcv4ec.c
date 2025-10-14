// 
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

// This file generated from indigo_aux_wcv4ec.driver

// TODO: Test with real hardware or simulator!!!

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_wcv4ec.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_aux_wcv4ec"
#define DRIVER_LABEL         "WandererCover V4-EC Cover"
#define AUX_DEVICE_NAME      "WandererCover V4-EC"
#define PRIVATE_DATA         ((wcv4ec_private_data *)device->private_data)

//+ define

#define DEVICE_ID            "WandererCoverV4"

//- define

#pragma mark - Property definitions

#define AUX_LIGHT_SWITCH_PROPERTY      (PRIVATE_DATA->aux_light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM       (AUX_LIGHT_SWITCH_PROPERTY->items + 0)
#define AUX_LIGHT_SWITCH_OFF_ITEM      (AUX_LIGHT_SWITCH_PROPERTY->items + 1)

#define AUX_LIGHT_INTENSITY_PROPERTY   (PRIVATE_DATA->aux_light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM       (AUX_LIGHT_INTENSITY_PROPERTY->items + 0)

#define AUX_DETECT_OPEN_CLOSE_PROPERTY      (PRIVATE_DATA->aux_detect_open_close_property)
#define AUX_DETECT_OPEN_CLOSE_OPEN_ITEM     (AUX_DETECT_OPEN_CLOSE_PROPERTY->items + 0)
#define AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM    (AUX_DETECT_OPEN_CLOSE_PROPERTY->items + 1)

#define AUX_DETECT_OPEN_CLOSE_PROPERTY_NAME "X_COVER_DETECT_OPEN_CLOSE"

#define AUX_SET_OPEN_CLOSE_PROPERTY      (PRIVATE_DATA->aux_set_open_close_property)
#define AUX_SET_OPEN_CLOSE_OPEN_ITEM     (AUX_SET_OPEN_CLOSE_PROPERTY->items + 0)
#define AUX_SET_OPEN_CLOSE_CLOSE_ITEM    (AUX_SET_OPEN_CLOSE_PROPERTY->items + 1)

#define AUX_SET_OPEN_CLOSE_PROPERTY_NAME "X_COVER_SET_OPEN_CLOSE"

#define AUX_HEATER_PROPERTY            (PRIVATE_DATA->aux_heater_property)
#define AUX_HEATER_OFF_ITEM            (AUX_HEATER_PROPERTY->items + 0)
#define AUX_HEATER_LOW_ITEM            (AUX_HEATER_PROPERTY->items + 1)
#define AUX_HEATER_HIGH_ITEM           (AUX_HEATER_PROPERTY->items + 2)
#define AUX_HEATER_MAX_ITEM            (AUX_HEATER_PROPERTY->items + 3)

#define AUX_HEATER_PROPERTY_NAME       "X_HEATER"
#define AUX_HEATER_OFF_ITEM_NAME       "OFF"
#define AUX_HEATER_LOW_ITEM_NAME       "LOW"
#define AUX_HEATER_HIGH_ITEM_NAME      "HIGH"
#define AUX_HEATER_MAX_ITEM_NAME       "MAX"

#define AUX_COVER_PROPERTY             (PRIVATE_DATA->aux_cover_property)
#define AUX_COVER_OPEN_ITEM            (AUX_COVER_PROPERTY->items + 0)
#define AUX_COVER_CLOSE_ITEM           (AUX_COVER_PROPERTY->items + 1)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_light_switch_property;
	indigo_property *aux_light_intensity_property;
	indigo_property *aux_detect_open_close_property;
	indigo_property *aux_set_open_close_property;
	indigo_property *aux_heater_property;
	indigo_property *aux_cover_property;
	//+ data
	time_t operation_start_time;
	bool operation_running;
	char model_id[1550];
	char firmware[20];
	double close_position;
	double open_position;
	double current_position;
	double input_voltage;
	int brightness;
	bool ready;
	//- data
} wcv4ec_private_data;

#pragma mark - Low level code

//+ code

static bool wcv4ec_read_status(indigo_device *device) {
	char status[256] = { 0 };
	indigo_uni_discard(PRIVATE_DATA->handle);
	PRIVATE_DATA->ready = false;
	long res = indigo_uni_read_line(PRIVATE_DATA->handle, status, 256);
	if (strncmp(status, DEVICE_ID, strlen(DEVICE_ID))) {   // first part of the message is cleared by tcflush() or "done";
		if (status[0] == '\0') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BRANCH: no id, status= '%s'", status);
			res = indigo_uni_read_line(PRIVATE_DATA->handle, status, 256);
		}
		if (!strncmp(status, "done", strlen("done"))) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "BRANCH: done");
			PRIVATE_DATA->ready = true;
			res = indigo_uni_read_line(PRIVATE_DATA->handle, status, 256);
		}
	}
	if (res > 0) {
		char *buf;
		char* token = strtok_r(status, "A", &buf);
		if (token == NULL) {
			return false;
		}
		strncpy(PRIVATE_DATA->model_id, token, sizeof(PRIVATE_DATA->model_id));
		if (strcmp(PRIVATE_DATA->model_id, DEVICE_ID)) {
			return false;
		}
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		strncpy(PRIVATE_DATA->firmware, token, sizeof(PRIVATE_DATA->firmware));
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->close_position = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->open_position = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->current_position = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->input_voltage = atof(token);
		/* inctroduced with firmware 20240618 */
		token = strtok_r(NULL, "A", &buf);
		if (token != NULL) {
			PRIVATE_DATA->brightness = atoi(token);
		} else {
			PRIVATE_DATA->brightness = 0;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nclose_position = %.2f\nopen_position = %.2f\ncurrent_position = %.2f\ninput_voltage = %.2fV\nbrightness = %d/255\ndone = %d\n",
		PRIVATE_DATA->model_id, PRIVATE_DATA->firmware, PRIVATE_DATA->close_position, PRIVATE_DATA->open_position, PRIVATE_DATA->current_position, PRIVATE_DATA->input_voltage, PRIVATE_DATA->brightness, PRIVATE_DATA->ready);
		return true;
	}
	return false;
}

static bool wcv4ec_command(indigo_device *device, int command) {
	if (indigo_uni_printf(PRIVATE_DATA->handle, "%d\n", command) > 0) {
		return true;
	}
	return false;
}

static bool wcv4ec_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		indigo_sleep(1);
		if (wcv4ec_read_status(device)) {
			if (!strcmp(PRIVATE_DATA->model_id, DEVICE_ID)) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model_id);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void wcv4ec_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	if (wcv4ec_read_status(device)) {
		bool update = false;
		if (fabs(PRIVATE_DATA->close_position - PRIVATE_DATA->current_position) < 6 && PRIVATE_DATA->operation_running) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"Close");
			AUX_COVER_CLOSE_ITEM->sw.value = true;
			AUX_COVER_OPEN_ITEM->sw.value = false;
			AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->operation_running = false;
			PRIVATE_DATA->operation_start_time = 0;
			update = true;
		} else if (fabs(PRIVATE_DATA->open_position - PRIVATE_DATA->current_position) < 6 && PRIVATE_DATA->operation_running) {
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
		if (fabs(AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.value - PRIVATE_DATA->open_position) > 0.01) {
			AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.value = PRIVATE_DATA->open_position;
			update = true;
		}
		if (fabs(AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.value - PRIVATE_DATA->close_position) > 0.01) {
			AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.value = PRIVATE_DATA->close_position;
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
	if (time(NULL) - PRIVATE_DATA->operation_start_time > 60 && PRIVATE_DATA->operation_start_time > 0) {
		AUX_COVER_CLOSE_ITEM->sw.value = false;
		AUX_COVER_OPEN_ITEM->sw.value = false;
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
		PRIVATE_DATA->operation_running = false;
		PRIVATE_DATA->operation_start_time = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME,"Open/close operation timeout");
		indigo_update_property(device, AUX_COVER_PROPERTY, "Open/close operation timeout");
	}
	indigo_execute_handler_in(device, 1, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = wcv4ec_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
			indigo_define_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_PROPERTY, NULL);
			indigo_define_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_COVER_PROPERTY, NULL);
		//+ aux.on_disconnect
		wcv4ec_command(device, 9999); // turn light off
		wcv4ec_command(device, 2000); // turn the heater off
		//- aux.on_disconnect
		wcv4ec_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_light_switch_handler(indigo_device *device) {
	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_SWITCH.on_change
	if (!wcv4ec_command(device, AUX_LIGHT_SWITCH_ON_ITEM->sw.value ? (int)(AUX_LIGHT_INTENSITY_ITEM->number.value) : 9999)) {
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_LIGHT_SWITCH.on_change
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
}

static void aux_light_intensity_handler(indigo_device *device) {
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_INTENSITY.on_change
	if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
		if (!wcv4ec_command(device, (int)(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, AUX_LIGHT_SWITCH_ON_ITEM, true);
	}
	//- aux.AUX_LIGHT_INTENSITY.on_change
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
}

static void aux_detect_open_close_handler(indigo_device *device) {
	AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DETECT_OPEN_CLOSE.on_change
	if (PRIVATE_DATA->operation_running) {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Operation in progress");
		return;
	} else {
		bool success = false;
		if (AUX_DETECT_OPEN_CLOSE_OPEN_ITEM->sw.value) {
			success = wcv4ec_command(device, 100001);
		} else if (AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM->sw.value) {
			success = wcv4ec_command(device, 100000);
		}
		if (success) {
			PRIVATE_DATA->operation_running = true; // let the status callback set correct open/close when we are done
			char status_line[128] = {0};
			indigo_uni_discard(PRIVATE_DATA->handle);
			do {
				indigo_uni_read_line(PRIVATE_DATA->handle, status_line, 128);
			} while (strncmp(status_line, "OpenSet", strlen("OpenSet")) && strncmp(status_line, "CloseSet", strlen("CloseSet")));
		} else {
			AUX_DETECT_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Autodetect open/close failed");
		}
		AUX_DETECT_OPEN_CLOSE_OPEN_ITEM->sw.value = false;
		AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM->sw.value = false;
	}
	//- aux.AUX_DETECT_OPEN_CLOSE.on_change
	indigo_update_property(device, AUX_DETECT_OPEN_CLOSE_PROPERTY, NULL);
}

static void aux_set_open_close_handler(indigo_device *device) {
	AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SET_OPEN_CLOSE.on_change
	if (PRIVATE_DATA->operation_running) {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Operation in progress");
		return;
	}
	if (AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.target <= AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.target + 45) {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, "Open position can not be smaller than Close + 45");
		return;
	}
	bool success = wcv4ec_command(device, 40000 + (int)(AUX_SET_OPEN_CLOSE_OPEN_ITEM->number.target * 100));
	success = success && wcv4ec_command(device, 10000 + (int)(AUX_SET_OPEN_CLOSE_CLOSE_ITEM->number.target * 100));
	if (success) {
		indigo_sleep(1);
	} else {
		AUX_SET_OPEN_CLOSE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Set open/close failed");
	}
	//- aux.AUX_SET_OPEN_CLOSE.on_change
	indigo_update_property(device, AUX_SET_OPEN_CLOSE_PROPERTY, NULL);
}

static void aux_heater_handler(indigo_device *device) {
	AUX_HEATER_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER.on_change
	bool success = true;
	if (AUX_HEATER_OFF_ITEM->sw.value) {
		success = wcv4ec_command(device, 2000);
	} else if (AUX_HEATER_LOW_ITEM->sw.value) {
		success = wcv4ec_command(device, 2050);
	} else if (AUX_HEATER_HIGH_ITEM->sw.value) {
		success = wcv4ec_command(device, 2100);
	} else if (AUX_HEATER_MAX_ITEM->sw.value) {
		success = wcv4ec_command(device, 2150);
	}
	if (success) {
		indigo_sleep(1);
	} else {
		AUX_HEATER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_HEATER.on_change
	indigo_update_property(device, AUX_HEATER_PROPERTY, NULL);
}

static void aux_cover_handler(indigo_device *device) {
	AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_COVER.on_change
	if (PRIVATE_DATA->operation_running) {
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AUX_COVER_PROPERTY, "Operation in progress");
		return;
	}
	if (wcv4ec_command(device, AUX_COVER_OPEN_ITEM->sw.value ? 1001 : 1000)) {
		PRIVATE_DATA->operation_start_time = time(NULL);
		PRIVATE_DATA->operation_running = true;
		AUX_COVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_sleep(1);
	} else {
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_COVER.on_change
	indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity", 0, 255, 1, 50);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%.0f");
		AUX_DETECT_OPEN_CLOSE_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DETECT_OPEN_CLOSE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Detect cover open/close position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DETECT_OPEN_CLOSE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DETECT_OPEN_CLOSE_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Detect Open", false);
		indigo_init_switch_item(AUX_DETECT_OPEN_CLOSE_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Detect Close", false);
		AUX_SET_OPEN_CLOSE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_SET_OPEN_CLOSE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Set cover open/close position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_SET_OPEN_CLOSE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_SET_OPEN_CLOSE_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Set Open [°]", 0, 290, 1, 110);
		indigo_init_number_item(AUX_SET_OPEN_CLOSE_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Set Close [°]", 0, 290, 1, 22);
		AUX_HEATER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_HEATER_PROPERTY_NAME, AUX_MAIN_GROUP, "Heater", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AUX_HEATER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_HEATER_OFF_ITEM, AUX_HEATER_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(AUX_HEATER_LOW_ITEM, AUX_HEATER_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(AUX_HEATER_HIGH_ITEM, AUX_HEATER_HIGH_ITEM_NAME, "High", false);
		indigo_init_switch_item(AUX_HEATER_MAX_ITEM, AUX_HEATER_MAX_ITEM_NAME, "Max", false);
		AUX_COVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_COVER_PROPERTY_NAME, AUX_MAIN_GROUP, "Cover (open/close)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_COVER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_COVER_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(AUX_COVER_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Close", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_SWITCH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_INTENSITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DETECT_OPEN_CLOSE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SET_OPEN_CLOSE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_COVER_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_SWITCH_PROPERTY, aux_light_switch_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_INTENSITY_PROPERTY, aux_light_intensity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DETECT_OPEN_CLOSE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DETECT_OPEN_CLOSE_PROPERTY, aux_detect_open_close_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SET_OPEN_CLOSE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SET_OPEN_CLOSE_PROPERTY, aux_set_open_close_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_PROPERTY, aux_heater_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_COVER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_COVER_PROPERTY, aux_cover_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(AUX_DETECT_OPEN_CLOSE_PROPERTY);
	indigo_release_property(AUX_SET_OPEN_CLOSE_PROPERTY);
	indigo_release_property(AUX_HEATER_PROPERTY);
	indigo_release_property(AUX_COVER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_wcv4ec(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wcv4ec_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

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
