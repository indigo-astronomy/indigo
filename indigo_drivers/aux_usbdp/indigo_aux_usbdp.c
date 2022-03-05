// Copyright (c) 2019 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumen@skyarchive.org>

/** INDIGO USB_Dewpoint aux driver
 \file indigo_aux_usbdp.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_aux_usbdp"

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

#include "indigo_aux_usbdp.h"

#define PRIVATE_DATA								((usbdp_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY					(PRIVATE_DATA->outlet_names_property)
#define AUX_HEATER_OUTLET_NAME_1_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_2_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_NAME_3_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_PROPERTY					(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM					(AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM					(AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM					(AUX_HEATER_OUTLET_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_STATE_PROPERTY		(PRIVATE_DATA->heater_outlet_state_property)
#define AUX_HEATER_OUTLET_STATE_1_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_STATE_2_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_STATE_3_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 2)

#define AUX_WEATHER_PROPERTY					(PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM			(AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM				(AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM				(AUX_WEATHER_PROPERTY->items + 2)

#define AUX_TEMPERATURE_SENSORS_PROPERTY		(PRIVATE_DATA->temperature_sensors_property)
#define AUX_TEMPERATURE_SENSOR_1_ITEM			(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSOR_2_ITEM			(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)

#define AUX_CALLIBRATION_PROPERTY				(PRIVATE_DATA->temperature_callibration_property)
#define AUX_CALLIBRATION_SENSOR_1_ITEM			(AUX_CALLIBRATION_PROPERTY->items + 0)
#define AUX_CALLIBRATION_SENSOR_2_ITEM			(AUX_CALLIBRATION_PROPERTY->items + 1)
#define AUX_CALLIBRATION_SENSOR_3_ITEM			(AUX_CALLIBRATION_PROPERTY->items + 2)
#define AUX_CALLIBRATION_PROPERTY_NAME			"AUX_TEMPERATURE_CALLIBRATION"
#define AUX_CALLIBRATION_SENSOR_1_ITEM_NAME		"SENSOR_1"
#define AUX_CALLIBRATION_SENSOR_2_ITEM_NAME		"SENSOR_2"
#define AUX_CALLIBRATION_SENSOR_3_ITEM_NAME		"SENSOR_3"

#define AUX_DEW_THRESHOLD_PROPERTY				(PRIVATE_DATA->dew_threshold_property)
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM			(AUX_DEW_THRESHOLD_PROPERTY->items + 0)
#define AUX_DEW_THRESHOLD_SENSOR_2_ITEM			(AUX_DEW_THRESHOLD_PROPERTY->items + 1)

#define AUX_HEATER_AGGRESSIVITY_PROPERTY		(PRIVATE_DATA->heater_aggressivity_property)
#define AUX_HEATER_AGGRESSIVITY_1_ITEM			(AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 0)
#define AUX_HEATER_AGGRESSIVITY_2_ITEM			(AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 1)
#define AUX_HEATER_AGGRESSIVITY_5_ITEM			(AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 2)
#define AUX_HEATER_AGGRESSIVITY_10_ITEM			(AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 3)
#define AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME	"AUX_HEATER_AGGRESSIVITY"
#define AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME		"AGGRESSIVITY_1"
#define AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME		"AGGRESSIVITY_2"
#define AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME		"AGGRESSIVITY_5"
#define AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME	"AGGRESSIVITY_10"

#define AUX_LINK_CH_2AND3_PROPERTY					(PRIVATE_DATA->link_channels_23_property)
#define AUX_LINK_CH_2AND3_LINKED_ITEM				(AUX_LINK_CH_2AND3_PROPERTY->items + 0)
#define AUX_LINK_CH_2AND3_NOT_LINKED_ITEM			(AUX_LINK_CH_2AND3_PROPERTY->items + 1)
#define AUX_LINK_CH_2AND3_PROPERTY_NAME				"AUX_LINK_CHANNELS_2AND3"
#define AUX_LINK_CH_2AND3_LINKED_ITEM_NAME			"LINKED"
#define AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME		"NOT_LINKED"

#define AUX_DEW_WARNING_PROPERTY				(PRIVATE_DATA->dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM			(AUX_DEW_WARNING_PROPERTY->items+0)
#define AUX_DEW_WARNING_SENSOR_2_ITEM			(AUX_DEW_WARNING_PROPERTY->items+1)
#define AUX_DEW_WARNING_SENSOR_3_ITEM			(AUX_DEW_WARNING_PROPERTY->items+2)

#define AUX_DEW_CONTROL_PROPERTY				(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM				(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM			(AUX_DEW_CONTROL_PROPERTY->items + 1)


#define AUX_GROUP															"Auxiliary"
#define SETTINGS_GROUP														"Settings"

typedef struct {
	int handle;
	uint8_t requested_aggressivity;
	indigo_timer *aux_timer;
	indigo_property *outlet_names_property;
	indigo_property *heater_outlet_property;
	indigo_property *heater_outlet_state_property;
	indigo_property *heating_mode_property;
	indigo_property *weather_property;
	indigo_property *temperature_sensors_property;
	indigo_property *dew_warning_property;
	indigo_property *heater_aggressivity_property;
	indigo_property *link_channels_23_property;
	indigo_property *dew_threshold_property;
	indigo_property *temperature_callibration_property;
	int version;
	pthread_mutex_t mutex;
} usbdp_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

#define UDP_CMD_LEN 6

#define UDP_STATUS_CMD "SGETAL"
#define UDP1_STATUS_RESPONSE "Tloc=%f-Tamb=%f-RH=%f-DP=%f-TH=%d-C=%d"
#define UDP2_STATUS_RESPONSE "##%f/%f/%f/%f/%f/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u**"

#define UDP_IDENTIFY_CMD "SWHOIS"
#define UDP1_IDENTIFY_RESPONSE "UDP"
#define UDP2_IDENTIFY_RESPONSE "UDP2(%u)" // Firmware version? Like "UDP2(1446)"

#define UDP_RESET_CMD "SEERAZ"
#define UDP_RESET_RESPONSE "EEPROM RESET"

#define UDP2_OUTPUT_CMD "S%1uO%03u"         // channel 1-3, power 0-100
#define UDP2_THRESHOLD_CMD "STHR%1u%1u"     // channel 1-2, value 0-9
#define UDP2_CALIBRATION_CMD "SCA%1u%1u%1u" // channel 1-2-Amb, value 0-9
#define UDP2_LINK_CMD "SLINK%1u"            // 0 or 1 to link channels 2 and 3
#define UDP2_AUTO_CMD "SAUTO%1u"            // 0 or 1 to enable auto mode
#define UDP2_AGGRESSIVITY_CMD "SAGGR%1u"    // 1-4 (1, 2, 5, 10)
#define UDP_DONE_RESPONSE  "DONE"

typedef struct {
	float temp_loc;
	float temp_amb;
	float rh;
	float dewpoint;
	int threshold;
	int c;
} usbdp_status_v1_t;

typedef struct {
	float temp_ch1;
	float temp_ch2;
	float temp_amb;
	float rh;
	float dewpoint;
	int  output_ch1;
	int  output_ch2;
	int  output_ch3;
	int  cal_ch1;
	int  cal_ch2;
	int  cal_amb;
	int  threshold_ch1;
	int  threshold_ch2;
	int  auto_mode;
	int  ch2_3_linked;
	int  aggressivity;
} usbdp_status_v2_t;

typedef struct {
	char version;
	union {
		usbdp_status_v1_t v1;
		usbdp_status_v2_t v2;
	};
} usbdp_status_t;


static bool usbdp_command(indigo_device *device, char *command, char *response, int max) {
	/* Wait a bit before flushing as usb to serial caches data */
	indigo_usleep(20000);
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));

	if (response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) == -1) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Command %s -> no response", command);
			return false;
		}
	}

	INDIGO_DRIVER_LOG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


