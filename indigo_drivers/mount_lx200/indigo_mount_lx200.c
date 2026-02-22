// Copyright (c) 2016-2025 CloudMakers, s. r. o.
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO LX200 driver
 \file indigo_mount_lx200.c
 */

#define DRIVER_VERSION 0x03000031
#define DRIVER_NAME	"indigo_mount_lx200"

#define NYX_BASE64_THRESHOLD_VERSION "1.32.0"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_base64.h>

#include "indigo_mount_lx200.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#define PRIVATE_DATA				((lx200_private_data *)device->private_data)

#define MOUNT_TYPE_PROPERTY							(PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_DETECT_ITEM					(MOUNT_TYPE_PROPERTY->items+0)
#define MOUNT_TYPE_MEADE_ITEM						(MOUNT_TYPE_PROPERTY->items+1)
#define MOUNT_TYPE_10MICRONS_ITEM				(MOUNT_TYPE_PROPERTY->items+2)
#define MOUNT_TYPE_GEMINI_ITEM				 	(MOUNT_TYPE_PROPERTY->items+3)
#define MOUNT_TYPE_STARGO_ITEM					(MOUNT_TYPE_PROPERTY->items+4)
#define MOUNT_TYPE_STARGO2_ITEM					(MOUNT_TYPE_PROPERTY->items+5)
#define MOUNT_TYPE_AP_ITEM							(MOUNT_TYPE_PROPERTY->items+6)
#define MOUNT_TYPE_ON_STEP_ITEM					(MOUNT_TYPE_PROPERTY->items+7)
#define MOUNT_TYPE_AGOTINO_ITEM					(MOUNT_TYPE_PROPERTY->items+8)
#define MOUNT_TYPE_ZWO_ITEM				 			(MOUNT_TYPE_PROPERTY->items+9)
#define MOUNT_TYPE_NYX_ITEM				 			(MOUNT_TYPE_PROPERTY->items+10)
#define MOUNT_TYPE_OAT_ITEM				 			(MOUNT_TYPE_PROPERTY->items+11)
#define MOUNT_TYPE_TEEN_ASTRO_ITEM			(MOUNT_TYPE_PROPERTY->items+12)
#define MOUNT_TYPE_GENERIC_ITEM         (MOUNT_TYPE_PROPERTY->items+13)


#define MOUNT_TYPE_PROPERTY_NAME				"X_MOUNT_TYPE"
#define MOUNT_TYPE_DETECT_ITEM_NAME			"DETECT"
#define MOUNT_TYPE_MEADE_ITEM_NAME			"MEADE"
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
#define MOUNT_TYPE_TEEN_ASTRO_ITEM_NAME	"TEEN_ASTRO"
#define MOUNT_TYPE_GENERIC_ITEM_NAME		"GENERIC"

#define MOUNT_MODE_PROPERTY							(PRIVATE_DATA->alignment_mode_property)
#define EQUATORIAL_ITEM									(MOUNT_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM									(MOUNT_MODE_PROPERTY->items+1)

#define MOUNT_MODE_PROPERTY_NAME				"X_MOUNT_MODE"
#define EQUATORIAL_ITEM_NAME						"EQUATORIAL"
#define ALTAZ_MODE_ITEM_NAME						"ALTAZ"

#define ZWO_BUZZER_PROPERTY							(PRIVATE_DATA->zwo_buzzer_property)
#define ZWO_BUZZER_OFF_ITEM							(ZWO_BUZZER_PROPERTY->items+0)
#define ZWO_BUZZER_LOW_ITEM							(ZWO_BUZZER_PROPERTY->items+1)
#define ZWO_BUZZER_HIGH_ITEM						(ZWO_BUZZER_PROPERTY->items+2)

#define ZWO_BUZZER_PROPERTY_NAME				"X_ZWO_BUZZER"
#define ZWO_BUZZER_OFF_ITEM_NAME				"OFF"
#define ZWO_BUZZER_LOW_ITEM_NAME				"LOW"
#define ZWO_BUZZER_HIGH_ITEM_NAME				"HIGH"

#define NYX_WIFI_AP_PROPERTY						(PRIVATE_DATA->nyx_wifi_ap_property)
#define NYX_WIFI_AP_SSID_ITEM						(NYX_WIFI_AP_PROPERTY->items+0)
#define NYX_WIFI_AP_PASSWORD_ITEM				(NYX_WIFI_AP_PROPERTY->items+1)

#define NYX_WIFI_AP_PROPERTY_NAME				"X_NYX_WIFI_AP"
#define NYX_WIFI_AP_SSID_ITEM_NAME			"AP_SSID"
#define NYX_WIFI_AP_PASSWORD_ITEM_NAME	"AP_PASSWORD"

#define NYX_WIFI_CL_PROPERTY						(PRIVATE_DATA->nyx_wifi_cl_property)
#define NYX_WIFI_CL_SSID_ITEM						(NYX_WIFI_CL_PROPERTY->items+0)
#define NYX_WIFI_CL_PASSWORD_ITEM				(NYX_WIFI_CL_PROPERTY->items+1)

#define NYX_WIFI_CL_PROPERTY_NAME				"X_NYX_WIFI_CL"
#define NYX_WIFI_CL_SSID_ITEM_NAME			"CL_SSID"
#define NYX_WIFI_CL_PASSWORD_ITEM_NAME	"CL_PASSWORD"

#define NYX_WIFI_RESET_PROPERTY					(PRIVATE_DATA->nyx_wifi_reset_property)
#define NYX_WIFI_RESET_ITEM							(NYX_WIFI_RESET_PROPERTY->items+0)

#define NYX_WIFI_RESET_PROPERTY_NAME		"X_NYX_WIFI_RESET"
#define NYX_WIFI_RESET_ITEM_NAME				"RESET"

#define NYX_LEVELER_PROPERTY						(PRIVATE_DATA->nyx_leveler_property)
#define NYX_LEVELER_PICH_ITEM						(NYX_LEVELER_PROPERTY->items+0)
#define NYX_LEVELER_ROLL_ITEM						(NYX_LEVELER_PROPERTY->items+1)
#define NYX_LEVELER_COMPASS_ITEM				(NYX_LEVELER_PROPERTY->items+2)

#define NYX_LEVELER_PROPERTY_NAME				"X_NYX_LEVELER"
#define NYX_LEVELER_PICH_ITEM_NAME			"PICH"
#define NYX_LEVELER_ROLL_ITEM_NAME			"ROLL"
#define NYX_LEVELER_COMPASS_ITEM_NAME		"COMPASS"

#define AUX_WEATHER_PROPERTY            (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM    (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_PRESSURE_ITEM				(AUX_WEATHER_PROPERTY->items + 1)

#define AUX_INFO_PROPERTY								(PRIVATE_DATA->aux_info_property)
#define AUX_INFO_VOLTAGE_ITEM						(AUX_INFO_PROPERTY->items + 0)

// Onstep has eight auxiliary device slots (1-indexed) which can have user defined purposes
#define ONSTEP_AUX_DEVICE_COUNT					8
#define AUX_GROUP												"Powerbox"
#define AUX_POWER_OUTLET_PROPERTY				(PRIVATE_DATA->power_outlet_property)
#define AUX_HEATER_OUTLET_PROPERTY			(PRIVATE_DATA->heater_outlet_property)
#define ONSTEP_AUX_HEATER_OUTLET_MAPPING (PRIVATE_DATA->onstep_aux_power_outlet_slot_mapping)
#define ONSTEP_AUX_POWER_OUTLET_MAPPING	(PRIVATE_DATA->onstep_aux_heater_outlet_slot_mapping)

#define IS_PARKED (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PROPERTY->count == 2 && MOUNT_PARK_PARKED_ITEM->sw.value)

typedef enum {
	ONSTEP_AUX_NONE = 0, 		// Auxiliary slot is disabled
	ONSTEP_AUX_SWITCH = 1,	// Auxiliary slot is a on/off switch -> power outlet in indigo
	ONSTEP_AUX_ANALOG = 2		// Auxiliary slot is an analog / pwm output -> heater outlet in indigo
	//TODO implement Momentary Switch, Dew Heater and Intervalometer
} onstep_aux_device_purpose;

typedef struct {
	indigo_uni_handle *handle;
	int device_count;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	bool motioned;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
	indigo_property *mount_type_property;
	indigo_property *zwo_buzzer_property;
	indigo_property *nyx_wifi_ap_property;
	indigo_property *nyx_wifi_cl_property;
	indigo_property *nyx_wifi_reset_property;
	indigo_property *nyx_leveler_property;
	indigo_property *weather_property;
	indigo_property *aux_info_property;
	indigo_property *power_outlet_property;
	indigo_property *heater_outlet_property;
	bool slewing, tracking;
	bool parked, parking, unparking;
	bool homed, homing;
	double timeout;
	char response[128];
	bool use_dst_commands;
	long time_difference;
	int utc_offset;
	bool focus_aborted;
	int onstep_aux_power_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];	// maps power outlet property item index to onstep aux slot
	int onstep_aux_heater_outlet_slot_mapping[ONSTEP_AUX_DEVICE_COUNT];	// maps heater outlet property item index to onstep aux slot
} lx200_private_data;

