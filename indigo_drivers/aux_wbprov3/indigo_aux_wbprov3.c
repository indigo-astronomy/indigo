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

/** INDIGO WandererBox Pro V3 aux driver
 \file indigo_aux_wbprov3.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_aux_wbprov3"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/termios.h>


#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_aux_wbprov3.h"

#define PRIVATE_DATA	((ppb_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY	(PRIVATE_DATA->outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_1_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_NAME_2_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_HEATER_OUTLET_NAME_3_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_NAME_2_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_NAME_3_ITEM (AUX_OUTLET_NAMES_PROPERTY->items + 5)

#define AUX_POWER_OUTLET_PROPERTY	(PRIVATE_DATA->power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM		(AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM		(AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM		(AUX_POWER_OUTLET_PROPERTY->items + 2)

#define AUX_POWER_OUTLET_VOLTAGE_PROPERTY	(PRIVATE_DATA->power_outlet_voltage_property)
#define AUX_POWER_OUTLET_VOLTAGE_1_ITEM		(AUX_POWER_OUTLET_VOLTAGE_PROPERTY->items + 0)

#define AUX_POWER_OUTLET_CURRENT_PROPERTY	(PRIVATE_DATA->power_outlet_current_property)			
#define AUX_POWER_OUTLET_CURRENT_1_ITEM		(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 0)		
#define AUX_POWER_OUTLET_CURRENT_2_ITEM		(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 1)

#define AUX_HEATER_OUTLET_PROPERTY	(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM	(AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM	(AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM	(AUX_HEATER_OUTLET_PROPERTY->items + 2)

#define AUX_USB_PORT_PROPERTY		(PRIVATE_DATA->usb_ports_proeprty)
#define AUX_USB_PORT_1_ITEM			(AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM			(AUX_USB_PORT_PROPERTY->items + 1)
#define AUX_USB_PORT_3_ITEM			(AUX_USB_PORT_PROPERTY->items + 2)
#define AUX_USB_PORT_4_ITEM			(AUX_USB_PORT_PROPERTY->items + 3)
#define AUX_USB_PORT_5_ITEM			(AUX_USB_PORT_PROPERTY->items + 4)

#define AUX_WEATHER_PROPERTY		(PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM	(AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM	(AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM	(AUX_WEATHER_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY	(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM	(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM	(AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_TEMPERATURE_SENSORS_PROPERTY		(PRIVATE_DATA->temperature_sensors_property)
#define AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)
#define AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 2)
#define AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 3)

#define AUX_DEW_WARNING_PROPERTY			(PRIVATE_DATA->dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM		(AUX_DEW_WARNING_PROPERTY->items + 0)

#define AUX_INFO_PROPERTY	(PRIVATE_DATA->info_property)
#define AUX_INFO_VOLTAGE_ITEM	(AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM	(AUX_INFO_PROPERTY->items + 1)

#define X_AUX_CALIBRATE_PROPERTY	(PRIVATE_DATA->calibrate_property)
#define X_AUX_CALIBRATE_ITEM	(X_AUX_CALIBRATE_PROPERTY->items + 0)

#define AUX_GROUP	"Powerbox"
#define WEATHER_GROUP	"Weather"

#define DEVICE_ID "ZXWBProV3"

typedef struct {
	int handle;
	indigo_timer *aux_timer;
	indigo_property *outlet_names_property;
	indigo_property *power_outlet_property;
	indigo_property *heater_outlet_property;
	indigo_property *heating_mode_property;
	indigo_property *weather_property;
	indigo_property *info_property;
	indigo_property *calibrate_property;
	indigo_property *usb_ports_proeprty;
	indigo_property *power_outlet_voltage_property;
	indigo_property *power_outlet_current_property;
	indigo_property *temperature_sensors_property;
	indigo_property *dew_warning_property;
	pthread_mutex_t mutex;
} ppb_private_data;

typedef struct {
	char model_id[50];
	char firmware[20];
	float probe1_temperature;
	float probe2_temperature;
	float probe3_temperature;
	float dht22_hunidity;
	float dht22_temperature;
	float input_current;
	float dc2_current;
	float dc3_4_current;
	float input_voltage;
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
	float dc3_4_voltage;
} wbprov3_status_t;

int wbprov3_parse_status(char *status_line, wbprov3_status_t *status) {
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
	status->probe1_temperature = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->probe2_temperature = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->probe3_temperature = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	if (!strcmp(token, "nan")) {
		status->dht22_hunidity = -127.0;
	} else {
		status->dht22_hunidity = atof(token);
	}
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	if (!strcmp(token, "nan")) {
		status->dht22_temperature = -127.0;
	} else {
		status->dht22_temperature = atof(token);
	}

	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->input_current = atof(token);
	
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc2_current = atof(token);
	
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc3_4_current = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->input_voltage = atof(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->usb31_1_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->usb31_2_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->usb31_3_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->usb20_1_3_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->usb20_4_6_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc3_4_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc5_pwm = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc6_pwm = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc7_pwm = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc8_9_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc10_11_status = atoi(token);
	
	token = strtok_r(NULL, "A", &buf);
	if (token == NULL) {
		return false;
	}
	status->dc3_4_voltage = atof(token)/10.0;
	
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "model_id = '%s'\nfirmware = '%s'\nprobe1_temperature = %.2fC\nprobe2_temperature = %.2fC\nprobe3_temperature = %.2fC\ndht22_hunidity = %.2f%%\ndht22_temperature = %.2fC\ninput_current = %.2fA\ndc2_current = %.2fA\ndc3_4_current = %.2fA\ninput_voltage = %.2fV\nusb31_1_status = %d\nusb31_2_status = %d\nusb31_3_status = %d\nusb20_1_3_status = %d\nusb20_4_6_status = %d\ndc3_4_status = %d\ndc5_pwm = %d\ndc6_pwm = %d\ndc7_pwm = %d\ndc8_9_status = %d\ndc10_11_status = %d\ndc3_4_voltage = %.1fV\n",
		status->model_id,
		status->firmware,
		status->probe1_temperature,
		status->probe2_temperature,
		status->probe3_temperature,
		status->dht22_hunidity,
		status->dht22_temperature,
		status->input_current,
		status->dc2_current,
		status->dc3_4_current,
		status->input_voltage,
		status->usb31_1_status,
		status->usb31_2_status,
		status->usb31_3_status,
		status->usb20_1_3_status,
		status->usb20_4_6_status,
		status->dc3_4_status,
		status->dc5_pwm,
		status->dc6_pwm,
		status->dc7_pwm,
		status->dc8_9_status,
		status->dc10_11_status,
		status->dc3_4_voltage
	); 
	return true;
}

static bool wbprov3_read_status(indigo_device *device, wbprov3_status_t *wb_stat) {
	char status[256] = {0};
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	int res = indigo_read_line(PRIVATE_DATA->handle, status, 256);
	if (strncmp(status, DEVICE_ID, strlen(DEVICE_ID))) {   // first part of the message is cleared by tcflush();
		res = indigo_read_line(PRIVATE_DATA->handle, status, 256);
	}
	if (res == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No status report");
		return false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Read: \"%s\" %d", status, res);
		return wbprov3_parse_status(status, wb_stat);
	}
}

// -------------------------------------------------------------------------------- Low level communication routines

static bool wbprov3_command(indigo_device *device, char *command) {
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
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Regulated outlets (DC3-DC4)", "Regulated outlets");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater outlet (DC5)", "Heater outlet");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater outlet (DC6)", "Heater outlet");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater outlet (DC7)", "Heater outlet");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Power outlets (DC8-DC9)", "Power outlets");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Power outlets (DC10-DC11)", "Power outlets");
		// -------------------------------------------------------------------------------- POWER_OUTLET
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (AUX_POWER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Regulated outlets (DC3-DC4)", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power outlets (DC8-DC9)", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power outlets (DC10-DC11)", true);
		// -------------------------------------------------------------------------------- POWER_OUTLET_VOLTAGE
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_GROUP, "Regulated outlets voltage", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_POWER_OUTLET_VOLTAGE_1_ITEM, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME, "Regulated outlet (DC3-DC4) [V]", 0, 13.2, 0.2, 13.2);
		// -------------------------------------------------------------------------------- POWER_OUTLET_VOLTAGE
		AUX_POWER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Output currents", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_POWER_OUTLET_CURRENT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Output current (DC2) [A]", 0, 20, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Output current (DC3-DC4) [A]", 0, 20, 0, 0);
		// -------------------------------------------------------------------------------- USB_PORT
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB Ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 5);
		if (AUX_USB_PORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "USB 3.1 port (1)", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "USB 3.1 port (2)", true);
		indigo_init_switch_item(AUX_USB_PORT_3_ITEM, AUX_USB_PORT_3_ITEM_NAME, "USB 3.1 port (3)", true);
		indigo_init_switch_item(AUX_USB_PORT_4_ITEM, AUX_USB_PORT_4_ITEM_NAME, "USB 2.0 ports (1-3)", true);
		indigo_init_switch_item(AUX_USB_PORT_5_ITEM, AUX_USB_PORT_5_ITEM_NAME, "USB 2.0 ports (4-6)", true);
		// -------------------------------------------------------------------------------- HEATER_OUTLET
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater outlet (DC5) [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater outlet (DC6) [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater outlet (DC7) [%]", 0, 100, 5, 0);
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		//AUX_DEW_CONTROL_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather info (DHT22)", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [°C]", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- AUX_TEMPERATURE_SENSORS
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, WEATHER_GROUP, "Temperature sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Temperature (DHT22) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Temperature (Temp probe 1) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM_NAME, "Temperature (Temp probe 2) [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM_NAME, "Temperature (Temp probe 3) [°C]", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- DEW_WARNING
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_IDLE_STATE, 1);
		if (AUX_DEW_WARNING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning (DHT22)", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 20, 0, 0);
		// -------------------------------------------------------------------------------- X_AUX_CALIBRATE
		X_AUX_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_AUX_CALIBRATE", "Advanced", "Calibrate sensors", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_AUX_CALIBRATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_AUX_CALIBRATE_ITEM, "CALIBRATE", "Calibrate", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_POWER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		if (indigo_property_match(AUX_POWER_OUTLET_CURRENT_PROPERTY, property))
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		if (indigo_property_match(AUX_USB_PORT_PROPERTY, property))
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_TEMPERATURE_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		if (indigo_property_match(AUX_INFO_PROPERTY, property))
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
		if (indigo_property_match(X_AUX_CALIBRATE_PROPERTY, property))
			indigo_define_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_update_states(indigo_device *device) {
	wbprov3_status_t wb_stat;
	if (wbprov3_read_status(device, &wb_stat)) {
		AUX_WEATHER_TEMPERATURE_ITEM->number.value = wb_stat.dht22_temperature;
		AUX_WEATHER_HUMIDITY_ITEM->number.value = wb_stat.dht22_hunidity;
		// calculate dew point
		if(wb_stat.dht22_temperature > -100) {
			AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
			AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_aux_dewpoint(wb_stat.dht22_temperature, wb_stat.dht22_hunidity);
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_IDLE_STATE;
			AUX_WEATHER_DEWPOINT_ITEM->number.value = -273.15;
		} 
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);

		AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM->number.value = wb_stat.dht22_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM->number.value = wb_stat.probe1_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM->number.value = wb_stat.probe2_temperature;
		AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM->number.value = wb_stat.probe3_temperature;

		if (wb_stat.dht22_temperature < -100 && wb_stat.probe1_temperature <-100 && wb_stat.probe2_temperature && wb_stat.probe3_temperature) {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_IDLE_STATE;
		} else {
			AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);

		AUX_INFO_VOLTAGE_ITEM->number.value = wb_stat.input_voltage;
		AUX_INFO_CURRENT_ITEM->number.value = wb_stat.input_current;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);

		if (AUX_USB_PORT_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_USB_PORT_1_ITEM->sw.value = wb_stat.usb31_1_status;
			AUX_USB_PORT_2_ITEM->sw.value = wb_stat.usb31_2_status;
			AUX_USB_PORT_3_ITEM->sw.value = wb_stat.usb31_3_status;
			AUX_USB_PORT_4_ITEM->sw.value = wb_stat.usb20_1_3_status;
			AUX_USB_PORT_5_ITEM->sw.value = wb_stat.usb20_4_6_status;
			indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		}

		if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_1_ITEM->sw.value = wb_stat.dc3_4_status;
			AUX_POWER_OUTLET_2_ITEM->sw.value = wb_stat.dc8_9_status;
			AUX_POWER_OUTLET_3_ITEM->sw.value = wb_stat.dc10_11_status;
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}

		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.value = wb_stat.dc3_4_voltage;
			indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		}

		AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = wb_stat.dc2_current;
		AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = wb_stat.dc3_4_current;
		indigo_update_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);

		if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_HEATER_OUTLET_1_ITEM->number.value = (wb_stat.dc5_pwm / 255.0) * 100;
			AUX_HEATER_OUTLET_2_ITEM->number.value = (wb_stat.dc6_pwm / 255.0) * 100;
			AUX_HEATER_OUTLET_3_ITEM->number.value = (wb_stat.dc7_pwm / 255.0) * 100;
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}

		if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value && AUX_WEATHER_PROPERTY->state == INDIGO_OK_STATE) {
			if (
				((AUX_WEATHER_DEWPOINT_ITEM->number.value + 1) > wb_stat.dht22_temperature) &&
				(wb_stat.dc5_pwm != 255 || wb_stat.dc6_pwm != 255 || wb_stat.dc7_pwm != 255)
			) {
				wbprov3_command(device, "5255");
				wbprov3_command(device, "6255");
				wbprov3_command(device, "7255");
				indigo_send_message(device, "Heating started: Aproaching dewpoint");
			}

			if (
				((AUX_WEATHER_DEWPOINT_ITEM->number.value + 2) < wb_stat.dht22_temperature) &&
				(wb_stat.dc5_pwm != 0 || wb_stat.dc6_pwm != 0 || wb_stat.dc7_pwm != 0)
			) {
				wbprov3_command(device, "5000");
				wbprov3_command(device, "6000");
				wbprov3_command(device, "7000");
				indigo_send_message(device, "Heating stopped: Conditions are dry");
			}
		}

		// Dew point warning
		if (wb_stat.dht22_temperature < -100) { // no sensor
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_IDLE_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;
		} else if (wb_stat.dht22_temperature - 1 <= AUX_WEATHER_DEWPOINT_ITEM->number.value) {
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
		} else {
			AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
			AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
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

static void aux_connection_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			indigo_usleep(ONE_SECOND_DELAY);
			wbprov3_status_t wb_stat;
			if (wbprov3_read_status(device, &wb_stat)) {
				if (!strcmp(wb_stat.model_id, DEVICE_ID)) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, wb_stat.model_id);
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, wb_stat.firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device is not WandererBox Pro V3");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device is not WandererBox Pro V3 1");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
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
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
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
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);

		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	wbprov3_command(device, AUX_POWER_OUTLET_1_ITEM->sw.value ? "101" : "100");
	wbprov3_command(device, AUX_POWER_OUTLET_2_ITEM->sw.value ? "201" : "200");
	wbprov3_command(device, AUX_POWER_OUTLET_3_ITEM->sw.value ? "211" : "210");
	indigo_usleep(ONE_SECOND_DELAY);
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_usb_port_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	wbprov3_command(device, AUX_USB_PORT_1_ITEM->sw.value ? "111" : "110");
	wbprov3_command(device, AUX_USB_PORT_2_ITEM->sw.value ? "121" : "120");
	wbprov3_command(device, AUX_USB_PORT_3_ITEM->sw.value ? "131" : "130");
	wbprov3_command(device, AUX_USB_PORT_4_ITEM->sw.value ? "141" : "140");
	wbprov3_command(device, AUX_USB_PORT_5_ITEM->sw.value ? "151" : "150");
	indigo_usleep(ONE_SECOND_DELAY);
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_power_outlet_voltage_handler(indigo_device *device) {
	char command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	sprintf(command, "%d", 20000 + (int)(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.target * 10));
	wbprov3_command(device, command);
	indigo_usleep(ONE_SECOND_DELAY);
	AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	char command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	sprintf(command, "%d", 5000 + (int)(AUX_HEATER_OUTLET_1_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	sprintf(command, "%d", 6000 + (int)(AUX_HEATER_OUTLET_2_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	sprintf(command, "%d", 7000 + (int)(AUX_HEATER_OUTLET_3_ITEM->number.target * 255.0 / 100.0));
	wbprov3_command(device, command);
	indigo_usleep(ONE_SECOND_DELAY);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_calibrate_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_AUX_CALIBRATE_ITEM->sw.value) {
		wbprov3_command(device, "66300744");
		indigo_usleep(ONE_SECOND_DELAY);
		X_AUX_CALIBRATE_ITEM->sw.value = false;
		X_AUX_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_AUX_CALIBRATE_PROPERTY, "Seensors recallibrated");
	}
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
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC5) [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s (DC6) [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s (DC7) [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC3-DC4)", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->label, INDIGO_NAME_SIZE, "%s (DC3-DC4) [V]", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s (DC8-DC9)", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s (DC10-DC11)", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
		
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET_VOLTAGE
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property, false);
		indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_voltage_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_USB_PORT_OUTLET
		AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_usb_port_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_heater_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_CALIBRATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_CALIBRATE
		indigo_property_copy_values(X_AUX_CALIBRATE_PROPERTY, property, false);
		X_AUX_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_AUX_CALIBRATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_calibrate_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
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
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_wbprov3(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static ppb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"WandererBox Pro V3",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, "WandererBox Pro V3 Powerbox", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(ppb_private_data));
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
