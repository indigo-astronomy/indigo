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

#define DRIVER_VERSION 0x0001
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

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_novas.h"
#include "indigo_mount_temma.h"

#define PRIVATE_DATA        ((temma_private_data *)device->private_data)

#define TEMMA_GET_VERSION						"v", true
#define TEMMA_GET_POSITION					"E", true
#define TEMMA_GET_GOTO_STATE				"s", true
#define TEMMA_GET_CORRECTION_SPEED	"lg", true
#define TEMMA_SET_VOLTAGE_12V				"v1", false
#define TEMMA_SET_VOLTAGE_24V				"v2", false
#define TEMMA_SET_STELLAR_RATE			"LL", false
#define TEMMA_SET_SOLAR_RATE				"LK", false
#define TEMMA_GOTO_STOP							"PS", false

#define TEMMA_SLEW_SLOW_EAST				"MB", false
#define TEMMA_SLEW_SLOW_WEST				"MD", false
#define TEMMA_SLEW_SLOW_NORTH				"MH", false
#define TEMMA_SLEW_SLOW_SOUTH				"MP", false
#define TEMMA_SLEW_FAST_EAST				"MC", false
#define TEMMA_SLEW_FAST_WEST				"ME", false
#define TEMMA_SLEW_FAST_NORTH				"MI", false
#define TEMMA_SLEW_FAST_SOUTH				"MQ", false
#define TEMMA_SLEW_STOP							"MA", false

#define TEMMA_TRACKING_ON						"STN-OFF", true
#define TEMMA_TRACKING_OFF					"STN-ON", true

typedef struct {
	bool parked;
	int handle;
	int device_count;
	double currentRA;
	double currentDec;
	char pierSide;
	bool isBusy;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char product[128];
} temma_private_data;

static bool temma_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	
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
	if (tcsetattr(PRIVATE_DATA->handle,TCSANOW, &options) != 0) {
		close(PRIVATE_DATA->handle);
		return false;
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
		return false;
	}
}