// Compare two version strings in the format "major.minor.patch"
// Returns: -1 if version1 < version2, 0 if equal, 1 if version1 > version2

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
	if (token) {
		v1_major = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v1_minor = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v1_patch = atoi(token);
	}
	indigo_safe_free(v1_copy);
	token = strtok(v2_copy, ".");
	int v2_major = 0, v2_minor = 0, v2_patch = 0;
	if (token) {
		v2_major = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v2_minor = atoi(token);
	}
	token = strtok(NULL, ".");
	if (token) {
		v2_patch = atoi(token);
	}
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
	if (cp) {
		*cp = c1;
	}
}

static bool meade_validate_handle(indigo_device *device);

static bool meade_no_reply_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool meade_simple_reply_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, 1, "", "", INDIGO_DELAY(PRIVATE_DATA->timeout));
		if (!(MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_STARGO2_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value)) {
			// :SCMM/DD/YY# returns two delimiters PRIVATE_DATA->response:
			// "1Updating Planetary Data#                                #"
			// readout progress part
			if (result && !strncmp(command, ":SC", 3) && (MOUNT_TYPE_AP_ITEM->sw.value || *PRIVATE_DATA->response == '1')) {
				char progress[128];
				indigo_uni_read_section(PRIVATE_DATA->handle, progress, sizeof(progress), "#", "#", INDIGO_DELAY(0.1));
				indigo_uni_read_section(PRIVATE_DATA->handle, progress, sizeof(progress), "#", "#", INDIGO_DELAY(0.1));
			}
		}
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool meade_command(indigo_device *device, char *command, ...) {
	if (!meade_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "#", "#", INDIGO_DELAY(PRIVATE_DATA->timeout), INDIGO_DELAY(0.1));
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
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
	return meade_no_reply_command(device, buffer);
}

static void keep_alive_callback(indigo_device *device) {
	if (!IS_CONNECTED) { // Ping mount if master device (mount) is not connected
		meade_command(device, ":GR#");
		indigo_execute_handler_in(device, 5, keep_alive_callback);
	}
}

static bool meade_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "lx200")) {
		if (MOUNT_TYPE_NYX_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 19200, INDIGO_LOG_DEBUG);
		} else {
			PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
			if (PRIVATE_DATA->handle != NULL) {
				// sometimes the first command after power on in OnStep fails and just returns '0'
				// so we try two times for the default baudrate of 9600
				PRIVATE_DATA->timeout = 1;
				if ((!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) < 6) && (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) < 6)) {
					indigo_uni_close(&PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 19200, INDIGO_LOG_DEBUG);
					if (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) < 6) {
						indigo_uni_close(&PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
						if (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) < 6) {
							indigo_uni_close(&PRIVATE_DATA->handle);
							PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 230400, INDIGO_LOG_DEBUG);
							if (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) < 6) {
								indigo_uni_close(&PRIVATE_DATA->handle);
							}
						}
					}
				}
				PRIVATE_DATA->timeout = 3;
			}
		}
	} else {
		if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			PRIVATE_DATA->handle = indigo_uni_open_url(name, 9999, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
		} else {
			PRIVATE_DATA->handle = indigo_uni_open_url(name, 4030, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
		}
	}
	if (PRIVATE_DATA->handle != NULL) {
		if (PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
			indigo_uni_set_socket_nodelay_option(PRIVATE_DATA->handle);
			indigo_execute_handler(device, keep_alive_callback);
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		indigo_uni_discard(PRIVATE_DATA->handle);
		PRIVATE_DATA->timeout = 3;
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void meade_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
	PRIVATE_DATA->device_count = 0;
}

static bool meade_validate_handle(indigo_device *device) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!indigo_uni_is_valid(PRIVATE_DATA->handle)) {
		meade_close(device);
		indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------  mount commands

static bool meade_set_utc(indigo_device *device, time_t secs, int utc_offset) {
	PRIVATE_DATA->time_difference = time(NULL) - secs;
	time_t seconds = secs + utc_offset * 3600;
	struct tm tm;
	indigo_gmtime(&seconds, &tm);
	if (!meade_simple_reply_command(device, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (PRIVATE_DATA->use_dst_commands) {
		meade_no_reply_command(device, ":SH%d#", indigo_get_dst_state());
	}
	if (!meade_simple_reply_command(device, ":SG%+03d#", -utc_offset) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (!meade_simple_reply_command(device, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	return true;
}

static bool meade_get_utc(indigo_device *device, time_t *secs, int *utc_offset) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value || MOUNT_TYPE_GENERIC_ITEM->sw.value) {
		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		char separator[2];
		if (meade_command(device, ":GC#") && sscanf(PRIVATE_DATA->response, "%d%c%d%c%d", &tm.tm_mon, separator, &tm.tm_mday, separator, &tm.tm_year) == 5) {
			if (meade_command(device, ":GL#") && sscanf(PRIVATE_DATA->response, "%d%c%d%c%d", &tm.tm_hour, separator, &tm.tm_min, separator, &tm.tm_sec) == 5) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (meade_command(device, ":GG#")) {
					if (MOUNT_TYPE_AP_ITEM->sw.value && PRIVATE_DATA->response[0] == ':') {
						if (PRIVATE_DATA->response[1] == 'A') {
							switch (PRIVATE_DATA->response[2]) {
								case '1':
									strcpy(PRIVATE_DATA->response, "-05");
									break;
								case '2':
									strcpy(PRIVATE_DATA->response, "-04");
									break;
								case '3':
									strcpy(PRIVATE_DATA->response, "-03");
									break;
								case '4':
									strcpy(PRIVATE_DATA->response, "-02");
									break;
								case '5':
									strcpy(PRIVATE_DATA->response, "-01");
									break;
							}
						} else if (PRIVATE_DATA->response[1] == '@') {
							switch (PRIVATE_DATA->response[2]) {
								case '4':
									strcpy(PRIVATE_DATA->response, "-12");
									break;
								case '5':
									strcpy(PRIVATE_DATA->response, "-11");
									break;
								case '6':
									strcpy(PRIVATE_DATA->response, "-10");
									break;
								case '7':
									strcpy(PRIVATE_DATA->response, "-09");
									break;
								case '8':
									strcpy(PRIVATE_DATA->response, "-08");
									break;
								case '9':
									strcpy(PRIVATE_DATA->response, "-07");
									break;
							}
						} else if (PRIVATE_DATA->response[1] == '0') {
							strcpy(PRIVATE_DATA->response, "-06");
						}
					}
					*utc_offset = -atoi(PRIVATE_DATA->response);
					*secs = indigo_timegm(&tm) - *utc_offset * 3600;
					PRIVATE_DATA->time_difference = time(NULL) - *secs;
					return true;
				}
			}
		}
	} else {
		*secs = time(NULL);
		PRIVATE_DATA->time_difference = 0;
	}
	return true;
}

static void meade_get_site(indigo_device *device, double *latitude, double *longitude) {
	if (MOUNT_TYPE_STARGO2_ITEM->sw.value || MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		return;
	}
	if (meade_command(device, ":Gt#")) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, 't', '*');
		}
		*latitude = indigo_stod(PRIVATE_DATA->response);
	}
	if (meade_command(device, ":Gg#")) {
		if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			str_replace(PRIVATE_DATA->response, 'g', '*');
		}
		*longitude = indigo_stod(PRIVATE_DATA->response);
		if (*longitude < 0) {
			*longitude += 360;
		}
		// LX200 protocol returns negative longitude for the east
		*longitude = 360 - *longitude;
	}
}

static bool meade_set_site(indigo_device *device, double latitude, double longitude, double elevation) {
	bool result = true;
	if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		return false;
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_simple_reply_command(device, indigo_dtos(latitude, "%+03d*%02d:%02d"));
		result = MOUNT_TYPE_STARGO_ITEM->sw.value; // ignore result for Avalon StarGO
	} else {
		result = meade_simple_reply_command(device, ":St%s#", indigo_dtos(latitude, "%+03d*%02d")) && *PRIVATE_DATA->response == '1';
	}
	// LX200 protocol expects negative longitude for the east
	longitude = 360 - fmod(longitude + 360, 360);
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		result = meade_simple_reply_command(device, ":Sg%s#", indigo_dtos(longitude, "%+04d*%02d:%02d"));
		result = MOUNT_TYPE_STARGO_ITEM->sw.value; // ignore result for Avalon StarGO
	} else {
		result = meade_simple_reply_command(device, ":Sg%s#", indigo_dtos(longitude, "%03d*%02d")) && *PRIVATE_DATA->response == '1';
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		result = meade_simple_reply_command(device, ":Sv%.1f#", elevation) && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool meade_get_coordinates(indigo_device *device, double *ra, double *dec) {
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (meade_command(device, ":GRH#")) {
			*ra = indigo_stod(PRIVATE_DATA->response);
			if (meade_command(device, ":GDH#")) {
				*dec = indigo_stod(PRIVATE_DATA->response);
				return true;
			}
		}
	} else if (meade_command(device, ":GR#")) {
		if (strlen(PRIVATE_DATA->response) < 8) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
				meade_command(device, ":P#");
				meade_command(device, ":GR#");
			} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
				meade_no_reply_command(device, ":U1#");
				meade_command(device, ":GR#");
			} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
				meade_no_reply_command(device, ":U#");
				meade_command(device, ":GR#");
			}
		}
		*ra = indigo_stod(PRIVATE_DATA->response);
		if (meade_command(device, ":GD#")) {
			*dec = indigo_stod(PRIVATE_DATA->response);
			return true;
		}
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on);

