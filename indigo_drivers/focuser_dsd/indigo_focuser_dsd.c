// Copyright (C) 2019 Rumen G. Bogdanovski
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

/** INDIGO DSD focuser driver
 \file indigo_focuser_dsd.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_dsd"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "indigo_driver_xml.h"

#include "indigo_io.h"
#include "indigo_focuser_dsd.h"

#define dsd_VENDOR_ID                   0x03c3

#define PRIVATE_DATA                    ((dsd_private_data *)device->private_data)

#define EAF_BEEP_PROPERTY               (PRIVATE_DATA->beep_property)
#define EAF_BEEP_ON_ITEM                (EAF_BEEP_PROPERTY->items+0)
#define EAF_BEEP_OFF_ITEM               (EAF_BEEP_PROPERTY->items+1)
#define EAF_BEEP_PROPERTY_NAME          "EAF_BEEP_ON_MOVE"
#define EAF_BEEP_ON_ITEM_NAME           "ON"
#define EAF_BEEP_OFF_ITEM_NAME          "OFF"


// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int handle;
	//EAF_INFO info;
	int current_position, target_position, max_position, backlash;
	double prev_temp;
	indigo_timer *focuser_timer, *temperature_timer;
	pthread_mutex_t port_mutex;
	indigo_property *beep_property;
} dsd_private_data;

static void compensate_focus(indigo_device *device, double new_temp);

/* Deepsky Dad Commands ======================================================================== */

#define DSD_CMD_LEN 100

static bool dsd_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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

			if (c == ')')
				break;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}


