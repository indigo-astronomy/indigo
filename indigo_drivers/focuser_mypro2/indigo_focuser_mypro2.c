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

/** myFocuserPro2 focuser driver
 \file indigo_focuser_mypro2.c
 */

#define DRIVER_VERSION 0x0005
#define DRIVER_NAME "indigo_focuser_mypro2"

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

#include "indigo_focuser_mypro2.h"

#define SERIAL_BAUDRATE            "9600"

#define PRIVATE_DATA                    ((mfp_private_data *)device->private_data)


#define X_STEP_MODE_PROPERTY          (PRIVATE_DATA->step_mode_property)
#define X_STEP_MODE_FULL_ITEM         (X_STEP_MODE_PROPERTY->items+0)
#define X_STEP_MODE_HALF_ITEM         (X_STEP_MODE_PROPERTY->items+1)
#define X_STEP_MODE_FOURTH_ITEM       (X_STEP_MODE_PROPERTY->items+2)
#define X_STEP_MODE_EIGTH_ITEM        (X_STEP_MODE_PROPERTY->items+3)
#define X_STEP_MODE_16TH_ITEM         (X_STEP_MODE_PROPERTY->items+4)
#define X_STEP_MODE_32TH_ITEM         (X_STEP_MODE_PROPERTY->items+5)
#define X_STEP_MODE_64TH_ITEM         (X_STEP_MODE_PROPERTY->items+6)
#define X_STEP_MODE_128TH_ITEM        (X_STEP_MODE_PROPERTY->items+7)

#define X_STEP_MODE_PROPERTY_NAME     "X_STEP_MODE"
#define X_STEP_MODE_FULL_ITEM_NAME    "FULL"
#define X_STEP_MODE_HALF_ITEM_NAME    "HALF"
#define X_STEP_MODE_FOURTH_ITEM_NAME  "FOURTH"
#define X_STEP_MODE_EIGTH_ITEM_NAME   "EIGTH"
#define X_STEP_MODE_16TH_ITEM_NAME    "16TH"
#define X_STEP_MODE_32TH_ITEM_NAME    "32TH"
#define X_STEP_MODE_64TH_ITEM_NAME    "64TH"
#define X_STEP_MODE_128TH_ITEM_NAME   "128TH"

#define X_COILS_MODE_PROPERTY              (PRIVATE_DATA->coils_mode_property)
#define X_COILS_MODE_IDLE_OFF_ITEM         (X_COILS_MODE_PROPERTY->items+0)
#define X_COILS_MODE_ALWAYS_ON_ITEM        (X_COILS_MODE_PROPERTY->items+1)

#define X_COILS_MODE_PROPERTY_NAME         "X_COILS_MODE"
#define X_COILS_MODE_IDLE_OFF_ITEM_NAME    "OFF_WHEN_IDLE"
#define X_COILS_MODE_ALWAYS_ON_ITEM_NAME   "ALWAYS_ON"

#define X_SETTLE_TIME_PROPERTY             (PRIVATE_DATA->timings_property)
#define X_SETTLE_TIME_ITEM                 (X_SETTLE_TIME_PROPERTY->items+0)

#define X_SETTLE_TIME_PROPERTY_NAME        "X_SETTLE_TIME"
#define X_SETTLE_TIME_ITEM_NAME            "SETTLE_TIME"


// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int handle;
	uint32_t current_position, target_position, max_position;
	bool positive_last_move;
	int backlash;
	double prev_temp;
	indigo_timer *focuser_timer, *temperature_timer;
	pthread_mutex_t port_mutex;
	indigo_property *step_mode_property, *coils_mode_property, *current_control_property, *timings_property, *model_hint_property;
} mfp_private_data;

static void compensate_focus(indigo_device *device, double new_temp);

/* My FP2 Commands ======================================================================== */

#define MFP_CMD_LEN 100

typedef enum {
	COILS_MODE_IDLE_OFF = 0,
	COILS_MODE_ALWAYS_ON = 1
} coilsmode_t;

typedef enum {
	STEP_MODE_FULL = 1,
	STEP_MODE_HALF = 2,
	STEP_MODE_FOURTH = 4,
	STEP_MODE_EIGTH = 8,
	STEP_MODE_16TH = 16,
	STEP_MODE_32TH = 32,
	STEP_MODE_64TH = 64,
	STEP_MODE_128TH = 128
} stepmode_t;