static bool meade_slew(indigo_device *device, double ra, double dec) {
	if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
			meade_set_tracking(device, true);
		}
	}
	if (!meade_simple_reply_command(device, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f")) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (!meade_simple_reply_command(device, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f")) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (!meade_simple_reply_command(device, ":MS#") || *PRIVATE_DATA->response != '0') {
		if (MOUNT_TYPE_ZWO_ITEM->sw.value && *PRIVATE_DATA->response == 'e') {
			int error_code = 0;
			sscanf(PRIVATE_DATA->response, "e%d", &error_code);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, "Error: %s", message);
			}
		}
		if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			int error_code = atoi(PRIVATE_DATA->response);
			char *message = meade_error_string(device, error_code);
			if (message) {
				indigo_send_message(device, "Error: %s", message);
			}
		}
		return false;
	}
	return true;
}

static bool meade_sync(indigo_device *device, double ra, double dec) {
	if (!meade_simple_reply_command(device, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f")) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (!meade_simple_reply_command(device, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f")) || *PRIVATE_DATA->response != '1') {
		return false;
	}
	if (!meade_command(device, ":CM#") || *PRIVATE_DATA->response == 0) {
		return false;
	}
	if (MOUNT_TYPE_ZWO_ITEM->sw.value && *PRIVATE_DATA->response == 'e') {
		int error_code = 0;
		sscanf(PRIVATE_DATA->response, "e%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, "Error: %s", message);
		}
		return false;
	}
	if (MOUNT_TYPE_NYX_ITEM->sw.value && *PRIVATE_DATA->response == 'E') {
		int error_code = 0;
		sscanf(PRIVATE_DATA->response, "E%d", &error_code);
		char *message = meade_error_string(device, error_code);
		if (message) {
			indigo_send_message(device, "Error: %s", message);
		}
		return false;
	}
	return true;
}

static bool meade_pec(indigo_device *device, bool on) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		return meade_no_reply_command(device, on ? "$QZ+" : "$QZ-");
	}
	return false;
}

static bool meade_set_guide_rate(indigo_device *device, int ra, int dec) {
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (meade_no_reply_command(device, ":X20%02d#", ra)) {
			return meade_no_reply_command(device, ":X21%02d#", dec);
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		// asi miunt has one guide rate for ra and dec
		if (ra < 10) {
			ra = 10;
		}
		if (ra > 90) {
			ra = 90;
		}
		double rate = ra / 100.0;
		return (meade_no_reply_command(device, ":Rg%.1lf#", rate));
	}
	return false;
}

static bool meade_get_guide_rate(indigo_device *device, int *ra, int *dec) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		bool res = meade_command(device, ":Ggr#");
		if (!res) {
			return false;
		}
		double rate = 0;
		int parsed = sscanf(PRIVATE_DATA->response, "%lf", &rate);
		if (parsed != 1) {
			return false;
		}
		*ra = *dec = (int)(rate * 100);
		return true;
	}
	return false;
}

static bool meade_set_tracking(indigo_device *device, bool on) {
	if (on) { // TBD
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 192, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":X122#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return meade_no_reply_command(device, ":RT2#");
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_no_reply_command(device, ":RT1#");
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_no_reply_command(device, ":RT0#");
			}
		} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_command(device, ":Te#") && *PRIVATE_DATA->response == '1';
		} else if (MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
				return meade_command(device, ":TQ#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
				return meade_command(device, ":TS#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
				return meade_command(device, ":TL#:Te#") && *PRIVATE_DATA->response == '1';
			} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
				return meade_command(device, ":TK#:Te#") && *PRIVATE_DATA->response == '1';
			}
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT1#") && *PRIVATE_DATA->response == '1';
		} else {
			return meade_no_reply_command(device, ":AP#");
		}
	} else {
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 191, "");
		} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":X120#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT9#");
		} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
			return meade_no_reply_command(device, ":Td#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_command(device, ":MT0#") && *PRIVATE_DATA->response == '1';
		} else {
			return meade_no_reply_command(device, ":AL#");
		}
	}
	return false;
}

static bool meade_set_tracking_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
		PRIVATE_DATA->lastTrackRate = 'q';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 131, "");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT2#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS1.000#");
		} else {
			return meade_no_reply_command(device, ":TQ#");
		}
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
		PRIVATE_DATA->lastTrackRate = 's';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 134, "");
		} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TSOLAR#");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT1#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS0.997#");
		} else {
			return meade_no_reply_command(device, ":TS#");
		}
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
		PRIVATE_DATA->lastTrackRate = 'l';
		if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
			return gemini_set(device, 133, "");
		} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
			return meade_no_reply_command(device, ":RT0#");
		} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
			return meade_no_reply_command(device, ":XSS0.965#");
		} else {
			return meade_no_reply_command(device, ":TL#");
		}
	} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'k') {
		PRIVATE_DATA->lastTrackRate = 'k';
		if (MOUNT_TYPE_NYX_ITEM->sw.value) {
			return meade_no_reply_command(device, ":TK#");
		}
	}
	return true;
}

static bool meade_get_tracking_rate(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		if (meade_command(device, ":GT#")) {
			double rate = atof(PRIVATE_DATA->response);
			if (rate <= 57.9) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (rate <= 60.0) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			} else if (rate <= 60.14) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
			} else {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			}
			return true;
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		if (meade_command(device, ":GT#")) {
			if (strchr(PRIVATE_DATA->response, '0')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			} else if (strchr(PRIVATE_DATA->response, '1')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (strchr(PRIVATE_DATA->response, '2')) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			}
			return true;
		}
	}
	return false;
}

static bool meade_set_slew_rate(indigo_device *device) {
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":RG2#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":RC0#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":RC1#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":RC3#");
		}
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":R1#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":R4#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":R7#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":R9#");
		}
	} else {
		if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
			PRIVATE_DATA->lastSlewRate = 'g';
			return meade_no_reply_command(device, ":RG#");
		} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
			PRIVATE_DATA->lastSlewRate = 'c';
			return meade_no_reply_command(device, ":RC#");
		} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
			PRIVATE_DATA->lastSlewRate = 'm';
			return meade_no_reply_command(device, ":RM#");
		} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
			PRIVATE_DATA->lastSlewRate = 's';
			return meade_no_reply_command(device, ":RS#");
		}
	}
	return true;
}

static bool meade_motion_dec(indigo_device *device) {
	bool stopped = true;
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (PRIVATE_DATA->lastMotionNS == 'n' || PRIVATE_DATA->lastMotionNS == 's') {
			stopped = meade_no_reply_command(device, ":Q#");
		}
	} else {
		if (PRIVATE_DATA->lastMotionNS == 'n') {
			stopped = meade_no_reply_command(device, ":Qn#");
		} else if (PRIVATE_DATA->lastMotionNS == 's')
			stopped = meade_no_reply_command(device, ":Qs#");
	}
	if (stopped) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 'n';
			return meade_no_reply_command(device, ":Mn#");
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 's';
			return meade_no_reply_command(device, ":Ms#");
		} else {
			PRIVATE_DATA->lastMotionNS = 0;
		}
	}
	return stopped;
}

static bool meade_motion_ra(indigo_device *device) {
	bool stopped = true;
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		if (PRIVATE_DATA->lastMotionWE == 'w' || PRIVATE_DATA->lastMotionWE == 'e') {
			stopped = meade_no_reply_command(device, ":Q#");
		}
	} else {
		if (PRIVATE_DATA->lastMotionWE == 'w') {
			stopped = meade_no_reply_command(device, ":Qw#");
		} else if (PRIVATE_DATA->lastMotionWE == 'e')
			stopped = meade_no_reply_command(device, ":Qe#");
	}
	if (stopped) {
		if (MOUNT_MOTION_WEST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'w';
			return meade_no_reply_command(device, ":Mw#");
		} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'e';
			return meade_no_reply_command(device, ":Me#");
		} else {
			PRIVATE_DATA->lastMotionWE = 0;
		}
	}
	return stopped;
}

static bool meade_park(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hP#");
	}
	if (MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		return meade_no_reply_command(device, ":KA#");
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hC#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X362#") && strcmp(PRIVATE_DATA->response, "pB") == 0;
	}
	return false;
}

static bool meade_unpark(indigo_device *device) {
	if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hU#");
	}
	if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hW#");
	}
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value) {
		return meade_no_reply_command(device, ":PO#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X370#") && strcmp(PRIVATE_DATA->response, "p0") == 0;
	}
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hR#");
	}
	return false;
}

static bool meade_park_set(indigo_device *device) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_simple_reply_command(device, ":hQ#") || *PRIVATE_DATA->response != '1';
	}
	return false;
}

static bool meade_home(indigo_device *device) {
	if (MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hF#");
	}
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_ZWO_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value || MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hC#");
	}
	if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		return meade_command(device, ":X361#") && strcmp(PRIVATE_DATA->response, "pA") == 0;
	}
	return false;
}

static bool meade_home_set(indigo_device *device) {
	if (MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_NYX_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hF#");
	}
	if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		return meade_no_reply_command(device, ":hB#");
	}
	return false;
}

static bool meade_stop(indigo_device *device) {
	return meade_no_reply_command(device, ":Q#");
}

