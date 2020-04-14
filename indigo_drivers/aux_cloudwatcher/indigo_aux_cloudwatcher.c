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

#define DRIVER_VERSION         0x0001
#define AUX_CLOUDWATCHER_NAME  "AAG CloudWatcher"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_aux_driver.h>

#define DEFAULT_BAUDRATE "9600"

#define MAX_DEVICES 1

#define PRIVATE_DATA                   ((aag_private_data *)device->private_data)

#define AUX_RELAYS_GROUP	"Relay control"

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

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY      (PRIVATE_DATA->gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_2_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_3_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 5)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 6)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 7)

#define AUX_SENSORS_GROUP	"Sensors"

#define AUX_SENSOR_NAMES_PROPERTY      (PRIVATE_DATA->sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 5)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 6)
#define AUX_SENSOR_NAME_8_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_SENSORS_PROPERTY     (PRIVATE_DATA->sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_5_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 4)
#define AUX_GPIO_SENSOR_6_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 5)
#define AUX_GPIO_SENSOR_7_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 6)
#define AUX_GPIO_SENSOR_8_ITEM        (AUX_GPIO_SENSORS_PROPERTY->items + 7)


typedef struct {
	int power_voltage;  ///< Internal Supply Voltage
	int ir_sky_temperature;     ///< IR Sky Temperature
	int ir_sensor_temperature;  ///< IR Sensor Temperature
	int ambient_temperature; ///< Ambient temperature. In newer models there is no ambient temperature sensor so -10000 is returned.
	float rh;                 ///< Relative humidity
	float rh_temperature;     ///< Temperature from RH sensor
	int rain_frequency;     ///< Rain frequency
	int rain_heater;        ///< PWM Duty Cycle
	int rain_sensor_temperature; ///< Rain sensor temperature (used as ambient temperature in models where there is no ambient temperature sensor)
	int ldr;               ///< Ambient light sensor
	int switch_status;      ///< The status of the internal switch
	int wind_speed;         ///< The wind speed measured by the anemometer
	float read_cycle;       ///< Time used in the readings
	// ??? Shall I keep those?
	int totalReadings;     ///< Total number of readings taken by the Cloud Watcher Controller
	int internalErrors;    ///< Total number of internal errors
	int firstByteErrors;   ///< First byte errors count
	int secondByteErrors;  ///< Second byte errors count
	int pecByteErrors;     ///< PEC byte errors count
	int commandByteErrors; ///< Command byte errors count
} cloudwatcher_data;

typedef struct {
	int handle;
	float firmware;
	bool udp;
	pthread_mutex_t port_mutex;
	indigo_timer *sensors_timer;

	indigo_property *sky_correction_property,
	                *constants_property,
	                *gpio_outlet_pulse_property,
	                *sensor_names_property,
	                *sensors_property;
} aag_private_data;

typedef struct {
	indigo_device *device;
	aag_private_data *private_data;
} aag_device_data;

static aag_device_data device_data[MAX_DEVICES] = {0};

static void create_port_device(int device_index);
static void delete_port_device(int device_index);


#define DEVICE_CONNECTED_MASK            0x80

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)

#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define LUNATICO_CMD_LEN 100
#define BLOCK_SIZE 15

/* Linatico AAG CloudWatcher device Commands ======================================================================== */
static bool aag_command(indigo_device *device, const char *command, char *response, int block_count, int sleep) {
	int max = block_count * BLOCK_SIZE;
	char c;
	char buff[LUNATICO_CMD_LEN];
	struct timeval tv;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0)
			break;
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		if (PRIVATE_DATA->udp) {
			result = read(PRIVATE_DATA->handle, buff, LUNATICO_CMD_LEN);
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
			tv.tv_usec = 100000;
			timeout = 0;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			if (PRIVATE_DATA->udp) {
				result = read(PRIVATE_DATA->handle, response, LUNATICO_CMD_LEN);
				if (result < 1) {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
					return false;
				}
				index = result;
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
	const char *internal_name_block = "!N CloudWatcher";
	for (int i = 0; i < 15; i++) {
		if (buffer[i] != internal_name_block[i]) {
			return false;
		}
	}
	return true;
}

static bool aag_reset_buffers(indigo_device *device) {
	bool r = aag_command(device, "z!", NULL, 0, 0);
	if (!r) return false;
	return true;
}

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
	if (res != 1) return false;

	return true;
}


