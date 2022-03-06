// Copyright (C) 2020 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO Lunatico AAG CloudWatcher AUX driver
 \file indigo_aux_cloudwatcher.c
 */

#include "indigo_aux_cloudwatcher.h"

#define DRIVER_VERSION         0x0007
#define AUX_CLOUDWATCHER_NAME  "AAG CloudWatcher"

#define DRIVER_NAME              "indigo_aux_skywatcher"
#define DRIVER_INFO              "AAG CloudWatcher"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE "9600"

#define NO_READING (-100000)

#define PRIVATE_DATA                   ((aag_private_data *)device->private_data)

#define SETTINGS_GROUP	 "Settings"
#define THRESHOLDS_GROUP "Tresholds"
#define WARNINGS_GROUP   "Warnings"
#define WEATHER_GROUP    "Weather"
#define SWITCH_GROUP     "Switch Control"
#define STATUS_GROUP     "Device status"

// Switch
#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)

#define AUX_GPIO_OUTLET_PROPERTY       (PRIVATE_DATA->gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM         (AUX_GPIO_OUTLET_PROPERTY->items + 0)

// Heater contorol state
#define X_HEATER_CONTROL_STATE_PROPERTY_NAME    "X_HEATER_CONTROL_STATE"
#define X_HEATER_CONTROL_NORMAL_ITEM_NAME       "NORMAL"
#define X_HEATER_CONTROL_INCREASE_ITEM_NAME     "INCREASE"
#define X_HEATER_CONTROL_PULSE_ITEM_NAME        "PULSE"

#define X_HEATER_CONTROL_STATE_PROPERTY         (PRIVATE_DATA->heater_control_state)
#define X_HEATER_CONTROL_NORMAL_ITEM            (X_HEATER_CONTROL_STATE_PROPERTY->items + 0)
#define X_HEATER_CONTROL_INCREASE_ITEM          (X_HEATER_CONTROL_STATE_PROPERTY->items + 1)
#define X_HEATER_CONTROL_PULSE_ITEM           (X_HEATER_CONTROL_STATE_PROPERTY->items + 2)

#define X_SKY_CORRECTION_PROPERTY_NAME  "X_SKY_CORRECTION"
#define X_SKY_CORRECTION_K1_ITEM_NAME   "K1"
#define X_SKY_CORRECTION_K2_ITEM_NAME   "K2"
#define X_SKY_CORRECTION_K3_ITEM_NAME   "K3"
#define X_SKY_CORRECTION_K4_ITEM_NAME   "K4"
#define X_SKY_CORRECTION_K5_ITEM_NAME   "K5"

#define X_SKY_CORRECTION_PROPERTY      (PRIVATE_DATA->sky_correction_property)
#define X_SKY_CORRECTION_K1_ITEM       (X_SKY_CORRECTION_PROPERTY->items + 0)
#define X_SKY_CORRECTION_K2_ITEM       (X_SKY_CORRECTION_PROPERTY->items + 1)
#define X_SKY_CORRECTION_K3_ITEM       (X_SKY_CORRECTION_PROPERTY->items + 2)
#define X_SKY_CORRECTION_K4_ITEM       (X_SKY_CORRECTION_PROPERTY->items + 3)
#define X_SKY_CORRECTION_K5_ITEM       (X_SKY_CORRECTION_PROPERTY->items + 4)

#define X_CONSTANTS_PROPERTY_NAME              "X_AAG_CONSTANTS"
#define X_CONSTANTS_ZENER_VOLTAGE_ITEM_NAME    "ZENER_VOLTAGE"
#define X_CONSTANTS_LDR_MAX_R_ITEM_NAME        "LDR_MAX_R"
#define X_CONSTANTS_LDR_PULLUP_R_ITEM_NAME     "LDR_PULLUP_R"
#define X_CONSTANTS_RAIN_BETA_ITEM_NAME        "RAIN_BETA_FACTOR"
#define X_CONSTANTS_RAIN_R_AT_25_ITEM_NAME     "RAIN_R_AT_25"
#define X_CONSTANTS_RAIN_PULLUP_R_ITEM_NAME    "RAIN_PULLUP_R"
#define X_CONSTANTS_AMBIENT_BETA_ITEM_NAME     "AMBIENT_BETA_FACTOR"
#define X_CONSTANTS_AMBIENT_R_AT_25_ITEM_NAME  "AMBIENT_R_AT_25"
#define X_CONSTANTS_AMBIENT_PULLUP_R_ITEM_NAME "AMBIENT_PULLUP_R"
#define X_CONSTANTS_ANEMOMETER_STATE_ITEM_NAME "ANEMOMETER_STATUS"


#define X_CONSTANTS_PROPERTY               (PRIVATE_DATA->constants_property)
#define X_CONSTANTS_ZENER_VOLTAGE_ITEM     (X_CONSTANTS_PROPERTY->items + 0)
#define X_CONSTANTS_LDR_MAX_R_ITEM         (X_CONSTANTS_PROPERTY->items + 1)
#define X_CONSTANTS_LDR_PULLUP_R_ITEM      (X_CONSTANTS_PROPERTY->items + 2)
#define X_CONSTANTS_RAIN_BETA_ITEM         (X_CONSTANTS_PROPERTY->items + 3)
#define X_CONSTANTS_RAIN_R_AT_25_ITEM      (X_CONSTANTS_PROPERTY->items + 4)
#define X_CONSTANTS_RAIN_PULLUP_R_ITEM     (X_CONSTANTS_PROPERTY->items + 5)
#define X_CONSTANTS_AMBIENT_BETA_ITEM      (X_CONSTANTS_PROPERTY->items + 6)
#define X_CONSTANTS_AMBIENT_R_AT_25_ITEM   (X_CONSTANTS_PROPERTY->items + 7)
#define X_CONSTANTS_AMBIENT_PULLUP_R_ITEM  (X_CONSTANTS_PROPERTY->items + 8)
#define X_CONSTANTS_ANEMOMETER_STATE_ITEM  (X_CONSTANTS_PROPERTY->items + 9)

#define X_SENSOR_READINGS_PROPERTY_NAME              AUX_INFO_PROPERTY_NAME
#define X_SENSOR_RAW_SKY_TEMPERATURE_ITEM_NAME       "RAW_IR_SKY_TEMPERATURE"
#define X_SENSOR_SKY_TEMPERATURE_ITEM_NAME           "IR_SKY_TEMPERATURE"
#define X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM_NAME     "IR_SENSOR_TEMPERATURE"
#define X_SENSOR_RAIN_CYCLES_ITEM_NAME               "RAIN_CYCLES"
#define X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM_NAME   "RAIN_SENSOR_TEMPERATURE"
#define X_SENSOR_RAIN_HEATER_POWER_ITEM_NAME         "RAIN_SENSOR_HEATER_POWER"
#define X_SENSOR_SKY_BRIGHTNESS_ITEM_NAME            "SKY_BRIGHTNES"
#define X_SENSOR_AMBIENT_TEMPERATURE_ITEM_NAME       "AMBIENT_TEMPERATURE"

#define X_SENSOR_READINGS_PROPERTY               (PRIVATE_DATA->sensor_readings_property)
#define X_SENSOR_RAW_SKY_TEMPERATURE_ITEM        (X_SENSOR_READINGS_PROPERTY->items + 0)
#define X_SENSOR_SKY_TEMPERATURE_ITEM            (X_SENSOR_READINGS_PROPERTY->items + 1)
#define X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM      (X_SENSOR_READINGS_PROPERTY->items + 2)
#define X_SENSOR_RAIN_CYCLES_ITEM                (X_SENSOR_READINGS_PROPERTY->items + 3)
#define X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM    (X_SENSOR_READINGS_PROPERTY->items + 4)
#define X_SENSOR_RAIN_HEATER_POWER_ITEM          (X_SENSOR_READINGS_PROPERTY->items + 5)
#define X_SENSOR_SKY_BRIGHTNESS_ITEM             (X_SENSOR_READINGS_PROPERTY->items + 6)
#define X_SENSOR_AMBIENT_TEMPERATURE_ITEM        (X_SENSOR_READINGS_PROPERTY->items + 7)

#define AUX_WEATHER_PROPERTY                     (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM             (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM      (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM                (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_HUMIDITY_ITEM                (AUX_WEATHER_PROPERTY->items + 3)
#define AUX_WEATHER_WIND_SPEED_ITEM              (AUX_WEATHER_PROPERTY->items + 4)


// DEW
#define AUX_DEW_THRESHOLD_PROPERTY				(PRIVATE_DATA->dew_threshold_property)
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM			(AUX_DEW_THRESHOLD_PROPERTY->items + 0)

#define AUX_DEW_WARNING_PROPERTY				(PRIVATE_DATA->dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM			(AUX_DEW_WARNING_PROPERTY->items + 0)

// WIND
#define AUX_WIND_THRESHOLD_PROPERTY				(PRIVATE_DATA->wind_threshold_property)
#define AUX_WIND_THRESHOLD_SENSOR_1_ITEM		(AUX_WIND_THRESHOLD_PROPERTY->items + 0)

#define AUX_WIND_WARNING_PROPERTY				(PRIVATE_DATA->wind_warning_property)
#define AUX_WIND_WARNING_SENSOR_1_ITEM			(AUX_WIND_WARNING_PROPERTY->items + 0)

// RAIN
#define AUX_RAIN_THRESHOLD_PROPERTY				(PRIVATE_DATA->rain_threshold_property)
#define AUX_RAIN_THRESHOLD_SENSOR_1_ITEM		(AUX_RAIN_THRESHOLD_PROPERTY->items + 0)

#define AUX_RAIN_WARNING_PROPERTY				(PRIVATE_DATA->rain_warning_property)
#define AUX_RAIN_WARNING_SENSOR_1_ITEM			(AUX_RAIN_WARNING_PROPERTY->items + 0)


// Humidity
#define AUX_HUMIDITY_THRESHOLDS_PROPERTY      (PRIVATE_DATA->rh_condition_thresholds_property)
#define AUX_HUMIDITY_HUMID_THRESHOLD_ITEM     (AUX_HUMIDITY_THRESHOLDS_PROPERTY->items + 0)
#define AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM    (AUX_HUMIDITY_THRESHOLDS_PROPERTY->items + 1)
#define AUX_HUMIDITY_DRY_THRESHOLD_ITEM       (AUX_HUMIDITY_THRESHOLDS_PROPERTY->items + 2)

#define AUX_HUMIDITY_PROPERTY		            	(PRIVATE_DATA->rh_condition_property)
#define AUX_HUMIDITY_HUMID_ITEM		        		(AUX_HUMIDITY_PROPERTY->items + 0)
#define AUX_HUMIDITY_NORMAL_ITEM              (AUX_HUMIDITY_PROPERTY->items + 1)
#define AUX_HUMIDITY_DRY_ITEM                 (AUX_HUMIDITY_PROPERTY->items + 2)