static bool meade_guide_dec(indigo_device *device, int north, int south) {
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		if (north > 0) {
			return meade_no_reply_command(device, ":Mn%03d#", north);
		} else if (south > 0) {
			return meade_no_reply_command(device, ":Ms%03d#", south);
		}
	} else {
		if (north > 0) {
			return meade_no_reply_command(device, ":Mgn%04d#", north);
		} else if (south > 0) {
			return meade_no_reply_command(device, ":Mgs%04d#", south);
		}
	}
	return false;
}

static bool meade_guide_ra(indigo_device *device, int west, int east) {
	if (MOUNT_TYPE_AP_ITEM->sw.value) {
		if (west > 0) {
			return meade_no_reply_command(device, ":Mw%03d#", west);
		} else if (east > 0) {
			return meade_no_reply_command(device, ":Me%03d#", east);
		}
	} else {
		if (west > 0) {
			return meade_no_reply_command(device, ":Mgw%04d#", west);
		} else if (east > 0) {
			return meade_no_reply_command(device, ":Mge%04d#", east);
		}
	}
	return false;
}

static bool meade_focus_abort(indigo_device *device) {
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (meade_no_reply_command(device, ":FQ#")) {
			return true;
		}
	}
	return false;
}

static bool meade_focus_rel(indigo_device *device, bool slow, int steps) {
	if (steps == 0) {
		return true;
	}
	PRIVATE_DATA->focus_aborted = false;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_no_reply_command(device, slow ? ":FS#" : ":FF#"))
			return false;
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
		if (!meade_no_reply_command(device, steps > 0 ? ":F+#" : ":F-#"))
			return false;
		if (steps < 0) {
			steps = - steps;
		}
		for (int i = 0; i < steps; i++) {
			if (PRIVATE_DATA->focus_aborted) {
				return meade_focus_abort(device);
			}
			indigo_usleep(1000);
		}
		if (!meade_no_reply_command(device, ":FQ#"))
			return false;
		return true;
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		if (!meade_no_reply_command(device, ":FR%+d#", steps))
			return false;
		while (true) {
			if (PRIVATE_DATA->focus_aborted) {
				return meade_focus_abort(device);
			}
			indigo_usleep(100000);
			if (!meade_command(device, ":FT#"))
				return false;
			if (*PRIVATE_DATA->response == 'S') {
				break;
			}
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

static bool meade_detect_generic_mount(indigo_device *device) {
	// These commands are found in the classic LX200 Instruction Manual, and the compatible mounts refer to that manual.
	if (!meade_command(device, ":GR#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GR# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GD#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GD# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GC#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GC# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GL#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GL# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GG#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GG# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":GS#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":GS# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":Gg#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":Gg# failed."));
		return false;
	}
	PRIVATE_DATA->response[0] = 0;
	if (!meade_command(device, ":Gt#") || strlen(PRIVATE_DATA->response) == 0) {
		INDIGO_LOG(indigo_log(":Gt# failed."));
		return false;
	}
	return true;
}

static bool meade_detect_mount(indigo_device *device) {
	bool result = true;
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: %s", PRIVATE_DATA->response);
		strncpy(PRIVATE_DATA->product, PRIVATE_DATA->response, 64);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (!strncmp(PRIVATE_DATA->product, "LX", 2) || !strncmp(PRIVATE_DATA->product, "Autostar", 8)) {
			indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_MEADE_ITEM, true);
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
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 1;
	MOUNT_PARK_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Meade");
	if (meade_command(device, ":GVF#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Version: %s", PRIVATE_DATA->response);
		char *sep = strchr(PRIVATE_DATA->response, '|');
		if (sep != NULL) {
			*sep = 0;
		}
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GW#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Status: %s", PRIVATE_DATA->response);
		MOUNT_MODE_PROPERTY->hidden = false;
		if (PRIVATE_DATA->response[0] == 'P' || PRIVATE_DATA->response[0] == 'G') {
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		} else {
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
		}
	}
	if (meade_command(device, ":GH#")) {
		PRIVATE_DATA->use_dst_commands = *PRIVATE_DATA->response != 0;
	}
}

static void meade_update_meade_state(indigo_device *device) {
	if (meade_command(device, ":D#")) {
		PRIVATE_DATA->slewing = *PRIVATE_DATA->response;
	}
	if (meade_command(device, ":GW#")) {
		PRIVATE_DATA->tracking = PRIVATE_DATA->response[1] == 'T';
	}
}

static void meade_init_10microns_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "10Micron");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_no_reply_command(device, ":EMUAP#");
	meade_no_reply_command(device, ":U1#");
}

static void meade_update_10microns_state(indigo_device *device) {
	PRIVATE_DATA->tracking = PRIVATE_DATA->slewing = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->unparking = PRIVATE_DATA->homing = false;
	if (meade_command(device, ":Gstat#")) {
		switch (atoi(PRIVATE_DATA->response)) {
			case 0:
				PRIVATE_DATA->tracking = true;
				break;
			case 2:
				PRIVATE_DATA->parking = true;
				break;
			case 3:
				PRIVATE_DATA->unparking = true;
				break;
			case 4:
				PRIVATE_DATA->homing = true;
				break;
			case 5:
				PRIVATE_DATA->parked = true;
				break;
			case 6:
				PRIVATE_DATA->slewing = true;
				break;
		}
	}
}

static void meade_init_gemini_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Losmandy");
	meade_no_reply_command(device, ":p0#");
}

static void meade_update_gemini_state(indigo_device *device) {
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parked = PRIVATE_DATA->parking = false;
	if (meade_command(device, ":Gv#")) {
		switch (PRIVATE_DATA->response[0]) {
			case 'S':
			case 'C':
				PRIVATE_DATA->slewing = true;
				break;
			case 'T':
			case 'G':
				PRIVATE_DATA->tracking = true;
				break;
		}
	}
	if (meade_command(device, ":h?#")) {
		switch (PRIVATE_DATA->response[0]) {
			case '1':
				PRIVATE_DATA->parked = true;
				break;
			case '2':
				PRIVATE_DATA->parking = true;
				break;
		}
	}
	if (meade_command(device, ":Gm#")) {
		if (PRIVATE_DATA->response[0] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[0] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_stargo_mount(indigo_device *device) {
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	meade_simple_reply_command(device, ":TTSFh#");
	if (meade_command(device, ":X22#")) {
		int ra, dec;
		if (sscanf(PRIVATE_DATA->response, "%db%d#", &ra, &dec) == 2) {
			MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target = ra;
			MOUNT_GUIDE_RATE_DEC_ITEM->number.value = MOUNT_GUIDE_RATE_DEC_ITEM->number.target = dec;
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	meade_simple_reply_command(device, ":TTSFd#"); // disable meridian flip
}

static void meade_update_stargo_state(indigo_device *device) {
	if (meade_command(device, ":X34#")) {
		PRIVATE_DATA->slewing = (PRIVATE_DATA->response[1] == '5' || PRIVATE_DATA->response[2] == '5');
		PRIVATE_DATA->tracking = PRIVATE_DATA->response[1] == '1' && PRIVATE_DATA->response[2] == '1';
	}
	PRIVATE_DATA->parked = PRIVATE_DATA->parking = false;
	if (meade_command(device, ":X38#")) {
		switch (PRIVATE_DATA->response[1]) {
			case '2':
				PRIVATE_DATA->parked = true;
				break;
			case 'B':
				PRIVATE_DATA->parking = true;
				break;
		}
	}
}

static void meade_init_stargo2_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 2;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Avalon");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Avalon StarGO2");
}

static void meade_init_ap_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "AstroPhysics");
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	meade_no_reply_command(device, "#");
	meade_no_reply_command(device, ":U#");
	meade_no_reply_command(device, ":Br 00:00:00#");
}

static void meade_init_onstep_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "On-Step");
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Model: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":$QZ?#")) {
		indigo_set_switch(MOUNT_PEC_PROPERTY, PRIVATE_DATA->response[0] == 'P' ? MOUNT_PEC_ENABLED_ITEM : MOUNT_PEC_DISABLED_ITEM, true);
	}
}

static void meade_update_onstep_state(indigo_device *device) {
	PRIVATE_DATA->tracking = PRIVATE_DATA->slewing = PRIVATE_DATA->parked = PRIVATE_DATA->parking = PRIVATE_DATA->homed = false;
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
		}
		if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'P')) {
			PRIVATE_DATA->parked = true;
		} else if (strchr(PRIVATE_DATA->response, 'I')) {
			PRIVATE_DATA->parking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
		if (strchr(PRIVATE_DATA->response, 'o')) {
			MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_IDLE_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'W')) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		} else if (strchr(PRIVATE_DATA->response, 'T')) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
}

static void meade_init_agotino_mount(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->hidden = true;
	MOUNT_SLEW_RATE_PROPERTY->hidden = true;
	MOUNT_MOTION_RA_PROPERTY->hidden = true;
	MOUNT_MOTION_DEC_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "aGotino");
}

static void meade_init_zwo_mount(indigo_device *device) {
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	ZWO_BUZZER_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ZWO");
	if (meade_command(device, ":GV#")) {
		strcpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
		strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	MOUNT_GUIDE_RATE_DEC_ITEM->number.min = MOUNT_GUIDE_RATE_RA_ITEM->number.min = 10;
	MOUNT_GUIDE_RATE_DEC_ITEM->number.max = MOUNT_GUIDE_RATE_RA_ITEM->number.max = 90;
	int ra_rate, dec_rate;
	if (meade_get_guide_rate(device, &ra_rate, &dec_rate)) {
		MOUNT_GUIDE_RATE_RA_ITEM->number.target = MOUNT_GUIDE_RATE_RA_ITEM->number.value = (double)ra_rate;
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target = MOUNT_GUIDE_RATE_DEC_ITEM->number.value = (double)dec_rate;
	}
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'G')) {
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, 'Z')) {
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
		}
	}
	if (meade_command(device, ":GBu#")) {
		if (strchr(PRIVATE_DATA->response, '0')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_OFF_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, '1')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_LOW_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, '2')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_HIGH_ITEM, true);
		}
	}
}

