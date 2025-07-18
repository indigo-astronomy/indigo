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

#define DRIVER_VERSION 0x002F
#define DRIVER_NAME	"indigo_mount_lx200"

#define NYX_BASE64_THRESHOLD_VERSION "1.32.0"

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
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_base64.h>

#include "indigo_mount_lx200.h"

#define PRIVATE_DATA				((lx200_private_data *)device->private_data)

#define MOUNT_MODE_PROPERTY							(PRIVATE_DATA->alignment_mode_property)
#define EQUATORIAL_ITEM									(MOUNT_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM									(MOUNT_MODE_PROPERTY->items+1)

#define MOUNT_MODE_PROPERTY_NAME				"X_MOUNT_MODE"
#define EQUATORIAL_ITEM_NAME						"EQUATORIAL"
#define ALTAZ_MODE_ITEM_NAME						"ALTAZ"

#define FORCE_FLIP_PROPERTY							(PRIVATE_DATA->force_flip_property)
#define FORCE_FLIP_ENABLED_ITEM					(FORCE_FLIP_PROPERTY->items+0)
#define FORCE_FLIP_DISABLED_ITEM				(FORCE_FLIP_PROPERTY->items+1)

#define FORCE_FLIP_PROPERTY_NAME				"X_FORCE_FLIP"
#define FORCE_FLIP_ENABLED_ITEM_NAME		"ENABLED"
#define FORCE_FLIP_DISABLED_ITEM_NAME		"DISABLED"

#define MOUNT_TYPE_PROPERTY							(PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_DETECT_ITEM					(MOUNT_TYPE_PROPERTY->items+0)
#define MOUNT_TYPE_MEADE_ITEM						(MOUNT_TYPE_PROPERTY->items+1)
#define MOUNT_TYPE_EQMAC_ITEM						(MOUNT_TYPE_PROPERTY->items+2)
#define MOUNT_TYPE_10MICRONS_ITEM				(MOUNT_TYPE_PROPERTY->items+3)
#define MOUNT_TYPE_GEMINI_ITEM				 	(MOUNT_TYPE_PROPERTY->items+4)
#define MOUNT_TYPE_STARGO_ITEM					(MOUNT_TYPE_PROPERTY->items+5)
#define MOUNT_TYPE_STARGO2_ITEM					(MOUNT_TYPE_PROPERTY->items+6)
#define MOUNT_TYPE_AP_ITEM							(MOUNT_TYPE_PROPERTY->items+7)
#define MOUNT_TYPE_ON_STEP_ITEM					(MOUNT_TYPE_PROPERTY->items+8)
#define MOUNT_TYPE_AGOTINO_ITEM					(MOUNT_TYPE_PROPERTY->items+9)
#define MOUNT_TYPE_ZWO_ITEM				 			(MOUNT_TYPE_PROPERTY->items+10)
#define MOUNT_TYPE_NYX_ITEM				 			(MOUNT_TYPE_PROPERTY->items+11)
#define MOUNT_TYPE_OAT_ITEM				 			(MOUNT_TYPE_PROPERTY->items+12)
#define MOUNT_TYPE_TEEN_ASTRO_ITEM				 	(MOUNT_TYPE_PROPERTY->items+13)
#define MOUNT_TYPE_GENERIC_ITEM         (MOUNT_TYPE_PROPERTY->items+14)


#define MOUNT_TYPE_PROPERTY_NAME				"X_MOUNT_TYPE"
#define MOUNT_TYPE_DETECT_ITEM_NAME			"DETECT"
#define MOUNT_TYPE_MEADE_ITEM_NAME			"MEADE"
#define MOUNT_TYPE_EQMAC_ITEM_NAME			"EQMAC"
#define MOUNT_TYPE_10MICRONS_ITEM_NAME	"10MIC"
#define MOUNT_TYPE_GEMINI_ITEM_NAME			"GEMINI"
#define MOUNT_TYPE_STARGO_ITEM_NAME			"STARGO"
#define MOUNT_TYPE_STARGO2_ITEM_NAME		"STARGO2"
#define MOUNT_TYPE_AP_ITEM_NAME					"AP"
#define MOUNT_TYPE_ON_STEP_ITEM_NAME		"ONSTEP"
#define MOUNT_TYPE_AGOTINO_ITEM_NAME		"AGOTINO"
#define MOUNT_TYPE_ZWO_ITEM_NAME				"ZWO_AM"
#define MOUNT_TYPE_NYX_ITEM_NAME				"NYX"
#define MOUNT_TYPE_OAT_ITEM_NAME				"OAT"
#define MOUNT_TYPE_TEEN_ASTRO_ITEM_NAME	    "TEEN_ASTRO"
#define MOUNT_TYPE_GENERIC_ITEM_NAME		"GENERIC"

#define ZWO_BUZZER_PROPERTY				(PRIVATE_DATA->zwo_buzzer_property)
#define ZWO_BUZZER_OFF_ITEM				(ZWO_BUZZER_PROPERTY->items+0)
#define ZWO_BUZZER_LOW_ITEM				(ZWO_BUZZER_PROPERTY->items+1)
#define ZWO_BUZZER_HIGH_ITEM			(ZWO_BUZZER_PROPERTY->items+2)

#define ZWO_BUZZER_PROPERTY_NAME		"X_ZWO_BUZZER"
#define ZWO_BUZZER_OFF_ITEM_NAME		"OFF"
#define ZWO_BUZZER_LOW_ITEM_NAME		"LOW"
#define ZWO_BUZZER_HIGH_ITEM_NAME		"HIGH"

#define NYX_WIFI_AP_PROPERTY				(PRIVATE_DATA->nyx_wifi_ap_property)
#define NYX_WIFI_AP_SSID_ITEM				(NYX_WIFI_AP_PROPERTY->items+0)
#define NYX_WIFI_AP_PASSWORD_ITEM		(NYX_WIFI_AP_PROPERTY->items+1)

#define NYX_WIFI_CL_PROPERTY				(PRIVATE_DATA->nyx_wifi_cl_property)
#define NYX_WIFI_CL_SSID_ITEM				(NYX_WIFI_CL_PROPERTY->items+0)
#define NYX_WIFI_CL_PASSWORD_ITEM		(NYX_WIFI_CL_PROPERTY->items+1)

#define NYX_WIFI_RESET_PROPERTY			(PRIVATE_DATA->nyx_wifi_reset_property)
#define NYX_WIFI_RESET_ITEM					(NYX_WIFI_RESET_PROPERTY->items+0)

#define NYX_LEVELER_PROPERTY				(PRIVATE_DATA->nyx_leveler_property)
#define NYX_LEVELER_PICH_ITEM				(NYX_LEVELER_PROPERTY->items+0)
#define NYX_LEVELER_ROLL_ITEM				(NYX_LEVELER_PROPERTY->items+1)
#define NYX_LEVELER_COMPASS_ITEM		(NYX_LEVELER_PROPERTY->items+2)

#define NYX_WIFI_AP_PROPERTY_NAME				"X_NYX_WIFI_AP"
#define NYX_WIFI_AP_SSID_ITEM_NAME			"AP_SSID"
#define NYX_WIFI_AP_PASSWORD_ITEM_NAME	"AP_PASSWORD"

#define NYX_WIFI_CL_PROPERTY_NAME				"X_NYX_WIFI_CL"
#define NYX_WIFI_CL_SSID_ITEM_NAME			"CL_SSID"
#define NYX_WIFI_CL_PASSWORD_ITEM_NAME	"CL_PASSWORD"

#define NYX_WIFI_RESET_PROPERTY_NAME		"X_NYX_WIFI_RESET"
#define NYX_WIFI_RESET_ITEM_NAME				"RESET"

#define NYX_LEVELER_PROPERTY_NAME				"X_NYX_LEVELER"
#define NYX_LEVELER_PICH_ITEM_NAME			"PICH"
#define NYX_LEVELER_ROLL_ITEM_NAME			"ROLL"
#define NYX_LEVELER_COMPASS_ITEM_NAME		"COMPASS"

#define AUX_WEATHER_PROPERTY            (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM    (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_PRESSURE_ITEM				(AUX_WEATHER_PROPERTY->items + 1)

#define AUX_INFO_PROPERTY								(PRIVATE_DATA->info_property)
#define AUX_INFO_VOLTAGE_ITEM						(AUX_INFO_PROPERTY->items + 0)
// Onstep has eight auxiliary device slots (1-indexed) which can have user defined purposes
#define ONSTEP_AUX_DEVICE_COUNT					8
#define AUX_GROUP												"Auxiliary Functions"
#define AUX_POWER_OUTLET_PROPERTY				(PRIVATE_DATA->power_outlet_property)
#define ONSTEP_AUX_HEATER_OUTLET_MAPPING (PRIVATE_DATA->onstep_aux_power_outlet_slot_mapping)
#define ONSTEP_AUX_HEATER_OUTLET_COUNT 	(PRIVATE_DATA->onstep_aux_heater_outlet_count)
#define AUX_HEATER_OUTLET_PROPERTY			(PRIVATE_DATA->heater_outlet_property)
#define ONSTEP_AUX_POWER_OUTLET_COUNT		(PRIVATE_DATA->onstep_aux_power_outlet_count)
#define ONSTEP_AUX_POWER_OUTLET_MAPPING	(PRIVATE_DATA->onstep_aux_heater_outlet_slot_mapping)

#define IS_PARKED (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PROPERTY->count == 2 && MOUNT_PARK_PARKED_ITEM->sw.value)

typedef enum {
	ONSTEP_AUX_NONE = 0, // Auxiliary slot is disabled
	ONSTEP_AUX_SWITCH = 1, // Auxiliary slot is a on/off switch -> power outlet in indigo
	ONSTEP_AUX_ANALOG = 2 // Auxiliary slot is an analog / pwm output -> heater outlet in indigo
	//TODO implement Momentary Switch, Dew Heater and Intervalometer
} onstep_aux_device_purpose;


typedef struct {
	int handle;
	int device_count;
	bool is_network;
	bool wifi_reset;
	indigo_timer *position_timer;
	indigo_timer *keep_alive_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	bool motioned;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
	indigo_property *force_flip_property;
	indigo_property *mount_type_property;
	indigo_property *zwo_buzzer_property;
	indigo_property *nyx_wifi_ap_property;
	indigo_property *nyx_wifi_cl_property;
	indigo_property *nyx_wifi_reset_property;
	indigo_property *nyx_leveler_property;
	indigo_property *weather_property;
	indigo_property *info_property;
	indigo_property *power_outlet_property;
	indigo_property *heater_outlet_property;
	indigo_timer *focuser_timer;
	indigo_timer *aux_timer;

	char prev_state[30];
	bool is_site_set;
	bool use_dst_commands;
	bool park_changed;
	bool home_changed;
	bool tracking_changed;
	bool tracking_rate_changed;
	bool focus_aborted;
	int prev_tracking_rate;
	bool prev_home_state;
	// maps power outlet property item index to onstep aux slot
	int onstep_aux_power_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];
	// the number of heater outlets defined in onstep
	int onstep_aux_heater_outlet_count;
	// maps heater outlet property item index to onstep aux slot
	int onstep_aux_heater_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];
	// the number of power outlets defined in onstep
	int onstep_aux_power_outlet_count;
} lx200_private_data;

/**
 * Compare two version strings in the format "major.minor.patch"
 * Returns: -1 if version1 < version2, 0 if equal, 1 if version1 > version2
 */
