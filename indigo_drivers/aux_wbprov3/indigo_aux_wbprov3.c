// 
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_aux_wbprov3.driver

// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

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

#include "indigo_aux_wbprov3.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_aux_wbprov3"
#define DRIVER_LABEL         "WandererBox Pro V3 Powerbox"
#define AUX_DEVICE_NAME      "WandererBox Pro V3"
#define PRIVATE_DATA         ((wbprov3_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Powerbox"
#define WEATHER_GROUP        "Weather"
#define DEVICE_ID            "ZXWBProV3"

//- define

#pragma mark - Property definitions

// AUX_OUTLET_NAMES handles definition
#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_NAME_2_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_HEATER_OUTLET_NAME_3_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_NAME_2_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_NAME_3_ITEM   (AUX_OUTLET_NAMES_PROPERTY->items + 5)

// AUX_POWER_OUTLET handles definition
#define AUX_POWER_OUTLET_PROPERTY      (PRIVATE_DATA->aux_power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM        (AUX_POWER_OUTLET_PROPERTY->items + 2)

// AUX_POWER_OUTLET_VOLTAGE handles definition
#define AUX_POWER_OUTLET_VOLTAGE_PROPERTY (PRIVATE_DATA->aux_power_outlet_voltage_property)
#define AUX_POWER_OUTLET_VOLTAGE_1_ITEM   (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->items + 0)

// AUX_POWER_OUTLET_CURRENT handles definition
#define AUX_POWER_OUTLET_CURRENT_PROPERTY (PRIVATE_DATA->aux_power_outlet_current_property)
#define AUX_POWER_OUTLET_CURRENT_1_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_CURRENT_2_ITEM   (AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 1)

// AUX_USB_PORT handles definition
#define AUX_USB_PORT_PROPERTY          (PRIVATE_DATA->aux_usb_port_property)
#define AUX_USB_PORT_1_ITEM            (AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM            (AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM            (AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM            (AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM            (AUX_USB_PORT_PROPERTY->items + 4)

// AUX_HEATER_OUTLET handles definition
#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 2)

// AUX_DEW_CONTROL handles definition
#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

// AUX_WEATHER handles definition
#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

// AUX_TEMPERATURE_SENSORS handles definition
#define AUX_TEMPERATURE_SENSORS_PROPERTY      (PRIVATE_DATA->aux_temperature_sensors_property)
#define AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)
#define AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 2)
#define AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 3)

// AUX_DEW_WARNING handles definition
#define AUX_DEW_WARNING_PROPERTY       (PRIVATE_DATA->aux_dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM  (AUX_DEW_WARNING_PROPERTY->items + 0)

// AUX_INFO handles definition
#define AUX_INFO_PROPERTY              (PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM          (AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM          (AUX_INFO_PROPERTY->items + 1)

// X_AUX_CALIBRATE handles definition
#define X_AUX_CALIBRATE_PROPERTY       (PRIVATE_DATA->x_aux_calibrate_property)
#define X_AUX_CALIBRATE_ITEM           (X_AUX_CALIBRATE_PROPERTY->items + 0)

#define X_AUX_CALIBRATE_PROPERTY_NAME  "X_AUX_CALIBRATE"
#define X_AUX_CALIBRATE_ITEM_NAME      "CALIBRATE"

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_power_outlet_property;
	indigo_property *aux_power_outlet_voltage_property;
	indigo_property *aux_power_outlet_current_property;
	indigo_property *aux_usb_port_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_temperature_sensors_property;
	indigo_property *aux_dew_warning_property;
	indigo_property *aux_info_property;
	indigo_property *x_aux_calibrate_property;
	indigo_timer *aux_timer;
	indigo_timer *aux_connection_handler_timer;
	indigo_timer *aux_outlet_names_handler_timer;
	indigo_timer *aux_power_outlet_handler_timer;
	indigo_timer *aux_power_outlet_voltage_handler_timer;
	indigo_timer *aux_usb_port_handler_timer;
	indigo_timer *aux_heater_outlet_handler_timer;
	indigo_timer *aux_dew_control_handler_timer;
	indigo_timer *aux_x_aux_calibrate_handler_timer;
	//+ data
	char model_id[50];
	char firmware[20];
	double probe1_temperature;
	double probe2_temperature;
	double probe3_temperature;
	double dht22_hunidity;
	double dht22_temperature;
	double input_current;
	double dc2_current;
	double dc3_4_current;
	double input_voltage;
	bool usb31_1_status;
	bool usb31_2_status;
	bool usb31_3_status;
	bool usb20_1_3_status;
	bool usb20_4_6_status;
	bool dc3_4_status;
	uint8_t dc5_pwm;
	uint8_t dc6_pwm;
	uint8_t dc7_pwm;
	bool dc8_9_status;
	bool dc10_11_status;
	double dc3_4_voltage;
	//- data
} wbprov3_private_data;