// Wind
#define AUX_WIND_THRESHOLDS_PROPERTY      		(PRIVATE_DATA->wind_condition_thresholds_property)
#define AUX_WIND_STRONG_THRESHOLD_ITEM    		(AUX_WIND_THRESHOLDS_PROPERTY->items + 0)
#define AUX_WIND_MODERATE_THRESHOLD_ITEM  		(AUX_WIND_THRESHOLDS_PROPERTY->items + 1)

#define AUX_WIND_PROPERTY                 		(PRIVATE_DATA->wind_condition_property)
#define AUX_WIND_STRONG_ITEM              		(AUX_WIND_PROPERTY->items + 0)
#define AUX_WIND_MODERATE_ITEM            		(AUX_WIND_PROPERTY->items + 1)
#define AUX_WIND_CALM_ITEM                		(AUX_WIND_PROPERTY->items + 2)

// RAIN
#define AUX_RAIN_THRESHOLDS_PROPERTY      		(PRIVATE_DATA->rain_condition_thresholds_property)
#define AUX_RAIN_RAINING_THRESHOLD_ITEM   		(AUX_RAIN_THRESHOLDS_PROPERTY->items + 0)
#define AUX_RAIN_WET_THRESHOLD_ITEM       		(AUX_RAIN_THRESHOLDS_PROPERTY->items + 1)

#define AUX_RAIN_PROPERTY                 		(PRIVATE_DATA->rain_condition_property)
#define AUX_RAIN_RAINING_ITEM             		(AUX_RAIN_PROPERTY->items + 0)
#define AUX_RAIN_WET_ITEM                 		(AUX_RAIN_PROPERTY->items + 1)
#define AUX_RAIN_DRY_ITEM                 		(AUX_RAIN_PROPERTY->items + 2)

// CLOUD detection
#define AUX_CLOUD_THRESHOLDS_PROPERTY      		(PRIVATE_DATA->cloud_condition_thresholds_property)
#define AUX_CLOUD_CLEAR_THRESHOLD_ITEM      	(AUX_CLOUD_THRESHOLDS_PROPERTY->items + 0)
#define AUX_CLOUD_CLOUDY_THRESHOLD_ITEM     	(AUX_CLOUD_THRESHOLDS_PROPERTY->items + 1)

#define AUX_CLOUD_PROPERTY                  	(PRIVATE_DATA->cloud_condition_property)
#define AUX_CLOUD_CLEAR_ITEM                	(AUX_CLOUD_PROPERTY->items + 0)
#define AUX_CLOUD_CLOUDY_ITEM               	(AUX_CLOUD_PROPERTY->items + 1)
#define AUX_CLOUD_OVERCAST_ITEM             	(AUX_CLOUD_PROPERTY->items + 2)

// SKY drkness
#define AUX_SKY_THRESHOLDS_PROPERTY        		(PRIVATE_DATA->sky_condition_thresholds_property)
#define AUX_SKY_DARK_THRESHOLD_ITEM        		(AUX_SKY_THRESHOLDS_PROPERTY->items + 0)
#define AUX_SKY_LIGHT_THRESHOLD_ITEM       		(AUX_SKY_THRESHOLDS_PROPERTY->items + 1)

#define AUX_SKY_PROPERTY                   		(PRIVATE_DATA->sky_condition_property)
#define AUX_SKY_DARK_ITEM                  		(AUX_SKY_PROPERTY->items + 0)
#define AUX_SKY_LIGHT_ITEM                 		(AUX_SKY_PROPERTY->items + 1)
#define AUX_SKY_VERY_LIGHT_ITEM            		(AUX_SKY_PROPERTY->items + 2)

// Anemometer type
#define X_ANEMOMETER_TYPE_PROPERTY_NAME           "X_ANEMOMETER_TYPE"
#define X_ANEMOMETER_TYPE_BLACK_ITEM_NAME         "BLACK"
#define X_ANEMOMETER_TYPE_GREY_ITEM_NAME          "GREY"

#define X_ANEMOMETER_TYPE_PROPERTY           (PRIVATE_DATA->anemometer_type_property)
#define X_ANEMOMETER_TYPE_BLACK_ITEM         (X_ANEMOMETER_TYPE_PROPERTY->items + 0)
#define X_ANEMOMETER_TYPE_GREY_ITEM          (X_ANEMOMETER_TYPE_PROPERTY->items + 1)

// Rain sensor heater parameters
#define X_RAIN_SENSOR_HEATER_SETUP_PROPERTY_NAME             "X_RAIN_SENSOR_HEATER_SETUP"
#define X_RAIN_SENSOR_HEATER_TEMPETATURE_LOW_ITEM_NAME       "TEMPETATURE_LOW"
#define X_RAIN_SENSOR_HEATER_TEMPETATURE_HIGH_ITEM_NAME      "TEMPETATURE_HIGH"
#define X_RAIN_SENSOR_HEATER_DELTA_LOW_ITEM_NAME             "TEMPETATURE_DELTA_LOW"
#define X_RAIN_SENSOR_HEATER_DELTA_HIGH_ITEM_NAME            "TEMPETATURE_DELTA_HIGH"
#define X_RAIN_SENSOR_HEATER_IMPULSE_TEMPERATURE_ITEM_NAME   "HEAT_IMPULSE_TEMPETATURE"
#define X_RAIN_SENSOR_HEATER_IMPULSE_DURATION_ITEM_NAME      "HEAT_IMPULSE_DURATION"
#define X_RAIN_SENSOR_HEATER_IMPULSE_CYCLE_ITEM_NAME         "HEAT_IMPULSE_CYCLE"
#define X_RAIN_SENSOR_HEATER_MIN_POWER_ITEM_NAME             "HEATER_MIN_POWER"

#define X_RAIN_SENSOR_HEATER_SETUP_PROPERTY                  (PRIVATE_DATA->rain_sensor_heater_setup_property)
#define X_RAIN_SENSOR_HEATER_TEMPETATURE_LOW_ITEM            (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 0)
#define X_RAIN_SENSOR_HEATER_TEMPETATURE_HIGH_ITEM           (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 1)
#define X_RAIN_SENSOR_HEATER_DELTA_LOW_ITEM                  (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 2)
#define X_RAIN_SENSOR_HEATER_DELTA_HIGH_ITEM                 (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 3)
#define X_RAIN_SENSOR_HEATER_IMPULSE_TEMPERATURE_ITEM        (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 4)
#define X_RAIN_SENSOR_HEATER_IMPULSE_DURATION_ITEM           (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 5)
#define X_RAIN_SENSOR_HEATER_IMPULSE_CYCLE_ITEM              (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 6)
#define X_RAIN_SENSOR_HEATER_MIN_POWER_ITEM                  (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->items + 7)

typedef enum {
	normal,
	increasing,
	pulse
} heating_algorithm_state;

typedef struct {
	int power_voltage;
	int ir_sky_temperature;
	int ir_sensor_temperature;
	int ambient_temperature;     // In newer models there is no ambient temperature sensor so -10000 is returned.
	float rh;
	float rh_temperature;
	int rain_frequency;
	int rain_heater;
	int rain_sensor_temperature; // used as ambient temperature in models where there is no ambient temperature sensor)
	int ldr;                     // Ambient light sensor
	int switch_status;
	float wind_speed;            // The wind speed (m/s)
	float read_duration;         // Reding duration (s)
} cloudwatcher_data;

typedef struct {
	int handle;
	float firmware;
	bool udp;
	bool anemometer_black;
	bool cancel_reading;
	pthread_mutex_t port_mutex;

	heating_algorithm_state heating_state;
	time_t pulse_start_time;
	time_t wet_start_time;
	float desired_sensor_temperature;
	float sensor_heater_power;
	indigo_timer *sensors_timer;
	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *heater_control_state,
	                *sky_correction_property,
	                *constants_property,
	                *sensor_readings_property,
	                *weather_property,
	                *dew_threshold_property,
	                *wind_threshold_property,
	                *rain_threshold_property,
	                *dew_warning_property,
	                *wind_warning_property,
	                *rain_warning_property,
	                *rh_condition_thresholds_property,
	                *rh_condition_property,
	                *wind_condition_thresholds_property,
	                *wind_condition_property,
	                *rain_condition_thresholds_property,
	                *rain_condition_property,
	                *cloud_condition_thresholds_property,
	                *cloud_condition_property,
	                *sky_condition_thresholds_property,
	                *sky_condition_property,
	                *anemometer_type_property,
	                *rain_sensor_heater_setup_property,
	                *sensors_property;
} aag_private_data;

#define DEVICE_CONNECTED_MASK            0x80

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define is_connected(dev)                ((dev) && (dev)->gp_bits & DEVICE_CONNECTED_MASK)
#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define MAX_LEN 250
#define BLOCK_SIZE 15

#define ABS_ZERO 273.15
#define NUMBER_OF_READS 5
#define REFRESH_INTERVAL 15.0

/* Linatico AAG CloudWatcher device Commands ======================================================================== */
static bool aag_command(indigo_device *device, const char *command, char *response, int block_count, int sleep) {
	int max = block_count * BLOCK_SIZE;
	char c;
	char buff[MAX_LEN];
	struct timeval tv;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0)
			break;
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		if (PRIVATE_DATA->udp) {
			result = read(PRIVATE_DATA->handle, buff, MAX_LEN);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			break;
		} else {
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
		}
	}

	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (sleep > 0)
		usleep(sleep);

	// read responce
	if (response != NULL) {
		int index = 0;
		int timeout = 3;
		while (index < max) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			timeout = 1;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			if (PRIVATE_DATA->udp) {
				result = read(PRIVATE_DATA->handle, response, MAX_LEN);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				index = (int)result;
				break;
			} else {
				result = read(PRIVATE_DATA->handle, &c, 1);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				response[index++] = c;
			}
		}
		if ((index > BLOCK_SIZE) && (response[index - BLOCK_SIZE] == '!')) {
			response[index - BLOCK_SIZE] = '\0';
		} else {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			response[index-1] = '\0';
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wrong response %s -> %s", command, response);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


static bool aag_is_cloudwatcher(indigo_device *device, char *name) {
	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "A!", buffer, 2, 0);

	if (!r) return false;

	if (name) sscanf(buffer, "%*s %15s", name);

	const char *internal_name_block_aag = "!N CloudWatcher";
	const char *internal_name_block_pcw = "!N PocketCW";

	bool is_cw = true;
	for (int i = 0; i < 15; i++) {
		if (buffer[i] != internal_name_block_aag[i]) {
			is_cw = false;
			break;
		}
	}
	if (is_cw) return true;

	is_cw = true;
	for (int i = 0; i < 11; i++) {
		if (buffer[i] != internal_name_block_pcw[i]) {
			is_cw = false;
			break;
		}
	}
	return is_cw;
}

//static bool aag_reset_buffers(indigo_device *device) {
//	bool r = aag_command(device, "z!", NULL, 0, 0);
//	if (!r) return false;
//	return true;
//}

static bool aag_get_firmware_version(indigo_device *device, char *version) {
	if (version == NULL) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "B!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!V %4s", version);
	if (res != 1) return false;

	PRIVATE_DATA->firmware = atof(version);
	return true;
}


