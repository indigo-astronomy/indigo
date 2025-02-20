// Copyright (c) 2022 Rumen G.Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO ZWO AM driver
 \file indigo_mount_asi.c
 */

#define DRIVER_VERSION 0x001A
#define DRIVER_NAME	"indigo_mount_asi"

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
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>

#include "indigo_mount_asi.h"

#define PRIVATE_DATA                    ((asi_private_data *)device->private_data)

#define MOUNT_MODE_PROPERTY             (PRIVATE_DATA->x_mode_property)
#define EQUATORIAL_ITEM                 (MOUNT_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM                 (MOUNT_MODE_PROPERTY->items+1)

#define MOUNT_MODE_PROPERTY_NAME        "X_MOUNT_MODE"
#define EQUATORIAL_ITEM_NAME            "EQUATORIAL"
#define ALTAZ_MODE_ITEM_NAME            "ALTAZ"

#define ZWO_BUZZER_PROPERTY             (PRIVATE_DATA->x_buzzer_property)
#define ZWO_BUZZER_OFF_ITEM             (ZWO_BUZZER_PROPERTY->items+0)
#define ZWO_BUZZER_LOW_ITEM             (ZWO_BUZZER_PROPERTY->items+1)
#define ZWO_BUZZER_HIGH_ITEM            (ZWO_BUZZER_PROPERTY->items+2)

#define ZWO_BUZZER_PROPERTY_NAME        "X_BUZZER"
#define ZWO_BUZZER_OFF_ITEM_NAME        "OFF"
#define ZWO_BUZZER_LOW_ITEM_NAME        "LOW"
#define ZWO_BUZZER_HIGH_ITEM_NAME       "HIGH"

#define ZWO_MERIDIAN_PROPERTY           (PRIVATE_DATA->x_meridian_property)
#define ZWO_MERIDIAN_AUTO_FLIP_ITEM     (ZWO_MERIDIAN_PROPERTY->items+0)
#define ZWO_MERIDIAN_TRACK_PASSED_ITEM  (ZWO_MERIDIAN_PROPERTY->items+1)

#define ZWO_MERIDIAN_PROPERTY_NAME           "X_MERIDIAN"
#define ZWO_MERIDIAN_AUTO_FLIP_ITEM_NAME     "AUTO_FLIP_AT_LIMIT"
#define ZWO_MERIDIAN_TRACK_PASSED_ITEM_NAME  "TRACK_PASSED_MERIDIAN"

#define ZWO_MERIDIAN_LIMIT_PROPERTY     (PRIVATE_DATA->x_meridian_limit_property)
#define ZWO_MERIDIAN_LIMIT_ITEM         (ZWO_MERIDIAN_LIMIT_PROPERTY->items+0)

#define ZWO_MERIDIAN_LIMIT_PROPERTY_NAME   "X_MERIDIAN_LIMIT"
#define ZWO_MERIDIAN_LIMIT_ITEM_NAME       "LIMIT"

typedef struct {
	int handle;
	int device_count;
	bool is_network;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	bool motioned;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	uint32_t firmware;
	indigo_property *x_mode_property;
	indigo_property *x_buzzer_property;
	indigo_property *x_meridian_property;
	indigo_property *x_meridian_limit_property;
	indigo_timer *focuser_timer;
	bool home_changed;
	bool tracking_changed;
	bool tracking_rate_changed;
	bool focus_aborted;
	int prev_tracking_rate;
	bool prev_home_state;
	int prev_tracking_error;
	bool park_requested;
} asi_private_data;

static char *asi_error_string(unsigned int code) {
	const char *error_string[] = {
		"",
		"Prameters out of range",
		"Format error",
		"Mount not initialized",
		"Mount is moving",
		"Target is below horizon",
		"Target is beow the altitude limit",
		"Time and location are not set",
		"Warning: Meridian reached, tracking stopeed",
		"Target is on the other side of the meridian"
	};
	if (code > 9) code = 0;
	return (char *)error_string[code];
}

static int asi_error_code(char *response) {
	int error_code = 0;
	int parsed = sscanf(response, "e%d", &error_code);
	if (parsed == 1) return error_code;
	return 0;
}

