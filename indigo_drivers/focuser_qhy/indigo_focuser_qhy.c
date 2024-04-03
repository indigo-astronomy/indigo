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

#define DRIVER_VERSION 0x0006
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

#define PRIVATE_DATA                    ((mfp_private_data *)device->private_data)

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
} mfp_private_data;

static void compensate_focus(indigo_device *device, double new_temp);

/* My FP2 Commands ======================================================================== */

#define MFP_CMD_LEN 100

#define NO_TEMP_READING                (-127)

static bool mfp_command(indigo_device *device, const char *command, char *response, int max, int sleep) {
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
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

// QHY commands
#define MAX_KV 50
#define MAX_KEY_LEN 50
#define MAX_VALUE_LEN 50

typedef struct kv {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} kv_t;

typedef enum {
    START,
    KEY,
    COLON,
    VALUE,
    COMMA
} json_state_t;

int json_parse(kv_t* kvs, const char* json) {
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
					//printf("%d: key: %s, value: %s\n", key_index, kvs[key_index].key, kvs[key_index].value);
					state = COMMA;
				}
			} else if(is_string == 0 && (c == ',' || c == '}')) {
				if (buffer_index != 0) {
					buffer[buffer_index] = '\0';
					buffer_index = 0;
					strncpy(kvs[key_index].value, buffer, MAX_VALUE_LEN);
					//printf("%d: key: %s, value: %s\n", key_index, kvs[key_index].key, kvs[key_index].value);
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

typedef struct {
	int idx;
	union {
		struct {
			char version[50];
			char version_board[50];
		};
		struct {
			double chip_temp;
			double voltage;
			double out_temp;
		};
		int position;
	};
} qhy_response;

int qhy_simple_command(int device, int cmd_id) {
	char command[50];
	sprintf(command, "{\"cmd_id\":%d}", cmd_id);
	return indigo_write(device, command, strlen(command));
}

int qhy_request_version(int device) {
	return qhy_simple_command(device, 1);
}

int qhy_relative_move(int device, bool out, int steps) {
	char command[50];
	sprintf(command, "{\"cmd_id\":2,\"dir\":%d,\"step\":%d}", out ? 1 : -1, steps);
	return indigo_write(device, command, strlen(command));
}

int qhy_abort(int device) {
	return qhy_simple_command(device, 3);
}

int qhy_request_temperature(int device) {
	return qhy_simple_command(device, 4);
}

int qhy_request_position(int device) {
	return qhy_simple_command(device, 5);
}

int qhy_absolute_move(int device, int position) {
	char command[50];
	sprintf(command, "{\"cmd_id\":6,\"tar\":%d}", position);
	return indigo_write(device, command, strlen(command));
}

int qhy_set_reverse(int device, bool reverse) {
	char command[50];
	sprintf(command, "{\"cmd_id\":7,\"rev\":%d}", reverse ? 1 : 0);
	return indigo_write(device, command, strlen(command));
}

int qhy_sync_position(int device, int position) {
	char command[50];
	sprintf(command, "{\"cmd_id\":11,\"init_val\":%d}", position);
	return indigo_write(device, command, strlen(command));
}

int qhy_set_speed(int device, int speed) {
	char command[50];
	sprintf(command, "{\"cmd_id\":13,\"speed\":%d}", speed);
	return indigo_write(device, command, strlen(command));
}

/* From INDI driver - no idea what those hadcoded values mean */
int qhy_set_hold(int device) {
	char command[50];
	sprintf(command, "{\"cmd_id\":16,\"ihold\":0,\"irun\":5}");
	return indigo_write(device, command, strlen(command));
}

int qhy_parse_response(char *response, qhy_response *qresponse) {
	kv_t kvs[10];

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
		// isreboot ???
		return -1;
	}
	return 0;
}

void print_resp(qhy_response resp) {
	int cmd_id = resp.idx;

	if (cmd_id == 2 || cmd_id == 3 || cmd_id == 6 || cmd_id == 7 || cmd_id == 11 || cmd_id == 13 || cmd_id == 16) {
		printf("command %d returned: no value\n", cmd_id);
	} else if (cmd_id == 1) {
		printf("command %d returned: version = %s, board_version = %s\n", cmd_id, resp.version, resp.version_board);
	} else if (cmd_id == 4) {
		printf("command %d returned: out_temp = %g, chip_temp = %g, voltage = %g\n", cmd_id, resp.out_temp, resp.chip_temp, resp.voltage);
	} else if (cmd_id == 5) {
		printf("command %d returned: position = %d\n", cmd_id, resp.position);
	} else if (cmd_id == -1) {

	} else {
		printf("command %d - unknown\n", cmd_id);
	}
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving;
	uint32_t position;

	/*
	if (!mfp_get_position(device, &position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->current_position = (double)position;
	}
	*/

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
	/*
	if (!mfp_get_temperature(device, &temp)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_temperature(%d, -> %f) failed", PRIVATE_DATA->handle, temp);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_TEMPERATURE_ITEM->number.value = temp;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "mfp_get_temperature(%d, -> %f) succeeded", PRIVATE_DATA->handle, FOCUSER_TEMPERATURE_ITEM->number.value);
	}
	*/

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

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	uint32_t current_position;
	/*
	if (!mfp_get_position(device, &current_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_position(%d) failed", PRIVATE_DATA->handle);
	}
	*/
	PRIVATE_DATA->current_position = (double)current_position;

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	/*
	if (!mfp_goto_position_bl(device, (uint32_t)PRIVATE_DATA->target_position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_goto_position_bl(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	*/

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}


static indigo_result mfp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
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

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
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
				if (!indigo_is_device_url(name, "qfocuser")) {
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
				} else if ( 0 /*!mfp_get_position(device, &position) */ ) {  // check if it is MFP Focuser first
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
					mfp_command(device, "{\"cmd_id\":1}", board, MFP_CMD_LEN, 100);
					uint32_t value;
					/*
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
					*/

					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					device->is_connected = true;

					indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);


					FOCUSER_MODE_PROPERTY->hidden = false;
					FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
					//mfp_get_temperature(device, &FOCUSER_TEMPERATURE_ITEM->number.value);
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

			//mfp_stop(device);
			//mfp_save_settings(device);

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
		/*
		if (!mfp_set_reverse(device, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_reverse(%d, %d) failed", PRIVATE_DATA->handle, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		*/
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
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
				/*
				if (!mfp_goto_position_bl(device, (uint32_t)PRIVATE_DATA->target_position)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_goto_position_bl(%d, %d) failed", PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				}
				*/
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* RESET CURRENT POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				/*
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
				*/
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		/*
		if (!mfp_set_max_position(device, PRIVATE_DATA->max_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_max_position(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (!mfp_get_max_position(device, &PRIVATE_DATA->max_position)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_get_max_position(%d) failed", PRIVATE_DATA->handle);
		}
		*/
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		/*
		if (!mfp_set_speed(device, (uint32_t)FOCUSER_SPEED_ITEM->number.target)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "mfp_set_speed(%d) failed", PRIVATE_DATA->handle);
			FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		*/
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
			uint32_t position;
			/*
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
			*/
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

		/*
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
		*/
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
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH_PROPERTY
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
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
	static mfp_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_QHY_NAME,
		focuser_attach,
		mfp_enumerate_properties,
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