static bool aag_get_serial_number(indigo_device *device, char *serial_number) {
	if (serial_number == NULL) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "K!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!K %4s", serial_number);
	if (res != 1) {
		serial_number[0]='\0';
		return false;
	}

	char *s = serial_number;
	while (*s) {
		if (isdigit(*s++) == 0) {
			serial_number[0]='\0';
			return false;
		}
	}

	return true;
}


static bool aag_get_values(indigo_device *device, int *power_voltage, int *ambient_temperature, int *ldr_value, int *rain_sensor_temperature) {
	int zener_v;
	int ambient_temp = NO_READING;
	int ldr_r;
	int rain_sensor_temp;

	if (PRIVATE_DATA->firmware >= 3.0) {
		char buffer[BLOCK_SIZE * 4];
		bool r = aag_command(device, "C!", buffer, 4, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!4 %d!5 %d", &zener_v, &ldr_r, &rain_sensor_temp);
		if (res != 3) return false;
	} else {
		char buffer[BLOCK_SIZE * 5];
		bool r = aag_command(device, "C!", buffer, 5, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!3 %d!4 %d!5 %d", &zener_v, &ambient_temp, &ldr_r, &rain_sensor_temp);

		if (res != 4) return false;
	}

	*power_voltage           = zener_v;
	*ambient_temperature     = ambient_temp;
	*ldr_value               = ldr_r;
	*rain_sensor_temperature = rain_sensor_temp;

	INDIGO_DRIVER_DEBUG(
		DRIVER_NAME,
		"Values: version = %f, power_voltage = %d, ambient_temperature = %d, ldr_value = %d, rain_sensor_temperature = %d",
		PRIVATE_DATA->firmware,
		*power_voltage,
		*ambient_temperature,
		*ldr_value,
		*rain_sensor_temperature
	);

	return true;
}


static bool aag_get_ir_sky_temperature(indigo_device *device, int *temp) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "S!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!1 %d", temp);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "temp = %d", *temp);
	return true;
}


static bool aag_get_sensor_temperature(indigo_device *device, int *temp) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "T!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!2 %d", temp);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "temp = %d", *temp);
	return true;
}


static bool aag_get_rain_frequency(indigo_device *device, int *rain_freqency) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "E!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!R %d", rain_freqency);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "rain_freqency = %d", *rain_freqency);
	return true;
}


static bool set_pwm_duty_cycle(indigo_device *device, int pwm_duty_cycle) {
	if (pwm_duty_cycle < 0) pwm_duty_cycle = 0;
	if (pwm_duty_cycle > 1023) pwm_duty_cycle = 1023;

	int new_pwm = pwm_duty_cycle;
	char command[7] = "Pxxxx!";

	int n = new_pwm / 1000;
	command[1] = n + '0';
	new_pwm = new_pwm - n * 1000;

	n = new_pwm / 100;
	command[2] = n + '0';
	new_pwm = new_pwm - n * 100;

	n = new_pwm / 10;
	command[3] = n + '0';
	new_pwm = new_pwm - n * 10;

	command[4] = new_pwm + '0';

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, command, buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!Q %d", &new_pwm);
	if (res != 1) return false;

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pwm_duty_cycle = %d, new_pwm = %d", pwm_duty_cycle, new_pwm);

	if (new_pwm != pwm_duty_cycle) return false;
	return true;
}


static bool aag_get_pwm_duty_cycle(indigo_device *device, int *pwm_duty_cycle) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "Q!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!Q %d", pwm_duty_cycle);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pwm_duty_cycle = %d", *pwm_duty_cycle);
	return true;
}


static bool aag_open_swith(indigo_device *device) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "G!", buffer, 2, 0);
	if (!r) return false;

	if (buffer[1] != 'X') return false;
	return true;
}


static bool aag_close_swith(indigo_device *device) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "H!", buffer, 2, 0);
	if (!r) return false;

	if (buffer[1] != 'Y') return false;
	return true;
}


static bool aag_get_swith(indigo_device *device, bool *closed) {
	char buffer[BLOCK_SIZE * 2];

	bool r = aag_command(device, "F!", buffer, 2, 0);
	if (!r) return false;

	if (buffer[1] == 'Y') {
		*closed = true;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSED = TRUE");
	}
	else if (buffer[1] == 'X') {
		*closed = false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSED = FALSE");
	}
	else {
		return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSED = UNKNOWN");
	}
	return true;
}


static bool aag_get_rh_temperature(indigo_device *device, float *rh, float *temperature) {
	if (PRIVATE_DATA->firmware < 5.6) return false;

	int rhi, tempi;
	char buffer[BLOCK_SIZE * 2];

	bool precise = true;
	bool r = aag_command(device, "t!", buffer, 2, 0);
	if (!r) return false;
	int res = sscanf(buffer, "!th%d", &tempi);
	if (res != 1) {
		res = sscanf(buffer, "!t %d", &tempi);
		if (res != 1) return false;
		precise = false;
	}
	if (precise) {
		*temperature = ((tempi * 175.72) / 65536) - 46.85;
	} else {
		*temperature = (tempi * 1.7572) - 46.85;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "tempi = %d", tempi);

	if (*temperature < -50 || *temperature > 80) return false;

	precise = true;
	r = aag_command(device, "h!", buffer, 2, 0);
	if (!r) return false;
	res = sscanf(buffer, "!hh%d", &rhi);
	if (res != 1) {
		res = sscanf(buffer, "!h %d", &rhi);
		if (res != 1) return false;
		precise = false;
	}

	if (precise) {
		*rh = ((rhi * 125) / 65536) - 6;
	} else {
		*rh = ((rhi * 1.7572) / 100) - 6;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "rhi = %d", rhi);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "temperature = %f, rh = %f", *temperature, *rh);
	return true;
}


static bool aag_get_electrical_constants(
		indigo_device *device,
		double *zener_constant,
		double *ldr_max_res,
		double *ldr_pull_up_res,
		double *rain_beta,
		double *rain_res_at_25,
		double *rain_pull_up_res
	) {

	if (PRIVATE_DATA->firmware < 3.0) return false;

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "M!", buffer, 2, 0);
	if (!r) return false;

	if (buffer[1] != 'M') return false;

	*zener_constant    = (256 * buffer[2] + buffer[3]) / 100.0;
	*ldr_max_res       = (256 * buffer[4] + buffer[5]) / 1.0;
	*ldr_pull_up_res   = (256 * buffer[6] + buffer[7]) / 10.0;
	*rain_beta         = (256 * buffer[8] + buffer[9]) / 1.0;
	*rain_res_at_25    = (256 * buffer[10] + buffer[11]) / 10.0;
	*rain_pull_up_res  = (256 * buffer[12] + buffer[13]) / 10.0;

	INDIGO_DRIVER_DEBUG(
		DRIVER_NAME,
		"zener_constant = %f, ldr_max_res = %f, ldr_pull_up_res = %f, rain_beta = %f, rain_res_at_25 = %f, rain_pull_up_res = %f",
		*zener_constant,
		*ldr_max_res,
		*ldr_pull_up_res,
		*rain_beta,
		*rain_res_at_25,
		*rain_pull_up_res
	);

	return true;
}


static bool aag_is_anemometer_present(indigo_device *device, bool *present) {
	if (!present) return false;

	if (PRIVATE_DATA->firmware < 5.0) {
		*present = false;
		return true;
	}

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "v!", buffer, 2, 0);
	if (!r) return false;

	int ipresent;
	int res = sscanf(buffer, "!v %d", &ipresent);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ipresent = %d", ipresent);
	*present = (ipresent == 1) ? true : false;

	return true;
}


static bool aag_get_wind_speed(indigo_device *device, float *wind_speed) {
	if (!wind_speed) return false;

	if (PRIVATE_DATA->firmware < 5.0) {
		*wind_speed = 0;
		return true;
	}

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "V!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!w %f", wind_speed);
	if (res != 1) return false;

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "raw_wind_speed = %f", *wind_speed);

	if (PRIVATE_DATA->anemometer_black && *wind_speed != 0) *wind_speed = *wind_speed * 0.84 + 3;
	*wind_speed /= 3.6; //Convert to m/s

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "wind_speed = %f", *wind_speed);
	return true;
}


static float aggregate_floats(float values[], int num) {
	float average = 0.0;

	for (int i = 0; i < num; i++) {
		average += values[i];
	}

	average /= num;

	float std_dev = 0.0;

	for (int i = 0; i < num; i++) {
		std_dev += (values[i] - average) * (values[i] - average);
	}

	std_dev /= num;
	std_dev = sqrt(std_dev);

	float avg  = 0.0;
	int count = 0;

	for (int i = 0; i < num; i++) {
		if (fabs(values[i] - average) <= std_dev) {
			avg += values[i];
			count++;
		}
	}
	avg /= count;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Average: %f\n", avg);

	return avg;
}


static int aggregate_integers(int values[], int num) {
	float fvalues[num];
	for (int i = 0; i < num; i++) {
		fvalues[i] = (float)values[i];
	}
	return (int)aggregate_floats(fvalues, num);
}


static bool aag_get_cloudwatcher_data(indigo_device *device, cloudwatcher_data *cwd) {
	int sky_temperature[NUMBER_OF_READS];
	int ir_sensor_temperature[NUMBER_OF_READS];
	int rain_frequesncy[NUMBER_OF_READS];
	int internal_supply_voltage[NUMBER_OF_READS];
	int ambient_temperature[NUMBER_OF_READS];
	int ldr_value[NUMBER_OF_READS];
	int rain_sensor_temperature[NUMBER_OF_READS];
	float wind_speed[NUMBER_OF_READS];

	int check = 0;
	cwd->read_duration = 0;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < NUMBER_OF_READS; i++) {
		check = aag_get_ir_sky_temperature(device, &sky_temperature[i]);
		if (!check) return false;

		check = aag_get_sensor_temperature(device, &ir_sensor_temperature[i]);
		if (!check) return false;

		check = aag_get_rain_frequency(device, &rain_frequesncy[i]);
		if (!check) return false;

		check = aag_get_values(device, &internal_supply_voltage[i], &ambient_temperature[i], &ldr_value[i], &rain_sensor_temperature[i]);
		if (!check) return false;

		check = aag_get_wind_speed(device, &wind_speed[i]);
		if (!check) return false;

		if (PRIVATE_DATA->cancel_reading) return false;
	}

	if (!aag_get_rh_temperature(device, &cwd->rh, &cwd->rh_temperature)) {
		cwd->rh_temperature = NO_READING;
		cwd->rh = NO_READING;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	cwd->read_duration = (float)(end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;

	cwd->ir_sky_temperature      = aggregate_integers(sky_temperature, NUMBER_OF_READS);
	cwd->ir_sensor_temperature   = aggregate_integers(ir_sensor_temperature, NUMBER_OF_READS);
	cwd->rain_frequency          = aggregate_integers(rain_frequesncy, NUMBER_OF_READS);
	cwd->power_voltage           = aggregate_integers(internal_supply_voltage, NUMBER_OF_READS);
	cwd->ambient_temperature     = aggregate_integers(ambient_temperature, NUMBER_OF_READS);
	cwd->ldr                     = aggregate_integers(ldr_value, NUMBER_OF_READS);
	cwd->rain_sensor_temperature = aggregate_integers(rain_sensor_temperature, NUMBER_OF_READS);
	cwd->wind_speed              = aggregate_floats(wind_speed, NUMBER_OF_READS);

	check = aag_get_pwm_duty_cycle(device, &cwd->rain_heater);
	if (!check) return false;

	return true;
}


static bool aag_populate_constants(indigo_device *device) {
	bool res1 = aag_get_electrical_constants(
		device,
		&X_CONSTANTS_ZENER_VOLTAGE_ITEM->number.value,
		&X_CONSTANTS_LDR_MAX_R_ITEM->number.value,
		&X_CONSTANTS_LDR_PULLUP_R_ITEM->number.value,
		&X_CONSTANTS_RAIN_BETA_ITEM->number.value,
		&X_CONSTANTS_RAIN_R_AT_25_ITEM->number.value,
		&X_CONSTANTS_RAIN_PULLUP_R_ITEM->number.value
	);
	X_CONSTANTS_AMBIENT_BETA_ITEM->number.value = 3811;
	X_CONSTANTS_AMBIENT_R_AT_25_ITEM->number.value = 10;
	X_CONSTANTS_AMBIENT_PULLUP_R_ITEM->number.value = 9.9;

	bool present = false;
	bool res2 = aag_is_anemometer_present(device, &present);
	X_CONSTANTS_ANEMOMETER_STATE_ITEM->number.value = (double)present;

	if (res1 && res2) {
		X_CONSTANTS_PROPERTY ->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CONSTANTS_PROPERTY, NULL);
		return true;
	}
	X_CONSTANTS_PROPERTY ->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, X_CONSTANTS_PROPERTY, "Failed reading device constants");
	indigo_update_property(device, X_CONSTANTS_PROPERTY, NULL);
	return false;
}


