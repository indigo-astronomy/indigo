// Copyright (c) 2016 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO RainbowAstro driver
 \file indigo_mount_rainbow.c
 */

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME	"indigo_mount_rainbow"

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
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_mount_rainbow.h"

#define PRIVATE_DATA        ((rainbow_private_data *)device->private_data)

typedef struct {
	int handle;
	indigo_timer *position_timer, *reader;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE;
	struct tm utc;
	unsigned long version;
} rainbow_private_data;

static bool rainbow_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "rainbow")) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
	} else {
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 4030, &proto);
	}
	if (PRIVATE_DATA->handle > 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static bool rainbow_command(indigo_device *device, char *command, indigo_property *property) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	bool result = indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "-> %s", command);
	if (property && !result) {
		property->state = INDIGO_ALERT_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, property, NULL);
	}
	return result;
}

static bool rainbow_sync_command(indigo_device *device, char *command, indigo_property *property) {
	property->state = INDIGO_ALERT_STATE;
	if (rainbow_command(device, command, NULL)) {
		for (int i = 0; i < 100; i++) {
			indigo_usleep(10000);
			if (property->state == INDIGO_OK_STATE) {
				if (IS_CONNECTED)
					indigo_update_property(device, property, NULL);
				return true;
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Failed to set %s", property->name);
	return false;
}

static void rainbow_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void rainbow_reader(indigo_device *device) {
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Reader started");
	char response[128], c;
	struct timeval tv;
	fd_set readout;
	FD_ZERO(&readout);
	FD_SET(PRIVATE_DATA->handle, &readout);
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	while (PRIVATE_DATA->handle > 0) {
		int i = 0;
		while (true) {
			response[i] = 0;
			if (i == sizeof(response) - 1)
				break;
			if (select(PRIVATE_DATA->handle + 1, &readout, NULL, NULL, &tv) <= 0)
				break;
			if (read(PRIVATE_DATA->handle, &c, 1) < 1)
				break;
			response[i++] = c;
			if (c == '#') {
				response[i] = 0;
				break;
			}
		}
		if (*response == 0)
			continue;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<- %s", response);
		if (!strncmp(response, ":GR", 3)) {
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = indigo_stod(response + 3);
			continue;
		}
		if (!strncmp(response, ":GD", 3)) {
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = indigo_stod(response + 3);
			continue;
		}
		if (!strcmp(response, ":CL0#")) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			continue;
		}
		if (!strcmp(response, ":CL1#")) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, NULL);
			continue;
		}
		if (!strcmp(response, ":MM0#")) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			continue;
		}
		if (!strcmp(response, ":CHO#")) {
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			continue;
		}
		if (!strncmp(response, ":CH", 3)) {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Park failed");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			continue;
		}
		if (!strncmp(response, ":GC", 3)) {
			char separator;
			sscanf(response + 3, "%d%c%d%c%d", &PRIVATE_DATA->utc.tm_mon, &separator, &PRIVATE_DATA->utc.tm_mday, &separator, &PRIVATE_DATA->utc.tm_year);
			PRIVATE_DATA->utc.tm_year += 100;
			PRIVATE_DATA->utc.tm_mon -= 1;
			continue;
		}
		if (!strncmp(response, ":GG", 3)) {
			int offset = -atoi(response + 3);
			PRIVATE_DATA->utc.tm_gmtoff =  offset * 3600;
			sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", offset);
			continue;
		}
		if (!strncmp(response, ":GL", 3)) {
			char separator;
			if (PRIVATE_DATA->version < 200625) {
				time_t secs = time(NULL);
				PRIVATE_DATA->utc = *localtime(&secs);
			}
			sscanf(response + 3, "%d%c%d%c%d", &PRIVATE_DATA->utc.tm_hour, &separator, &PRIVATE_DATA->utc.tm_min, &separator, &PRIVATE_DATA->utc.tm_sec);
			PRIVATE_DATA->utc.tm_isdst = -1;
			time_t secs = mktime(&PRIVATE_DATA->utc);
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			continue;
		}
		if (!strncmp(response, ":Gt", 3)) {
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response + 3);
			continue;
		}
		if (!strncmp(response, ":Gg", 3)) {
			double longitude = indigo_stod(response + 3);
			if (longitude < 0)
				longitude += 360;
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 360 - longitude;
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED)
				indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			continue;
		}
		if (!strncmp(response, ":AV#", 3)) {
			strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "RainbowAstro");
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
			strncpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response + 3, 6);
			PRIVATE_DATA->version = atol(response + 3);
			if (PRIVATE_DATA->version < 200625) {
				indigo_send_message(device, "Please update firmware of your mount to the version 200625 or later!");
			}
			MOUNT_INFO_PROPERTY->state = INDIGO_OK_STATE;
			continue;
		}
		if (!strcmp(response, ":AT0#")) {
			if (!MOUNT_TRACKING_OFF_ITEM->sw.value || MOUNT_TRACKING_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}
			continue;
		}
		if (!strcmp(response, ":AT1#")) {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value || MOUNT_TRACKING_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}
			continue;
		}
		if (!strcmp(response, ":CT0#")) {
			if (!MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value || MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
			continue;
		}
		if (!strcmp(response, ":CT1#")) {
			if (!MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value || MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
			continue;
		}
		if (!strcmp(response, ":CT2#")) {
			if (!MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value || MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
			continue;
		}
		if (!strcmp(response, ":CT3#")) {
			if (!MOUNT_TRACK_RATE_CUSTOM_ITEM->sw.value || MOUNT_TRACK_RATE_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
			continue;
		}
		if (!strncmp(response, ":CU0=", 5)) {
			double value = round(100 * atof(response + 5));
			if (MOUNT_GUIDE_RATE_RA_ITEM->number.value != value || MOUNT_GUIDE_RATE_PROPERTY->state != INDIGO_OK_STATE) {
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = value;
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
				if (IS_CONNECTED)
					indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Reader finished");
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementations

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		rainbow_command(device, ":GR#:GD#:CL#", MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
		rainbow_command(device, PRIVATE_DATA->version >= 200625 ? ":GC#:GG#:GL#" : ":GL#", MOUNT_UTC_TIME_PROPERTY);
		rainbow_command(device, ":AT#", MOUNT_TRACKING_PROPERTY);
		rainbow_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY);
		indigo_reschedule_timer(device, 1, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		MOUNT_GUIDE_RATE_PROPERTY->count = 1;
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY->count = 1;
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = rainbow_open(device);
		if (result) {
			indigo_set_timer(device, 0, rainbow_reader, &PRIVATE_DATA->reader);
			rainbow_sync_command(device, ":AV#", MOUNT_INFO_PROPERTY);
			rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY);
			rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY);
			rainbow_sync_command(device, ":CU0#", MOUNT_GUIDE_RATE_PROPERTY);
			rainbow_sync_command(device, ":Gt#:Gg#", MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
			rainbow_sync_command(device, ":GR#:GD#:CL#", MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
			rainbow_sync_command(device, ":AT#", MOUNT_TRACKING_PROPERTY);
			rainbow_sync_command(device, ":Ct?#", MOUNT_TRACK_RATE_PROPERTY);
			rainbow_sync_command(device, PRIVATE_DATA->version >= 200625 ? ":GC#:GG#:GL#" : ":GL#", MOUNT_UTC_TIME_PROPERTY);
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
			indigo_set_timer(device, 1, position_timer_callback, &PRIVATE_DATA->position_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		rainbow_close(device);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->reader);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_callback(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	rainbow_command(device, ":Ch#", NULL);
	indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
}

static void mount_geographic_coordinates_callback(indigo_device *device) {
	char command[128];
	if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
	double longitude = (360 - MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - 360;
	sprintf(command, ":St%s#:Sg%s#", indigo_dtos(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, "%+03d*%02d'%02d"), indigo_dtos(longitude, "%+04d*%02d'%02d"));
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	rainbow_command(device, command, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_callback(indigo_device *device) {
	char command[128];
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			rainbow_command(device, ":CtR#", MOUNT_TRACK_RATE_PROPERTY);
		} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			rainbow_command(device, ":CtS#", MOUNT_TRACK_RATE_PROPERTY);
		} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
			rainbow_command(device, ":CtM#", MOUNT_TRACK_RATE_PROPERTY);
		} else if (MOUNT_TRACK_RATE_CUSTOM_ITEM->sw.value) {
			rainbow_command(device, ":CtU#", MOUNT_TRACK_RATE_PROPERTY);
		}
		sprintf(command, ":CtA#:Sr%s#:Sd%s#:MS#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%04.1f"), indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%04.1f"));
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		sprintf(command, ":Ck%07.3f%+7.3f#", MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target * 15, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
	}
	rainbow_command(device, command, MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_callback(indigo_device *device) {
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		rainbow_command(device, ":Q#", NULL);
		MOUNT_MOTION_NORTH_ITEM->sw.value = false;
		MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		MOUNT_MOTION_WEST_ITEM->sw.value = false;
		MOUNT_MOTION_EAST_ITEM->sw.value = false;
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
	}
}

static void mount_motion_ns_callback(indigo_device *device) {
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
			rainbow_command(device, ":RG#:Mn#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
			rainbow_command(device, ":RC#:Mn#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
			rainbow_command(device, ":RM#:Mn#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
			rainbow_command(device, ":RS#:Mn#", MOUNT_SLEW_RATE_PROPERTY);
		}
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
			rainbow_command(device, ":RG#:Ms#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
			rainbow_command(device, ":RC#:Ms#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
			rainbow_command(device, ":RM#:Ms#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
			rainbow_command(device, ":RS#:Ms#", MOUNT_SLEW_RATE_PROPERTY);
		}
	} else {
		rainbow_command(device, PRIVATE_DATA->version >= 200625 ? ":Qn#:Qs#" : ":Q#", MOUNT_MOTION_DEC_PROPERTY);
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_we_callback(indigo_device *device) {
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
			rainbow_command(device, ":RG#:Mw#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
			rainbow_command(device, ":RC#:Mw#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
			rainbow_command(device, ":RM#:Mw#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
			rainbow_command(device, ":RS#:Mw#", MOUNT_SLEW_RATE_PROPERTY);
		}
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
			rainbow_command(device, ":RG#:Me#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
			rainbow_command(device, ":RC#:Me#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
			rainbow_command(device, ":RM#:Me#", MOUNT_SLEW_RATE_PROPERTY);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
			rainbow_command(device, ":RS#:Me#", MOUNT_SLEW_RATE_PROPERTY);
		}
	} else {
		rainbow_command(device, PRIVATE_DATA->version >= 200625 ? ":Qw#:Qe#" : ":Q#", MOUNT_MOTION_RA_PROPERTY);
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_set_host_time_callback(indigo_device *device) {
	char command[128];
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		time_t secs = time(NULL);
		struct tm tm = *localtime(&secs);
		sprintf(command, ":SC%02d/%02d/%02d#:SG%+03ld#:SL%02d:%02d:%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100, -(tm.tm_gmtoff / 3600), tm.tm_hour, tm.tm_min, tm.tm_sec);
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		if (rainbow_command(device, command, MOUNT_SET_HOST_TIME_PROPERTY)) {
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		}
	}
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_callback(indigo_device *device) {
	char command[128];
	time_t secs = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (secs == -1) {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
	} else {
		struct tm tm = *localtime(&secs);
		sprintf(command, ":SC%02d/%02d/%02d#:SG%+03ld#:SL%02d:%02d:%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100, -(tm.tm_gmtoff / 3600), tm.tm_hour, tm.tm_min, tm.tm_sec);
		if (rainbow_command(device, command, NULL)) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
}

static void mount_tracking_callback(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		rainbow_command(device, ":CtA#", MOUNT_TRACKING_PROPERTY);
	} else {
		rainbow_command(device, ":CtL#", MOUNT_TRACKING_PROPERTY);
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_guide_rate_callback(indigo_device *device) {
	char command[128];
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	sprintf(command, ":CU0=%3.1f#", MOUNT_GUIDE_RATE_RA_ITEM->number.value / 100.0);
	rainbow_command(device, command, MOUNT_GUIDE_RATE_PROPERTY);
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
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
		if (IS_CONNECTED) {
			if (MOUNT_PARK_PARKED_ITEM->sw.value) {
				MOUNT_PARK_PARKED_ITEM->sw.value = false;
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, mount_park_callback, NULL);
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_geographic_coordinates_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_equatorial_coordinates_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_abort_motion_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_ns_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_we_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_set_host_time_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_utc_time_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_tracking_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_guide_rate_callback, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// --------------------------------------------------------------------------------

static rainbow_private_data *private_data = NULL;

static indigo_device *mount = NULL;

indigo_result indigo_mount_rainbow(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_RAINBOW_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "RainbowAstro Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(rainbow_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
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
