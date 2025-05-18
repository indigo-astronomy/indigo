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
// 2.0 by Rumen G. Bogdanovski

/** Q-Focuser focuser driver
	\file indigo_focuser_qhy.c
*/

#define DRIVER_VERSION 0x0004
#define DRIVER_NAME "indigo_focuser_qhy"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <indigo/indigo_driver_xml.h>

#include <indigo/indigo_io.h>

#include "indigo_focuser_qhy.h"

#define SERIAL_BAUDRATE            "9600"

#define PRIVATE_DATA                    ((qhy_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                    gp_bits

#define NO_TEMP_READING                (-50)

#define CB_SIZE 5
typedef struct {
	double buffer[CB_SIZE];
	int head;
	int tail;
} circular_buffer;

typedef struct {
	int handle;
	int32_t current_position, target_position;
	double prev_temp;
	circular_buffer temperature_buffer;
	indigo_timer *focuser_timer, *temperature_timer;
	pthread_mutex_t port_mutex;
} qhy_private_data;

static void compensate_focus(indigo_device *device, double new_temp);

/* Circular buffer functions ================================================================= */

static void cb_clear(circular_buffer* cb) {
	cb->head = 0;
	cb->tail = 0;
}

static void cb_write(circular_buffer* cb, double data) {
	if ((cb->head + 1) % CB_SIZE == cb->tail) {
		// Overwrite the oldest data when the buffer is full
		cb->tail = (cb->tail + 1) % CB_SIZE;
	}
	cb->buffer[cb->head] = data;
	cb->head = (cb->head + 1) % CB_SIZE;
}

static double cb_average(circular_buffer* cb) {
	if (cb->head == cb->tail) {
		return NO_TEMP_READING;  // Return NO_TEMP_READING when the buffer is empty
	}

	double sum = 0.0;
	int count = 0;
	int index = cb->tail;

	while (index != cb->head) {
		sum += cb->buffer[index];
		count++;
		index = (index + 1) % CB_SIZE;
	}

	return sum / count;
}

/* simple JSON parser ==================================================================== */

#define MAX_KV 50
#define MAX_KEY_LEN 50
#define MAX_VALUE_LEN 50

#define MAX_CMD_LEN 150

typedef struct kv {
	char key[MAX_KEY_LEN];
	char value[MAX_VALUE_LEN];
} kv_t;

typedef struct {
	int idx;
	union {
		struct {
			char version[MAX_VALUE_LEN];
			char version_board[MAX_VALUE_LEN];
		};
		struct {
			double chip_temp;
			double voltage;
			double out_temp;
		};
		int position;
	};
} qhy_response;

typedef enum {
	START,
	KEY,
	COLON,
	VALUE,
	COMMA
} json_state_t;

static int json_parse(kv_t* kvs, const char* json) {
    json_state_t state = START;
    char buffer[MAX_VALUE_LEN];
    int buffer_index = 0;
    int key_index = 0;
    int is_string = 0;

    for (int i = 0; json[i] != '\0' ; i++) {
        char c = json[i];
        switch (state) {
		case START:
			if (c == '{') {
				state = KEY;
			}
			break;
		case KEY:
			if (c == '\"') {
				kvs[key_index] = (kv_t) {0};
				if (buffer_index != 0) {
					buffer[buffer_index] = '\0';
					buffer_index = 0;
					state = COLON;
				}
			} else {
				buffer[buffer_index++] = c;
				if (buffer_index == MAX_KEY_LEN) {
					return -1;
				}
			}
			break;
		case COLON:
			if (c == ':') {
				strncpy(kvs[key_index].key, buffer, MAX_KEY_LEN);
				state = VALUE;
				is_string = 0;
			}
			break;
		case VALUE:
			if (c == '\"') {
				is_string = 1;
				if (buffer_index != 0) {
					buffer[buffer_index] = '\0';
					buffer_index = 0;
					strncpy(kvs[key_index].value, buffer, MAX_VALUE_LEN);
					state = COMMA;
				}
			} else if(is_string == 0 && (c == ',' || c == '}')) {
				if (buffer_index != 0) {
					buffer[buffer_index] = '\0';
					buffer_index = 0;
					strncpy(kvs[key_index].value, buffer, MAX_VALUE_LEN);
					state = KEY;
					key_index++;
				}
			} else {
				buffer[buffer_index++] = c;
				if (buffer_index == MAX_VALUE_LEN) {
					return -2;
				}
			}
			break;
		case COMMA:
			if (c == ',') {
				state = KEY;
				key_index++;
				if(key_index == MAX_KV) {
					return -3;
				}
			}
			break;
        }
    }
	return 0;
}