bool process_data_and_update(indigo_device *device, cloudwatcher_data data) {
	// Rain sensor temperature
	float rain_sensor_temp = data.rain_sensor_temperature;
	if (rain_sensor_temp > 1022) {
		rain_sensor_temp = 1022;
	} else if (rain_sensor_temp < 1) {
		rain_sensor_temp = 1;
	}
	rain_sensor_temp = X_CONSTANTS_RAIN_PULLUP_R_ITEM->number.value / ((1023.0 / rain_sensor_temp) - 1.0);
	rain_sensor_temp = log(rain_sensor_temp / X_CONSTANTS_RAIN_R_AT_25_ITEM->number.value);
	X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM->number.value =
		1.0 / (rain_sensor_temp / X_CONSTANTS_RAIN_BETA_ITEM->number.value + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;

	// Rain and warning
	X_SENSOR_RAIN_CYCLES_ITEM->number.value = data.rain_frequency;
	if (AUX_RAIN_THRESHOLD_SENSOR_1_ITEM->number.value > data.rain_frequency) {
		AUX_RAIN_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_RAIN_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
	} else {
		AUX_RAIN_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_RAIN_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_RAIN_WARNING_PROPERTY, NULL);

	if (data.rain_frequency <= AUX_RAIN_RAINING_THRESHOLD_ITEM->number.value) {
		AUX_RAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_RAIN_PROPERTY, AUX_RAIN_RAINING_ITEM, true);
	} else if (data.rain_frequency <= AUX_RAIN_WET_THRESHOLD_ITEM->number.value) {
		AUX_RAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_RAIN_PROPERTY, AUX_RAIN_WET_ITEM, true);
	} else {
		AUX_RAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_RAIN_PROPERTY, AUX_RAIN_DRY_ITEM, true);
	}
	indigo_update_property(device, AUX_RAIN_PROPERTY, NULL);

	// Rain heater
	X_SENSOR_RAIN_HEATER_POWER_ITEM->number.value = 100.0 * data.rain_heater / 1023.0;

	// Ambient light
	float ambient_light = (float)data.ldr;
	if (ambient_light > 1022.0) {
		ambient_light = 1022.0;
	} else if (ambient_light < 1) {
		ambient_light = 1.0;
	}
	X_SENSOR_SKY_BRIGHTNESS_ITEM->number.value = X_CONSTANTS_LDR_PULLUP_R_ITEM->number.value / ((1023.0 / ambient_light) - 1.0);

	if (X_SENSOR_SKY_BRIGHTNESS_ITEM->number.value >= AUX_SKY_DARK_THRESHOLD_ITEM->number.value) {
		AUX_SKY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_SKY_PROPERTY, AUX_SKY_DARK_ITEM, true);
	} else if (X_SENSOR_SKY_BRIGHTNESS_ITEM->number.value >= AUX_SKY_LIGHT_THRESHOLD_ITEM->number.value) {
		AUX_SKY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_SKY_PROPERTY, AUX_SKY_LIGHT_ITEM, true);
	} else {
		AUX_SKY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_SKY_PROPERTY, AUX_SKY_VERY_LIGHT_ITEM, true);
	}
	indigo_update_property(device, AUX_SKY_PROPERTY, NULL);

	// Ambient temperature and dewpoint
	float ambient_temperature = data.ambient_temperature;
	if (ambient_temperature < -200) {
		if (data.rh_temperature < -200) {
			ambient_temperature = data.ir_sensor_temperature / 100.0;
		} else {
			ambient_temperature = data.rh_temperature;
		}
	} else {
		if (ambient_temperature > 1022) {
			ambient_temperature = 1022;
		} else if (ambient_temperature < 1) {
			ambient_temperature = 1;
		}
		ambient_temperature = X_CONSTANTS_AMBIENT_PULLUP_R_ITEM->number.value / ((1023.0 / ambient_temperature) - 1.0);
		ambient_temperature = log(ambient_temperature / X_CONSTANTS_AMBIENT_R_AT_25_ITEM->number.value);
		ambient_temperature =
			1.0 / (ambient_temperature / X_CONSTANTS_AMBIENT_BETA_ITEM->number.value + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;
	}
	X_SENSOR_AMBIENT_TEMPERATURE_ITEM->number.value = AUX_WEATHER_TEMPERATURE_ITEM->number.value = ambient_temperature;
	if (data.rh != NO_READING) {
		AUX_WEATHER_HUMIDITY_ITEM->number.value = data.rh;
		AUX_WEATHER_DEWPOINT_ITEM->number.value = ambient_temperature - (100.0 - data.rh) / 5.0;
	} else {
		AUX_WEATHER_DEWPOINT_ITEM->number.value = -ABS_ZERO;
		AUX_WEATHER_HUMIDITY_ITEM->number.value = 0;
	}

	// Dew point warning
	if (data.rh == NO_READING) { // no sensor
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_IDLE_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;
	} else if (ambient_temperature - AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value <= AUX_WEATHER_DEWPOINT_ITEM->number.value) {
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
	} else {
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_DEW_WARNING_PROPERTY, NULL);

	if (data.rh == NO_READING) {
		AUX_HUMIDITY_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_HUMID_ITEM, false);
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_NORMAL_ITEM, false);
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_DRY_ITEM, false);
	} else if (data.rh >= AUX_HUMIDITY_HUMID_THRESHOLD_ITEM->number.value) {
		AUX_HUMIDITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_HUMID_ITEM, true);
	} else if (data.rh >= AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM->number.value) {
		AUX_HUMIDITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_NORMAL_ITEM, true);
	} else {
		AUX_HUMIDITY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_HUMIDITY_PROPERTY, AUX_HUMIDITY_DRY_ITEM, true);
	}
	indigo_update_property(device, AUX_HUMIDITY_PROPERTY, NULL);

	// Sky temperature
	float sky_temperature = data.ir_sky_temperature / 100.0;
	float ir_sensor_temperature = data.ir_sensor_temperature / 100.0;
	X_SENSOR_RAW_SKY_TEMPERATURE_ITEM->number.value = sky_temperature;
	X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM->number.value = ir_sensor_temperature;
	float k1 = X_SKY_CORRECTION_K1_ITEM->number.value;
	float k2 = X_SKY_CORRECTION_K2_ITEM->number.value;
	float k3 = X_SKY_CORRECTION_K3_ITEM->number.value;
	float k4 = X_SKY_CORRECTION_K4_ITEM->number.value;
	float k5 = X_SKY_CORRECTION_K5_ITEM->number.value;
	X_SENSOR_SKY_TEMPERATURE_ITEM->number.value = AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM->number.value =
		sky_temperature - ((k1 / 100.0) * (ir_sensor_temperature - k2 / 10.0) +
		                   (k3 / 100.0) * pow(exp(k4 / 1000 * ir_sensor_temperature), (k5 / 100.0)));

	if (AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM->number.value <= AUX_CLOUD_CLEAR_THRESHOLD_ITEM->number.value) {
		AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_CLEAR_ITEM, true);
	} else if (AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM->number.value <= AUX_CLOUD_CLOUDY_THRESHOLD_ITEM->number.value) {
		AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_CLOUDY_ITEM, true);
	} else {
		AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_OVERCAST_ITEM, true);
	}
	indigo_update_property(device, AUX_CLOUD_PROPERTY, NULL);

	// Wind speed and warning
	AUX_WEATHER_WIND_SPEED_ITEM->number.value = data.wind_speed;
	if (X_CONSTANTS_ANEMOMETER_STATE_ITEM->number.value < 1) {
		AUX_WIND_WARNING_PROPERTY->state = INDIGO_IDLE_STATE;
		AUX_WIND_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;
	} else if (AUX_WIND_THRESHOLD_SENSOR_1_ITEM->number.value < data.wind_speed) {
		AUX_WIND_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_WIND_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
	} else {
		AUX_WIND_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_WIND_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_WIND_WARNING_PROPERTY, NULL);

	if (X_CONSTANTS_ANEMOMETER_STATE_ITEM->number.value < 1) {
		AUX_WIND_PROPERTY->state = INDIGO_IDLE_STATE;
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_STRONG_ITEM, false);
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_MODERATE_ITEM, false);
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_CALM_ITEM, false);
	} else if (data.wind_speed >= AUX_WIND_STRONG_THRESHOLD_ITEM->number.value) {
		AUX_WIND_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_STRONG_ITEM, true);
	} else if (data.wind_speed >= AUX_WIND_MODERATE_THRESHOLD_ITEM->number.value) {
		AUX_WIND_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_MODERATE_ITEM, true);
	} else {
		AUX_WIND_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(AUX_WIND_PROPERTY, AUX_WIND_CALM_ITEM, true);
	}
	indigo_update_property(device, AUX_WIND_PROPERTY, NULL);

	X_SENSOR_READINGS_PROPERTY->state = INDIGO_OK_STATE;
	AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_SENSOR_READINGS_PROPERTY, NULL);
	indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);

	return true;
}