#define NO_TEMP_READING                (-127)

static bool mfp_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
	char c;
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
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
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
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}
			response[index++] = c;

			if (c == '#')
				break;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


static bool mfp_get_info(indigo_device *device, char *board, char *firmware) {
	if(!board || !firmware) return false;

	char response[MFP_CMD_LEN]={0};
	if (mfp_command(device, ":04#", response, sizeof(response), 100)) {
		char *delim = NULL;
		if ((delim = strchr(response, '\n'))) {
			*delim = ' ';
		}
		if (!delim) return false;
		if ((delim = strchr(response, '\r'))) {
			*delim = ' ';
		}
		if (!delim) return false;

		if ((delim = strrchr(response, '#'))) {
			*delim = '\0';
		}
		if (!delim) return false;

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "response = %s", response);

		int parsed = sscanf(response, "F%s %s", board, firmware);
		if (parsed != 2) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, ":04# -> %s = %s %s", response, board, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool mfp_command_get_int_value(indigo_device *device, const char *command, char expect, uint32_t *value) {
	if (!value) return false;

	char response[MFP_CMD_LEN]={0};
	if (mfp_command(device, command, response, sizeof(response), 100)) {
		char format[100];
		sprintf(format, "%c%%d#", expect);
		int parsed = sscanf(response, format, value);
		if (parsed != 1) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s = %d", command, response, *value);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool mfp_stop(indigo_device *device) {
	return mfp_command(device, ":27#", NULL, 0, 100);
}


static bool mfp_sync_position(indigo_device *device, uint32_t pos) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":31%06d#", pos);
	return mfp_command(device, command, NULL, 0, 100);
}


static bool mfp_set_reverse(indigo_device *device, bool enabled) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":14%d#", enabled ? 1 : 0);
	return mfp_command(device, command, NULL, 0, 100);
}


static bool mfp_get_reverse(indigo_device *device, bool *reversed) {
	uint32_t _reversed;
	if (!mfp_command_get_int_value(device, ":13#", 'R', &_reversed))
		return false;
	*reversed = _reversed;
	return true;
}

static bool mfp_get_position(indigo_device *device, uint32_t *pos) {
	return mfp_command_get_int_value(device, ":00#", 'P', pos);
}

static bool mfp_goto_position(indigo_device *device, uint32_t position) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":05%06d#", position);
	return mfp_command(device, command, NULL, 0, 100);
}

static bool mfp_goto_position_bl(indigo_device *device, uint32_t position) {
	uint32_t target_position = indigo_compensate_backlash(
		position,
		(int)PRIVATE_DATA->current_position,
		(int)FOCUSER_BACKLASH_ITEM->number.value,
		&PRIVATE_DATA->positive_last_move
	);
	return mfp_goto_position(device, target_position);
}

static bool mfp_get_step_mode(indigo_device *device, stepmode_t *mode) {
	uint32_t _mode;
	bool res = mfp_command_get_int_value(device, ":29#", 'S', &_mode);
	*mode = _mode;
	return res;
}


static bool mfp_set_step_mode(indigo_device *device, stepmode_t mode) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":30%d#", mode);
	return mfp_command(device, command, NULL, 0, 100);
}

static bool mfp_enable_backlash(indigo_device *device, bool enable) {
	bool res = false;
	if (enable) {
		res = mfp_command(device, ":731#", NULL, 0, 100) && mfp_command(device, ":751#", NULL, 0, 100);
	} else {
		res = mfp_command(device, ":730#", NULL, 0, 100) && mfp_command(device, ":750#", NULL, 0, 100);
	}
	return res;
}

static bool mfp_get_max_position(indigo_device *device, uint32_t *position) {
	return mfp_command_get_int_value(device, ":08#", 'M', position);
}


static bool mfp_set_max_position(indigo_device *device, uint32_t position) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":07%d#", position);
	return mfp_command(device, command, NULL, 0, 100);
}


static bool mfp_get_settle_buffer(indigo_device *device, uint32_t *delay) {
	return mfp_command_get_int_value(device, ":72#", '3', delay);
}