#pragma mark - Low level code

//+ code

static bool wbprov3_read_status(indigo_device *device) {
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
		PRIVATE_DATA->probe1_temperature = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->probe2_temperature = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->probe3_temperature = atof(token);
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
		PRIVATE_DATA->dc2_current = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc3_4_current = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->input_voltage = atof(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb31_1_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb31_2_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb31_3_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb20_1_3_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->usb20_4_6_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc3_4_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc5_pwm = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc6_pwm = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc7_pwm = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc8_9_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc10_11_status = atoi(token);
		token = strtok_r(NULL, "A", &buf);
		if (token == NULL) {
			return false;
		}
		PRIVATE_DATA->dc3_4_voltage = atof(token)/10.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nprobe1_temperature = %.2fC\nprobe2_temperature = %.2fC\nprobe3_temperature = %.2fC\ndht22_hunidity = %.2f%%\ndht22_temperature = %.2fC\ninput_current = %.2fA\ndc2_current = %.2fA\ndc3_4_current = %.2fA\ninput_voltage = %.2fV\nusb31_1_status = %d\nusb31_2_status = %d\nusb31_3_status = %d\nusb20_1_3_status = %d\nusb20_4_6_status = %d\ndc3_4_status = %d\ndc5_pwm = %d\ndc6_pwm = %d\ndc7_pwm = %d\ndc8_9_status = %d\ndc10_11_status = %d\ndc3_4_voltage = %.1fV\n", PRIVATE_DATA->model_id, PRIVATE_DATA->firmware, PRIVATE_DATA->probe1_temperature, PRIVATE_DATA->probe2_temperature, PRIVATE_DATA->probe3_temperature, PRIVATE_DATA->dht22_hunidity, PRIVATE_DATA->dht22_temperature, PRIVATE_DATA->input_current, PRIVATE_DATA->dc2_current, PRIVATE_DATA->dc3_4_current, PRIVATE_DATA->input_voltage, PRIVATE_DATA->usb31_1_status, PRIVATE_DATA->usb31_2_status, PRIVATE_DATA->usb31_3_status, PRIVATE_DATA->usb20_1_3_status, PRIVATE_DATA->usb20_4_6_status, PRIVATE_DATA->dc3_4_status, PRIVATE_DATA->dc5_pwm, PRIVATE_DATA->dc6_pwm, PRIVATE_DATA->dc7_pwm, PRIVATE_DATA->dc8_9_status, PRIVATE_DATA->dc10_11_status, PRIVATE_DATA->dc3_4_voltage);
		return true;	}
	return false;
}

static bool wbprov3_command(indigo_device *device, char *command) {
	indigo_uni_discard(PRIVATE_DATA->handle);
	if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) > 0) {
		return true;
	}
	return false;
}

static bool wbprov3_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		indigo_sleep(1);
		if (wbprov3_read_status(device)) {
			if (!strcmp(PRIVATE_DATA->model_id, DEVICE_ID)) {
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model_id);
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
		indigo_send_message(device, "Handshake failed");
	}
	return false;
}

static void wbprov3_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

//- code

#pragma mark - High level code (aux)

