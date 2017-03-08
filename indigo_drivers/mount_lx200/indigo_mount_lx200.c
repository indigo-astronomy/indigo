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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO LX200 driver
 \file indigo_mount_lx200.c
 */

#define DRIVER_VERSION 0x0001

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

#include "indigo_driver_xml.h"
#include "indigo_mount_lx200.h"
#include "indigo_io.h"

#define PRIVATE_DATA        ((lx200_private_data *)device->private_data)

//#undef INDIGO_DEBUG_DRIVER
//#define INDIGO_DEBUG_DRIVER(c) c

#undef INDIGO_LOG
#define INDIGO_LOG(c) c

#define RA_MIN_DIF					0.1
#define DEC_MIN_DIF					0.1

#define ALIGNMENT_MODE_PROPERTY					(PRIVATE_DATA->alignment_mode_property)
#define POLAR_MODE_ITEM                 (ALIGNMENT_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM                 (ALIGNMENT_MODE_PROPERTY->items+1)

#define ALIGNMENT_MODE_PROPERTY_NAME		"X_ALIGNMENT_MODE"
#define POLAR_MODE_ITEM_NAME            "POLAR"
#define ALTAZ_MODE_ITEM_NAME            "ALTAZ"

typedef struct {
	bool parked;
	int handle;
	int device_count;
	indigo_property *device_port;
	double currentRA;
	double currentDec;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
} lx200_private_data;

bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep);

bool meade_open(indigo_device *device) {
	char *name = PRIVATE_DATA->device_port->items->text.value;
	if (strncmp(name, "lx200://", 8)) {
		PRIVATE_DATA->handle = indigo_open_serial(name);
	} else {
		char *host = name + 8;
		char *colon = strchr(host, ':');
		if (colon == NULL) {
			PRIVATE_DATA->handle = indigo_open_tcp(host, 4030);
		} else {
			char host_name[INDIGO_NAME_SIZE];
			strncpy(host_name, host, colon - host);
			int port = atoi(colon + 1);
			PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
		}
	}
	if (PRIVATE_DATA->handle >= 0) {
		indigo_send_message(device, "lx200: connected to %s", name);
		INDIGO_LOG(indigo_log("lx200: connected to %s", name));
		return true;
	} else {
		INDIGO_ERROR(indigo_error("lx200: failed to connect to %s", name));
		indigo_send_message(device, "lx200: failed to connect to %s", name);
		return false;
	}
}

bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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
	unsigned long remains = strlen(command);
	char *data = command;
	long written = 0;
	while (remains > 0) {
		written = write(PRIVATE_DATA->handle, data, remains);
		if (written < 0) {
			INDIGO_ERROR(indigo_error("lx200: Failed to write to %s -> %s (%d)", PRIVATE_DATA->device_port->items->text.value, strerror(errno), errno));
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		data += written;
		remains -= written;
	}
	if (sleep > 0)
		usleep(sleep);
	// read response
	if (response != NULL) {
		int index = 0;
		remains = max;
		int timeout = 3;
		while (remains > 0) {
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
				INDIGO_ERROR(indigo_error("lx200: Failed to read from %s -> %s (%d)", PRIVATE_DATA->device_port->items->text.value, strerror(errno), errno));
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
	INDIGO_DEBUG_DRIVER(indigo_debug("lx200: Command %s -> %s", command, response != NULL ? response : "NULL"));
	return true;
}

void meade_close(indigo_device *device) {
	close(PRIVATE_DATA->handle);
	PRIVATE_DATA->handle = 0;
	INDIGO_ERROR(indigo_error("lx200: disconnected from %s", PRIVATE_DATA->device_port->items->text.value));
}

static void meade_get_coords(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GR#", response, 127, 0))
		PRIVATE_DATA->currentRA = indigo_stod(response);
	if (meade_command(device, ":GD#", response, 127, 0))
		PRIVATE_DATA->currentDec = indigo_stod(response);
}

static void meade_get_utc(indigo_device *device) {
	struct tm tm;
	char response[128];
	memset(&tm, 0, sizeof(tm));
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	if (meade_command(device, ":GC#", response, 127, 0) && sscanf(response, "%02d/%02d/%02d", &tm.tm_mon, &tm.tm_mday, &tm.tm_year) == 3) {
		if (meade_command(device, ":GL#", response, 127, 0) && sscanf(response, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
			tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
			tm.tm_mon -= 1;
			tm.tm_isdst = -1;
			time_t secs = mktime(&tm);
			indigo_timetoiso(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			if (meade_command(device, ":GG#", response, 127, 0)) {
				sprintf(MOUNT_UTC_OFFEST_ITEM->text.value, "%g", atof(response));
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0 && !PRIVATE_DATA->parked) {
		meade_get_coords(device);
		double diffRA = fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target - PRIVATE_DATA->currentRA);
		double diffDec = fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target - PRIVATE_DATA->currentDec);
		if (diffRA <= RA_MIN_DIF && diffDec <= DEC_MIN_DIF)
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		meade_get_utc(device);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		PRIVATE_DATA->device_port = DEVICE_PORT_PROPERTY;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		ALIGNMENT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, ALIGNMENT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Alignment mode", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ALIGNMENT_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		ALIGNMENT_MODE_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (indigo_property_match(ALIGNMENT_MODE_PROPERTY, property) && !ALIGNMENT_MODE_PROPERTY->hidden)
			indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
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
				result = meade_open(device);
			}
			if (result) {
				char response[128];
				if (meade_command(device, ":GVP#", response, 127, 0)) {
					INDIGO_LOG(indigo_log("lx200: product:  %s", response));
					strncpy(PRIVATE_DATA->product, response, 64);
				}
				ALIGNMENT_MODE_PROPERTY->hidden = true;
				if (!strcmp(PRIVATE_DATA->product, "EQMac")) {
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
					MOUNT_UTC_TIME_PROPERTY->hidden = true;
					MOUNT_TRACKING_PROPERTY->hidden = true;
					MOUNT_PARK_PROPERTY->count = 2; // Can unpark!
					meade_get_coords(device);
					if (PRIVATE_DATA->currentRA == 0 && PRIVATE_DATA->currentDec == 0) {
						MOUNT_PARK_PARKED_ITEM->sw.value = true;
						MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
						PRIVATE_DATA->parked = true;
					} else {
						MOUNT_PARK_PARKED_ITEM->sw.value = false;
						MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
						PRIVATE_DATA->parked = false;
					}
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "EQMac");
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				} else {
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
					MOUNT_UTC_TIME_PROPERTY->hidden = false;
					MOUNT_PARK_PARKED_ITEM->sw.value = false;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					PRIVATE_DATA->parked = false;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
					if (meade_command(device, ":GVF#", response, 127, 0)) {
						INDIGO_LOG(indigo_log("lx200: version:  %s", response));
						char *sep = strchr(response, '|');
						if (sep != NULL)
							*sep = 0;
						strcpy(MOUNT_INFO_MODEL_ITEM->text.value, response);
					} else {
						strcpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
					}
					if (meade_command(device, ":GVN#", response, 127, 0)) {
						INDIGO_LOG(indigo_log("lx200: firmware: %s", response));
						strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
					}
					if (meade_command(device, ":GW#", response, 127, 0)) {
						INDIGO_LOG(indigo_log("lx200: status:   %s", response));
						ALIGNMENT_MODE_PROPERTY->hidden = false;
						if (*response == 'P') {
							POLAR_MODE_ITEM->sw.value = true;
							MOUNT_TRACKING_ON_ITEM->sw.value = true;
							MOUNT_TRACKING_OFF_ITEM->sw.value = false;
						} else if (*response == 'A') {
							ALTAZ_MODE_ITEM->sw.value = true;
							MOUNT_TRACKING_ON_ITEM->sw.value = true;
							MOUNT_TRACKING_OFF_ITEM->sw.value = false;
						} else {
							ALTAZ_MODE_ITEM->sw.value = true;
							MOUNT_TRACKING_ON_ITEM->sw.value = false;
							MOUNT_TRACKING_OFF_ITEM->sw.value = true;
						}
						
						indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
					}
					if (meade_command(device, ":Gt#", response, 127, 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (meade_command(device, ":Gg#", response, 127, 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = indigo_stod(response);
					meade_get_coords(device);
					meade_get_utc(device);
				}
				if (!PRIVATE_DATA->parked)
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			PRIVATE_DATA->position_timer = NULL;
			if (--PRIVATE_DATA->device_count == 0) {
				meade_close(device);
			}
			if (!ALIGNMENT_MODE_PROPERTY->hidden)
				indigo_delete_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (!PRIVATE_DATA->parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
			meade_command(device, ":hP#", NULL, 0, 0);
			PRIVATE_DATA->parked = true;
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
		}
		if (PRIVATE_DATA->parked && MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			meade_command(device, ":hU#", NULL, 0, 0);
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		char command[128], response[128];
		sprintf(command, ":St%s#", indigo_dtos(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, "%+03d*%02d"));
		if (!meade_command(device, command, response, 1, 0) || *response != '1') {
			INDIGO_ERROR(indigo_error("lx200: %s failed", command));
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			double longitude = 360-MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
			if (longitude > 360)
				longitude -= 360;
			sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%0d*%02d"));
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				INDIGO_ERROR(indigo_error("lx200: %s failed", command));
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (PRIVATE_DATA->parked) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
				if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
					meade_command(device, ":TQ#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'q';
				} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
					meade_command(device, ":TS#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 's';
				} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
					meade_command(device, ":TL#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'l';
				}
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				char command[128], response[128];
				sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					INDIGO_ERROR(indigo_error("lx200: %s failed", command));
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						INDIGO_ERROR(indigo_error("lx200: %s failed", command));
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (!meade_command(device, ":MS#", response, 1, 100000) || *response != '0') {
							INDIGO_ERROR(indigo_error("lx200: :MS# failed"));
							MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						}
					}
				}
			} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM) {
				char command[128], response[128];
				sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					INDIGO_ERROR(indigo_error("lx200: %s failed", command));
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						INDIGO_ERROR(indigo_error("lx200: %s failed", command));
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (!meade_command(device, ":CM#", response, 127, 100000) || *response == 0) {
							INDIGO_ERROR(indigo_error("lx200: :CM# failed"));
							MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						}
					}
				}
			}
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
				if (indigo_cancel_timer(device, &PRIVATE_DATA->position_timer)) {
					PRIVATE_DATA->position_timer = NULL;
					meade_command(device, ":Q#", NULL, 0, 0);
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
					indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
				}
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
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mout is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
				meade_command(device, ":RG#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'g';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
				meade_command(device, ":RC#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'c';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
				meade_command(device, ":RM#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'm';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
				meade_command(device, ":RS#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 's';
			}
			if (PRIVATE_DATA->lastMotionNS == 'n') {
				meade_command(device, ":Qn#", NULL, 0, 0);
			} else if (PRIVATE_DATA->lastMotionNS == 's') {
				meade_command(device, ":Qs#", NULL, 0, 0);
			}
			if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 'n';
				meade_command(device, ":Mn#", NULL, 0, 0);
			} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 's';
				meade_command(device, ":Ms#", NULL, 0, 0);
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
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
				meade_command(device, ":RG#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'g';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
				meade_command(device, ":RC#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'c';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
				meade_command(device, ":RM#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 'm';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
				meade_command(device, ":RS#", NULL, 0, 0);
				PRIVATE_DATA->lastSlewRate = 's';
			}
			if (PRIVATE_DATA->lastMotionWE == 'w') {
				meade_command(device, ":Qw#", NULL, 0, 0);
			} else if (PRIVATE_DATA->lastMotionWE == 'e') {
				meade_command(device, ":Qe#", NULL, 0, 0);
			}
			if (MOUNT_MOTION_WEST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'w';
				meade_command(device, ":Mwn#", NULL, 0, 0);
			} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'e';
				meade_command(device, ":Me#", NULL, 0, 0);
			}
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
			time_t secs = time(NULL);
			struct tm tm = *localtime(&secs);
			char command[20], response[2];
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_timetoiso(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
				indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			}
		}
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		time_t secs = indigo_isototime(MOUNT_UTC_ITEM->text.value);
		if (secs == -1) {
			INDIGO_ERROR(indigo_error("indigo_mount_lx200: Wrong date/time format!"));
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		} else {
			struct tm tm = *localtime(&secs);
			char command[20], response[2];
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!meade_command(device, command, response, 1, 0) || *response != '1')
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			else
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		if (MOUNT_TRACKING_ON_ITEM->sw.value) {
			if (ALTAZ_MODE_ITEM->sw.value) {
				meade_command(device, ":AA#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (POLAR_MODE_ITEM->sw.value) {
				meade_command(device, ":AP#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			meade_command(device, ":AL#", NULL, 0, 0);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ALIGNMENT_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		indigo_property_copy_values(ALIGNMENT_MODE_PROPERTY, property, false);
		if (ALTAZ_MODE_ITEM->sw.value) {
			meade_command(device, ":AA#", NULL, 0, 0);
		} else if (POLAR_MODE_ITEM->sw.value) {
			meade_command(device, ":AP#", NULL, 0, 0);
		}
		ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(ALIGNMENT_MODE_PROPERTY);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_LOG(indigo_log("%s attached", device->name));
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
				result = meade_open(device);
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
				meade_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		char command[128];
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			sprintf(command, ":Mgn%4.0f#", GUIDER_GUIDE_NORTH_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			sprintf(command, ":Mgs%4.0f#", GUIDER_GUIDE_SOUTH_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
		}
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		char command[128];
		if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			sprintf(command, ":Mgw%4.0f#", GUIDER_GUIDE_WEST_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
		} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			sprintf(command, ":Mge%4.0f#", GUIDER_GUIDE_EAST_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
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
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static lx200_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_lx200(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = {
		MOUNT_LX200_NAME, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		mount_detach
	};
	static indigo_device mount_guider_template = {
		MOUNT_LX200_GUIDER_NAME, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};
	
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, "LX200 Mount", __FUNCTION__, DRIVER_VERSION, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(lx200_private_data));
			memset(private_data, 0, sizeof(lx200_private_data));
			assert(private_data != NULL);
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

