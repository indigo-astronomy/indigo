// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

/** INDIGO LX200 Server agent
 \file indigo_agent_lx200_server.c
 */

#define DRIVER_VERSION 0x02000004
#define DRIVER_NAME	"indigo_agent_lx200_server"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>

#include "indigo_agent_lx200_server.h"

#define DEVICE_PRIVATE_DATA										((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((agent_private_data *)client->client_context)

#define LX200_DEVICES_PROPERTY								(DEVICE_PRIVATE_DATA->lx200_devices_property)
#define LX200_DEVICES_MOUNT_ITEM							(LX200_DEVICES_PROPERTY->items+0)
#define LX200_DEVICES_GUIDER_ITEM							(LX200_DEVICES_PROPERTY->items+1)

#define LX200_CONFIGURATION_PROPERTY					(DEVICE_PRIVATE_DATA->lx200_configuration_property)
#define LX200_CONFIGURATION_PORT_ITEM					(LX200_CONFIGURATION_PROPERTY->items+0)
#define LX200_CONFIGURATION_EPOCH_ITEM				(LX200_CONFIGURATION_PROPERTY->items+1)

#define LX200_SERVER_PROPERTY									(DEVICE_PRIVATE_DATA->lx200_server_property)
#define LX200_SERVER_STARTED_ITEM							(LX200_SERVER_PROPERTY->items+0)
#define LX200_SERVER_STOPPED_ITEM							(LX200_SERVER_PROPERTY->items+1)

#define MOUNT_EQUATORIAL_COORDINATES_PROPERTY	(DEVICE_PRIVATE_DATA->mount_equatorial_coordinates_property)
#define MOUNT_EQUATORIAL_COORDINATES_RA_ITEM	(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+0)
#define MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM	(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+1)

#define MOUNT_ON_COORDINATES_SET_PROPERTY			(DEVICE_PRIVATE_DATA->mount_on_coordinates_set_property)
#define MOUNT_ON_COORDINATES_SET_TRACK_ITEM		(MOUNT_ON_COORDINATES_SET_PROPERTY->items+0)
#define MOUNT_ON_COORDINATES_SET_SYNC_ITEM		(MOUNT_ON_COORDINATES_SET_PROPERTY->items+1)

#define MOUNT_SLEW_RATE_PROPERTY							(DEVICE_PRIVATE_DATA->mount_slew_rate_property)
#define MOUNT_SLEW_RATE_GUIDE_ITEM						(MOUNT_SLEW_RATE_PROPERTY->items+0)
#define MOUNT_SLEW_RATE_CENTERING_ITEM				(MOUNT_SLEW_RATE_PROPERTY->items+1)
#define MOUNT_SLEW_RATE_FIND_ITEM							(MOUNT_SLEW_RATE_PROPERTY->items+2)
#define MOUNT_SLEW_RATE_MAX_ITEM							(MOUNT_SLEW_RATE_PROPERTY->items+3)

#define MOUNT_MOTION_DEC_PROPERTY							(DEVICE_PRIVATE_DATA->mount_motion_dec_property)
#define MOUNT_MOTION_NORTH_ITEM								(MOUNT_MOTION_DEC_PROPERTY->items+0)
#define MOUNT_MOTION_SOUTH_ITEM						    (MOUNT_MOTION_DEC_PROPERTY->items+1)

#define MOUNT_MOTION_RA_PROPERTY							(DEVICE_PRIVATE_DATA->mount_motion_ra_property)
#define MOUNT_MOTION_WEST_ITEM								(MOUNT_MOTION_RA_PROPERTY->items+0)
#define MOUNT_MOTION_EAST_ITEM						    (MOUNT_MOTION_RA_PROPERTY->items+1)

#define MOUNT_ABORT_MOTION_PROPERTY						(DEVICE_PRIVATE_DATA->mount_abort_motion_property)
#define MOUNT_ABORT_MOTION_ITEM								(MOUNT_ABORT_MOTION_PROPERTY->items+0)