static int qhy_parse_response(char *response, qhy_response *qresponse) {
	kv_t kvs[10] = {0};

    int res = json_parse(kvs, response);
	if (res < 0) return res;

	int cmd_id = -1;

	int i = 0;
    while (kvs[i].key[0] != '\0') {
        if(!strcmp(kvs[i].key, "idx")) {
			cmd_id = atoi(kvs[i].value);
			break;
		}
        i++;
    }

	qresponse->idx = cmd_id;

	if (cmd_id == 2 || cmd_id == 3 || cmd_id == 6 || cmd_id == 7 || cmd_id == 11 || cmd_id == 13 || cmd_id == 16) {
		return 0;
	} else if (cmd_id == 1) {
		int i = 0;
    	while (kvs[i].key[0] != '\0') {
        	if(!strcmp(kvs[i].key, "version")) {
				strncpy(qresponse->version, kvs[i].value, 50);
			} else if(!strcmp(kvs[i].key, "bv")) {
				strncpy(qresponse->version_board, kvs[i].value, 50);
			}
        	i++;
		}
	} else if (cmd_id == 4) {
		int i = 0;
    	while (kvs[i].key[0] != '\0') {
        	if(!strcmp(kvs[i].key, "o_t")) {
				qresponse->out_temp = atof(kvs[i].value)/1000.0;
			} else if(!strcmp(kvs[i].key, "c_t")) {
				qresponse->chip_temp = atof(kvs[i].value)/1000.0;
			} else if(!strcmp(kvs[i].key, "c_r")) {
				qresponse->voltage = atof(kvs[i].value)/10.0;
			}
        	i++;
		}
	} else if (cmd_id == 5) {
		int i = 0;
    	while (kvs[i].key[0] != '\0') {
        	if(!strcmp(kvs[i].key, "pos")) {
				qresponse->position = atoi(kvs[i].value);
				break;
			}
		  	i++;
		}
	} else if (cmd_id == -1) {
		// I have no idea why it is responding with -1, Cable reconnect usually fixes it!!!
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Q-Focuser responded with command ID = %d - ubnormal situation. Try reconnecting the focuser!\n", cmd_id);
		return -1;
	}
	return 0;
}

static void qhy_print_response(qhy_response resp) {
	int cmd_id = resp.idx;
	if (cmd_id == 2 || cmd_id == 3 || cmd_id == 6 || cmd_id == 7 || cmd_id == 11 || cmd_id == 13 || cmd_id == 16) {
		indigo_error("command %d returned: no value\n", cmd_id);
	} else if (cmd_id == 1) {
		indigo_error("command %d returned: version = %s, board_version = %s\n", cmd_id, resp.version, resp.version_board);
	} else if (cmd_id == 4) {
		indigo_error("command %d returned: out_temp = %g, chip_temp = %g, voltage = %g\n", cmd_id, resp.out_temp, resp.chip_temp, resp.voltage);
	} else if (cmd_id == 5) {
		indigo_error("command %d returned: position = %d\n", cmd_id, resp.position);
	} else if (cmd_id == -1) {
		indigo_error("command %d - ubnormal situation\n", cmd_id);
	} else {
		indigo_error("command %d - unknown\n", cmd_id);
	}
}


/* QHY Q-Focuser Commands ======================================================================== */

static bool qhy_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
	char c;
	struct timeval tv;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	// flush
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
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

			if (c == '}')
				break;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static int qhy_simple_command(indigo_device *device, int cmd_id, qhy_response *parsed_response) {
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":%d}", cmd_id);

	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, parsed_response);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

static int qhy_get_version(indigo_device *device, char *version, char *board_version) {
	qhy_response parsed_response;
	int result = qhy_simple_command(device, 1, &parsed_response);
	if (result < 0) {
		return result;
	}
	if(parsed_response.idx != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Responce expected 1 received %d", parsed_response.idx);
		return -1;
	}
	strncpy(version, parsed_response.version, 50);
	strncpy(board_version, parsed_response.version_board, 50);
	return 0;
}

