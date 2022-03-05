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

#define DRIVER_VERSION 0x0013
#define DRIVER_NAME	"indigo_mount_lx200"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_mount_lx200.h"

#define PRIVATE_DATA        ((lx200_private_data *)device->private_data)

#define ALIGNMENT_MODE_PROPERTY					(PRIVATE_DATA->alignment_mode_property)
#define POLAR_MODE_ITEM                 (ALIGNMENT_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM                 (ALIGNMENT_MODE_PROPERTY->items+1)

#define ALIGNMENT_MODE_PROPERTY_NAME		"X_ALIGNMENT_MODE"
#define POLAR_MODE_ITEM_NAME            "POLAR"
#define ALTAZ_MODE_ITEM_NAME            "ALTAZ"

#define FORCE_FLIP_PROPERTY							(PRIVATE_DATA->force_flip_property)
#define FORCE_FLIP_ENABLED_ITEM         (FORCE_FLIP_PROPERTY->items+0)
#define FORCE_FLIP_DISABLED_ITEM        (FORCE_FLIP_PROPERTY->items+1)

#define FORCE_FLIP_PROPERTY_NAME				"X_FORCE_FLIP"
#define FORCE_FLIP_ENABLED_ITEM_NAME		"ENABLED"
#define FORCE_FLIP_DISABLED_ITEM_NAME		"DISABLED"

#define MOUNT_TYPE_PROPERTY							(PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_DETECT_ITEM          (MOUNT_TYPE_PROPERTY->items+0)
#define MOUNT_TYPE_MEADE_ITEM          	(MOUNT_TYPE_PROPERTY->items+1)
#define MOUNT_TYPE_EQMAC_ITEM          	(MOUNT_TYPE_PROPERTY->items+2)
#define MOUNT_TYPE_10MICRONS_ITEM       (MOUNT_TYPE_PROPERTY->items+3)
#define MOUNT_TYPE_GEMINI_ITEM         	(MOUNT_TYPE_PROPERTY->items+4)
#define MOUNT_TYPE_AVALON_ITEM          (MOUNT_TYPE_PROPERTY->items+5)
#define MOUNT_TYPE_AP_ITEM          		(MOUNT_TYPE_PROPERTY->items+6)
#define MOUNT_TYPE_ON_STEP_ITEM         (MOUNT_TYPE_PROPERTY->items+7)
#define MOUNT_TYPE_AGOTINO_ITEM         (MOUNT_TYPE_PROPERTY->items+8)
#define MOUNT_TYPE_ZWO_ITEM         		(MOUNT_TYPE_PROPERTY->items+9)

#define MOUNT_TYPE_PROPERTY_NAME				"X_MOUNT_TYPE"
#define MOUNT_TYPE_DETECT_ITEM_NAME			"DETECT"
#define MOUNT_TYPE_MEADE_ITEM_NAME			"MEADE"
#define MOUNT_TYPE_EQMAC_ITEM_NAME			"EQMAC"
#define MOUNT_TYPE_10MICRONS_ITEM_NAME	"10MIC"
#define MOUNT_TYPE_GEMINI_ITEM_NAME			"GEMINI"
#define MOUNT_TYPE_AVALON_ITEM_NAME			"AVALON"
#define MOUNT_TYPE_AP_ITEM_NAME					"AP"
#define MOUNT_TYPE_ON_STEP_ITEM_NAME		"ONSTEP"
#define MOUNT_TYPE_AGOTINO_ITEM_NAME		"AGOTINO"
#define MOUNT_TYPE_ZWO_ITEM_NAME				"ZWO_AM"

typedef struct {
	bool parked;
	int handle;
	int device_count;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	bool motioned;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
	indigo_property *force_flip_property;
	indigo_property *mount_type_property;
	indigo_timer *focuser_timer;
	bool use_dst_commands;
} lx200_private_data;

static bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool meade_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "lx200")) {
		PRIVATE_DATA->handle = indigo_open_serial(name);
	} else {
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 4030, &proto);
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		// flush the garbage if any...
		char c;
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		while (true) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
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
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
		}
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
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