static bool mfp_set_settle_buffer(indigo_device *device, uint32_t delay) {
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":71%d#", delay);
	return mfp_command(device, command, NULL, 0, 100);
}


static bool mfp_get_coils_mode(indigo_device *device, coilsmode_t *mode) {
	uint32_t _mode;
	bool res = mfp_command_get_int_value(device, ":11#", 'O', &_mode);
	*mode = _mode;
	return res;
}


static bool mfp_set_coils_mode(indigo_device *device, coilsmode_t mode) {
	if (mode > 1) return false;
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":12%d#", mode);
	return mfp_command(device, command, NULL, 0, 100);
}

static bool mfp_set_speed(indigo_device *device, uint32_t speed) {
	if (speed > 2) return false;
	char command[MFP_CMD_LEN];
	snprintf(command, MFP_CMD_LEN, ":15%d#", speed);
	return mfp_command(device, command, NULL, 0, 100);
}


static bool mfp_is_moving(indigo_device *device, bool *is_moving) {
	return mfp_command_get_int_value(device, ":01#", 'I', (uint32_t *)is_moving);
}


static bool mfp_save_settings(indigo_device *device) {
	return mfp_command(device, ":48#", NULL, 0, 100);
}


static bool mfp_get_temperature(indigo_device *device, double *temperature) {
	char response[MFP_CMD_LEN]={0};
	if (mfp_command(device, ":06#", response, sizeof(response), 100)) {
		int parsed = sscanf(response, "Z%lf#", temperature);
		if (parsed != 1) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, ":06# -> %s = %lf", response, *temperature);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving;
	uint32_t position;

	if (!mfp_is_moving(device, &moving)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_is_moving(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if (!mfp_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->current_position = (double)position;
	}

	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void temperature_timer_callback(indigo_device *device) {
	double temp;
	static bool has_sensor = true;
	//bool moving = false;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	if (!mfp_get_temperature(device, &temp)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_temperature(%d, -> %f) failed", PRIVATE_DATA->handle, temp);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "mfp_get_temperature(%d, -> %f) succeeded", PRIVATE_DATA->handle, FOCUSER_TEMPERATURE_ITEM->number.value);
	}

	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) { /* -127 is returned when the sensor is not connected */
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
			has_sensor = false;
		}
	} else {
		has_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		compensate_focus(device, temp);
	} else {
		/* reset temp so that the compensation starts when auto mode is selected */
		PRIVATE_DATA->prev_temp = NO_TEMP_READING;
	}

	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;

	/* we do not have previous temperature reading */
	if (PRIVATE_DATA->prev_temp <= NO_TEMP_READING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}

	/* we do not have current temperature reading or focuser is moving */
	if ((new_temp <= NO_TEMP_READING) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	/* temperature difference if more than 1 degree so compensation needed */
	if ((fabs(temp_difference) >= 1.0) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.1f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %f", temp_difference);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	uint32_t current_position;
	if (!mfp_get_position(device, &current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
	}
	PRIVATE_DATA->current_position = (double)current_position;

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	if (!mfp_goto_position_bl(device, (uint32_t)PRIVATE_DATA->target_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_goto_position_bl(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}


static indigo_result mfp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_STEP_MODE_PROPERTY, property))
			indigo_define_property(device, X_STEP_MODE_PROPERTY, NULL);
		if (indigo_property_match(X_COILS_MODE_PROPERTY, property))
			indigo_define_property(device, X_COILS_MODE_PROPERTY, NULL);
		if (indigo_property_match(X_SETTLE_TIME_PROPERTY, property))
			indigo_define_property(device, X_SETTLE_TIME_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		PRIVATE_DATA->handle = -1;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, SERIAL_BAUDRATE);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 2000000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;

		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;

		FOCUSER_SPEED_PROPERTY->hidden = false;
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 2;
		FOCUSER_SPEED_ITEM->number.step = 0;

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 100;
		FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;

		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;

		// -------------------------------------------------------------------------- STEP_MODE_PROPERTY
		X_STEP_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_STEP_MODE_PROPERTY_NAME, "Advanced", "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 8);
		if (X_STEP_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_STEP_MODE_PROPERTY->hidden = false;
		indigo_init_switch_item(X_STEP_MODE_FULL_ITEM, X_STEP_MODE_FULL_ITEM_NAME, "Full step", false);
		indigo_init_switch_item(X_STEP_MODE_HALF_ITEM, X_STEP_MODE_HALF_ITEM_NAME, "1/2 step", false);
		indigo_init_switch_item(X_STEP_MODE_FOURTH_ITEM, X_STEP_MODE_FOURTH_ITEM_NAME, "1/4 step", false);
		indigo_init_switch_item(X_STEP_MODE_EIGTH_ITEM, X_STEP_MODE_EIGTH_ITEM_NAME, "1/8 step", false);
		indigo_init_switch_item(X_STEP_MODE_16TH_ITEM, X_STEP_MODE_16TH_ITEM_NAME, "1/16 step", false);
		indigo_init_switch_item(X_STEP_MODE_32TH_ITEM, X_STEP_MODE_32TH_ITEM_NAME, "1/32 step", false);
		indigo_init_switch_item(X_STEP_MODE_64TH_ITEM, X_STEP_MODE_64TH_ITEM_NAME, "1/64 step", false);
		indigo_init_switch_item(X_STEP_MODE_128TH_ITEM, X_STEP_MODE_128TH_ITEM_NAME, "1/128 step", false);

		// -------------------------------------------------------------------------- COILS_MODE_PROPERTY
		X_COILS_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_COILS_MODE_PROPERTY_NAME, "Advanced", "Coils Power", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_COILS_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_COILS_MODE_PROPERTY->hidden = false;
		indigo_init_switch_item(X_COILS_MODE_IDLE_OFF_ITEM, X_COILS_MODE_IDLE_OFF_ITEM_NAME, "OFF when idle", false);
		indigo_init_switch_item(X_COILS_MODE_ALWAYS_ON_ITEM, X_COILS_MODE_ALWAYS_ON_ITEM_NAME, "Always ON", false);
		//--------------------------------------------------------------------------- TIMINGS_PROPERTY
		X_SETTLE_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, X_SETTLE_TIME_PROPERTY_NAME, "Advanced", "Settle time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_SETTLE_TIME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_SETTLE_TIME_ITEM, X_SETTLE_TIME_ITEM_NAME, "Settle time (ms)", 0, 999, 10, 0);
		// --------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void update_step_mode_switches(indigo_device * device) {
	stepmode_t value;

	if (!mfp_get_step_mode(device, &value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_step_mode(%d) failed", PRIVATE_DATA->handle);
		return;
	}

	switch (value) {
	case STEP_MODE_FULL:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_FULL_ITEM, true);
		break;
	case STEP_MODE_HALF:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_HALF_ITEM, true);
		break;
	case STEP_MODE_FOURTH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_FOURTH_ITEM, true);
		break;
	case STEP_MODE_EIGTH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_EIGTH_ITEM, true);
		break;
	case STEP_MODE_16TH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_16TH_ITEM, true);
		break;
	case STEP_MODE_32TH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_32TH_ITEM, true);
		break;
	case STEP_MODE_64TH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_64TH_ITEM, true);
		break;
	case STEP_MODE_128TH:
		indigo_set_switch(X_STEP_MODE_PROPERTY, X_STEP_MODE_128TH_ITEM, true);
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_step_mode(%d) wrong value %d", PRIVATE_DATA->handle, value);
	}
}