static bool asi_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool asi_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "asi")) {
		PRIVATE_DATA->is_network = false;
		PRIVATE_DATA->handle = indigo_open_serial(name);
	} else {
		PRIVATE_DATA->is_network = true;
		indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
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
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static void network_disconnection(__attribute__((unused)) indigo_device *device);

static bool asi_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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
			if (PRIVATE_DATA->is_network) {
				// This is a disconnection
				indigo_set_timer(device, 0, network_disconnection, NULL);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected disconnection from %s", DEVICE_PORT_ITEM->text.value);
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
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static void asi_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// ---------------------------------------------------------------------  mount commands

static bool asi_set_utc(indigo_device *device, time_t *secs, int utc_offset) {
	char command[128], response[128];
	time_t seconds = *secs + utc_offset * 3600;
	struct tm tm;
	gmtime_r(&seconds, &tm);
	sprintf(command, ":SC%02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
	bool result = asi_command(device, command, response, 1, 0);
	if (!result || *response != '1') {
		return false;
	} else {
		sprintf(command, ":SG%+03d#", -utc_offset);
		if (!asi_command(device, command, response, 1, 0) || *response != '1') {
			return false;
		} else {
			sprintf(command, ":SL%02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (!asi_command(device, command, response, 1, 0) || *response != '1') {
				return false;
			} else {
				return true;
			}
		}
	}
}

static bool asi_get_utc(indigo_device *device, time_t *secs, int *utc_offset) {
	struct tm tm;
	char response[128];
	memset(&tm, 0, sizeof(tm));
	char separator[2];
	if (asi_command(device, ":GC#", response, sizeof(response), 0) && sscanf(response, "%d%c%d%c%d", &tm.tm_mon, separator, &tm.tm_mday, separator, &tm.tm_year) == 5) {
		if (asi_command(device, ":GL#", response, sizeof(response), 0) && sscanf(response, "%d%c%d%c%d", &tm.tm_hour, separator, &tm.tm_min, separator, &tm.tm_sec) == 5) {
			tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
			tm.tm_mon -= 1;
			if (asi_command(device, ":GG#", response, sizeof(response), 0)) {
				*utc_offset = -atoi(response);
				*secs = timegm(&tm) - *utc_offset * 3600;
				return true;
			}
		}
	}
	return false;
}

static bool asi_get_sidereal_time(indigo_device *device, double *siderial_time) {
	int h, m, s;
	char response[128];

	if (asi_command(device, ":GS#", response, sizeof(response), 0) && (sscanf(response, "%d:%d:%d", &h, &m, &s) == 3)) {
		*siderial_time = (double)h + m/60.0 + s/3600.0;
		return true;
	} else {
		return false;
	}
}

static void asi_get_site(indigo_device *device, double *latitude, double *longitude) {
	char response[128];
	if (asi_command(device, ":Gt#", response, sizeof(response), 0)) {
		*latitude = indigo_stod(response);
	}
	// LX200 protocol returns negative longitude for the east
	if (asi_command(device, ":Gg#", response, sizeof(response), 0)) {
		*longitude = indigo_stod(response);
		if (*longitude < 0)
			*longitude += 360;
		*longitude = 360 - *longitude;
	}
}

static bool asi_set_site(indigo_device *device, double latitude, double longitude) {
	char command[128], response[128];
	sprintf(command, ":St%s#", indigo_dtos(latitude, "%+03d*%02d"));
	if (!asi_command(device, command, response, 1, 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
		return false;
	} else {
		// LX200 protocol expects negative longitude for the east
		longitude = fmod((360 - longitude), 360);
		sprintf(command, ":Sg%s#", indigo_dtos(longitude, "%03d*%02d"));
		if (!asi_command(device, command, response, 1, 0) || *response != '1') {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
			return false;
		}
	}
	return true;
}

static bool asi_get_meridian_settings(indigo_device *device, bool *flip_enabled, bool *track_passed, int *track_passed_limit) {
	char response[128];
	if (asi_command(device, ":GTa#", response, sizeof(response), 0)) {
		if (strlen(response) != 5) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,"unexpected response '%s'", response);
			return false;
		}
		if (flip_enabled != NULL) {
			if (response[0] == '0') {
				*flip_enabled = false;
			} else {
				*flip_enabled = true;
			}
		}
		if (track_passed != NULL) {
			if (response[1] == '0') {
				*track_passed = false;
			} else {
				*track_passed = true;
			}
		}
		if (track_passed_limit != NULL) {
			char *limit_str = &response[2];
			*track_passed_limit = atoi(limit_str);
		}
		return true;
	}
	return false;
}

static bool asi_set_meridian_action(indigo_device *device, bool flip, bool track) {
	char current[128];
	char request[128];
	if (asi_command(device, ":GTa#", current, sizeof(current), 0)) {
		if (flip) {
			current[0] = '1';
		} else {
			current[0] = '0';
		}
		if (track) {
			current[1] = '1';
		} else {
			current[1] = '0';
		}
		sprintf(request, ":STa%s#", current);
		return asi_command(device, request, current, sizeof(current), 0);
	}
	return false;
}

static bool asi_set_meridian_limit(indigo_device *device, int16_t limit) {
	char current[128];
	char request[128];
	if (limit < -15 || limit > 15) {
		return false;
	}
	if (asi_command(device, ":GTa#", current, sizeof(current), 0)) {
		sprintf(current+2, "%+03d", limit);
		sprintf(request, ":STa%s#", current);
		return asi_command(device, request, current, sizeof(current), 0);
	}
	return false;
}

static bool asi_get_coordinates(indigo_device *device, double *ra, double *dec) {
	char response[128];
	if (asi_command(device, ":GR#", response, sizeof(response), 0)) {
		*ra = indigo_stod(response);
		if (asi_command(device, ":GD#", response, sizeof(response), 0)) {
			*dec = indigo_stod(response);
			return true;
		}
	}
	return false;
}

static bool asi_slew(indigo_device *device, double ra, double dec, int *error_code) {
	char command[128], response[128];
	sprintf(command, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f"));
	if (!asi_command(device, command, response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		*error_code = asi_error_code(response);
		return false;
	}
	sprintf(command, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f"));
	if (!asi_command(device, command, response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		*error_code = asi_error_code(response);
		return false;
	}
	if (!asi_command(device, ":MS#", response, sizeof(response), 100000) || *response != '0') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":MS# failed with response: %s", response);
		*error_code = asi_error_code(response);
		return false;
	}
	*error_code = 0;
	return true;
}

static bool asi_sync(indigo_device *device, double ra, double dec, int *error_code) {
	char command[128], response[128];
	sprintf(command, ":Sr%s#", indigo_dtos(ra, "%02d:%02d:%02.0f"));
	if (!asi_command(device, command, response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		*error_code = asi_error_code(response);
		return false;
	}
	sprintf(command, ":Sd%s#", indigo_dtos(dec, "%+03d*%02d:%02.0f"));
	if (!asi_command(device, command, response, sizeof(response), 0) || *response != '1') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed with response: %s", command, response);
		*error_code = asi_error_code(response);
		return false;
	}
	if (!asi_command(device, ":CM#", response, sizeof(response), 100000) || *response == 'e') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, ":CM# failed with response: %s", response);
		*error_code = asi_error_code(response);
		return false;
	}
	*error_code = 0;
	return true;
}

static bool asi_set_guide_rate(indigo_device *device, int ra, int dec) {
	char command[128];
	// asi miunt has one guide rate for ra and dec
	if (ra < 10) ra = 10;
	if (ra > 90) ra = 90;
	float rate = ra / 100.0;
	sprintf(command, ":Rg%.1f#", rate);
	return (asi_command(device, command, NULL, 0, 0));
}

static bool asi_get_guide_rate(indigo_device *device, int *ra, int *dec) {
	char response[128] = {0};
	bool res = asi_command(device, ":Ggr#", response, sizeof(response), 0);
	if (!res) return false;
	float rate = 0;
	int parsed = sscanf(response, "%f", &rate);
	if (parsed != 1) return false;
	*ra = *dec = rate * 100;
	return true;
}

// NOTE! requires firmware 1.1.1
static bool asi_get_tracking_status(indigo_device *device, bool *is_tracking, int *error_code) {
	char response[128] = {0};
	bool res = asi_command(device, ":GAT#", response, sizeof(response), 0);
	if (!res) return false;
	if (response[0] == '0' && response[1] == '\0') {
		*is_tracking = 0;
		*error_code = 0;
	} else if (response[0] == '1' && response[1] == '\0') {
		*is_tracking = 1;
		*error_code = 0;
	} else if (response[0] == 'e') {
		*is_tracking = 0;
		*error_code = atoi(response+1);
	} else {
		return false;
	}
	return true;
}

static bool asi_set_tracking(indigo_device *device, bool on) {
	char response[64] = {0};
	if (on) {
		return asi_command(device, ":Te#", response, sizeof(response), 0) && *response == '1';
	} else {
		return asi_command(device, ":Td#", response, sizeof(response), 0) && *response == '1';
	}
}

static bool asi_set_tracking_rate(indigo_device *device) {
	if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
		PRIVATE_DATA->lastTrackRate = 'q';
		return asi_command(device, ":TQ#", NULL, 0, 0);
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
		PRIVATE_DATA->lastTrackRate = 's';
		return asi_command(device, ":TS#", NULL, 0, 0);
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
		PRIVATE_DATA->lastTrackRate = 'l';
		return asi_command(device, ":TL#", NULL, 0, 0);
	}
	return true;
}

static bool asi_set_slew_rate(indigo_device *device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'g') {
		PRIVATE_DATA->lastSlewRate = 'g';
		return asi_command(device, ":R1#", NULL, 0, 0);
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'c') {
		PRIVATE_DATA->lastSlewRate = 'c';
		return asi_command(device, ":R4#", NULL, 0, 0);
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 'm') {
		PRIVATE_DATA->lastSlewRate = 'm';
		return asi_command(device, ":R7#", NULL, 0, 0);
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value && PRIVATE_DATA->lastSlewRate != 's') {
		PRIVATE_DATA->lastSlewRate = 's';
		return asi_command(device, ":R9#", NULL, 0, 0);
	}
	return true;
}