// --------------------------------------------------------------------------------- Common stuff
static bool aag_open(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OPEN REQUESTED: %d -> %d", PRIVATE_DATA->handle, DEVICE_CONNECTED);
	if (DEVICE_CONNECTED) return false;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}

	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "aag")) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening local device on port: '%s', baudrate = %d", DEVICE_PORT_ITEM->text.value, atoi(DEVICE_BAUDRATE_ITEM->text.value));
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
		PRIVATE_DATA->udp = false;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening network device on host: %s", DEVICE_PORT_ITEM->text.value);
		indigo_network_protocol proto = INDIGO_PROTOCOL_UDP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 10000, &proto);
		PRIVATE_DATA->udp = true;
	}
	if (PRIVATE_DATA->handle < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_global_unlock(device);
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return false;
	}
	set_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}


static void aag_close(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CLOSE REQUESTED: %d -> %d", PRIVATE_DATA->handle, DEVICE_CONNECTED);
	if (!DEVICE_CONNECTED) return;

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d)", PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	PRIVATE_DATA->handle = 0;
	clear_connected_flag(device);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
}

// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static int aag_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_PORTS
	DEVICE_PORTS_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
	DEVICE_BAUDRATE_PROPERTY->hidden = true;
	indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 8;
	// -------------------------------------------------------------------------------- GPIO OUTLETS
	AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, SWITCH_GROUP, "Switch outlet", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	if (AUX_GPIO_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Switch", false);
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, SWITCH_GROUP, "Switch name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Internal switch", "Switch");
	// -------------------------------------------------------------------------------- X_HEATER_CONTROL_STATE
	X_HEATER_CONTROL_STATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_HEATER_CONTROL_STATE_PROPERTY_NAME, STATUS_GROUP, "Heater control state", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (X_HEATER_CONTROL_STATE_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(X_HEATER_CONTROL_NORMAL_ITEM, X_HEATER_CONTROL_NORMAL_ITEM_NAME, "Normal", false);
	indigo_init_switch_item(X_HEATER_CONTROL_INCREASE_ITEM, X_HEATER_CONTROL_INCREASE_ITEM_NAME, "Inscreasing", false);
	indigo_init_switch_item(X_HEATER_CONTROL_PULSE_ITEM, X_HEATER_CONTROL_PULSE_ITEM_NAME, "Pulse", false);
	// -------------------------------------------------------------------------------- X_SKY_CORRECTION
	X_SKY_CORRECTION_PROPERTY = indigo_init_number_property(NULL, device->name, X_SKY_CORRECTION_PROPERTY_NAME, SETTINGS_GROUP, "Sky temperature correction", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
	if (X_SKY_CORRECTION_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_SKY_CORRECTION_K1_ITEM, X_SKY_CORRECTION_K1_ITEM_NAME, X_SKY_CORRECTION_K1_ITEM_NAME, -999, 999, 0, 3);
	indigo_init_number_item(X_SKY_CORRECTION_K2_ITEM, X_SKY_CORRECTION_K2_ITEM_NAME, X_SKY_CORRECTION_K2_ITEM_NAME, -999, 999, 0, 0);
	indigo_init_number_item(X_SKY_CORRECTION_K3_ITEM, X_SKY_CORRECTION_K3_ITEM_NAME, X_SKY_CORRECTION_K3_ITEM_NAME, -999, 999, 0, 4);
	indigo_init_number_item(X_SKY_CORRECTION_K4_ITEM, X_SKY_CORRECTION_K4_ITEM_NAME, X_SKY_CORRECTION_K4_ITEM_NAME, -999, 999, 0, 100);
	indigo_init_number_item(X_SKY_CORRECTION_K5_ITEM, X_SKY_CORRECTION_K5_ITEM_NAME, X_SKY_CORRECTION_K5_ITEM_NAME, -999, 999, 0, 100);
	// -------------------------------------------------------------------------------- X_CONSTANTS
	X_CONSTANTS_PROPERTY = indigo_init_number_property(NULL, device->name, X_CONSTANTS_PROPERTY_NAME, STATUS_GROUP, "Device Constants", INDIGO_OK_STATE, INDIGO_RO_PERM, 10);
	if (X_CONSTANTS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_CONSTANTS_ZENER_VOLTAGE_ITEM, X_CONSTANTS_ZENER_VOLTAGE_ITEM_NAME, "Zener voltage (V)", -100, 100, 0, 3);
	indigo_init_number_item(X_CONSTANTS_LDR_MAX_R_ITEM, X_CONSTANTS_LDR_MAX_R_ITEM_NAME, "LDR max R (kΩ)", 0, 100000, 0, 1744);
	indigo_init_number_item(X_CONSTANTS_LDR_PULLUP_R_ITEM, X_CONSTANTS_LDR_PULLUP_R_ITEM_NAME, "LDR Pullup R (kΩ)", 0, 100000, 0, 56);
	indigo_init_number_item(X_CONSTANTS_RAIN_BETA_ITEM, X_CONSTANTS_RAIN_BETA_ITEM_NAME, "Rain Beta factor", -100000, 100000, 0, 3450);
	indigo_init_number_item(X_CONSTANTS_RAIN_R_AT_25_ITEM, X_CONSTANTS_RAIN_R_AT_25_ITEM_NAME, "Rain R at 25°C (kΩ)", 0, 100000, 0, 1);
	indigo_init_number_item(X_CONSTANTS_RAIN_PULLUP_R_ITEM, X_CONSTANTS_RAIN_PULLUP_R_ITEM_NAME, "Rain Pullup R (kΩ)", 0, 100000, 0, 1);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_BETA_ITEM, X_CONSTANTS_AMBIENT_BETA_ITEM_NAME, "Ambient Beta factor", -100000, 100000, 0, 3811);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_R_AT_25_ITEM, X_CONSTANTS_AMBIENT_R_AT_25_ITEM_NAME, "Ambient R at 25°C (kΩ)", 0, 100000, 0, 10);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_PULLUP_R_ITEM, X_CONSTANTS_AMBIENT_PULLUP_R_ITEM_NAME, "Ambient Pullup R (kΩ)", 0, 100000, 0, 9.9);
	indigo_init_number_item(X_CONSTANTS_ANEMOMETER_STATE_ITEM, X_CONSTANTS_ANEMOMETER_STATE_ITEM_NAME, "Anemometer Status", 0, 10, 0, 0);
	// -------------------------------------------------------------------------------- X_SENSOR_READINGS_PROPERTY
	X_SENSOR_READINGS_PROPERTY = indigo_init_number_property(NULL, device->name, X_SENSOR_READINGS_PROPERTY_NAME, WEATHER_GROUP, "Sensor Readings", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 8);
	if (X_SENSOR_READINGS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_SENSOR_RAW_SKY_TEMPERATURE_ITEM, X_SENSOR_RAW_SKY_TEMPERATURE_ITEM_NAME, "Raw infrared sky temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(X_SENSOR_RAW_SKY_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(X_SENSOR_SKY_TEMPERATURE_ITEM, X_SENSOR_SKY_TEMPERATURE_ITEM_NAME, "Infrared sky temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(X_SENSOR_SKY_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM, X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM_NAME, "Infrared sensor temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(X_SENSOR_IR_SENSOR_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(X_SENSOR_RAIN_CYCLES_ITEM, X_SENSOR_RAIN_CYCLES_ITEM_NAME, "Rain (cycles)", 0, 100000, 0, 0);
	indigo_copy_value(X_SENSOR_RAIN_CYCLES_ITEM->number.format, "%.0f");
	indigo_init_number_item(X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM, X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM_NAME, "Rain sensor temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(X_SENSOR_RAIN_HEATER_POWER_ITEM, X_SENSOR_RAIN_HEATER_POWER_ITEM_NAME, "Rain sensor heater power (%)", 0, 100, 1, 0);
	indigo_copy_value(X_SENSOR_RAIN_HEATER_POWER_ITEM->number.format, "%.0f");
	indigo_init_number_item(X_SENSOR_SKY_BRIGHTNESS_ITEM, X_SENSOR_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness (kΩ)", 0, 100000, 1, 0);
	indigo_copy_value(X_SENSOR_SKY_BRIGHTNESS_ITEM->number.format, "%.0f");
	indigo_init_number_item(X_SENSOR_AMBIENT_TEMPERATURE_ITEM, X_SENSOR_AMBIENT_TEMPERATURE_ITEM_NAME, "Ambient temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(X_SENSOR_AMBIENT_TEMPERATURE_ITEM->number.format, "%.1f");
	// -------------------------------------------------------------------------------- DEW_THRESHOLD
	AUX_DEW_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_DEW_THRESHOLD_PROPERTY_NAME, THRESHOLDS_GROUP, "Dew warning threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_DEW_THRESHOLD_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_1_ITEM, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME, "Temperature difference (°C)", 0, 9, 0, 2);
	// -------------------------------------------------------------------------------- WIND_THRESHOLD
	AUX_WIND_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WIND_THRESHOLD_PROPERTY_NAME, THRESHOLDS_GROUP, "Wind warning threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_WIND_THRESHOLD_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_WIND_THRESHOLD_SENSOR_1_ITEM, AUX_WIND_THRESHOLD_SENSOR_1_ITEM_NAME, "Wind speed (m/s)", 0, 50, 0, 6);
	// -------------------------------------------------------------------------------- RAIN_THRESHOLD
	AUX_RAIN_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_RAIN_THRESHOLD_PROPERTY_NAME, THRESHOLDS_GROUP, "Rain warning threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_WIND_THRESHOLD_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_RAIN_THRESHOLD_SENSOR_1_ITEM, AUX_RAIN_THRESHOLD_SENSOR_1_ITEM_NAME, "Rain (cycles)", 0, 100000, 1, 400);
	// -------------------------------------------------------------------------------- DEW_WARNING
	AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WARNINGS_GROUP, "Dew warning", INDIGO_BUSY_STATE, 1);
	if (AUX_DEW_WARNING_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning", INDIGO_IDLE_STATE);
	// -------------------------------------------------------------------------------- RAIN_WARNING
	AUX_RAIN_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_RAIN_WARNING_PROPERTY_NAME, WARNINGS_GROUP, "Rain warning", INDIGO_BUSY_STATE, 1);
	if (AUX_RAIN_WARNING_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_light_item(AUX_RAIN_WARNING_SENSOR_1_ITEM, AUX_RAIN_WARNING_SENSOR_1_ITEM_NAME, "Rain warning", INDIGO_IDLE_STATE);
	// -------------------------------------------------------------------------------- WIND_WARNING
	AUX_WIND_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_WIND_WARNING_PROPERTY_NAME, WARNINGS_GROUP, "Wind warning", INDIGO_BUSY_STATE, 1);
	if (AUX_WIND_WARNING_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_light_item(AUX_WIND_WARNING_SENSOR_1_ITEM, AUX_WIND_WARNING_SENSOR_1_ITEM_NAME, "Wind warning", INDIGO_IDLE_STATE);
	// -------------------------------------------------------------------------------- AUX_HUMIDITY_THRESHOLDS
	AUX_HUMIDITY_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HUMIDITY_THRESHOLDS_PROPERTY_NAME, THRESHOLDS_GROUP, "Relative humidity thresholds (%)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_HUMIDITY_THRESHOLDS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_HUMIDITY_HUMID_THRESHOLD_ITEM, AUX_HUMIDITY_HUMID_ITEM_NAME, "Humid (more than)", 0, 100, 0, 60);
	indigo_init_number_item(AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM, AUX_HUMIDITY_NORMAL_ITEM_NAME, "Normal (more than)", 0, 100, 0, 30);
	// -------------------------------------------------------------------------------- AUX_WIND_THRESHOLDS
	AUX_WIND_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WIND_THRESHOLDS_PROPERTY_NAME, THRESHOLDS_GROUP, "Wind thresholds (m/s)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_WIND_THRESHOLDS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_WIND_STRONG_THRESHOLD_ITEM, AUX_WIND_STRONG_ITEM_NAME, "Strong wind (more than)", 0, 100, 0, 10);
	indigo_init_number_item(AUX_WIND_MODERATE_THRESHOLD_ITEM, AUX_WIND_MODERATE_ITEM_NAME, "Moderate wind (more than)", 0, 100, 0, 1);
	// -------------------------------------------------------------------------------- AUX_RAIN_THRESHOLDS
	AUX_RAIN_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_RAIN_THRESHOLDS_PROPERTY_NAME, THRESHOLDS_GROUP, "Rain sensor thresholds (kΩ)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_RAIN_THRESHOLDS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_RAIN_RAINING_THRESHOLD_ITEM, AUX_RAIN_RAINING_ITEM_NAME, "Raining (less than)", 0, 100000, 0, 400);
	indigo_init_number_item(AUX_RAIN_WET_THRESHOLD_ITEM, AUX_RAIN_WET_ITEM_NAME, "Wet (less than)", 0, 100000, 0, 1700);
	// -------------------------------------------------------------------------------- AUX_CLOUD_THRESHOLDS
	AUX_CLOUD_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_CLOUD_THRESHOLDS_PROPERTY_NAME, THRESHOLDS_GROUP, "Cloud thresholds (°C)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_CLOUD_THRESHOLDS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_CLOUD_CLEAR_THRESHOLD_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear (less than)", -200, 80, 0, -15);
	indigo_init_number_item(AUX_CLOUD_CLOUDY_THRESHOLD_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy (less than)", -200, 80, 0, 0);
	// -------------------------------------------------------------------------------- AUX_SKY_THRESHOLDS
	AUX_SKY_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_SKY_THRESHOLDS_PROPERTY_NAME, THRESHOLDS_GROUP, "Sky darkness threshold (kΩ)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_SKY_THRESHOLDS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_SKY_DARK_THRESHOLD_ITEM, AUX_SKY_DARK_ITEM_NAME, "Dark (more than)", 0, 100000, 0, 2100);
	indigo_init_number_item(AUX_SKY_LIGHT_THRESHOLD_ITEM, AUX_SKY_LIGHT_ITEM_NAME, "Light (more than)", 0, 100000, 0, 6);
	// -------------------------------------------------------------------------------- AUX_HUMIDITY
	AUX_HUMIDITY_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_HUMIDITY_PROPERTY_NAME, WEATHER_GROUP, "Humidity condition", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (AUX_HUMIDITY_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_HUMIDITY_HUMID_ITEM, AUX_HUMIDITY_HUMID_ITEM_NAME, "Humid", false);
	indigo_init_switch_item(AUX_HUMIDITY_NORMAL_ITEM, AUX_HUMIDITY_NORMAL_ITEM_NAME, "Normal", false);
	indigo_init_switch_item(AUX_HUMIDITY_DRY_ITEM, AUX_HUMIDITY_DRY_ITEM_NAME, "Dry", false);
	// -------------------------------------------------------------------------------- AUX_WIND
	AUX_WIND_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_WIND_PROPERTY_NAME, WEATHER_GROUP, "Wind condition", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (AUX_WIND_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_WIND_STRONG_ITEM, AUX_WIND_STRONG_ITEM_NAME, "Strong", false);
	indigo_init_switch_item(AUX_WIND_MODERATE_ITEM, AUX_WIND_MODERATE_ITEM_NAME, "Moderate", false);
	indigo_init_switch_item(AUX_WIND_CALM_ITEM, AUX_WIND_CALM_ITEM_NAME, "Calm", false);
	// -------------------------------------------------------------------------------- AUX_RAIN
	AUX_RAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_RAIN_PROPERTY_NAME, WEATHER_GROUP, "Rain condition", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (AUX_RAIN_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_RAIN_RAINING_ITEM, AUX_RAIN_RAINING_ITEM_NAME, "Raining", false);
	indigo_init_switch_item(AUX_RAIN_WET_ITEM, AUX_RAIN_WET_ITEM_NAME, "Wet", false);
	indigo_init_switch_item(AUX_RAIN_DRY_ITEM, AUX_RAIN_DRY_ITEM_NAME, "Dry", false);
	// -------------------------------------------------------------------------------- AUX_CLOUD
	AUX_CLOUD_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_CLOUD_PROPERTY_NAME, WEATHER_GROUP, "Cloud condition", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (AUX_CLOUD_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_CLOUD_CLEAR_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear", false);
	indigo_init_switch_item(AUX_CLOUD_CLOUDY_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy", false);
	indigo_init_switch_item(AUX_CLOUD_OVERCAST_ITEM, AUX_CLOUD_OVERCAST_ITEM_NAME, "Overcast", false);
	// -------------------------------------------------------------------------------- AUX_SKY
	AUX_SKY_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_SKY_PROPERTY_NAME, WEATHER_GROUP, "Sky condition", INDIGO_BUSY_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
	if (AUX_SKY_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_SKY_DARK_ITEM, AUX_SKY_DARK_ITEM_NAME, "Dark", false);
	indigo_init_switch_item(AUX_SKY_LIGHT_ITEM, AUX_SKY_LIGHT_ITEM_NAME, "Light", false);
	indigo_init_switch_item(AUX_SKY_VERY_LIGHT_ITEM, AUX_SKY_VERY_LIGHT_ITEM_NAME, "Very light", false);
	// -------------------------------------------------------------------------------- X_ANEMOMETER_TYPE
	X_ANEMOMETER_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ANEMOMETER_TYPE_PROPERTY_NAME, SETTINGS_GROUP, "Anemometer type (if present)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (X_ANEMOMETER_TYPE_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(X_ANEMOMETER_TYPE_BLACK_ITEM, X_ANEMOMETER_TYPE_BLACK_ITEM_NAME, "Black", true);
	indigo_init_switch_item(X_ANEMOMETER_TYPE_GREY_ITEM, X_ANEMOMETER_TYPE_GREY_ITEM_NAME, "Grey", false);
	PRIVATE_DATA->anemometer_black = true;
	// -------------------------------------------------------------------------------- AUX_WEATHER
	AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather conditions", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 5);
	if (AUX_WEATHER_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(AUX_WEATHER_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM, X_SENSOR_SKY_TEMPERATURE_ITEM_NAME, "Infrared sky temperature (°C)", -200, 80, 1, 0);
	indigo_copy_value(AUX_WEATHER_IR_SKY_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint (°C)", -200, 80, 1, 0);
	indigo_copy_value(AUX_WEATHER_DEWPOINT_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Relative humidity (%)", 0, 100, 0, 0);
	indigo_copy_value(AUX_WEATHER_HUMIDITY_ITEM->number.format, "%.0f");
	indigo_init_number_item(AUX_WEATHER_WIND_SPEED_ITEM, AUX_WEATHER_WIND_SPEED_ITEM_NAME, "Wind speed (m/s)", 0, 200, 0, 0);
	indigo_copy_value(AUX_WEATHER_WIND_SPEED_ITEM->number.format, "%.1f");
	// -------------------------------------------------------------------------------- X_RAIN_SENSOR_HEATER_SETUP
	X_RAIN_SENSOR_HEATER_SETUP_PROPERTY = indigo_init_number_property(NULL, device->name, X_RAIN_SENSOR_HEATER_SETUP_PROPERTY_NAME, SETTINGS_GROUP, "Rain sensor heater setup", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (X_RAIN_SENSOR_HEATER_SETUP_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_TEMPETATURE_LOW_ITEM, X_RAIN_SENSOR_HEATER_TEMPETATURE_LOW_ITEM_NAME, "Temperature low (°C)", -200, 80, 0, 0);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_TEMPETATURE_HIGH_ITEM, X_RAIN_SENSOR_HEATER_TEMPETATURE_HIGH_ITEM_NAME, "Temperature high (°C)", -200, 80, 0, 20);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_DELTA_LOW_ITEM, X_RAIN_SENSOR_HEATER_DELTA_LOW_ITEM_NAME, "Temperature delta low (°C)", -200, 80, 0, 6);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_DELTA_HIGH_ITEM, X_RAIN_SENSOR_HEATER_DELTA_HIGH_ITEM_NAME, "Temperature delta high (°C)", -200, 80, 0, 4);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_IMPULSE_TEMPERATURE_ITEM, X_RAIN_SENSOR_HEATER_IMPULSE_TEMPERATURE_ITEM_NAME, "Heat impulse temperature (°C)", -200, 80, 0, 10);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_IMPULSE_DURATION_ITEM, X_RAIN_SENSOR_HEATER_IMPULSE_DURATION_ITEM_NAME, "Heat impulse duration (s)", 0, 1500, 0, 60);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_IMPULSE_CYCLE_ITEM, X_RAIN_SENSOR_HEATER_IMPULSE_CYCLE_ITEM_NAME, "Heat impulse cycle (s)", 0, 1500, 0, 600);
	indigo_init_number_item(X_RAIN_SENSOR_HEATER_MIN_POWER_ITEM, X_RAIN_SENSOR_HEATER_MIN_POWER_ITEM_NAME, "Heater minimum power (%)", 10, 20, 0, 10);
	//---------------------------------------------------------------------------
	return INDIGO_OK;
}


static void aag_reset_properties(indigo_device *device) {
	int i;
	X_HEATER_CONTROL_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < X_HEATER_CONTROL_STATE_PROPERTY->count; i++)
		X_HEATER_CONTROL_STATE_PROPERTY->items[i].sw.value = false;

	X_CONSTANTS_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < X_CONSTANTS_PROPERTY->count; i++)
		X_CONSTANTS_PROPERTY->items[i].number.value = 0;

	X_SENSOR_READINGS_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < X_SENSOR_READINGS_PROPERTY->count; i++)
		X_SENSOR_READINGS_PROPERTY->items[i].number.value = 0;

	AUX_WEATHER_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_WEATHER_PROPERTY->count; i++)
		AUX_WEATHER_PROPERTY->items[i].number.value = 0;

	AUX_DEW_WARNING_PROPERTY->state = INDIGO_BUSY_STATE;
	AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;

	AUX_RAIN_WARNING_PROPERTY->state = INDIGO_BUSY_STATE;
	AUX_RAIN_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;

	AUX_WIND_WARNING_PROPERTY->state = INDIGO_BUSY_STATE;
	AUX_WIND_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;

	AUX_HUMIDITY_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_HUMIDITY_PROPERTY->count; i++)
		AUX_HUMIDITY_PROPERTY->items[i].sw.value = NULL;

	AUX_WIND_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_WIND_PROPERTY->count; i++)
		AUX_WIND_PROPERTY->items[i].sw.value = NULL;

	AUX_RAIN_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_RAIN_PROPERTY->count; i++)
		AUX_RAIN_PROPERTY->items[i].sw.value = NULL;

	AUX_CLOUD_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_CLOUD_PROPERTY->count; i++)
		AUX_CLOUD_PROPERTY->items[i].sw.value = NULL;

	AUX_SKY_PROPERTY->state = INDIGO_BUSY_STATE;
	for (i = 0; i < AUX_SKY_PROPERTY->count; i++)
		AUX_SKY_PROPERTY->items[i].sw.value = NULL;
}


