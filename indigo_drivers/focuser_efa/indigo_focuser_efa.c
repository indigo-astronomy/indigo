// Copyright (c) 2019 CloudMakers, s. r. o.
// All rights reserved.
//
// EFA focuser command set is extracted from INDI driver written
// by Phil Shepherd with help of Peter Chance.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Celestron / PlaneWave EFA focuser driver
 \file indigo_focuser_efa.c
 */


#define DRIVER_VERSION 0x0011
#define DRIVER_NAME "indigo_focuser_efa"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_efa.h"

#define SOM 0x3B
#define APP	0x20
#define FOC	0x12
#define FAN 0x13

#define PRIVATE_DATA													((efa_private_data *)device->private_data)

#define X_FOCUSER_FANS_PROPERTY								(PRIVATE_DATA->fans_property)
#define X_FOCUSER_FANS_OFF_ITEM								(X_FOCUSER_FANS_PROPERTY->items+0)
#define X_FOCUSER_FANS_ON_ITEM								(X_FOCUSER_FANS_PROPERTY->items+1)

#define X_FOCUSER_CALIBRATION_PROPERTY				(PRIVATE_DATA->calibration_property)
#define X_FOCUSER_CALIBRATION_ITEM						(X_FOCUSER_CALIBRATION_PROPERTY->items+0)