static bool asi_motion_dec(indigo_device *device) {
	bool stopped = true;
	if (PRIVATE_DATA->lastMotionNS == 'n') {
		stopped = asi_command(device, ":Qn#", NULL, 0, 0);
	} else if (PRIVATE_DATA->lastMotionNS == 's') {
		stopped = asi_command(device, ":Qs#", NULL, 0, 0);
	}
	if (stopped) {
		if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 'n';
			return asi_command(device, ":Mn#", NULL, 0, 0);
		} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionNS = 's';
			return asi_command(device, ":Ms#", NULL, 0, 0);
		} else {
			PRIVATE_DATA->lastMotionNS = 0;
		}
	}
	return stopped;
}

static bool asi_motion_ra(indigo_device *device) {
	bool stopped = true;
	if (PRIVATE_DATA->lastMotionWE == 'w') {
		stopped = asi_command(device, ":Qw#", NULL, 0, 0);
	} else if (PRIVATE_DATA->lastMotionWE == 'e') {
		stopped = asi_command(device, ":Qe#", NULL, 0, 0);
	}
	if (stopped) {
		if (MOUNT_MOTION_WEST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'w';
			return asi_command(device, ":Mw#", NULL, 0, 0);
		} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
			PRIVATE_DATA->lastMotionWE = 'e';
			return asi_command(device, ":Me#", NULL, 0, 0);
		} else {
			PRIVATE_DATA->lastMotionWE = 0;
		}
	}
	return stopped;
}