// aux state checking timer callback

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ aux.on_timer
	if (wbprov3_read_status(device)) {
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
		AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM->number.value = PRIVATE_DATA->probe1_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM->number.value = PRIVATE_DATA->probe2_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM->number.value = PRIVATE_DATA->probe3_temperature;
		if (PRIVATE_DATA->dht22_temperature < -100 && PRIVATE_DATA->probe1_temperature <-100 && PRIVATE_DATA->probe2_temperature && PRIVATE_DATA->probe3_temperature) {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_IDLE_STATE;
		} else {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		AUX_INFO_VOLTAGE_ITEM->number.value = PRIVATE_DATA->input_voltage;
		AUX_INFO_CURRENT_ITEM->number.value = PRIVATE_DATA->input_current;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
		if (AUX_USB_PORT_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_USB_PORT_1_ITEM->sw.value = PRIVATE_DATA->usb31_1_status;
			AUX_USB_PORT_2_ITEM->sw.value = PRIVATE_DATA->usb31_2_status;
			AUX_USB_PORT_3_ITEM->sw.value = PRIVATE_DATA->usb31_3_status;
			AUX_USB_PORT_4_ITEM->sw.value = PRIVATE_DATA->usb20_1_3_status;
			AUX_USB_PORT_5_ITEM->sw.value = PRIVATE_DATA->usb20_4_6_status;
			indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		}
		if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_1_ITEM->sw.value = PRIVATE_DATA->dc3_4_status;
			AUX_POWER_OUTLET_2_ITEM->sw.value = PRIVATE_DATA->dc8_9_status;
			AUX_POWER_OUTLET_3_ITEM->sw.value = PRIVATE_DATA->dc10_11_status;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.value = PRIVATE_DATA->dc3_4_voltage;
			indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		}
		AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = PRIVATE_DATA->dc2_current;
		AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = PRIVATE_DATA->dc3_4_current;
		indigo_update_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_HEATER_OUTLET_1_ITEM->number.value = (PRIVATE_DATA->dc5_pwm / 255.0) * 100;
			AUX_HEATER_OUTLET_2_ITEM->number.value = (PRIVATE_DATA->dc6_pwm / 255.0) * 100;
			AUX_HEATER_OUTLET_3_ITEM->number.value = (PRIVATE_DATA->dc7_pwm / 255.0) * 100;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}
		if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value && AUX_WEATHER_PROPERTY->state == INDIGO_OK_STATE) {
			if (((AUX_WEATHER_DEWPOINT_ITEM->number.value + 1) > PRIVATE_DATA->dht22_temperature) && (PRIVATE_DATA->dc5_pwm != 255 || PRIVATE_DATA->dc6_pwm != 255 || PRIVATE_DATA->dc7_pwm != 255)) {
				wbprov3_command(device, "5255");
				wbprov3_command(device, "6255");
				wbprov3_command(device, "7255");
				indigo_send_message(device, "Heating started: Aproaching dewpoint");
			}
			if (((AUX_WEATHER_DEWPOINT_ITEM->number.value + 2) < PRIVATE_DATA->dht22_temperature) && (PRIVATE_DATA->dc5_pwm != 0 || PRIVATE_DATA->dc6_pwm != 0 || PRIVATE_DATA->dc7_pwm != 0)) {
				wbprov3_command(device, "5000");
				wbprov3_command(device, "6000");
				wbprov3_command(device, "7000");
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
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->aux_timer);
	//- aux.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = wbprov3_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_outlet_names_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_power_outlet_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_power_outlet_voltage_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_usb_port_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_heater_outlet_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_dew_control_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_x_aux_calibrate_handler_timer);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
		//+ aux.on_disconnect
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- aux.on_disconnect
		wbprov3_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// AUX_OUTLET_NAMES change handler

static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC5) [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s (DC6) [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s (DC7) [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC3-DC4)", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC3-DC4) [V]", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s (DC8-DC9)", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s (DC10-DC11)", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
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
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_POWER_OUTLET change handler

static void aux_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET.on_change
	wbprov3_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "101" : "100");
	wbprov3_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "201" : "200");
	wbprov3_command(device, AUX_POWER_OUTLET_3_ITEM->sw.value ? "211" : "210");
	indigo_sleep(1);
	//- aux.AUX_POWER_OUTLET.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_POWER_OUTLET_VOLTAGE change handler

static void aux_power_outlet_voltage_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_POWER_OUTLET_VOLTAGE.on_change
	char command[16];
	snprintf(command, sizeof(command), "%d", 20000 + (int)(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.target * 10));
	wbprov3_command(device, command);
	indigo_sleep(1);
	//- aux.AUX_POWER_OUTLET_VOLTAGE.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_USB_PORT change handler

static void aux_usb_port_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_USB_PORT.on_change
	wbprov3_command(device, AUX_USB_PORT_1_ITEM->sw.value ? "111" : "110");
	wbprov3_command(device, AUX_USB_PORT_2_ITEM->sw.value ? "121" : "120");
	wbprov3_command(device, AUX_USB_PORT_3_ITEM->sw.value ? "131" : "130");
	wbprov3_command(device, AUX_USB_PORT_4_ITEM->sw.value ? "141" : "140");
	wbprov3_command(device, AUX_USB_PORT_5_ITEM->sw.value ? "151" : "150");
	indigo_sleep(1);
	//- aux.AUX_USB_PORT.on_change
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_HEATER_OUTLET change handler