static int compare_versions(const char *version1, const char *version2) {
	if (!version1 || !version2) {
		return 0;
	}

	char *v1_copy = strdup(version1);
	char *v2_copy = strdup(version2);

	if (!v1_copy || !v2_copy) {
		indigo_safe_free(v1_copy);
		indigo_safe_free(v2_copy);
		return 0;
	}

	int v1_major = 0, v1_minor = 0, v1_patch = 0;
	char *token = strtok(v1_copy, ".");
	if (token) v1_major = atoi(token);
	token = strtok(NULL, ".");
	if (token) v1_minor = atoi(token);
	token = strtok(NULL, ".");
	if (token) v1_patch = atoi(token);

	indigo_safe_free(v1_copy);

	token = strtok(v2_copy, ".");
	int v2_major = 0, v2_minor = 0, v2_patch = 0;
	if (token) v2_major = atoi(token);
	token = strtok(NULL, ".");
	if (token) v2_minor = atoi(token);
	token = strtok(NULL, ".");
	if (token) v2_patch = atoi(token);

	indigo_safe_free(v2_copy);

	if (v1_major != v2_major) {
		return (v1_major > v2_major) ? 1 : -1;
	}
	if (v1_minor != v2_minor) {
		return (v1_minor > v2_minor) ? 1 : -1;
	}
	if (v1_patch != v2_patch) {
		return (v1_patch > v2_patch) ? 1 : -1;
	}

	return 0;
}

static char *meade_error_string(indigo_device *device, unsigned int code) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Prameters out of range",
			"Format error",
			"Mount not initialized",
			"Mount is Moving",
			"Target is below horizon",
			"Target is beow the altitude limit",
			"Time and location is not set",
			"Unkonwn error"
		};
		if (code > 8) return NULL;
		return (char *)error_string[code];
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Below the horizon limit",
			"Above overhead limit",
			"Controller in standby",
			"Mount is parked",
			"Slew in progress",
			"Outside limits",
			"Hardware fault",
			"Already in motion",
			"Unspecified error"
		};
		if (code > 9) return NULL;
		return (char *)error_string[code];
	} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		const char *error_string[] = {
			NULL,
			"Below the horizon limit",
			"No object selected",
			"Same side",
			"Mount is parked",
			"Slew in progress",
			"Outside limits",
			"Guide in progress",
			"Above overhead limit",
			"Hardware fault"
			"Unspecified error"
		};
		if (code > 9) return NULL;
		return (char *)error_string[code];
	}
	return NULL;
}

static void str_replace(char *string, char c0, char c1) {
	char *cp = strchr(string, c0);
	if (cp)
		*cp = c1;
}

static bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool meade_open(indigo_device *device) {
	char response[128] = "";
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "lx200")) {
		PRIVATE_DATA->is_network = false;
		if (MOUNT_TYPE_NYX_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value){
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
		} else {
			PRIVATE_DATA->handle = indigo_open_serial(name);
			if (PRIVATE_DATA->handle > 0) {
				// sometimes the first command after power on in OnStep fails and just returns '0'
				// so we try two times for the default baudrate of 9600
				if ((!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) < 6) &&
				    (!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) < 6)) {
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 19200);
					if (!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) < 6) {
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
						if (!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) < 6) {
							close(PRIVATE_DATA->handle);
							PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 230400);
							if (!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) < 6) {
								close(PRIVATE_DATA->handle);
								PRIVATE_DATA->handle = -1;
							}
						}
					}
				}
			}
		}
	} else {
		PRIVATE_DATA->is_network = true;
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
		if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value)
			PRIVATE_DATA->handle = indigo_open_network_device(name, 9999, &proto);
		else
			PRIVATE_DATA->handle = indigo_open_network_device(name, 4030, &proto);
	}
	if (PRIVATE_DATA->handle >= 0) {
		if (PRIVATE_DATA->is_network) {
			int opt = 1;
			if (setsockopt(PRIVATE_DATA->handle, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(int)) < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to disable Nagle algorithm");
			}
		}
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
			if (result == 0) {
				break;
			}
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
		PRIVATE_DATA->wifi_reset = false;
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void network_disconnection(__attribute__((unused)) indigo_device *device);

static bool meade_command(indigo_device *device, char *command, char *response, int max, int sleep) {
	if (PRIVATE_DATA->handle == 0 || PRIVATE_DATA->wifi_reset) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	// flush, and detect network disconnection
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		if (PRIVATE_DATA->is_network) {
			tv.tv_usec = 50;
		} else {
			tv.tv_usec = 5000;
		}

		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0) {
			break;
		}
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			if (PRIVATE_DATA->is_network) {
				// This is a disconnection
				indigo_set_timer(device, 0, network_disconnection, NULL);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected disconnection from %s", DEVICE_PORT_ITEM->text.value);
			}
			return false;
		}
	}
	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (sleep > 0) {
		indigo_usleep(sleep);
	}
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
			if (result <= 0) {
				break;
			}
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			if (c == '#') {
				break;
			}
			response[index++] = c;
		}
		response[index] = 0;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	indigo_usleep(50000);
	return true;
}

static bool meade_command_progress(indigo_device *device, char *command, char *response, int max, int sleep) {
	if (PRIVATE_DATA->handle == 0 || PRIVATE_DATA->wifi_reset) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	// flush, and detect network disconnection
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0) {
			break;
		}
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			if (PRIVATE_DATA->is_network) {
				// This is a disconnection
				indigo_set_timer(device, 0, network_disconnection, NULL);
				INDIGO_DRIVER_LOG (DRIVER_NAME, "Disconnection from %s", DEVICE_PORT_ITEM->text.value);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	// write command
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (sleep > 0) {
		indigo_usleep(sleep);
	}
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
			if (result <= 0) {
				break;
			}
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			if (c == '#') {
				break;
			}
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
		if (result <= 0) {
			break;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		if (c < 0)
			c = ':';
		if (c == '#') {
			break;
		}
		progress[index++] = c;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Progress width: %d", index);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static bool gemini_set(indigo_device *device, int command, char *parameter) {
	char buffer[128];
	char *end = buffer + sprintf(buffer, ">%d:%s", command, parameter);
	uint8_t checksum = buffer[0];
	for (size_t i = 1; i < strlen(buffer); i++)
		checksum = checksum ^ buffer[i];
	checksum = checksum % 128 + 64;
	*end++ = checksum;
	*end++ = '#';
	*end++ = 0;
	return meade_command(device, buffer, NULL, 0, 0);
}

static void meade_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// ---------------------------------------------------------------------  mount commands

static bool meade_set_utc(indigo_device *device, time_t *secs, int utc_offset) {
	char command[128], response[128];
	time_t seconds = *secs + utc_offset * 3600;
	struct tm tm;
	gmtime_r(&seconds, &tm);
	sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
	// :SCMM/DD/YY# returns two delimiters response:
	// "1Updating Planetary Data#                                #"
	// readout progress part
	bool result;
	if (
		MOUNT_TYPE_ON_STEP_ITEM->sw.value ||
		MOUNT_TYPE_ZWO_ITEM->sw.value ||
		MOUNT_TYPE_STARGO2_ITEM->sw.value ||
		MOUNT_TYPE_NYX_ITEM->sw.value ||
		MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value
	) {
		result = meade_command(device, command, response, 1, 0);
	} else {
		result = meade_command_progress(device, command, response, sizeof(response), 0);
	}
	if (!result || *response != '1') {
		return false;
	} else {
		if (PRIVATE_DATA->use_dst_commands) {
			sprintf(command, ":SH%d#", indigo_get_dst_state());
			meade_command(device, command, NULL, 0, 0);
		}
		sprintf(command, ":SG%+03d#", -utc_offset);
		if (!meade_command(device, command, response, 1, 0) || *response != '1') {
			return false;
		} else {
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!meade_command(device, command, response, 1, 0) || *response != '1') {
				return false;
			} else {
				return true;
			}
		}
	}
}

static bool meade_get_utc(indigo_device *device, time_t *secs, int *utc_offset) {
	if (
		MOUNT_TYPE_MEADE_ITEM->sw.value ||
		MOUNT_TYPE_GEMINI_ITEM->sw.value ||
		MOUNT_TYPE_10MICRONS_ITEM->sw.value ||
		MOUNT_TYPE_AP_ITEM->sw.value ||
		MOUNT_TYPE_ZWO_ITEM->sw.value ||
		MOUNT_TYPE_NYX_ITEM->sw.value ||
		MOUNT_TYPE_OAT_ITEM->sw.value ||
		MOUNT_TYPE_ON_STEP_ITEM->sw.value ||
		MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value ||
		MOUNT_TYPE_GENERIC_ITEM->sw.value
	) {
		struct tm tm;
		char response[128];
		memset(&tm, 0, sizeof(tm));
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
					*utc_offset = -atoi(response);
					*secs = timegm(&tm) - *utc_offset * 3600;
					return true;
				}
			}
		}
		return false;
	}
	return true;
}

static void meade_get_site(indigo_device *device, double *latitude, double *longitude) {
	char response[128];
	if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
		return;
	}
	if (meade_command(device, ":Gt#", response, sizeof(response), 0)) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value)
			str_replace(response, 't', '*');
		*latitude = indigo_stod(response);
	}
	if (meade_command(device, ":Gg#", response, sizeof(response), 0)) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value)
			str_replace(response, 'g', '*');
		*longitude = indigo_stod(response);
		if (*longitude < 0)
			*longitude += 360;
		// LX200 protocol returns negative longitude for the east
		*longitude = 360 - *longitude;
	}
}

static bool meade_set_site(indigo_device *device, double latitude, double longitude, double elevation) {
	char command[128], response[128];
	bool result = true;
	if (MOUNT_TYPE_AGOTINO_ITEM->sw.value)
		return false;
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		sprintf(command, ":St%s#", indigo_dtos(latitude, "%+03d*%02d:%02d"));
	else
		sprintf(command, ":St%s#", indigo_dtos(latitude, "%+03d*%02d"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
		result = MOUNT_TYPE_STARGO_ITEM->sw.value; // ignore result for Avalon StarGO
	}
	// LX200 protocol expects negative longitude for the east
	longitude = 360 - fmod(longitude + 360, 360);
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%+04d*%02d:%02d"));
	else
		sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%03d*%02d"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
		result = MOUNT_TYPE_STARGO_ITEM->sw.value; // ignore result for Avalon StarGO
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		sprintf(command, ":Sv%.1f#", elevation);
		if (!meade_command(device, command, response, 1, 0) || *response != '1') {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
			result = false;
		}
	}
	PRIVATE_DATA->is_site_set = result;
	return result;
}

static bool meade_get_coordinates(indigo_device *device, double *ra, double *dec) {
	char response[128];
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (meade_command(device, ":GRH#", response, sizeof(response), 0)) {
			*ra = indigo_stod(response);
			if (meade_command(device, ":GDH#", response, sizeof(response), 0)) {
				*dec = indigo_stod(response);
				return true;
			}
		}
	} else if (meade_command(device, ":GR#", response, sizeof(response), 0)) {
		if (strlen(response) < 8) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
				meade_command(device, ":P#", response, sizeof(response), 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				meade_command(device, ":U1#", NULL, 0, 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			} else if (
				MOUNT_TYPE_GEMINI_ITEM->sw.value ||
				MOUNT_TYPE_AP_ITEM->sw.value ||
				MOUNT_TYPE_ON_STEP_ITEM->sw.value ||
				MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value
			) {
				meade_command(device, ":U#", NULL, 0, 0);
				meade_command(device, ":GR#", response, sizeof(response), 0);
			}
		}
		*ra = indigo_stod(response);
		if (meade_command(device, ":GD#", response, sizeof(response), 0)) {
			*dec = indigo_stod(response);
			return true;
		}
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on);