static int qhy_relative_move(indigo_device *device, bool out, int steps) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":2,\"dir\":%d,\"step\":%d}", out ? 1 : -1, steps);
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 2) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

static int qhy_abort(indigo_device *device) {
	qhy_response response;
	int result = qhy_simple_command(device, 3, &response);
	if (result < 0) {
		return result;
	}
	if(response.idx != 3 && response.idx != 5) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Responce expected 3 or 5 received %d", response.idx);
		return -1;
	}
	return 0;
}

int qhy_get_temperature_voltage(indigo_device *device, double *chip_temp, double *out_temp, double *voltage) {
	qhy_response parsed_response;
	int result = qhy_simple_command(device, 4, &parsed_response);
	if (result < 0) {
		return result;
	}
	if(parsed_response.idx != 4) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Responce expected 4 received %d", parsed_response.idx);
		return -1;
	}
	if (chip_temp) *chip_temp = parsed_response.chip_temp;
	if (out_temp) *out_temp = parsed_response.out_temp;
	if (voltage) *voltage = parsed_response.voltage;
	return 0;
}

static int qhy_get_position(indigo_device *device, int *position) {
	qhy_response parsed_response;
	int result = qhy_simple_command(device, 5, &parsed_response);
	if (result < 0) {
		return result;
	}
	if(parsed_response.idx != 5) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Responce expected 4 received %d", parsed_response.idx);
		return -1;
	}
	*position = parsed_response.position;
	return 0;
}

static int qhy_absolute_move(indigo_device *device, int position) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":6,\"tar\":%d}", position);
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 6) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

static int qhy_set_reverse(indigo_device *device, bool reverse) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":7,\"rev\":%d}", reverse ? 1 : 0);
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 7) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

static int qhy_sync_position(indigo_device *device, int position) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":11,\"init_val\":%d}", position);
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 11) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

static int qhy_set_speed(indigo_device *device, int speed) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":13,\"speed\":%d}", speed);
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 13) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

