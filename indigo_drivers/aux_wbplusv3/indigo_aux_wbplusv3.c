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

// This file generated from indigo_aux_wbplusv3.driver

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

#include "indigo_aux_wbplusv3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_aux_wbplusv3"
#define DRIVER_LABEL         "WandererBox Plus V3 Powerbox"
#define AUX_DEVICE_NAME      "WandererBox Plus V3"
#define PRIVATE_DATA         ((wbplusv3_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Powerbox"
#define WEATHER_GROUP        "Weather"
#define DEVICE_ID            "ZXWBPlusV3"

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_NAME_2_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 2)

#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)

#define AUX_POWER_OUTLET_VOLTAGE_PROPERTY (PRIVATE_DATA->aux_power_outlet_voltage_property)
#define AUX_POWER_OUTLET_VOLTAGE_1_ITEM   (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->items + 0)

#define AUX_USB_PORT_PROPERTY          (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM            (AUX_USB_PORT_PROPERTY->items + 0)

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)

#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

#define AUX_TEMPERATURE_SENSORS_PROPERTY      (PRIVATE_DATA->aux_temperature_sensors_property)
#define AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)

#define AUX_DEW_WARNING_PROPERTY       (PRIVATE_DATA->aux_dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM  (AUX_DEW_WARNING_PROPERTY->items + 0)

#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM          (AUX_INFO_PROPERTY->items + 1)

#define X_AUX_CALIBRATE_PROPERTY       (PRIVATE_DATA->x_aux_calibrate_property)
#define X_AUX_CALIBRATE_ITEM           (X_AUX_CALIBRATE_PROPERTY->items + 0)

#define X_AUX_CALIBRATE_PROPERTY_NAME  "X_AUX_CALIBRATE"
#define X_AUX_CALIBRATE_ITEM_NAME      "CALIBRATE"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_power_outlet_voltage_property;
	indigo_property *aux_usb_port_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_temperature_sensors_property;
	indigo_property *aux_dew_warning_property;
	indigo_property *aux_info_property;
	indigo_property *x_aux_calibrate_property;
	//+ data
	char model_id[1550];
	char firmware[20];
	double probe_temperature;
	double dht22_hunidity;
	double dht22_temperature;
	double input_current;
	double input_voltage;
	bool usb_status;
	bool dc2_status;
	uint8_t dc3_pwm;
	bool dc4_6_status;
	double dc2_voltage;
	//- data
} wbplusv3_private_data;

#pragma mark - Low level code

//+ code

static bool wbplusv3_read_status(indigo_device *device) {
	char status[256] = { 0 };
	indigo_uni_discard(PRIVATE_DATA->handle);
	long res = indigo_uni_read_line(PRIVATE_DATA->handle, status, 256);
	if (strncmp(status, DEVICE_ID, strlen(DEVICE_ID))) {   // first part of the message is cleared by tcflush();
		res = indigo_uni_read_line(PRIVATE_DATA->handle, status, 256);
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
		PRIVATE_DATA->probe_temperature = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		if (!strcmp(token, "nan")) {
			PRIVATE_DATA->dht22_hunidity = -127.0;
		} else {
			PRIVATE_DATA->dht22_hunidity = atof(token);
		}
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		if (!strcmp(token, "nan")) {
			PRIVATE_DATA->dht22_temperature = -127.0;
		} else {
			PRIVATE_DATA->dht22_temperature = atof(token);
		}
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->input_current = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->input_voltage = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc2_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc3_pwm = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc4_6_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc2_voltage = atof(token)/10.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nprobe_temperature = %.2fC\ndht22_hunidity = %.2f%%\ndht22_temperature = %.2fC\ninput_current = %.2fA\ninput_voltage = %.2fV\nusb_status = %d\ndc2_status = %d\ndc3_pwm = %d\ndc4_6_status = %d\ndc2_voltage = %.1fV\n", PRIVATE_DATA->model_id, PRIVATE_DATA->firmware, PRIVATE_DATA->probe_temperature, PRIVATE_DATA->dht22_hunidity, PRIVATE_DATA->dht22_temperature, PRIVATE_DATA->input_current, PRIVATE_DATA->input_voltage, PRIVATE_DATA->usb_status, PRIVATE_DATA->dc2_status, PRIVATE_DATA->dc3_pwm, PRIVATE_DATA->dc4_6_status, PRIVATE_DATA->dc2_voltage);
		return true;
	}
	return false;
}