static void meade_update_zwo_state(indigo_device *device) {
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->homed = false;
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
		} else if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
	}
	if (meade_command(device, ":Gm#")) {
		if (PRIVATE_DATA->response[0] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[0] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_nyx_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 4;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = true;
	NYX_WIFI_AP_PROPERTY->hidden = false;
	NYX_WIFI_CL_PROPERTY->hidden = false;
	NYX_WIFI_RESET_PROPERTY->hidden = false;
	NYX_LEVELER_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "PegasusAstro");
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (meade_command(device, ":GVP#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Model: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response);
	}
	if (!meade_simple_reply_command(device, ":SXEM,1#") || *PRIVATE_DATA->response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't set EQ mode");
	}
	if (!meade_simple_reply_command(device, ":SX91,U#") || *PRIVATE_DATA->response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't unlock brake");
	}
	char *separator = NULL;
	*NYX_WIFI_AP_SSID_ITEM->text.value = 0;
	*NYX_WIFI_AP_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WL>#") && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_AP_SSID_ITEM->text.value, PRIVATE_DATA->response, INDIGO_VALUE_SIZE);
		strncpy(NYX_WIFI_AP_PASSWORD_ITEM->text.value, separator, INDIGO_VALUE_SIZE);
	}
	*NYX_WIFI_CL_SSID_ITEM->text.value = 0;
	*NYX_WIFI_CL_PASSWORD_ITEM->text.value = 0;
	if (meade_command(device, ":WLD#") && *PRIVATE_DATA->response != ':' && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		strncpy(NYX_WIFI_CL_SSID_ITEM->text.value, PRIVATE_DATA->response, INDIGO_VALUE_SIZE);
		indigo_send_message(device, "Mount is connected to network '%s' with IP address %s", PRIVATE_DATA->response, separator);
	}
	if (meade_command(device, ":GX9D#") && (separator = strchr(PRIVATE_DATA->response, ':'))) {
		*separator++ = 0;
		NYX_LEVELER_PICH_ITEM->number.value = atof(PRIVATE_DATA->response);
		NYX_LEVELER_ROLL_ITEM->number.value = atof(separator);
	}
	if (meade_command(device, ":GX9E#")) {
		NYX_LEVELER_COMPASS_ITEM->number.value = atof(PRIVATE_DATA->response);
	}
	meade_no_reply_command(device, ":RE00.03#:RA00.03#");
}

static void meade_update_nyx_state(indigo_device *device) {
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->homed = PRIVATE_DATA->homing = false;
	if (meade_command(device, ":GU#")) {
		if (strchr(PRIVATE_DATA->response, 'N') == NULL) {
			PRIVATE_DATA->slewing = true;
		} else if (strchr(PRIVATE_DATA->response, 'n') == NULL) {
			PRIVATE_DATA->tracking = true;
		}
		if (strchr(PRIVATE_DATA->response, 'I')) {
			PRIVATE_DATA->parking = true;
		} else if (strchr(PRIVATE_DATA->response, 'P')) {
			PRIVATE_DATA->parked = true;
		}
		if (strchr(PRIVATE_DATA->response, 'h')) {
			PRIVATE_DATA->homing = true;
		} else if (strchr(PRIVATE_DATA->response, 'H')) {
			PRIVATE_DATA->homed = true;
		}
		if (strchr(PRIVATE_DATA->response, 'W') && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (strchr(PRIVATE_DATA->response, 'T') && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
	char *colon;
	if (meade_command(device, ":GX9D#") && (colon = strchr(PRIVATE_DATA->response, ':'))) {
		*colon++ = 0;
		NYX_LEVELER_PICH_ITEM->number.value = atof(PRIVATE_DATA->response);
		NYX_LEVELER_ROLL_ITEM->number.value = atof(colon);
	}
	if (meade_command(device, ":GX9E#")) {
		NYX_LEVELER_COMPASS_ITEM->number.value = atof(PRIVATE_DATA->response);
	}
}

static void meade_init_oat_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 1;
	MOUNT_PARK_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_PARK_PARKED_ITEM->sw.value = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "OpenAstroTech");
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

static void meade_update_oat_state(indigo_device *device) {
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->homing = false;
	if (meade_command(device, ":GX#")) {
		if (!strncmp(PRIVATE_DATA->response, "Slew", 4)) {
			PRIVATE_DATA->slewing = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Tracking", 8)) {
			PRIVATE_DATA->tracking = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Parking", 7)) {
			PRIVATE_DATA->parking = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Parked", 6)) {
			PRIVATE_DATA->parked = true;
		} else if (!strncmp(PRIVATE_DATA->response, "Homing", 7)) {
			PRIVATE_DATA->homing = true;
		}
	}
}

static void meade_init_teenastro_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->hidden = false;
	MOUNT_PARK_SET_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->hidden = false;
	MOUNT_HOME_SET_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 3;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_PEC_PROPERTY->hidden = false;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "TeenAstro");
	if (meade_command(device, ":GVN#")) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Firmware: %s", PRIVATE_DATA->response);
		INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, PRIVATE_DATA->response);
	}
}

static void meade_update_teenastro_state(indigo_device *device) {
	if (meade_command(device, ":GXI#")) {
		if (PRIVATE_DATA->response[0] == '1') {
			PRIVATE_DATA->tracking = true;
		} else if (PRIVATE_DATA->response[0] == '2' || PRIVATE_DATA->response[0] == '3') {
			PRIVATE_DATA->slewing = true;
		}
		if (PRIVATE_DATA->response[2] == 'P') {
			PRIVATE_DATA->parked = true;
		} else if (PRIVATE_DATA->response[2] == 'I') {
			PRIVATE_DATA->parking = true;
		}
		if (PRIVATE_DATA->response[3] == 'H') {
			PRIVATE_DATA->homed = true;
		}
		if (PRIVATE_DATA->response[13] == 'W' && !MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (PRIVATE_DATA->response[13] == 'E' && !MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
}

static void meade_init_generic_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_INFO_PROPERTY->count = 1;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
}

static void meade_update_generic_state(indigo_device *device) {
	// After Track or Slew
	// NOTE: Distance bar `:D#` is not working (e.g. classic LX200).
	if (fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value - PRIVATE_DATA->lastRA) > 2.0/60.0 || fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value - PRIVATE_DATA->lastDec) > 2.0/60.0) {
		PRIVATE_DATA->slewing = true;
	}
}

static void meade_init_mount(indigo_device *device) {
	MOUNT_MODE_PROPERTY->hidden = true;
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
	MOUNT_UTC_TIME_PROPERTY->hidden = true;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->hidden = false;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_PARK_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
	MOUNT_PARK_PARKED_ITEM->sw.value = true;
	MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
	MOUNT_PARK_SET_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->count = 1;
	MOUNT_HOME_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_HOME_SET_PROPERTY->hidden = true;
	MOUNT_HOME_SET_PROPERTY->count = 2;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->count = 3;
	MOUNT_SLEW_RATE_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->hidden = true;
	MOUNT_MOTION_RA_PROPERTY->hidden = false;
	MOUNT_MOTION_DEC_PROPERTY->hidden = false;
	MOUNT_INFO_PROPERTY->count = 3;
	strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Unknown");
	strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "Unknown");
	strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "Unknown");
	ZWO_BUZZER_PROPERTY->hidden = true;
	NYX_WIFI_AP_PROPERTY->hidden = true;
	NYX_WIFI_CL_PROPERTY->hidden = true;
	NYX_WIFI_RESET_PROPERTY->hidden = true;
	NYX_LEVELER_PROPERTY->hidden = true;
	PRIVATE_DATA->use_dst_commands = false;
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parking = PRIVATE_DATA->parked = PRIVATE_DATA->unparking = PRIVATE_DATA->homing = PRIVATE_DATA->homed = false;
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		meade_init_meade_mount(device);
		meade_update_meade_state(device);
	} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		meade_init_10microns_mount(device);
		meade_update_10microns_state(device);
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		meade_init_gemini_mount(device);
		meade_update_gemini_state(device);
	} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_init_stargo_mount(device);
		meade_update_stargo_state(device);
	} else if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
		meade_init_stargo2_mount(device);
	} else if (MOUNT_TYPE_AP_ITEM->sw.value) {
		meade_init_ap_mount(device);
	} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
		meade_init_onstep_mount(device);
		meade_update_onstep_state(device);
	} else if (MOUNT_TYPE_AGOTINO_ITEM->sw.value) {
		meade_init_agotino_mount(device);
	} else if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		meade_init_zwo_mount(device);
		meade_update_zwo_state(device);
	} else if (MOUNT_TYPE_NYX_ITEM->sw.value) {
		meade_init_nyx_mount(device);
		meade_update_nyx_state(device);
	} else if (MOUNT_TYPE_OAT_ITEM->sw.value) {
		meade_init_oat_mount(device);
		meade_update_oat_state(device);
	} else if (MOUNT_TYPE_TEEN_ASTRO_ITEM->sw.value) {
		meade_init_teenastro_mount(device);
		meade_update_teenastro_state(device);
	} else {
		meade_init_generic_mount(device);
		meade_update_generic_state(device);
	}
	meade_get_tracking_rate(device);
	if (PRIVATE_DATA->parking) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->unparking) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->parked) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (PRIVATE_DATA->homing) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
		MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (PRIVATE_DATA->homed) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
	} else if (MOUNT_HOME_PROPERTY->count == 2) {
		indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (PRIVATE_DATA->tracking) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
	} else {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	}
	time_t secs = 0;
	meade_get_utc(device, &secs, &PRIVATE_DATA->utc_offset);
	time_t now = time(NULL);
	if (labs(secs - now) > 24 * 60 * 60) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount is not initialized, initializing...");
		meade_set_utc(device, now, indigo_get_utc_offset());
		meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value);
	} else {
		double latitude = 0, longitude = 0;
		meade_get_site(device, &latitude, &longitude);
		MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
	}
}