#define MOUNT_PARK_PROPERTY										(DEVICE_PRIVATE_DATA->mount_park_property)
#define MOUNT_PARK_PARKED_ITEM								(MOUNT_PARK_PROPERTY->items+0)
#define MOUNT_PARK_UNPARKED_ITEM							(MOUNT_PARK_PROPERTY->items+1)

typedef struct {
	indigo_property *lx200_devices_property;
	indigo_property *lx200_configuration_property;
	indigo_property *lx200_server_property;
	indigo_property *mount_equatorial_coordinates_property;
	indigo_property *mount_on_coordinates_set_property;
	indigo_property *mount_slew_rate_property;
	indigo_property *mount_motion_dec_property;
	indigo_property *mount_motion_ra_property;
	indigo_property *mount_abort_motion_property;
	indigo_property *mount_park_property;
	indigo_device *device;
	indigo_client *client;
	bool unparked;
	double ra, dec;
	double target_ra, target_dec;
	int server_socket;
} agent_private_data;

typedef struct {
	int client_socket;
	indigo_device *device;
} handler_data;

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// -------------------------------------------------------------------------------- LX200 server implementation

static char * doubleToSexa(double value, char *format) {
	static char buffer[128];
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = round(60.0 * (m - floor(m)));
	if (s == 60) {
		s = 0;
		++m;
	}
	if (m == 60) {
		m = 0;
		++d;
	}
	if (value < 0) {
		d = -d;
	}
	snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, (int)s);
	return buffer;
}

static void unpark(indigo_device *device) {
	if (!DEVICE_PRIVATE_DATA->unparked) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_PARK_PROPERTY->device);
		indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_PARK_PROPERTY);
		for (int i = 0; i < 10; i++) {
			if (DEVICE_PRIVATE_DATA->unparked) {
				INDIGO_DRIVER_DEBUG(LX200_SERVER_AGENT_NAME, "Unparked");
				break;
			}
			sleep(1);
		}
	}
}