typedef struct {
	int handle;
	indigo_timer *timer;
	pthread_mutex_t mutex;
	pthread_mutex_t serial_mutex;
	indigo_property *fans_property;
	indigo_property *calibration_property;
	bool is_celestron;
	bool is_efa;
} efa_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool efa_command(indigo_device *device, uint8_t *packet_out, uint8_t *packet_in) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int bits = 0;
	int delay = 0;
	int result = 0;
	if (PRIVATE_DATA->is_efa) {
		for (int i = 0; i < 10; i++) {
			bits = 0;
			result = ioctl(PRIVATE_DATA->handle, TIOCMGET, &bits);
			if ((bits & TIOCM_CTS) == 0 || result < 0)
				break;
			delay = delay == 0 ? 1 : delay * 2;
			indigo_usleep(i * 10000);
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → CTS %s (%dms, %04x)", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : ((bits & TIOCM_CTS) ? "not cleared" : "cleared"), delay * 10, bits);
		bits = TIOCM_RTS;
		result = ioctl(PRIVATE_DATA->handle, TIOCMBIS, &bits);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← RTS %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "set");
	}
	int count = packet_out[1], sum = 0;
	if (count == 3) {
		count = packet_out[1] = 4;
		packet_out[5] = 0;
	}
	for (int i = 0; i <= count; i++)
		sum += packet_out[i + 1];
	packet_out[count + 2] = (-sum) & 0xFF;
	if (count == 3)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← %02X %02X %02X→%02X [%02X] %02X", PRIVATE_DATA->handle, packet_out[0], packet_out[1], packet_out[2], packet_out[3], packet_out[4], packet_out[5]);
	else if (count == 4)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← %02X %02X %02X→%02X [%02X %02X] %02X", PRIVATE_DATA->handle, packet_out[0], packet_out[1], packet_out[2], packet_out[3], packet_out[4], packet_out[5], packet_out[6]);
	else if (count == 5)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← %02X %02X %02X→%02X [%02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_out[0], packet_out[1], packet_out[2], packet_out[3], packet_out[4], packet_out[5], packet_out[6], packet_out[7]);
	else if (count == 6)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← %02X %02X %02X→%02X [%02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_out[0], packet_out[1], packet_out[2], packet_out[3], packet_out[4], packet_out[5], packet_out[6], packet_out[7], packet_out[8]);
	else if (count == 7)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← %02X %02X %02X→%02X [%02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_out[0], packet_out[1], packet_out[2], packet_out[3], packet_out[4], packet_out[5], packet_out[6], packet_out[7], packet_out[8], packet_out[8]);
	if (indigo_write(PRIVATE_DATA->handle, (const char *)packet_out, count + 3)) {
		for (int i = 0; i < 10; i++) {
			long result = read(PRIVATE_DATA->handle, packet_in, 1);
			if (result <= 0) {
				if (result == 0 || errno == EAGAIN)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → TIMEOUT", PRIVATE_DATA->handle);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → ERROR (%s)", PRIVATE_DATA->handle, strerror(errno));
				break;
			}
			if (packet_in[0] != 0x3B) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X, skipped", packet_in[0]);
				continue;
			}
			result = read(PRIVATE_DATA->handle, packet_in + 1, 1);
			if (result <= 0) {
				if (result == 0 || errno == EAGAIN)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → TIMEOUT", PRIVATE_DATA->handle);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → ERROR (%s)", PRIVATE_DATA->handle, strerror(errno));
				break;
			}
			count = packet_in[1];
			result = indigo_read(PRIVATE_DATA->handle, (char *)packet_in + 2, count + 1);
			if (result > 0) {
				if (count == 3)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5]);
				else if (count == 4)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6]);
				else if (count == 5)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7]);
				else if (count == 6)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8]);
				else if (count == 7)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8], packet_in[9]);
				else if (count == 8)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8], packet_in[9], packet_in[10]);
				else if (count == 9)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8], packet_in[9], packet_in[10], packet_in[11]);
				else if (count == 10)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8], packet_in[9], packet_in[10], packet_in[11], packet_in[12]);
				else if (count == 11)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → %02X %02X %02X→%02X [%02X %02X %02X %02X %02X %02X %02X %02X %02X] %02X", PRIVATE_DATA->handle, packet_in[0], packet_in[1], packet_in[2], packet_in[3], packet_in[4], packet_in[5], packet_in[6], packet_in[7], packet_in[8], packet_in[9], packet_in[10], packet_in[11], packet_in[12], packet_in[13]);
				if (memcmp(packet_in, packet_out, count + 3) == 0) {
					if (PRIVATE_DATA->is_efa) {
						result = ioctl(PRIVATE_DATA->handle, TIOCMBIC, &bits);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← RTS %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "cleared");
					}
					continue;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				return packet_in[2] == packet_out[3] && packet_in[4] == packet_out[4];
			} else {
				if (result == 0 || errno == EAGAIN)
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → TIMEOUT", PRIVATE_DATA->handle);
				else
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d → ERROR (%s)", PRIVATE_DATA->handle, strerror(errno));
				break;
			}
		}
	} else {
		if (PRIVATE_DATA->is_efa) {
			result = ioctl(PRIVATE_DATA->handle, TIOCMBIC, &bits);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← RTS %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "cleared");
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return false;
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		X_FOCUSER_FANS_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_FANS", FOCUSER_MAIN_GROUP, "Fans", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_FANS_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_FOCUSER_FANS_PROPERTY->hidden = true;
		indigo_init_switch_item(X_FOCUSER_FANS_OFF_ITEM, "OFF", "Off", true);
		indigo_init_switch_item(X_FOCUSER_FANS_ON_ITEM, "ON", "On", false);
		X_FOCUSER_CALIBRATION_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_CALIBRATION", FOCUSER_MAIN_GROUP, "Calibration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_FOCUSER_CALIBRATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		X_FOCUSER_CALIBRATION_PROPERTY->hidden = true;
		indigo_init_switch_item(X_FOCUSER_CALIBRATION_ITEM, "CALIBRATE", "Calibrate", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 3799422;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 3799422;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = FOCUSER_POSITION_ITEM->number.min = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min;
		FOCUSER_STEPS_ITEM->number.max = FOCUSER_POSITION_ITEM->number.max = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max;
		FOCUSER_STEPS_ITEM->number.step = FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_FOCUSER_FANS_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
		if (indigo_property_match(X_FOCUSER_CALIBRATION_PROPERTY, property))
			indigo_define_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uint8_t response_packet[16];
	if (PRIVATE_DATA->is_efa) {
		uint8_t get_temp_packet[16] = { SOM, 0x04, APP, FOC, 0x26, 0x00, 0 };
		if (efa_command(device, get_temp_packet, response_packet)) {
			int raw_temp = response_packet[6] << 8 | response_packet[7];
			if (raw_temp & 0x8000) {
				raw_temp = raw_temp - 0x10000;
			}
			double temp = raw_temp / 16.0;
			if (FOCUSER_TEMPERATURE_ITEM->number.value != temp) {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
		}
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#define THRESHOLD 50000

static long focuser_position(indigo_device *device) {
	uint8_t response_packet[16];
	uint8_t get_position_packet[16] = { SOM, 0x03, APP, FOC, 0x01, 0 };
	if (efa_command(device, get_position_packet, response_packet)) {
		long position = response_packet[5] << 16 | response_packet[6] << 8 | response_packet[7];
		if (position & 0x800000)
			position = -(position ^ 0xFFFFFF) - 1;
		return position;
	}
	return 0;
}

static void focuser_goto(indigo_device *device, long target) {
	uint8_t response_packet[16];
	uint8_t check_state_packet[16] = { SOM, 0x03, APP, FOC, 0x13, 0 };
	
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);

	long position = focuser_position(device);

	if (PRIVATE_DATA->is_efa) {
		uint8_t slew_positive_packet[16] = { SOM, 0x04, APP, FOC, 0x24, 0x09, 0 };
		uint8_t slew_negative_packet[16] = { SOM, 0x04, APP, FOC, 0x25, 0x09, 0 };
		uint8_t stop_packet[16] = { SOM, 0x04, APP, FOC, 0x24, 0x00, 0 };
		if (labs(target - position) > THRESHOLD) {
			if (!efa_command(device, target > position ? slew_positive_packet : slew_negative_packet, response_packet))
				goto failure;
			while (true) {
				if (efa_command(device, check_state_packet, response_packet)) {
					if (response_packet[5] != 0x00)
						goto failure;
				}
				position = focuser_position(device);
				FOCUSER_POSITION_ITEM->number.value = position;
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				if (labs(target - position) <= THRESHOLD) {
					if (!efa_command(device, stop_packet, response_packet))
						goto failure;
					break;
				}
				indigo_usleep(300000);
			}
		}
	}
	uint8_t goto_packet[16] = { SOM, 0x06, APP, FOC, PRIVATE_DATA->is_efa ? 0x17 : 0x02, (target >> 16) & 0xFF, (target >> 8) & 0xFF, target & 0xFF, 0 };
	if (!efa_command(device, goto_packet, response_packet))
		goto failure;
	while (true) {
		position = focuser_position(device);
		FOCUSER_POSITION_ITEM->number.value = position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		if (efa_command(device, check_state_packet, response_packet)) {
			if (response_packet[5] == 0xFE)
				goto failure;
			if (response_packet[5] == 0xFF)
				break;
		}
		indigo_usleep(300000);
	}
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	return;
failure:
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_calibration_handler(indigo_device *device) {
	if (X_FOCUSER_CALIBRATION_ITEM->sw.value) {
		uint8_t response_packet[16];
		uint8_t start_calibration_packet[16] = { SOM, 0x04, APP, FOC, 0x2A, 1, 0 };
		uint8_t check_calibration_packet[16] = { SOM, 0x03, APP, FOC, 0x2B, 0 };
		uint8_t get_limits_packet[16] = { SOM, 0x03, APP, FOC, 0x2C, 0 };
		X_FOCUSER_CALIBRATION_ITEM->sw.value = false;
		if (!efa_command(device, start_calibration_packet, response_packet))
			goto failure;
		while (true) {
			indigo_usleep(300000);
			FOCUSER_POSITION_ITEM->number.value = focuser_position(device);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (efa_command(device, check_calibration_packet, response_packet)) {
				if (response_packet[5] > 0) {
					if (efa_command(device, get_limits_packet, response_packet)) {
						FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = (response_packet[5] << 24) + (response_packet[6] << 16) + (response_packet[7] << 8) + response_packet[8];
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (response_packet[9] << 24) + (response_packet[10] << 16) + (response_packet[11] << 8) + response_packet[12];
						FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
					} else {
						FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
					}
					indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
					X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, "Calibrated");
					break;
				}
				if (response_packet[6] == 0) {
					X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, "Calibration failed");
					break;
				}
			}
		}
		return;
	failure:
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
	}
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uint8_t response_packet[16];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			int bits = TIOCM_RTS;
			int result = ioctl(PRIVATE_DATA->handle, TIOCMBIC, &bits);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← RTS %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "cleared");
		}
		if (PRIVATE_DATA->handle > 0) {
			PRIVATE_DATA->is_efa = true;
			PRIVATE_DATA->is_celestron = false;
			uint8_t get_version_packet[16] = { SOM, 0x03, APP, FOC, 0xFE, 0 };
			if (efa_command(device, get_version_packet, response_packet)) {
				if (response_packet[1] == 5) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "PlaneWave EFA");
					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%d.%d", response_packet[5], response_packet[6]);
					PRIVATE_DATA->is_efa = true;
					PRIVATE_DATA->is_celestron = false;
				} else if (response_packet[1] == 7) {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Celestron Focus Motor");
					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%d.%d.%d", response_packet[5], response_packet[6], (response_packet[7] << 8) + response_packet[8]);
					PRIVATE_DATA->is_efa = false;
					PRIVATE_DATA->is_celestron = true;
				} else {
					strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Uknown device");
					strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Uknown version");
				}
				INDIGO_DRIVER_LOG(DRIVER_NAME, "%s %s detected", INFO_DEVICE_MODEL_ITEM->text.value, INFO_DEVICE_FW_REVISION_ITEM->text.value);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EFA not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, "Failed to open port %s (%s)", DEVICE_PORT_ITEM->text.value, strerror(errno));
		}
		if (PRIVATE_DATA->handle > 0) {
			FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = focuser_position(device);
			if (PRIVATE_DATA->is_efa) {
				uint8_t get_calibration_status_packet[16] = { SOM, 0x03, APP, FOC, 0x30, 0 };
				if (!efa_command(device, get_calibration_status_packet, response_packet) || response_packet[5] == 0) {
					indigo_send_message(device, "Warning! Focuser is not calibrated!");
				}
				uint8_t set_stop_detect_packet[16] = { SOM, 0x04, APP, FOC, 0xEF, 0x01, 0 };
				efa_command(device, set_stop_detect_packet, response_packet);
				uint8_t get_fans_packet[16] = { SOM, 0x03, APP, FAN, 0x28, 0 };
				if (efa_command(device, get_fans_packet, response_packet)) {
					indigo_set_switch(X_FOCUSER_FANS_PROPERTY, X_FOCUSER_FANS_ON_ITEM, response_packet[5] == 0);
				}
				X_FOCUSER_FANS_PROPERTY->hidden = false;
				X_FOCUSER_CALIBRATION_PROPERTY->hidden = true;
				indigo_define_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
				FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
				FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
				FOCUSER_LIMITS_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			} else {
				uint8_t get_calibration_status_packet[16] = { SOM, 0x03, APP, FOC, 0x2B, 0 };
				if (!efa_command(device, get_calibration_status_packet, response_packet) || response_packet[5] == 0) {
					indigo_send_message(device, "Warning! Focuser is not calibrated!");
				}
				uint8_t get_limits_packet[16] = { SOM, 0x03, APP, FOC, 0x2C, 0 };
				if (efa_command(device, get_limits_packet, response_packet)) {
					FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = (response_packet[5] << 24) + (response_packet[6] << 16) + (response_packet[7] << 8) + response_packet[8];
					FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (response_packet[9] << 24) + (response_packet[10] << 16) + (response_packet[11] << 8) + response_packet[12];
				}
				X_FOCUSER_CALIBRATION_PROPERTY->hidden = false;
				indigo_define_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
				X_FOCUSER_FANS_PROPERTY->hidden = true;
				FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
				FOCUSER_ON_POSITION_SET_PROPERTY->hidden = true;
				FOCUSER_LIMITS_PROPERTY->perm = INDIGO_RO_PERM;
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (PRIVATE_DATA->handle > 0) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			uint8_t stop_packet[16] = { SOM, 0x06, APP, FOC, 0x24, 0x00, 0 };
			efa_command(device, stop_packet, response_packet);
			indigo_delete_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
			indigo_delete_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	long position = FOCUSER_POSITION_ITEM->number.value + (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ? 1 : -1) * FOCUSER_STEPS_ITEM->number.value;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	focuser_goto(device, position);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	long position = FOCUSER_POSITION_ITEM->number.value;
	if (position < FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value;
	if (position > FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value)
		position = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value;
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		uint8_t response_packet[16];
		uint8_t sync_packet[16] = { SOM, 0x06, APP, FOC, 0x04, (position >> 16) & 0xFF, (position >> 8) & 0xFF, position & 0xFF, 0 };
		if (efa_command(device, sync_packet, response_packet))
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		else
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		focuser_goto(device, position);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_motion_handler(indigo_device *device) {
//	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		uint8_t response_packet[16];
		if (X_FOCUSER_CALIBRATION_PROPERTY->state == INDIGO_BUSY_STATE) {
			uint8_t stop_packet[16] = { SOM, 0x04, APP, FOC, 0x2A, 0, 0 };
			if (efa_command(device, stop_packet, response_packet)) {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			uint8_t stop_packet[16] = { SOM, 0x06, APP, FOC, 0x24, 0x00, 0 };
			if (efa_command(device, stop_packet, response_packet)) {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
//	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_fans_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uint8_t response_packet[16];
	uint8_t fans_packet[16] = { SOM, 0x04, APP, FAN, 0x27, X_FOCUSER_FANS_ON_ITEM->sw.value ? 0x01 : 0x00, 0 };
	if (efa_command(device, fans_packet, response_packet)) {
		X_FOCUSER_FANS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_FOCUSER_FANS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
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
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_motion_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_FOCUSER_FANS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_FANS
		indigo_property_copy_values(X_FOCUSER_FANS_PROPERTY, property, false);
		X_FOCUSER_FANS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FOCUSER_FANS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_fans_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_FOCUSER_CALIBRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_CALIBRATION
		indigo_property_copy_values(X_FOCUSER_CALIBRATION_PROPERTY, property, false);
		X_FOCUSER_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_FOCUSER_CALIBRATION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_calibration_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_FANS_PROPERTY);
	indigo_release_property(X_FOCUSER_CALIBRATION_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	pthread_mutex_destroy(&PRIVATE_DATA->serial_mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_efa(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static efa_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"EFA Focuser",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "Celestron / PlaneWave EFA Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(efa_private_data));
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