static bool asi_home(indigo_device *device) {
	return asi_command(device, ":hC#", NULL, 0, 0);
}

static bool asi_park(indigo_device *device) {
	return asi_command(device, ":hP#", NULL, 0, 0);
}

static bool asi_stop(indigo_device *device) {
	return asi_command(device, ":Q#", NULL, 0, 0);
}

static bool asi_clear_alignment_data(indigo_device *device) {
	char response[64] = {0};
	return asi_command(device, ":NSC#", response, sizeof(response), 0) && *response == '1';
}

static bool asi_guide_dec(indigo_device *device, int north, int south) {
	char command[128];
	if (north > 0) {
		sprintf(command, ":Mgn%04d#", north);
		return asi_command(device, command, NULL, 0, 0);
	} else if (south > 0) {
		sprintf(command, ":Mgs%04d#", south);
		return asi_command(device, command, NULL, 0, 0);
	}
	return false;
}

static bool asi_guide_ra(indigo_device *device, int west, int east) {
	char command[128];
	if (west > 0) {
		sprintf(command, ":Mgw%04d#", west);
		return asi_command(device, command, NULL, 0, 0);
	} else if (east > 0) {
		sprintf(command, ":Mge%04d#", east);
		return asi_command(device, command, NULL, 0, 0);
	}
	return false;
}

static void asi_update_site_items(indigo_device *device) {
	double latitude, longitude;
	asi_get_site(device, &latitude, &longitude);
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
}