static bool temma_command(indigo_device *device, char *command, bool wait) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	indigo_write(PRIVATE_DATA->handle, "\r\n", 2);
	// read response
	if (wait) {
		char buffer[128];
		int remains = sizeof(buffer) - 1;
		int index = 0;
		int timeout = 3;
		while (remains > 0) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = 0;
			tv.tv_usec = 300000;
			timeout = 0;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
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
				PRIVATE_DATA->pierSide = buffer[13];
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Coords %c %g %g", PRIVATE_DATA->pierSide, PRIVATE_DATA->currentRA, PRIVATE_DATA->currentDec);
				break;
			}
			case 'v': {
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Takahashi");
				strncpy(MOUNT_INFO_MODEL_ITEM->text.value, buffer + 4, INDIGO_VALUE_SIZE);
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				break;
			}
			case 's': {
				PRIVATE_DATA->isBusy = buffer[0] == '1';
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
		INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void temma_set_lst(indigo_device *device) {
	char buffer[128];
	double lst = indigo_lst(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	sprintf(buffer, "T%.2d%.2d%.2d\r\n", (int)lst, ((int)(lst * 60)) % 60, ((int)(lst * 3600)) % 60);
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
		sprintf(buffer, "I+%.2d%.2d%.1d\r\n", d, m, s);
	else
		sprintf(buffer, "I-%.2d%.2d%.1d\r\n", d, m, s);
	temma_command(device, buffer, false);
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0 && !PRIVATE_DATA->parked) {
		temma_command(device, TEMMA_GET_POSITION);
		temma_command(device, TEMMA_GET_GOTO_STATE);
		if (PRIVATE_DATA->isBusy) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			if (MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value)
				temma_command(device, TEMMA_TRACKING_ON);
			else
				temma_command(device, TEMMA_TRACKING_OFF);
		}
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		SIMULATION_PROPERTY->hidden = true;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		MOUNT_TRACKING_PROPERTY->hidden = true;
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		// TBD
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = temma_open(device);
			}
			if (result) {
				int repeat = 5;
				while (repeat-- > 0)
					if ((result = temma_command(device, TEMMA_GET_VERSION)))
						break;
				if (result) {
					temma_set_lst(device);
					temma_set_latitude(device);
					temma_command(device, TEMMA_SET_VOLTAGE_12V);
					temma_command(device, TEMMA_GET_POSITION);
					temma_command(device, TEMMA_GET_CORRECTION_SPEED);
					temma_command(device, TEMMA_GET_GOTO_STATE);
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to get version, not temma mount?");
					PRIVATE_DATA->device_count--;
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to open serial port");
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			PRIVATE_DATA->position_timer = NULL;
			if (--PRIVATE_DATA->device_count == 0) {
				temma_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (!PRIVATE_DATA->parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
			char buffer[128];
			int ra = indigo_lst(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) * 3600;
			int ra_h = ra / 3600;
			int ra_m = (ra / 60) % 60;
			int ra_s = ra % 60;
			temma_set_lst(device);
			sprintf(buffer, "P%02d%02d%02d%+90000", ra_h, ra_m, ra_s);
			temma_command(device, buffer, true);
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		temma_set_latitude(device);
		temma_set_lst(device);
		temma_command(device, TEMMA_GET_POSITION);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (PRIVATE_DATA->parked) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			char buffer[128];
			int ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value * 3600;
			int ra_h = ra / 3600;
			int ra_m = (ra / 60) % 60;
			int ra_s = ra % 60;
			int dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value * 600;
			int dec_d = dec / 600;
			int dec_m = (dec / 10) % 60;
			int dec_s = dec % 10;
			if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				sprintf(buffer, "D%02d%02d%02d%+02d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
				temma_set_lst(device);
				temma_command(device, "Z", false);
			} else {
				sprintf(buffer, "P%02d%02d%02d%+02d%02d%d", ra_h, ra_m, ra_s, dec_d, dec_m, dec_s);
			}
			temma_set_lst(device);
			temma_command(device, buffer, true);
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (PRIVATE_DATA->parked) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mout is parked!");
		} else {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
				temma_command(device, TEMMA_SLEW_STOP);
				temma_command(device, TEMMA_GOTO_STOP);
				for (int i = 0; i < 16; i++) {
					usleep(250000);
					temma_command(device, TEMMA_GET_GOTO_STATE);
					if (!PRIVATE_DATA->isBusy)
						break;
					temma_command(device, TEMMA_GOTO_STOP);
				}
				indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
			temma_command(device, TEMMA_SET_SOLAR_RATE);
		} else if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
			temma_command(device, TEMMA_SET_STELLAR_RATE);
		} else {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			temma_command(device, TEMMA_SET_STELLAR_RATE);
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		}
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mout is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
					temma_command(device, TEMMA_SLEW_SLOW_NORTH);
				else
					temma_command(device, TEMMA_SLEW_FAST_NORTH);
			} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
					temma_command(device, TEMMA_SLEW_SLOW_SOUTH);
				else
					temma_command(device, TEMMA_SLEW_FAST_SOUTH);
			} else {
				temma_command(device, TEMMA_SLEW_STOP);
			}
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mout is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			if (MOUNT_MOTION_WEST_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
					temma_command(device, TEMMA_SLEW_SLOW_WEST);
				else
					temma_command(device, TEMMA_SLEW_FAST_WEST);
			} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
					temma_command(device, TEMMA_SLEW_SLOW_EAST);
				else
					temma_command(device, TEMMA_SLEW_FAST_EAST);
			} else {
				temma_command(device, TEMMA_SLEW_STOP);
			}
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
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
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			// TBD
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			// TBD
		}
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			// TBD
		} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			// TBD
		}
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
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static temma_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_temma(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = {
		MOUNT_TEMMA_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	};
	static indigo_device mount_guider_template = {
		MOUNT_TEMMA_GUIDER_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	};
	
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, "Takahashi Temma Mount", __FUNCTION__, DRIVER_VERSION, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(temma_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(temma_private_data));
			mount = malloc(sizeof(indigo_device));
			assert(mount != NULL);
			memcpy(mount, &mount_template, sizeof(indigo_device));
			mount->private_data = private_data;
			indigo_attach_device(mount);
			mount_guider = malloc(sizeof(indigo_device));
			assert(mount_guider != NULL);
			memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
			mount_guider->private_data = private_data;
			indigo_attach_device(mount_guider);
			break;
			
		case INDIGO_DRIVER_SHUTDOWN:
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