static bool meade_command_progress(indigo_device *device, char *command, char *response, int max, int sleep) {
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
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "readout progress part...");
	char progress[128];
	// read progress
	int index = 0;
	int timeout = 60;
	while (index < sizeof(progress)) {
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
		progress[index++] = c;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Progress width: %d", index);
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
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void meade_get_coords(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GR#", response, sizeof(response), 0)) {
		if (strlen(response) < 8) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
				meade_command(device, ":P#", response, sizeof(response), 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				meade_command(device, ":U1#", NULL, 0, 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				meade_command(device, ":U#", NULL, 0, 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			}
		}
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = indigo_stod(response);
	}
	if (meade_command(device, ":GD#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = indigo_stod(response);
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (meade_command(device, ":D#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = *response ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		if (meade_command(device, ":Gv#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (*response == 'S' || *response == 'C') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
		if (meade_command(device, ":X34#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (response[1] == '5' || response[2] == '5') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		if (meade_command(device, ":GU#", response, sizeof(response), 0))
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strchr(response, 'N') ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	} else {
		if (PRIVATE_DATA->motioned) {
			// After Motion NS or EW
			if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			// After Track or Slew
			if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - PRIVATE_DATA->lastRA) < 2.0/60.0 && fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - PRIVATE_DATA->lastDec) < 2.0/60.0)
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			else
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	}
	// for Unknown case
	PRIVATE_DATA->lastRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	PRIVATE_DATA->lastDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
}

static void meade_get_utc(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
		struct tm tm;
		char response[128];
		memset(&tm, 0, sizeof(tm));
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		char separator[2];
		if (meade_command(device, ":GC#", response, sizeof(response), 0) && sscanf(response, "%d%c%d%c%d", &tm.tm_mon, separator, &tm.tm_mday, separator, &tm.tm_year) == 5) {
			if (meade_command(device, ":GL#", response, sizeof(response), 0) && sscanf(response, "%d%c%d%c%d", &tm.tm_hour, separator, &tm.tm_min, separator, &tm.tm_sec) == 5) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (meade_command(device, ":GG#", response, sizeof(response), 0)) {
					if (MOUNT_TYPE_AP_ITEM->sw.value && response[0] == ':') {
						if (response[1] == 'A') {
							switch (response[2]) {
								case '1':
									strcpy(response, "-05");
									break;
								case '2':
									strcpy(response, "-04");
									break;
								case '3':
									strcpy(response, "-03");
									break;
								case '4':
									strcpy(response, "-02");
									break;
								case '5':
									strcpy(response, "-01");
									break;
							}
						} else if (response[1] == '@') {
							switch (response[2]) {
								case '4':
									strcpy(response, "-12");
									break;
								case '5':
									strcpy(response, "-11");
									break;
								case '6':
									strcpy(response, "-10");
									break;
								case '7':
									strcpy(response, "-09");
									break;
								case '8':
									strcpy(response, "-08");
									break;
								case '9':
									strcpy(response, "-07");
									break;
							}
						} else if (response[1] == '0') {
							strcpy(response, "-06");
						}
					}
					tm.tm_gmtoff = -atoi(response) * 3600;
					sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", -atoi(response));
					if (PRIVATE_DATA->use_dst_commands) {
						if (meade_command(device, ":GH#", response, sizeof(response), 0)) {
							tm.tm_isdst = atoi(response);
						}
					} else {
						tm.tm_isdst = -1;
					}
					time_t secs = mktime(&tm);
					indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		}
	}
}

static void meade_get_observatory(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":Gt#", response, sizeof(response), 0)) {
		MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
	}
	if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
		double longitude = indigo_stod(response);
		if (longitude < 0)
			longitude += 360;
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 360 - longitude;
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

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
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
		// -------------------------------------------------------------------------------- FORCE_FLIP
		FORCE_FLIP_PROPERTY = indigo_init_switch_property(NULL, device->name, FORCE_FLIP_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Meridian flip mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FORCE_FLIP_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(FORCE_FLIP_ENABLED_ITEM, FORCE_FLIP_ENABLED_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(FORCE_FLIP_DISABLED_ITEM, FORCE_FLIP_DISABLED_ITEM_NAME, "Disabled", false);
		FORCE_FLIP_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 10);
		if (MOUNT_TYPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_TYPE_DETECT_ITEM, MOUNT_TYPE_DETECT_ITEM_NAME, "Autodetect", true);
		indigo_init_switch_item(MOUNT_TYPE_MEADE_ITEM, MOUNT_TYPE_MEADE_ITEM_NAME, "Meade", false);
		indigo_init_switch_item(MOUNT_TYPE_EQMAC_ITEM, MOUNT_TYPE_EQMAC_ITEM_NAME, "EQMac", false);
		indigo_init_switch_item(MOUNT_TYPE_10MICRONS_ITEM, MOUNT_TYPE_10MICRONS_ITEM_NAME, "10Microns", false);
		indigo_init_switch_item(MOUNT_TYPE_GEMINI_ITEM, MOUNT_TYPE_GEMINI_ITEM_NAME, "Gemini Losmandy", false);
		indigo_init_switch_item(MOUNT_TYPE_AVALON_ITEM, MOUNT_TYPE_AVALON_ITEM_NAME, "Avalon StarGO", false);
		indigo_init_switch_item(MOUNT_TYPE_AP_ITEM, MOUNT_TYPE_AP_ITEM_NAME, "Astro-Physics GTO", false);
		indigo_init_switch_item(MOUNT_TYPE_ON_STEP_ITEM, MOUNT_TYPE_ON_STEP_ITEM_NAME, "OnStep", false);
		indigo_init_switch_item(MOUNT_TYPE_AGOTINO_ITEM, MOUNT_TYPE_AGOTINO_ITEM_NAME, "aGotino", false);
		indigo_init_switch_item(MOUNT_TYPE_ZWO_ITEM, MOUNT_TYPE_ZWO_ITEM_NAME, "ZWO AM", false);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(MOUNT_TYPE_PROPERTY, property))
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(ALIGNMENT_MODE_PROPERTY, property))
			indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
		if (indigo_property_match(FORCE_FLIP_PROPERTY, property))
			indigo_define_property(device, FORCE_FLIP_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static void mount_connect_callback(indigo_device *device) {
	char response[128];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = meade_open(device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				if (meade_command(device, ":GVP#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Product:  %s", response);
					strncpy(PRIVATE_DATA->product, response, 64);
				}
				if (!strncmp(PRIVATE_DATA->product, "LX", 2) || !strncmp(PRIVATE_DATA->product, "Autostar", 8)) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_MEADE_ITEM, true);
				} else if (!strcmp(PRIVATE_DATA->product, "EQMac")) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_EQMAC_ITEM, true);
				} else if (!strncmp(PRIVATE_DATA->product, "10micron", 8)) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_10MICRONS_ITEM, true);
				} else if (!strncmp(PRIVATE_DATA->product, "Losmandy", 8)) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_GEMINI_ITEM, true);
				} else if (!strncmp(PRIVATE_DATA->product, "Avalon", 6)) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_AVALON_ITEM, true);
				} else if (!strncmp(PRIVATE_DATA->product, "On-Step", 7)) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ON_STEP_ITEM, true);
				} else if (!strncmp(PRIVATE_DATA->product, "AM", 2) && !isdigit(PRIVATE_DATA->product[2])) {
					indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ZWO_ITEM, true);
				}
			}
			indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
			ALIGNMENT_MODE_PROPERTY->hidden = true;
			FORCE_FLIP_PROPERTY->hidden = true;
			if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_PARK_PROPERTY->count = 1;
				MOUNT_PARK_PARKED_ITEM->sw.value = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				PRIVATE_DATA->parked = false;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
				if (meade_command(device, ":GVF#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Version:  %s", response);
					char *sep = strchr(response, '|');
					if (sep != NULL)
						*sep = 0;
					indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, response);
				} else {
					indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
				}
				if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
					indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
				}
				if (meade_command(device, ":GW#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Status:   %s", response);
					ALIGNMENT_MODE_PROPERTY->hidden = false;
					ALIGNMENT_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					if (*response == 'P' || *response == 'G') {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, POLAR_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, response[1] == 'T');
					} else if (*response == 'A') {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, response[1] == 'T');
					} else {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, false);
					}
					indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
				}
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
				if (meade_command(device, ":GH#", response, sizeof(response), 0)) {
					PRIVATE_DATA->use_dst_commands = *response != 0;
				}
			} else if (MOUNT_TYPE_EQMAC_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
				MOUNT_UTC_TIME_PROPERTY->hidden = true;
				MOUNT_TRACKING_PROPERTY->hidden = true;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
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
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				MOUNT_HOME_PROPERTY->hidden = false;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "10Micron");
				indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				MOUNT_PARK_PROPERTY->count = 2;
				PRIVATE_DATA->parked = false;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				meade_command(device, ":EMUAP#", NULL, 0, 0);
				meade_command(device, ":U1#", NULL, 0, 0);
				if (meade_command(device, ":Gstat#", response, sizeof(response), 0)) {
					if (*response == '0') {
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					} else if (*response == '5') {
						indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
						PRIVATE_DATA->parked = true;
					}
				}
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Losmandy");
				indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
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
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
				MOUNT_UTC_TIME_PROPERTY->hidden = true;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
				MOUNT_HOME_PROPERTY->hidden = false;
				FORCE_FLIP_PROPERTY->hidden = false;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "AvalonGO");
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
				if (meade_command(device, ":X22#", response, sizeof(response), 0)) {
					int ra, dec;
					if (sscanf(response, "%db%d#", &ra, &dec) == 2) {
						MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target = ra;
						MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = dec;
						MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
					} else {
						MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
					}
				}
				meade_command(device, ":TTSFd#",  NULL, 0, 0);
				indigo_define_property(device, FORCE_FLIP_PROPERTY, NULL);
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "AstroPhysics");
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				MOUNT_PARK_PROPERTY->count = 2;
				PRIVATE_DATA->parked = true;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				meade_command(device, ":U#", NULL, 0, 0);
				meade_command(device, ":Br 00:00:00#", NULL, 0, 0);
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				MOUNT_PEC_PROPERTY->hidden = false;
				MOUNT_PARK_PROPERTY->count = 1;
				MOUNT_PARK_PARKED_ITEM->sw.value = false;
				PRIVATE_DATA->parked = false;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "On-Step");
				if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
					indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
				}
				if (meade_command(device, ":GW#", response, sizeof(response), 0)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Status:   %s", response);
					ALIGNMENT_MODE_PROPERTY->hidden = false;
					ALIGNMENT_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					if (*response == 'P' || *response == 'G') {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, POLAR_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, response[1] == 'T');
					} else if (*response == 'A') {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, response[1] == 'T');
					} else {
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					}
					indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
				}
				if (meade_command(device, ":$QZ?#", response, sizeof(response), 0))
					indigo_set_switch(MOUNT_PEC_PROPERTY, response[0] == 'P' ? MOUNT_PEC_ENABLED_ITEM : MOUNT_PEC_DISABLED_ITEM, true);
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
				MOUNT_UTC_TIME_PROPERTY->hidden = true;
				MOUNT_TRACKING_PROPERTY->hidden = true;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				MOUNT_PARK_PROPERTY->hidden = true;
				MOUNT_MOTION_RA_PROPERTY->hidden = true;
				MOUNT_MOTION_DEC_PROPERTY->hidden = true;
				MOUNT_SLEW_RATE_PROPERTY->hidden = true;
				MOUNT_TRACK_RATE_PROPERTY->hidden = true;
				MOUNT_INFO_PROPERTY->count = 1;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "aGotino");
				PRIVATE_DATA->parked = false;
				meade_get_coords(device);
			} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
				if (meade_command(device, ":GV#", response, sizeof(response), 0)) {
					MOUNT_INFO_PROPERTY->count = 3;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ZWO");
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "AM5");
					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
				}
				ALIGNMENT_MODE_PROPERTY->hidden = false;
				ALIGNMENT_MODE_PROPERTY->perm = INDIGO_RO_PERM;
				if (meade_command(device, ":GU#", response, sizeof(response), 0)) {
					if (strchr(response, 'n'))
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					if (strchr(response, 'G'))
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, POLAR_MODE_ITEM, true);
					if (strchr(response, 'Z'))
						indigo_set_switch(ALIGNMENT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
				}
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
				indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
				MOUNT_UTC_TIME_PROPERTY->hidden = false;
				MOUNT_TRACKING_PROPERTY->hidden = false;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				MOUNT_PARK_PROPERTY->hidden = false;
				MOUNT_PARK_PROPERTY->count = 1;
				PRIVATE_DATA->parked = false;
				MOUNT_MOTION_RA_PROPERTY->hidden = false;
				MOUNT_MOTION_DEC_PROPERTY->hidden = false;
				MOUNT_SLEW_RATE_PROPERTY->hidden = false;
				MOUNT_TRACK_RATE_PROPERTY->hidden = false;
				meade_get_observatory(device);
				meade_get_coords(device);
				meade_get_utc(device);
			} else {
				MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
				MOUNT_UTC_TIME_PROPERTY->hidden = true;
				MOUNT_TRACKING_PROPERTY->hidden = true;
				MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
				MOUNT_PARK_PROPERTY->hidden = true;
				MOUNT_MOTION_RA_PROPERTY->hidden = true;
				MOUNT_MOTION_DEC_PROPERTY->hidden = true;
				MOUNT_INFO_PROPERTY->count = 1;
				strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
				PRIVATE_DATA->parked = false;
				meade_get_coords(device);
			}
			// initialize target
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			MOUNT_TYPE_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		if (--PRIVATE_DATA->device_count == 0) {
			meade_command(device, ":Q#", NULL, 0, 0);
			meade_close(device);
		}
		indigo_delete_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
		indigo_delete_property(device, FORCE_FLIP_PROPERTY, NULL);
		MOUNT_TYPE_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}