static bool meade_slew(indigo_device *device, double ra, double dec) {
	char command[128], response[128];
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
			meade_set_tracking(device, true);
		}
	}
	sprintf(command, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		return false;
	}
	sprintf(command, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		return false;
	}
	if (!meade_command(device, ":MS#", response, 1, 100000) || *response != '0') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS# failed with response: %s", response);
		if (MOUNT_TYPE_ZWO_ITEM->sw.value && *response == 'e') {
			int error_code = 0;
			sscanf(response, "e%d", &error_code);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, "Error: %s", message);
			}
		}
		if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			int error_code = atoi(response);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, "Error: %s", message);
			}
		}
		if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			int error_code = atoi(response);
			char *message = "Location not set, please set site coordinates";
			if (PRIVATE_DATA->is_site_set || error_code != 6) {
				message = meade_error_string(device, error_code);
			}
			if (message) {
				indigo_send_message(device, "Error: %s", message);
			}
		}
		return false;
	}
	return true;
}

static bool meade_sync(indigo_device *device, double ra, double dec) {
	char command[128], response[128];
	sprintf(command, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		return false;
	}
	sprintf(command, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f"));
	if (!meade_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		return false;
	}
	if (!meade_command(device, ":CM#", response, sizeof(response), 100000) || *response == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed with response: %s", response);
		return false;
	}
	if (MOUNT_TYPE_ZWO_ITEM->sw.value && *response == 'e') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed with response: %s", response);
		int error_code = 0;
		sscanf(response, "e%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, "Error: %s", message);
		}
		return false;
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value && *response == 'E') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed with response: %s", response);
		int error_code = 0;
		sscanf(response, "E%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, "Error: %s", message);
		}
		return false;
	}
	return true;
}

static bool meade_force_flip(indigo_device *device, bool on) {
	char response[128];
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		return meade_command(device, on ? ":TTSFd#" : ":TTSFs#", response, 1, 0);
	return false;
}

static bool meade_pec(indigo_device *device, bool on) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value)
		return meade_command(device, on ? "$QZ+" : "$QZ-", NULL, 0, 0);
	return false;
}

static bool meade_set_guide_rate(indigo_device *device, int ra, int dec) {
	char command[128];
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		sprintf(command, ":X20%02d#", ra);
		if (meade_command(device, command, NULL, 0, 0)) {
			sprintf(command, ":X21%02d#", dec);
			return meade_command(device, command, NULL, 0, 0);
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		// asi miunt has one guide rate for ra and dec
		if (ra < 10) ra = 10;
		if (ra > 90) ra = 90;
		float rate = ra / 100.0;
		sprintf(command, ":Rg%.1f#", rate);
		return (meade_command(device, command, NULL, 0, 0));
	}
	return false;
}

static bool meade_get_guide_rate(indigo_device *device, int *ra, int *dec) {
	char response[128] = {0};
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		bool res = meade_command(device, ":Ggr#", response, sizeof(response), 0);
		if (!res) return false;
		float rate = 0;
		int parsed = sscanf(response, "%f", &rate);
		if (parsed !=1) return false;
		*ra = *dec = rate * 100;
		return true;
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on) {
	char response[128] = {0};
	if (on) { // TBD
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 192, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_command(device, ":X122#", NULL, 0, 0);
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return meade_command(device, ":RT2#", NULL, 0, 0);
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_command(device, ":RT1#", NULL, 0, 0);
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_command(device, ":RT0#", NULL, 0, 0);
			}
		} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_command(device, ":Te#", response, sizeof(response), 0) && *response == '1';
		} else if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return meade_command(device, ":TQ#:Te#", response, sizeof(response), 0) && *response == '1';
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_command(device, ":TS#:Te#", response, sizeof(response), 0) && *response == '1';
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_command(device, ":TL#:Te#", response, sizeof(response), 0) && *response == '1';
			} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
				return meade_command(device, ":TK#:Te#", response, sizeof(response), 0) && *response == '1';
			}
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT1#", response, sizeof(response), 0) && *response == '1';
		} else {
			return meade_command(device, ":AP#", NULL, 0, 0);
		}
	} else {
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 191, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_command(device, ":X120#", NULL, 0, 0);
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_command(device, ":RT9#", NULL, 0, 0);
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_command(device, ":Td#", NULL, 0, 0);
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT0#", response, sizeof(response), 0) && *response == '1';
		} else {
			return meade_command(device, ":AL#", NULL, 0, 0);
		}
	}
	return false;
}

static bool meade_set_tracking_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
		PRIVATE_DATA->lastTrackRate = 'q';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
			return gemini_set(device, 131, "");
		else if (MOUNT_TYPE_AP_ITEM->sw.value)
			return meade_command(device, ":RT2#", NULL, 0, 0);
		else if (MOUNT_TYPE_OAT_ITEM->sw.value)
			return meade_command(device, ":XSS1.000#", NULL, 0, 0);
		else
			return meade_command(device, ":TQ#", NULL, 0, 0);
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
		PRIVATE_DATA->lastTrackRate = 's';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
			return gemini_set(device, 134, "");
		else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value)
			return meade_command(device, ":TSOLAR#", NULL, 0, 0);
		else if (MOUNT_TYPE_AP_ITEM->sw.value)
			return meade_command(device, ":RT1#", NULL, 0, 0);
		else if (MOUNT_TYPE_OAT_ITEM->sw.value)
			return meade_command(device, ":XSS0.997#", NULL, 0, 0);
		else
			return meade_command(device, ":TS#", NULL, 0, 0);
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
		PRIVATE_DATA->lastTrackRate = 'l';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
			return gemini_set(device, 133, "");
		else if (MOUNT_TYPE_AP_ITEM->sw.value)
			return meade_command(device, ":RT0#", NULL, 0, 0);
		else if (MOUNT_TYPE_OAT_ITEM->sw.value)
			return meade_command(device, ":XSS0.965#", NULL, 0, 0);
		else
			return meade_command(device, ":TL#", NULL, 0, 0);
	} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'k') {
		PRIVATE_DATA->lastTrackRate = 'k';
		if (MOUNT_TYPE_NYX_ITEM->sw.value)
			return meade_command(device, ":TK#", NULL, 0, 0);
	}
	return true;
}

static bool meade_set_slew_rate(indigo_device *device) {
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_command(device, ":RG2#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_command(device, ":RC0#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_command(device, ":RC1#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_command(device, ":RC3#", NULL, 0, 0);
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_command(device, ":R1#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_command(device, ":R4#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_command(device, ":R7#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_command(device, ":R9#", NULL, 0, 0);
		}
	} else {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_command(device, ":RG#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_command(device, ":RC#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_command(device, ":RM#", NULL, 0, 0);
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_command(device, ":RS#", NULL, 0, 0);
		}
	}
	return true;
}

static bool meade_motion_dec(indigo_device *device) {
	bool stopped = true;
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (PRIVATE_DATA->lastMotionNS == 'n' || PRIVATE_DATA->lastMotionNS == 's')
			stopped = meade_command(device, ":Q#", NULL, 0, 0);
	} else {
		if (PRIVATE_DATA->lastMotionNS == 'n')
			stopped = meade_command(device, ":Qn#", NULL, 0, 0);
		else if (PRIVATE_DATA->lastMotionNS == 's')
			stopped = meade_command(device, ":Qs#", NULL, 0, 0);
	}
	if (stopped) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 'n';
			return meade_command(device, ":Mn#", NULL, 0, 0);
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 's';
			return meade_command(device, ":Ms#", NULL, 0, 0);
		} else {
			PRIVATE_DATA->lastMotionNS = 0;
		}
	}
	return stopped;
}

static bool meade_motion_ra(indigo_device *device) {
	bool stopped = true;
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (PRIVATE_DATA->lastMotionWE == 'w' || PRIVATE_DATA->lastMotionWE == 'e')
			stopped = meade_command(device, ":Q#", NULL, 0, 0);
	} else {
		if (PRIVATE_DATA->lastMotionWE == 'w')
			stopped = meade_command(device, ":Qw#", NULL, 0, 0);
		else if (PRIVATE_DATA->lastMotionWE == 'e')
			stopped = meade_command(device, ":Qe#", NULL, 0, 0);
	}
	if (stopped) {
		if (MOUNT_MOTION_WEST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'w';
			return meade_command(device, ":Mw#", NULL, 0, 0);
		} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'e';
			return meade_command(device, ":Me#", NULL, 0, 0);
		} else {
			PRIVATE_DATA->lastMotionWE = 0;
		}
	}
	return stopped;
}

static bool meade_park(indigo_device *device) {
	char response[128];
	if (
		MOUNT_TYPE_MEADE_ITEM->sw.value ||
		MOUNT_TYPE_EQMAC_ITEM->sw.value ||
		MOUNT_TYPE_ON_STEP_ITEM->sw.value ||
		MOUNT_TYPE_NYX_ITEM->sw.value ||
		MOUNT_TYPE_OAT_ITEM->sw.value ||
		MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value
	) {
		return meade_command(device, ":hP#", NULL, 0, 0);
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value)
		return meade_command(device, ":KA#", NULL, 0, 0);
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
		return meade_command(device, ":hC#", NULL, 0, 0);
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		return meade_command(device, ":X362#", response, sizeof(response), 0) && strcmp(response, "pB") == 0;
	return false;
}

static bool meade_unpark(indigo_device *device) {
	char response[128];
	if (MOUNT_TYPE_EQMAC_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value)
		return meade_command(device, ":hU#", NULL, 0, 0);
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value)
		return meade_command(device, ":hW#", NULL, 0, 0);
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value)
		return meade_command(device, ":PO#", NULL, 0, 0);
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		return meade_command(device, ":X370#", response, sizeof(response), 0) && strcmp(response, "p0") == 0;
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)
		return meade_command(device, ":hR#", NULL, 0, 0);
	return false;
}

static bool meade_park_set(indigo_device *device) {
	char response[128];
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)
		return meade_command(device, ":hQ#", response, 1, 0) || *response != '1';
	return false;
}

static bool meade_home(indigo_device *device) {
	char response[128];
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value)
		return meade_command(device, ":hF#", NULL, 0, 0);
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)
		return meade_command(device, ":hC#", NULL, 0, 0);
	if (MOUNT_TYPE_STARGO_ITEM->sw.value)
		return meade_command(device, ":X361#", response, sizeof(response), 0) && strcmp(response, "pA") == 0;
	return false;
}

static bool meade_home_set(indigo_device *device) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value)
		return meade_command(device, ":hF#", NULL, 0, 0);
	if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)
		return meade_command(device, ":hB#", NULL, 0, 0);
	return false;
}


static bool meade_stop(indigo_device *device) {
	return meade_command(device, ":Q#", NULL, 0, 0);
}

