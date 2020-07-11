// Copyright (c) 2020 CloudMakers, s. r. o.
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

/** INDIGO PCM Eight driver
 \file indigo_mount_pcm8.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_mount_pcm8"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_mount_pcm8.h"

#define PRIVATE_DATA        ((pcm8_private_data *)device->private_data)

// G11, EXOS2/iEXOS100
// SIDEREAL, LUNAR, SOLAR

static int RATES[][3] = {
	{ 1333, 1285, 1337},
	{ 1200, 1156, 1203 }
};

typedef struct {
	int handle;
	indigo_timer *position_timer;
	pthread_mutex_t port_mutex;
	indigo_network_protocol proto;
	bool park;
	int rate;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
} pcm8_private_data;

static bool pcm8_command(indigo_device *device, char *command, char *response, int max, int sleep);

static bool pcm8_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_is_device_url(name, "pcm8")) {
		PRIVATE_DATA->proto = -1;
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
	} else {
		PRIVATE_DATA->proto = INDIGO_PROTOCOL_UDP;
		PRIVATE_DATA->handle = indigo_open_network_device(name, 54372, &PRIVATE_DATA->proto);
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
}

static bool pcm8_command(indigo_device *device, char *command, char *response, int max, int sleep) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	char c;
	struct timeval tv;
	// flush
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0)
			break;
		if (result < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
		if (PRIVATE_DATA->proto == INDIGO_PROTOCOL_UDP) {
			char buf[64];
			result = recv(PRIVATE_DATA->handle, buf, sizeof(buf), 0);
		} else {
			result = read(PRIVATE_DATA->handle, &c, 1);
		}
		if (result < 1) {
			pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
			return false;
		}
	}
	for (int repeat = 10; ; repeat--) {
	// write command
		if (PRIVATE_DATA->proto == INDIGO_PROTOCOL_UDP) {
			send(PRIVATE_DATA->handle, command, strlen(command), 0);
		} else {
			indigo_write(PRIVATE_DATA->handle, command, strlen(command));
		}
		if (sleep > 0)
			indigo_usleep(sleep);
		// read response
		if (response != NULL) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result == 0) {
				if (repeat > 0) {
					continue;
				} else {
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s failed", command);
					return false;
				}
			}
			if (PRIVATE_DATA->proto == INDIGO_PROTOCOL_UDP) {
				long bytes_read = recv(PRIVATE_DATA->handle, response, max, 0);
				response[bytes_read] = 0;
			} else {
				// TBD
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
			for (char *tmp = response; *tmp; tmp++) {
				if (*tmp == '!') {
					*tmp = 0;
					break;
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return true;
}

static void pcm8_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static bool pcm8_get_tracking_rate(indigo_device *device) {
	char response[32];
	if (pcm8_command(device, "ESGx!", response, sizeof(response), 0)) {
		int rate = (int)strtol(response + 4, NULL, 16);
		if (rate == 0) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		} else {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			if (rate == RATES[PRIVATE_DATA->rate][0]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			} else if (rate == RATES[PRIVATE_DATA->rate][1]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (rate == RATES[PRIVATE_DATA->rate][2]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			}
		}
		return true;
	}
	return false;
}

static bool pcm8_set_tracking_rate(indigo_device *device) {
	char command[32], response[32];
	int rate = 0;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			rate = RATES[PRIVATE_DATA->rate][0];
		} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
			rate = RATES[PRIVATE_DATA->rate][1];
		} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			rate = RATES[PRIVATE_DATA->rate][2];
		}
	}
	sprintf(command, "ESTr%04X!", rate);
	if (pcm8_command(device, command, response, sizeof(response), 0)) {
		return true;
	}
	return false;
}

static bool pcm8_point(indigo_device *device, uint32_t ra, uint32_t dec) {
	char command[32], response[32];
	sprintf(command, "ESPt0%06X!", ra);
	if (!pcm8_command(device, command, response, sizeof(response), 0)) {
		return false;
	}
	sprintf(command, "ESPt1%06X!", dec);
	if (!pcm8_command(device, command, response, sizeof(response), 0)) {
		return false;
	}
	return true;
}

static bool pcm8_move(indigo_device *device, int axis, int direction, int speed) {
	char command[32], response[32];
	if (speed == 0) {
		if (axis == 0) {
			return pcm8_set_tracking_rate(device);
		} else {
			return pcm8_command(device, "ESSr10000!", response, sizeof(response), 0);
		}
	} else {
		sprintf(command, "ESSd%d%d!", axis, direction);
		if (!pcm8_command(device, command, response, sizeof(response), 0)) {
			return false;
		}
		sprintf(command, "ESSr%d%04X!", axis, abs(speed));
		if (!pcm8_command(device, command, response, sizeof(response), 0)) {
			return false;
		}
	}
	return true;
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
		// -------------------------------------------------------------------------------- MOUNT_INFO
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Explore Scientific");
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

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		char response[32];
		uint32_t raw_ra, raw_dec;
		if (pcm8_command(device, "ESGp0!", response, sizeof(response), 0)) {
			raw_ra = (int)strtol(response + 5, NULL, 16);
			if (pcm8_command(device, "ESGp1!", response, sizeof(response), 0)) {
				raw_dec = (int)strtol(response + 5, NULL, 16);
				if (raw_ra == 0 && raw_dec == 0 && MOUNT_TRACKING_OFF_ITEM->sw.value && PRIVATE_DATA->park) {
					PRIVATE_DATA->park = false;
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
				}
			}
		}
		// TBD
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

static void mount_connect_callback(indigo_device *device) {
	char response[32];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = pcm8_open(device);
		if (result) {
			if (pcm8_command(device, "ESGv!", response, sizeof(response), 0)) {
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response + 4);
				if (strstr(response, "G11")) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "G11");
					PRIVATE_DATA->rate = 0;
				} else if (strstr(response, "EXOS2")) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "EXOS2");
					PRIVATE_DATA->rate = 1;
				} else if (strstr(response, "ES1A")) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "iEXOS100");
					PRIVATE_DATA->rate = 1;
				}
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				if (pcm8_get_tracking_rate(device)) {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
					MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
					MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		PRIVATE_DATA->position_timer = NULL;
		pcm8_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_tracking_callback(indigo_device *device) {
	if (pcm8_set_tracking_rate(device)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_callback(indigo_device *device) {
	if (pcm8_set_tracking_rate(device)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_park_callback(indigo_device *device) {
	PRIVATE_DATA->park = true;
	if (!pcm8_point(device, 0, 0)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	}
}

static void mount_abort_motion_callback(indigo_device *device) {
	if (pcm8_move(device, 0, 0, 0) && pcm8_move(device, 1, 0, 0)) {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	}
	MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
}

static void mount_motion_callback(indigo_device *device) {
	int rate = 0, direction = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		rate = RATES[PRIVATE_DATA->rate][0];
	else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		rate = 0x1000;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		rate = 0x3000;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		rate = 0xFFFF;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
		direction = 0;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value) {
		direction = 1;
	} else {
		rate = 0;
	}
	if (MOUNT_MOTION_DEC_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (pcm8_move(device, 1, direction, rate))
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	} else if (MOUNT_MOTION_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (pcm8_move(device, 0, direction, rate))
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	}
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
		if (IS_CONNECTED) {
			bool parked = MOUNT_PARK_PARKED_ITEM->sw.value;
			indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
			if (!parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
				indigo_set_timer(device, 0, mount_park_callback, NULL);
			}
			if (parked && MOUNT_PARK_UNPARKED_ITEM->sw.value) {
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_coordinates(device, "Mount is parked!");
		} else {
			// TBD
			indigo_update_coordinates(device, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked!");
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_abort_motion_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		} else {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		} else {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_motion_callback, NULL);
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
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_track_rate_callback, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
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

static pcm8_private_data *private_data = NULL;

static indigo_device *mount = NULL;

indigo_result indigo_mount_pcm8(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_RAINBOW_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "PCM Eight Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(pcm8_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(pcm8_private_data));
			mount = malloc(sizeof(indigo_device));
			assert(mount != NULL);
			memcpy(mount, &mount_template, sizeof(indigo_device));
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