static bool asi_detect_mount(indigo_device *device) {
	char response[128];
	bool result = true;
	if (asi_command(device, ":GVP#", response, sizeof(response), 0)) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: '%s'", response);
		strncpy(PRIVATE_DATA->product, response, 64);
		if (!strncmp(PRIVATE_DATA->product, "AM", 2) && isdigit(PRIVATE_DATA->product[2])) {
		} else {
			result = false;
		}
	} else {
		result = false;
	}
	return result;
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation
static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		char response[128];

		// read coordinates and moving state
		double ra = 0, dec = 0;
		bool success = false;
		if ((success = asi_get_coordinates(device, &ra, &dec))) {
			indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		}
		if (success && (success = asi_command(device, ":GU#", response, sizeof(response), 0))) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = strchr(response, 'N') ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
			if (strchr(response, 'n')) {
				if (MOUNT_TRACKING_ON_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
				}
			} else {
				if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
				}
			}
			if (strchr(response, 'H')) {
				if (PRIVATE_DATA->prev_home_state == false) {
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
					indigo_update_property(device, MOUNT_HOME_PROPERTY, "At home");
				}
				PRIVATE_DATA->prev_home_state = true;
			} else {
				if (PRIVATE_DATA->prev_home_state == true) {
					indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, false);
					indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
				}
				PRIVATE_DATA->prev_home_state = false;
			}
			if (strchr(response, 'N')) {
				if (PRIVATE_DATA->park_requested == true) {
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					/* We can not detect when the mount is parked or unparked as the parked
					   flag is never set (a firmware bug) this is why we set it to unparked
					   when the goto park is finished
					*/
					indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
					indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked (perked item not set due to a firmware bug)");
				}
				PRIVATE_DATA->park_requested = false;
			}
		}

		if (success && (success = asi_command(device, ":Gm#", response, sizeof(response), 0))) {
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
		if (!success) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		PRIVATE_DATA->lastRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		PRIVATE_DATA->lastDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_update_coordinates(device, NULL);

		// read time
		int utc_offset;
		time_t secs;
		if (asi_get_utc(device, &secs, &utc_offset)) {
			sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", utc_offset);
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);

		if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
			double st;
			asi_get_sidereal_time(device, &st);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount LST = %lf, Host LST = %lf, offset = %.2fs", st, MOUNT_LST_TIME_ITEM->number.value, fabs(st - MOUNT_LST_TIME_ITEM->number.value)*3600);
		}

		bool is_tracking;
		int error_code;
		if (asi_get_tracking_status(device, &is_tracking, &error_code)) {
			if (PRIVATE_DATA->prev_tracking_error != error_code && error_code > 0) {
				indigo_send_message(device, asi_error_string(error_code));
			} else if (PRIVATE_DATA->prev_tracking_error == 8 && error_code == 0) {
				indigo_send_message(device, "Tracking can be started");
			}
			PRIVATE_DATA->prev_tracking_error = error_code;
		}

		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