static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[128], response[128];
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
		if (!PRIVATE_DATA->parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_EQMAC_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value)
				meade_command(device, ":hP#", NULL, 0, 0);
			else if (MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value)
				meade_command(device, ":hC#", NULL, 0, 0);
			else if (MOUNT_TYPE_AVALON_ITEM->sw.value)
				meade_command(device, ":X362#", NULL, 0, 0);
			else if (MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value)
				meade_command(device, ":KA#", NULL, 0, 0);
			PRIVATE_DATA->parked = true;
			if (MOUNT_PARK_PROPERTY->count == 1)
				MOUNT_PARK_PARKED_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
		}
		if (PRIVATE_DATA->parked && MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			if (MOUNT_TYPE_EQMAC_ITEM->sw.value)
				meade_command(device, ":hU#", NULL, 0, 0);
			else if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
				meade_command(device, ":hW#", NULL, 0, 0);
			else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value)
				meade_command(device, ":PO#", NULL, 0, 0);
			else if (MOUNT_TYPE_AVALON_ITEM->sw.value)
				meade_command(device, ":X370#", NULL, 0, 0);
			else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value)
				meade_command(device, ":hR#", NULL, 0, 0);
			PRIVATE_DATA->parked = false;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (!MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
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
			}
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_PARK_PROPERTY->count == 2 && PRIVATE_DATA->parked) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, "Mount is parked!");
		} else {
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->motioned = false;
			indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
				if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
					if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
						meade_command(device, ">130:131E#", NULL, 0, 0);
					else if (MOUNT_TYPE_AP_ITEM->sw.value)
						meade_command(device, ":RT2#", NULL, 0, 0);
					else
						meade_command(device, ":TQ#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 'q';
				} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
					if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
						meade_command(device, ">130:134@", NULL, 0, 0);
					else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value)
						meade_command(device, ":TSOLAR#", NULL, 0, 0);
					else if (MOUNT_TYPE_AP_ITEM->sw.value)
						meade_command(device, ":RT1#", NULL, 0, 0);
					else
						meade_command(device, ":TS#", NULL, 0, 0);
					PRIVATE_DATA->lastTrackRate = 's';
				} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
					if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
						meade_command(device, ">130:133G#", NULL, 0, 0);
					else if (MOUNT_TYPE_AP_ITEM->sw.value)
						meade_command(device, ":RT0#", NULL, 0, 0);
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
		if (MOUNT_PARK_PROPERTY->count == 2 && PRIVATE_DATA->parked) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked!");
		} else {
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->motioned = true;
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
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
		if (MOUNT_PARK_PROPERTY->count == 2 && PRIVATE_DATA->parked) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		} else {
			PRIVATE_DATA->parked = false;
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
					meade_command(device, ":RG2#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'g';
				} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
					meade_command(device, ":RC0#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'c';
				} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
					meade_command(device, ":RC1#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'm';
				} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
					meade_command(device, ":RC3#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 's';
				}
			} else {
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
			}
			if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
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
		if (MOUNT_PARK_PROPERTY->count == 2 && PRIVATE_DATA->parked) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		} else {
			PRIVATE_DATA->parked = false;
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
					meade_command(device, ":RG2#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'g';
				} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
					meade_command(device, ":RC0#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'c';
				} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
					meade_command(device, ":RC1#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 'm';
				} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
					meade_command(device, ":RC3#", NULL, 0, 0);
					PRIVATE_DATA->lastSlewRate = 's';
				}
			} else {
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
			}
			if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
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
			sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
			// :SCMM/DD/YY# returns two delimiters response:
			// "1Updating Planetary Data#                                #"
			// readout progress part
			bool result;
			if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value)
				result = meade_command(device, command, response, 1, 0);
			else
				result = meade_command_progress(device, command, response, sizeof(response), 0);
			if (!result || *response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (PRIVATE_DATA->use_dst_commands) {
					sprintf(command, ":SH%d#", tm.tm_isdst);
					meade_command(device, command, NULL, 0, 0);
				}
				sprintf(command, ":SG%+03ld#", -(tm.tm_gmtoff / 3600 - tm.tm_isdst));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
						MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
						indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
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
		time_t secs = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
		if (secs == -1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_mount_lx200: Wrong date/time format!");
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		} else {
			struct tm tm = *localtime(&secs);
			sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
			// :SCMM/DD/YY# returns two delimiters response:
			// "1Updating Planetary Data#                                #"
			// readout progress part
			bool result;
			if (MOUNT_TYPE_ON_STEP_ITEM->sw.value)
				result = meade_command(device, command, response, 1, 0);
			else
				result = meade_command_progress(device, command, response, sizeof(response), 0);
			if (!result || *response != '1') {
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (PRIVATE_DATA->use_dst_commands) {
					sprintf(command, ":SH%d#", tm.tm_isdst);
					meade_command(device, command, NULL, 0, 0);
				}
				sprintf(command, ":SG%+03ld#", -(tm.tm_gmtoff / 3600 - tm.tm_isdst));
				if (!meade_command(device, command, response, 1, 0) || *response != '1') {
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
					if (!meade_command(device, command, response, 1, 0) || *response != '1') {
						MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
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
			if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				meade_command(device, ":AP#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
				meade_command(device, ">190:192F#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				meade_command(device, ":X122#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} if (MOUNT_TYPE_AP_ITEM->sw.value) {
				if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
					meade_command(device, ":RT2#", NULL, 0, 0);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
					meade_command(device, ":RT1#", NULL, 0, 0);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
					meade_command(device, ":RT0#", NULL, 0, 0);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value) {
				meade_command(device, ":Te#", NULL, 0, 0);
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
			if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
				meade_command(device, ">190:191E#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				meade_command(device, ":X120#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} if (MOUNT_TYPE_AP_ITEM->sw.value) {
				meade_command(device, ":RT9#", NULL, 0, 0);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value) {
				meade_command(device, ":Td#", NULL, 0, 0);
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
		ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			if (ALTAZ_MODE_ITEM->sw.value) {
				meade_command(device, ":AA#", NULL, 0, 0);
			} else if (POLAR_MODE_ITEM->sw.value) {
				meade_command(device, ":AP#", NULL, 0, 0);
			}
			indigo_update_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FORCE_FLIP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FORCE_FLIP
		indigo_property_copy_values(FORCE_FLIP_PROPERTY, property, false);
		FORCE_FLIP_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			if (FORCE_FLIP_ENABLED_ITEM->sw.value) {
				meade_command(device, ":TTSFd#", NULL, 0, 0);
			} else if (FORCE_FLIP_DISABLED_ITEM->sw.value) {
				meade_command(device, ":TTRFd#", NULL, 0, 0);
			}
			indigo_update_property(device, FORCE_FLIP_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		indigo_property_copy_values(MOUNT_TYPE_PROPERTY, property, false);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (MOUNT_TYPE_EQMAC_ITEM->sw.value) {
			strcpy(DEVICE_PORT_ITEM->text.value, "lx200://localhost");
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		}
		indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PEC
		indigo_property_copy_values(MOUNT_PEC_PROPERTY, property, false);
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
		if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			if (meade_command(device, MOUNT_PEC_ENABLED_ITEM->sw.value ? "$QZ+" : "$QZ-", NULL, 0, 0))
				MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
		if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
			sprintf(command, ":X20%02d#", (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.target));
			if (meade_command(device, command, NULL, 0, 0)) {
				sprintf(command, ":X21%02d#", (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.target));
				if (meade_command(device, command, NULL, 0, 0)) {
					MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		}
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		if (MOUNT_HOME_ITEM->sw.value) {
			if (MOUNT_TYPE_AVALON_ITEM->sw.value) {
				if (meade_command(device, ":X361#", response, sizeof(response), 0) && strcmp(response, "pA") == 0) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				}
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				if (meade_command(device, ":hF#", NULL, 0, 0)) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
			MOUNT_HOME_ITEM->sw.value = false;
		}
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, ALIGNMENT_MODE_PROPERTY);
			indigo_save_property(device, NULL, FORCE_FLIP_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_TYPE_PROPERTY);
		}
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
	indigo_release_property(ALIGNMENT_MODE_PROPERTY);
	indigo_release_property(FORCE_FLIP_PROPERTY);
	indigo_release_property(MOUNT_TYPE_PROPERTY);
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
			result = meade_open(device->master_device);
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
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_callback(indigo_device *device) {
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		indigo_usleep(1000 * (int)GUIDER_GUIDE_NORTH_ITEM->number.value);
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		indigo_usleep(1000 * (int)GUIDER_GUIDE_SOUTH_ITEM->number.value);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_ra_callback(indigo_device *device) {
	if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		indigo_usleep(1000 * (int)GUIDER_GUIDE_WEST_ITEM->number.value);
	} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		indigo_usleep(1000 * (int)GUIDER_GUIDE_EAST_ITEM->number.value);
	}
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		char command[128];
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		if (MOUNT_TYPE_AP_ITEM->sw.value) {
			if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
				sprintf(command, ":Mn%3d#", (int)GUIDER_GUIDE_NORTH_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
				sprintf(command, ":Ms%3d#", (int)GUIDER_GUIDE_SOUTH_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			}
		} else {
			if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
				sprintf(command, ":Mgn%4d#", (int)GUIDER_GUIDE_NORTH_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
				sprintf(command, ":Mgs%4d#", (int)GUIDER_GUIDE_SOUTH_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			}
		}
		indigo_set_timer(device, 0, guider_guide_dec_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		char command[128];
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		if (MOUNT_TYPE_AP_ITEM->sw.value) {
			if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
				sprintf(command, ":Mw%3d#", (int)GUIDER_GUIDE_WEST_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
				sprintf(command, ":Me%3d#", (int)GUIDER_GUIDE_EAST_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			}
		} else {
			if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
				sprintf(command, ":Mgw%4d#", (int)GUIDER_GUIDE_WEST_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
				sprintf(command, ":Mge%4d#", (int)GUIDER_GUIDE_EAST_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			}
		}
		indigo_set_timer(device, 0, guider_guide_ra_callback, NULL);
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

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	if (IS_CONNECTED) {
		char response[16];
		if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
			meade_command(device, ":FQ#", NULL, 0, 0);
			if (meade_command(device, ":FP#", response, sizeof(response), 0)) {
				FOCUSER_POSITION_ITEM->number.value = atoi(response);
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			if (!meade_command(device, ":FG#", response, sizeof(response), 0)) {
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				FOCUSER_POSITION_ITEM->number.value = atoi(response);
				if (!meade_command(device, ":FT#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				} else if (response[0] == 'M') {
					indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				} else {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
				}
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
	char command[16], response[16];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			result = meade_open(device->master_device);
		}
		if (result) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
				FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
				FOCUSER_SPEED_ITEM->number.max = 2;
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				meade_command(device, FOCUSER_SPEED_ITEM->number.value == 1 ? ":FS#" : ":FF#", NULL, 0, 0);
				if (meade_command(device, ":FP#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.value = atoi(response);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
				FOCUSER_SPEED_ITEM->number.max = 4;
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				sprintf(command, "F%d", (int)FOCUSER_SPEED_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
				if (meade_command(device, ":FG#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.value = atoi(response);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
				} else {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				if (meade_command(device, ":FI#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.min = atoi(response);
				}
				if (meade_command(device, ":FM#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.max = atoi(response);
				}
			}
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
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	char command[16], response[16];
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_SPEED
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		if (IS_CONNECTED) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
				meade_command(device, FOCUSER_SPEED_ITEM->number.value == 1 ? ":FS#" : ":FF#", NULL, 0, 0);
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				sprintf(command, "F%d", (int)FOCUSER_SPEED_ITEM->number.value);
				meade_command(device, command, NULL, 0, 0);
			}
			FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
			if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value) {
				meade_command(device, ":F+#", NULL, 0, 0);
			} else {
				meade_command(device, ":F-#", NULL, 0, 0);
			}
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, FOCUSER_STEPS_ITEM->number.value / 1000, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			int sign = FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? 1 : -1;
			sprintf(command, "FR%+d", sign * (int)FOCUSER_STEPS_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			sprintf(command, "FS%+d", (int)FOCUSER_POSITION_ITEM->number.value);
			meade_command(device, command, NULL, 0, 0);
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
				meade_command(device, ":FQ#", NULL, 0, 0);
				if (meade_command(device, ":FP#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.value = atoi(response);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				meade_command(device, ":FQ#", NULL, 0, 0);
				if (meade_command(device, ":FG#", response, sizeof(response), 0)) {
					FOCUSER_POSITION_ITEM->number.value = atoi(response);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

static lx200_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;
static indigo_device *mount_focuser = NULL;

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
	static indigo_device mount_focuser_template = INDIGO_DEVICE_INITIALIZER(
	 MOUNT_LX200_FOCUSER_NAME,
	 focuser_attach,
	 indigo_focuser_enumerate_properties,
	 focuser_change_property,
	 NULL,
	 focuser_detach
	 );

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "LX200 Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(lx200_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			mount_guider->master_device = mount;
			indigo_attach_device(mount_guider);
			mount_focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_focuser_template);
			mount_focuser->private_data = private_data;
			mount_focuser->master_device = mount;
			indigo_attach_device(mount_focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			VERIFY_NOT_CONNECTED(mount_focuser);
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
			if (mount_focuser != NULL) {
				indigo_detach_device(mount_focuser);
				free(mount_focuser);
				mount_focuser = NULL;
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