static bool aag_heating_algorithm(indigo_device *device) {
	float temp_low                  = (float)X_RAIN_SENSOR_HEATER_TEMPETATURE_LOW_ITEM->number.value;
	float temp_high                 = (float)X_RAIN_SENSOR_HEATER_TEMPETATURE_HIGH_ITEM->number.value;
	float delta_low                 = (float)X_RAIN_SENSOR_HEATER_DELTA_LOW_ITEM->number.value;
	float delta_high                = (float)X_RAIN_SENSOR_HEATER_DELTA_HIGH_ITEM->number.value;
	float heater_impulse_temp       = (float)X_RAIN_SENSOR_HEATER_IMPULSE_TEMPERATURE_ITEM->number.value;
	float impulse_duration          = (float)X_RAIN_SENSOR_HEATER_IMPULSE_DURATION_ITEM->number.value;
	float impulse_cycle             = (float)X_RAIN_SENSOR_HEATER_IMPULSE_CYCLE_ITEM->number.value;
	float min_power                 = (float)X_RAIN_SENSOR_HEATER_MIN_POWER_ITEM->number.value;

	float ambient_temp              = (float)AUX_WEATHER_TEMPERATURE_ITEM->number.value;
	float rain_sensor_temp          = (float)X_SENSOR_RAIN_SENSOR_TEMPERATURE_ITEM->number.value;

	float refresh = REFRESH_INTERVAL;

	if (PRIVATE_DATA->sensor_heater_power == -1) {
		// If not already setted
		PRIVATE_DATA->sensor_heater_power = X_SENSOR_RAIN_HEATER_POWER_ITEM->number.value;
	}

	time_t current_time = time(NULL);

	if (AUX_RAIN_DRY_ITEM->sw.value != true && PRIVATE_DATA->heating_state == normal) {
		// We check if sensor is wet.
		if (PRIVATE_DATA->wet_start_time == -1) {
			// First moment wet
			PRIVATE_DATA->wet_start_time = time(NULL);
		} else {
			// We have been wet for a while
			if (current_time - PRIVATE_DATA->wet_start_time >= impulse_cycle) {
				// We have been a cycle wet. Apply pulse
				PRIVATE_DATA->wet_start_time = -1;
				PRIVATE_DATA->heating_state = increasing;
				PRIVATE_DATA->pulse_start_time = -1;
			}
		}
	} else {
		// is not wet
		PRIVATE_DATA->wet_start_time = -1;
	}

	if (PRIVATE_DATA->heating_state == pulse) {
		if (current_time - PRIVATE_DATA->pulse_start_time > impulse_duration) {
			// Pulse ends
			PRIVATE_DATA->heating_state  = normal;
			PRIVATE_DATA->wet_start_time   = -1;
			PRIVATE_DATA->pulse_start_time = -1;
		}
	}

	if (PRIVATE_DATA->heating_state == normal) {
		// Compute desired temperature
		if (ambient_temp < temp_low) {
			PRIVATE_DATA->desired_sensor_temperature = delta_low;
		} else if (ambient_temp > temp_high) {
			PRIVATE_DATA->desired_sensor_temperature = ambient_temp + delta_high;
		} else {
			// Between temp_low and temp_high
			float delta = ((((ambient_temp - temp_low) / (temp_high - temp_low)) * (delta_high - delta_low)) + delta_low);
			PRIVATE_DATA->desired_sensor_temperature = ambient_temp + delta;
			if (PRIVATE_DATA->desired_sensor_temperature < temp_low) {
				PRIVATE_DATA->desired_sensor_temperature = delta_low;
			}
		}
	} else {
		PRIVATE_DATA->desired_sensor_temperature = ambient_temp + heater_impulse_temp;
	}

	if (PRIVATE_DATA->heating_state == increasing) {
		if (rain_sensor_temp < PRIVATE_DATA->desired_sensor_temperature) {
			PRIVATE_DATA->sensor_heater_power = 100.0;
		} else {
			// the pulse starts
			PRIVATE_DATA->pulse_start_time = time(NULL);
			PRIVATE_DATA->heating_state = pulse;
		}
	}

	if ((PRIVATE_DATA->heating_state == normal) || (PRIVATE_DATA->heating_state == pulse)) {
		// Check desired temperature and act accordingly
		// Obtain the difference in temperature and modifier
		float diff = fabs(PRIVATE_DATA->desired_sensor_temperature - rain_sensor_temp);
		float refresh_modifier = sqrt(refresh / 10.0);
		float modifier = 1;

		if (diff > 8) {
			modifier = (1.4 / refresh_modifier);
		} else if (diff > 4) {
			modifier = (1.2 / refresh_modifier);
		} else if (diff > 3) {
			modifier = (1.1 / refresh_modifier);
		} else if (diff > 2) {
			modifier = (1.06 / refresh_modifier);
		} else if (diff > 1) {
			modifier = (1.04 / refresh_modifier);
		} else if (diff > 0.5) {
			modifier = (1.02 / refresh_modifier);
		} else if (diff > 0.3) {
			modifier = (1.01 / refresh_modifier);
		}

		if (rain_sensor_temp > PRIVATE_DATA->desired_sensor_temperature) {
			// Lower heating
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Temp: %f, Desired: %f, Lowering: %f %f -> %f\n", rain_sensor_temp, PRIVATE_DATA->desired_sensor_temperature, modifier, PRIVATE_DATA->sensor_heater_power, PRIVATE_DATA->sensor_heater_power / modifier);
			PRIVATE_DATA->sensor_heater_power /= modifier;
		} else {
			// increase heating
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Temp: %f, Desired: %f, Increasing: %f %f -> %f\n", rain_sensor_temp, PRIVATE_DATA->desired_sensor_temperature, modifier, PRIVATE_DATA->sensor_heater_power, PRIVATE_DATA->sensor_heater_power * modifier);
			PRIVATE_DATA->sensor_heater_power *= modifier;
		}
	}

	if (PRIVATE_DATA->sensor_heater_power < min_power) {
		PRIVATE_DATA->sensor_heater_power = min_power;
	}

	if (PRIVATE_DATA->sensor_heater_power > 100.0) {
		PRIVATE_DATA->sensor_heater_power = 100.0;
	}

	int raw_sensor_heater = (int)(PRIVATE_DATA->sensor_heater_power * 1023.0 / 100.0);

	set_pwm_duty_cycle(device, raw_sensor_heater);

	if (PRIVATE_DATA->heating_state == normal) {
		indigo_set_switch(X_HEATER_CONTROL_STATE_PROPERTY, X_HEATER_CONTROL_NORMAL_ITEM, true);
	} else if (PRIVATE_DATA->heating_state == increasing) {
		indigo_set_switch(X_HEATER_CONTROL_STATE_PROPERTY, X_HEATER_CONTROL_INCREASE_ITEM, true);
	} else if (PRIVATE_DATA->heating_state == pulse) {
		indigo_set_switch(X_HEATER_CONTROL_STATE_PROPERTY, X_HEATER_CONTROL_PULSE_ITEM, true);
	}
	X_HEATER_CONTROL_STATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_HEATER_CONTROL_STATE_PROPERTY, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "HEATER State: %d\n", PRIVATE_DATA->heating_state);

	return true;
}