static void meade_update_mount_state(indigo_device *device) {
	double ra = 0, dec = 0;
	if (meade_get_coordinates(device, &ra, &dec)) {
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		PRIVATE_DATA->lastRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		PRIVATE_DATA->lastDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	}
	if (MOUNT_TYPE_MEADE_ITEM->sw.value) {
		meade_update_meade_state(device);
	} else if (MOUNT_TYPE_10MICRONS_ITEM->sw.value) {
		meade_update_10microns_state(device);
	} else if (MOUNT_TYPE_GEMINI_ITEM->sw.value) {
		meade_update_gemini_state(device);
	} else if (MOUNT_TYPE_STARGO_ITEM->sw.value) {
		meade_update_stargo_state(device);
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
	if (MOUNT_SIDE_OF_PIER_PROPERTY->do_update) {
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
	}
	if (!NYX_LEVELER_PROPERTY->hidden && NYX_LEVELER_PROPERTY->do_update) {
		indigo_update_property(device, NYX_LEVELER_PROPERTY, NULL);
	}
	
	//		if (MOUNT_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE) {
	//			if (MOUNT_MOTION_DEC_PROPERTY->state != INDIGO_BUSY_STATE && MOUNT_MOTION_RA_PROPERTY->state != INDIGO_BUSY_STATE && MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
	//				MOUNT_MOTION_NORTH_ITEM->sw.value = false;
	//				MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	//				MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//				indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	//				MOUNT_MOTION_WEST_ITEM->sw.value = false;
	//				MOUNT_MOTION_EAST_ITEM->sw.value = false;
	//				MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//				indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	//				if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
	//					MOUNT_HOME_PROPERTY->state=INDIGO_OK_STATE;
	//					indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
	//				}
	//				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	//				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	//				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//				indigo_update_coordinates(device, NULL);
	//				MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//				indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
	//			}
	//		}
	sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", PRIVATE_DATA->utc_offset);
	indigo_timetoisogm(time(NULL) - PRIVATE_DATA->time_difference, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		meade_update_mount_state(device);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = PRIVATE_DATA->slewing ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (PRIVATE_DATA->tracking && !MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			} else if (!PRIVATE_DATA->tracking && !MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (MOUNT_PARK_PARKED_ITEM->sw.value && PRIVATE_DATA->parked) {
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value && !PRIVATE_DATA->parked) {
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_PARK_PARKED_ITEM->sw.value && !PRIVATE_DATA->parking) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value && !PRIVATE_DATA->unparking) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_PARK_PROPERTY->state == INDIGO_OK_STATE) {
			if (MOUNT_PARK_PARKED_ITEM->sw.value && !PRIVATE_DATA->parked) {
				if (PRIVATE_DATA->unparking) {
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				}
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value && (PRIVATE_DATA->parked || PRIVATE_DATA->parking)) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_PARK_PROPERTY->state == INDIGO_ALERT_STATE) {
			if (PRIVATE_DATA->parked || PRIVATE_DATA->parking) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
			} else if (!PRIVATE_DATA->parked || PRIVATE_DATA->unparking) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			}
		}
		if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (MOUNT_HOME_ITEM->sw.value && PRIVATE_DATA->homed) {
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_HOME_PROPERTY->count == 2 && MOUNT_AWAY_ITEM->sw.value && !PRIVATE_DATA->homed) {
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			} else if (MOUNT_HOME_ITEM->sw.value && !PRIVATE_DATA->homing) {
				if (MOUNT_HOME_PROPERTY->count == 2) {
					indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
				} else {
					MOUNT_HOME_ITEM->sw.value = false;
				}
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_HOME_PROPERTY->state == INDIGO_OK_STATE) {
			if (MOUNT_HOME_ITEM->sw.value && !PRIVATE_DATA->homed) {
				if (!PRIVATE_DATA->homing) {
					if (MOUNT_HOME_PROPERTY->count == 2) {
						indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
					} else {
						MOUNT_HOME_ITEM->sw.value = false;
					}
				}
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (!MOUNT_HOME_ITEM->sw.value && (PRIVATE_DATA->homed || PRIVATE_DATA->homing)) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
				MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_HOME_PROPERTY->state == INDIGO_ALERT_STATE) {
			if (PRIVATE_DATA->homed || PRIVATE_DATA->homing) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
			} else {
				if (MOUNT_HOME_PROPERTY->count == 2) {
					indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
				} else {
					MOUNT_HOME_ITEM->sw.value = false;
				}
			}
		}
		if (MOUNT_PARK_PROPERTY->do_update) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
		if (MOUNT_HOME_PROPERTY->do_update) {
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_execute_handler_in(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, position_timer_callback);
	}
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
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
			indigo_define_property(device, MOUNT_MODE_PROPERTY, NULL);
			indigo_define_property(device, ZWO_BUZZER_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_AP_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_CL_PROPERTY, NULL);
			indigo_define_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
			indigo_define_property(device, NYX_LEVELER_PROPERTY, NULL);
			MOUNT_TYPE_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_execute_handler(device, position_timer_callback);
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		meade_stop(device);
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, MOUNT_MODE_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_CL_PROPERTY, NULL);
		indigo_delete_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		indigo_delete_property(device, NYX_LEVELER_PROPERTY, NULL);
		MOUNT_TYPE_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (--PRIVATE_DATA->device_count <= 0) {
			meade_close(device);
		}
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_callback(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (MOUNT_PARK_PROPERTY->count == 1) {
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		}
		if (meade_park(device)) {
			if (!(MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_GEMINI_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value))
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking");
	} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value) {
		if (meade_unpark(device)) {
			if (!(MOUNT_TYPE_10MICRONS_ITEM->sw.value || MOUNT_TYPE_STARGO_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value))
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
	if (meade_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value)) {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_eq_coords_callback(indigo_device *device) {
	char message[50] = "";

	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);

	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (meade_set_tracking_rate(device) && meade_slew(device, ra, dec)) {
			indigo_usleep(500000); // wait for the mount to start slewing to get correct state in the position timer
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
			// properties will be handled in position timer when the mount stops
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Failed to abort");
		}
	}
}

