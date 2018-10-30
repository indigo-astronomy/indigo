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

/** INDIGO LX200 driver
 \file indigo_mount_lx200.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_mount_lx200"

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

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_mount_lx200.h"

#define PRIVATE_DATA        ((lx200_private_data *)device->private_data)

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
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
	bool isMeade;
	bool isEQMac;
	bool is10Microns;
	bool isGemini;
	bool isAvalon;
} lx200_private_data;

static bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool meade_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
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
			host_name[colon - host] = 0;
			int port = atoi(colon + 1);
			PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
		}
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
		return false;
	}
}

static bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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
		usleep(sleep);
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

//static bool gemini_command(indigo_device *device, char *command, char *response, int max) {
//	char buffer[128];
//	uint8_t checksum = command[0];
//	for (size_t i=1; i < strlen(command); i++)
//		checksum = checksum ^ command[i];
//	checksum = checksum % 128;
//	checksum += 64;
//	snprintf(buffer, sizeof(buffer), "%s%c#", command, checksum);
//	return meade_command(device, buffer, response, max, 0);
//}

static void meade_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void meade_get_coords(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GR#", response, sizeof(response), 0)) {
		if (strlen(response) < 8) {
			if (PRIVATE_DATA->is10Microns)
				meade_command(device, ":U1#", NULL, 0, 0);
			else if (PRIVATE_DATA->isGemini)
				meade_command(device, ":U#", NULL, 0, 0);
			else
				meade_command(device, ":P#", response, sizeof(response), 0);
			meade_command(device, ":GR#", response, sizeof(response), 0);
		}
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = indigo_stod(response);
	}
	if (meade_command(device, ":GD#", response, sizeof(response), 0))
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = indigo_stod(response);
	if (PRIVATE_DATA->isMeade || PRIVATE_DATA->is10Microns) {
		if (meade_command(device, ":D#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = *response ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->isGemini) {
		if (meade_command(device, ":Gv#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (*response == 'S' || *response == 'C') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->isAvalon) {
		if (meade_command(device, ":X34#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (response[1] == '5' || response[2] == '5') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target) < 1.0/3600.0 && fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target) < 1.0/3600.0)
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	}
}

static void meade_get_utc(indigo_device *device) {
	if (PRIVATE_DATA->isMeade || PRIVATE_DATA->isGemini || PRIVATE_DATA->is10Microns) {
		struct tm tm;
		char response[128];
		memset(&tm, 0, sizeof(tm));
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		if (meade_command(device, ":GC#", response, sizeof(response), 0) && sscanf(response, "%02d/%02d/%02d", &tm.tm_mon, &tm.tm_mday, &tm.tm_year) == 3) {
			if (meade_command(device, ":GL#", response, sizeof(response), 0) && sscanf(response, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (meade_command(device, ":GG#", response, sizeof(response), 0)) {
					tm.tm_isdst = -1;
					tm.tm_gmtoff = atoi(response) * 3600;
					time_t secs = mktime(&tm);
					indigo_timetoiso(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
					sprintf(MOUNT_UTC_OFFEST_ITEM->text.value, "%g", atof(response));
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		}
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		meade_get_coords(device);
		indigo_update_coordinates(device, NULL);
		meade_get_utc(device);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

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
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		ALIGNMENT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, ALIGNMENT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Alignment mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (ALIGNMENT_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		ALIGNMENT_MODE_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(ALIGNMENT_MODE_PROPERTY, property) && !ALIGNMENT_MODE_PROPERTY->hidden)
			indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
	}
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
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = meade_open(device);
			}
			if (result) {
				if (meade_command(device, ":GVP#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "product:  %s", response);
					strncpy(PRIVATE_DATA->product, response, 64);
				}
				ALIGNMENT_MODE_PROPERTY->hidden = true;
				if (!strncmp(PRIVATE_DATA->product, "LX", 2) || !strncmp(PRIVATE_DATA->product, "Autostar", 8)) {
					PRIVATE_DATA->isMeade = true;
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
					MOUNT_UTC_TIME_PROPERTY->hidden = false;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					MOUNT_PARK_PROPERTY->count = 1;
					MOUNT_PARK_PARKED_ITEM->sw.value = false;
					PRIVATE_DATA->parked = false;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
					if (meade_command(device, ":GVF#", response, sizeof(response), 0)) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "version:  %s", response);
						char *sep = strchr(response, '|');
						if (sep != NULL)
							*sep = 0;
						strncpy(MOUNT_INFO_MODEL_ITEM->text.value, response, INDIGO_VALUE_SIZE);
					} else {
						strncpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, INDIGO_VALUE_SIZE);
					}
					if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "firmware: %s", response);
						strncpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response, INDIGO_VALUE_SIZE);
					}
					if (meade_command(device, ":GW#", response, sizeof(response), 0)) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "status:   %s", response);
						ALIGNMENT_MODE_PROPERTY->hidden = false;
						if (*response == 'P') {
							indigo_set_switch(ALIGNMENT_MODE_PROPERTY, POLAR_MODE_ITEM, true);
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						} else if (*response == 'A') {
							indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						} else {
							indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
						}
						indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
					}
					if (meade_command(device, ":Gt#", response, sizeof(response), 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
						double longitude = indigo_stod(response);
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
					meade_get_coords(device);
					meade_get_utc(device);
				} else if (!strcmp(PRIVATE_DATA->product, "EQMac")) {
					PRIVATE_DATA->isEQMac = true;
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
					MOUNT_UTC_TIME_PROPERTY->hidden = true;
					MOUNT_TRACKING_PROPERTY->hidden = true;
					meade_get_coords(device);
					if (MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value == 0 && MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value == 0) {
						indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
						PRIVATE_DATA->parked = true;
					} else {
						indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
						PRIVATE_DATA->parked = false;
					}
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "N/A");
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "EQMac");
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				} else if (!strncmp(PRIVATE_DATA->product, "10micron", 8)) {
					PRIVATE_DATA->is10Microns = true;
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
					MOUNT_UTC_TIME_PROPERTY->hidden = false;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "10Micron");
					strncpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, INDIGO_VALUE_SIZE);
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
					MOUNT_PARK_PROPERTY->count = 2;
					PRIVATE_DATA->parked = false;
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					if (meade_command(device, ":Gstat#", response, sizeof(response), 0)) {
						if (*response == '0') {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						} else if (*response == '5') {
							indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
							PRIVATE_DATA->parked = true;
						}
					}
					if (meade_command(device, ":Gt#", response, sizeof(response), 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
						double longitude = indigo_stod(response);
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
					meade_get_coords(device);
					meade_get_utc(device);
				} else if (!strncmp(PRIVATE_DATA->product, "Losmandy", 8)) {
					PRIVATE_DATA->isGemini = true;
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
					MOUNT_UTC_TIME_PROPERTY->hidden = false;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Losmandy");
					strncpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, INDIGO_VALUE_SIZE);
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
					MOUNT_PARK_PROPERTY->count = 2;
					PRIVATE_DATA->parked = false;
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					if (meade_command(device, ":Gv#", response, 1, 0)) {
						if (*response == 'T' || *response == 'G') {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						} else if (*response == 'N') {
							indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
							PRIVATE_DATA->parked = true;
						}
					}
					if (meade_command(device, ":Gt#", response, sizeof(response), 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
						double longitude = indigo_stod(response);
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
					meade_get_coords(device);
					meade_get_utc(device);
				} else if (!strncmp(PRIVATE_DATA->product, "Avalon", 6)) {
					PRIVATE_DATA->isAvalon = true;
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
					MOUNT_UTC_TIME_PROPERTY->hidden = true;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
					strncpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, INDIGO_VALUE_SIZE);
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
					MOUNT_PARK_PROPERTY->count = 2;
					PRIVATE_DATA->parked = false;
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					if (meade_command(device, ":X34#", response, sizeof(response), 0)) {
						if (response[1] == '1') {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						} else {
							indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
							PRIVATE_DATA->parked = true;
						}
					}
					if (meade_command(device, ":Gt#", response, sizeof(response), 0))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
						double longitude = indigo_stod(response);
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
					meade_get_coords(device);
					meade_get_utc(device);
				} else {
					MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
					MOUNT_UTC_TIME_PROPERTY->hidden = true;
					MOUNT_TRACKING_PROPERTY->hidden = true;
					MOUNT_PARK_PROPERTY->hidden = true;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
					meade_get_coords(device);
				}
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
			if (PRIVATE_DATA->isGemini)
				meade_command(device, ":hC#", NULL, 0, 0);
			else if (PRIVATE_DATA->isAvalon)
				meade_command(device, ":X362#", NULL, 0, 0);
			else
				meade_command(device, ":hP#", NULL, 0, 0);
			PRIVATE_DATA->parked = true;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
		}
		if (PRIVATE_DATA->parked && MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			if (PRIVATE_DATA->isEQMac)
				meade_command(device, ":hU#", NULL, 0, 0);
			else if (PRIVATE_DATA->isGemini)
				meade_command(device, ":hW#", NULL, 0, 0);
			else if (PRIVATE_DATA->is10Microns)
				meade_command(device, ":PO#", NULL, 0, 0);
			else if (PRIVATE_DATA->isAvalon)
				meade_command(device, ":X370#", NULL, 0, 0);
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
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				double longitude = fmod((360 - MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value), 360);
				sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%03d*%02d"));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
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
				if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
					if (PRIVATE_DATA->isGemini)
						meade_command(device, ">130:131E#", NULL, 0, 0);
					else
						meade_command(device, ":TQ#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'q';
				} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
					if (PRIVATE_DATA->isGemini)
						meade_command(device, ">130:134@", NULL, 0, 0);
					else if (PRIVATE_DATA->is10Microns)
						meade_command(device, ":TSOLAR#", NULL, 0, 0);
					else
						meade_command(device, ":TS#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 's';
				} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
					if (PRIVATE_DATA->isGemini)
						meade_command(device, ">130:133G#", NULL, 0, 0);
					else
						meade_command(device, ":TL#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'l';
				}
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (!meade_command(device, ":MS#", response, 1, 100000) || *response != '0') {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS# failed");
							MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						}
					}
				}
			} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (!meade_command(device, ":CM#", response, sizeof(response), 100000) || *response == 0) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed");
							MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
						}
					}
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
			if (PRIVATE_DATA->isAvalon) {
				if (PRIVATE_DATA->lastMotionNS == 'n' || PRIVATE_DATA->lastMotionNS == 's') {
					meade_command(device, ":Q#", NULL, 0, 0);
				}
			} else {
				if (PRIVATE_DATA->lastMotionNS == 'n') {
					meade_command(device, ":Qn#", NULL, 0, 0);
				} else if (PRIVATE_DATA->lastMotionNS == 's') {
					meade_command(device, ":Qs#", NULL, 0, 0);
				}
			}
			if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 'n';
				meade_command(device, ":Mn#", NULL, 0, 0);
			} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionNS = 's';
				meade_command(device, ":Ms#", NULL, 0, 0);
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
			if (PRIVATE_DATA->isAvalon) {
				if (PRIVATE_DATA->lastMotionWE == 'w' || PRIVATE_DATA->lastMotionWE == 'e') {
					meade_command(device, ":Q#", NULL, 0, 0);
				}
			} else {
				if (PRIVATE_DATA->lastMotionWE == 'w') {
					meade_command(device, ":Qw#", NULL, 0, 0);
				} else if (PRIVATE_DATA->lastMotionWE == 'e') {
					meade_command(device, ":Qe#", NULL, 0, 0);
				}
			}
			if (MOUNT_MOTION_WEST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'w';
				meade_command(device, ":Mw#", NULL, 0, 0);
			} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
				PRIVATE_DATA->lastMotionWE = 'e';
				meade_command(device, ":Me#", NULL, 0, 0);
			} else {
				PRIVATE_DATA->lastMotionWE = 0;
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
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":SG%02ld#", tm.tm_gmtoff / 3600);
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
						MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
						indigo_timetoiso(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
						indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
					}
				}
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
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_mount_lx200: Wrong date/time format!");
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		} else {
			struct tm tm = *localtime(&secs);
			char command[20], response[2];
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
					sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						sprintf(command, ":SG%02ld#", tm.tm_gmtoff / 3600);
						if (!meade_command(device, command, response, 1, 0) || *response != '1') {
							MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
						} else {
						MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
					}
				}
			}
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		if (MOUNT_TRACKING_ON_ITEM->sw.value) {
			if (PRIVATE_DATA->is10Microns) {
				meade_command(device, ":AP#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (PRIVATE_DATA->isGemini) {
				meade_command(device, ">190:192F#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (PRIVATE_DATA->isAvalon) {
				meade_command(device, ":X122#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				if (ALTAZ_MODE_ITEM->sw.value) {
					meade_command(device, ":AA#", NULL, 0, 0);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else if (POLAR_MODE_ITEM->sw.value) {
					meade_command(device, ":AP#", NULL, 0, 0);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
		} else {
			if (PRIVATE_DATA->isGemini) {
				meade_command(device, ">190:191E#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (PRIVATE_DATA->isAvalon) {
				meade_command(device, ":X120#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				meade_command(device, ":AL#", NULL, 0, 0);
			}
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static lx200_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_lx200(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_LX200_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_LX200_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "LX200 Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(lx200_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(lx200_private_data));
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
