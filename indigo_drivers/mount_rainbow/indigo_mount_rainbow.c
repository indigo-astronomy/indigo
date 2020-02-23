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

#define DRIVER_VERSION 0x0002
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
	bool parked;
	int handle;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	char lastUTC[INDIGO_VALUE_SIZE];
} rainbow_private_data;

static bool rainbow_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool rainbow_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "rainbow")) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
	} else {
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 4030, &proto);
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static bool rainbow_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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
	if (sleep > 0)
		indigo_usleep(sleep);
	// read response
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
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			if (c < 0)
				c = ':';
			if (c == '#')
				break;
			response[index++] = c;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static void rainbow_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void rainbow_get_coords(indigo_device *device) {
	char response[128];
	if (rainbow_command(device, ":GR#", response, sizeof(response), 0))
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = indigo_stod(response + 3);
	if (rainbow_command(device, ":GD#", response, sizeof(response), 0))
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = indigo_stod(response + 3);
	if (rainbow_command(device, ":CL#", response, sizeof(response), 0))
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strcmp(response, ":CL0") ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
}

static void rainbow_get_utc(indigo_device *device) {
	// TBD
}

static void rainbow_get_observatory(indigo_device *device) {
	char response[128];
	if (rainbow_command(device, ":Gt#", response, sizeof(response), 0))
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response + 3);
	if (rainbow_command(device, ":Gg#", response, sizeof(response), 0)) {
		double longitude = indigo_stod(response + 3);
		if (longitude < 0)
			longitude += 360;
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 360 - longitude;
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		rainbow_get_coords(device);
		indigo_update_coordinates(device, NULL);
		rainbow_get_utc(device);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_EPOCH
		MOUNT_EPOCH_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		MOUNT_GUIDE_RATE_PROPERTY->count = 1;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[128], response[128];
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			bool result = rainbow_open(device);
			if (result) {
				if (rainbow_command(device, ":AV#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response + 3);
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "RainbowAstro");
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
					strncpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response + 3, INDIGO_VALUE_SIZE);
				}
				MOUNT_UTC_TIME_PROPERTY->hidden = true; // TBD - unclear
				// TBD - park state
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				PRIVATE_DATA->parked = false;
				if (rainbow_command(device, ":AT#", response, sizeof(response), 0)) {
					if (!strcmp(response, ":AT1")) {
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					} else {
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					}
				}
				if (rainbow_command(device, ":Ct?#", response, sizeof(response), 0)) {
					if (!strcmp(response, ":CT0")) {
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						PRIVATE_DATA->lastTrackRate = 'R';
					} else if (!strcmp(response, ":CT1")) {
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						PRIVATE_DATA->lastTrackRate = 'S';
					} else if (!strcmp(response, ":CT2")) {
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						PRIVATE_DATA->lastTrackRate = 'M';
					} else {
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						PRIVATE_DATA->lastTrackRate = 'U';
					}
				}
				if (rainbow_command(device, ":CU0", response, sizeof(response), 0)) {
					MOUNT_GUIDE_RATE_RA_ITEM->number.value = round(100 * atof(response + 5));
				}
				rainbow_get_observatory(device);
				rainbow_get_coords(device);
				rainbow_get_utc(device);
				PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			PRIVATE_DATA->position_timer = NULL;
			rainbow_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (!PRIVATE_DATA->parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
			// TBD
			PRIVATE_DATA->parked = true;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
		}
		if (PRIVATE_DATA->parked && MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			PRIVATE_DATA->parked = false;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			sprintf(command, ":St%s#", indigo_dtos(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, "%+03d*%02d"));
			if (!rainbow_command(device, command, NULL, 0, 0)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				double longitude = fmod((360 - MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value), 360);
				sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%03d*%02d"));
				if (!rainbow_command(device, command, NULL, 0, 0) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (PRIVATE_DATA->parked) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, "Mount is parked!");
		} else {
			indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
				if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'R') {
					rainbow_command(device, ":CtR#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'R';
				} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'S') {
					rainbow_command(device, ":CtS#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'S';
				} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'M') {
					rainbow_command(device, ":CtM#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'M';
				} else if (MOUNT_TRACK_RATE_CUSTOM_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'U') {
					rainbow_command(device, ":CtU#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'U';
				}
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%04.1f"));
				if (!rainbow_command(device, command, response, 1, 0) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%04.1f"));
					if (!rainbow_command(device, command, response, 1, 0) || *response != '1') {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (!rainbow_command(device, ":MS#", NULL, 0, 0)) { // TBD - unclear
							INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS# failed");
							MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						}
					}
				}
			} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				sprintf(command, ":Ck%07.3f%+7.3f#", MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
				if (!rainbow_command(device, command, NULL, 0, 0)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
			indigo_update_coordinates(device, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (PRIVATE_DATA->parked) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
				PRIVATE_DATA->position_timer = NULL;
				rainbow_command(device, ":Q#", NULL, 0, 0);
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
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
				rainbow_command(device, ":RG#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'g';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
				rainbow_command(device, ":RC#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'c';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
				rainbow_command(device, ":RM#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'm';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
				rainbow_command(device, ":RS#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 's';
			}
			if (PRIVATE_DATA->lastMotionNS == 'n' || PRIVATE_DATA->lastMotionNS == 's') {
				rainbow_command(device, ":Q#", NULL, 0, 0);
			}
			if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 'n';
				rainbow_command(device, ":Mn#", NULL, 0, 0);
			} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 's';
				rainbow_command(device, ":Ms#", NULL, 0, 0);
			} else {
				PRIVATE_DATA->lastMotionNS = 0;
			}
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
				rainbow_command(device, ":RG#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'g';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
				rainbow_command(device, ":RC#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'c';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
				rainbow_command(device, ":RM#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'm';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
				rainbow_command(device, ":RS#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 's';
			}
			if (PRIVATE_DATA->lastMotionWE == 'w' || PRIVATE_DATA->lastMotionWE == 'e') {
				rainbow_command(device, ":Q#", NULL, 0, 0);
			}
			if (MOUNT_MOTION_WEST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'w';
				rainbow_command(device, ":Mw#", NULL, 0, 0);
			} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'e';
				rainbow_command(device, ":Me#", NULL, 0, 0);
			} else {
				PRIVATE_DATA->lastMotionWE = 0;
			}
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
			// TBD
		}
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		// TBD
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		if (MOUNT_TRACKING_ON_ITEM->sw.value) {
			rainbow_command(device, ":CtA#", NULL, 0, 0);
		} else {
			rainbow_command(device, ":CtL#", NULL, 0, 0);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		sprintf(command, ":CU0=%3.1f#", MOUNT_GUIDE_RATE_RA_ITEM->number.value / 100.0);
		rainbow_command(device, command, NULL, 0, 0);
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
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
			private_data = malloc(sizeof(rainbow_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(rainbow_private_data));
			mount = malloc(sizeof(indigo_device));
			assert(mount != NULL);
			memcpy(mount, &mount_template, sizeof(indigo_device));
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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