static bool wbplusv3_command(indigo_device *device, int command) {
	if (indigo_uni_printf(PRIVATE_DATA->handle, "%d\n", command) > 0) {
		return true;
	}
	return false;
}

static bool wbplusv3_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		indigo_sleep(1);
		if (wbplusv3_read_status(device)) {
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

static void wbplusv3_close(indigo_device *device) {
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
	if (wbplusv3_read_status(device)) {
		AUX_WEATHER_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->dht22_temperature;
		AUX_WEATHER_HUMIDITY_ITEM->number.value = PRIVATE_DATA->dht22_hunidity;
		// calculate dew point
		if (PRIVATE_DATA->dht22_temperature > -100) {
			AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
			AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_aux_dewpoint(PRIVATE_DATA->dht22_temperature, PRIVATE_DATA->dht22_hunidity);
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_IDLE_STATE;
			AUX_WEATHER_DEWPOINT_ITEM->number.value = -273.15;
		}
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
		AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM->number.value = PRIVATE_DATA->dht22_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM->number.value = PRIVATE_DATA->probe_temperature;
		if (PRIVATE_DATA->dht22_temperature < -100 && PRIVATE_DATA->probe_temperature <-100) {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_IDLE_STATE;
		} else {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		AUX_INFO_VOLTAGE_ITEM->number.value = PRIVATE_DATA->input_voltage;
		AUX_INFO_CURRENT_ITEM->number.value = PRIVATE_DATA->input_current;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
		if (AUX_USB_PORT_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_USB_PORT_1_ITEM->sw.value = PRIVATE_DATA->usb_status;
			indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		}
		if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_1_ITEM->sw.value = PRIVATE_DATA->dc2_status;
			AUX_POWER_OUTLET_2_ITEM->sw.value = PRIVATE_DATA->dc4_6_status;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.value = PRIVATE_DATA->dc2_voltage;
			indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		}
		if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_HEATER_OUTLET_1_ITEM->number.value = (PRIVATE_DATA->dc3_pwm / 255.0) * 100;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}
		if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value && AUX_WEATHER_PROPERTY->state == INDIGO_OK_STATE) {
			if (((AUX_WEATHER_DEWPOINT_ITEM->number.value + 1) > PRIVATE_DATA->dht22_temperature) && PRIVATE_DATA->dc3_pwm != 255) {
				wbplusv3_command(device, 3255);
				indigo_send_message(device, "Heating started: Aproaching dewpoint");
			}
			if (((AUX_WEATHER_DEWPOINT_ITEM->number.value + 2) < PRIVATE_DATA->dht22_temperature) && PRIVATE_DATA->dc3_pwm != 0) {
				wbplusv3_command(device, 3000);
				indigo_send_message(device, "Heating stopped: Conditions are dry");
			}
		}
		// Dew point warning
		if (PRIVATE_DATA->dht22_temperature < -100) { // no sensor
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_IDLE_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;
		} else if (PRIVATE_DATA->dht22_temperature - 1 <= AUX_WEATHER_DEWPOINT_ITEM->number.value) {
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
		} else {
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, 1, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = wbplusv3_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
			indigo_execute_handler(device, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
		wbplusv3_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC3) [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC2)", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC2) [V]", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s (DC4-DC6)", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	wbplusv3_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? 121 : 120);
	wbplusv3_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? 101 : 100);
	indigo_sleep(1);
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

static void aux_power_outlet_voltage_handler(indigo_device *device) {
	AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET_VOLTAGE.on_change
	wbplusv3_command(device, 20000 + (int)(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.target * 10));
	indigo_sleep(1);
	//- aux.AUX_POWER_OUTLET_VOLTAGE.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
}

static void aux_usb_port_handler(indigo_device *device) {
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_USB_PORT.on_change
	wbplusv3_command(device, AUX_USB_PORT_1_ITEM->sw.value ? 111 : 110);
	indigo_sleep(1);
	//- aux.AUX_USB_PORT.on_change
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	wbplusv3_command(device, 3000 + (int)(AUX_HEATER_OUTLET_1_ITEM->number.target * 255.0 / 100.0));
	if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value) {
		indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	indigo_sleep(1);
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_dew_control_handler(indigo_device *device) {
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
}

static void aux_x_aux_calibrate_handler(indigo_device *device) {
	X_AUX_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_CALIBRATE.on_change
	if (X_AUX_CALIBRATE_ITEM->sw.value) {
		wbplusv3_command(device, 66300744);
		indigo_sleep(1);
		X_AUX_CALIBRATE_ITEM->sw.value = false;
		indigo_send_message(device, "Seensors recallibrated");
	}
	//- aux.X_AUX_CALIBRATE.on_change
	indigo_update_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Regulated outlet (DC2)", "Regulated outlet");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater outlet (DC3)", "Heater outlet");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Power outlets (DC4-DC6)", "Power outlets");
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Regulated outlet (DC2)", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power outlets (DC4-DC6)", true);
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_GROUP, "Regulated outlets voltage", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_POWER_OUTLET_VOLTAGE_1_ITEM, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME, "Regulated outlet (DC2) [V]", 0, 13.2, 0.2, 13.2);
		strcpy(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.format, "%.2f");
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB Ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "USB 2.0 && 3.1 ports", true);
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater outlet (DC3) [%]", 0, 100, 5, 0);
		strcpy(AUX_HEATER_OUTLET_1_ITEM->number.format, "%.0f");
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather info (DHT22)", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [°C]", 0, 0, 0, 0);
		strcpy(AUX_WEATHER_TEMPERATURE_ITEM->number.format, "%.2f");
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		strcpy(AUX_WEATHER_HUMIDITY_ITEM->number.format, "%.0f");
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [°C]", 0, 0, 0, 0);
		strcpy(AUX_WEATHER_DEWPOINT_ITEM->number.format, "%.2f");
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, WEATHER_GROUP, "Temperature sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Temperature (DHT22) [°C]", 0, 0, 0, 0);
		strcpy(AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM->number.format, "%.2f");
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Temperature (Temp probe) [°C]", 0, 0, 0, 0);
		strcpy(AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM->number.format, "%.2f");
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_OK_STATE, 1);
		if (AUX_DEW_WARNING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning (DHT22)", INDIGO_IDLE_STATE);
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 0, 0, 0);
		strcpy(AUX_INFO_VOLTAGE_ITEM->number.format, "%.2f");
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 0, 0, 0);
		strcpy(AUX_INFO_CURRENT_ITEM->number.format, "%.2f");
		X_AUX_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_CALIBRATE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Calibrate sensors", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_CALIBRATE_ITEM, X_AUX_CALIBRATE_ITEM_NAME, "Calibrate", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_USB_PORT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_CONTROL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_TEMPERATURE_SENSORS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_WARNING_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_CALIBRATE_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
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
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_PROPERTY, aux_power_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, aux_power_outlet_voltage_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_USB_PORT_PROPERTY, aux_usb_port_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DEW_CONTROL_PROPERTY, aux_dew_control_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_CALIBRATE_PROPERTY, aux_x_aux_calibrate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_CALIBRATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_wbplusv3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wbplusv3_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(wbplusv3_private_data));
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