static void asi_init_mount(indigo_device *device) {
	char response[128];
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_TRACKING_PROPERTY->hidden = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
	MOUNT_HOME_PROPERTY->hidden = false;
	MOUNT_MOTION_RA_PROPERTY->hidden = false;
	MOUNT_MOTION_DEC_PROPERTY->hidden = false;
	MOUNT_SLEW_RATE_PROPERTY->hidden = false;
	MOUNT_TRACK_RATE_PROPERTY->hidden = false;
	MOUNT_MODE_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
	ZWO_BUZZER_PROPERTY->hidden = false;
	PRIVATE_DATA->firmware = 0;
	if (asi_command(device, ":GV#", response, sizeof(response), 0)) {
		char fv[3] = {0};
		if (3 == sscanf(response, "%hhd.%hhd.%hhd", &fv[0], &fv[1], &fv[2])) {
			PRIVATE_DATA->firmware = (fv[0] << 16) | (fv[1] << 8) | fv[2];
			INDIGO_DRIVER_DEBUG(DRIVER_NAME,"%s firmware version %s (0x%06X)", PRIVATE_DATA->product, response, PRIVATE_DATA->firmware);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,"unexpected firmware format '%s'", response);
		}
		MOUNT_INFO_PROPERTY->count = 3;
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "ZWO");
		strcpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product);
		strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response);
	}

	if (PRIVATE_DATA->firmware >= 0x010204) {
		ZWO_MERIDIAN_PROPERTY->hidden = false;
		ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = false;
		bool flip, track;
		int limit;
		if (asi_get_meridian_settings(device, &flip, &track, &limit)) {
			ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value = flip;
			ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value = track;
			ZWO_MERIDIAN_LIMIT_ITEM->number.value = ZWO_MERIDIAN_LIMIT_ITEM->number.target = limit;
		}
		MOUNT_PARK_PROPERTY->hidden = false;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = false;
	} else {
		ZWO_MERIDIAN_PROPERTY->hidden = true;
		ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = true;
		MOUNT_PARK_PROPERTY->hidden = true;
		MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = true;
	}
	indigo_define_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
	indigo_define_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);

	MOUNT_GUIDE_RATE_DEC_ITEM->number.min =
	MOUNT_GUIDE_RATE_RA_ITEM->number.min = 10;
	MOUNT_GUIDE_RATE_DEC_ITEM->number.max =
	MOUNT_GUIDE_RATE_RA_ITEM->number.max = 90;
	int ra_rate, dec_rate;
	if (asi_get_guide_rate(device, &ra_rate, &dec_rate)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide rate read");
		MOUNT_GUIDE_RATE_RA_ITEM->number.target = MOUNT_GUIDE_RATE_RA_ITEM->number.value = (double)ra_rate;
		MOUNT_GUIDE_RATE_DEC_ITEM->number.target = MOUNT_GUIDE_RATE_DEC_ITEM->number.value = (double)dec_rate;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guide rate can not be read read, seting");
		asi_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target);
	}

	if (asi_command(device, ":GU#", response, sizeof(response), 0)) {
		if (strchr(response, 'G'))
			indigo_set_switch(MOUNT_MODE_PROPERTY, EQUATORIAL_ITEM, true);
		if (strchr(response, 'Z'))
			indigo_set_switch(MOUNT_MODE_PROPERTY, ALTAZ_MODE_ITEM, true);
	}
	indigo_define_property(device, MOUNT_MODE_PROPERTY, NULL);
	asi_update_site_items(device);
	time_t secs;
	int utc_offset;
	asi_get_utc(device, &secs, &utc_offset);
	// if date is before January 1, 2001 1:00:00 AM we consifer mount not initialized
	if (secs < 978310800) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount is not initialized, initializing...");
		secs = time(NULL);
		utc_offset = indigo_get_utc_offset();
		asi_set_utc(device, &secs, utc_offset);
		asi_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	}
	/* Tracking rate */
	if (asi_command(device, ":GT#", response, sizeof(response), 0)) {
		if (strchr(response, '0')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
		} else if (strchr(response, '1')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
		} else if (strchr(response, '2')) {
			indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
		}
	}
	/* Buzzer volume */
	if (asi_command(device, ":GBu#", response, sizeof(response), 0)) {
		if (strchr(response, '0')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_OFF_ITEM, true);
		} else if (strchr(response, '1')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_LOW_ITEM, true);
		} else if (strchr(response, '2')) {
			indigo_set_switch(ZWO_BUZZER_PROPERTY, ZWO_BUZZER_HIGH_ITEM, true);
		}
	}
	indigo_define_property(device, ZWO_BUZZER_PROPERTY, NULL);
}

static void mount_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = asi_open(device);
		}
		if (result && !asi_detect_mount(device)) {
			result = false;
			indigo_send_message(device, "Handshake failed, not a ZWO AM mount");
			asi_close(device);
		}
		if (result) {
			asi_init_mount(device);
			// initialize target
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		if (--PRIVATE_DATA->device_count == 0) {
			asi_stop(device);
			asi_close(device);
		}
		indigo_delete_property(device, MOUNT_MODE_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
		indigo_delete_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void mount_home_callback(indigo_device *device) {
	if (MOUNT_HOME_ITEM->sw.value) {
		MOUNT_HOME_ITEM->sw.value = false;
		if (!asi_home(device)) {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
		} else {
			PRIVATE_DATA->prev_home_state = false;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home");
		}
	}
}

static void mount_park_callback(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
		if (asi_set_tracking(device, false) && asi_park(device)) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Going to park position");
			/* Wait 1s to make sure the mount reports moving. We use moving flag to detect
			   when finished because fs a firmware bug mount never reports parked.
			*/
			indigo_sleep(1);
			PRIVATE_DATA->park_requested = true;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
	}
}

static void mount_clear_alignmnet_callback(indigo_device *device) {
	if (MOUNT_ALIGNMENT_RESET_ITEM->sw.value) {
		MOUNT_ALIGNMENT_RESET_ITEM->sw.value = false;
		if (!asi_clear_alignment_data(device)) {
			MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, "Alignment data clearing failed");
		} else {
			MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, "Alignment data cleared");
		}
	}
}

static void mount_geo_coords_callback(indigo_device *device) {
	if (asi_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value))
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_eq_coords_callback(indigo_device *device) {
	char message[50] = {0};
	int error_code = 0;
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (asi_set_tracking_rate(device) && asi_slew(device, ra, dec, &error_code)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			strcpy(message, asi_error_string(error_code));
			if (*message == '\0') strcpy(message, "Slew failed");
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (asi_sync(device, ra, dec, &error_code)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			strcpy(message, asi_error_string(error_code));
			if (*message == '\0') strcpy(message, "Sync failed");
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
		if (asi_stop(device)) {
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

			PRIVATE_DATA->prev_home_state = false;
			MOUNT_HOME_ITEM->sw.value = false;
			MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);

			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Failed to abort");
		}
	}
}

