// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO IOPTRON driver
 \file indigo_mount_ioptron.c
 */

#define DRIVER_VERSION 0x000F
#define DRIVER_NAME	"indigo_mount_ioptron"

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
#include <indigo/indigo_novas.h>

#include "indigo_mount_ioptron.h"

#define PRIVATE_DATA        ((ioptron_private_data *)device->private_data)

#define RA_MIN_DIF					0.1
#define DEC_MIN_DIF					0.1

typedef struct {
	int handle;
	int device_count;
	double currentRA;
	double currentDec;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastSlewRate, lastTrackRate, lastMotionRA, lastMotionDec;
	double lastRA, lastDec;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	unsigned protocol;
	bool gotonova;
} ioptron_private_data;

static bool ieq_command(indigo_device *device, char *command, char *response, int max) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;
		// flush
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
	}
		// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
		// read response
	if (response != NULL) {
		int index = 0;
		*response = 0;
		while (index < max) {
			tv.tv_usec = 500000;
			tv.tv_sec = 0;
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
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
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command '%s' -> '%s'", command, response != NULL ? response : "NULL");
	return true;
}

static void ieq_get_coords(indigo_device *device) {
	char response[128];
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100) {
		if (ieq_command(device, ":GR#", response, sizeof(response)))
			PRIVATE_DATA->currentRA = indigo_stod(response);
		if (ieq_command(device, ":GD#", response, sizeof(response)))
			PRIVATE_DATA->currentDec = indigo_stod(response);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	} else if (PRIVATE_DATA->protocol <= 0x0205) {
		long ra, dec;
		if (ieq_command(device, ":GEC#", response, sizeof(response)) && sscanf(response, "%9ld%8ld", &dec, &ra) == 2) {
			PRIVATE_DATA->currentDec = dec / 100.0 / 60.0 / 60.0;
			PRIVATE_DATA->currentRA = ra / 1000.0 / 60.0 / 60.0;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (PRIVATE_DATA->protocol >= 0x0300) {
		long ra, dec;
		char side_of_pier, pointing_state;
		if (ieq_command(device, ":GEP#", response, sizeof(response)) && sscanf(response, "%9ld%8ld%c%c", &dec, &ra, &side_of_pier, &pointing_state) == 2) {
			PRIVATE_DATA->currentDec = dec / 100.0 / 60.0 / 60.0;
			PRIVATE_DATA->currentRA = ra / 1000.0 / 60.0 / 60.0;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			if (!((MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value && side_of_pier == '0') || (MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value && side_of_pier == '1'))) {
				MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
				if (side_of_pier == '0')
					MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = true;
				else if (side_of_pier == '1')
					MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = true;
				MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		}
	}
}

static void ieq_get_utc(indigo_device *device) {
	struct tm tm;
	char response[128];
	memset(&tm, 0, sizeof(tm));
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	if (PRIVATE_DATA->protocol == 0x0000) {
		if (ieq_command(device, ":GC#", response, sizeof(response)) && sscanf(response, "%02d:%02d:%02d", &tm.tm_mon, &tm.tm_mday, &tm.tm_year) == 3) {
			if (ieq_command(device, ":GL#", response, sizeof(response)) && sscanf(response, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (ieq_command(device, ":GG#", response, sizeof(response))) {
					tm.tm_isdst = -1;
					tm.tm_gmtoff = atoi(response);
					time_t secs = mktime(&tm);
					indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
					sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", atoi(response));
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		}
	} else if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100) {
		if (ieq_command(device, ":GC#", response, sizeof(response)) && sscanf(response, "%02d/%02d/%02d", &tm.tm_mon, &tm.tm_mday, &tm.tm_year) == 3) {
			if (ieq_command(device, ":GL#", response, sizeof(response)) && sscanf(response, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (ieq_command(device, ":GG#", response, sizeof(response))) {
					tm.tm_isdst = -1;
					tm.tm_gmtoff = atoi(response) * 60;
					time_t secs = mktime(&tm);
					indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
					sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", atoi(response));
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol <= 0x0205) {
		int tz;
		if (ieq_command(device, ":GLT#", response, sizeof(response)) && sscanf(response, "%4d%1d%2d%2d%2d%2d%2d%2d", &tz, &tm.tm_isdst, &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 8) {
			tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
			tm.tm_mon -= 1;
			tm.tm_gmtoff = tz * 60;
			time_t secs = mktime(&tm);
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", tz / 60);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (PRIVATE_DATA->protocol >= 0x0300) {
		if (ieq_command(device, ":GUT#", response, sizeof(response))) {
			//bool dst = response[4] == 1;
			response[4] = 0;
			int tz = atoi(response);
			uint64_t jd = atoll(response + 5) / 8.64e+7 + JD2000;
			uint64_t secs = (jd - DELTA_UTC_UT1 - 2440587.5) * 86400.0;
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", tz / 60);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
}

static bool ieq_open(indigo_device *device) {
	char response[128] = "";
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "ieq")) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 9600);
		if (PRIVATE_DATA->handle >= 0) {
			if (!ieq_command(device, ":MountInfo#", response, sizeof(response)) || strlen(response) < 4 || strlen(response) > 5) {
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
			}
		}
	} else {
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 4030, &proto);
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (ieq_command(device, ":V#", response, sizeof(response))) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "version:  %s", response);
		}
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "iOptron");
		PRIVATE_DATA->protocol = 0x0000;
		PRIVATE_DATA->gotonova = false;
		if (ieq_command(device, ":MountInfo#", response, sizeof(response))) {
			PRIVATE_DATA->protocol = 0x0100;
			strncpy(PRIVATE_DATA->product, response, 64);
			if (!strcmp(PRIVATE_DATA->product, "8407")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ45 EQ/iEQ30");
			} else if (!strcmp(PRIVATE_DATA->product, "8497")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ45 Alt/Az");
			} else if (!strcmp(PRIVATE_DATA->product, "8408")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "ZEQ25");
			} else if (!strcmp(PRIVATE_DATA->product, "8498")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "SmartEQ");
			} else if (!strcmp(PRIVATE_DATA->product, "0010")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Cube II EQ");
			} else if (!strcmp(PRIVATE_DATA->product, "0011")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "SmartEQ Pro+");
			} else if (!strcmp(PRIVATE_DATA->product, "0025")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM25");
			} else if (!strcmp(PRIVATE_DATA->product, "0026")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM25-EC");
			} else if (!strcmp(PRIVATE_DATA->product, "0030")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ30 Pro");
			} else if (!strcmp(PRIVATE_DATA->product, "0040")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM40");
			} else if (!strcmp(PRIVATE_DATA->product, "0041")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM40-EC");
			} else if (!strcmp(PRIVATE_DATA->product, "0045")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ 45 Pro EQ");
			} else if (!strcmp(PRIVATE_DATA->product, "0046")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ 45 Pro Alt/Az");
			} else if (!strcmp(PRIVATE_DATA->product, "0060")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM60");
			} else if (!strcmp(PRIVATE_DATA->product, "0061")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM60-EC");
			} else if (!strcmp(PRIVATE_DATA->product, "0120")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM120");
			} else if (!strcmp(PRIVATE_DATA->product, "0121")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM120-EC");
			} else if (!strcmp(PRIVATE_DATA->product, "0122")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "CEM120-EC2");
			} else if (!strcmp(PRIVATE_DATA->product, "5010")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Cube II AA");
			} else if (!strcmp(PRIVATE_DATA->product, "5035")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "AZ Mount Pro");
			} else if (!strcmp(PRIVATE_DATA->product, "5045")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ45 Pro AA");
			} else if (strlen(PRIVATE_DATA->product) == 4) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unknown mount '%s'", response);
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Unknown mount");
			} else if (!strcmp(PRIVATE_DATA->product, ":EQ45")) {
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEQ45");
				PRIVATE_DATA->protocol = 0x0000;
			}
		} else {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "No MountInfo reply, GOTONOVA?", response);
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "GOTONOVA");
			PRIVATE_DATA->gotonova = true;
		}
		if (!PRIVATE_DATA->gotonova && PRIVATE_DATA->protocol != 0x0000) {
			if (ieq_command(device, ":FW1#", response, sizeof(response))) {
				PRIVATE_DATA->protocol = 0x0100;
				int product = atoi(PRIVATE_DATA->product);
				if (strncmp("140807", response, 6) <= 0 && (product == 60 || product == 61 || product == 45 || product == 46)) {
					PRIVATE_DATA->protocol = 0x0200;
				}
				if (strncmp("160610", response, 6) <= 0 && (product == 10  || product == 5010)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("161101", response, 6) <= 0 && (product == 30 || product == 45  || product == 60 || product == 61)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("181018", response, 6) <= 0 && (product == 40 || product == 41)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("170518", response, 6) <= 0 && (product == 26)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("170106", response, 6) <= 0 && (product == 25)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("161028", response, 6) <= 0 && (product == 11)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("170410", response, 6) <= 0 && (product == 5035)) {
					PRIVATE_DATA->protocol = 0x0205;
				}
				if (strncmp("171001", response, 6) <= 0 && (product == 120 || product == 121 || product == 122)) {
					PRIVATE_DATA->protocol = 0x0300;
				}
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware #1:  %s", response);
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
				if (ieq_command(device, ":FW2#", response, sizeof(response))) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware #2:  %s", response);
					strcat(MOUNT_INFO_FIRMWARE_ITEM->text.value, " / ");
					strcat(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
				}
			}
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Product:  %s (%s) protocol %d.%d", MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, PRIVATE_DATA->protocol >> 8, PRIVATE_DATA->protocol & 0xFF);
		if (PRIVATE_DATA->gotonova) {
			MOUNT_PARK_PROPERTY->hidden = true;
			MOUNT_TRACKING_PROPERTY->hidden = true;
			MOUNT_SLEW_RATE_PROPERTY->hidden = true;
			MOUNT_MOTION_RA_PROPERTY->hidden = true;
			MOUNT_MOTION_DEC_PROPERTY->hidden = true;
		} else if (PRIVATE_DATA->protocol == 0x0000) {
			MOUNT_PARK_PROPERTY->hidden = true;
			MOUNT_TRACKING_PROPERTY->hidden = true;
			MOUNT_SLEW_RATE_PROPERTY->hidden = true;
			MOUNT_MOTION_RA_PROPERTY->hidden = true;
			MOUNT_MOTION_DEC_PROPERTY->hidden = true;
			MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		} else if (PRIVATE_DATA->protocol == 0x0100) {
			if (ieq_command(device, ":AP#", response, 1)) {
				if (*response == '0') {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				} else {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				}
			}
			if (ieq_command(device, ":AT#", response, 1)) {
				if (*response == '0') {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				} else {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				}
			}
			if (ieq_command(device, ":QT#", response, 1)) {
				switch (*response) {
					case '0':
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			}
		} else if (PRIVATE_DATA->protocol == 0x0200) {
			if (ieq_command(device, ":GAS#", response, sizeof(response))) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				switch (response[1]) {
					case '1':
					case '5':
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						break;
					case '6':
						indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
						break;
				}
				switch (response[2]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			}
		} else if (PRIVATE_DATA->protocol >= 0x0205) {
			if (ieq_command(device, ":GLS#", response, sizeof(response))) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				switch (response[14]) {
					case '1':
					case '5':
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						break;
					case '6':
						indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
						break;
				}
				switch (response[15]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			}
		}
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void ieq_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	char response[128];
	if (PRIVATE_DATA->handle > 0) {
		ieq_get_coords(device);
		if (PRIVATE_DATA->gotonova) {
			if (ieq_command(device, ":SE?#", response, 1) && *response == '1') {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else if (PRIVATE_DATA->protocol == 0x0000) {
			if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - PRIVATE_DATA->currentRA) < 1 / 60. && fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - PRIVATE_DATA->currentDec) < 1 / 60.) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else if (PRIVATE_DATA->protocol == 0x0100) {
			if (ieq_command(device, ":AH#", response, 1) && *response == '1') {
				if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE && MOUNT_PARK_PARKED_ITEM->sw.value) {
					ieq_command(device, ":MP1#", response, 1);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked - please switch mount off");
				}
			} else if (ieq_command(device, ":SE?#", response, 1) && *response == '1') {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else if (PRIVATE_DATA->protocol == 0x0200) {
			if (ieq_command(device, ":GAS#", response, sizeof(response))) {
				switch (response[1]) {
					case '0': // stopped (not at zero position)
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '2': // slewing
					case '3': // guiding
					case '4': // meridian flipping
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						break;
					case '1': // tracking with PEC disabled
					case '5': // tracking with PEC enabled (only for non-encoder edition)
						if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '6': // parked
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
							indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
							MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked - please switch mount off");
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '7': // stopped at zero position
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
							MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_HOME_PROPERTY, "At home");
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
				}
				if (response[0] == '2') {
					bool update = MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
					if (ieq_command(device, ":Gt#", response, sizeof(response))) {
						double latitude = atol(response) / 60.0 / 60.0;
						if (latitude != MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) {
							MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
							update = true;
						}
					}
					if (ieq_command(device, ":Gg#", response, sizeof(response))) {
						double longitude = atol(response) / 60.0 / 60.0;
						if (longitude < 0)
							longitude += 360;
						if (longitude != MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) {
							MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
							update = true;
						}
					}
					if (update) {
						MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
				} else if (response[0] == '1') {
					if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
						MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
				}
			}
		} else if (PRIVATE_DATA->protocol >= 0x0205) {
			if (ieq_command(device, ":GLS#", response, sizeof(response))) {
				switch (response[14]) {
					case '0': // stopped (not at zero position)
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '2': // slewing
					case '3': // guiding
					case '4': // meridian flipping
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						break;
					case '1': // tracking with PEC disabled
					case '5': // tracking with PEC enabled (only for non-encoder edition)
						if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '6': // parked
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
							indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
							MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked - please switch mount off");
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
					case '7': // stopped at zero position
						if (MOUNT_TRACKING_ON_ITEM->sw.value) {
							indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
							MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						}
						if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
							MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, MOUNT_HOME_PROPERTY, "At home");
						}
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						break;
				}
				if (response[13] == '2') {
					char val[7];
					strncpy(val, response + 7, 6);
					bool update = MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
					double latitude = atol(val) / 60.0 / 60.0 - 90;
					if (latitude != MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) {
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
						update = true;
					}
					strncpy(val, response, 7);
						double longitude = atol(val) / 60.0 / 60.0;
					if (longitude < 0)
						longitude += 360;
					if (longitude != MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) {
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
						update = true;
					}
					if (update) {
						MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
				} else if (response[13] == '1') {
					if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
						MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
				}
			}
		}
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
		indigo_update_coordinates(device, NULL);
		ieq_get_utc(device);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.2 : 1, &PRIVATE_DATA->position_timer);
	}
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		SIMULATION_PROPERTY->hidden = true;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->count = 5;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
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
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = ieq_open(device);
			}
			if (result) {
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100) {
					if (ieq_command(device, ":Gt#", response, sizeof(response)))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
					if (ieq_command(device, ":Gg#", response, sizeof(response))) {
						double  longitude = indigo_stod(response);
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
				} else if (PRIVATE_DATA->protocol == 0x0200) {
					MOUNT_HOME_PROPERTY->hidden = false;
					MOUNT_PARK_SET_PROPERTY->hidden = false;
					MOUNT_PARK_SET_PROPERTY->count = 1;
					if (ieq_command(device, ":Gt#", response, sizeof(response)))
						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = atol(response) / 60.0 / 60.0;
					if (ieq_command(device, ":Gg#", response, sizeof(response))) {
						double longitude = atol(response) / 60.0 / 60.0;
						if (longitude < 0)
							longitude += 360;
						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
					}
				} else if (PRIVATE_DATA->protocol >= 0x0205) {
					MOUNT_HOME_PROPERTY->hidden = false;
					MOUNT_PARK_SET_PROPERTY->hidden = false;
					MOUNT_PARK_SET_PROPERTY->count = 1;
					if (ieq_command(device, ":GLS#", response, sizeof(response))) {
						if (response[12] == '2') {
							char val[7];
							strncpy(val, response + 6, 6);
							bool update = MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
							double latitude = atol(val) / 60.0 / 60.0 - 90;
							if (latitude != MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) {
								MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
								update = true;
							}
							strncpy(val, response, 7);
							double longitude = atol(val) / 60.0 / 60.0;
							if (longitude < 0)
								longitude += 360;
							if (longitude != MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) {
								MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
								update = true;
							}
							if (update) {
								MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
								indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
							}
						} else if (response[12] == '1') {
							if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
								MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
								indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
							}
						}
					}
				}
				ieq_get_coords(device);
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
				ieq_get_utc(device);
				PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			if (--PRIVATE_DATA->device_count == 0) {
				ieq_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(MOUNT_PARK_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		indigo_property_copy_values(MOUNT_PARK_SET_PROPERTY, property, false);
		if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value)
			ieq_command(device, ":SZP#", response, 1);
		MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			ieq_command(device, ":MP1#", response, 1);
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
		} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			ieq_command(device, ":MP0#", response, 1);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			if (strncmp(PRIVATE_DATA->product, "CEM60", 5)) {
				ieq_command(device, ":MH#", response, 1);
			} else {
				ieq_command(device, ":MSH#", response, 1);
			}
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
				sprintf(command, ":St%s#", indigo_dtos(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, "%+03d*%02d:%02.0f"));
			else if (PRIVATE_DATA->protocol <= 0x0205)
				sprintf(command, ":St%+07.0f#", MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value * 60 * 60);
			else if (PRIVATE_DATA->protocol >= 0x0300)
				sprintf(command, ":SLA%+09.0f#", MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value * 60 * 60 * 100);
			if (!ieq_command(device, command, response, 1) || *response != '1') {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				double longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
				if (longitude > 180)
					longitude -= 360;
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
					sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%+04d*%02d:%02.0f"));
				else if (PRIVATE_DATA->protocol <= 0x0205)
					sprintf(command, ":Sg%+07.0f#", longitude * 60 * 60);
				else if (PRIVATE_DATA->protocol >= 0x0300)
					sprintf(command, ":SLO%+09.0f#", longitude * 60 * 60 * 100);

				if (!ieq_command(device, command, response, 1) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				if (PRIVATE_DATA->gotonova) {
					if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '0') {
						ieq_command(device, ":STR0#", response, 1);
						PRIVATE_DATA->lastTrackRate = '0';
					} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '1') {
						ieq_command(device, ":STR1", response, 1);
						PRIVATE_DATA->lastTrackRate = '1';
					} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '2') {
						ieq_command(device, ":STR2#", response, 1);
						PRIVATE_DATA->lastTrackRate = '2';
					}
				} else {
					if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '0') {
						ieq_command(device, ":RT0#", response, 1);
						PRIVATE_DATA->lastTrackRate = '0';
					} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '1') {
						ieq_command(device, ":RT1", response, 1);
						PRIVATE_DATA->lastTrackRate = '1';
					} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '2') {
						ieq_command(device, ":RT2#", response, 1);
						PRIVATE_DATA->lastTrackRate = '2';
					} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '3') {
						ieq_command(device, ":RT3#", response, 1);
						PRIVATE_DATA->lastTrackRate = '3';
					} else if (MOUNT_TRACK_RATE_CUSTOM_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != '4') {
						ieq_command(device, ":RT4#", response, 1);
						PRIVATE_DATA->lastTrackRate = '4';
					}
					ieq_command(device, ":ST1#", response, 1);
				}
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
					sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				else if (PRIVATE_DATA->protocol <= 0x0205)
					sprintf(command, ":Sr%08.0f#", MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target * 60 * 60 * 1000);
				else if (PRIVATE_DATA->protocol >= 0x0300)
					sprintf(command, ":SRA%09.0f#", MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target * 15 * 60 * 60 * 100);
				if (!ieq_command(device, command, response, 1) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
						sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					else if (PRIVATE_DATA->protocol >= 0x0200)
						sprintf(command, ":Sd%+09.0f#", MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target * 60 * 60 * 100);
					if (!ieq_command(device, command, response, 1) || *response != '1') {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (PRIVATE_DATA->protocol <= 0x0205) {
							if (!ieq_command(device, ":MS#", response, 1) || *response != '1') {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS# failed");
								MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
							}
						} else if (PRIVATE_DATA->protocol >= 0x0300) {
							if (!ieq_command(device, ":MS1#", response, 1) || *response != '1') {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS1# failed");
								MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
							}
						}
					}
				}
			} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				char command[128], response[128];
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
					sprintf(command, ":Sr%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, "%02d:%02d:%02.0f"));
				else if (PRIVATE_DATA->protocol >= 0x0200)
					sprintf(command, ":Sr%08.0f#", MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target * 60 * 60 * 1000);
				if (!ieq_command(device, command, response, 1) || *response != '1') {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
						sprintf(command, ":Sd%s#", indigo_dtos(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, "%+03d*%02d:%02.0f"));
					else if (PRIVATE_DATA->protocol >= 0x0200)
						sprintf(command, ":Sd%+08.0f#", MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target * 60 * 60 * 100);
					if (!ieq_command(device, command, response, 1) || *response != '1') {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
						MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0000) {
							if (!ieq_command(device, ":CMR#", response, 1)) {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CMR# failed");
								MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
							}
						} else {
							if (!ieq_command(device, ":CM#", response, 1) || *response != '1') {
								INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed");
								MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
							}
						}
					}
				}
			}
			indigo_update_coordinates(device, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
				ieq_command(device, ":Q#", response, 1);
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
					ieq_command(device, ":q#", NULL, 0);
				else if (PRIVATE_DATA->protocol >= 0x0200)
					ieq_command(device, ":q#", response, 1);
				PRIVATE_DATA->lastMotionDec = 0;
				MOUNT_MOTION_NORTH_ITEM->sw.value = false;
				MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
				MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
				PRIVATE_DATA->lastMotionRA = 0;
				MOUNT_MOTION_WEST_ITEM->sw.value = false;
				MOUNT_MOTION_EAST_ITEM->sw.value = false;
				MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_coordinates(device, NULL);
				MOUNT_ABORT_MOTION_ITEM->sw.value = false;
				MOUNT_ABORT_MOTION_PROPERTY->state = *response == '1';
				indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '1') {
				ieq_command(device, ":SR1#", response, 1);
				PRIVATE_DATA->lastSlewRate = '1';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '3') {
				ieq_command(device, ":SR3#", response, 1);
				PRIVATE_DATA->lastSlewRate = '3';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '5') {
				ieq_command(device, ":SR5#", response, 1);
				PRIVATE_DATA->lastSlewRate = '5';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '9') {
				ieq_command(device, ":SR9#", response, 1);
				PRIVATE_DATA->lastSlewRate = '9';
			}
			if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
				if (PRIVATE_DATA->lastMotionDec != 'n') {
					ieq_command(device, ":mn#", NULL, 0);
					PRIVATE_DATA->lastMotionDec = 'n';
				}
			} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				if (PRIVATE_DATA->lastMotionDec != 's') {
					ieq_command(device, ":ms#", NULL, 0);
					PRIVATE_DATA->lastMotionDec = 's';
				}
			} else {
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
					ieq_command(device, ":q#", NULL, 0);
				else if (PRIVATE_DATA->protocol >= 0x0200)
					ieq_command(device, ":qD#", response, 1);
				PRIVATE_DATA->lastMotionDec = 0;
			}
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '1') {
				ieq_command(device, ":SR1#", response, 1);
				PRIVATE_DATA->lastSlewRate = '1';
			} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '3') {
				ieq_command(device, ":SR3#", response, 1);
				PRIVATE_DATA->lastSlewRate = '3';
			} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '5') {
				ieq_command(device, ":SR5#", response, 1);
				PRIVATE_DATA->lastSlewRate = '5';
			} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != '9') {
				ieq_command(device, ":SR9#", response, 1);
				PRIVATE_DATA->lastSlewRate = '9';
			}
			if (MOUNT_MOTION_WEST_ITEM->sw.value) {
				if (PRIVATE_DATA->lastMotionRA != 'w') {
					ieq_command(device, ":mw#", NULL, 0);
					PRIVATE_DATA->lastMotionRA = 'w';
				}
			} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
				if (PRIVATE_DATA->lastMotionRA != 'e') {
					ieq_command(device, ":me#", NULL, 0);
					PRIVATE_DATA->lastMotionRA = 'e';
				}
			} else {
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol == 0x0100)
					ieq_command(device, ":q#", NULL, 0);
				else if (PRIVATE_DATA->protocol >= 0x0200)
					ieq_command(device, ":qR#", response, 1);
				PRIVATE_DATA->lastMotionRA = 0;
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
			if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
				sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			else if (PRIVATE_DATA->protocol >= 0x0200)
				sprintf(command, ":SL%02d%02d%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!ieq_command(device, command, response, 1) || *response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
					sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
				else if (PRIVATE_DATA->protocol >= 0x0200)
					sprintf(command, ":SC%02d%02d%02d#", tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday);
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol < 0x0100)
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
				else if (!ieq_command(device, command, response, 1) || *response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					if (PRIVATE_DATA->gotonova)
						sprintf(command, ":SG%+03ld#", tm.tm_gmtoff / 60 / 60);
					else if (PRIVATE_DATA->protocol <= 0x0100)
						sprintf(command, ":SG%+03ld:00#", tm.tm_gmtoff / 60 / 60);
					else if (PRIVATE_DATA->protocol >= 0x0200)
						sprintf(command, ":SG%+04ld#", tm.tm_gmtoff / 60);
					if (!ieq_command(device, command, response, 1) || *response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						if (PRIVATE_DATA->gotonova) {
							MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
							MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
							indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
							indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
						} else {
							sprintf(command, ":SDS%d#", tm.tm_isdst);
							if (!ieq_command(device, command, response, 1) || *response != '1') {
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
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		} else {
			struct tm tm = *localtime(&secs);
			if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
				sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			else if (PRIVATE_DATA->protocol <= 0x0205)
				sprintf(command, ":SL%02d%02d%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			else if (PRIVATE_DATA->protocol >= 0x0300)
				sprintf(command, ":SUT%013llu", (uint64_t)((JD - JD2000) * 8.64e+7));
			if (!ieq_command(device, command, response, 1) || *response != '1') {
				MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (PRIVATE_DATA->gotonova || PRIVATE_DATA->protocol <= 0x0100)
					sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year - 100);
				else if (PRIVATE_DATA->protocol >= 0x0200)
					sprintf(command, ":SC%02d%02d%02d#", tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday);
				if (!ieq_command(device, command, response, 1) || *response != '1') {
					MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else if (PRIVATE_DATA->protocol <= 0x0205) {
					if (PRIVATE_DATA->gotonova)
						sprintf(command, ":SG%+03ld#", tm.tm_gmtoff / 60 / 60);
					else if (PRIVATE_DATA->protocol <= 0x0100)
						sprintf(command, ":SG%+03ld:00#", tm.tm_gmtoff / 60 / 60);
					else if (PRIVATE_DATA->protocol >= 0x0200)
						sprintf(command, ":SG%+04ld#", tm.tm_gmtoff / 60);
					if (!ieq_command(device, command, response, 1) || *response != '1') {
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
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			if (MOUNT_TRACKING_ON_ITEM->sw.value)
				ieq_command(device, ":ST1#", response, 1);
			else
				ieq_command(device, ":ST0#", response, 1);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
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

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void start_tracking(indigo_device *device) {
	char response[2];
	if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
		ieq_command(device, ":ST1#", response, 1);
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
	}
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
				result = ieq_open(device->master_device);
			}
			if (result) {
				if (ieq_command(device, ":AG#", response, sizeof(response)))
					GUIDER_RATE_ITEM->number.value = atoi(response);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				ieq_close(device->master_device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		start_tracking(device->master_device);
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			sprintf(command, ":Mn%05d#", (int)GUIDER_GUIDE_NORTH_ITEM->number.value);
			ieq_command(device, command, NULL, 0);
		} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			sprintf(command, ":Ms%05d#", (int)GUIDER_GUIDE_SOUTH_ITEM->number.value);
			ieq_command(device, command, NULL, 0);
		}
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		start_tracking(device->master_device);
		if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			sprintf(command, ":Mw%05d#", (int)GUIDER_GUIDE_WEST_ITEM->number.value);
			ieq_command(device, command, NULL, 0);
		} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			sprintf(command, ":Me%05d#", (int)GUIDER_GUIDE_EAST_ITEM->number.value);
			ieq_command(device, command, NULL, 0);
		}
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- GUIDER_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		sprintf(command, ":RG%03d#", (int)GUIDER_RATE_ITEM->number.value);
		ieq_command(device, command, response, 1);
		GUIDER_RATE_PROPERTY->state = *response == '1';
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
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

static ioptron_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_ioptron(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_IOPTRON_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_IOPTRON_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "iOptron Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);


	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(ioptron_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(ioptron_private_data));
			mount = malloc(sizeof(indigo_device));
			assert(mount != NULL);
			memcpy(mount, &mount_template, sizeof(indigo_device));
			mount->private_data = private_data;
			indigo_attach_device(mount);
			mount_guider = malloc(sizeof(indigo_device));
			assert(mount_guider != NULL);
			memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
			mount_guider->private_data = private_data;
			mount_guider->master_device = mount;
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