static bool usbdp_status(indigo_device *device, usbdp_status_t *status) {
	char response[80];
	if (!usbdp_command(device, UDP_STATUS_CMD, response, sizeof(response))) {
		return false;
	}

	status->version = PRIVATE_DATA->version;

	if (status->version == 1) {
		int parsed = sscanf(response, UDP1_STATUS_RESPONSE,
			&status->v1.temp_loc,
			&status->v1.temp_amb,
			&status->v1.rh,
			&status->v1.dewpoint,
			&status->v1.threshold,
			&status->v1.c
		);
		if (parsed == 6) {
			status->version = PRIVATE_DATA->version;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Tloc=%f Tamb=%f RH=%f DP=%f TH=%d C=%d", status->v1.temp_loc, status->v1.temp_amb, status->v1.rh, status->v1.dewpoint, status->v1.threshold, status->v1.c);
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error: parsed %d of 6 values in response \"%s\"", parsed, response);
			return false;
		}
	} else if (status->version == 2) {
		int parsed = sscanf(response, UDP2_STATUS_RESPONSE,
			&status->v2.temp_ch1,
			&status->v2.temp_ch2,
			&status->v2.temp_amb,
			&status->v2.rh,
			&status->v2.dewpoint,
			&status->v2.output_ch1,
			&status->v2.output_ch2,
			&status->v2.output_ch3,
			&status->v2.cal_ch1,
			&status->v2.cal_ch2,
			&status->v2.cal_amb,
			&status->v2.threshold_ch1,
			&status->v2.threshold_ch2,
			&status->v2.auto_mode,
			&status->v2.ch2_3_linked,
			&status->v2.aggressivity
		);
		if (parsed == 16) {
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error: parsed %d of 16 values in response \"%s\"", parsed, response);
			return false;
		}

	} else {
		return false;
	}
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, "X_AUX_OUTLET_NAMES", SETTINGS_GROUP, "Outlet/Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater/Sensor #1", "Heater/Sensor #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater/Sensor #2", "Heater/Sensor #2");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater #3", "Heater #3");
		// -------------------------------------------------------------------------------- HEATER OUTLETS
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Heater outlets state", INDIGO_OK_STATE, 3);
		if (AUX_HEATER_OUTLET_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1", INDIGO_IDLE_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2", INDIGO_IDLE_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_3_ITEM, AUX_HEATER_OUTLET_STATE_3_ITEM_NAME, "Heater #3", INDIGO_IDLE_STATE);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient Temperature (°C)", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint (°C)", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- SENSORS
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_GROUP, "Temperature Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_TEMPERATURE_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", -50, 100, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", -50, 100, 0, 0);

		// -------------------------------------------------------------------------------- SENSOR_CALLIBRATION
		AUX_CALLIBRATION_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_CALLIBRATION_PROPERTY_NAME, SETTINGS_GROUP, "Temperature Sensors Callibration", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_CALLIBRATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_1_ITEM, AUX_CALLIBRATION_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_2_ITEM, AUX_CALLIBRATION_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_3_ITEM, AUX_CALLIBRATION_SENSOR_3_ITEM_NAME, "Ambient sensor (°C)", 0, 9, 1, 0);
		// -------------------------------------------------------------------------------- DEW_THRESHOLD
		AUX_DEW_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_DEW_THRESHOLD_PROPERTY_NAME, SETTINGS_GROUP, "Dew Thresholds", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_DEW_THRESHOLD_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_1_ITEM, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_2_ITEM, AUX_DEW_THRESHOLD_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", 0, 9, 1, 0);
		// -------------------------------------------------------------------------------- LINK_CHAANNELS_2AND3
		AUX_LINK_CH_2AND3_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LINK_CH_2AND3_PROPERTY_NAME, SETTINGS_GROUP, "Link chanels 2 and 3", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LINK_CH_2AND3_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_LINK_CH_2AND3_LINKED_ITEM, AUX_LINK_CH_2AND3_LINKED_ITEM_NAME, "Linked", false);
		indigo_init_switch_item(AUX_LINK_CH_2AND3_NOT_LINKED_ITEM, AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME, "Not Linked", true);
		// -------------------------------------------------------------------------------- HEATER_AGGRESSIVITY
		AUX_HEATER_AGGRESSIVITY_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, SETTINGS_GROUP, "Auto mode heater aggressivity", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AUX_HEATER_AGGRESSIVITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_1_ITEM, AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME, "Aggressivity 1%", true);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_2_ITEM, AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME, "Aggressivity 2%", false);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_5_ITEM, AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME, "Aggressivity 5%", false);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_10_ITEM, AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME, "Aggressivity 10%", false);
		// -------------------------------------------------------------------------------- DEW_WARNING
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, AUX_GROUP, "Dew warning", INDIGO_OK_STATE, 2);
		if (AUX_DEW_WARNING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Sensor #1", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_2_ITEM, AUX_DEW_WARNING_SENSOR_2_ITEM_NAME, "Sesnor #2", INDIGO_OK_STATE);
		//indigo_init_light_item(AUX_DEW_WARNING_SENSOR_3_ITEM, AUX_DEW_WARNING_SENSOR_3_ITEM_NAME, "Ambient", INDIGO_OK_STATE);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyACM0");
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
		if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_STATE_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_TEMPERATURE_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		if (indigo_property_match(AUX_CALLIBRATION_PROPERTY, property))
			indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		if (indigo_property_match(AUX_LINK_CH_2AND3_PROPERTY, property))
			indigo_define_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_AGGRESSIVITY_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;

	//char response[128];
	bool updateHeaterOutlet = false;
	bool updateHeaterOutletState = false;
	bool updateWeather = false;
	bool updateSensors = false;
	bool updateAutoHeater = false;
	bool updateCallibration = false;
	bool updateThreshold = false;
	bool updateAggressivity = false;
	bool updateLinked = false;
	int channel_1_state;
	int channel_2_state;
	int channel_3_state;

	int dew_warning_1;
	int dew_warning_2;

	usbdp_status_t status;

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (usbdp_status(device, &status)) {
		if (status.version == 1) {
			if ((fabs(((double)status.v1.temp_amb - AUX_WEATHER_TEMPERATURE_ITEM->number.value)*100) >= 1) ||
			    (fabs(((double)status.v1.rh - AUX_WEATHER_HUMIDITY_ITEM->number.value)*100) >= 1) ||
			    (fabs(((double)status.v1.dewpoint - AUX_WEATHER_DEWPOINT_ITEM->number.value)*100) >= 1)) {
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = status.v1.temp_amb;
				AUX_WEATHER_HUMIDITY_ITEM->number.value = status.v1.rh;
				AUX_WEATHER_DEWPOINT_ITEM->number.value = status.v1.dewpoint;
				updateWeather = true;
			}

			if (fabs(((double)status.v1.temp_loc - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1) {
				AUX_TEMPERATURE_SENSOR_1_ITEM->number.value = status.v1.temp_loc;
				updateSensors = true;
			}
		} else if (status.version == 2) {
			if ((fabs(((double)status.v2.temp_amb - AUX_WEATHER_TEMPERATURE_ITEM->number.value)*100) >= 1) ||
			    (fabs(((double)status.v2.rh - AUX_WEATHER_HUMIDITY_ITEM->number.value)*100) >= 1) ||
			    (fabs(((double)status.v2.dewpoint - AUX_WEATHER_DEWPOINT_ITEM->number.value)*100) >= 1)) {
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = status.v2.temp_amb;
				AUX_WEATHER_HUMIDITY_ITEM->number.value = status.v2.rh;
				AUX_WEATHER_DEWPOINT_ITEM->number.value = status.v2.dewpoint;
				updateWeather = true;
			}

			if ((fabs(((double)status.v2.temp_ch1 - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1) ||
			    (fabs(((double)status.v2.temp_ch1 - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1)) {
				AUX_TEMPERATURE_SENSOR_1_ITEM->number.value = status.v2.temp_ch1;
				AUX_TEMPERATURE_SENSOR_2_ITEM->number.value = status.v2.temp_ch2;
				updateSensors = true;
			}

			if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value != status.v2.auto_mode) {
				if (status.v2.auto_mode) {
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
				} else {
					indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
				}
				updateAutoHeater = true;
			}

			if (((int)(AUX_HEATER_OUTLET_1_ITEM->number.value) != status.v2.output_ch1) ||
			    ((int)(AUX_HEATER_OUTLET_2_ITEM->number.value) != status.v2.output_ch2) ||
			    ((int)(AUX_HEATER_OUTLET_3_ITEM->number.value) != status.v2.output_ch3)) {
				AUX_HEATER_OUTLET_1_ITEM->number.value = status.v2.output_ch1;
				AUX_HEATER_OUTLET_2_ITEM->number.value = status.v2.output_ch2;
				AUX_HEATER_OUTLET_3_ITEM->number.value = status.v2.output_ch3;
				updateHeaterOutlet = true;
			}

			// OUTLET STATUS UPDATE
			if ((bool)status.v2.output_ch1 && status.v2.auto_mode) {
				channel_1_state = INDIGO_BUSY_STATE;
			} else if ((bool)status.v2.output_ch1) {
				channel_1_state = INDIGO_OK_STATE;
			} else {
				channel_1_state = INDIGO_IDLE_STATE;
			}

			if ((bool)status.v2.output_ch2 && status.v2.auto_mode) {
				channel_2_state = INDIGO_BUSY_STATE;
			} else if ((bool)status.v2.output_ch2) {
				channel_2_state = INDIGO_OK_STATE;
			} else {
				channel_2_state = INDIGO_IDLE_STATE;
			}

			if ((bool)status.v2.output_ch3 && status.v2.auto_mode && status.v2.ch2_3_linked) {
				channel_3_state = INDIGO_BUSY_STATE;
			} else if ((bool)status.v2.output_ch3) {
				channel_3_state = INDIGO_OK_STATE;
			} else {
				channel_3_state = INDIGO_IDLE_STATE;
			}

			if ((AUX_HEATER_OUTLET_STATE_1_ITEM->light.value != channel_1_state) ||
			    (AUX_HEATER_OUTLET_STATE_2_ITEM->light.value != channel_2_state) ||
			    (AUX_HEATER_OUTLET_STATE_3_ITEM->light.value != channel_3_state)) {
				AUX_HEATER_OUTLET_STATE_1_ITEM->light.value = channel_1_state;
				AUX_HEATER_OUTLET_STATE_2_ITEM->light.value = channel_2_state;
				AUX_HEATER_OUTLET_STATE_3_ITEM->light.value = channel_3_state;
				updateHeaterOutletState = true;
			}

			if (((int)(AUX_CALLIBRATION_SENSOR_1_ITEM->number.value) != status.v2.cal_ch1) ||
			    ((int)(AUX_CALLIBRATION_SENSOR_2_ITEM->number.value) != status.v2.cal_ch2) ||
			    ((int)(AUX_CALLIBRATION_SENSOR_3_ITEM->number.value) != status.v2.cal_amb)) {
				AUX_CALLIBRATION_SENSOR_1_ITEM->number.value = status.v2.cal_ch1;
				AUX_CALLIBRATION_SENSOR_2_ITEM->number.value = status.v2.cal_ch2;
				AUX_CALLIBRATION_SENSOR_3_ITEM->number.value = status.v2.cal_amb;
				updateCallibration = true;
			}

			if (((int)(AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value) != status.v2.threshold_ch1) ||
			    ((int)(AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value) != status.v2.threshold_ch2)) {
				AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value = status.v2.threshold_ch1;
				AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value = status.v2.threshold_ch2;
				updateThreshold = true;
			}

			if (PRIVATE_DATA->requested_aggressivity != status.v2.aggressivity) {
				switch (status.v2.aggressivity) {
				case(1):
					indigo_set_switch(AUX_HEATER_AGGRESSIVITY_PROPERTY, AUX_HEATER_AGGRESSIVITY_1_ITEM, true);
					break;
				case(2):
					indigo_set_switch(AUX_HEATER_AGGRESSIVITY_PROPERTY, AUX_HEATER_AGGRESSIVITY_2_ITEM, true);
					break;
				case(3):
					indigo_set_switch(AUX_HEATER_AGGRESSIVITY_PROPERTY, AUX_HEATER_AGGRESSIVITY_5_ITEM, true);
					break;
				case(4):
					indigo_set_switch(AUX_HEATER_AGGRESSIVITY_PROPERTY, AUX_HEATER_AGGRESSIVITY_10_ITEM, true);
					break;
				}
				PRIVATE_DATA->requested_aggressivity = status.v2.aggressivity;
				updateAggressivity = true;
			}

			if (AUX_LINK_CH_2AND3_LINKED_ITEM->sw.value != status.v2.ch2_3_linked) {
				if (status.v2.ch2_3_linked) {
					indigo_set_switch(AUX_LINK_CH_2AND3_PROPERTY, AUX_LINK_CH_2AND3_LINKED_ITEM, true);
				} else {
					indigo_set_switch(AUX_LINK_CH_2AND3_PROPERTY, AUX_LINK_CH_2AND3_NOT_LINKED_ITEM, true);
				}
				updateLinked = true;
			}
		}
	}

	/* Dew warning is issued when:
		"Temp1 + Calibration1 <= Dewpoint + Threshold1"
		"Temp2 + Calibration2 <= Dewpoint + Threshold2"
	 */
	if ((AUX_TEMPERATURE_SENSOR_1_ITEM->number.value + AUX_CALLIBRATION_SENSOR_1_ITEM->number.value) <=
	    (AUX_WEATHER_DEWPOINT_ITEM->number.value + AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value)) {
		dew_warning_1 = INDIGO_ALERT_STATE;
	} else {
		dew_warning_1 = INDIGO_OK_STATE;
	}
	if ((AUX_TEMPERATURE_SENSOR_2_ITEM->number.value + AUX_CALLIBRATION_SENSOR_2_ITEM->number.value) <=
	    (AUX_WEATHER_DEWPOINT_ITEM->number.value + AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value)) {
		dew_warning_2 = INDIGO_ALERT_STATE;
	} else {
		dew_warning_2 = INDIGO_OK_STATE;
	}
	if ((AUX_DEW_WARNING_SENSOR_1_ITEM->light.value != dew_warning_1) ||
	    (AUX_DEW_WARNING_SENSOR_2_ITEM->light.value != dew_warning_2)) {
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = dew_warning_1;
		AUX_DEW_WARNING_SENSOR_2_ITEM->light.value = dew_warning_2;
		indigo_update_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
	}

	if (updateCallibration) {
		AUX_CALLIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
	}
	if (updateThreshold) {
		AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	}
	if (updateAggressivity) {
		AUX_HEATER_AGGRESSIVITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
	}
	if (updateLinked) {
		AUX_LINK_CH_2AND3_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
	}
	if (updateHeaterOutlet) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	if (updateHeaterOutletState) {
		AUX_HEATER_OUTLET_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
	}
	if (updateAutoHeater) {
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	if (updateWeather) {
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	}
	if (updateSensors) {
		AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
	}

	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_connection_handler(indigo_device *device) {
	char command[8];
	char response[80];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			if (usbdp_command(device, UDP_IDENTIFY_CMD, response, sizeof(response))) {
				if (!strcmp(response, UDP1_IDENTIFY_RESPONSE)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to USB_Dewpoint v1 at %s", DEVICE_PORT_ITEM->text.value);
					PRIVATE_DATA->version = 1;
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "USB_Dewpoint v1");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
					indigo_update_property(device, INFO_PROPERTY, NULL);

					// for V1 we need one name only
					// indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
					// AUX_OUTLET_NAMES_PROPERTY->count = 1;
					// indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);

					AUX_HEATER_OUTLET_PROPERTY->hidden = true;
					AUX_HEATER_OUTLET_STATE_PROPERTY->hidden = true;
					AUX_DEW_CONTROL_PROPERTY->hidden = true;
					AUX_HEATER_AGGRESSIVITY_PROPERTY->hidden = true;
					AUX_LINK_CH_2AND3_PROPERTY->hidden = true;
					indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
					AUX_TEMPERATURE_SENSORS_PROPERTY->count = 1;
					indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
					indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
					indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
					AUX_DEW_WARNING_PROPERTY->count = 1;
					indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
				} else if (!strncmp(response, UDP2_IDENTIFY_RESPONSE, 4)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to USB_Dewpoint v2 at %s", DEVICE_PORT_ITEM->text.value);
					PRIVATE_DATA->version = 2;
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "USB_Dewpoint v2");
					sprintf(INFO_DEVICE_INTERFACE_ITEM->text.value, "%d", INDIGO_INTERFACE_AUX_WEATHER | INDIGO_INTERFACE_AUX_POWERBOX);
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
					indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
					indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
					indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
					indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
					indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
					indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
					indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
					indigo_define_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
					indigo_define_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
					indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB_Dewpoint not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB_Dewpoint not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			usbdp_status_t status;
			if (usbdp_status(device, &status)) {
				if (PRIVATE_DATA->version == 1) {

				} else if (PRIVATE_DATA->version == 2){

				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'SGETAL' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'SGETAL' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);

		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		if (PRIVATE_DATA->handle > 0) {
			if (PRIVATE_DATA->version == 2) {
				// maybe stop automatic mode too ?
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Stopping heaters...");
				// Stop heaters on disconnect
				sprintf(command, UDP2_OUTPUT_CMD, 1, 0);
				usbdp_command(device, command, response, sizeof(response));
				// maybe check responce if "DONE" ?
				sprintf(command, UDP2_OUTPUT_CMD, 2, 0);
				usbdp_command(device, command, response, sizeof(response));
				// maybe check responce if "DONE" ?
				sprintf(command, UDP2_OUTPUT_CMD, 3, 0);
				usbdp_command(device, command, response, sizeof(response));
				// maybe check responce if "DONE" ?
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
	}
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_TEMPERATURE_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_TEMPERATURE_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_CALLIBRATION_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_CALLIBRATION_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_DEW_THRESHOLD_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_DEW_THRESHOLD_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s (°C)", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_DEW_WARNING_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_DEW_WARNING_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);

	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_heater_outlet_handler(indigo_device *device) {
	char command[16], response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		sprintf(command, UDP2_OUTPUT_CMD, 1, (int)(AUX_HEATER_OUTLET_1_ITEM->number.value));
		usbdp_command(device, command, response, sizeof(response));
		// maybe check responce if "DONE" ?
		sprintf(command, UDP2_OUTPUT_CMD, 2, (int)(AUX_HEATER_OUTLET_2_ITEM->number.value));
		usbdp_command(device, command, response, sizeof(response));
		// maybe check responce if "DONE" ?
		sprintf(command, UDP2_OUTPUT_CMD, 3, (int)(AUX_HEATER_OUTLET_3_ITEM->number.value));
		usbdp_command(device, command, response, sizeof(response));
		// maybe check responce if "DONE" ?
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_callibration_handler(indigo_device *device) {
	char command[16], response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if ((IS_CONNECTED) && (PRIVATE_DATA->version == 2)) {
		sprintf(command, UDP2_CALIBRATION_CMD,
			(int)(AUX_CALLIBRATION_SENSOR_1_ITEM->number.value),
			(int)(AUX_CALLIBRATION_SENSOR_2_ITEM->number.value),
			(int)(AUX_CALLIBRATION_SENSOR_3_ITEM->number.value)
		);
		usbdp_command(device, command, response, sizeof(response));

		AUX_CALLIBRATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_dew_threshold_handler(indigo_device *device) {
	char command[16], response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if ((IS_CONNECTED) && (PRIVATE_DATA->version == 2)) {
		sprintf(command, UDP2_THRESHOLD_CMD,
			(int)(AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value),
			(int)(AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value)
		);
		usbdp_command(device, command, response, sizeof(response));

		AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_aggressivity_handler(indigo_device *device) {
	char command[16], response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if ((IS_CONNECTED) && (PRIVATE_DATA->version == 2)) {
		if (AUX_HEATER_AGGRESSIVITY_1_ITEM->sw.value) {
			PRIVATE_DATA->requested_aggressivity = 1;
		} else if (AUX_HEATER_AGGRESSIVITY_2_ITEM->sw.value) {
			PRIVATE_DATA->requested_aggressivity = 2;
		} else if (AUX_HEATER_AGGRESSIVITY_5_ITEM->sw.value) {
			PRIVATE_DATA->requested_aggressivity = 3;
		} else if (AUX_HEATER_AGGRESSIVITY_10_ITEM->sw.value) {
			PRIVATE_DATA->requested_aggressivity = 4;
		}
		sprintf(command, UDP2_AGGRESSIVITY_CMD, PRIVATE_DATA->requested_aggressivity);
		usbdp_command(device, command, response, sizeof(response));
		AUX_HEATER_AGGRESSIVITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_dew_control_handler(indigo_device *device) {
	char response[128];
	char command[10];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		sprintf(command, UDP2_AUTO_CMD, AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0);
		usbdp_command(device, command, response, sizeof(response));
		// maybe check responce if "DONE" ?
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_link_channels_handler(indigo_device *device) {
	char response[128];
	char command[10];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		sprintf(command, UDP2_LINK_CMD, AUX_LINK_CH_2AND3_LINKED_ITEM->sw.value ? 1 : 0);
		usbdp_command(device, command, response, sizeof(response));
		// maybe check responce if "DONE" ?
		AUX_LINK_CH_2AND3_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_outlet_names_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_heater_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_CALLIBRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_CALLIBRATIN
		indigo_property_copy_values(AUX_CALLIBRATION_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_callibration_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_THRESHOLD
		indigo_property_copy_values(AUX_DEW_THRESHOLD_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_dew_threshold_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_dew_control_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_LINK_CH_2AND3_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_LINK_CHANNELS_2AND3
		indigo_property_copy_values(AUX_LINK_CH_2AND3_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_link_channels_handler, NULL);
		indigo_update_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_HEATER_AGGRESSIVITY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_AGGRESSIVITY
		indigo_property_copy_values(AUX_HEATER_AGGRESSIVITY_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_aggressivity_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			//int current_count = AUX_OUTLET_NAMES_PROPERTY->count;
			//AUX_OUTLET_NAMES_PROPERTY->count = 3;
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			//AUX_OUTLET_NAMES_PROPERTY->count = current_count;
			indigo_save_property(device, NULL, AUX_CALLIBRATION_PROPERTY);
			indigo_save_property(device, NULL, AUX_DEW_THRESHOLD_PROPERTY);
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
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
	indigo_release_property(AUX_CALLIBRATION_PROPERTY);
	indigo_release_property(AUX_DEW_THRESHOLD_PROPERTY);
	indigo_release_property(AUX_HEATER_AGGRESSIVITY_PROPERTY);
	indigo_release_property(AUX_LINK_CH_2AND3_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_usbdp(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbdp_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"USB Dewpoint",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, "USB Dewpoint", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(usbdp_private_data));
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