static bool aag_get_values(indigo_device *device, int *power_voltage, int *ambient_temperature, int *ldr_value, int *rain_sensor_temperature) {
	int zenerV;
	int ambTemp = -10000;
	int ldrRes;
	int rainSensTemp;

	if (PRIVATE_DATA->firmware >= 3.0) {
		char buffer[BLOCK_SIZE * 4];
		bool r = aag_command(device, "C!", buffer, 4, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!4 %d!5 %d", &zenerV, &ldrRes, &rainSensTemp);
		if (res != 3) return false;
	} else {
		char buffer[BLOCK_SIZE * 5];
		bool r = aag_command(device, "C!", buffer, 5, 0);
		if (!r) return false;

		int res = sscanf(buffer, "!6 %d!3 %d!4 %d!5 %d", &zenerV, &ambTemp, &ldrRes, &rainSensTemp);

		if (res != 4) return false;
	}

	*power_voltage           = zenerV;
	*ambient_temperature     = ambTemp;
	*ldr_value               = ldrRes;
	*rain_sensor_temperature = rainSensTemp;

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

	if (*temperature < -40 || *temperature > 80) return false;

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

static bool aag_get_wind_speed(indigo_device *device, int *wind_speed) {
	if (!wind_speed) return false;

	if (PRIVATE_DATA->firmware < 5.0) {
		*wind_speed = 0;
		return true;
	}

	char buffer[BLOCK_SIZE * 2];
	bool r = aag_command(device, "V!", buffer, 2, 0);
	if (!r) return false;

	int res = sscanf(buffer, "!w %d", wind_speed);
	if (res != 1) return false;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "raw_wind_speed = %d", *wind_speed);

	if (true && *wind_speed != 0) *wind_speed = (int)((float)(*wind_speed) * 0.84 + 3);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "wind_speed = %d", *wind_speed);
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

#define NUMBER_OF_READS 5

static bool aag_get_cloudwatcher_data(indigo_device *device, cloudwatcher_data *cwd) {
	int skyTemperature[NUMBER_OF_READS];
	int sensorTemperature[NUMBER_OF_READS];
	int rainFrequency[NUMBER_OF_READS];
	int internalSupplyVoltage[NUMBER_OF_READS];
	int ambientTemperature[NUMBER_OF_READS];
	int ldrValue[NUMBER_OF_READS];
	int rainSensorTemperature[NUMBER_OF_READS];
	int windSpeed[NUMBER_OF_READS];

	int check = 0;

	static int totalReadings = 0;
	totalReadings++;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < NUMBER_OF_READS; i++) {
		check = aag_get_ir_sky_temperature(device, &skyTemperature[i]);
		if (!check) return false;

		check = aag_get_sensor_temperature(device, &sensorTemperature[i]);
		if (!check) return false;

		check = aag_get_rain_frequency(device, &rainFrequency[i]);
		if (!check) return false;

		check = aag_get_values(device, &internalSupplyVoltage[i], &ambientTemperature[i], &ldrValue[i], &rainSensorTemperature[i]);
		if (!check) return false;

		check = aag_get_wind_speed(device, &windSpeed[i]);
		if (!check) return false;
	}

	if (!aag_get_rh_temperature(device, &cwd->rh, &cwd->rh_temperature)) {
		cwd->rh_temperature = -1000;
		cwd->rh = -1000;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	float rc = (float)(end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;

	cwd->read_cycle = rc;

	cwd->ir_sky_temperature      = aggregate_integers(skyTemperature, NUMBER_OF_READS);
	cwd->ir_sensor_temperature   = aggregate_integers(sensorTemperature, NUMBER_OF_READS);
	cwd->rain_frequency          = aggregate_integers(rainFrequency, NUMBER_OF_READS);
	cwd->power_voltage           = aggregate_integers(internalSupplyVoltage, NUMBER_OF_READS);
	cwd->ambient_temperature     = aggregate_integers(ambientTemperature, NUMBER_OF_READS);
	cwd->ldr                     = aggregate_integers(ldrValue, NUMBER_OF_READS);
	cwd->rain_sensor_temperature = aggregate_integers(rainSensorTemperature, NUMBER_OF_READS);
	cwd->wind_speed              = aggregate_integers(windSpeed, NUMBER_OF_READS);
	cwd->totalReadings   = totalReadings;

	/*
	check = getIRErrors(&cwd->firstByteErrors, &cwd->commandByteErrors, &cwd->secondByteErrors, &cwd->pecByteErrors);
	if (!check) return false;

	cwd->internalErrors = cwd->firstByteErrors + cwd->commandByteErrors + cwd->secondByteErrors + cwd->pecByteErrors;
	*/
	check = aag_get_pwm_duty_cycle(device, &cwd->rain_heater);
	if (!check) return false;

	/*
	check = getSwitchStatus(&cwd->switchStatus);
	if (!check) return false;
	*/
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

/*
bool process_data_and update(indigo_device *device) {
    CloudWatcherData data;

    int r = cwc->getAllData(&data);

    if (!r)
    {
        return false;
    }

    const int N_DATA = 11;
    double values[N_DATA];
    char *names[N_DATA];

    names[0]  = const_cast<char *>("supply");
    values[0] = data.supply;

    names[1]  = const_cast<char *>("sky");
    values[1] = data.sky;

    names[2]  = const_cast<char *>("sensor");
    values[2] = data.sensor;

    names[3]  = const_cast<char *>("ambient");
    values[3] = data.ambient;

    names[4]  = const_cast<char *>("rain");
    values[4] = data.rain;

    names[5]  = const_cast<char *>("rainHeater");
    values[5] = data.rainHeater;

    names[6]  = const_cast<char *>("rainTemp");
    values[6] = data.rainTemperature;

    names[7]  = const_cast<char *>("LDR");
    values[7] = data.ldr;

    names[8]       = const_cast<char *>("readCycle");
    values[8]      = data.readCycle;
    lastReadPeriod = data.readCycle;

    names[9]  = const_cast<char *>("windSpeed");
    values[9] = data.windSpeed;

    names[10]  = const_cast<char *>("totalReadings");
    values[10] = data.totalReadings;

    INumberVectorProperty *nvp = getNumber("readings");
    IUUpdateNumber(nvp, values, names, N_DATA);
    nvp->s = IPS_OK;
    IDSetNumber(nvp, nullptr);

    const int N_ERRORS = 5;
    double valuesE[N_ERRORS];
    char *namesE[N_ERRORS];

    namesE[0]  = const_cast<char *>("internalErrors");
    valuesE[0] = data.internalErrors;

    namesE[1]  = const_cast<char *>("firstAddressByteErrors");
    valuesE[1] = data.firstByteErrors;

    namesE[2]  = const_cast<char *>("commandByteErrors");
    valuesE[2] = data.commandByteErrors;

    namesE[3]  = const_cast<char *>("secondAddressByteErrors");
    valuesE[3] = data.secondByteErrors;

    namesE[4]  = const_cast<char *>("pecByteErrors");
    valuesE[4] = data.pecByteErrors;

    INumberVectorProperty *nvpE = getNumber("unitErrors");
    IUUpdateNumber(nvpE, valuesE, namesE, N_ERRORS);
    nvpE->s = IPS_OK;
    IDSetNumber(nvpE, nullptr);

    const int N_SENS = 9;
    double valuesS[N_SENS];
    char *namesS[N_SENS];

    float skyTemperature = float(data.sky) / 100.0;
    namesS[0]            = const_cast<char *>("infraredSky");
    valuesS[0]           = skyTemperature;

    namesS[1]  = const_cast<char *>("infraredSensor");
    valuesS[1] = float(data.sensor) / 100.0;

    namesS[2]  = const_cast<char *>("rainSensor");
    valuesS[2] = data.rain;

    float rainSensorTemperature = data.rainTemperature;
    if (rainSensorTemperature > 1022)
    {
        rainSensorTemperature = 1022;
    }
    if (rainSensorTemperature < 1)
    {
        rainSensorTemperature = 1;
    }
    rainSensorTemperature = constants.rainPullUpResistance / ((1023.0 / rainSensorTemperature) - 1.0);
    rainSensorTemperature = log(rainSensorTemperature / constants.rainResistanceAt25);
    rainSensorTemperature =
        1.0 / (rainSensorTemperature / constants.rainBetaFactor + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;

    namesS[3]  = const_cast<char *>("rainSensorTemperature");
    valuesS[3] = rainSensorTemperature;

    float rainSensorHeater = data.rainHeater;
    rainSensorHeater       = 100.0 * rainSensorHeater / 1023.0;
    namesS[4]              = const_cast<char *>("rainSensorHeater");
    valuesS[4]             = rainSensorHeater;

    float ambientLight = float(data.ldr);
    if (ambientLight > 1022.0)
    {
        ambientLight = 1022.0;
    }
    if (ambientLight < 1)
    {
        ambientLight = 1.0;
    }
    ambientLight = constants.ldrPullUpResistance / ((1023.0 / ambientLight) - 1.0);
    namesS[5]    = const_cast<char *>("brightnessSensor");
    valuesS[5]   = ambientLight;

    setParameterValue("WEATHER_BRIGHTNESS", ambientLight);

    float ambientTemperature = data.ambient;

    if (ambientTemperature == -10000)
    {
        ambientTemperature = float(data.sensor) / 100.0;
    }
    else
    {
        if (ambientTemperature > 1022)
        {
            ambientTemperature = 1022;
        }
        if (ambientTemperature < 1)
        {
            ambientTemperature = 1;
        }
        ambientTemperature = constants.ambientPullUpResistance / ((1023.0 / ambientTemperature) - 1.0);
        ambientTemperature = log(ambientTemperature / constants.ambientResistanceAt25);
        ambientTemperature =
            1.0 / (ambientTemperature / constants.ambientBetaFactor + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;
    }

    namesS[6]  = const_cast<char *>("ambientTemperatureSensor");
    valuesS[6] = ambientTemperature;

    INumberVectorProperty *nvpSky = getNumber("skyCorrection");
    float k1                      = getNumberValueFromVector(nvpSky, "k1");
    float k2                      = getNumberValueFromVector(nvpSky, "k2");
    float k3                      = getNumberValueFromVector(nvpSky, "k3");
    float k4                      = getNumberValueFromVector(nvpSky, "k4");
    float k5                      = getNumberValueFromVector(nvpSky, "k5");

    float correctedTemperature =
        skyTemperature - ((k1 / 100.0) * (ambientTemperature - k2 / 10.0) +
                          (k3 / 100.0) * pow(exp(k4 / 1000 * ambientTemperature), (k5 / 100.0)));

    namesS[7]  = const_cast<char *>("correctedInfraredSky");
    valuesS[7] = correctedTemperature;

    namesS[8]  = const_cast<char *>("windSpeed");
    valuesS[8] = data.windSpeed;

    INumberVectorProperty *nvpS = getNumber("sensors");
    IUUpdateNumber(nvpS, valuesS, namesS, N_SENS);
    nvpS->s = IPS_OK;
    IDSetNumber(nvpS, nullptr);

    ISState states[2];
    char *namesSw[2];
    namesSw[0] = const_cast<char *>("open");
    namesSw[1] = const_cast<char *>("close");
    //IDLog("%d\n", data.switchStatus);
    if (data.switchStatus == 1)
    {
        states[0] = ISS_OFF;
        states[1] = ISS_ON;
    }
    else
    {
        states[0] = ISS_ON;
        states[1] = ISS_OFF;
    }

    ISwitchVectorProperty *svpSw = getSwitch("deviceSwitch");
    IUUpdateSwitch(svpSw, states, namesSw, 2);
    svpSw->s = IPS_OK;
    IDSetSwitch(svpSw, nullptr);

    //IDLog("%d\n", data.switchStatus);

    setParameterValue("WEATHER_CLOUD", correctedTemperature);
    setParameterValue("WEATHER_RAIN", data.rain);

    INumberVectorProperty *consts = getNumber("constants");
    int anemometerStatus          = getNumberValueFromVector(consts, "anemometerStatus");

    //IDLog("%d\n", data.switchStatus);

    if (anemometerStatus)
    {
        setParameterValue("WEATHER_WIND_SPEED", data.windSpeed);
    }
    else
    {
        setParameterValue("WEATHER_WIND_SPEED", 0);
    }
    return true;
}
*/

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
	// -------------------------------------------------------------------------------- AUTHENTICATION
	//AUTHENTICATION_PROPERTY->hidden = false;
	//AUTHENTICATION_PROPERTY->count = 1;
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_PORTS
	DEVICE_PORTS_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
	DEVICE_BAUDRATE_PROPERTY->hidden = true;
	strncpy(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE, INDIGO_VALUE_SIZE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 7;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	X_SKY_CORRECTION_PROPERTY = indigo_init_number_property(NULL, device->name, X_SKY_CORRECTION_PROPERTY_NAME, AUX_RELAYS_GROUP, "Sky temperature correction", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
	if (X_SKY_CORRECTION_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_SKY_CORRECTION_K1_ITEM, X_SKY_CORRECTION_K1_ITEM_NAME, X_SKY_CORRECTION_K1_ITEM_NAME, -999, 999, 0, 33);
	indigo_init_number_item(X_SKY_CORRECTION_K2_ITEM, X_SKY_CORRECTION_K2_ITEM_NAME, X_SKY_CORRECTION_K2_ITEM_NAME, -999, 999, 0, 0);
	indigo_init_number_item(X_SKY_CORRECTION_K3_ITEM, X_SKY_CORRECTION_K3_ITEM_NAME, X_SKY_CORRECTION_K3_ITEM_NAME, -999, 999, 0, 4);
	indigo_init_number_item(X_SKY_CORRECTION_K4_ITEM, X_SKY_CORRECTION_K4_ITEM_NAME, X_SKY_CORRECTION_K4_ITEM_NAME, -999, 999, 0, 100);
	indigo_init_number_item(X_SKY_CORRECTION_K5_ITEM, X_SKY_CORRECTION_K5_ITEM_NAME, X_SKY_CORRECTION_K5_ITEM_NAME, -999, 999, 0, 100);

	// -------------------------------------------------------------------------------- X_CONSTANTS
	X_CONSTANTS_PROPERTY = indigo_init_number_property(NULL, device->name, X_CONSTANTS_PROPERTY_NAME, AUX_RELAYS_GROUP, "AAG Constants", INDIGO_OK_STATE, INDIGO_RO_PERM, 10);
	if (X_CONSTANTS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_CONSTANTS_ZENER_VOLTAGE_ITEM, X_CONSTANTS_ZENER_VOLTAGE_ITEM_NAME, "Zener voltage (V)", -100, 100, 0, 3);
	indigo_init_number_item(X_CONSTANTS_LDR_MAX_R_ITEM, X_CONSTANTS_LDR_MAX_R_ITEM_NAME, "LDR max R (KOhm)", 0, 100000, 0, 1744);
	indigo_init_number_item(X_CONSTANTS_LDR_PULLUP_R_ITEM, X_CONSTANTS_LDR_PULLUP_R_ITEM_NAME, "LDR Pullup R (KOhm)", 0, 100000, 0, 56);
	indigo_init_number_item(X_CONSTANTS_RAIN_BETA_ITEM, X_CONSTANTS_RAIN_BETA_ITEM_NAME, "Rain Beta factor", -100000, 100000, 0, 3450);
	indigo_init_number_item(X_CONSTANTS_RAIN_R_AT_25_ITEM, X_CONSTANTS_RAIN_R_AT_25_ITEM_NAME, "Rain R at 25degC (KOhm)", 0, 100000, 0, 1);
	indigo_init_number_item(X_CONSTANTS_RAIN_PULLUP_R_ITEM, X_CONSTANTS_RAIN_PULLUP_R_ITEM_NAME, "Rain Pullup R (KOhm)", 0, 100000, 0, 1);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_BETA_ITEM, X_CONSTANTS_AMBIENT_BETA_ITEM_NAME, "Ambient Beta factor", -100000, 100000, 0, 3811);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_R_AT_25_ITEM, X_CONSTANTS_AMBIENT_R_AT_25_ITEM_NAME, "Ambient R at 25degC (KOhm)", 0, 100000, 0, 10);
	indigo_init_number_item(X_CONSTANTS_AMBIENT_PULLUP_R_ITEM, X_CONSTANTS_AMBIENT_PULLUP_R_ITEM_NAME, "Ambient Pullup R (KOhm)", 0, 100000, 0, 9.9);
	indigo_init_number_item(X_CONSTANTS_ANEMOMETER_STATE_ITEM, X_CONSTANTS_ANEMOMETER_STATE_ITEM_NAME, "Anemometer Status", 0, 10, 0, 0);
	// -------------------------------------------------------------------------------- GPIO PULSE OUTLETS
	AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, "AUX_OUTLET_PULSE_LENGTHS", AUX_RELAYS_GROUP, "Relay pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Relay #1", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Relay #2", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Relay #3", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Relay #4", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Relay #5", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Relay #6", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Relay #7", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Relay #8", 0, 100000, 100, 0);
	// -------------------------------------------------------------------------------- SENSOR_NAMES
	AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
	if (AUX_SENSOR_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_SENSOR_NAME_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor 1", "Sensor #1");
	indigo_init_text_item(AUX_SENSOR_NAME_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor 2", "Sensor #2");
	indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor 3", "Sensor #3");
	indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor 4", "Sensor #4");
	indigo_init_text_item(AUX_SENSOR_NAME_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor 5", "Sensor #5");
	indigo_init_text_item(AUX_SENSOR_NAME_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor 6", "Sensor #6");
	indigo_init_text_item(AUX_SENSOR_NAME_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor 7", "Sensor #7");
	indigo_init_text_item(AUX_SENSOR_NAME_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Sensor 8", "Sensor #8");
	// -------------------------------------------------------------------------------- GPIO_SENSORS
	AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
	if (AUX_GPIO_SENSORS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_SENSOR_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Sensor #5", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Sensor #6", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Sensor #7", 0, 1024, 1, 0);
	indigo_init_number_item(AUX_GPIO_SENSOR_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Sensor #8", 0, 1024, 1, 0);
	//---------------------------------------------------------------------------
	indigo_define_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
	indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	return INDIGO_OK;
}

static void sensors_timer_callback(indigo_device *device) {

	//int power_voltage, ambient_temperature, ldr_value, rain_sensor_temperature;
	//bool success = aag_get_values(device, &power_voltage, &ambient_temperature, &ldr_value, &rain_sensor_temperature);
	cloudwatcher_data cwd;
	bool success = aag_get_cloudwatcher_data(device, &cwd);

	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->sensors_timer);
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (DEVICE_CONNECTED) {
		if (indigo_property_match(X_CONSTANTS_PROPERTY, property))
			indigo_define_property(device, X_CONSTANTS_PROPERTY, NULL);
		if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property))
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		if (indigo_property_match(AUX_GPIO_SENSORS_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	if (indigo_property_match(X_SKY_CORRECTION_PROPERTY, property))
		indigo_define_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
	if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);

	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		if (aag_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!DEVICE_CONNECTED) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				aag_reset_buffers(device);
				if (aag_open(device)) {
					char board[LUNATICO_CMD_LEN] = "N/A";
					char firmware[LUNATICO_CMD_LEN] = "N/A";
					char serial_number[LUNATICO_CMD_LEN] = "N/A";
					if (aag_is_cloudwatcher(device, board)) {
						strncpy(INFO_DEVICE_MODEL_ITEM->text.value, board, INDIGO_VALUE_SIZE);
						aag_get_firmware_version(device, firmware);
						strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware, INDIGO_VALUE_SIZE);
						aag_get_serial_number(device, serial_number);
						strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, serial_number, INDIGO_VALUE_SIZE);
						indigo_update_property(device, INFO_PROPERTY, NULL);
						indigo_define_property(device, X_CONSTANTS_PROPERTY, NULL);
						indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
						indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);

						aag_populate_constants(device);
						PRIVATE_DATA->sensors_timer = indigo_set_timer(device, 0, sensors_timer_callback);
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					} else {
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
						aag_close(device);
					}
				}
			}
		} else {
			if (DEVICE_CONNECTED) {
				indigo_cancel_timer(device, &PRIVATE_DATA->sensors_timer);

				indigo_delete_property(device, X_CONSTANTS_PROPERTY, NULL);
				indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
				indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);

				aag_close(device);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(X_SKY_CORRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SKY_CORRECTION
		indigo_property_copy_values(X_SKY_CORRECTION_PROPERTY, property, false);
		X_SKY_CORRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CONSTANTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(X_CONSTANTS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;

		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_SENSOR_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_SENSOR_NAMES
		indigo_property_copy_values(AUX_SENSOR_NAMES_PROPERTY, property, false);
		if (DEVICE_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_SENSOR_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_2_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_4_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_5_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_5_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_6_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_6_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_7_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_7_ITEM->text.value);
		snprintf(AUX_GPIO_SENSOR_8_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_SENSOR_NAME_8_ITEM->text.value);
		AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (DEVICE_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUTHENTICATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUTHENTICATION_PROPERTY
		indigo_property_copy_values(AUTHENTICATION_PROPERTY, property, false);
		if (!DEVICE_CONNECTED) return INDIGO_OK;
		if (AUTHENTICATION_PASSWORD_ITEM->text.value[0] == 0) {
		} else {
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_SKY_CORRECTION_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	aag_close(device);
	indigo_device_disconnect(NULL, device->name);
	indigo_release_property(X_CONSTANTS_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, X_SKY_CORRECTION_PROPERTY, NULL);
	indigo_release_property(X_SKY_CORRECTION_PROPERTY);

	indigo_delete_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	return indigo_aux_detach(device);
}

// --------------------------------------------------------------------------------

static int device_number = 0;

static void create_port_device(int device_index) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_CLOUDWATCHER_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (device_index >= MAX_DEVICES) return;
	if (device_data[device_index].device != NULL) return;

	if (device_data[device_index].private_data == NULL) {
		device_data[device_index].private_data = malloc(sizeof(aag_private_data));
		assert(device_data[device_index].private_data != NULL);
		memset(device_data[device_index].private_data, 0, sizeof(aag_private_data));
		pthread_mutex_init(&device_data[device_index].private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data[device_index].device = malloc(sizeof(indigo_device));
	assert(device_data[device_index].device != NULL);

	memcpy(device_data[device_index].device, &aux_template, sizeof(indigo_device));
	sprintf(device_data[device_index].device->name, "%s", AUX_CLOUDWATCHER_NAME);

	device_data[device_index].device->private_data = device_data[device_index].private_data;
	indigo_attach_device(device_data[device_index].device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device with port index = %d", device_index);
}


static void delete_port_device(int device_index) {
	if (device_index >= MAX_DEVICES) return;

	if (device_data[device_index].device != NULL) {
		indigo_detach_device(device_data[device_index].device);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Locical Device with index = %d", device_index);
		free(device_data[device_index].device);
		device_data[device_index].device = NULL;
	}

	if (device_data[device_index].private_data != NULL) {
		free(device_data[device_index].private_data);
		device_data[device_index].private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
}


indigo_result indigo_aux_cloudwatcher(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int index = 0; index < MAX_DEVICES; index++) {
			create_port_device(index);
		}
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		for (int index = 0; index < MAX_DEVICES; index++) {
			delete_port_device(index);
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