static bool meade_guide_dec(indigo_device *device, int north, int south) {
	char command[128];
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		if (north > 0) {
			sprintf(command, ":Mn%03d#", north);
			return meade_command(device, command, NULL, 0, 0);
		} else if (south > 0) {
			sprintf(command, ":Ms%03d#", south);
			return meade_command(device, command, NULL, 0, 0);
		}
	} else {
		if (north > 0) {
			sprintf(command, ":Mgn%04d#", north);
			return meade_command(device, command, NULL, 0, 0);
		} else if (south > 0) {
			sprintf(command, ":Mgs%04d#", south);
			return meade_command(device, command, NULL, 0, 0);
		}
	}
	return false;
}

static bool meade_guide_ra(indigo_device *device, int west, int east) {
	char command[128];
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		if (west > 0) {
			sprintf(command, ":Mw%03d#", west);
			return meade_command(device, command, NULL, 0, 0);
		} else if (east > 0) {
			sprintf(command, ":Me%03d#", east);
			return meade_command(device, command, NULL, 0, 0);
		}
	} else {
		if (west > 0) {
			sprintf(command, ":Mgw%04d#", west);
			return meade_command(device, command, NULL, 0, 0);
		} else if (east > 0) {
			sprintf(command, ":Mge%04d#", east);
			return meade_command(device, command, NULL, 0, 0);
		}
	}
	return false;
}

static bool meade_focus_rel(indigo_device *device, bool slow, int steps) {
	char command[128], response[128];
	if (steps == 0)
		return true;
	PRIVATE_DATA->focus_aborted = false;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_command(device, slow ? ":FS#" : ":FF#", NULL, 0, 0))
			return false;
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_command(device, steps > 0 ? ":F+#" : ":F-#", NULL, 0, 0))
			return false;
		if (steps < 0)
			steps = - steps;
		for (int i = 0; i < steps; i++) {
			if (PRIVATE_DATA->focus_aborted)
				return true;
			indigo_usleep(1000);
		}
		if (!meade_command(device, ":FQ#", NULL, 0, 0))
			return false;
		return true;
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		sprintf(command, ":FR%+d#", steps);
		if (!meade_command(device, command, NULL, 0, 0))
			return false;
		while (true) {
			if (PRIVATE_DATA->focus_aborted)
				return true;
			indigo_usleep(100000);
			if (!meade_command(device, ":FT#", response, sizeof((response)), 0))
				return false;
			if (*response == 'S') {
				break;
			}
		}
	}
	return false;
}

static bool meade_focus_abort(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (meade_command(device, ":FQ#", NULL, 0, 0)) {
			PRIVATE_DATA->focus_aborted = true;
			return true;
		}
	}
	return false;
}

static void meade_update_site_items(indigo_device *device) {
	double latitude = 0, longitude = 0;
	meade_get_site(device, &latitude, &longitude);
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
}

static void meade_update_site_if_changed(indigo_device *device) {
	double latitude = 0, longitude = 0;
	meade_get_site(device, &latitude, &longitude);

	double current_latitude = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
	double current_longitude = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;

	double lat_diff = fabs(current_latitude - latitude);
	double lon_diff = fabs(current_longitude - longitude);

	// Handle 0-360 transition for longitude
	if (lon_diff > 180) {
		lon_diff = 360 - lon_diff;
	}

	if (lat_diff > 0.0028 || lon_diff > 0.0028) { // 10 arcseconds
		MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	}
}

static bool meade_detect_generic_mount(indigo_device *device) {
	char response[128];
	response[0] = 0;
	// These commands are found in the classic LX200 Instruction Manual, and the compatible mounts refer to that manual.
	if (!meade_command(device, ":GR#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GR# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":GD#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GD# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":GC#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GC# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":GL#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GL# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":GG#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GG# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":GS#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":GS# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":Gg#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":Gg# failed."));
		return false;
	}
	response[0] = 0;
	if (!meade_command(device, ":Gt#", response, sizeof(response), 0) || strlen(response) == 0) {
		INDIGO_LOG(indigo_log(":Gt# failed."));
		return false;
	}
	return true;
}

static bool meade_detect_mount(indigo_device *device) {
	char response[128];
	bool result = true;
	if (meade_command(device, ":GVP#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: '%s'", response);
		strncpy(PRIVATE_DATA->product, response, 64);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (!strncmp(PRIVATE_DATA->product, "LX", 2) || !strncmp(PRIVATE_DATA->product, "Autostar", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_MEADE_ITEM, true);
		} else if (!strcmp(PRIVATE_DATA->product, "EQMac")) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_EQMAC_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "10micron", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_10MICRONS_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "Losmandy", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_GEMINI_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "Avalon", 6)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_STARGO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "On-Step", 7)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ON_STEP_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "TeenAstro", 9)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_TEEN_ASTRO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "AM", 2) && isdigit(PRIVATE_DATA->product[2])) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_ZWO_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "NYX", 3)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_NYX_ITEM, true);
		} else if (!strncmp(PRIVATE_DATA->product, "OpenAstroTracker", 16)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_OAT_ITEM, true);
		} else {
			// The classic LX200 and some of the LX200-compatible mounts doesn't implement ":GVP#"
			if (meade_detect_generic_mount(device)) {
				indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_GENERIC_ITEM, true);
			} else {
				MOUNT_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
				result = false;
			}
		}
	} else {
		MOUNT_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
		result = false;
	}
	indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
	return result;
}

static void meade_update_mount_state(indigo_device *device);

static void meade_init_meade_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->perm = INDIGO_RW_PERM;
	MOUNT_PARK_PROPERTY->count = 1;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
	if (meade_command(device, ":GVF#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Version: %s", response);
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
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Status: %s", response);
		MOUNT_MODE_PROPERTY->hidden = false;
		if (response[0] == 'P' || response[0] == 'G')
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		else
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
		indigo_define_property(device, MOUNT_MODE_PROPERTY, NULL);
	}
	if (meade_command(device, ":GH#", response, sizeof(response), 0)) {
		PRIVATE_DATA->use_dst_commands = *response != 0;
	}
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_eqmac_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
	MOUNT_UTC_TIME_PROPERTY->hidden = true;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "N/A");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "EQMac");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	meade_update_mount_state(device);
}

static void meade_init_10microns_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "10Micron");
	indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_command(device, ":EMUAP#", NULL, 0, 0);
	meade_command(device, ":U1#", NULL, 0, 0);
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_gemini_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Losmandy");
	indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_command(device, ":p0#", NULL, 0, 0);
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_stargo_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
	MOUNT_UTC_TIME_PROPERTY->hidden = true;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_command(device, ":TTSFh#", response, 1, 0);
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
	meade_command(device, ":TTSFd#", response, 1, 0);
	indigo_define_property(device, FORCE_FLIP_PROPERTY, NULL);
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_stargo2_mount(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO2");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	meade_update_mount_state(device);
}

static void meade_init_ap_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "AstroPhysics");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	meade_command(device, "#", NULL, 0, 0);
	meade_command(device, ":U#", NULL, 0, 0);
	meade_command(device, ":Br 00:00:00#", NULL, 0, 0);
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_onstep_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "On-Step");
	if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
		indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}
	if (meade_command(device, ":$QZ?#", response, sizeof(response), 0)) {
		indigo_set_switch(MOUNT_PEC_PROPERTY, response[0] == 'P' ? MOUNT_PEC_ENABLED_ITEM : MOUNT_PEC_DISABLED_ITEM, true);
	}
	time_t secs = time(NULL);
	int utc_offset = indigo_get_utc_offset();
	meade_set_utc(device, &secs, utc_offset);

	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_agotino_mount(indigo_device *device) {
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
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "aGotino");
	meade_update_mount_state(device);
}

static void meade_init_zwo_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_MOTION_RA_PROPERTY->hidden = false;
	MOUNT_MOTION_DEC_PROPERTY->hidden = false;
	MOUNT_SLEW_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->hidden = false;
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
	FORCE_FLIP_PROPERTY->hidden = true;
	ZWO_BUZZER_PROPERTY->hidden = false;
	if (meade_command(device, ":GV#", response, sizeof(response), 0)) {
		MOUNT_INFO_PROPERTY->count = 3;
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ZWO");
		strcpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
		strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}

	MOUNT_GUIDE_RATE_DEC_ITEM->number.min =
	MOUNT_GUIDE_RATE_RA_ITEM->number.min = 10;
	MOUNT_GUIDE_RATE_DEC_ITEM->number.max =
	MOUNT_GUIDE_RATE_RA_ITEM->number.max = 90;
	int ra_rate, dec_rate;
	if (meade_get_guide_rate(device, &ra_rate, &dec_rate)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide rate read");
		MOUNT_GUIDE_RATE_RA_ITEM->number.target = MOUNT_GUIDE_RATE_RA_ITEM->number.value = (double)ra_rate;
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target = MOUNT_GUIDE_RATE_DEC_ITEM->number.value = (double)dec_rate;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide rate can not be read read, seting");
		meade_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target);
	}

	if (meade_command(device, ":GU#", response, sizeof(response), 0)) {
		if (strchr(response, 'G'))
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		if (strchr(response, 'Z'))
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
	}
	indigo_define_property(device, MOUNT_MODE_PROPERTY, NULL);
	meade_update_site_items(device);
	time_t secs = 0;
	int utc_offset;
	meade_get_utc(device, &secs, &utc_offset);
	// if date is before January 1, 2001 1:00:00 AM we consifer mount not initialized
	if (secs < 978310800) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount is not initialized, initializing...");
		secs = time(NULL);
		utc_offset = indigo_get_utc_offset();
		meade_set_utc(device, &secs, utc_offset);
		meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value);
	}
	/* Tracking rate */
	if (meade_command(device, ":GT#", response, sizeof(response), 0)) {
		if (strchr(response, '0')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		} else if (strchr(response, '1')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
		} else if (strchr(response, '2')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
		}
	}
	/* Buzzer volume */
	if (meade_command(device, ":GBu#", response, sizeof(response), 0)) {
		if (strchr(response, '0')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_OFF_ITEM, true);
		} else if (strchr(response, '1')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_LOW_ITEM, true);
		} else if (strchr(response, '2')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_HIGH_ITEM, true);
		}
	}
	indigo_define_property(device, ZWO_BUZZER_PROPERTY, NULL);
	meade_update_mount_state(device);
}

static void meade_init_nyx_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
	NYX_WIFI_AP_PROPERTY->hidden = false;
	NYX_WIFI_CL_PROPERTY->hidden = false;
	NYX_WIFI_RESET_PROPERTY->hidden = false;
	NYX_LEVELER_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "PegasusAstro");
	if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
		indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}
	if (meade_command(device, ":GVP#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Model: %s", response);
		indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, response);
	}
	if (!meade_command(device, ":SXEM,1#", response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't set EQ mode");
	}
	if (!meade_command(device, ":SX91,U#", response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't unlock brake");
	}
	char *separator = NULL;
	*NYX_WIFI_AP_SSID_ITEM->text.value = 0;
	*NYX_WIFI_AP_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WL>#", response, sizeof(response), 0) && (separator = strchr(response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_AP_SSID_ITEM->text.value, response, INDIGO_VALUE_SIZE);
		strncpy(NYX_WIFI_AP_PASSWORD_ITEM->text.value, separator, INDIGO_VALUE_SIZE);
	}
	*NYX_WIFI_CL_SSID_ITEM->text.value = 0;
	*NYX_WIFI_CL_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WLD#", response, sizeof(response), 0) && *response != ':' && (separator = strchr(response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_CL_SSID_ITEM->text.value, response, INDIGO_VALUE_SIZE);
		indigo_send_message(device, "Mount is connected to network '%s' with IP address %s", response, separator);
	}


	if (meade_command(device, ":GX9D#", response, sizeof(response), 0) && (separator = strchr(response, ':'))) {
		*separator++ = 0;
		NYX_LEVELER_PICH_ITEM->number.value = atof(response);
		NYX_LEVELER_ROLL_ITEM->number.value = atof(separator);
	}
	if (meade_command(device, ":GX9E#", response, sizeof(response), 0)) {
		NYX_LEVELER_COMPASS_ITEM->number.value = atof(response);
	}
	meade_command(device, ":RE00.03#:RA00.03#", NULL, 0, 0);
	time_t secs = time(NULL);
	int utc_offset = indigo_get_utc_offset();
	meade_set_utc(device, &secs, utc_offset);
	indigo_define_property(device, NYX_WIFI_AP_PROPERTY, NULL);
	indigo_define_property(device, NYX_WIFI_CL_PROPERTY, NULL);
	indigo_define_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
	indigo_define_property(device, NYX_LEVELER_PROPERTY, NULL);
	meade_update_site_items(device);
	meade_update_mount_state(device);
}