static void mount_motion_dec_callback(indigo_device *device) {
	if (asi_set_slew_rate(device) && asi_motion_dec(device)) {
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
	if (asi_set_slew_rate(device) && asi_motion_ra(device)) {
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
		if (asi_set_utc(device, &secs, indigo_get_utc_offset())) {
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, " Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
	} else {
		if (asi_set_utc(device, &secs, offset)) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
}

static void mount_tracking_callback(indigo_device *device) {
	if (asi_set_tracking(device, MOUNT_TRACKING_ON_ITEM->sw.value)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_callback(indigo_device *device) {
	if (asi_set_tracking_rate(device)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_guide_rate_callback(indigo_device *device) {
	MOUNT_GUIDE_RATE_DEC_ITEM->number.value =
	MOUNT_GUIDE_RATE_DEC_ITEM->number.target =
	MOUNT_GUIDE_RATE_RA_ITEM->number.value = MOUNT_GUIDE_RATE_RA_ITEM->number.target;
	if (asi_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.target, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.target))
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	else
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void zwo_buzzer_callback(indigo_device *device) {
	if (ZWO_BUZZER_OFF_ITEM->sw.value) {
		asi_command(device, ":SBu0#", NULL, 0, 0);
	} else if (ZWO_BUZZER_LOW_ITEM->sw.value) {
		asi_command(device, ":SBu1#", NULL, 0, 0);
	} else if (ZWO_BUZZER_HIGH_ITEM->sw.value) {
		asi_command(device, ":SBu2#", NULL, 0, 0);
	}
	ZWO_BUZZER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
}

static void zwo_meridian_action_callback(indigo_device *device) {
	if (asi_set_meridian_action(device, ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value, ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value)) {
		ZWO_MERIDIAN_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		ZWO_MERIDIAN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	asi_get_meridian_settings(device, &ZWO_MERIDIAN_AUTO_FLIP_ITEM->sw.value, &ZWO_MERIDIAN_TRACK_PASSED_ITEM->sw.value, NULL);
	indigo_update_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
}

static void zwo_meridian_limit_callback(indigo_device *device) {
	if (asi_set_meridian_limit(device, (int)ZWO_MERIDIAN_LIMIT_ITEM->number.target)) {
		ZWO_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		ZWO_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	int value;
	asi_get_meridian_settings(device, NULL, NULL, &value);
	ZWO_MERIDIAN_LIMIT_ITEM->number.value = value;
	indigo_update_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		//strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ZWO_Mount");
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		MOUNT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Mount mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(EQUATORIAL_ITEM, EQUATORIAL_ITEM_NAME, "Equatorial mode", false);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		MOUNT_MODE_PROPERTY->hidden = true;
		// ---------------------------------------------------------------------------- ZWO_BUZZER
		ZWO_BUZZER_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_BUZZER_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Buzzer volume", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (ZWO_BUZZER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZWO_BUZZER_OFF_ITEM, ZWO_BUZZER_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(ZWO_BUZZER_LOW_ITEM, ZWO_BUZZER_LOW_ITEM_NAME, "Low", false);
		indigo_init_switch_item(ZWO_BUZZER_HIGH_ITEM, ZWO_BUZZER_HIGH_ITEM_NAME, "High", false);
		ZWO_BUZZER_PROPERTY->hidden = true;
		// ---------------------------------------------------------------------------- ZWO_MERIDIAN
		ZWO_MERIDIAN_PROPERTY = indigo_init_switch_property(NULL, device->name, ZWO_MERIDIAN_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Action at meridian", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (ZWO_MERIDIAN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZWO_MERIDIAN_AUTO_FLIP_ITEM, ZWO_MERIDIAN_AUTO_FLIP_ITEM_NAME, "Enable auto meridian flip (at limit)", false);
		indigo_init_switch_item(ZWO_MERIDIAN_TRACK_PASSED_ITEM, ZWO_MERIDIAN_TRACK_PASSED_ITEM_NAME, "Enable tracking passed meridian (to the limit)", false);
		ZWO_MERIDIAN_PROPERTY->hidden = true;
		// ---------------------------------------------------------------------------- ZWO_MERIDIAN_LIMIT
		ZWO_MERIDIAN_LIMIT_PROPERTY = indigo_init_number_property(NULL, device->name, ZWO_MERIDIAN_LIMIT_PROPERTY_NAME, MOUNT_ADVANCED_GROUP, "Meridian limit", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (ZWO_MERIDIAN_LIMIT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(ZWO_MERIDIAN_LIMIT_ITEM, ZWO_MERIDIAN_LIMIT_ITEM_NAME, "Limit (°) (<0° before meridian)", -15, 15, 0, 0);
		ZWO_MERIDIAN_LIMIT_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(MOUNT_MODE_PROPERTY);
		indigo_define_matching_property(ZWO_BUZZER_PROPERTY);
		indigo_define_matching_property(ZWO_MERIDIAN_PROPERTY);
		indigo_define_matching_property(ZWO_MERIDIAN_LIMIT_PROPERTY);
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
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_RESET
		indigo_property_copy_values(MOUNT_ALIGNMENT_RESET_PROPERTY, property, false);
		if (MOUNT_ALIGNMENT_RESET_ITEM->sw.value) {
			MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_clear_alignmnet_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_home_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_park_callback, NULL);
		}
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
		PRIVATE_DATA->motioned = false; // WTF?
		indigo_property_copy_targets(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_eq_coords_callback, NULL);
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
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_motion_dec_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_motion_ra_callback, NULL);
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
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_tracking_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_track_rate_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_guide_rate_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_BUZZER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ZWO_BUZZER
		indigo_property_copy_values(ZWO_BUZZER_PROPERTY, property, false);
		ZWO_BUZZER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ZWO_BUZZER_PROPERTY, NULL);
		indigo_set_timer(device, 0, zwo_buzzer_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_MERIDIAN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ZWO_MERIDIAN
		indigo_property_copy_values(ZWO_MERIDIAN_PROPERTY, property, false);
		ZWO_MERIDIAN_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ZWO_MERIDIAN_PROPERTY, NULL);
		indigo_set_timer(device, 0, zwo_meridian_action_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZWO_MERIDIAN_LIMIT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ZWO_MERIDIAN_LIMIT
		indigo_property_copy_values(ZWO_MERIDIAN_LIMIT_PROPERTY, property, false);
		ZWO_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ZWO_MERIDIAN_LIMIT_PROPERTY, NULL);
		indigo_set_timer(device, 0, zwo_meridian_limit_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_EPOCH_PROPERTY);
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
	indigo_release_property(ZWO_MERIDIAN_PROPERTY);
	indigo_release_property(ZWO_MERIDIAN_LIMIT_PROPERTY);
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
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = asi_open(device->master_device);
		}
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			char response[128];
			if (asi_command(device, ":GVP#", response, sizeof(response), 0)) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Product: '%s'", response);
				strncpy(PRIVATE_DATA->product, response, 64);
				if (!strncmp(PRIVATE_DATA->product, "AM", 2) && isdigit(PRIVATE_DATA->product[2])) {
					GUIDER_GUIDE_NORTH_ITEM->number.max =
					GUIDER_GUIDE_SOUTH_ITEM->number.max =
					GUIDER_GUIDE_EAST_ITEM->number.max =
					GUIDER_GUIDE_WEST_ITEM->number.max = 3000;
				}
			}
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (--PRIVATE_DATA->device_count == 0) {
			asi_close(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void guider_guide_dec_callback(indigo_device *device) {
	int north = GUIDER_GUIDE_NORTH_ITEM->number.value;
	int south = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	asi_guide_dec(device, north, south);
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
	asi_guide_ra(device, west, east);
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

static void device_network_disconnection(indigo_device* device, indigo_timer_callback callback) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		callback(device);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;  // The alert state signals the unexpected disconnection
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		// Sending message as this update will not pass through the agent
		indigo_send_message(device, "Error: Device disconnected unexpectedly", device->name);
	}
	// Otherwise not previously connected, nothing to do
}

// --------------------------------------------------------------------------------

static asi_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

static void network_disconnection(__attribute__((unused)) indigo_device* device) {
	// Since the two devices share the same TCP connection,
	// process the disconnection on all of them
	device_network_disconnection(mount, mount_connect_callback);
	device_network_disconnection(mount_guider, guider_connect_callback);
}

indigo_result indigo_mount_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_ASI_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);

	static indigo_device_match_pattern patterns[1] = {0};
	patterns[0].vendor_id = 0x03C3;
	patterns[0].product_id = 0x4001;
	INDIGO_REGISER_MATCH_PATTERNS(mount_template, patterns, 1);

	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_ASI_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO AM Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			tzset();
			private_data = indigo_safe_malloc(sizeof(asi_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			mount->master_device = mount;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			mount_guider->master_device = mount;
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