static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	char command[16];
	snprintf(command, sizeof(command), "%d", 5000 + (int)(AUX_HEATER_OUTLET_1_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	snprintf(command, sizeof(command), "%d", 6000 + (int)(AUX_HEATER_OUTLET_2_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	snprintf(command, sizeof(command), "%d", 7000 + (int)(AUX_HEATER_OUTLET_3_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	indigo_sleep(1);
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// AUX_DEW_CONTROL change handler

static void aux_dew_control_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// X_AUX_CALIBRATE change handler

static void aux_x_aux_calibrate_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	X_AUX_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_CALIBRATE.on_change
	if (X_AUX_CALIBRATE_ITEM->sw.value) {
		wbprov3_command(device, "66300744");
		indigo_sleep(1);
		X_AUX_CALIBRATE_ITEM->sw.value = false;
	}
	//- aux.X_AUX_CALIBRATE.on_change
	indigo_update_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// aux attach API callback

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Regulated outlets (DC3-DC4)", "Regulated outlets");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater outlet (DC5)", "Heater outlet");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater outlet (DC6)", "Heater outlet");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater outlet (DC7)", "Heater outlet");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Power outlets (DC8-DC9)", "Power outlets");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Power outlets (DC10-DC11)", "Power outlets");
		AUX_OUTLET_NAMES_PROPERTY->hidden = false;
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Regulated outlets (DC3-DC4)", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power outlets (DC8-DC9)", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power outlets (DC10-DC11)", true);
		AUX_POWER_OUTLET_PROPERTY->hidden = false;
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_GROUP, "Regulated outlets voltage", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_POWER_OUTLET_VOLTAGE_1_ITEM, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME, "Regulated outlet (DC3-DC4) [V]", 0, 13.2, 0.2, 13.2);
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY->hidden = false;
		AUX_POWER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Output currents", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_POWER_OUTLET_CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Output current (DC2) [A]", 0, 20, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Output current (DC3-DC4) [A]", 0, 20, 0, 0);
		AUX_POWER_OUTLET_CURRENT_PROPERTY->hidden = false;
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB Ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 5);
		if (AUX_USB_PORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "USB 3.1 port (1)", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "USB 3.1 port (2)", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "USB 3.1 port (3)", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "USB 2.0 ports (1-3)", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "USB 2.0 ports (4-6)", true);
		AUX_USB_PORT_PROPERTY->hidden = false;
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater outlet (DC5) [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater outlet (DC6) [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater outlet (DC7) [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_PROPERTY->hidden = false;
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		AUX_DEW_CONTROL_PROPERTY->hidden = false;
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather info (DHT22)", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [°C]", -50, 100, 0, 0);
		AUX_WEATHER_PROPERTY->hidden = false;
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, WEATHER_GROUP, "Temperature sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Temperature (DHT22) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Temperature (Temp probe 1) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM_NAME, "Temperature (Temp probe 2) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM_NAME, "Temperature (Temp probe 3) [°C]", -50, 100, 0, 0);
		AUX_TEMPERATURE_SENSORS_PROPERTY->hidden = false;
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_OK_STATE, 1);
		if (AUX_DEW_WARNING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning (DHT22)", INDIGO_IDLE_STATE);
		AUX_DEW_WARNING_PROPERTY->hidden = false;
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 20, 0, 0);
		AUX_INFO_PROPERTY->hidden = false;
		X_AUX_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_CALIBRATE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Calibrate sensors", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_CALIBRATE_ITEM, X_AUX_CALIBRATE_ITEM_NAME, "Calibrate", false);
		X_AUX_CALIBRATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// aux enumerate API callback

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_POWER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
		indigo_define_matching_property(AUX_USB_PORT_PROPERTY);
		indigo_define_matching_property(AUX_HEATER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_DEW_CONTROL_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
		indigo_define_matching_property(AUX_DEW_WARNING_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
		indigo_define_matching_property(X_AUX_CALIBRATE_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

// aux change property API callback

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_connection_handler, &PRIVATE_DATA->aux_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_outlet_names_handler_timer == NULL) {
			indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
			AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_outlet_names_handler, &PRIVATE_DATA->aux_outlet_names_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_power_outlet_handler_timer == NULL) {
			indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_power_outlet_handler, &PRIVATE_DATA->aux_power_outlet_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_power_outlet_voltage_handler_timer == NULL) {
			indigo_property_copy_values(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property, false);
			AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_power_outlet_voltage_handler, &PRIVATE_DATA->aux_power_outlet_voltage_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_usb_port_handler_timer == NULL) {
			indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
			AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_usb_port_handler, &PRIVATE_DATA->aux_usb_port_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_heater_outlet_handler_timer == NULL) {
			indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_heater_outlet_handler, &PRIVATE_DATA->aux_heater_outlet_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_dew_control_handler_timer == NULL) {
			indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
			AUX_DEW_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_dew_control_handler, &PRIVATE_DATA->aux_dew_control_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_CALIBRATE_PROPERTY, property)) {
		if (PRIVATE_DATA->aux_x_aux_calibrate_handler_timer == NULL) {
			indigo_property_copy_values(X_AUX_CALIBRATE_PROPERTY, property, false);
			X_AUX_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_x_aux_calibrate_handler, &PRIVATE_DATA->aux_x_aux_calibrate_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

// aux detach API callback

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(X_AUX_CALIBRATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

// WandererBox Pro V3 Powerbox driver entry point

indigo_result indigo_aux_wbprov3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wbprov3_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(wbprov3_private_data));
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