static void start_worker_thread(handler_data *data) {
	indigo_device *device = data->device;
	int client_socket = data->client_socket;
	char buffer_in[128];
	char buffer_out[128];
	long result = 1;

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	INDIGO_DRIVER_TRACE(LX200_SERVER_AGENT_NAME, "%d: CONNECTED", client_socket);

	while (true) {
		*buffer_in = 0;
		*buffer_out = 0;
		result = read(client_socket, buffer_in, 1);
		if (result <= 0) {
			break;
		}
		if (*buffer_in == 6) {
			strcpy(buffer_out, "P");
		} else if (*buffer_in == '#') {
			continue;
		} else if (*buffer_in == ':') {
			int i = 0;
			while (i < sizeof(buffer_in)) {
				result = read(client_socket, buffer_in + i, 1);
				if (result <= 0) {
					break;
				}
				if (buffer_in[i] == '#') {
					buffer_in[i] = 0;
					break;
				}
				i++;
			}
			if (result == -1) {
				break;
			}
			if (strcmp(buffer_in, "GVP") == 0) {
				strcpy(buffer_out, "indigo#");
			} else if (strcmp(buffer_in, "GR") == 0) {
				double ra = DEVICE_PRIVATE_DATA->ra;
				double dec = DEVICE_PRIVATE_DATA->dec;
				indigo_j2k_to_eq(LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				strcpy(buffer_out, doubleToSexa(ra, "%02d:%02d:%02d#"));
			} else if (strcmp(buffer_in, "GD") == 0) {
				double ra = DEVICE_PRIVATE_DATA->ra;
				double dec = DEVICE_PRIVATE_DATA->dec;
				indigo_j2k_to_eq(LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				strcpy(buffer_out, doubleToSexa(dec, "%+03d*%02d'%02d#"));
			} else if (strncmp(buffer_in, "Sr", 2) == 0) {
				int h = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &h, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->target_ra = h + m/60.0 + s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &h, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->target_ra = h + m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "Sd", 2) == 0) {
				int d = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &d, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->target_dec = d > 0 ? d + m/60.0 + s/3600.0 : d - m/60.0 - s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &d, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->target_dec = d > 0 ? d + m/60.0 : d - m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "MS", 2) == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_ON_COORDINATES_SET_PROPERTY, MOUNT_ON_COORDINATES_SET_TRACK_ITEM, true);
				MOUNT_ON_COORDINATES_SET_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_ON_COORDINATES_SET_PROPERTY->device);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->access_token = MOUNT_ON_COORDINATES_SET_PROPERTY->access_token;
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_ON_COORDINATES_SET_PROPERTY);
				double ra = DEVICE_PRIVATE_DATA->target_ra;
				double dec = DEVICE_PRIVATE_DATA->target_dec;
				indigo_eq_to_j2k(LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
				strcpy(buffer_out, "0");
			} else if (strncmp(buffer_in, "CM", 2) == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_ON_COORDINATES_SET_PROPERTY, MOUNT_ON_COORDINATES_SET_SYNC_ITEM, true);
				MOUNT_ON_COORDINATES_SET_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_ON_COORDINATES_SET_PROPERTY->device);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->access_token = MOUNT_ON_COORDINATES_SET_PROPERTY->access_token;
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_ON_COORDINATES_SET_PROPERTY);
				double ra = DEVICE_PRIVATE_DATA->target_ra;
				double dec = DEVICE_PRIVATE_DATA->target_dec;
				indigo_eq_to_j2k(LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
				MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
				strcpy(buffer_out, "OK#");
			} else if (strcmp(buffer_in, "RG") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_GUIDE_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
			} else if (strcmp(buffer_in, "RC") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_CENTERING_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
			} else if (strcmp(buffer_in, "RM") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_FIND_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
			} else if (strcmp(buffer_in, "RS") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_MAX_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
			} else if (strcmp(buffer_in, "Sw2") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_CENTERING_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw3") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_FIND_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw4") == 0) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_MAX_ITEM, true);
				MOUNT_SLEW_RATE_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_SLEW_RATE_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_SLEW_RATE_PROPERTY);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Mn") == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_MOTION_DEC_PROPERTY, MOUNT_MOTION_NORTH_ITEM, true);
				MOUNT_MOTION_DEC_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_DEC_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_DEC_PROPERTY);
			} else if (strcmp(buffer_in, "Qn") == 0) {
				indigo_set_switch(MOUNT_MOTION_DEC_PROPERTY, MOUNT_MOTION_NORTH_ITEM, false);
				MOUNT_MOTION_DEC_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_DEC_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_DEC_PROPERTY);
			} else if (strcmp(buffer_in, "Ms") == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_MOTION_DEC_PROPERTY, MOUNT_MOTION_SOUTH_ITEM, true);
				MOUNT_MOTION_DEC_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_DEC_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_DEC_PROPERTY);
			} else if (strcmp(buffer_in, "Qs") == 0) {
				indigo_set_switch(MOUNT_MOTION_DEC_PROPERTY, MOUNT_MOTION_SOUTH_ITEM, false);
				MOUNT_MOTION_DEC_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_DEC_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_DEC_PROPERTY);
			} else if (strcmp(buffer_in, "Mw") == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_MOTION_RA_PROPERTY, MOUNT_MOTION_WEST_ITEM, true);
				MOUNT_MOTION_RA_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_RA_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_RA_PROPERTY);
			} else if (strcmp(buffer_in, "Qw") == 0) {
				indigo_set_switch(MOUNT_MOTION_RA_PROPERTY, MOUNT_MOTION_WEST_ITEM, false);
				MOUNT_MOTION_RA_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_RA_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_RA_PROPERTY);
			} else if (strcmp(buffer_in, "Me") == 0) {
				unpark(device);
				indigo_set_switch(MOUNT_MOTION_RA_PROPERTY, MOUNT_MOTION_EAST_ITEM, true);
				MOUNT_MOTION_RA_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_RA_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_RA_PROPERTY);
			} else if (strcmp(buffer_in, "Qe") == 0) {
				indigo_set_switch(MOUNT_MOTION_RA_PROPERTY, MOUNT_MOTION_EAST_ITEM, false);
				MOUNT_MOTION_RA_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_MOTION_RA_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_MOTION_RA_PROPERTY);
			} else if (strcmp(buffer_in, "Q") == 0) {
				indigo_set_switch(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_ABORT_MOTION_ITEM, true);
				MOUNT_ABORT_MOTION_PROPERTY->access_token = indigo_get_device_or_master_token(MOUNT_ABORT_MOTION_PROPERTY->device);
				indigo_change_property(DEVICE_PRIVATE_DATA->client, MOUNT_ABORT_MOTION_PROPERTY);
			} else if (strncmp(buffer_in, "SC", 2) == 0) {
				strcpy(buffer_out, "1Updating        planetary data. #                              #");
			} else if (strncmp(buffer_in, "S", 1) == 0) {
				strcpy(buffer_out, "1");
			}
			if (*buffer_out) {
				indigo_write(client_socket, buffer_out, strlen(buffer_out));
				if (*buffer_in == 'G') {
					INDIGO_DRIVER_TRACE(LX200_SERVER_AGENT_NAME, "%d: '%s' -> '%s'", client_socket, buffer_in, buffer_out);
				} else {
					INDIGO_DRIVER_DEBUG(LX200_SERVER_AGENT_NAME, "%d: '%s' -> '%s'", client_socket, buffer_in, buffer_out);
				}
			} else {
				INDIGO_DRIVER_DEBUG(LX200_SERVER_AGENT_NAME, "%d: '%s' -> ", client_socket, buffer_in);
			}
		}
	}
	INDIGO_DRIVER_TRACE(LX200_SERVER_AGENT_NAME, "%d: DISCONNECTED", client_socket);

	close(client_socket);
	free(data);
}