static void meade_init_oat_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 1;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "OpenAstroTech");
	indigo_copy_value(MOUNT_INFO_MODEL_ITEM->text.value, "N/A");
	if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
		indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}
	PRIVATE_DATA->use_dst_commands = false;
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_teenastro_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
	MOUNT_MODE_PROPERTY->hidden = true;
	FORCE_FLIP_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "TeenAstro");
	if (meade_command(device, ":GVN#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", response);
		indigo_copy_value(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}

	time_t secs = time(NULL);
	int utc_offset = indigo_get_utc_offset();
	meade_set_utc(device, &secs, utc_offset);

	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_generic_mount(indigo_device *device) {
	// A list of commands can be found in the classic LX200 Instruction Manual.
	// It is assumed that some of the compatible mounts (very old) are created
	// with reference to that list.
	// This type provides a generic mount with the such minimum necessary commands.
	// See also: meade_detect_generic_mount()
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	// NOTE: The classic LX200 have broken `:D#` (distance-bar) command response.
	//   If this command is used, it would be better to separate the mount types.
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_MOTION_RA_PROPERTY->hidden = false;
	MOUNT_MOTION_DEC_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
	meade_update_site_items(device);
	meade_update_mount_state(device);
}

static void meade_init_mount(indigo_device *device) {
	ZWO_BUZZER_PROPERTY->hidden = true;
	NYX_WIFI_AP_PROPERTY->hidden = true;
	NYX_WIFI_CL_PROPERTY->hidden = true;
	NYX_WIFI_RESET_PROPERTY->hidden = true;
	NYX_LEVELER_PROPERTY->hidden = true;
	memset(PRIVATE_DATA->prev_state, 0, sizeof(PRIVATE_DATA->prev_state));
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		meade_init_meade_mount(device);
	}
	else if (MOUNT_TYPE_EQMAC_ITEM->sw.value) {
		meade_init_eqmac_mount(device);
	}
	else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		meade_init_10microns_mount(device);
	}
	else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		meade_init_gemini_mount(device);
	}
	else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_init_stargo_mount(device);
	}
	else if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
		meade_init_stargo2_mount(device);
	}
	else if (MOUNT_TYPE_AP_ITEM->sw.value) {
		meade_init_ap_mount(device);
	}
	else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		meade_init_onstep_mount(device);
	}
	else if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		meade_init_agotino_mount(device);
	}
	else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		meade_init_zwo_mount(device);
	}
	else if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		meade_init_nyx_mount(device);
	}
	else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		meade_init_oat_mount(device);
	}
	else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		meade_init_teenastro_mount(device);
	}
	else
		meade_init_generic_mount(device);
}

static void meade_update_meade_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":D#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = *response ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (meade_command(device, ":GW#", response, sizeof(response), 0)) {
		if (response[1] == 'T') {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		}
	}
}

static void meade_update_eqmac_state(indigo_device *device) {
	if (MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value == 0 && MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value == 0) {
		if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_changed = true;
		}
	} else {
		if (!MOUNT_PARK_UNPARKED_ITEM->sw.value) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			PRIVATE_DATA->park_changed = true;
		}
	}
}

static void meade_update_10microns_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":Gstat#", response, sizeof(response), 0)) {
		switch (atoi(response)) {
			case 0:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 2:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 3:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 4:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 5:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 6:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			default:
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					PRIVATE_DATA->tracking_changed = true;
				}
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

static void meade_update_gemini_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":Gv#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (*response == 'S' || *response == 'C') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (*response == 'T') {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (meade_command(device, ":h?#", response, sizeof(response), 0)) {
		if (*response == '1') {
			if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (*response == '2') {
			if (MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else {
			if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		}
	}
}

static void meade_update_avalon_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":X34#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (response[1] > '1' || response[2] > '1') ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (response[1] == '1') {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (!MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (meade_command(device, ":X38#", response, sizeof(response), 0)) {
		switch (response[1]) {
			case '2':
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 'A':
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			case 'B':
				if (MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE || !MOUNT_PARK_PARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
			default:
				if (MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE || !MOUNT_PARK_UNPARKED_ITEM->sw.value) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->park_changed = true;
				}
				if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->home_changed = true;
				}
				break;
		}
	}
}

