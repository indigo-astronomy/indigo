// Copyright (c) 2019-2025 Rumen G. Bogdanovski
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

// This file generated from indigo_aux_usbdp.driver

// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_usbdp.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_aux_usbdp"
#define DRIVER_LABEL         "USB Dewpoint"
#define AUX_DEVICE_NAME      "USB Dewpoint"
#define PRIVATE_DATA         ((usbdp_private_data *)device->private_data)

//+ define

#define AUX_GROUP            "Auxiliary"
#define SETTINGS_GROUP       "Settings"
#define UDP_CMD_LEN          6

#define UDP_STATUS_CMD       "SGETAL"
#define UDP1_STATUS_RESPONSE "Tloc=%f-Tamb=%f-RH=%f-DP=%f-TH=%d-C=%d"
#define UDP2_STATUS_RESPONSE "##%f/%f/%f/%f/%f/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u**"

#define UDP_IDENTIFY_CMD     "SWHOIS"
#define UDP1_IDENTIFY_RESPONSE "UDP"
#define UDP2_IDENTIFY_RESPONSE "UDP2(%u)" // Firmware version? Like "UDP2(1446)"

#define UDP_RESET_CMD        "SEERAZ"
#define UDP_RESET_RESPONSE   "EEPROM RESET"

#define UDP2_OUTPUT_CMD      "S%1uO%03u"         // channel 1-3, power 0-100
#define UDP2_THRESHOLD_CMD   "STHR%1u%1u"     // channel 1-2, value 0-9
#define UDP2_CALIBRATION_CMD "SCA%1u%1u%1u" // channel 1-2-Amb, value 0-9
#define UDP2_LINK_CMD        "SLINK%1u"            // 0 or 1 to link channels 2 and 3
#define UDP2_AUTO_CMD        "SAUTO%1u"            // 0 or 1 to enable auto mode
#define UDP2_AGGRESSIVITY_CMD "SAGGR%1u"    // 1-4 (1, 2, 5, 10)
#define UDP_DONE_RESPONSE    "DONE"

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_HEATER_OUTLET_NAME_1_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_2_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_NAME_3_ITEM  (AUX_OUTLET_NAMES_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_PROPERTY     (PRIVATE_DATA->aux_heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM       (AUX_HEATER_OUTLET_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_STATE_PROPERTY (PRIVATE_DATA->aux_heater_outlet_state_property)
#define AUX_HEATER_OUTLET_STATE_1_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_STATE_2_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_STATE_3_ITEM   (AUX_HEATER_OUTLET_STATE_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY       (PRIVATE_DATA->aux_dew_control_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM    (AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM (AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_WEATHER_PROPERTY           (PRIVATE_DATA->aux_weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM   (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM      (AUX_WEATHER_PROPERTY->items + 2)

#define AUX_TEMPERATURE_SENSORS_PROPERTY (PRIVATE_DATA->aux_temperature_sensors_property)
#define AUX_TEMPERATURE_SENSOR_1_ITEM    (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSOR_2_ITEM    (AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)

#define AUX_CALLIBRATION_PROPERTY           (PRIVATE_DATA->aux_callibration_property)
#define AUX_CALLIBRATION_SENSOR_1_ITEM      (AUX_CALLIBRATION_PROPERTY->items + 0)
#define AUX_CALLIBRATION_SENSOR_2_ITEM      (AUX_CALLIBRATION_PROPERTY->items + 1)
#define AUX_CALLIBRATION_SENSOR_3_ITEM      (AUX_CALLIBRATION_PROPERTY->items + 2)

#define AUX_CALLIBRATION_PROPERTY_NAME      "AUX_TEMPERATURE_CALLIBRATION"
#define AUX_CALLIBRATION_SENSOR_1_ITEM_NAME "SENSOR_1"
#define AUX_CALLIBRATION_SENSOR_2_ITEM_NAME "SENSOR_2"
#define AUX_CALLIBRATION_SENSOR_3_ITEM_NAME "SENSOR_3"

#define AUX_DEW_THRESHOLD_PROPERTY      (PRIVATE_DATA->aux_dew_threshold_property)
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM (AUX_DEW_THRESHOLD_PROPERTY->items + 0)
#define AUX_DEW_THRESHOLD_SENSOR_2_ITEM (AUX_DEW_THRESHOLD_PROPERTY->items + 1)

#define AUX_LINK_CH_2AND3_PROPERTY             (PRIVATE_DATA->aux_link_ch_2and3_property)
#define AUX_LINK_CH_2AND3_LINKED_ITEM          (AUX_LINK_CH_2AND3_PROPERTY->items + 0)
#define AUX_LINK_CH_2AND3_NOT_LINKED_ITEM      (AUX_LINK_CH_2AND3_PROPERTY->items + 1)

#define AUX_LINK_CH_2AND3_PROPERTY_NAME        "AUX_LINK_CHANNELS_2AND3"
#define AUX_LINK_CH_2AND3_LINKED_ITEM_NAME     "LINKED"
#define AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME "NOT_LINKED"

#define AUX_HEATER_AGGRESSIVITY_PROPERTY      (PRIVATE_DATA->aux_heater_aggressivity_property)
#define AUX_HEATER_AGGRESSIVITY_1_ITEM        (AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 0)
#define AUX_HEATER_AGGRESSIVITY_2_ITEM        (AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 1)
#define AUX_HEATER_AGGRESSIVITY_5_ITEM        (AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 2)
#define AUX_HEATER_AGGRESSIVITY_10_ITEM       (AUX_HEATER_AGGRESSIVITY_PROPERTY->items + 3)

#define AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME "AUX_HEATER_AGGRESSIVITY"
#define AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME   "AGGRESSIVITY_1"
#define AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME   "AGGRESSIVITY_2"
#define AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME   "AGGRESSIVITY_5"
#define AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME  "AGGRESSIVITY_10"

#define AUX_DEW_WARNING_PROPERTY       (PRIVATE_DATA->aux_dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM  (AUX_DEW_WARNING_PROPERTY->items + 0)
#define AUX_DEW_WARNING_SENSOR_2_ITEM  (AUX_DEW_WARNING_PROPERTY->items + 1)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_heater_outlet_property;
	indigo_property *aux_heater_outlet_state_property;
	indigo_property *aux_dew_control_property;
	indigo_property *aux_weather_property;
	indigo_property *aux_temperature_sensors_property;
	indigo_property *aux_callibration_property;
	indigo_property *aux_dew_threshold_property;
	indigo_property *aux_link_ch_2and3_property;
	indigo_property *aux_heater_aggressivity_property;
	indigo_property *aux_dew_warning_property;
	indigo_timer *aux_timer;
	indigo_timer *aux_connection_handler_timer;
	indigo_timer *aux_outlet_names_handler_timer;
	indigo_timer *aux_heater_outlet_handler_timer;
	indigo_timer *aux_dew_control_handler_timer;
	indigo_timer *aux_callibration_handler_timer;
	indigo_timer *aux_dew_threshold_handler_timer;
	indigo_timer *aux_link_ch_2and3_handler_timer;
	indigo_timer *aux_heater_aggressivity_handler_timer;
	//+ data
	int version;
	uint8_t requested_aggressivity;
	char response[128];
	//- data
} usbdp_private_data;

#pragma mark - Low level code

//+ code

static bool usbdp_command(indigo_device *device, char *command, ...) {
	indigo_usleep(20000);
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
		if (result > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool usbdp_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (usbdp_command(device, UDP_IDENTIFY_CMD)) {
			if (!strcmp(PRIVATE_DATA->response, UDP1_IDENTIFY_RESPONSE)) {
				PRIVATE_DATA->version = 1;
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "USB_Dewpoint v1");
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
				AUX_TEMPERATURE_SENSORS_PROPERTY->count = 1;
				AUX_DEW_WARNING_PROPERTY->count = 1;
				return true;
			} else if (!strncmp(PRIVATE_DATA->response, UDP2_IDENTIFY_RESPONSE, 4)) {
				PRIVATE_DATA->version = 2;
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "USB_Dewpoint v2");
				sprintf(INFO_DEVICE_INTERFACE_ITEM->text.value, "%d", INDIGO_INTERFACE_AUX_WEATHER | INDIGO_INTERFACE_AUX_POWERBOX);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
			indigo_send_message(device, "Handshake failed");
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
	}
	return false;
}

static void usbdp_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ aux.on_timer
	bool updateHeaterOutlet = false;
	bool updateHeaterOutletState = false;
	bool updateWeather = false;
	bool updateSensors = false;
	bool updateAutoHeater = false;
	bool updateCallibration = false;
	bool updateThreshold = false;
	bool updateAggressivity = false;
	bool updateLinked = false;
	int channel_1_state, channel_2_state, channel_3_state, dew_warning_1, dew_warning_2;
	if (usbdp_command(device, UDP_STATUS_CMD)) {
		if (PRIVATE_DATA->version == 1) {
			float temp_loc, temp_amb, rh, dewpoint;
			int threshold, c;
			int parsed = sscanf(PRIVATE_DATA->response, UDP1_STATUS_RESPONSE, &temp_loc, &temp_amb, &rh, &dewpoint, &threshold, &c);
			if (parsed == 6) {
				if ((fabs(((double)temp_amb - AUX_WEATHER_TEMPERATURE_ITEM->number.value)*100) >= 1) || (fabs(((double)rh - AUX_WEATHER_HUMIDITY_ITEM->number.value)*100) >= 1) || (fabs(((double)dewpoint - AUX_WEATHER_DEWPOINT_ITEM->number.value)*100) >= 1)) {
					AUX_WEATHER_TEMPERATURE_ITEM->number.value = temp_amb;
					AUX_WEATHER_HUMIDITY_ITEM->number.value = rh;
					AUX_WEATHER_DEWPOINT_ITEM->number.value = dewpoint;
					updateWeather = true;
				}
				if (fabs(((double)temp_loc - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1) {
					AUX_TEMPERATURE_SENSOR_1_ITEM->number.value = temp_loc;
					updateSensors = true;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error: parsed %d of 6 values in response \"%s\"", parsed, PRIVATE_DATA->response);
			}
		} else if (PRIVATE_DATA->version == 2) {
			float temp_ch1, temp_ch2, temp_amb, rh, dewpoint;
			int output_ch1, output_ch2, output_ch3, cal_ch1, cal_ch2, cal_amb, threshold_ch1, threshold_ch2, auto_mode, ch2_3_linked, aggressivity;
			int parsed = sscanf(PRIVATE_DATA->response, UDP2_STATUS_RESPONSE, &temp_ch1, &temp_ch2, &temp_amb, &rh, &dewpoint, &output_ch1, &output_ch2, &output_ch3, &cal_ch1, &cal_ch2, &cal_amb, &threshold_ch1, &threshold_ch2, &auto_mode, &ch2_3_linked, &aggressivity);
			if (parsed == 16) {
				if ((fabs(((double)temp_amb - AUX_WEATHER_TEMPERATURE_ITEM->number.value)*100) >= 1) || (fabs(((double)rh - AUX_WEATHER_HUMIDITY_ITEM->number.value)*100) >= 1) ||  (fabs(((double)dewpoint - AUX_WEATHER_DEWPOINT_ITEM->number.value)*100) >= 1)) {
					AUX_WEATHER_TEMPERATURE_ITEM->number.value = temp_amb;
					AUX_WEATHER_HUMIDITY_ITEM->number.value = rh;
					AUX_WEATHER_DEWPOINT_ITEM->number.value = dewpoint;
					updateWeather = true;
				}
				if ((fabs(((double)temp_ch1 - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1) || (fabs(((double)temp_ch1 - AUX_TEMPERATURE_SENSOR_1_ITEM->number.value)*100) >= 1)) {
					AUX_TEMPERATURE_SENSOR_1_ITEM->number.value = temp_ch1;
					AUX_TEMPERATURE_SENSOR_2_ITEM->number.value = temp_ch2;
					updateSensors = true;
				}
				if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value != auto_mode) {
					if (auto_mode) {
						indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_AUTOMATIC_ITEM, true);
					} else {
						indigo_set_switch(AUX_DEW_CONTROL_PROPERTY, AUX_DEW_CONTROL_MANUAL_ITEM, true);
					}
					updateAutoHeater = true;
				}
				if (((int)(AUX_HEATER_OUTLET_1_ITEM->number.value) != output_ch1) ||  ((int)(AUX_HEATER_OUTLET_2_ITEM->number.value) != output_ch2) || ((int)(AUX_HEATER_OUTLET_3_ITEM->number.value) != output_ch3)) {
					AUX_HEATER_OUTLET_1_ITEM->number.value = output_ch1;
					AUX_HEATER_OUTLET_2_ITEM->number.value = output_ch2;
					AUX_HEATER_OUTLET_3_ITEM->number.value = output_ch3;
					updateHeaterOutlet = true;
				}
				if ((bool)output_ch1 && auto_mode) {
					channel_1_state = INDIGO_BUSY_STATE;
				} else if ((bool)output_ch1) {
					channel_1_state = INDIGO_OK_STATE;
				} else {
					channel_1_state = INDIGO_IDLE_STATE;
				}
				if ((bool)output_ch2 && auto_mode) {
					channel_2_state = INDIGO_BUSY_STATE;
				} else if ((bool)output_ch2) {
					channel_2_state = INDIGO_OK_STATE;
				} else {
					channel_2_state = INDIGO_IDLE_STATE;
				}
				if ((bool)output_ch3 && auto_mode && ch2_3_linked) {
					channel_3_state = INDIGO_BUSY_STATE;
				} else if ((bool)output_ch3) {
					channel_3_state = INDIGO_OK_STATE;
				} else {
					channel_3_state = INDIGO_IDLE_STATE;
				}
				if ((AUX_HEATER_OUTLET_STATE_1_ITEM->light.value != channel_1_state) || (AUX_HEATER_OUTLET_STATE_2_ITEM->light.value != channel_2_state) || (AUX_HEATER_OUTLET_STATE_3_ITEM->light.value != channel_3_state)) {
					AUX_HEATER_OUTLET_STATE_1_ITEM->light.value = channel_1_state;
					AUX_HEATER_OUTLET_STATE_2_ITEM->light.value = channel_2_state;
					AUX_HEATER_OUTLET_STATE_3_ITEM->light.value = channel_3_state;
					updateHeaterOutletState = true;
				}
				if (((int)(AUX_CALLIBRATION_SENSOR_1_ITEM->number.value) != cal_ch1) || ((int)(AUX_CALLIBRATION_SENSOR_2_ITEM->number.value) != cal_ch2) || ((int)(AUX_CALLIBRATION_SENSOR_3_ITEM->number.value) != cal_amb)) {
					AUX_CALLIBRATION_SENSOR_1_ITEM->number.value = cal_ch1;
					AUX_CALLIBRATION_SENSOR_2_ITEM->number.value = cal_ch2;
					AUX_CALLIBRATION_SENSOR_3_ITEM->number.value = cal_amb;
					updateCallibration = true;
				}
				if (((int)(AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value) != threshold_ch1) || ((int)(AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value) != threshold_ch2)) {
					AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value = threshold_ch1;
					AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value = threshold_ch2;
					updateThreshold = true;
				}
				if (PRIVATE_DATA->requested_aggressivity != aggressivity) {
					switch (aggressivity) {
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
					PRIVATE_DATA->requested_aggressivity = aggressivity;
					updateAggressivity = true;
				}
				if (AUX_LINK_CH_2AND3_LINKED_ITEM->sw.value != ch2_3_linked) {
					if (ch2_3_linked) {
						indigo_set_switch(AUX_LINK_CH_2AND3_PROPERTY, AUX_LINK_CH_2AND3_LINKED_ITEM, true);
					} else {
						indigo_set_switch(AUX_LINK_CH_2AND3_PROPERTY, AUX_LINK_CH_2AND3_NOT_LINKED_ITEM, true);
					}
					updateLinked = true;
				}			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME,"Error: parsed %d of 16 values in response \"%s\"", parsed, PRIVATE_DATA->response);
			}
		}
	}
	/* Dew warning is issued when:
		"Temp1 + Calibration1 <= Dewpoint + Threshold1"
		"Temp2 + Calibration2 <= Dewpoint + Threshold2"
	 */
	if ((AUX_TEMPERATURE_SENSOR_1_ITEM->number.value + AUX_CALLIBRATION_SENSOR_1_ITEM->number.value) <= (AUX_WEATHER_DEWPOINT_ITEM->number.value + AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value)) {
		dew_warning_1 = INDIGO_ALERT_STATE;
	} else {
		dew_warning_1 = INDIGO_OK_STATE;
	}
	if ((AUX_TEMPERATURE_SENSOR_2_ITEM->number.value + AUX_CALLIBRATION_SENSOR_2_ITEM->number.value) <= (AUX_WEATHER_DEWPOINT_ITEM->number.value + AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value)) {
		dew_warning_2 = INDIGO_ALERT_STATE;
	} else {
		dew_warning_2 = INDIGO_OK_STATE;
	}
	if ((AUX_DEW_WARNING_SENSOR_1_ITEM->light.value != dew_warning_1) || (AUX_DEW_WARNING_SENSOR_2_ITEM->light.value != dew_warning_2)) {
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
	//- aux.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = usbdp_open(device);
		if (connection_result) {
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
			indigo_define_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
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
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_heater_outlet_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_dew_control_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_callibration_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_dew_threshold_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_link_ch_2and3_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_heater_aggressivity_handler_timer);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		//+ aux.on_disconnect
		if (PRIVATE_DATA->version == 2) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Stopping heaters...");
			usbdp_command(device, UDP2_OUTPUT_CMD, 1, 0);
			usbdp_command(device, UDP2_OUTPUT_CMD, 2, 0);
			usbdp_command(device, UDP2_OUTPUT_CMD, 3, 0);
		}
		//- aux.on_disconnect
		usbdp_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
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
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_define_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
		indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_OUTLET.on_change
	usbdp_command(device, UDP2_OUTPUT_CMD, 1, (int)(AUX_HEATER_OUTLET_1_ITEM->number.value));
	usbdp_command(device, UDP2_OUTPUT_CMD, 2, (int)(AUX_HEATER_OUTLET_2_ITEM->number.value));
	usbdp_command(device, UDP2_OUTPUT_CMD, 3, (int)(AUX_HEATER_OUTLET_3_ITEM->number.value));
	//- aux.AUX_HEATER_OUTLET.on_change
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_dew_control_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DEW_CONTROL.on_change
	usbdp_command(device, UDP2_AUTO_CMD, AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? 1 : 0);
	//- aux.AUX_DEW_CONTROL.on_change
	indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_callibration_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_CALLIBRATION_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_CALLIBRATION.on_change
	usbdp_command(device, UDP2_CALIBRATION_CMD, (int)(AUX_CALLIBRATION_SENSOR_1_ITEM->number.value), (int)(AUX_CALLIBRATION_SENSOR_2_ITEM->number.value), (int)(AUX_CALLIBRATION_SENSOR_3_ITEM->number.value));
	//- aux.AUX_CALLIBRATION.on_change
	indigo_update_property(device, AUX_CALLIBRATION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_dew_threshold_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_DEW_THRESHOLD.on_change
	usbdp_command(device, UDP2_THRESHOLD_CMD, (int)(AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value), (int)(AUX_DEW_THRESHOLD_SENSOR_2_ITEM->number.value));
	//- aux.AUX_DEW_THRESHOLD.on_change
	indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_link_ch_2and3_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_LINK_CH_2AND3_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LINK_CH_2AND3.on_change
	usbdp_command(device, UDP2_LINK_CMD, AUX_LINK_CH_2AND3_LINKED_ITEM->sw.value ? 1 : 0);
	//- aux.AUX_LINK_CH_2AND3.on_change
	indigo_update_property(device, AUX_LINK_CH_2AND3_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_aggressivity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	AUX_HEATER_AGGRESSIVITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_HEATER_AGGRESSIVITY.on_change
	if (AUX_HEATER_AGGRESSIVITY_1_ITEM->sw.value) {
		PRIVATE_DATA->requested_aggressivity = 1;
	} else if (AUX_HEATER_AGGRESSIVITY_2_ITEM->sw.value) {
		PRIVATE_DATA->requested_aggressivity = 2;
	} else if (AUX_HEATER_AGGRESSIVITY_5_ITEM->sw.value) {
		PRIVATE_DATA->requested_aggressivity = 3;
	} else if (AUX_HEATER_AGGRESSIVITY_10_ITEM->sw.value) {
		PRIVATE_DATA->requested_aggressivity = 4;
	}
	usbdp_command(device, UDP2_AGGRESSIVITY_CMD, PRIVATE_DATA->requested_aggressivity);
	//- aux.AUX_HEATER_AGGRESSIVITY.on_change
	indigo_update_property(device, AUX_HEATER_AGGRESSIVITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 5;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, SETTINGS_GROUP, "Outlet/Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater/Sensor #1", "Heater/Sensor #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater/Sensor #2", "Heater/Sensor #2");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater #3", "Heater #3");
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Heater outlets state", INDIGO_OK_STATE, 3);
		if (AUX_HEATER_OUTLET_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1", INDIGO_IDLE_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2", INDIGO_IDLE_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_3_ITEM, AUX_HEATER_OUTLET_STATE_3_ITEM_NAME, "Heater #3", INDIGO_IDLE_STATE);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient Temperature (°C)", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 0, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint (°C)", 0, 0, 0, 0);
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, AUX_GROUP, "Temperature Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_TEMPERATURE_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", 0, 0, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", 0, 0, 0, 0);
		AUX_CALLIBRATION_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_CALLIBRATION_PROPERTY_NAME, SETTINGS_GROUP, "Temperature Sensors Callibration", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_CALLIBRATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_1_ITEM, AUX_CALLIBRATION_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_2_ITEM, AUX_CALLIBRATION_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_CALLIBRATION_SENSOR_3_ITEM, AUX_CALLIBRATION_SENSOR_3_ITEM_NAME, "Ambient sensor (°C)", 0, 9, 1, 0);
		AUX_DEW_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_DEW_THRESHOLD_PROPERTY_NAME, SETTINGS_GROUP, "Dew Thresholds", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_DEW_THRESHOLD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_1_ITEM, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME, "Sensor #1 (°C)", 0, 9, 1, 0);
		indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_2_ITEM, AUX_DEW_THRESHOLD_SENSOR_2_ITEM_NAME, "Sensor #2 (°C)", 0, 9, 1, 0);
		AUX_LINK_CH_2AND3_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LINK_CH_2AND3_PROPERTY_NAME, SETTINGS_GROUP, "Link chanels 2 and 3", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LINK_CH_2AND3_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_LINK_CH_2AND3_LINKED_ITEM, AUX_LINK_CH_2AND3_LINKED_ITEM_NAME, "Linked", false);
		indigo_init_switch_item(AUX_LINK_CH_2AND3_NOT_LINKED_ITEM, AUX_LINK_CH_2AND3_NOT_LINKED_ITEM_NAME, "Not Linked", true);
		AUX_HEATER_AGGRESSIVITY_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_HEATER_AGGRESSIVITY_PROPERTY_NAME, SETTINGS_GROUP, "Auto mode heater aggressivity", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AUX_HEATER_AGGRESSIVITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_1_ITEM, AUX_HEATER_AGGRESSIVITY_1_ITEM_NAME, "Aggressivity 1%", true);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_2_ITEM, AUX_HEATER_AGGRESSIVITY_2_ITEM_NAME, "Aggressivity 2%", false);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_5_ITEM, AUX_HEATER_AGGRESSIVITY_5_ITEM_NAME, "Aggressivity 5%", false);
		indigo_init_switch_item(AUX_HEATER_AGGRESSIVITY_10_ITEM, AUX_HEATER_AGGRESSIVITY_10_ITEM_NAME, "Aggressivity 10%", false);
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, AUX_GROUP, "Dew warning", INDIGO_OK_STATE, 2);
		if (AUX_DEW_WARNING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Sensor #1", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_2_ITEM, AUX_DEW_WARNING_SENSOR_2_ITEM_NAME, "Sesnor #2", INDIGO_OK_STATE);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_CONTROL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_TEMPERATURE_SENSORS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_CALLIBRATION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_THRESHOLD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LINK_CH_2AND3_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_AGGRESSIVITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_DEW_WARNING_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler, aux_outlet_names_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_OUTLET_PROPERTY, aux_heater_outlet_handler, aux_heater_outlet_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DEW_CONTROL_PROPERTY, aux_dew_control_handler, aux_dew_control_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_CALLIBRATION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_CALLIBRATION_PROPERTY, aux_callibration_handler, aux_callibration_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_THRESHOLD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_DEW_THRESHOLD_PROPERTY, aux_dew_threshold_handler, aux_dew_threshold_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LINK_CH_2AND3_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LINK_CH_2AND3_PROPERTY, aux_link_ch_2and3_handler, aux_link_ch_2and3_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_AGGRESSIVITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_HEATER_AGGRESSIVITY_PROPERTY, aux_heater_aggressivity_handler, aux_heater_aggressivity_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_CALLIBRATION_PROPERTY);
			indigo_save_property(device, NULL, AUX_DEW_THRESHOLD_PROPERTY);
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
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
	indigo_release_property(AUX_CALLIBRATION_PROPERTY);
	indigo_release_property(AUX_DEW_THRESHOLD_PROPERTY);
	indigo_release_property(AUX_LINK_CH_2AND3_PROPERTY);
	indigo_release_property(AUX_HEATER_AGGRESSIVITY_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_usbdp(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbdp_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

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