static void mount_motion_dec_callback(indigo_device *device) {
	if (meade_set_slew_rate(device) && meade_motion_dec(device)) {
		if (PRIVATE_DATA->lastMotionNS) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_callback(indigo_device *device) {
	if (meade_set_slew_rate(device) && meade_motion_ra(device)) {
		if (PRIVATE_DATA->lastMotionWE) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_set_host_time_callback(indigo_device *device) {
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		time_t secs = time(NULL);
		if (meade_set_utc(device, secs, indigo_get_utc_offset())) {
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
	} else {
		if (meade_set_utc(device, secs, offset)) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
}

static void mount_tracking_callback(indigo_device *device) {
	if (meade_set_tracking(device, MOUNT_TRACKING_ON_ITEM->sw.value)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
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

static void mount_pec_callback(indigo_device *device) {
	if (meade_pec(device, MOUNT_PEC_ENABLED_ITEM->sw.value)) {
		MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
}

static void mount_guide_rate_callback(indigo_device *device) {
	if (MOUNT_TYPE_ZWO_ITEM->sw.value) {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value =
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target =
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target;
	}
	if (meade_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target)) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void zwo_buzzer_callback(indigo_device *device) {
	if (ZWO_BUZZER_OFF_ITEM->sw.value) {
		meade_no_reply_command(device, ":SBu0#");
	} else if (ZWO_BUZZER_LOW_ITEM->sw.value) {
		meade_no_reply_command(device, ":SBu1#");
	} else if (ZWO_BUZZER_HIGH_ITEM->sw.value) {
		meade_no_reply_command(device, ":SBu2#");
	}
	ZWO_BUZZER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
}

static void nyx_ap_callback(indigo_device *device) {
	NYX_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	if (meade_simple_reply_command(device, ":WA%s#", NYX_WIFI_AP_SSID_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
		if (meade_simple_reply_command(device, ":WB%s#", NYX_WIFI_AP_PASSWORD_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
			if (meade_simple_reply_command(device, ":WLC#") && *PRIVATE_DATA->response == '1') {
				indigo_send_message(device, "Created access point with SSID %s", NYX_WIFI_AP_SSID_ITEM->text.value);
				NYX_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	indigo_update_property(device, NYX_WIFI_AP_PROPERTY, NULL);
}

static void nyx_cl_callback(indigo_device *device) {
	char ssid[128] = "";
	char password[128] = "";
	bool encode = false;
	if (compare_versions(MOUNT_INFO_FIRMWARE_ITEM->text.value, NYX_BASE64_THRESHOLD_VERSION) >= 0) {
		base64_encode((unsigned char *)ssid, (unsigned char *)NYX_WIFI_CL_SSID_ITEM->text.value, (long)strlen(NYX_WIFI_CL_SSID_ITEM->text.value));
		base64_encode((unsigned char *)password, (unsigned char*)NYX_WIFI_CL_PASSWORD_ITEM->text.value, (long)strlen(NYX_WIFI_CL_PASSWORD_ITEM->text.value));
		encode = true;
	}
	if (meade_simple_reply_command(device, ":WS%s#", encode ? ssid : NYX_WIFI_CL_SSID_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
		if (meade_simple_reply_command(device, ":WP%s#", encode ? password : NYX_WIFI_CL_PASSWORD_ITEM->text.value) && *PRIVATE_DATA->response == '1') {
			if (meade_no_reply_command(device, ":WLC#")) {
				indigo_send_message(device, "WiFi reset!");
				NYX_WIFI_CL_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
				if (PRIVATE_DATA->handle && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
					indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
				}
				return;
			}
		}
	}
	NYX_WIFI_CL_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
}

static void nyx_reset_callback(indigo_device *device) {
	if (meade_no_reply_command(device, ":WLZ#")) {
		indigo_send_message(device, "WiFi reset!");
		NYX_WIFI_RESET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		if (PRIVATE_DATA->handle && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE) {
			indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
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
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		MOUNT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Mount mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		MOUNT_MODE_PROPERTY->hidden = true;
		indigo_init_switch_item(EQUATORIAL_ITEM, EQUATORIAL_ITEM_NAME, "Equatorial mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 14);
		if (MOUNT_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_TYPE_DETECT_ITEM, MOUNT_TYPE_DETECT_ITEM_NAME, "Autodetect", true);
		indigo_init_switch_item(MOUNT_TYPE_MEADE_ITEM, MOUNT_TYPE_MEADE_ITEM_NAME, "Meade", false);
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
		if (ZWO_BUZZER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		ZWO_BUZZER_PROPERTY->hidden = true;
		indigo_init_switch_item(ZWO_BUZZER_OFF_ITEM, ZWO_BUZZER_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(ZWO_BUZZER_LOW_ITEM, ZWO_BUZZER_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(ZWO_BUZZER_HIGH_ITEM, ZWO_BUZZER_HIGH_ITEM_NAME, "High", false);
		// ---------------------------------------------------------------------------- NYX_WIFI_AP
		NYX_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_AP_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_AP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		NYX_WIFI_AP_PROPERTY->hidden = true;
		indigo_init_text_item(NYX_WIFI_AP_SSID_ITEM, NYX_WIFI_AP_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_AP_PASSWORD_ITEM, NYX_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		// ---------------------------------------------------------------------------- NYX_WIFI_CL
		NYX_WIFI_CL_PROPERTY = indigo_init_text_property(NULL, device->name, NYX_WIFI_CL_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Client WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (NYX_WIFI_CL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		NYX_WIFI_CL_PROPERTY->hidden = true;
		indigo_init_text_item(NYX_WIFI_CL_SSID_ITEM, NYX_WIFI_CL_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(NYX_WIFI_CL_PASSWORD_ITEM, NYX_WIFI_CL_PASSWORD_ITEM_NAME, "Password", "");
		// ---------------------------------------------------------------------------- NYX_WIFI_RESET
		NYX_WIFI_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, NYX_WIFI_RESET_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Reset WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (NYX_WIFI_RESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		NYX_WIFI_RESET_PROPERTY->hidden = true;
		indigo_init_switch_item(NYX_WIFI_RESET_ITEM, NYX_WIFI_RESET_ITEM_NAME, "Reset", false);
		// ---------------------------------------------------------------------------- NYX_LEVELER
		NYX_LEVELER_PROPERTY = indigo_init_number_property(NULL, device->name, NYX_LEVELER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Leveler", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (NYX_LEVELER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		NYX_LEVELER_PROPERTY->hidden = true;
		indigo_init_number_item(NYX_LEVELER_PICH_ITEM, NYX_LEVELER_PICH_ITEM_NAME, "Pitch [°]", 0, 360, 0, 0);
		indigo_init_number_item(NYX_LEVELER_ROLL_ITEM, NYX_LEVELER_ROLL_ITEM_NAME, "Roll [°]", 0, 360, 0, 0);
		indigo_init_number_item(NYX_LEVELER_COMPASS_ITEM, NYX_LEVELER_COMPASS_ITEM_NAME, "Compas [°]", 0, 360, 0, 0);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_TYPE_PROPERTY);
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZWO_BUZZER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_AP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_CL_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_RESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(NYX_WIFI_RESET_PROPERTY);
	}
	return indigo_mount_enumerate_properties(device, client, property);
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
		indigo_execute_handler(device, mount_connect_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if ((!PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->unparking && !PRIVATE_DATA->homing && MOUNT_PARK_PARKED_ITEM->sw.value) || (PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->unparking && !PRIVATE_DATA->homing && MOUNT_PARK_UNPARKED_ITEM->sw.value)) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_execute_handler(device, mount_park_callback);
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		indigo_property_copy_values(MOUNT_PARK_SET_PROPERTY, property, false);
		MOUNT_PARK_SET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
		indigo_execute_handler(device, mount_park_set_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (!PRIVATE_DATA->parked && !PRIVATE_DATA->parking && !PRIVATE_DATA->unparking && !PRIVATE_DATA->homing && !PRIVATE_DATA->homed && MOUNT_HOME_ITEM->sw.value) {
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
			indigo_execute_handler(device, mount_home_callback);
		} else {
			MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME_SET
		indigo_property_copy_values(MOUNT_HOME_SET_PROPERTY, property, false);
		MOUNT_HOME_SET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
		indigo_execute_handler(device, mount_home_set_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_execute_handler(device, mount_geo_coords_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (IS_PARKED) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		} else {
			PRIVATE_DATA->motioned = false; // WTF?
			indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			// Update to busy moved to callback to avoid race condition with position timer
			indigo_execute_handler(device, mount_eq_coords_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		PRIVATE_DATA->motioned = true; // WTF?
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_URGENT, mount_abort_callback);
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
			indigo_execute_handler(device, mount_motion_dec_callback);
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
			indigo_execute_handler(device, mount_motion_ra_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		indigo_execute_handler(device, mount_set_host_time_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_execute_handler(device, mount_set_utc_time_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (IS_PARKED) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_execute_handler(device, mount_tracking_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		indigo_execute_handler(device, mount_track_rate_callback);
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
			indigo_execute_handler(device, mount_pec_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		indigo_execute_handler(device, mount_guide_rate_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		indigo_property_copy_values(MOUNT_TYPE_PROPERTY, property, false);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (MOUNT_TYPE_STARGO2_ITEM->sw.value) {
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
		indigo_execute_handler(device, zwo_buzzer_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_AP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_AP
		indigo_property_copy_values(NYX_WIFI_AP_PROPERTY, property, false);
		NYX_WIFI_AP_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_AP_PROPERTY, NULL);
		indigo_execute_handler(device, nyx_ap_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_CL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_CL
		indigo_property_copy_values(NYX_WIFI_CL_PROPERTY, property, false);
		NYX_WIFI_CL_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_CL_PROPERTY, NULL);
		indigo_execute_handler(device, nyx_cl_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(NYX_WIFI_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- NYX_WIFI_RESET
		indigo_property_copy_values(NYX_WIFI_RESET_PROPERTY, property, false);
		NYX_WIFI_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, NYX_WIFI_RESET_PROPERTY, NULL);
		indigo_execute_handler(device, nyx_reset_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
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

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_NORTH_ITEM->number.max = GUIDER_GUIDE_SOUTH_ITEM->number.max = GUIDER_GUIDE_EAST_ITEM->number.max = GUIDER_GUIDE_WEST_ITEM->number.max = 3000;
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
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				if (!meade_detect_mount(device->master_device)) {
					result = false;
					indigo_send_message(device, "Autodetection failed!");
					meade_close(device);
				}
			}
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->device_count == 0) {
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_finish_callback(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_dec_callback(indigo_device *device) {
	int north = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	int south = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
	meade_guide_dec(device, north, south);
	if (north > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)north) / 1000.0, guider_guide_dec_finish_callback);
	} else if (south > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)south) / 1000.0, guider_guide_dec_finish_callback);
	} else {
		guider_guide_dec_finish_callback(device);
	}
}

static void guider_guide_ra_finish_callback(indigo_device *device) {
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_ra_callback(indigo_device *device) {
	int west = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
	int east = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	meade_guide_ra(device, west, east);
	if (west > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)west) / 1000.0, guider_guide_ra_finish_callback);
	} else if (east > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)east) / 1000.0, guider_guide_ra_finish_callback);
	} else {
		guider_guide_ra_finish_callback(device);
	}
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
		indigo_execute_handler(device, guider_connect_callback);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		if (GUIDER_GUIDE_DEC_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_execute_handler(device, guider_guide_dec_callback);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		if (GUIDER_GUIDE_RA_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_execute_handler(device, guider_guide_ra_callback);
		}
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
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = meade_open(device->master_device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				if (!meade_detect_mount(device->master_device)) {
					result = false;
					indigo_send_message(device, "Autodetection failed!");
					meade_close(device);
				}
			}
		}
		if (result) {
			if (MOUNT_TYPE_MEADE_ITEM->sw.value || MOUNT_TYPE_AP_ITEM->sw.value || MOUNT_TYPE_ON_STEP_ITEM->sw.value || MOUNT_TYPE_OAT_ITEM->sw.value) {
				FOCUSER_SPEED_ITEM->number.min = FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
				FOCUSER_SPEED_ITEM->number.max = 2;
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
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
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->device_count == 0) {
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_steps_callback(indigo_device *device) {
	int steps = (int)(FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? -FOCUSER_STEPS_ITEM->number.value : FOCUSER_STEPS_ITEM->number.value);
	if (meade_focus_rel(device, FOCUSER_SPEED_ITEM->number.value == FOCUSER_SPEED_ITEM->number.min, steps)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_callback(indigo_device *device) {
	if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		PRIVATE_DATA->focus_aborted = true;
	}
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
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
		indigo_execute_handler(device, focuser_connect_callback);
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
			indigo_execute_handler(device, focuser_steps_callback);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		focuser_abort_callback(device);
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
		// -------------------------------------------------------------------------------- AUX_WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, "Info", "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_WEATHER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		AUX_WEATHER_PROPERTY->hidden = true;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Pressure [mb]", 0, 2000, 0, 0);
		// -------------------------------------------------------------------------------- AUX_INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, "Info", "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (AUX_INFO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		AUX_INFO_PROPERTY->hidden = true;
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 15, 0, 0);
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		char name[32];
		char label[32];
		for (int i = 0; i < 8; i++) {
			snprintf(name, sizeof(name), AUX_HEATER_OUTLET_ITEM_NAME, i + 1);
			snprintf(label, sizeof(label), "Heater #%d [%%]", i + 1);
			indigo_init_number_item(AUX_HEATER_OUTLET_PROPERTY->items, name, label, 0, 100, 1, 0);
		}
		AUX_HEATER_OUTLET_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		AUX_POWER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_POWER_OUTLET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		for (int i = 0; i < 8; i++) {
			snprintf(name, sizeof(name), AUX_POWER_OUTLET_ITEM_NAME, i + 1);
			snprintf(label, sizeof(label), "Outlet #%d", i + 1);
			indigo_init_switch_item(AUX_POWER_OUTLET_PROPERTY->items, name, label, true);
		}
		AUX_POWER_OUTLET_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_WEATHER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_INFO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_HEATER_OUTLET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static void nyx_aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	bool updateWeather = false;
	bool updateInfo = false;
	if (meade_command(device, ":GX9A#")) {
		double temperature = atof(PRIVATE_DATA->response);
		if (AUX_WEATHER_TEMPERATURE_ITEM->number.value != temperature) {
			AUX_WEATHER_TEMPERATURE_ITEM->number.value = temperature;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9B#")) {
		double pressure = atof(PRIVATE_DATA->response);
		if (AUX_WEATHER_PRESSURE_ITEM->number.value != pressure) {
			AUX_WEATHER_PRESSURE_ITEM->number.value = pressure;
			updateWeather = true;
		}
	}
	if (meade_command(device, ":GX9V#")) {
		double voltage = atof(PRIVATE_DATA->response);
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
	indigo_execute_handler_in(device, 10, nyx_aux_timer_callback);
}

static void onstep_aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
		bool do_update = false;
		for (int i = 0; i < AUX_HEATER_OUTLET_PROPERTY->count; i++) {
			int onstep_slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
			// responds with a number between 0 for fully off and 255 for fully on
			meade_command(device, ":GXX%d#", onstep_slot);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received PRIVATE_DATA->response %s for slot %d", PRIVATE_DATA->response, onstep_slot);
			indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
			// convert to percent
			int new_value = (int)(atoi(PRIVATE_DATA->response) / 2.56 + 0.5);
			if (new_value != (int) item->number.value) {
				item->number.value = new_value;
				do_update = true;
			}
		}
		if (do_update) {
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}
	}
	if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
		bool do_update = false;
		for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
			int onstep_slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
			// the PRIVATE_DATA->response is 0 when disabled and 1 when the switch is enabled
			meade_command(device, ":GXX%d#", onstep_slot);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "received PRIVATE_DATA->response %s for slot %d", PRIVATE_DATA->response, onstep_slot);
			indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
			bool active = PRIVATE_DATA->response[0] - '0';
			if (active != item->sw.value) {
				item->sw.value = active;
				AUX_POWER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
				do_update = true;
			}
		}
		if (do_update) {
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, 2, onstep_aux_timer_callback);
}

static void aux_connect_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = meade_open(device->master_device);
		}
		if (result) {
			if (MOUNT_TYPE_DETECT_ITEM->sw.value) {
				if (!meade_detect_mount(device->master_device)) {
					result = false;
					indigo_send_message(device, "Autodetection failed!");
					meade_close(device);
				}
			}
		}
		if (result) {
			if (MOUNT_TYPE_NYX_ITEM->sw.value) {
				AUX_WEATHER_PROPERTY->hidden = false;
				AUX_INFO_PROPERTY->hidden = false;
				indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
				indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				indigo_execute_handler(device, nyx_aux_timer_callback);
			} else if (MOUNT_TYPE_ON_STEP_ITEM->sw.value) {
				// first we request Onstep to list active aux slots
				meade_command(device, ":GXY0#");
				// Onstep responds with a string like "11000000" to indicate that the first and second aux device is enabled
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep active device string: %s", PRIVATE_DATA->response);
				// in the first pass over the active devices we count how many auxiliary devices of each purpose we have
				AUX_HEATER_OUTLET_PROPERTY->count = 0;
				AUX_POWER_OUTLET_PROPERTY->count = 0;
				for (int i = 0; i < ONSTEP_AUX_DEVICE_COUNT; i++) {
					bool active = PRIVATE_DATA->response[i] == '1';
					if (active) {
						// Now we get the name and purpose of each active aux device, it is one-indexed
						meade_command(device, ":GXY%d#", i + 1);
						// Onstep responds with a string like "my switch,2" where the part before "," is the name and after the purpose as int
						char *comma = strchr(PRIVATE_DATA->response, ',');
						if (comma == NULL) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "Onstep AUX Device at slot %d invalid PRIVATE_DATA->response", i + 1);
							continue;
						}
						*comma++ = '\0';
						char *name = PRIVATE_DATA->response;
						onstep_aux_device_purpose purpose = *comma - '0';
						if (purpose == ONSTEP_AUX_ANALOG) {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Heater Outlet at slot %d with name %s", i + 1, name);
							strcpy(AUX_HEATER_OUTLET_PROPERTY->items[AUX_HEATER_OUTLET_PROPERTY->count].label, name);
							ONSTEP_AUX_HEATER_OUTLET_MAPPING[AUX_HEATER_OUTLET_PROPERTY->count++] = i + 1;
						} else if (purpose == ONSTEP_AUX_SWITCH) {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Power Outlet Device at slot %d with name %s and purpose switch", i + 1, name);
							strcpy(AUX_POWER_OUTLET_PROPERTY->items[AUX_POWER_OUTLET_PROPERTY->count].label, name);
							ONSTEP_AUX_POWER_OUTLET_MAPPING[AUX_POWER_OUTLET_PROPERTY->count++] = i + 1;
						} else {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Onstep AUX Device at index %d not recognized", i + 1, name);
						}
					}
				}
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				AUX_HEATER_OUTLET_PROPERTY->hidden = false;
				AUX_POWER_OUTLET_PROPERTY->hidden = false;
				indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
				indigo_execute_handler(device, onstep_aux_timer_callback);
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
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		if (--PRIVATE_DATA->device_count == 0) {
			meade_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	for (int i = 0; i < AUX_HEATER_OUTLET_PROPERTY->count; i++) {
		indigo_item *item = AUX_HEATER_OUTLET_PROPERTY->items + i;
		int val = MIN((int) round(item->number.target * 2.56), 255);
		int slot = ONSTEP_AUX_HEATER_OUTLET_MAPPING[i];
		meade_simple_reply_command(device, ":SXX%d,V%d#", slot, val);
		if (PRIVATE_DATA->response[0] == '1') {
			item->number.value = item->number.target;
		} else {
			AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
}

static void aux_power_outlet_handler(indigo_device *device) {
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	for (int i = 0; i < AUX_POWER_OUTLET_PROPERTY->count; i++) {
		indigo_item *item = AUX_POWER_OUTLET_PROPERTY->items + i;
		bool val = item->sw.value;
		int slot = ONSTEP_AUX_POWER_OUTLET_MAPPING[i];
		meade_simple_reply_command(device, ":SXX%d,V%d#", slot, val);
		if (PRIVATE_DATA->response[0] != '1') {
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
		indigo_execute_handler(device, aux_connect_handler);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_targets(AUX_HEATER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_execute_handler(device, aux_heater_outlet_handler);
		return INDIGO_OK;
	}
	if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_execute_handler(device, aux_power_outlet_handler);
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connect_handler(device);
	}
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// --------------------------------------------------------------------------------

static lx200_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;
static indigo_device *mount_focuser = NULL;
static indigo_device *mount_aux = NULL;

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

	if (action == last_action) {
		return INDIGO_OK;
	}

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