static void meade_update_onstep_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GU#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strchr(response, 'N') ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
		if (strchr(response, 'n')) {
			if (MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
			if (strchr(response, '(')) {
				if (!MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else if (strchr(response, 'O')) {
				if (!MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else if (strchr(response, 'k')) {
				if (!MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else {
				if (!MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			}
			if (PRIVATE_DATA->tracking_rate_changed == true) {
				PRIVATE_DATA->tracking_rate_changed = false;
				indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
		}
		if (strchr(response, 'P')) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
				indigo_send_message(device, "Parked");
			}
		} else if (strchr(response, 'p')) {
			if (!MOUNT_PARK_UNPARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (strchr(response, 'I')) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (strchr(response, 'F')) {
			if (!MOUNT_PARK_UNPARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_ALERT_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if (strchr(response, 'H')) {
		if (PRIVATE_DATA->prev_home_state == false) {
			MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
			PRIVATE_DATA->home_changed = true;
			indigo_send_message(device, "At home");
		}
		PRIVATE_DATA->prev_home_state = true;
	} else {
		if (PRIVATE_DATA->prev_home_state == true) {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
			PRIVATE_DATA->home_changed = true;
		}
		PRIVATE_DATA->prev_home_state = false;
	}

	if (meade_command(device, ":Gm#", response, sizeof(response), 0)) {
		if (strchr(response, 'W') && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		} else if (strchr(response, 'E') && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		} else if (strchr(response, 'N') && (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value || MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value)){
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		}
	}
}

static void meade_update_zwo_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GU#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strchr(response, 'N') ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
		if (strchr(response, 'n')) {
			if (MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		}
		if (strchr(response, 'H')) {
			if (PRIVATE_DATA->prev_home_state == false) {
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
				PRIVATE_DATA->home_changed = true;
			}
			PRIVATE_DATA->prev_home_state = true;
		} else {
			if (PRIVATE_DATA->prev_home_state == true) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
				PRIVATE_DATA->home_changed = true;
			}
			PRIVATE_DATA->prev_home_state = false;
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	if (meade_command(device, ":Gm#", response, sizeof(response), 0)) {
		if (strchr(response, 'W') && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		} else if (strchr(response, 'E') && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		} else if (strchr(response, 'N') && (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value || MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value)){
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		}
	}
}

static void meade_update_nyx_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":GU#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strchr(response, 'N') ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
		if (strchr(response, 'n')) {
			if (MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else {
			if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
			if (strchr(response, '(')) {
				if (!MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else if (strchr(response, 'O')) {
				if (!MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else if (strchr(response, 'k')) {
				if (!MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			} else {
				if (!MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
					PRIVATE_DATA->tracking_rate_changed = true;
				}
			}
			if (PRIVATE_DATA->tracking_rate_changed == true) {
				PRIVATE_DATA->tracking_rate_changed = false;
				indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			}
		}
		if (strchr(response, 'P')) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (strchr(response, 'I')) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (strchr(response, 'F')) {
			if (!MOUNT_PARK_UNPARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_ALERT_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else {
			if (!MOUNT_PARK_UNPARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		}
		if (strchr(response, 'H')) {
			if (MOUNT_HOME_PROPERTY->state != INDIGO_OK_STATE) {
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->home_changed = true;
			}
		} else if (strchr(response, 'h')) {
			if (MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE) {
				MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->home_changed = true;
			}
		}
		if (strchr(response, 'W')) {
			if (!MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		} else if (strchr(response, 'T')) {
			if (!MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		} else if (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value || MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	char *colon;
	bool leveler_change = false;
	if (meade_command(device, ":GX9D#", response, sizeof(response), 0) && (colon = strchr(response, ':'))) {
		*colon++ = 0;
		double pitch = atof(response);
		double roll = atof(colon);
		if (NYX_LEVELER_PICH_ITEM->number.value != pitch || NYX_LEVELER_ROLL_ITEM->number.value != roll) {
			NYX_LEVELER_PICH_ITEM->number.value = pitch;
			NYX_LEVELER_ROLL_ITEM->number.value = roll;
			leveler_change = true;
		}
	}
	if (meade_command(device, ":GX9E#", response, sizeof(response), 0)) {
		double compass = atof(response);
		if (NYX_LEVELER_COMPASS_ITEM->number.value != compass) {
			NYX_LEVELER_COMPASS_ITEM->number.value = compass;
			leveler_change = true;
		}
	}
	if (leveler_change) {
		indigo_update_property(device, NYX_LEVELER_PROPERTY, NULL);
	}
}

static void meade_update_oat_state(indigo_device *device) {
	char response[128];
	if (meade_command(device, ":D#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = *response ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (meade_command(device, ":GX#", response, sizeof(response), 0)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strstr(response, "Slew") ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		if (!strncmp(response, "Idle", 4)) {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else if (!strncmp(response, "Tracking", 8)) {
			if (!MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				PRIVATE_DATA->tracking_changed = true;
			}
		} else if (!strncmp(response, "Parked", 6)) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_OK_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (!strncmp(response, "Parking", 7)) {
			if (!MOUNT_PARK_PARKED_ITEM->sw.value || MOUNT_PARK_PROPERTY->state != INDIGO_BUSY_STATE) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->park_changed = true;
			}
		} else if (!strncmp(response, "Homing", 7)) {
			if (MOUNT_HOME_PROPERTY->state != INDIGO_BUSY_STATE) {
				MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->home_changed = true;
			}
		}
	}
}

static void meade_update_teenastro_state(indigo_device *device) {
	char response[128] = {0};
	if (meade_command(device, ":GXI#", response, sizeof(response), 0)) {
		// Byte 0 is tracking status
		if (PRIVATE_DATA->prev_state[0] != response[0]) {
			if (response[0] == '0') {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->tracking_changed = true;
			} else if (response[0] == '1') {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->tracking_changed = true;
			} else if (response[0] == '2' || response[0] == '3') {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->tracking_changed = true;
			}
		}

		// Byte 1 is tracking rate
		if (PRIVATE_DATA->prev_state[1] != response[1]) {
			if (response[1] == '0') {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->tracking_rate_changed = true;
			} else if (response[1] == '1') {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->tracking_rate_changed = true;
			} else if (response[1] == '2') {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->tracking_rate_changed = true;
			}
		}

		// Byte 2 is park status
		if (PRIVATE_DATA->prev_state[2] != response[2]) {
			if (response[2] == 'P') {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			} else if (response[2] == 'p') {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->park_changed = true;
			} else if (response[2] == 'I') {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->park_changed = true;
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}

		//byte 3 is home status
		if (PRIVATE_DATA->prev_state[3] != response[3]) {
			if (response[3] == 'H') {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->home_changed = true;
			} else {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
				PRIVATE_DATA->home_changed = true;
			}
		}

		// Byte 4 is current slew rate
		// TBD

		// Byte 13 is pier side
		if (PRIVATE_DATA->prev_state[13] != response[13]) {
			if (response[13] == 'W') {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			} else if (response[13] == 'E') {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			} else if (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value || MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
				MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
				MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		}

		// Byte 15 is the error status
		// TBD

		strncpy(PRIVATE_DATA->prev_state, response, sizeof(PRIVATE_DATA->prev_state));
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

static void meade_update_generic_state(indigo_device *device) {
	if (PRIVATE_DATA->motioned) {
		// After Motion NS or EW
		if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		// After Track or Slew
		// NOTE: Distance bar `:D#` is not working (e.g. classic LX200).
		if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - PRIVATE_DATA->lastRA) < 2.0/60.0 && fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - PRIVATE_DATA->lastDec) < 2.0/60.0)
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	}
}

static void meade_update_mount_state(indigo_device *device) {
	PRIVATE_DATA->park_changed = false;
	PRIVATE_DATA->home_changed = false;
	PRIVATE_DATA->tracking_changed = false;
	// read coordinates
	double ra = 0, dec = 0;
	if (meade_get_coordinates(device, &ra, &dec)) {
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		// check state
		if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
			meade_update_meade_state(device);
		} else if (MOUNT_TYPE_EQMAC_ITEM->sw.value) {
			meade_update_eqmac_state(device);
		} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
			meade_update_10microns_state(device);
		} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			meade_update_gemini_state(device);
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			meade_update_avalon_state(device);
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			meade_update_onstep_state(device);
		} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
			meade_update_zwo_state(device);
		} else if (MOUNT_TYPE_NYX_ITEM->sw.value) {
			meade_update_nyx_state(device);
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			meade_update_oat_state(device);
		} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			meade_update_teenastro_state(device);
		} else {
			meade_update_generic_state(device);
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->lastRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	PRIVATE_DATA->lastDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	// read time
	int utc_offset;
	time_t secs;
	if (meade_get_utc(device, &secs, &utc_offset)) {
		sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", utc_offset);
		indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0 && !PRIVATE_DATA->wifi_reset) {
		meade_update_site_if_changed(device);
		meade_update_mount_state(device);
		indigo_update_coordinates(device, NULL);
		if (PRIVATE_DATA->tracking_changed)
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		if (PRIVATE_DATA->park_changed)
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		if (PRIVATE_DATA->home_changed)
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

static void mount_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->is_site_set = false;
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = meade_open(device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				if (!meade_detect_mount(device)) {
					result = false;
					indigo_send_message(device, "Autodetection failed!");
					meade_close(device);
				}
			}
		}
		if (result) {
			meade_init_mount(device);
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
			if (PRIVATE_DATA->keep_alive_timer) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->keep_alive_timer);
			}
			meade_stop(device);
			meade_close(device);
		}
		indigo_delete_property(device, MOUNT_MODE_PROPERTY, NULL);
		indigo_delete_property(device, FORCE_FLIP_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_CL_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		indigo_delete_property(device, NYX_LEVELER_PROPERTY, NULL);
		MOUNT_TYPE_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void mount_park_callback(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (MOUNT_PARK_PROPERTY->count == 1)
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		if (meade_park(device)) {
			if (!(MOUNT_TYPE_EQMAC_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value))
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
	}
	if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
		if (meade_unpark(device)) {
			if (!(MOUNT_TYPE_EQMAC_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value))
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking");
	}
}

static void mount_park_set_callback(indigo_device *device) {
	if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
		MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
		if (meade_park_set(device)) {
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, "Current position set as park position");
		} else {
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, "Setting park position failed");
		}
	}
}

static void mount_home_callback(indigo_device *device) {
	if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_ITEM->sw.value = false;
		if (!meade_home(device)) {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		} else {
			PRIVATE_DATA->prev_home_state = false;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home");
		}
	}
}

static void mount_home_set_callback(indigo_device *device) {
	if (MOUNT_HOME_SET_CURRENT_ITEM->sw.value) {
		MOUNT_HOME_SET_CURRENT_ITEM->sw.value = false;
		if (meade_home_set(device)) {
			MOUNT_HOME_SET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, "Current position set as home");
		} else {
			MOUNT_HOME_SET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, "Setting home position failed");
		}
	}
}

static void mount_geo_coords_callback(indigo_device *device) {
	if (meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value))
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_eq_coords_callback(indigo_device *device) {
	char message[50] = {0};
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (meade_set_tracking_rate(device) && meade_slew(device, ra, dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			strcpy(message, "Slew failed");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (meade_sync(device, ra, dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			strcpy(message, "Sync failed");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	if (*message == '\0') {
		indigo_update_coordinates(device, NULL);
	} else {
		indigo_update_coordinates(device, message);
	}
}

static void mount_abort_callback(indigo_device *device) {
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		if (meade_stop(device)) {
			MOUNT_MOTION_NORTH_ITEM->sw.value = false;
			MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			MOUNT_MOTION_WEST_ITEM->sw.value = false;
			MOUNT_MOTION_EAST_ITEM->sw.value = false;
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				MOUNT_HOME_PROPERTY->state=INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
			}
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Failed to abort");
		}
	}
}

static void mount_motion_dec_callback(indigo_device *device) {
	if (meade_set_slew_rate(device) && meade_motion_dec(device)) {
		if (PRIVATE_DATA->lastMotionNS)
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_callback(indigo_device *device) {
	if (meade_set_slew_rate(device) && meade_motion_ra(device)) {
		if (PRIVATE_DATA->lastMotionWE)
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_set_host_time_callback(indigo_device *device) {
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		time_t secs = time(NULL);
		if (meade_set_utc(device, &secs, indigo_get_utc_offset())) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		} else {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_set_utc_time_callback(indigo_device *device) {
	time_t secs = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	int offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
	if (secs == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_mount_lx200: Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
	} else {
		if (meade_set_utc(device, &secs, offset)) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
}

static void mount_tracking_callback(indigo_device *device) {
	if (meade_set_tracking(device, MOUNT_TRACKING_ON_ITEM->sw.value))
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_callback(indigo_device *device) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (meade_set_tracking_rate(device)) {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_force_flip_callback(indigo_device *device) {
	if (meade_force_flip(device, FORCE_FLIP_ENABLED_ITEM->sw.value))
		FORCE_FLIP_PROPERTY->state = INDIGO_OK_STATE;
	else
		FORCE_FLIP_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FORCE_FLIP_PROPERTY, NULL);
}

static void mount_pec_callback(indigo_device *device) {
	if (meade_pec(device, MOUNT_PEC_ENABLED_ITEM->sw.value))
		MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
}

static void mount_guide_rate_callback(indigo_device *device) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value =
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target =
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target;
	}
	if (meade_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target))
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void zwo_buzzer_callback(indigo_device *device) {
	if (ZWO_BUZZER_OFF_ITEM->sw.value) {
		meade_command(device, ":SBu0#", NULL, 0, 0);
	} else if (ZWO_BUZZER_LOW_ITEM->sw.value) {
		meade_command(device, ":SBu1#", NULL, 0, 0);
	} else if (ZWO_BUZZER_HIGH_ITEM->sw.value) {
		meade_command(device, ":SBu2#", NULL, 0, 0);
	}
	ZWO_BUZZER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
}

static void nyx_ap_callback(indigo_device *device) {
	char command[64], response[64];
	snprintf(command, sizeof(command), ":WA%s#", NYX_WIFI_AP_SSID_ITEM->text.value);
	NYX_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	if (meade_command(device, command, response, sizeof(response), 0) && *response == '1') {
		snprintf(command, sizeof(command), ":WB%s#", NYX_WIFI_AP_PASSWORD_ITEM->text.value);
		if (meade_command(device, command, response, sizeof(response), 0) && *response == '1') {
			if (meade_command(device, ":WLC#", response, sizeof(response), 0) && *response == '1') {
				indigo_send_message(device, "Created access point with SSID %s", NYX_WIFI_AP_SSID_ITEM->text.value);
				NYX_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	indigo_update_property(device, NYX_WIFI_AP_PROPERTY, NULL);
}

static void nyx_cl_callback(indigo_device *device) {
	const size_t ssid_len = strlen(NYX_WIFI_CL_SSID_ITEM->text.value);
	const size_t pass_len = strlen(NYX_WIFI_CL_PASSWORD_ITEM->text.value);
	char command[64], response[64];
	char *encoded = NULL;

	if (compare_versions(MOUNT_INFO_FIRMWARE_ITEM->text.value, NYX_BASE64_THRESHOLD_VERSION) >= 0) {
		encoded = indigo_safe_malloc(MAX(ssid_len, pass_len)/3 * 4 + 4);
	}

	if (encoded != NULL) {
		base64_encode((unsigned char *)encoded, (unsigned char *)NYX_WIFI_CL_SSID_ITEM->text.value, ssid_len);
		snprintf(command, sizeof(command), ":WS%s#", encoded);
	} else {
		snprintf(command, sizeof(command), ":WS%s#", NYX_WIFI_CL_SSID_ITEM->text.value);
	}
	if (meade_command(device, command, response, sizeof(response), 0) && *response == '1') {
		if (encoded != NULL) {
			base64_encode((unsigned char *)encoded, (unsigned char*)NYX_WIFI_CL_PASSWORD_ITEM->text.value, pass_len);
			snprintf(command, sizeof(command), ":WP%s#", encoded);
		} else {
			snprintf(command, sizeof(command), ":WP%s#", NYX_WIFI_CL_PASSWORD_ITEM->text.value);
		}
		if (meade_command(device, command, response, sizeof(response), 0) && *response == '1') {
			if (meade_command(device, ":WLC#", NULL, 0, 0)) {
				indigo_send_message(device, "WiFi reset!");
				NYX_WIFI_CL_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
				if (PRIVATE_DATA->is_network) {
					PRIVATE_DATA->wifi_reset = true;
					indigo_set_timer(device, 0, network_disconnection, NULL);
				}
				return;
			}
		}
	}
	indigo_safe_free(encoded);
	NYX_WIFI_CL_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
}

static void nyx_reset_callback(indigo_device *device) {
	if (meade_command(device, ":WLZ#", NULL, 0, 0)) {
		indigo_send_message(device, "WiFi reset!");
		NYX_WIFI_RESET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		if (PRIVATE_DATA->is_network) {
			PRIVATE_DATA->wifi_reset = true;
			indigo_set_timer(device, 0, network_disconnection, NULL);
		}
		return;
	}
	NYX_WIFI_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		MOUNT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Mount mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(EQUATORIAL_ITEM, EQUATORIAL_ITEM_NAME, "Equatorial mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		MOUNT_MODE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FORCE_FLIP
		FORCE_FLIP_PROPERTY = indigo_init_switch_property(NULL, device->name, FORCE_FLIP_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Meridian flip mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (FORCE_FLIP_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(FORCE_FLIP_ENABLED_ITEM, FORCE_FLIP_ENABLED_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(FORCE_FLIP_DISABLED_ITEM, FORCE_FLIP_DISABLED_ITEM_NAME, "Disabled", false);
		FORCE_FLIP_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 15);
		if (MOUNT_TYPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_TYPE_DETECT_ITEM, MOUNT_TYPE_DETECT_ITEM_NAME, "Autodetect", true);
		indigo_init_switch_item(MOUNT_TYPE_MEADE_ITEM, MOUNT_TYPE_MEADE_ITEM_NAME, "Meade", false);
		indigo_init_switch_item(MOUNT_TYPE_EQMAC_ITEM, MOUNT_TYPE_EQMAC_ITEM_NAME, "EQMac", false);
		indigo_init_switch_item(MOUNT_TYPE_10MICRONS_ITEM, MOUNT_TYPE_10MICRONS_ITEM_NAME, "10Microns", false);
		indigo_init_switch_item(MOUNT_TYPE_GEMINI_ITEM, MOUNT_TYPE_GEMINI_ITEM_NAME, "Losmandy Gemini", false);
		indigo_init_switch_item(MOUNT_TYPE_STARGO_ITEM, MOUNT_TYPE_STARGO_ITEM_NAME, "Avalon StarGO", false);
		indigo_init_switch_item(MOUNT_TYPE_STARGO2_ITEM, MOUNT_TYPE_STARGO2_ITEM_NAME, "Avalon StarGO2", false);
		indigo_init_switch_item(MOUNT_TYPE_AP_ITEM, MOUNT_TYPE_AP_ITEM_NAME, "Astro-Physics GTO", false);
		indigo_init_switch_item(MOUNT_TYPE_ON_STEP_ITEM, MOUNT_TYPE_ON_STEP_ITEM_NAME, "OnStep", false);
		indigo_init_switch_item(MOUNT_TYPE_AGOTINO_ITEM, MOUNT_TYPE_AGOTINO_ITEM_NAME, "aGotino", false);
		indigo_init_switch_item(MOUNT_TYPE_ZWO_ITEM, MOUNT_TYPE_ZWO_ITEM_NAME, "ZWO AM", false);
		indigo_init_switch_item(MOUNT_TYPE_NYX_ITEM, MOUNT_TYPE_NYX_ITEM_NAME, "Pegasus NYX", false);
		indigo_init_switch_item(MOUNT_TYPE_OAT_ITEM, MOUNT_TYPE_OAT_ITEM_NAME, "OpenAstroTech", false);
		indigo_init_switch_item(MOUNT_TYPE_TEEN_ASTRO_ITEM, MOUNT_TYPE_TEEN_ASTRO_ITEM_NAME, "Teen Astro", false);
		indigo_init_switch_item(MOUNT_TYPE_GENERIC_ITEM, MOUNT_TYPE_GENERIC_ITEM_NAME, "Generic", false);
		// ---------------------------------------------------------------------------- ZWO_BUZZER
		ZWO_BUZZER_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_BUZZER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Buzzer volume", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (ZWO_BUZZER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(ZWO_BUZZER_OFF_ITEM, ZWO_BUZZER_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(ZWO_BUZZER_LOW_ITEM, ZWO_BUZZER_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(ZWO_BUZZER_HIGH_ITEM, ZWO_BUZZER_HIGH_ITEM_NAME, "High", false);
		ZWO_BUZZER_PROPERTY->hidden = true;
		// ---------------------------------------------------------------------------- NYX_WIFI_AP
		NYX_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_AP_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_AP_PROPERTY == NULL)
			return INDIGO_FAILED;
		NYX_WIFI_AP_PROPERTY->hidden = true;
		indigo_init_text_item(NYX_WIFI_AP_SSID_ITEM, NYX_WIFI_AP_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_AP_PASSWORD_ITEM, NYX_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		// ---------------------------------------------------------------------------- NYX_WIFI_CL
		NYX_WIFI_CL_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_CL_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Client WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_CL_PROPERTY == NULL)
			return INDIGO_FAILED;
		NYX_WIFI_CL_PROPERTY->hidden = true;
		indigo_init_text_item(NYX_WIFI_CL_SSID_ITEM, NYX_WIFI_CL_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_CL_PASSWORD_ITEM, NYX_WIFI_CL_PASSWORD_ITEM_NAME, "Password", "");
		// ---------------------------------------------------------------------------- NYX_WIFI_RESET
		NYX_WIFI_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, NYX_WIFI_RESET_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Reset WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (NYX_WIFI_RESET_PROPERTY == NULL)
			return INDIGO_FAILED;
		NYX_WIFI_RESET_PROPERTY->hidden = true;
		indigo_init_switch_item(NYX_WIFI_RESET_ITEM, NYX_WIFI_RESET_ITEM_NAME, "Reset", false);
		// ---------------------------------------------------------------------------- NYX_LEVELER
		NYX_LEVELER_PROPERTY = indigo_init_number_property(NULL, device->name, NYX_LEVELER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Leveler", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (NYX_LEVELER_PROPERTY == NULL)
			return INDIGO_FAILED;
		NYX_LEVELER_PROPERTY->hidden = true;
		indigo_init_number_item(NYX_LEVELER_PICH_ITEM, NYX_LEVELER_PICH_ITEM_NAME, "Pitch [°]", 0, 360, 0, 0);
		indigo_init_number_item(NYX_LEVELER_ROLL_ITEM, NYX_LEVELER_ROLL_ITEM_NAME, "Roll [°]", 0, 360, 0, 0);
		indigo_init_number_item(NYX_LEVELER_COMPASS_ITEM, NYX_LEVELER_COMPASS_ITEM_NAME, "Compas [°]", 0, 360, 0, 0);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_define_matching_property(MOUNT_TYPE_PROPERTY);
	if (IS_CONNECTED) {
		indigo_define_matching_property(MOUNT_MODE_PROPERTY);
		indigo_define_matching_property(FORCE_FLIP_PROPERTY);
		indigo_define_matching_property(ZWO_BUZZER_PROPERTY);
		indigo_define_matching_property(NYX_WIFI_AP_PROPERTY);
		indigo_define_matching_property(NYX_WIFI_CL_PROPERTY);
		indigo_define_matching_property(NYX_WIFI_RESET_PROPERTY);
		if (indigo_property_match(NYX_LEVELER_PROPERTY, property))
			indigo_define_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		bool parked = MOUNT_PARK_PARKED_ITEM->sw.value;
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if ((!parked && MOUNT_PARK_PARKED_ITEM->sw.value) || (parked && MOUNT_PARK_UNPARKED_ITEM->sw.value)) {
			if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE && !MOUNT_HOME_PROPERTY->hidden) {
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Can not park while mount is homing!");
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
				indigo_set_timer(device, 0, mount_park_callback, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		indigo_property_copy_values(MOUNT_PARK_SET_PROPERTY, property, false);
		MOUNT_PARK_SET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_park_set_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			if(MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
				// Ignore the request if the mount is already homing
			} else if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE && !MOUNT_PARK_PROPERTY->hidden) {
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
				MOUNT_HOME_ITEM->sw.value = false;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, "Can not go home while mount is being parked!");
			} else if (IS_PARKED) {
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
				MOUNT_HOME_ITEM->sw.value = false;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, "Mount is parked!");
			} else {
				MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
				indigo_set_timer(device, 0, mount_home_callback, NULL);
			}
			return INDIGO_OK;
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME_SET
		indigo_property_copy_values(MOUNT_HOME_SET_PROPERTY, property, false);
		MOUNT_HOME_SET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_home_set_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_geo_coords_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (IS_PARKED) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		} else {
			PRIVATE_DATA->motioned = false; // WTF?
			indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_eq_coords_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		PRIVATE_DATA->motioned = true; // WTF?
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_abort_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if (IS_PARKED) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_dec_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (IS_PARKED) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_ra_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_set_host_time_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_set_utc_time_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (IS_PARKED) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_tracking_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_track_rate_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FORCE_FLIP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FORCE_FLIP
		if (IS_PARKED) {
			FORCE_FLIP_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FORCE_FLIP_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(FORCE_FLIP_PROPERTY, property, false);
			FORCE_FLIP_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FORCE_FLIP_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_force_flip_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PEC
		if (IS_PARKED) {
			MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PEC_PROPERTY, "Mount is parked!");
		} else {
			indigo_property_copy_values(MOUNT_PEC_PROPERTY, property, false);
			MOUNT_PEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_pec_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_guide_rate_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		indigo_property_copy_values(MOUNT_TYPE_PROPERTY, property, false);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (MOUNT_TYPE_EQMAC_ITEM->sw.value) {
			strcpy(DEVICE_PORT_ITEM->text.value, "lx200://localhost");
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		} else if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
			strcpy(DEVICE_PORT_ITEM->text.value, "lx200://StarGo2.local:9624");
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
		}
		indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_BUZZER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ZWO_BUZZER
		indigo_property_copy_values(ZWO_BUZZER_PROPERTY, property, false);
		ZWO_BUZZER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_set_timer(device, 0, zwo_buzzer_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_AP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_AP
		indigo_property_copy_values(NYX_WIFI_AP_PROPERTY, property, false);
		NYX_WIFI_AP_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_AP_PROPERTY, NULL);
		indigo_set_timer(device, 0, nyx_ap_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_CL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_CL
		indigo_property_copy_values(NYX_WIFI_CL_PROPERTY, property, false);
		NYX_WIFI_CL_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
		indigo_set_timer(device, 0, nyx_cl_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_RESET
		indigo_property_copy_values(NYX_WIFI_RESET_PROPERTY, property, false);
		NYX_WIFI_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		indigo_set_timer(device, 0, nyx_reset_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
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
	indigo_release_property(MOUNT_MODE_PROPERTY);
	indigo_release_property(FORCE_FLIP_PROPERTY);
	indigo_release_property(ZWO_BUZZER_PROPERTY);
	indigo_release_property(NYX_WIFI_AP_PROPERTY);
	indigo_release_property(NYX_WIFI_CL_PROPERTY);
	indigo_release_property(NYX_WIFI_RESET_PROPERTY);
	indigo_release_property(NYX_LEVELER_PROPERTY);
	indigo_release_property(MOUNT_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void keep_alive_callback(indigo_device *device) {
	char response[128];
	meade_command(device, ":GVP#", response, sizeof(response), 0);
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->keep_alive_timer);
}

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
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = meade_open(device->master_device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			char response[128];
			if (meade_command(device, ":GVP#", response, sizeof(response), 0)) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: '%s'", response);
				strncpy(PRIVATE_DATA->product, response, 64);
				if (!strncmp(PRIVATE_DATA->product, "AM", 2) && isdigit(PRIVATE_DATA->product[2])) {
					GUIDER_GUIDE_NORTH_ITEM->number.max =
					GUIDER_GUIDE_SOUTH_ITEM->number.max =
					GUIDER_GUIDE_EAST_ITEM->number.max =
					GUIDER_GUIDE_WEST_ITEM->number.max = 3000;
				}
			}
			if (PRIVATE_DATA->is_network && !PRIVATE_DATA->keep_alive_timer) {
				/* In case of a network connection and there is no mount connected (to create chatter)
				   the commection is closed in several seconds. So we send :GVP# on a regular basis
				   to keep the connection alive */
				indigo_set_timer(device, 0, keep_alive_callback, &PRIVATE_DATA->keep_alive_timer);
			}
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			if (PRIVATE_DATA->keep_alive_timer) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->keep_alive_timer);
			}
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void guider_guide_dec_callback(indigo_device *device) {
	int north = GUIDER_GUIDE_NORTH_ITEM->number.value;
	int south = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	meade_guide_dec(device, north, south);
	if (north > 0) {
		indigo_usleep(1000 * north);
	} else if (south > 0) {
		indigo_usleep(1000 * south);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_ra_callback(indigo_device *device) {
	int west = GUIDER_GUIDE_WEST_ITEM->number.value;
	int east = GUIDER_GUIDE_EAST_ITEM->number.value;
	meade_guide_ra(device, west, east);
	if (west > 0) {
		indigo_usleep(1000 * west);
	} else if (east > 0) {
		indigo_usleep(1000 * east);
	}
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_guide_dec_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
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

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			result = meade_open(device->master_device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				meade_detect_mount(device->master_device);
			}
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
				FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
				FOCUSER_SPEED_ITEM->number.max = 2;
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				if (PRIVATE_DATA->is_network && !PRIVATE_DATA->keep_alive_timer) {
					/* In case of a network connection and there is no mount connected (to create chatter)
					 the commection is closed in several seconds. So we send :GVP# on a regular basis
					 to keep the connection alive */
					indigo_set_timer(device, 0, keep_alive_callback, &PRIVATE_DATA->keep_alive_timer);
				}
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			if (PRIVATE_DATA->keep_alive_timer) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->keep_alive_timer);
			}
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void focuser_steps_callback(indigo_device *device) {
	int steps = FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value;
	if (meade_focus_rel(device, FOCUSER_SPEED_ITEM->number.value == FOCUSER_SPEED_ITEM->number.min, steps))
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	else
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_callback(indigo_device *device) {
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		if (meade_focus_abort(device))
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		else
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_SPEED
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_STEPS
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_steps_callback, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_callback, NULL);
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

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Info", "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [mb]", 0, 2000, 0, 0);
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Info", "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void nyx_aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[128];
	bool updateWeather = false;
	bool updateInfo = false;
	if (meade_command(device, ":GX9A#", response, sizeof(response), 0)) {
		double temperature = atof(response);
		if (AUX_WEATHER_TEMPERATURE_ITEM->number.value != temperature) {
			AUX_WEATHER_TEMPERATURE_ITEM->number.value = temperature;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9B#", response, sizeof(response), 0)) {
		double pressure = atof(response);
		if (AUX_WEATHER_PRESSURE_ITEM->number.value != pressure) {
			AUX_WEATHER_PRESSURE_ITEM->number.value = pressure;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9V#", response, sizeof(response), 0)) {
		double voltage = atof(response);
		if (AUX_INFO_VOLTAGE_ITEM->number.value != voltage) {
			AUX_INFO_VOLTAGE_ITEM->number.value = voltage;
			updateInfo = true;
		}
	}
	if (updateWeather) {
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	}
	if (updateInfo) {
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 10, &PRIVATE_DATA->aux_timer);
}

static void onstep_aux_timer_callback(indigo_device *device) {

	if (!IS_CONNECTED) {
		return;
	}

	bool update_aux_heater_prop = false;
	for (int i = 0; i < ONSTEP_AUX_HEATER_OUTLET_COUNT; i++) {
		char command[7];
		char response[4];
		int onstep_slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
		snprintf(command, sizeof(command), ":GXX%d#", onstep_slot);
		// responds with a number between 0 for fully off and 255 for fully on
		meade_command(device, command, response, sizeof(response), 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received response %s for slot %d", response, onstep_slot);
		indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
		// convert to percent
		int new_value = (int)(atoi(response) / 2.56 + 0.5);
		if (new_value != (int) item->number.value) {
			item->number.value = new_value;
			update_aux_heater_prop = true;
		}
	}
	if (update_aux_heater_prop) {
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}

	bool update_aux_power_prop = false;
	for (int i = 0; i < ONSTEP_AUX_POWER_OUTLET_COUNT; i++) {
		char command[7];
		char response[4];
		int onstep_slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
		snprintf(command, sizeof(command), ":GXX%d#", onstep_slot);
		// the response is 0 when disabled and 1 when the switch is enabled
		meade_command(device, command, response, sizeof(response), 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received response %s for slot %d", response, onstep_slot);
		indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
		bool active = response[0] - '0';
		if (active != item->sw.value) {
			item->sw.value = active;
			update_aux_power_prop = true;
		}
	}
	if (update_aux_power_prop) {
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	}

	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);

}

static void onstep_aux_connect(indigo_device *device) {
	char aux_device_str[ONSTEP_AUX_DEVICE_COUNT + 1];
	// first we request Onstep to list active aux slots
	meade_command(device, ":GXY0#", aux_device_str, sizeof(aux_device_str), 0);
	// Onstep responds with a string like "11000000" to indicate that the first and second aux device is enabled
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep active device string: %s", aux_device_str);

	// in the first pass over the active devices we count how many auxiliary devices of each purpose we have
	for (int i = 0; i < ONSTEP_AUX_DEVICE_COUNT; i++) {
		bool active = aux_device_str[i] == '1';
		if (active) {
			char response[16];
			char command[7];
			// Now we get the name and purpose of each active aux device, it is one-indexed
			snprintf(command, sizeof(command), ":GXY%d#", i + 1);
			meade_command(device, command, response, sizeof(response), 0);
			// Onstep responds with a string like "my switch,2" where the part before "," is the name and after the purpose as int
			char *comma = strchr(response, ',');
			if (comma == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Onstep AUX Device at slot %d invalid response", i + 1);
				continue;
			}
			*comma++ = '\0';
			char *name = response;
			onstep_aux_device_purpose purpose = *comma - '0';
			if (purpose == ONSTEP_AUX_ANALOG) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Heater Outlet at slot %d with name %s", i + 1, name);
				ONSTEP_AUX_HEATER_OUTLET_MAPPING[ONSTEP_AUX_HEATER_OUTLET_COUNT] = i + 1;
				ONSTEP_AUX_HEATER_OUTLET_COUNT++;
			} else if (purpose == ONSTEP_AUX_SWITCH) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Power Outlet Device at slot %d with name %s and purpose switch", i + 1, name);
				ONSTEP_AUX_POWER_OUTLET_MAPPING[ONSTEP_AUX_POWER_OUTLET_COUNT] = i + 1;
				ONSTEP_AUX_POWER_OUTLET_COUNT++;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Device at index %d not recognized", i + 1, name);
			}
		}
	}
	// after that we create the property and items for each type
	for (int i = 0; i < ONSTEP_AUX_HEATER_OUTLET_COUNT; i++) {
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, ONSTEP_AUX_HEATER_OUTLET_COUNT);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return;
		char heater_name[32];
		char heater_label[32];
		snprintf(heater_name, sizeof(heater_name), AUX_HEATER_OUTLET_ITEM_NAME, i + 1);
		snprintf(heater_label, sizeof(heater_label), "Heater #%d [%%]", i + 1);
		indigo_init_number_item(AUX_HEATER_OUTLET_PROPERTY->items, heater_name, heater_label, 0, 100, 1, 0);
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	for (int i = 0; i < ONSTEP_AUX_POWER_OUTLET_COUNT; i++) {
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, ONSTEP_AUX_POWER_OUTLET_COUNT);
		if (AUX_POWER_OUTLET_PROPERTY == NULL)
			return;
		char power_name[32];
		char power_label[32];
		snprintf(power_name, sizeof(power_name), AUX_POWER_OUTLET_ITEM_NAME, i + 1);
		snprintf(power_label, sizeof(power_label), "Outlet #%d", i + 1);
		indigo_init_switch_item(AUX_POWER_OUTLET_PROPERTY->items, power_name, power_label, true);
		indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	}
}

static void aux_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {

		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			result = meade_open(device->master_device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				meade_detect_mount(device->master_device);
			}
			if (MOUNT_TYPE_NYX_ITEM->sw.value) {
				indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
				indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
				indigo_set_timer(device, 0, nyx_aux_timer_callback, &PRIVATE_DATA->aux_timer);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				onstep_aux_connect(device);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_timer(device, 0, onstep_aux_timer_callback, &PRIVATE_DATA->aux_timer);
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);

		if (--PRIVATE_DATA->device_count == 0) {
			if (PRIVATE_DATA->keep_alive_timer) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->keep_alive_timer);
			}
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void onstep_aux_heater_outlet_handler(indigo_device *device) {
	for (int i = 0; i < ONSTEP_AUX_HEATER_OUTLET_COUNT; i++) {
		indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
		int val = MIN((int) round(item->number.value * 2.56), 255);
		char response[2];
		char command[14];
		int slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
		snprintf(command, sizeof(command), ":SXX%d,V%d#", slot, val);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "setting aux slot %d to %d", slot, val);
		meade_command(device, command, response, sizeof(response), 0);
		if (response[0] == '1') {
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void onstep_aux_power_outlet_handler(indigo_device *device) {
	for (int i = 0; i < ONSTEP_AUX_POWER_OUTLET_COUNT; i++) {
		indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
		bool val = item->sw.value;
		char response[2];
		char command[14];
		int slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
		snprintf(command, sizeof(command), ":SXX%d,V%d#", slot, val);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "setting aux slot %d to %d", slot, val);
		meade_command(device, command, response, sizeof(response), 0);
		if (response[0] == '1') {
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AUX_POWER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connect_callback, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, onstep_aux_heater_outlet_handler, NULL);
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, onstep_aux_power_outlet_handler, NULL);
		return INDIGO_OK;
	}

	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connect_callback(device);
	}
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

static void device_network_disconnection(indigo_device* device, indigo_timer_callback callback) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		callback(device);
		if (!PRIVATE_DATA->wifi_reset) {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;  // The alert state signals the unexpected disconnection
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			// Sending message as this update will not pass through the agent
			indigo_send_message(device, "Error: Device disconnected unexpectedly", device->name);
		}
	}
	// Otherwise not previously connected, nothing to do
}

// --------------------------------------------------------------------------------

static lx200_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;
static indigo_device *mount_focuser = NULL;
static indigo_device *mount_aux = NULL;

static void network_disconnection(__attribute__((unused)) indigo_device* device) {
	// Since all three devices share the same TCP connection,
	// process the disconnection on all three of them
	device_network_disconnection(mount, mount_connect_callback);
	device_network_disconnection(mount_guider, guider_connect_callback);
	device_network_disconnection(mount_focuser, focuser_connect_callback);
	device_network_disconnection(mount_aux, aux_connect_callback);
}

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
	static indigo_device mount_aux_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_LX200_AUX_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	 );

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	static indigo_device_match_pattern patterns[1] = { 0 };
	strcpy(patterns[0].vendor_string, "Pegasus Astro");
	strcpy(patterns[0].product_string, "NYX");
	INDIGO_REGISER_MATCH_PATTERNS(mount_template, patterns, 1);

	SET_DRIVER_INFO(info, "LX200 Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			tzset();
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
			mount_aux = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_aux_template);
			mount_aux->private_data = private_data;
			mount_aux->master_device = mount;
			indigo_attach_device(mount_aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			VERIFY_NOT_CONNECTED(mount_focuser);
			VERIFY_NOT_CONNECTED(mount_aux);
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
			if (mount_aux != NULL) {
				indigo_detach_device(mount_aux);
				free(mount_aux);
				mount_aux = NULL;
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