// From INDI driver - no idea what those hadcoded values mean
static int qhy_set_hold(indigo_device *device) {
	qhy_response parsed_response;
	char command[MAX_CMD_LEN];
	char response[MAX_CMD_LEN];
	sprintf(command, "{\"cmd_id\":16,\"ihold\":0,\"irun\":5}");
	int result = qhy_command(device, command, response, MAX_CMD_LEN, 0);
	if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command '%s' failed", command);
		return result;
	}
	result = qhy_parse_response(response, &parsed_response);
	if (result < 0 || parsed_response.idx != 16) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Parsing response '%s' failed with %d", response, result);
	}
	return result;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	int position;

	if (qhy_get_position(device, &position) < 0)  {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_position(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->current_position = (double)position;
	}

	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void temperature_timer_callback(indigo_device *device) {
	double temp, temp_sample, chip_temp, voltage;
	static bool has_valid_temperature = true;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;

	if (qhy_get_temperature_voltage(device, &chip_temp, &temp_sample, &voltage) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_temperature_voltage(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"qhy_get_temperature_voltage(%d, -> %f, %f, %f) succeeded",
			PRIVATE_DATA->handle,
			temp_sample,
			chip_temp,
			voltage
		);
		if (temp_sample <= NO_TEMP_READING) {
			temp_sample = chip_temp;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No outside temperature reading, using chip temperature: %f", chip_temp);
		}

		// write to circular buffer and calculate average temperature to reduce noise
		cb_write(&PRIVATE_DATA->temperature_buffer, temp_sample);
		temp = cb_average(&PRIVATE_DATA->temperature_buffer);

		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Temperature: temp_sample = %f, chip_temp = %f, average_temp = %f",
			temp_sample,
			chip_temp,
			temp
		);

		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
	}

	if (FOCUSER_TEMPERATURE_ITEM->number.value <= NO_TEMP_READING) { /* < -50 is returned when the sensor is not connected */
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (has_valid_temperature) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "No valid temperature reading.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "No valid temperature reading.");
			has_valid_temperature = false;
		}
	} else {
		has_valid_temperature = true;
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
	if ((fabs(temp_difference) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.0f, threshold = %.2f",
			temp_difference,
			compensation,
			FOCUSER_COMPENSATION_ITEM->number.value,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
	} else {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Not compensating (not needed): temp_difference = %.2f, threshold = %.2f",
			temp_difference,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	int current_position;

	if (qhy_get_position(device, &current_position) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_position(%d) failed", PRIVATE_DATA->handle);
	}

	PRIVATE_DATA->current_position = (double)current_position;

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	if (qhy_absolute_move(device, PRIVATE_DATA->target_position) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_absolute_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}

static indigo_result qhy_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
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
		INFO_PROPERTY->count = 7;

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 2000000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;

		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = 1000;

		FOCUSER_SPEED_PROPERTY->hidden = false;
		FOCUSER_SPEED_ITEM->number.min = 1;
		FOCUSER_SPEED_ITEM->number.max = 8;
		FOCUSER_SPEED_ITEM->number.step = 1;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		strncpy(FOCUSER_SPEED_ITEM->label, "Speed (1 = fastest, 8 = slowest)", INDIGO_NAME_SIZE);

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 10;
		FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 10;
		FOCUSER_STEPS_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max;

		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -100000;
		FOCUSER_COMPENSATION_ITEM->number.max = 100000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = true;

		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
	int position;
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
				if (!indigo_is_device_url(name, "qfocuser")) {
					PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
					/* To be on the safe side - wait for 1 sec! */
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
				} else if (qhy_get_position(device, &position) < 0) {  // check if it is QHY Focuser first
					int res = close(PRIVATE_DATA->handle);
					if (res < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					} else {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "close(%d) = %d", PRIVATE_DATA->handle, res);
					}
					indigo_global_unlock(device);
					device->is_connected = false;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "connect failed: Q-Focuser did not respond");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, "Q-Focuser did not respond");
					return;
				} else { // Successfully connected
					char board[MAX_CMD_LEN] = "N/A";
					char firmware[MAX_CMD_LEN] = "N/A";
					if (qhy_get_version(device, firmware, board) == 0) {
						indigo_copy_value(INFO_DEVICE_HW_REVISION_ITEM->text.value, board);
						indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
						indigo_update_property(device, INFO_PROPERTY, NULL);
					}

					qhy_get_position(device, &position);
					FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = (double)position;
					PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;

					if (qhy_set_speed(device, FOCUSER_SPEED_ITEM->number.value) < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_set_speed(%d) failed", PRIVATE_DATA->handle);
					}
					FOCUSER_SPEED_ITEM->number.target = FOCUSER_SPEED_ITEM->number.value;

					if (qhy_set_reverse(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_set_reverse(%d) failed", PRIVATE_DATA->handle);
					}

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);

					double voltage = 0;
					/* Copied from INDI driver - no idea why if power is off hold is set */
					if(qhy_get_temperature_voltage(device, NULL, NULL, &voltage) < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_temperature_voltage(%d) failed", PRIVATE_DATA->handle);
					} else if (voltage == 0) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "Voltage is 0.0 V, focuser is running with no external power.");
						qhy_set_hold(device);
					}

					cb_clear(&PRIVATE_DATA->temperature_buffer);
					indigo_set_timer(device, 0, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);

			qhy_abort(device);

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
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		if (qhy_set_reverse(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value) < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_set_reverse(%d, %d) failed", PRIVATE_DATA->handle, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.target < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value ||
			FOCUSER_POSITION_ITEM->number.target > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value
		) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
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
				if (qhy_absolute_move(device, PRIVATE_DATA->target_position) < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_absolute_move(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				}
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* RESET CURRENT POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				if(qhy_sync_position(device, PRIVATE_DATA->target_position) < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_sync_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				int position;
				if (qhy_get_position(device, &position) < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_position(%d) failed", PRIVATE_DATA->handle);
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
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		if (qhy_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_set_speed(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
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
			int position;
			if (qhy_get_position(device, &position) < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_position(%d) failed", PRIVATE_DATA->handle);
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
			if (qhy_absolute_move(device, PRIVATE_DATA->target_position) < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_goto_position(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);

		if (qhy_abort(device) < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_stop(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		int position;
		if (qhy_get_position(device, &position) < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "qhy_get_position(%d) failed", PRIVATE_DATA->handle);
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
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
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
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
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
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	return indigo_focuser_detach(device);
}


// --------------------------------------------------------------------------------
indigo_result indigo_focuser_qhy(indigo_driver_action action, indigo_driver_info *info) {
	static qhy_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_QHY_NAME,
		focuser_attach,
		qhy_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "QHY Q-Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		private_data = indigo_safe_malloc(sizeof(qhy_private_data));
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