static bool dsd_get_info(indigo_device *device, char *board, char *firmware) {
	if(!board || !firmware) return false;

	char response[DSD_CMD_LEN]={0};
	if (dsd_command(device, "[GFRM]", response, sizeof(response), 100)) {
		int parsed = sscanf(response, "(Board=%[^','], Version=%[^')'])", board, firmware);
		if (parsed != 2) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RESPONSE: %s %s %s", response, board, firmware);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool dsd_get_position(indigo_device *device, uint32_t *pos) {
	if (!pos) return false;

	char response[DSD_CMD_LEN]={0};
	if (dsd_command(device, "[GPOS]", response, sizeof(response), 100)) {
		int parsed = sscanf(response, "(%d)", pos);
		if (parsed != 1) return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RESPONSE: %s %d", response, *pos);
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "NO response");
	return false;
}


static bool dsd_sync_position(indigo_device *device, uint32_t pos) {
	char cmd[DSD_CMD_LEN];
	char resp[DSD_CMD_LEN];
	snprintf(cmd, DSD_CMD_LEN, "[SPOS%06d]", pos);
	return dsd_command(device, cmd, resp, sizeof(resp), 100);
}


static bool dsd_set_reverse(indigo_device *device, bool enabled) {
	char cmd[DSD_CMD_LEN];
	char resp[DSD_CMD_LEN];
	snprintf(cmd, DSD_CMD_LEN, "[SREV%01d]", enabled ? 1 : 0);
	return dsd_command(device, cmd, resp, sizeof(resp), 100);
}


static bool dsd_goto_position(indigo_device *device, uint32_t position) {
	char cmd[DSD_CMD_LEN];
	char resp[DSD_CMD_LEN] = {0};

	snprintf(cmd, DSD_CMD_LEN, "[STRG%06d]", position);

	// Set Position First
	if (!dsd_command(device, cmd, resp, sizeof(resp), 100)) return false;

	if(strcmp(resp, "!101)") == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Move failed");
		return false;
	}

	// Start motion toward position
	return dsd_command(device, "[SMOV]", resp, sizeof(resp), 100);
}


// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving, moving_HC;
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	/*
	int res = EAFGetPosition(PRIVATE_DATA->handle, &(PRIVATE_DATA->current_position));
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->handle, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->current_position, res);

	res = EAFIsMoving(PRIVATE_DATA->handle, &moving, &moving_HC);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsMoving(%d) = %d", PRIVATE_DATA->handle, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	*/
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
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
	float temp;
	static bool has_sensor = true;
	static bool first_call = true;
	bool has_handcontrol;
	bool moving = false, moving_HC = false;

	/*
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	int res = EAFIsHandControl(PRIVATE_DATA->handle, &has_handcontrol);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsHandControl(%d) = %d", PRIVATE_DATA->handle, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFIsHandControl(%d, <- %d) = %d", PRIVATE_DATA->handle, has_handcontrol, res);
	}

	// Update temerature at first call even if HC is connected.
	//   We want alert sate and T=-273 if the sensor is not connected.
	if (first_call) has_handcontrol = false;

	if (has_handcontrol) {
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		res = EAFGetPosition(PRIVATE_DATA->handle, &(PRIVATE_DATA->current_position));
		//fprintf(stderr, "pos=%d\n", PRIVATE_DATA->current_position);
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->current_position, res);
		} else if (FOCUSER_POSITION_ITEM->number.value != PRIVATE_DATA->current_position) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->current_position, res);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			EAFIsMoving(PRIVATE_DATA->handle, &moving, &moving_HC);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			if (moving) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			FOCUSER_POSITION_ITEM->number.value = (double)PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else { // No Hand control, this does not guarantee that we have temperature sensor
		first_call = false;
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		res = EAFGetTemp(PRIVATE_DATA->handle, &temp);
		FOCUSER_TEMPERATURE_ITEM->number.value = (double)temp;
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->handle, FOCUSER_TEMPERATURE_ITEM->number.value, res);
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->handle, FOCUSER_TEMPERATURE_ITEM->number.value, res);
		}
		// static double ctemp = 0;
		// FOCUSER_TEMPERATURE_ITEM->number.value = ctemp;
		// temp = ctemp;
		// ctemp += 0.12;
		if (FOCUSER_TEMPERATURE_ITEM->number.value < -270.0) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
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
			PRIVATE_DATA->prev_temp = -273;
		}
	}
	*/
	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;

	/* we do not have previous temperature reading */
	if (PRIVATE_DATA->prev_temp < -270) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}

	/* we do not have current temperature reading or focuser is moving */
	if ((new_temp < -270) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	/* temperature difference if more than 1 degree so compensation needed */
	if ((abs(temp_difference) >= 1.0) && (abs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: temp_difference = %.2f, Compensation = %d, steps/degC = %.1f", temp_difference, compensation, FOCUSER_COMPENSATION_ITEM->number.value);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating (not needed): temp_difference = %f", temp_difference);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	/*
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	int res = EAFGetPosition(PRIVATE_DATA->handle, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->handle, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	*/
	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	/*

	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	res = EAFMove(PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->target_position, res);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	*/
	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
}


static indigo_result eaf_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(EAF_BEEP_PROPERTY, property))
			indigo_define_property(device, EAF_BEEP_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------

		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		//FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d",device->name ,PRIVATE_DATA->info.MaxStep);

		FOCUSER_SPEED_PROPERTY->hidden = true;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 10000;
		FOCUSER_BACKLASH_ITEM->number.step = 1;

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;

		// -------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		// -------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		EAF_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, EAF_BEEP_PROPERTY_NAME, "Advanced", "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (EAF_BEEP_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(EAF_BEEP_ON_ITEM, EAF_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(EAF_BEEP_OFF_ITEM, EAF_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);

		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
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
					if (strncmp(name, "dsd://", 8)) {
						PRIVATE_DATA->handle = indigo_open_serial(name);
						/* DSD resets on RTS, which is manipulated on connect! Wait for 2 seconds to recover! */
						sleep(2);
					} else {
						char *host = name + 8;
						char *colon = strchr(host, ':');
						if (colon == NULL) {
							PRIVATE_DATA->handle = indigo_open_tcp(host, 8080);
						} else {
							char host_name[INDIGO_NAME_SIZE];
							strncpy(host_name, host, colon - host);
							host_name[colon - host] = 0;
							int port = atoi(colon + 1);
							PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
						}
					}
					if ( PRIVATE_DATA->handle < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, " indigo_open_serial(%s): failed", DEVICE_PORT_ITEM->text.value);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					} else {
						int position;
						if (dsd_get_position(device, &position)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "RESPONSE: %d", position);
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}

						if (dsd_sync_position(device, 1234)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "sync OK");
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}

						position;
						if (dsd_get_position(device, &position)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "RESPONSE: %d", position);
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}

						if (dsd_goto_position(device, 1200)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "goto OK");
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}

						position;
						if (dsd_get_position(device, &position)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "RESPONSE: %d", position);
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}


						char board[100];
						char firmware[100];
						if (dsd_get_info(device, board, firmware)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "RESPONSE: #%s# #%s#", board, firmware);
						} else {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, " NO response");
						}
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

						device->is_connected = true;
						PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
						PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0.1, temperature_timer_callback);
					}
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				indigo_delete_property(device, EAF_BEEP_PROPERTY, NULL);
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
	} else if (indigo_property_match(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		/*
		int res = EAFSetReverse(PRIVATE_DATA->handle, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetReverse(%d, %d) = %d", PRIVATE_DATA->handle, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		*/
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
				/*
				int res = EAFMove(PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->target_position, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				*/
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
			} else { /* RESET CURRENT POSITION */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
				/*
				int res = EAFResetPostion(PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFResetPostion(%d, %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->target_position, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				res = EAFGetPosition(PRIVATE_DATA->handle, &PRIVATE_DATA->current_position);
				FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->handle, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				*/
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			}
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		/*
		int res = EAFSetMaxStep(PRIVATE_DATA->handle, PRIVATE_DATA->max_position);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetMaxStep(%d, -> %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->max_position, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetMaxStep(%d) = %d", PRIVATE_DATA->handle, res);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetMaxStep(PRIVATE_DATA->handle, &(PRIVATE_DATA->max_position));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->handle, res);
		}
		*/
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
		/*
		int res = EAFSetBacklash(PRIVATE_DATA->handle, PRIVATE_DATA->backlash);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetBacklash(%d, -> %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->backlash, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBacklash(%d) = %d", PRIVATE_DATA->handle, res);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetBacklash(PRIVATE_DATA->handle, &(PRIVATE_DATA->backlash));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->handle, res);
		}
		*/
		FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
			/*
			int res = EAFGetPosition(PRIVATE_DATA->handle, &PRIVATE_DATA->current_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->handle, res);
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
			res = EAFMove(PRIVATE_DATA->handle, PRIVATE_DATA->target_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->handle, PRIVATE_DATA->target_position, res);
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			*/
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			PRIVATE_DATA->focuser_timer = indigo_set_timer(device, 0.5, focuser_timer_callback);
		}
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		/*
		int res = EAFStop(PRIVATE_DATA->handle);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFStop(%d) = %d", PRIVATE_DATA->handle, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetPosition(PRIVATE_DATA->handle, &PRIVATE_DATA->current_position);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->handle, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		*/
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
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
	} else if (indigo_property_match(EAF_BEEP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- EAF_BEEP_PROPERTY
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_property_copy_values(EAF_BEEP_PROPERTY, property, false);
		EAF_BEEP_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
		/*
		int res = EAFSetBeep(PRIVATE_DATA->handle, EAF_BEEP_ON_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBeep(%d, %d) = %d", PRIVATE_DATA->handle, EAF_BEEP_ON_ITEM->sw.value, res);
			EAF_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		*/
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		indigo_update_property(device, EAF_BEEP_PROPERTY, NULL);
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
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(NULL, device->name);
	indigo_release_property(EAF_BEEP_PROPERTY);
	indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------
#define MAX_DEVICES 8
static int device_number = 1;
static dsd_private_data *private_data[MAX_DEVICES] = {NULL};
static indigo_device *focuser[MAX_DEVICES] = {NULL};

indigo_result indigo_focuser_dsd(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_DSD_NAME,
		focuser_attach,
		eaf_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Deep Sky Dad Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		/* figure out the number of devices to expose */
		if (getenv("FOCUSER_DSD_DEVICE_NUMBER") != NULL) {
			device_number = atoi(getenv("FOCUSER_DSD_DEVICE_NUMBER"));
			if (device_number < 1) device_number = 1;
			if (device_number > MAX_DEVICES) device_number = MAX_DEVICES;
		}


		for (int index = 0; index < device_number; index++) {
			private_data[index] = malloc(sizeof(dsd_private_data));
			assert(private_data[index] != NULL);
			memset(private_data[index], 0, sizeof(dsd_private_data));
			private_data[index]->handle = -1;
			focuser[index] = malloc(sizeof(indigo_device));
			assert(focuser[index] != NULL);
			memcpy(focuser[index], &focuser_template, sizeof(indigo_device));
			focuser[index]->private_data = private_data[index];
			sprintf(focuser[index]->name, "%s #%d", FOCUSER_DSD_NAME, index);
			indigo_attach_device(focuser[index]);
		}
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		for (int index = 0; index < device_number; index++) {
			if (focuser[index] != NULL) {
				indigo_detach_device(focuser[index]);
				free(focuser[index]);
				focuser[index] = NULL;
			}
			if (private_data[index] != NULL) {
				free(private_data[index]);
				private_data[index] = NULL;
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