static void update_coils_mode_switches(indigo_device * device) {
	coilsmode_t value;

	if (!mfp_get_coils_mode(device, &value)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_coils_mode(%d) failed", PRIVATE_DATA->handle);
		return;
	}

	switch (value) {
	case COILS_MODE_IDLE_OFF:
		indigo_set_switch(X_COILS_MODE_PROPERTY, X_COILS_MODE_IDLE_OFF_ITEM, true);
		break;
	case COILS_MODE_ALWAYS_ON:
		indigo_set_switch(X_COILS_MODE_PROPERTY, X_COILS_MODE_ALWAYS_ON_ITEM, true);
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_coils_mode(%d) wrong value %d", PRIVATE_DATA->handle, value);
	}
}

static void focuser_connect_callback(indigo_device *device) {
	uint32_t position;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			} else {
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				char *name = DEVICE_PORT_ITEM->text.value;
				if (!indigo_is_device_url(name, "mfp")) {
					PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
					/* MFP resets on RTS, which is manipulated on connect! Wait for 2 seconds to recover! */
					sleep(1);
				} else {
					indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
					PRIVATE_DATA->handle = indigo_open_network_device(name, 8080, &proto);
				}
				if ( PRIVATE_DATA->handle < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", DEVICE_PORT_ITEM->text.value);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					return;
				} else if (!mfp_get_position(device, &position)) {  // check if it is MFP Focuser first
					int res = close(PRIVATE_DATA->handle);
					if (res < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					}
					indigo_global_unlock(device);
					device->is_connected = false;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: MyFP2 AF did not respond");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, "MyFP2 AF did not respond");
					return;
				} else { // Successfully connected
					char board[MFP_CMD_LEN] = "N/A";
					char firmware[MFP_CMD_LEN] = "N/A";
					uint32_t value;
					if (mfp_get_info(device, board, firmware)) {
						indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
						indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
						indigo_update_property(device, INFO_PROPERTY, NULL);
					}

					mfp_get_position(device, &position);
					FOCUSER_POSITION_ITEM->number.value = (double)position;

					if(!mfp_enable_backlash(device, false)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_enable_backlash(%d) failed", PRIVATE_DATA->handle);
					}

					if (!mfp_get_max_position(device, &PRIVATE_DATA->max_position)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_max_position(%d) failed", PRIVATE_DATA->handle);
					}
					FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;

					if (!mfp_set_speed(device, FOCUSER_SPEED_ITEM->number.value)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_speed(%d) failed", PRIVATE_DATA->handle);
					}
					FOCUSER_SPEED_ITEM->number.target = FOCUSER_SPEED_ITEM->number.value;

					mfp_get_reverse(device, &FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
					FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;

					update_coils_mode_switches(device);
					indigo_define_property(device, X_COILS_MODE_PROPERTY, NULL);

					update_step_mode_switches(device);
					indigo_define_property(device, X_STEP_MODE_PROPERTY, NULL);

					if (!mfp_get_settle_buffer(device, &value)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_settle_buffer(%d) failed", PRIVATE_DATA->handle);
					}
					X_SETTLE_TIME_ITEM->number.value = (double)value;
					X_SETTLE_TIME_ITEM->number.target = (double)value;
					indigo_define_property(device, X_SETTLE_TIME_PROPERTY, NULL);

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);


					FOCUSER_MODE_PROPERTY->hidden = false;
					FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
					mfp_get_temperature(device, &FOCUSER_TEMPERATURE_ITEM->number.value);
					PRIVATE_DATA->prev_temp = FOCUSER_TEMPERATURE_ITEM->number.value;
					FOCUSER_COMPENSATION_PROPERTY->hidden = false;
					FOCUSER_COMPENSATION_ITEM->number.min = -10000;
					FOCUSER_COMPENSATION_ITEM->number.max = 10000;
					indigo_set_timer(device, 1, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);

			mfp_stop(device);
			mfp_save_settings(device);

			indigo_delete_property(device, X_STEP_MODE_PROPERTY, NULL);
			indigo_delete_property(device, X_COILS_MODE_PROPERTY, NULL);
			indigo_delete_property(device, X_SETTLE_TIME_PROPERTY, NULL);

			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			int res = close(PRIVATE_DATA->handle);
			if (res < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		if (!mfp_set_reverse(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_reverse(%d, %d) failed", PRIVATE_DATA->handle, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_w(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else { /* GOTO position */
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				if (!mfp_goto_position_bl(device, (uint32_t)PRIVATE_DATA->target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_goto_position_bl(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				}
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* RESET CURRENT POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				if(!mfp_sync_position(device, PRIVATE_DATA->target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_sync_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				uint32_t position;
				if (!mfp_get_position(device, &position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = (double)position;
				}
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		if (!mfp_set_max_position(device, PRIVATE_DATA->max_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_max_position(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!mfp_get_max_position(device, &PRIVATE_DATA->max_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_max_position(%d) failed", PRIVATE_DATA->handle);
		}
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		if (!mfp_set_speed(device, (uint32_t)FOCUSER_SPEED_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_speed(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			uint32_t position;
			if (!mfp_get_position(device, &position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
			} else {
				PRIVATE_DATA->current_position = (double)position;
			}

			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value;
			} else {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value;
			}

			// Make sure we do not attempt to go beyond the limits
			if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
			} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			if (!mfp_goto_position_bl(device, (uint32_t)PRIVATE_DATA->target_position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_goto_position_bl(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);

		if (!mfp_stop(device)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_stop(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		uint32_t position;
		if (!mfp_get_position(device, &position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->current_position = (double)position;
		}
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH_PROPERTY
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(X_STEP_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_STEP_MODE_PROPERTY
		indigo_property_copy_values(X_STEP_MODE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		X_STEP_MODE_PROPERTY->state = INDIGO_OK_STATE;
		stepmode_t mode = STEP_MODE_FULL;
		if(X_STEP_MODE_FULL_ITEM->sw.value) {
			mode = STEP_MODE_FULL;
		} else if(X_STEP_MODE_HALF_ITEM->sw.value) {
			mode = STEP_MODE_HALF;
		} else if(X_STEP_MODE_FOURTH_ITEM->sw.value) {
			mode = STEP_MODE_FOURTH;
		} else if(X_STEP_MODE_EIGTH_ITEM->sw.value) {
			mode = STEP_MODE_EIGTH;
		} else if(X_STEP_MODE_16TH_ITEM->sw.value) {
			mode = STEP_MODE_16TH;
		} else if(X_STEP_MODE_32TH_ITEM->sw.value) {
			mode = STEP_MODE_32TH;
		} else if(X_STEP_MODE_64TH_ITEM->sw.value) {
			mode = STEP_MODE_64TH;
		} else if(X_STEP_MODE_128TH_ITEM->sw.value) {
			mode = STEP_MODE_128TH;
		}
		if (!mfp_set_step_mode(device, mode)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_step_mode(%d, %d) failed", PRIVATE_DATA->handle, mode);
			X_STEP_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		update_step_mode_switches(device);
		indigo_update_property(device, X_STEP_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_SETTLE_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SETTLE_TIME_PROPERTY
		indigo_property_copy_values(X_SETTLE_TIME_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		X_SETTLE_TIME_PROPERTY->state = INDIGO_OK_STATE;

		if (!mfp_set_settle_buffer(device, (uint32_t)X_SETTLE_TIME_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_settle_buffer(%d, %d) failed", PRIVATE_DATA->handle, (uint32_t)X_SETTLE_TIME_ITEM->number.target);
			X_SETTLE_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		uint32_t value;
		if (!mfp_get_settle_buffer(device, &value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_settle_buffer(%d) failed", PRIVATE_DATA->handle);
		} else {
			X_SETTLE_TIME_ITEM->number.target = (double)value;
		}

		indigo_update_property(device, X_SETTLE_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_COILS_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_COILS_MODE_PROPERTY
		indigo_property_copy_values(X_COILS_MODE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		X_COILS_MODE_PROPERTY->state = INDIGO_OK_STATE;
		coilsmode_t mode = COILS_MODE_IDLE_OFF;
		if(X_COILS_MODE_IDLE_OFF_ITEM->sw.value) {
			mode = COILS_MODE_IDLE_OFF;
		} else if(X_COILS_MODE_ALWAYS_ON_ITEM->sw.value) {
			mode = COILS_MODE_ALWAYS_ON;
		}
		if (!mfp_set_coils_mode(device, mode)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_coils_mode(%d, %d) failed", PRIVATE_DATA->handle, mode);
			X_COILS_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		update_coils_mode_switches(device);
		indigo_update_property(device, X_COILS_MODE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_STEP_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_COILS_MODE_PROPERTY);
			indigo_save_property(device, NULL, X_SETTLE_TIME_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(X_STEP_MODE_PROPERTY);
	indigo_release_property(X_COILS_MODE_PROPERTY);
	indigo_release_property(X_SETTLE_TIME_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	return indigo_focuser_detach(device);
}


// --------------------------------------------------------------------------------
indigo_result indigo_focuser_mypro2(indigo_driver_action action, indigo_driver_info *info) {
	static mfp_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_MFP2_NAME,
		focuser_attach,
		mfp_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "myFocuserPro2 Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		private_data = indigo_safe_malloc(sizeof(mfp_private_data));
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