static void start_listener_thread(indigo_device *device) {
	struct sockaddr_in client_name;
	unsigned int name_len = sizeof(client_name);
	INDIGO_DRIVER_LOG(LX200_SERVER_AGENT_NAME, "Server started on %d", (int)LX200_CONFIGURATION_PORT_ITEM->number.value);
	while (DEVICE_PRIVATE_DATA->server_socket) {
		int client_socket = accept(DEVICE_PRIVATE_DATA->server_socket, (struct sockaddr *)&client_name, &name_len);
		if (client_socket != -1) {
			handler_data *data = indigo_safe_malloc(sizeof(handler_data));
			data->client_socket = client_socket;
			data->device = device;
			if (!indigo_async((void *(*)(void *))start_worker_thread, data))
				INDIGO_DRIVER_ERROR(LX200_SERVER_AGENT_NAME, "Can't create worker thread for connection (%s)", strerror(errno));
		}
	}
	INDIGO_DRIVER_LOG(LX200_SERVER_AGENT_NAME, "Server finished");
}

static bool start_server(indigo_device *device) {
	int port = (int)LX200_CONFIGURATION_PORT_ITEM->number.value;
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: socket() failed (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
		return false;
	}
  int reuse = 1;
  if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    close(server_socket);
    LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: setsockopt() failed (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
    return false;
  }
  struct sockaddr_in server_address;
	unsigned int server_address_length = sizeof(server_address);
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_socket, (struct sockaddr *)&server_address, server_address_length) < 0) {
		close(server_socket);
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: bind() failed (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
		return false;
	}
	if (getsockname(server_socket, (struct sockaddr *)&(server_address), &server_address_length) < 0) {
		close(server_socket);
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: getsockname() failed (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
		return false;
	}
	if (listen(server_socket, 5) < 0) {
		close(server_socket);
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: Can't listen on server socket (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
		return false;
	}
	if (port == 0) {
		LX200_CONFIGURATION_PORT_ITEM->number.value = port = ntohs(server_address.sin_port);
		LX200_CONFIGURATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LX200_CONFIGURATION_PROPERTY, NULL);
	}
  DEVICE_PRIVATE_DATA->server_socket = server_socket;
	if (!indigo_async((void *(*)(void *))start_listener_thread, device)) {
		close(server_socket);
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
    indigo_update_property(device, LX200_SERVER_PROPERTY, "%s: Can't create listener thread (%s)", LX200_SERVER_AGENT_NAME, strerror(errno));
		return false;
	}
	LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, LX200_SERVER_PROPERTY, NULL);
	return true;
}

