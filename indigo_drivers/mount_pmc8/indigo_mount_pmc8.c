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

/** INDIGO PMC Eight driver
 \file indigo_mount_pmc8.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME	"indigo_mount_pmc8"

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
#include <indigo/indigo_novas.h>

#include "indigo_mount_pmc8.h"

#define PRIVATE_DATA        					((pmc8_private_data *)device->private_data)

#define CONNECTION_MODE_PROPERTY     			(PRIVATE_DATA->connection_mode_property)
#define CONNECTION_UDP_ITEM  							(CONNECTION_MODE_PROPERTY->items+0)
#define CONNECTION_TCP_ITEM  							(CONNECTION_MODE_PROPERTY->items+1)
#define CONNECTION_SERIAL_ITEM  					(CONNECTION_MODE_PROPERTY->items+2)
#define CONNECTION_SERIAL_DTR_ITEM  			(CONNECTION_MODE_PROPERTY->items+3)

#define CONNECTION_MODE_PROPERTY_NAME   	"CONNECTION_MODE"
#define CONNECTION_UDP_ITEM_NAME   				"UDP"
#define CONNECTION_TCP_ITEM_NAME					"TCP"
#define CONNECTION_SERIAL_ITEM_NAME				"SERIAL"
#define CONNECTION_SERIAL_DTR_ITEM_NAME		"SERIAL_DTR"

#define MOUNT_TYPE_PROPERTY     					(PRIVATE_DATA->mount_type_property)
#define MOUNT_TYPE_G11  									(MOUNT_TYPE_PROPERTY->items+0)
#define MOUNT_TYPE_TITAN  								(MOUNT_TYPE_PROPERTY->items+1)
#define MOUNT_TYPE_EXOS2  								(MOUNT_TYPE_PROPERTY->items+2)
#define MOUNT_TYPE_IEXOS100  							(MOUNT_TYPE_PROPERTY->items+3)

#define MOUNT_TYPE_PROPERTY_NAME					"MOUNT_TYPE"
#define MOUNT_TYPE_G11_NAME								"G11"
#define MOUNT_TYPE_TITAN_NAME							"TITAN"
#define MOUNT_TYPE_EXOS2_NAME  						"EXOS-2"
#define MOUNT_TYPE_IEXOS100_NAME  				"iEXOS-100"

typedef struct {
	char *name;
	uint32_t count[2];
} mount_type_data;

typedef enum {
	PMC8_G11 = 0,
	PMC8_TITAN,
	PMC8_EXOS2,
	PMC8_IEXOS100
} model_type;

static mount_type_data MODELS[] = {
	{ "Losmandy G-11", { 4608000, 4608000 } },
	{ "Losmandy Titan", { 3456000, 3456000 } },
	{ "Explore Scientific EXOS II", { 4147200, 4147200 } },
	{ "Explore Scientific iEXOS-100", { 4147200, 4147200 } },
};

typedef struct {
	int handle;
	model_type type;
	int rate[3];
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec;
	indigo_property *connection_mode_property;
	indigo_property *mount_type_property;
	pthread_mutex_t port_mutex;
	indigo_network_protocol proto;
	bool park;
	bool is_udp, is_tcp, is_serial;
	int count_open;
	int version;
} pmc8_private_data;

static bool pmc8_command(indigo_device *device, char *command, char *response, int max, int sleep);
static void pmc8_close(indigo_device *device);
static void mount_equatorial_coordinates_handler(indigo_device *device);

static bool pmc8_open(indigo_device *device) {
	char response[128];
	char *name = DEVICE_PORT_ITEM->text.value;
	if (PRIVATE_DATA->count_open++ == 0) {
		if (!indigo_is_device_url(name, "pmc8")) {
			PRIVATE_DATA->proto = -1;
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
			if (CONNECTION_SERIAL_DTR_ITEM->sw.value) {
				int mode = TIOCM_DTR;
				ioctl(PRIVATE_DATA->handle, TIOCMBIC, &mode);
			}
		} else {
			PRIVATE_DATA->proto = INDIGO_PROTOCOL_UDP;
			PRIVATE_DATA->handle = indigo_open_network_device(name, 54372, &PRIVATE_DATA->proto);
		}
	}
	if (PRIVATE_DATA->handle > 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		indigo_device *d = device, *device = d->master_device;
		for (int retry = 0; retry < 3; retry++) {
			if (pmc8_command(device, "ESGv!", response, sizeof(response), 0) && !strncmp(response, "ESGv", 4)) {
				strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response + 6);
				PRIVATE_DATA->version = atoi(response + 6);
				if (PRIVATE_DATA->version < 20) {
					if (strstr(response, "G11")) {
						indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_G11, true);
						indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
					} else if (strstr(response, "TITAN")) {
						indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_TITAN, true);
						indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
					} else if (strstr(response, "EXOS2")) {
						indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_EXOS2, true);
						indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
					} else if (strstr(response, "ES1A")) {
						indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_IEXOS100, true);
						indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
					}
				} else {
					if (pmc8_command(device, "ESGi!", response, sizeof(response), 0) && !strncmp(response, "ESGi", 4)) {
						int type = 10 * (response[20] - '0') + response[21] - '0';
						if (type >= 4 && type <= 7) {
							indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_G11, true);
							indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
						} else if (type >= 8 && type <= 11) {
							indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_EXOS2, true);
							indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
						} else {
							// TBD there is iExos200/300 mentioned with no clear data
							indigo_set_switch(MOUNT_TYPE_PROPERTY, MOUNT_TYPE_IEXOS100, true);
							indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
						}
					}
				}
				if (MOUNT_TYPE_G11->sw.value) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, MODELS[0].name);
					PRIVATE_DATA->type = PMC8_G11;
				} else if (MOUNT_TYPE_TITAN->sw.value) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, MODELS[1].name);
					PRIVATE_DATA->type = PMC8_EXOS2;
				} else if (MOUNT_TYPE_EXOS2->sw.value) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, MODELS[2].name);
					PRIVATE_DATA->type = PMC8_EXOS2;
				} else if (MOUNT_TYPE_IEXOS100->sw.value) {
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, MODELS[3].name);
					PRIVATE_DATA->type = PMC8_IEXOS100;
				}
				double sec_per_count = 1296000.0/MODELS[PRIVATE_DATA->type].count[0];
				PRIVATE_DATA->rate[0] = round(15.0 / sec_per_count * 25);
				PRIVATE_DATA->rate[1] = round(14.685 / sec_per_count * 25);
				PRIVATE_DATA->rate[2] = round(15.041 / sec_per_count * 25);
				return true;
			} else {
				indigo_send_message(device, "Retrying connection in 10 seconds...");
				indigo_usleep(10 * ONE_SECOND_DELAY);
			}
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to initialize to %s", name);
		pmc8_close(device);
		return false;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s (%s)", name, strerror(errno));
		PRIVATE_DATA->count_open--;
		return false;
	}
}

static bool pmc8_command(indigo_device *device, char *command, char *response, int max, int sleep) {
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
		if (result < 0) {
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
			tv.tv_usec = 500000;
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
				int bytes_read = 0;
				while (bytes_read < max) {
					result = read(PRIVATE_DATA->handle, &c, 1);
					if (result < 1) {
						break;
					}
					response[bytes_read++] = c;
					if (c == '!' || c == '%' || c == '#')
						break;
				}
				response[bytes_read] = 0;
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

static void pmc8_close(indigo_device *device) {
	if (--PRIVATE_DATA->count_open == 0) {
		if (PRIVATE_DATA->handle > 0) {
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
		}
	}
}

static bool pmc8_get_tracking_rate(indigo_device *device) {
	char response[32];
	if (pmc8_command(device, "ESGx!", response, sizeof(response), 0)) {
		int rate = (int)strtol(response + 4, NULL, 16);
		if (rate == 0) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		} else {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			if (rate == PRIVATE_DATA->rate[0]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
			} else if (rate == PRIVATE_DATA->rate[1]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
			} else if (rate == PRIVATE_DATA->rate[2]) {
				indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
			}
		}
		return true;
	}
	return false;
}

static bool pmc8_set_tracking_rate(indigo_device *device, int offset) {
	char command[32], response[32];
	int rate = 0;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[0];
		} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[1];
		} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			rate = PRIVATE_DATA->rate[2];
		}
	}
	if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
		if (!pmc8_command(device, "ESSd01!", response, sizeof(response), 0)) {
			return false;
		}
	} else {
		if (!pmc8_command(device, "ESSd00!", response, sizeof(response), 0)) {
			return false;
		}
	}
	sprintf(command, "ESTr%04X!", rate + offset);
	if (pmc8_command(device, command, response, sizeof(response), 0)) {
		return true;
	}
	return false;
}

static bool pmc8_point(indigo_device *device, int32_t ha, int32_t dec) {
	char command[32], response[32];
	sprintf(command, MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value ? "ESSp0%06X!" : "ESPt0%06X!", ha & 0xFFFFFF);
	if (!pmc8_command(device, command, response, sizeof(response), 0)) {
		return false;
	}
	sprintf(command, MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value ? "ESSp1%06X!" :"ESPt1%06X!", dec & 0xFFFFFF);
	if (!pmc8_command(device, command, response, sizeof(response), 0)) {
		return false;
	}
	return true;
}

static bool pmc8_get_position(indigo_device *device, int32_t *ha, int32_t *dec) {
	char response[32];
	int32_t raw_ha = 0, raw_dec = 0;
	if (pmc8_command(device, "ESGp0!", response, sizeof(response), 0)) {
		raw_ha = (int)strtol(response + 5, NULL, 16);
		if (raw_ha & 0x800000)
			raw_ha |= 0xFF000000;
		if (pmc8_command(device, "ESGp1!", response, sizeof(response), 0)) {
			raw_dec = (int)strtol(response + 5, NULL, 16);
			if (raw_dec & 0x800000)
				raw_dec |= 0xFF000000;
			*ha = raw_ha;
			*dec = raw_dec;
			return true;
		}
	}
	return false;
}

static bool pmc8_move(indigo_device *device, int axis, int direction, int rate) {
	char command[32], response[32];
	if (rate == 0) {
		if (axis == 0) {
			return pmc8_set_tracking_rate(device, 0);
		} else {
			return pmc8_command(device, "ESSr10000!", response, sizeof(response), 0);
		}
	} else {
		sprintf(command, "ESSd%d%d!", axis, direction);
		if (!pmc8_command(device, command, response, sizeof(response), 0)) {
			return false;
		}
		sprintf(command, "ESSr%d%04X!", axis, abs(rate));
		if (!pmc8_command(device, command, response, sizeof(response), 0)) {
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
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		// -------------------------------------------------------------------------------- MOUNT_INFO
		strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Explore Scientific");
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- CONNECTION_MODE
		CONNECTION_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CONNECTION_MODE_PROPERTY_NAME, MAIN_GROUP, "Connnection mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (CONNECTION_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(CONNECTION_UDP_ITEM, CONNECTION_UDP_ITEM_NAME, "UDP", true);
		indigo_init_switch_item(CONNECTION_TCP_ITEM, CONNECTION_TCP_ITEM_NAME, "TCP", false);
		indigo_init_switch_item(CONNECTION_SERIAL_ITEM, CONNECTION_SERIAL_ITEM_NAME, "Serial", false);
		indigo_init_switch_item(CONNECTION_SERIAL_DTR_ITEM, CONNECTION_SERIAL_DTR_ITEM_NAME, "Serial (clear DTR)", false);
		// -------------------------------------------------------------------------------- MOUNT_TYPE
		MOUNT_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TYPE_PROPERTY_NAME, MAIN_GROUP, "Mount type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (CONNECTION_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_TYPE_G11, MOUNT_TYPE_G11_NAME, MODELS[0].name, false);
		indigo_init_switch_item(MOUNT_TYPE_TITAN, MOUNT_TYPE_TITAN_NAME, MODELS[1].name, false);
		indigo_init_switch_item(MOUNT_TYPE_EXOS2, MOUNT_TYPE_EXOS2_NAME, MODELS[2].name, false);
		indigo_init_switch_item(MOUNT_TYPE_IEXOS100, MOUNT_TYPE_IEXOS100_NAME, MODELS[3].name, false);
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(CONNECTION_MODE_PROPERTY, property))
		indigo_define_property(device, CONNECTION_MODE_PROPERTY, NULL);
	if (indigo_property_match(MOUNT_TYPE_PROPERTY, property))
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		int32_t raw_ha = 0, raw_dec = 0;
		if (pmc8_get_position(device, &raw_ha, &raw_dec)) {
			if (abs(raw_ha) < 0xFFF && abs(raw_dec) < 0xFFF && MOUNT_TRACKING_OFF_ITEM->sw.value && PRIVATE_DATA->park) {
				PRIVATE_DATA->park = false;
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
			}
			indigo_item *side_of_pier;
			uint32_t ra_count = MODELS[PRIVATE_DATA->type].count[0];
			uint32_t dec_count = MODELS[PRIVATE_DATA->type].count[1];
			double ha_angle = ((double)raw_ha / ra_count) * 24;
			double dec_angle = ((double)raw_dec / dec_count) * 360;
			double ha;
			if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
				if (raw_dec >= 0) {
					MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = 90 - dec_angle;
					ha = ha_angle - 6;
					side_of_pier = MOUNT_SIDE_OF_PIER_WEST_ITEM;
				} else {
					MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = 90 + dec_angle;
					ha = ha_angle + 6;
					side_of_pier = MOUNT_SIDE_OF_PIER_EAST_ITEM;
				}
			} else {
				if (raw_dec >= 0) {
					MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = -90 + dec_angle;
					ha = -(ha_angle - 6);
					side_of_pier = MOUNT_SIDE_OF_PIER_EAST_ITEM;
				} else {
					MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = -90 - dec_angle;
					ha = -(ha_angle + 6);
					side_of_pier = MOUNT_SIDE_OF_PIER_WEST_ITEM;
				}
			}
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - ha;
			if (MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value < 0)
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value += 24;
			else if (MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value > 24)
				MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value -= 24;
			if (!side_of_pier->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, side_of_pier, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
			indigo_update_coordinates(device, NULL);
			indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		}
		indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
	}
}

static void mount_connect_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = pmc8_open(device);
		if (result) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			if (pmc8_get_tracking_rate(device)) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
			MOUNT_TYPE_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		PRIVATE_DATA->position_timer = NULL;
		pmc8_close(device);
		indigo_delete_property(device, MOUNT_TYPE_PROPERTY, NULL);
		MOUNT_TYPE_PROPERTY->perm = INDIGO_RW_PERM;
		indigo_define_property(device, MOUNT_TYPE_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	for (int i = 0; i < 3 && MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE; i++) {
		double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha_angle = lst - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		double dec_angle = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		if (ha_angle < -12) {
			ha_angle += 24;
		} else if (ha_angle >= 12) {
			ha_angle -= 24;
		}
		if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0) {
			if (ha_angle < 0) {
				ha_angle = ha_angle + 6;
				dec_angle = -(dec_angle - 90);
			} else {
				ha_angle = ha_angle - 6;
				dec_angle = dec_angle - 90;
			}
		} else {
			if (ha_angle < 0) {
				ha_angle = -(ha_angle + 6);
				dec_angle = -(dec_angle + 90);
			} else {
				ha_angle = -(ha_angle - 6);
				dec_angle = dec_angle + 90;
			}
		}
		uint32_t ra_count = MODELS[PRIVATE_DATA->type].count[0];
		uint32_t dec_count = MODELS[PRIVATE_DATA->type].count[1];
		int32_t raw_dec = (dec_angle / 360.0) * dec_count;
		int32_t raw_ha = (ha_angle / 24.0) * ra_count;
		if (!pmc8_point(device, raw_ha, raw_dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value)
			break;
		raw_ha &= ~0xFFF;
		raw_dec &= ~0xFFF;
		while (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			int32_t current_raw_ha = 0, current_raw_dec = 0;
			if (pmc8_get_position(device, &current_raw_ha, &current_raw_dec)) {
				current_raw_ha &= ~0xFFF;
				current_raw_dec &= ~0xFFF;
				if (current_raw_ha == raw_ha && current_raw_dec == raw_dec)
					break;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_usleep(100000);
		}
		indigo_usleep(500000);
	}
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		if (pmc8_set_tracking_rate(device, 0)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (pmc8_set_tracking_rate(device, 0)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	if (pmc8_set_tracking_rate(device, 0)) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	mount_tracking_handler(device);
	PRIVATE_DATA->park = true;
	if (!pmc8_point(device, 0, 0)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	}
}

static void mount_abort_motion_handler(indigo_device *device) {
	if (pmc8_move(device, 0, 0, 0) && pmc8_move(device, 1, 0, 0)) {
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

static void mount_motion_handler(indigo_device *device) {
	int rate = 0, direction = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		rate = PRIVATE_DATA->rate[0];
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
		if (pmc8_move(device, 1, direction, rate))
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	} else if (MOUNT_MOTION_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (pmc8_move(device, 0, direction, rate))
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	}
}

static void mount_switch_connection_handler(indigo_device *device) {
	CONNECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
    char response[32];
    if (PRIVATE_DATA->is_udp && CONNECTION_TCP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESY!", response, sizeof(response), 0) || strcmp(response, "ESY0")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			mount_connect_handler(device);
    } else if (PRIVATE_DATA->is_udp && (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value)) {
			indigo_send_message(device, "Can't switch from UDP to SERIAL directly, switch to TCP first!");
			indigo_set_switch(CONNECTION_MODE_PROPERTY, CONNECTION_UDP_ITEM, true);
			CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
    } else if (PRIVATE_DATA->is_tcp && CONNECTION_UDP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESY!", response, sizeof(response), 0) || strcmp(response, "ESY1")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			mount_connect_handler(device);
    } else if (PRIVATE_DATA->is_tcp && (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value)) {
			if (!pmc8_command(device, "ESX!", response, sizeof(response), 0) || strcmp(response, "ESX0")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			mount_connect_handler(device);
    } else if (PRIVATE_DATA->is_serial && CONNECTION_UDP_ITEM->sw.value) {
			indigo_send_message(device, "Can't switch from SERIAL to UDP directly, switch to TCP first!");
			indigo_set_switch(CONNECTION_MODE_PROPERTY, CONNECTION_SERIAL_ITEM, true);
			CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
    } else if (PRIVATE_DATA->is_serial && CONNECTION_TCP_ITEM->sw.value) {
			if (!pmc8_command(device, "ESX!", response, sizeof(response), 0) || strcmp(response, "ESX1")) {
				CONNECTION_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			mount_connect_handler(device);
    }
	if (CONNECTION_MODE_PROPERTY->state == INDIGO_OK_STATE) {
		if (CONNECTION_UDP_ITEM->sw.value) {
			strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
		} else if (CONNECTION_TCP_ITEM->sw.value) {
			strcpy(DEVICE_PORT_ITEM->text.value, "tcp://192.168.47.1");
		} else {
			if (DEVICE_PORTS_PROPERTY->count > 1)
				strcpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
			else
				strcpy(DEVICE_PORT_ITEM->text.value, "");
		}
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
	}
	indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
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
		indigo_set_timer(device, 0, mount_connect_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		if (IS_CONNECTED) {
			bool parked = MOUNT_PARK_PARKED_ITEM->sw.value;
			indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
			if (!parked && MOUNT_PARK_PARKED_ITEM->sw.value) {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
				indigo_set_timer(device, 0, mount_park_handler, NULL);
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
		} else if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
			double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_equatorial_coordinates_handler, NULL);
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
			indigo_set_timer(device, 0, mount_abort_motion_handler, NULL);
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
			indigo_set_timer(device, 0, mount_motion_handler, NULL);
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
			indigo_set_timer(device, 0, mount_motion_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_tracking_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_track_rate_handler, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONNECTION_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION_MODE
		PRIVATE_DATA->is_udp = CONNECTION_UDP_ITEM->sw.value;
		PRIVATE_DATA->is_tcp = CONNECTION_TCP_ITEM->sw.value;
		PRIVATE_DATA->is_serial = (CONNECTION_SERIAL_ITEM->sw.value || CONNECTION_SERIAL_DTR_ITEM->sw.value);
		indigo_property_copy_values(CONNECTION_MODE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CONNECTION_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_switch_connection_handler, NULL);
		} else {
			if (CONNECTION_UDP_ITEM->sw.value) {
				strcpy(DEVICE_PORT_ITEM->text.value, "udp://192.168.47.1");
			} else if (CONNECTION_TCP_ITEM->sw.value) {
				strcpy(DEVICE_PORT_ITEM->text.value, "tcp://192.168.47.1");
			} else {
				if (DEVICE_PORTS_PROPERTY->count > 1)
					strcpy(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
				else
					strcpy(DEVICE_PORT_ITEM->text.value, "");
			}
			DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DEVICE_PORT_PROPERTY, NULL);
			CONNECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CONNECTION_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TYPE_PROPERTY, property)) {
		indigo_property_copy_values(MOUNT_TYPE_PROPERTY, property, false);
		MOUNT_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CONNECTION_MODE_PROPERTY);
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
		mount_connect_handler(device);
	}
	indigo_release_property(CONNECTION_MODE_PROPERTY);
	indigo_release_property(MOUNT_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// --------------------------------------------------------------------------------

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- GUIDER_RATE
		GUIDER_RATE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = pmc8_open(device);
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		pmc8_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_timer_ra_handler(indigo_device *device) {
	int rate = PRIVATE_DATA->rate[0] * (GUIDER_RATE_ITEM->number.value / 100.0);
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		pmc8_set_tracking_rate(device->master_device, -rate);
		indigo_usleep(1000 * GUIDER_GUIDE_EAST_ITEM->number.value);
		pmc8_set_tracking_rate(device->master_device, 0);
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		pmc8_set_tracking_rate(device->master_device, rate);
		indigo_usleep(1000 * GUIDER_GUIDE_WEST_ITEM->number.value);
		pmc8_set_tracking_rate(device->master_device, 0);
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_timer_dec_handler(indigo_device *device) {
	int rate = PRIVATE_DATA->rate[0] * (GUIDER_RATE_ITEM->number.value / 2500.0);
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		pmc8_move(device, 1, 1, rate);
		indigo_usleep(1000 * GUIDER_GUIDE_NORTH_ITEM->number.value);
		pmc8_move(device, 1, 1, 0);
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		pmc8_move(device, 1, 0, rate);
		indigo_usleep(1000 * GUIDER_GUIDE_SOUTH_ITEM->number.value);
		pmc8_move(device, 1, 0, 0);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer_dec == NULL) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_timer_dec_handler, &PRIVATE_DATA->guider_timer_dec);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer_ra == NULL) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_timer_ra_handler, &PRIVATE_DATA->guider_timer_ra);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COMMAND_GUIDE_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);

		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static pmc8_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_pmc8(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_PMC8_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);

	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_PMC8_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "PMC Eight Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(pmc8_private_data));
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
