// Copyright (c) 2017 CloudMakers, s. r. o.
// All rights reserved.
//
// Code is partially based on Temma driver created by Kok Chen.
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

/** INDIGO Takahashi Temma driver
 \file indigo_mount_temma.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME	"indigo_mount_temma"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_novas.h>

#include "indigo_mount_temma.h"

#define PRIVATE_DATA        ((temma_private_data *)device->private_data)

#define CCD_ADVANCED_GROUP         "Advanced"

#define CORRECTION_SPEED_PROPERTY			(PRIVATE_DATA->correction_speed_property)
#define CORRECTION_SPEED_RA_ITEM          (CORRECTION_SPEED_PROPERTY->items+0)
#define CORRECTION_SPEED_DEC_ITEM         (CORRECTION_SPEED_PROPERTY->items+1)

#define CORRECTION_SPEED_PROPERTY_NAME	  "TEMMA_CORRECTION_SPEED"
#define CORRECTION_SPEED_RA_ITEM_NAME     "RA"
#define CORRECTION_SPEED_DEC_ITEM_NAME    "DEC"

#define HIGH_SPEED_PROPERTY                (PRIVATE_DATA->high_speed_property)
#define HIGH_SPEED_LOW_ITEM                (HIGH_SPEED_PROPERTY->items+0)
#define HIGH_SPEED_HIGH_ITEM               (HIGH_SPEED_PROPERTY->items+1)
#define HIGH_SPEED_PROPERTY_NAME           "TEMMA_HIGH_SPEED"
#define HIGH_SPEED_LOW_ITEM_NAME           "LOW"
#define HIGH_SPEED_HIGH_ITEM_NAME          "HIGH"

#define ZENITH_PROPERTY                   (PRIVATE_DATA->zenith_property)
#define ZENITH_EAST_ITEM                  (ZENITH_PROPERTY->items+0)
#define ZENITH_WEST_ITEM                  (ZENITH_PROPERTY->items+1)
#define ZENITH_PROPERTY_NAME              "TEMMA_ZENITH"
#define ZENITH_EAST_ITEM_NAME             "EAST"
#define ZENITH_WEST_ITEM_NAME             "WEST"

#define TEMMA_GET_VERSION						"v"
#define TEMMA_GET_POSITION					"E"
#define TEMMA_GET_GOTO_STATE				"s"
#define TEMMA_GET_CORRECTION_SPEED	"lg"
#define TEMMA_SET_VOLTAGE_12V_OR_LOW_SPEED		"v1"
#define TEMMA_SET_VOLTAGE_24V_OR_HIGH_SPEED		"v2"
#define TEMMA_SET_STELLAR_RATE			"LL"
#define TEMMA_SET_SOLAR_RATE				"LK"
#define TEMMA_GOTO_STOP							"PS"

#define TEMMA_SLEW_SLOW_EAST				"MB"
#define TEMMA_SLEW_SLOW_WEST				"MD"
#define TEMMA_SLEW_SLOW_NORTH				"MH"
#define TEMMA_SLEW_SLOW_SOUTH				"MP"
#define TEMMA_SLEW_FAST_EAST				"MC"
#define TEMMA_SLEW_FAST_WEST				"ME"
#define TEMMA_SLEW_FAST_NORTH				"MI"
#define TEMMA_SLEW_FAST_SOUTH				"MQ"
#define TEMMA_SLEW_STOP							"MA"

#define TEMMA_SWITCH_SIDE_OF_MOUNT  "PT"

#define TEMMA_MOTOR_ON							"STN-OFF"
#define TEMMA_MOTOR_OFF							"STN-ON"
#define TEMMA_ZENITH                "Z"

typedef struct {
	int handle;
	int device_count;
	double currentRA;
	double currentDec;
	char telescopeSide;
	bool isBusy, startTracking, stopTracking;
	char slewCommand[3];
	indigo_timer *slew_timer;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char product[128];
	indigo_property *correction_speed_property;
	indigo_property *high_speed_property;
	indigo_property *zenith_property;
} temma_private_data;

static bool temma_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle >= 0) {
		struct termios options;
		memset(&options, 0, sizeof options);
		if (tcgetattr(PRIVATE_DATA->handle, &options) != 0) {
			close(PRIVATE_DATA->handle);
			return false;
		}
		cfsetispeed(&options,B9600);
		cfsetospeed(&options,B9600);
		options.c_cflag |= ( CS8 | PARENB | CRTSCTS );
		options.c_cflag &= ( ~PARODD & ~CSTOPB );
		cfsetispeed( &options, B19200 ) ;
		cfsetospeed( &options, B19200 ) ;
		options.c_iflag = IGNBRK;
		options.c_cc[VMIN] = 1;
		options.c_cc[VTIME] = 5;
		options.c_lflag = options.c_oflag = 0;
		if (tcsetattr(PRIVATE_DATA->handle ,TCSANOW, &options) != 0) {
			close(PRIVATE_DATA->handle);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
			return false;
		}
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static bool temma_command(indigo_device *device, char *command, bool wait) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
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
	indigo_write(PRIVATE_DATA->handle, "\r\n", 2);
	// read response
	if (wait) {
		char buffer[128];
		int index = 0;
		int max = sizeof(buffer) - 1;
		while (index < max) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = 0;
			tv.tv_usec = 300000;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "select failed from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			if (c == '\r')
				continue;
			if (c == '\n')
				break;
			buffer[index++] = c;
		}
		buffer[index] = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "command '%s' -> '%s'", command, buffer);
		switch (buffer[0]) {
			case 'E': {
				int d, m, s;
				sscanf(buffer + 1, "%02d%02d%02d", &d, &m, &s);
				PRIVATE_DATA->currentRA = d + m / 60.0 + s / 3600.0;
				sscanf(buffer + 8, "%02d%02d%01d", &d, &m, &s);
				if(buffer[7] == '-')
					PRIVATE_DATA->currentDec = -(d + m / 60.0 + s / 600.0);
				else
					PRIVATE_DATA->currentDec = d + m / 60.0 + s / 600.0;
				if ( buffer[13] == 'E' || buffer[13] == 'W' ) {
					// telescope side
					bool changed = PRIVATE_DATA->telescopeSide == (buffer[13] == 'E' ? 'W' : 'E');
					PRIVATE_DATA->telescopeSide = buffer[13];
					MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = PRIVATE_DATA->telescopeSide == 'W';
					MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = PRIVATE_DATA->telescopeSide == 'E';
					if (changed) {
						indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
					}
				}
				else if ( buffer[13] == 'F' ) {
					// fulfilled
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Coords %c %g %g", PRIVATE_DATA->telescopeSide, PRIVATE_DATA->currentRA, PRIVATE_DATA->currentDec);
				break;
			}
			case 'v': {
				switch (buffer[1]) {
					case 'e':
						indigo_copy_value(MOUNT_INFO_VENDOR_ITEM->text.value, "Takahashi");
						indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, buffer + 4);
						indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
						break;
					case '1':
					case '2':
						HIGH_SPEED_LOW_ITEM->sw.value = buffer[1] == '1';
						HIGH_SPEED_HIGH_ITEM->sw.value = buffer[1] == '2';
						break;
				};
				break;
			}
			case 's': {
				PRIVATE_DATA->isBusy = buffer[1] == '1';
				break;
			}
			case 'l': {
				if (buffer[1] == 'a')
					CORRECTION_SPEED_RA_ITEM->number.value = atoi(buffer + 3);
				else if (buffer[1] == 'b')
					CORRECTION_SPEED_DEC_ITEM->number.value = atoi(buffer + 3);
				else if (buffer[1] == 'g') {
					buffer[4] = 0;
					CORRECTION_SPEED_RA_ITEM->number.value = atoi(buffer + 2);
					buffer[7] = 0;
					CORRECTION_SPEED_DEC_ITEM->number.value = atoi(buffer + 5);
				}
				break;
			}
			default:
				break;
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "command '%s'", command);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}

static void temma_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void temma_set_lst(indigo_device *device) {
	char buffer[128];
	time_t utc = indigo_get_mount_utc(device);
	double lst = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	sprintf(buffer, "T%.2d%.2d%.2d", (int)lst, ((int)(lst * 60)) % 60, ((int)(lst * 3600)) % 60);
	temma_command(device, buffer, false);
}

static void temma_set_latitude(indigo_device *device) {
	char buffer[128];
	double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
	double l = fabs(lat);
	int d = (int)l;
	l = (l - d) * 60;
	int m = (int)l;
	l = (l - m) * 6;
	int s = (int)l;
	if (lat > 0)
		sprintf(buffer, "I+%.2d%.2d%.1d", d, m, s);
	else
		sprintf(buffer, "I-%.2d%.2d%.1d", d, m, s);
	temma_command(device, buffer, false);
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		temma_command(device, TEMMA_GET_POSITION, true);
		temma_command(device, TEMMA_GET_GOTO_STATE, true);
		if (PRIVATE_DATA->isBusy) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			if (PRIVATE_DATA->startTracking) {
				temma_command(device, TEMMA_MOTOR_ON, true);
				PRIVATE_DATA->startTracking = false;
			}
			if (PRIVATE_DATA->stopTracking) {
				temma_command(device, TEMMA_MOTOR_OFF, true);
				PRIVATE_DATA->stopTracking = false;
			}
		}
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
		indigo_update_coordinates(device, NULL);
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
}

static void slew_timer_callback(indigo_device *device) {
	if (*PRIVATE_DATA->slewCommand) {
		temma_command(device, PRIVATE_DATA->slewCommand, false);
		indigo_reschedule_timer(device, 0.25, &PRIVATE_DATA->slew_timer);
	}
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		*PRIVATE_DATA->slewCommand = 0;
		SIMULATION_PROPERTY->hidden = true;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;

		// CORRECTION_SPEED
		CORRECTION_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, CORRECTION_SPEED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Correction speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (CORRECTION_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(CORRECTION_SPEED_RA_ITEM, CORRECTION_SPEED_RA_ITEM_NAME, "RA speed (10% - 90%)", 10, 90, 1, 50);
		indigo_init_number_item(CORRECTION_SPEED_DEC_ITEM, CORRECTION_SPEED_DEC_ITEM_NAME, "Dec speed (10% - 90%)", 10, 90, 1, 50);
		// HIGH_SPEED
		HIGH_SPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, HIGH_SPEED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "High-speed or High-voltage config", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (HIGH_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(HIGH_SPEED_LOW_ITEM, HIGH_SPEED_LOW_ITEM_NAME, "12V or Low-speed", true);
		indigo_init_switch_item(HIGH_SPEED_HIGH_ITEM, HIGH_SPEED_HIGH_ITEM_NAME, "24V or High-speed", false);
		// ZENITH
		ZENITH_PROPERTY = indigo_init_switch_property(NULL, device->name, ZENITH_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Sync zenith", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (ZENITH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(ZENITH_EAST_ITEM, ZENITH_EAST_ITEM_NAME, "East zenith", false);
		indigo_init_switch_item(ZENITH_WEST_ITEM, ZENITH_WEST_ITEM_NAME, "West zenith", false);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;

		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(CORRECTION_SPEED_PROPERTY, property))
			indigo_define_property(device, CORRECTION_SPEED_PROPERTY, NULL);
		if (indigo_property_match(HIGH_SPEED_PROPERTY, property))
			indigo_define_property(device, HIGH_SPEED_PROPERTY, NULL);
		if (indigo_property_match(ZENITH_PROPERTY, property))
			indigo_define_property(device, ZENITH_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = temma_open(device);
		}
		if (result) {
			int repeat = 5;
			while (repeat-- > 0)
				if ((result = temma_command(device, TEMMA_GET_VERSION, true)))
					break;
			if (result) {
				temma_set_lst(device);
				temma_set_latitude(device);
				// TemmaPC set to 24V when TEMMA_GET_VERSION (`v`) command sent.
				temma_command(device, TEMMA_SET_VOLTAGE_12V_OR_LOW_SPEED, false);
				temma_command(device, TEMMA_GET_POSITION, true);
				temma_command(device, TEMMA_GET_CORRECTION_SPEED, true);
				temma_command(device, TEMMA_GET_GOTO_STATE, true);
				indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
				indigo_define_property(device, CORRECTION_SPEED_PROPERTY, NULL);
				indigo_define_property(device, HIGH_SPEED_PROPERTY, NULL);
				indigo_define_property(device, ZENITH_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get version, not temma mount?");
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open serial port");
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		PRIVATE_DATA->position_timer = NULL;
		indigo_delete_property(device, CORRECTION_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, HIGH_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, ZENITH_PROPERTY, NULL);
		if (--PRIVATE_DATA->device_count == 0) {
			temma_command(device, TEMMA_SLEW_STOP, false);
			temma_command(device, TEMMA_GOTO_STOP, false);
			temma_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, mount_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			char buffer[128];
			time_t utc = indigo_get_mount_utc(device);
			int ra = (indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_PARK_POSITION_HA_ITEM->number.value) * 3600;
			if (ra < 0)
				ra += 24 * 3600;
			int ra_h = ra / 3600;
			int ra_m = (ra / 60) % 60;
			int ra_s = ra % 60;
			int dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value * 600;
			int dec_d = dec / 600;
			int dec_m = abs((dec / 10) % 60);
			int dec_s = abs(dec % 10);
			temma_set_lst(device);
			sprintf(buffer, "P%02d%02d%02d%+03d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
			temma_command(device, buffer, true);
			temma_command(device, TEMMA_MOTOR_OFF, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			temma_set_latitude(device);
			temma_set_lst(device);
			temma_command(device, TEMMA_GET_POSITION, true);
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		char buffer[128];
		int ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value * 3600;
		int ra_h = ra / 3600;
		int ra_m = (ra / 60) % 60;
		int ra_s = ra % 60;
		int dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value * 600;
		int dec_d = dec / 600;
		int dec_m = abs((dec / 10) % 60);
		int dec_s = abs(dec % 10);
		temma_command(device, TEMMA_MOTOR_ON, true);
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			sprintf(buffer, "D%02d%02d%02d%+03d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
			temma_set_lst(device);
			temma_command(device, "Z", false);
			temma_set_lst(device);
			temma_command(device, buffer, true);
			PRIVATE_DATA->startTracking = true;
		} else if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			sprintf(buffer, "P%02d%02d%02d%+03d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
			temma_set_lst(device);
			temma_command(device, buffer, true);
			PRIVATE_DATA->startTracking = true;
		} else {
			sprintf(buffer, "P%02d%02d%02d%+03d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
			temma_set_lst(device);
			temma_command(device, buffer, true);
			PRIVATE_DATA->stopTracking = true;
		}
		indigo_update_coordinates(device, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
			temma_command(device, TEMMA_SLEW_STOP, false);
			temma_command(device, TEMMA_GOTO_STOP, false);
			for (int i = 0; i < 16; i++) {
				indigo_usleep(250000);
				temma_command(device, TEMMA_GET_GOTO_STATE, true);
				if (!PRIVATE_DATA->isBusy)
					break;
				temma_command(device, TEMMA_GOTO_STOP, false);
			}
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
			if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				temma_command(device, TEMMA_SET_SOLAR_RATE, false);
			} else if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				temma_command(device, TEMMA_SET_STELLAR_RATE, false);
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
				temma_command(device, TEMMA_SET_STELLAR_RATE, false);
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			}
			temma_command(device, TEMMA_MOTOR_ON, true);
			indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		if (MOUNT_TRACKING_ON_ITEM->sw.value) {
			temma_command(device, TEMMA_MOTOR_ON, true);
		} else {
			temma_command(device, TEMMA_MOTOR_OFF, true);
		}
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_SLOW_NORTH);
			else
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_FAST_NORTH);
			if (PRIVATE_DATA->slew_timer)
				indigo_reschedule_timer(device, 0.0, &PRIVATE_DATA->slew_timer);
			else
				indigo_set_timer(device, 0.0, slew_timer_callback, &PRIVATE_DATA->slew_timer);
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_SLOW_SOUTH);
			else
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_FAST_SOUTH);
			if (PRIVATE_DATA->slew_timer)
				indigo_reschedule_timer(device, 0.0, &PRIVATE_DATA->slew_timer);
			else
				indigo_set_timer(device, 0.0, slew_timer_callback, &PRIVATE_DATA->slew_timer);
		} else {
			*PRIVATE_DATA->slewCommand = 0;
			indigo_cancel_timer(device, &PRIVATE_DATA->slew_timer);
			temma_command(device, TEMMA_SLEW_STOP, false);
		}
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		if (MOUNT_MOTION_WEST_ITEM->sw.value) {
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_SLOW_WEST);
			else
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_FAST_WEST);
			if (PRIVATE_DATA->slew_timer)
				indigo_reschedule_timer(device, 0.0, &PRIVATE_DATA->slew_timer);
			else
				indigo_set_timer(device, 0.0, slew_timer_callback, &PRIVATE_DATA->slew_timer);
		} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_SLOW_EAST);
			else
				strcpy(PRIVATE_DATA->slewCommand, TEMMA_SLEW_FAST_EAST);
			if (PRIVATE_DATA->slew_timer)
				indigo_reschedule_timer(device, 0.0, &PRIVATE_DATA->slew_timer);
			else
				indigo_set_timer(device, 0.0, slew_timer_callback, &PRIVATE_DATA->slew_timer);
		} else {
			*PRIVATE_DATA->slewCommand = 0;
			indigo_cancel_timer(device, &PRIVATE_DATA->slew_timer);
			temma_command(device, TEMMA_SLEW_STOP, false);
		}
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SIDE_OF_PIER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SIDE_OF_PIER
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SIDE_OF_PIER_PROPERTY, property, false);
			if (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
				if (PRIVATE_DATA->telescopeSide == 'E') {
					temma_command(device, TEMMA_SWITCH_SIDE_OF_MOUNT, true);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Side of telescope switched : West -> East");
				}
			} else if (MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
				if (PRIVATE_DATA->telescopeSide == 'W') {
					temma_command(device, TEMMA_SWITCH_SIDE_OF_MOUNT, true);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Side of telescope switched : East -> West");
				}
			}
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CORRECTION_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CORRECTION_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(CORRECTION_SPEED_PROPERTY, property, false);
			char buffer[128];
			sprintf(buffer, "LA%02d", (int)CORRECTION_SPEED_RA_ITEM->number.value);
			temma_command(device, buffer, false);
			sprintf(buffer, "LB%02d", (int)CORRECTION_SPEED_DEC_ITEM->number.value);
			temma_command(device, buffer, false);
			CORRECTION_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CORRECTION_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(HIGH_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HIGH_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(HIGH_SPEED_PROPERTY, property, false);
			if (HIGH_SPEED_LOW_ITEM->sw.value || HIGH_SPEED_HIGH_ITEM->sw.value) {
				// show busy
				HIGH_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, HIGH_SPEED_PROPERTY, NULL);
				if (HIGH_SPEED_LOW_ITEM->sw.value) {
					temma_command(device, TEMMA_SET_VOLTAGE_12V_OR_LOW_SPEED, false);
				} else if (HIGH_SPEED_HIGH_ITEM->sw.value) {
					temma_command(device, TEMMA_SET_VOLTAGE_24V_OR_HIGH_SPEED, false);
				}
				// show ok
				HIGH_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, HIGH_SPEED_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(ZENITH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ZENITH
		if (IS_CONNECTED) {
			indigo_property_copy_values(ZENITH_PROPERTY, property, false);
			if (ZENITH_EAST_ITEM->sw.value || ZENITH_WEST_ITEM->sw.value) {
				// show busy
				ZENITH_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, ZENITH_PROPERTY, NULL);
				// check side of pier
				if ((ZENITH_EAST_ITEM->sw.value && PRIVATE_DATA->telescopeSide == 'W') ||
				    (ZENITH_WEST_ITEM->sw.value && PRIVATE_DATA->telescopeSide == 'E')) {
					// update side of pier
					temma_command(device, TEMMA_SWITCH_SIDE_OF_MOUNT, false);
				}
				// send zenith
				temma_set_lst(device);
				temma_command(device, TEMMA_ZENITH, false);
				ZENITH_EAST_ITEM->sw.value = false;
				ZENITH_WEST_ITEM->sw.value = false;
				// show ok
				ZENITH_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, ZENITH_PROPERTY, NULL);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CORRECTION_SPEED_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connect_callback(device);
	}
	indigo_release_property(CORRECTION_SPEED_PROPERTY);
	indigo_release_property(HIGH_SPEED_PROPERTY);
	indigo_release_property(ZENITH_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = temma_open(device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			temma_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			temma_command(device, TEMMA_SLEW_SLOW_NORTH, false);
			indigo_usleep(1000 * GUIDER_GUIDE_NORTH_ITEM->number.value);
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			temma_command(device, TEMMA_SLEW_SLOW_SOUTH, false);
			indigo_usleep(1000 * GUIDER_GUIDE_SOUTH_ITEM->number.value);
		}
		temma_command(device, TEMMA_SLEW_STOP, false);
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			temma_command(device, TEMMA_SLEW_SLOW_WEST, false);
			indigo_usleep(1000 * GUIDER_GUIDE_WEST_ITEM->number.value);
		} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			temma_command(device, TEMMA_SLEW_SLOW_EAST, false);
			indigo_usleep(1000 * GUIDER_GUIDE_EAST_ITEM->number.value);
		}
		temma_command(device, TEMMA_SLEW_STOP, false);
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static temma_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_temma(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_TEMMA_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_TEMMA_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Takahashi Temma Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(temma_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			indigo_attach_device(mount_guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (mount_guider != NULL) {
				indigo_detach_device(mount_guider);
				free(mount_guider);
				mount_guider = NULL;
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