static void shutdown_server(indigo_device *device) {
	int server_socket = DEVICE_PRIVATE_DATA->server_socket;
	if (server_socket) {
		DEVICE_PRIVATE_DATA->server_socket = 0;
		shutdown(server_socket, SHUT_RDWR);
		close(server_socket);
		LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, LX200_SERVER_PROPERTY, NULL);
}

static void start_server_callback(indigo_device *device) {
	if (start_server(device)) {
		LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_switch(LX200_SERVER_PROPERTY, LX200_SERVER_STOPPED_ITEM, true);
		LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, LX200_SERVER_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		CONFIG_PROPERTY->hidden = false;
		PROFILE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- local device properties
		LX200_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, LX200_DEVICES_PROPERTY_NAME, MAIN_GROUP, "Devices", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (LX200_DEVICES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(LX200_DEVICES_MOUNT_ITEM, LX200_DEVICES_MOUNT_ITEM_NAME, "Mount", "");
		indigo_init_text_item(LX200_DEVICES_GUIDER_ITEM, LX200_DEVICES_GUIDER_ITEM_NAME, "Guider", "");
		LX200_CONFIGURATION_PROPERTY = indigo_init_number_property(NULL, device->name, LX200_CONFIGURATION_PROPERTY_NAME, MAIN_GROUP, "Configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (LX200_CONFIGURATION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(LX200_CONFIGURATION_PORT_ITEM, LX200_CONFIGURATION_PORT_ITEM_NAME, "Server port", 0, 0xFFFF, 0, 4030);
		indigo_init_number_item(LX200_CONFIGURATION_EPOCH_ITEM, LX200_CONFIGURATION_EPOCH_ITEM_NAME, "Epoch (0=JNow, 2000=J2k)", 0, 2050, 0, 0);
		LX200_SERVER_PROPERTY = indigo_init_switch_property(NULL, device->name, LX200_SERVER_PROPERTY_NAME, MAIN_GROUP, "Server", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (LX200_SERVER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(LX200_SERVER_STARTED_ITEM, LX200_SERVER_STARTED_ITEM_NAME, "Started", false);
		indigo_init_switch_item(LX200_SERVER_STOPPED_ITEM, LX200_SERVER_STOPPED_ITEM_NAME, "Stopped", true);
		// -------------------------------------------------------------------------------- remote device properties
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, NULL, 0, 24, 0, 0);
		indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, NULL, -90, 90, 0, 90);
		MOUNT_ON_COORDINATES_SET_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_ON_COORDINATES_SET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_TRACK_ITEM, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SYNC_ITEM, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, NULL, false);
		MOUNT_SLEW_RATE_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 4);
		if (MOUNT_SLEW_RATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_SLEW_RATE_GUIDE_ITEM, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_CENTERING_ITEM, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_FIND_ITEM, MOUNT_SLEW_RATE_FIND_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_MAX_ITEM, MOUNT_SLEW_RATE_MAX_ITEM_NAME, NULL, false);
		MOUNT_MOTION_DEC_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_MOTION_DEC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_MOTION_NORTH_ITEM, MOUNT_MOTION_NORTH_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_MOTION_SOUTH_ITEM, MOUNT_MOTION_SOUTH_ITEM_NAME, NULL, false);
		MOUNT_MOTION_RA_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_MOTION_RA_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_MOTION_WEST_ITEM, MOUNT_MOTION_WEST_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_MOTION_EAST_ITEM, MOUNT_MOTION_EAST_ITEM_NAME, NULL, false);
		MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (MOUNT_ABORT_MOTION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, MOUNT_ABORT_MOTION_ITEM_NAME, NULL, false);
		MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, LX200_DEVICES_MOUNT_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_PARK_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, MOUNT_PARK_PARKED_ITEM_NAME, NULL, false);
		indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, MOUNT_PARK_UNPARKED_ITEM_NAME, NULL, false);

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client!= NULL && client == DEVICE_PRIVATE_DATA->client) {
		return INDIGO_OK;
	}
	indigo_result result = INDIGO_OK;
	if ((result = indigo_agent_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (indigo_property_match(LX200_DEVICES_PROPERTY, property))
			indigo_define_property(device, LX200_DEVICES_PROPERTY, NULL);
		if (indigo_property_match(LX200_CONFIGURATION_PROPERTY, property))
			indigo_define_property(device, LX200_CONFIGURATION_PROPERTY, NULL);
		if (indigo_property_match(LX200_SERVER_PROPERTY, property))
			indigo_define_property(device, LX200_SERVER_PROPERTY, NULL);
	}
	return result;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client) {
		return INDIGO_OK;
	}
	if (indigo_property_match(LX200_DEVICES_PROPERTY, property)) {
		indigo_property_copy_values(LX200_DEVICES_PROPERTY, property, false);
		INDIGO_COPY_NAME(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_ON_COORDINATES_SET_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_SLEW_RATE_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_MOTION_RA_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_MOTION_DEC_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_ABORT_MOTION_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		INDIGO_COPY_NAME(MOUNT_PARK_PROPERTY->device, LX200_DEVICES_MOUNT_ITEM->text.value);
		LX200_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LX200_DEVICES_PROPERTY, NULL);
	} else if (indigo_property_match(LX200_CONFIGURATION_PROPERTY, property)) {
		indigo_property_copy_values(LX200_CONFIGURATION_PROPERTY, property, false);
		if (LX200_CONFIGURATION_EPOCH_ITEM->number.target != 0 && LX200_CONFIGURATION_EPOCH_ITEM->number.target != 2000) {
			LX200_CONFIGURATION_EPOCH_ITEM->number.value = LX200_CONFIGURATION_EPOCH_ITEM->number.target = 0;
			indigo_send_message(device, "Warning! Valid values are 0 or 2000 only, value adjusted to 0");
		}
		LX200_CONFIGURATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, LX200_CONFIGURATION_PROPERTY, NULL);
	} else if (indigo_property_match(LX200_SERVER_PROPERTY, property)) {
		indigo_property_copy_values(LX200_SERVER_PROPERTY, property, false);
		if (LX200_SERVER_STARTED_ITEM->sw.value && DEVICE_PRIVATE_DATA->server_socket == 0) {
			LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 3, start_server_callback, NULL);
		} else if (LX200_SERVER_STOPPED_ITEM->sw.value && DEVICE_PRIVATE_DATA->server_socket != 0) {
			LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
			shutdown_server(device);
		} else {
			LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, LX200_SERVER_PROPERTY, NULL);
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, LX200_DEVICES_PROPERTY);
			indigo_save_property(device, NULL, LX200_CONFIGURATION_PROPERTY);
		}
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	shutdown_server(device);
	indigo_release_property(LX200_DEVICES_PROPERTY);
	indigo_release_property(LX200_CONFIGURATION_PROPERTY);
	indigo_release_property(LX200_SERVER_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_agent_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device) {
		return INDIGO_OK;
	}
	if (strcmp(device->name, CLIENT_PRIVATE_DATA->lx200_devices_property->items[0].text.value) == 0) {
		indigo_item *item;
		if (strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME) == 0) {
			if ((item = indigo_get_item(property, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)))
				CLIENT_PRIVATE_DATA->ra = item->number.value;
			if ((item = indigo_get_item(property, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)))
				CLIENT_PRIVATE_DATA->dec = item->number.value;
		} else if (strcmp(property->name, MOUNT_PARK_PROPERTY_NAME) == 0) {
			if ((item = indigo_get_item(property, MOUNT_PARK_UNPARKED_ITEM_NAME)))
				CLIENT_PRIVATE_DATA->unparked = (property->state == INDIGO_OK_STATE) && item->sw.value;
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device) {
		return INDIGO_OK;
	}
	if (strcmp(device->name, CLIENT_PRIVATE_DATA->lx200_devices_property->items[0].text.value) == 0) {
		agent_update_property(client, device, property, message);
	}
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_lx200_server(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		LX200_SERVER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		LX200_SERVER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		NULL,
		NULL,
		NULL
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "LX200 Server Agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			private_data->device = agent_device;
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			private_data->client = agent_client;
			agent_client->client_context = private_data;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
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