static void sensors_timer_callback(indigo_device *device) {
	cloudwatcher_data cwd;

	bool success = aag_get_cloudwatcher_data(device, &cwd);
	if (success) {
		process_data_and_update(device, cwd);
		aag_heating_algorithm(device);
	}

	bool sw_state = false;
	success = aag_get_swith(device, &sw_state);
	if (success && AUX_GPIO_OUTLET_1_ITEM->sw.value != sw_state) {
		AUX_GPIO_OUTLET_1_ITEM->sw.value = sw_state;
		AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	}

	float reschedule_time = REFRESH_INTERVAL - cwd.read_duration;
	if (reschedule_time < 1) reschedule_time = 1;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REFRESH_INTERVAL = %.2f, reschedule_time = %.2f,  cwd.read_duration = %.2f\n", REFRESH_INTERVAL, reschedule_time, cwd.read_duration);
	indigo_reschedule_timer(device, reschedule_time, &PRIVATE_DATA->sensors_timer);
}


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (DEVICE_CONNECTED) {
		if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(X_HEATER_CONTROL_STATE_PROPERTY, property))
			indigo_define_property(device, X_HEATER_CONTROL_STATE_PROPERTY, NULL);
		if (indigo_property_match(X_CONSTANTS_PROPERTY, property))
			indigo_define_property(device, X_CONSTANTS_PROPERTY, NULL);
		if (indigo_property_match(X_SENSOR_READINGS_PROPERTY, property))
			indigo_define_property(device, X_SENSOR_READINGS_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		if (indigo_property_match(AUX_RAIN_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_RAIN_WARNING_PROPERTY, NULL);
		if (indigo_property_match(AUX_WIND_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_WIND_WARNING_PROPERTY, NULL);
		if (indigo_property_match(AUX_HUMIDITY_PROPERTY, property))
			indigo_define_property(device, AUX_HUMIDITY_PROPERTY, NULL);
		if (indigo_property_match(AUX_WIND_PROPERTY, property))
			indigo_define_property(device, AUX_WIND_PROPERTY, NULL);
		if (indigo_property_match(AUX_RAIN_PROPERTY, property))
			indigo_define_property(device, AUX_RAIN_PROPERTY, NULL);
		if (indigo_property_match(AUX_CLOUD_PROPERTY, property))
			indigo_define_property(device, AUX_CLOUD_PROPERTY, NULL);
		if (indigo_property_match(AUX_SKY_PROPERTY, property))
			indigo_define_property(device, AUX_SKY_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(X_SKY_CORRECTION_PROPERTY, property))
		indigo_define_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
	if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property))
		indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	if (indigo_property_match(AUX_RAIN_THRESHOLD_PROPERTY, property))
		indigo_define_property(device, AUX_RAIN_THRESHOLD_PROPERTY, NULL);
	if (indigo_property_match(AUX_WIND_THRESHOLD_PROPERTY, property))
		indigo_define_property(device, AUX_WIND_THRESHOLD_PROPERTY, NULL);
	if (indigo_property_match(AUX_HUMIDITY_THRESHOLDS_PROPERTY, property))
		indigo_define_property(device, AUX_HUMIDITY_THRESHOLDS_PROPERTY, NULL);
	if (indigo_property_match(AUX_WIND_THRESHOLDS_PROPERTY, property))
		indigo_define_property(device, AUX_WIND_THRESHOLDS_PROPERTY, NULL);
	if (indigo_property_match(AUX_RAIN_THRESHOLDS_PROPERTY, property))
		indigo_define_property(device, AUX_RAIN_THRESHOLDS_PROPERTY, NULL);
	if (indigo_property_match(AUX_CLOUD_THRESHOLDS_PROPERTY, property))
		indigo_define_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
	if (indigo_property_match(AUX_SKY_THRESHOLDS_PROPERTY, property))
		indigo_define_property(device, AUX_SKY_THRESHOLDS_PROPERTY, NULL);
	if (indigo_property_match(X_ANEMOMETER_TYPE_PROPERTY, property))
		indigo_define_property(device, X_ANEMOMETER_TYPE_PROPERTY, NULL);
	if (indigo_property_match(X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, property))
		indigo_define_property(device, X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, NULL);

	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		if (aag_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (aag_open(device)) {
				char board[MAX_LEN] = "N/A";
				char firmware[MAX_LEN] = "N/A";
				char serial_number[MAX_LEN] = "N/A";
				/* Pocket CW needs ~2sec after connect, maybe arduino based which resets at connect?!? */
				indigo_usleep(ONE_SECOND_DELAY*2);
				if (aag_is_cloudwatcher(device, board)) {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
					aag_get_firmware_version(device, firmware);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
					aag_get_serial_number(device, serial_number);
					indigo_copy_value(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, serial_number);
					aag_get_swith(device, &AUX_GPIO_OUTLET_1_ITEM->sw.value);
					aag_reset_properties(device);
					if (X_ANEMOMETER_TYPE_BLACK_ITEM->sw.value) {
						PRIVATE_DATA->anemometer_black = true;
					} else {
						PRIVATE_DATA->anemometer_black = false;
					}
					PRIVATE_DATA->heating_state = normal;
					PRIVATE_DATA->pulse_start_time = -1;
					PRIVATE_DATA->wet_start_time = -1;
					PRIVATE_DATA->desired_sensor_temperature = 0;
					PRIVATE_DATA->sensor_heater_power = -1;
					PRIVATE_DATA->cancel_reading = false;
					indigo_update_property(device, INFO_PROPERTY, NULL);
					indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
					indigo_define_property(device, X_HEATER_CONTROL_STATE_PROPERTY, NULL);
					indigo_define_property(device, X_CONSTANTS_PROPERTY, NULL);
					indigo_define_property(device, X_SENSOR_READINGS_PROPERTY, NULL);
					indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
					indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
					indigo_define_property(device, AUX_RAIN_WARNING_PROPERTY, NULL);
					indigo_define_property(device, AUX_WIND_WARNING_PROPERTY, NULL);
					indigo_define_property(device, AUX_HUMIDITY_PROPERTY, NULL);
					indigo_define_property(device, AUX_WIND_PROPERTY, NULL);
					indigo_define_property(device, AUX_RAIN_PROPERTY, NULL);
					indigo_define_property(device, AUX_CLOUD_PROPERTY, NULL);
					indigo_define_property(device, AUX_SKY_PROPERTY, NULL);
					aag_populate_constants(device);
					indigo_set_timer(device, 0, sensors_timer_callback, &PRIVATE_DATA->sensors_timer);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
					aag_close(device);
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
			}
		}
	} else {
		if (DEVICE_CONNECTED) {
			// Stop timer faster - do not wait to finish readout cycle
			PRIVATE_DATA->cancel_reading = true;
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->sensors_timer);
			// To be on the safe side - set mim heating power
			set_pwm_duty_cycle(device, (int)(PRIVATE_DATA->sensor_heater_power * 1023.0 / 100.0));
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, X_HEATER_CONTROL_STATE_PROPERTY, NULL);
			indigo_delete_property(device, X_CONSTANTS_PROPERTY, NULL);
			indigo_delete_property(device, X_SENSOR_READINGS_PROPERTY, NULL);
			indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_delete_property(device, AUX_RAIN_WARNING_PROPERTY, NULL);
			indigo_delete_property(device, AUX_WIND_WARNING_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HUMIDITY_PROPERTY, NULL);
			indigo_delete_property(device, AUX_WIND_PROPERTY, NULL);
			indigo_delete_property(device, AUX_RAIN_PROPERTY, NULL);
			indigo_delete_property(device, AUX_CLOUD_PROPERTY, NULL);
			indigo_delete_property(device, AUX_SKY_PROPERTY, NULL);
			aag_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
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
		indigo_set_timer(device, 0, handle_aux_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		}
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		bool success = false;
		if (AUX_GPIO_OUTLET_1_ITEM->sw.value) {
			success = aag_close_swith(device);
		} else {
			success = aag_open_swith(device);
		}
		if (success) {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		} else {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, "Open/Close switch failed");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_SKY_CORRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SKY_CORRECTION
		indigo_property_copy_values(X_SKY_CORRECTION_PROPERTY, property, false);
		X_SKY_CORRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_THRESHOLD
		indigo_property_copy_values(AUX_DEW_THRESHOLD_PROPERTY, property, false);
		AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_RAIN_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_RAIN_THRESHOLD
		indigo_property_copy_values(AUX_RAIN_THRESHOLD_PROPERTY, property, false);
		AUX_RAIN_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_RAIN_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_WIND_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_WIND_THRESHOLD
		indigo_property_copy_values(AUX_WIND_THRESHOLD_PROPERTY, property, false);
		AUX_WIND_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WIND_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CONSTANTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CONSTANTS
		indigo_property_copy_values(X_CONSTANTS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_HUMIDITY_THRESHOLDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HUMIDITY_THRESHOLDS
		indigo_property_copy_values(AUX_HUMIDITY_THRESHOLDS_PROPERTY, property, false);
		AUX_HUMIDITY_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HUMIDITY_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_WIND_THRESHOLDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_WIND_THRESHOLDS
		indigo_property_copy_values(AUX_WIND_THRESHOLDS_PROPERTY, property, false);
		AUX_WIND_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WIND_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_RAIN_THRESHOLDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_RAIN_THRESHOLDS
		indigo_property_copy_values(AUX_RAIN_THRESHOLDS_PROPERTY, property, false);
		AUX_RAIN_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_RAIN_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_CLOUD_THRESHOLDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_CLOUD_THRESHOLDS
		indigo_property_copy_values(AUX_CLOUD_THRESHOLDS_PROPERTY, property, false);
		AUX_CLOUD_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_SKY_THRESHOLDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_SKY_THRESHOLDS
		indigo_property_copy_values(AUX_SKY_THRESHOLDS_PROPERTY, property, false);
		AUX_SKY_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_SKY_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_ANEMOMETER_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_ANEMOMETER_TYPE
		indigo_property_copy_values(X_ANEMOMETER_TYPE_PROPERTY, property, false);
		X_ANEMOMETER_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (X_ANEMOMETER_TYPE_BLACK_ITEM->sw.value) {
			PRIVATE_DATA->anemometer_black = true;
		} else {
			PRIVATE_DATA->anemometer_black = false;
		}
		indigo_update_property(device, X_ANEMOMETER_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIN_SENSOR_HEATER_SETUP
		indigo_property_copy_values(X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, property, false);
		X_RAIN_SENSOR_HEATER_SETUP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, X_SKY_CORRECTION_PROPERTY);
			indigo_save_property(device, NULL, AUX_DEW_THRESHOLD_PROPERTY);
			indigo_save_property(device, NULL, AUX_RAIN_THRESHOLD_PROPERTY);
			indigo_save_property(device, NULL, AUX_WIND_THRESHOLD_PROPERTY);
			indigo_save_property(device, NULL, AUX_HUMIDITY_THRESHOLDS_PROPERTY);
			indigo_save_property(device, NULL, AUX_WIND_THRESHOLDS_PROPERTY);
			indigo_save_property(device, NULL, AUX_RAIN_THRESHOLDS_PROPERTY);
			indigo_save_property(device, NULL, AUX_CLOUD_THRESHOLDS_PROPERTY);
			indigo_save_property(device, NULL, AUX_SKY_THRESHOLDS_PROPERTY);
			indigo_save_property(device, NULL, X_ANEMOMETER_TYPE_PROPERTY);
			indigo_save_property(device, NULL, X_RAIN_SENSOR_HEATER_SETUP_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (DEVICE_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_aux_connect_property(device);
	}
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(X_HEATER_CONTROL_STATE_PROPERTY);
	indigo_release_property(X_CONSTANTS_PROPERTY);
	indigo_release_property(X_SENSOR_READINGS_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(AUX_RAIN_WARNING_PROPERTY);
	indigo_release_property(AUX_WIND_WARNING_PROPERTY);
	indigo_release_property(AUX_HUMIDITY_PROPERTY);
	indigo_release_property(AUX_WIND_PROPERTY);
	indigo_release_property(AUX_RAIN_PROPERTY);
	indigo_release_property(AUX_CLOUD_PROPERTY);
	indigo_release_property(AUX_SKY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
	indigo_release_property(X_SKY_CORRECTION_PROPERTY);

	indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	indigo_release_property(AUX_DEW_THRESHOLD_PROPERTY);

	indigo_delete_property(device, AUX_RAIN_THRESHOLD_PROPERTY, NULL);
	indigo_release_property(AUX_RAIN_THRESHOLD_PROPERTY);

	indigo_delete_property(device, AUX_WIND_THRESHOLD_PROPERTY, NULL);
	indigo_release_property(AUX_WIND_THRESHOLD_PROPERTY);

	indigo_delete_property(device, AUX_HUMIDITY_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_HUMIDITY_THRESHOLDS_PROPERTY);

	indigo_delete_property(device, AUX_WIND_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_WIND_THRESHOLDS_PROPERTY);

	indigo_delete_property(device, AUX_RAIN_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_RAIN_THRESHOLDS_PROPERTY);

	indigo_delete_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_CLOUD_THRESHOLDS_PROPERTY);

	indigo_delete_property(device, AUX_SKY_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_SKY_THRESHOLDS_PROPERTY);

	indigo_delete_property(device, X_ANEMOMETER_TYPE_PROPERTY, NULL);
	indigo_release_property(X_ANEMOMETER_TYPE_PROPERTY);

	indigo_delete_property(device, X_RAIN_SENSOR_HEATER_SETUP_PROPERTY, NULL);
	indigo_release_property(X_RAIN_SENSOR_HEATER_SETUP_PROPERTY);

	return indigo_aux_detach(device);
}

// --------------------------------------------------------------------------------

indigo_result indigo_aux_cloudwatcher(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static indigo_device *aag_cw = NULL;
	static aag_private_data *private_data = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_CLOUDWATCHER_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = indigo_safe_malloc(sizeof(aag_private_data));
		aag_cw = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
		aag_cw->private_data = private_data;
		indigo_attach_device(aag_cw);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(aag_cw);
		last_action = action;
		if (aag_cw != NULL) {
			indigo_detach_device(aag_cw);
			free(aag_cw);
			aag_cw = NULL;
		}
		if (private_data != NULL) {
			free(private_data);
			private_data = NULL;
		}

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
