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

/** INDIGO Celestron NexStar AUX driver
 \file indigo_mount_nexstaraux.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME	"indigo_mount_nexstaraux"

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
#include <netinet/in.h>
#include <arpa/inet.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_novas.h>

#include "indigo_mount_nexstaraux.h"

#define PRIVATE_DATA        					((nexstaraux_private_data *)device->private_data)

typedef enum {
	MC_GET_POSITION = 0x01,
	MC_GOTO_FAST = 0x02,
	MC_SET_POSITION = 0x04,
	MC_SET_POS_GUIDERATE = 0x06,
	MC_SET_NEG_GUIDERATE = 0x07,
	MC_LEVEL_START = 0x0b,
	MC_PEC_RECORD_START = 0x0c,
	MC_PEC_PLAYBACK = 0x0d,
	MC_SET_POS_BACKLASH = 0x10,
	MC_SET_NEG_BACKLASH = 0x11,
	MC_LEVEL_DONE = 0x12,
	MC_SLEW_DONE = 0x13,
	MC_PEC_RECORD_DONE = 0x15,
	MC_PEC_RECORD_STOP = 0x16,
	MC_GOTO_SLOW = 0x17,
	MC_AT_INDEX = 0x18,
	MC_SEEK_INDEX = 0x19,
	MC_MOVE_POS = 0x24,
	MC_MOVE_NEG = 0x25,
	MC_ENABLE_CORDWRAP = 0x38,
	MC_DISABLE_CORDWRAP = 0x39,
	MC_SET_CORDWRAP_POS = 0x3a,
	MC_POLL_CORDWRAP = 0x3b,
	MC_GET_CORDWRAP_POS = 0x3c,
	MC_GET_POS_BACKLASH = 0x40,
	MC_GET_NEG_BACKLASH = 0x41,
	MC_SET_AUTOGUIDE_RATE = 0x46,
	MC_GET_AUTOGUIDE_RATE = 0x47,
	MC_GET_APPROACH = 0xfc,
	MC_SET_APPROACH = 0xfd,
	MC_GET_VER = 0xfe,
	GPS_GET_LAT = 0x01,
	GPS_GET_LONG = 0x02,
	GPS_GET_DATE = 0x03,
	GPS_GET_YEAR = 0x04,
	GPS_GET_SAT_INFO = 0x07,
	GPS_GET_RCVR_STATUS = 0x08,
	GPS_GET_TIME = 0x33,
	GPS_TIME_VALID = 0x36,
	GPS_LINKED = 0x37,
	GPS_GET_HW_VER = 0x38,
	GPS_GET_COMPASS = 0xa0,
	GPS_GET_VER = 0xfe
} commands;

typedef enum {
	ANY = 0x00,
	MAIN = 0x01,
	HC = 0x04,
	AZM = 0x10,
	ALT = 0x11,
	APP = 0x20,
	GPS = 0xb0
} targets;

typedef struct {
	int handle;
	int count_open;
	pthread_mutex_t port_mutex;
	indigo_timer *position_timer, *guider_timer_dec, *guider_timer_ra;
} nexstaraux_private_data;

static void nexstaraux_dump(indigo_device *device, char *dir, unsigned char *buffer) {
	switch (buffer[1]) {
		case 3:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s [%02x %02x] %02x > %02x %02x [%02x]", PRIVATE_DATA->handle, dir, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
			return;
		case 4:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s [%02x %02x] %02x > %02x %02x %02x [%02x]", PRIVATE_DATA->handle, dir, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			return;
		case 5:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s [%02x %02x] %02x > %02x %02x %02x %02x [%02x] = %d", PRIVATE_DATA->handle, dir, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[5] << 8 | buffer[6]);
			return;
		case 6:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s [%02x %02x] %02x > %02x %02x %02x %02x %02x [%02x] = %d", PRIVATE_DATA->handle, dir, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[5] << 16 | buffer[6] << 8 | buffer[7]);
			return;
		default:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s [%02x %02x] %02x > %02x %02x %02x %02x %02x %02x...", PRIVATE_DATA->handle, dir, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8]);
			return;
	}
}

static bool nexstaraux_command(indigo_device *device, targets src, targets dst, commands cmd, unsigned char *data, int length, unsigned char *reply) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	unsigned char buffer[16] = { 0 };
	buffer[0] = 0x3b;
	buffer[1] = (length += 3);
	buffer[2] = src;
	buffer[3] = dst;
	buffer[4] = cmd;
	if (data)
		memcpy(buffer + 5, data, length);
	int checksum = 0;
	for (int i = 1; i < length + 2; i++)
		checksum += buffer[i];
	buffer[length + 2] = (unsigned char)(((~checksum) + 1) & 0xFF);
	nexstaraux_dump(device, "<-", buffer);
	if (indigo_write(PRIVATE_DATA->handle, (char *)buffer, length + 3)) {
		while (true) {
			for (int i = 0; i < 10; i++) {
				if (read(PRIVATE_DATA->handle, reply, 1) == 1) {
					if (*reply == 0x3b)
						break;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> Failed", PRIVATE_DATA->handle);
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					return false;
				}
			}
			if (read(PRIVATE_DATA->handle, reply + 1, 1) == 1) {
				if (indigo_read(PRIVATE_DATA->handle, (char *)(reply + 2), reply[1] + 1)) {
					if (buffer[4] != reply[4] || buffer[2] != reply[3] || buffer[3] != reply[2]) {
						nexstaraux_dump(device, ">>", reply);
						continue;
					}
					nexstaraux_dump(device, "->", reply);
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					return true;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> Failed", PRIVATE_DATA->handle);
					pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
					return false;
				}
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- Failed", PRIVATE_DATA->handle);
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return false;
}

static bool nexstaraux_command_16(indigo_device *device, targets src, targets dst, commands cmd, uint16_t value, unsigned char *reply) {
	unsigned char data[3];
	data[0] = (value >> 8) & 0xFF;
	data[1] = value & 0xFF;
	return nexstaraux_command(device, src, dst, cmd, data, 2, reply);
}

static bool nexstaraux_command_24(indigo_device *device, targets src, targets dst, commands cmd, uint32_t value, unsigned char *reply) {
	unsigned char data[3];
	data[0] = (value >> 16) & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = value & 0xFF;
	return nexstaraux_command(device, src, dst, cmd, data, 3, reply);
}

static bool nexstaraux_open(indigo_device *d) {
	indigo_device *device = d->master_device;
	char *name = DEVICE_PORT_ITEM->text.value;
	if (PRIVATE_DATA->count_open++ == 0) {
		if (strncmp(name, "nexstar://", 10) == 0) {
			char *host = name + 10;
			indigo_network_protocol protocol_hint = INDIGO_PROTOCOL_TCP;
			if (*host == 0) {
				int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
				if (udp_socket < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create socket");
				} else {
					struct timeval tv;
					struct sockaddr_in local_addr;
					tv.tv_sec  = 3;
					tv.tv_usec = 0;
					setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
					memset((char *)&local_addr, 0, sizeof(local_addr));
					local_addr.sin_family = AF_INET;
					local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
					local_addr.sin_port = htons(55555);
					if (bind(udp_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to bind socket");
					} else {
						struct sockaddr_in remote_addr;
						socklen_t addrlen = sizeof(remote_addr);
						unsigned char message[2048];
						for (int n = 0; n < 5; n++) {
							long recvlen = recvfrom(udp_socket, message, sizeof(message), 0, (struct sockaddr *)&remote_addr, &addrlen);
							if (recvlen > 0) {
								sprintf(name, "nexstar://%s:%d", inet_ntoa(remote_addr.sin_addr), 2000 /*ntohs(remote_addr.sin_port)*/);
								DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
								indigo_update_property(device, DEVICE_PORT_PROPERTY, "Mount detected at %s", name);
								PRIVATE_DATA->handle = indigo_open_network_device(name, 2000, &protocol_hint);
								break;
							}
						}
						close(udp_socket);
					}
				}
			} else {
				PRIVATE_DATA->handle = indigo_open_network_device(name, 2000, &protocol_hint);
			}
		}
	}
	if (PRIVATE_DATA->handle > 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		unsigned char alt_version[16] = { 0 }, azm_version[16] = { 0 };
		if (nexstaraux_command(device, APP, ALT, MC_GET_VER, NULL, 0, alt_version) && nexstaraux_command(device, APP, AZM, MC_GET_VER, NULL, 0, azm_version)) {
			sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value,"%d.%d / %d.%d", alt_version[5], alt_version[6], azm_version[5], azm_version[6]);
			strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron");
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "NexStar AUX");
			strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			return true;
		} else {
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed", name);
			PRIVATE_DATA->count_open--;
			return false;
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		PRIVATE_DATA->count_open--;
		return false;
	}
}

static void nexstaraux_close(indigo_device *device) {
	if (--PRIVATE_DATA->count_open == 0) {
		if (PRIVATE_DATA->handle > 0) {
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
		}
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- SIMULATION
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		strcpy(DEVICE_PORT_ITEM->text.value, "nexstar://");
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		DEVICE_PORTS_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		MOUNT_GUIDE_RATE_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
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
	unsigned char reply[16] = { 0 };
	int raw_alt = -1, raw_azm = -1;
	if (nexstaraux_command(device, APP, ALT, MC_GET_POSITION, NULL, 0, reply)) {
		raw_alt = reply[5] << 16 | reply[6] << 8 | reply[7];
	}
	if (nexstaraux_command(device, APP, AZM, MC_GET_POSITION, NULL, 0, reply)) {
		raw_azm = reply[5] << 16 | reply[6] << 8 | reply[7];
	}
	if (raw_alt != -1 && raw_azm != -1) {
		double ha_angle = fmod(((double)raw_azm / 0x1000000) * 24 + 12, 24);
		double dec_angle = fmod(((double)raw_alt / 0x1000000) * 360, 360);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - ha_angle + 24, 24);
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec_angle;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, &PRIVATE_DATA->position_timer);
}

static void mount_connect_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = nexstaraux_open(device);
		if (result) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			if (nexstaraux_command_24(device, APP, AZM, MC_SET_POS_GUIDERATE, 0, reply) && nexstaraux_command_24(device, APP, ALT, MC_SET_POS_GUIDERATE, 0, reply)) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			} else {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (nexstaraux_command(device, APP, AZM, MC_GET_AUTOGUIDE_RATE, NULL, 0, reply)) {
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = reply[5] * 100.0 / 256;
				if (nexstaraux_command(device, APP, ALT, MC_GET_AUTOGUIDE_RATE, NULL, 0, reply)) {
					MOUNT_GUIDE_RATE_DEC_ITEM->number.value = reply[5] * 100.0 / 256;
					MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
			indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
		PRIVATE_DATA->position_timer = NULL;
		nexstaraux_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_tracking_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	uint_fast16_t rate;
	if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
		rate = 0xfffd;
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
		rate = 0xfffe;
	} else {
		rate = 0xFFFF;
	}
	commands set_guide_rate = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0 ? MC_SET_POS_GUIDERATE : MC_SET_NEG_GUIDERATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (nexstaraux_command_16(device, APP, AZM, set_guide_rate, rate, reply)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (nexstaraux_command_24(device, APP, AZM, set_guide_rate, 0, reply)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	double ha_angle = fmod(lst - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target + 24, 24);
	double dec_angle = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	int32_t raw_azm = (int32_t)((fmod(ha_angle + 12, 24) / 24.0) * 0x1000000) % 0x1000000;
	int32_t raw_alt = (int32_t)((dec_angle / 360.0) * 0x1000000) % 0x1000000;
	if (!nexstaraux_command_24(device, APP, AZM, MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value ? MC_SET_POSITION : MC_GOTO_FAST, raw_azm, reply) || !nexstaraux_command_24(device, APP, ALT, MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value ? MC_SET_POSITION : MC_GOTO_FAST, raw_alt, reply)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		while (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_usleep(1000000);
			if (nexstaraux_command(device, APP, AZM, MC_SLEW_DONE, NULL, 0, reply)) {
				if (reply[5] == 0x00)
					continue;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			if (nexstaraux_command(device, APP, ALT, MC_SLEW_DONE, NULL, 0, reply)) {
				if (reply[5] == 0x00)
					continue;
			} else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			break;
		}
		if (!nexstaraux_command_24(device, APP, AZM, MC_GOTO_SLOW, raw_azm, reply) || !nexstaraux_command_24(device, APP, ALT, MC_GOTO_SLOW, raw_alt, reply)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			while (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_usleep(1000000);
				if (nexstaraux_command(device, APP, AZM, MC_SLEW_DONE, NULL, 0, reply)) {
					if (reply[5] == 0x00)
						continue;
				} else {
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				if (nexstaraux_command(device, APP, ALT, MC_SLEW_DONE, NULL, 0, reply)) {
					if (reply[5] == 0x00)
						continue;
				} else {
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				break;
			}
		}
		if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			mount_tracking_handler(device);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	uint_fast16_t rate;
	if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
		rate = 0xfffd;
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
		rate = 0xfffe;
	} else {
		rate = 0xFFFF;
	}
	commands set_guide_rate = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0 ? MC_SET_POS_GUIDERATE : MC_SET_NEG_GUIDERATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (nexstaraux_command_16(device, APP, AZM, set_guide_rate, rate, reply)) {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	mount_tracking_handler(device);
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	if (!nexstaraux_command_24(device, APP, AZM, MC_GOTO_FAST, 0x800000, reply) || !nexstaraux_command_24(device, APP, ALT, MC_GOTO_FAST, 0x000000, reply)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	while (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_usleep(1000000);
		if (nexstaraux_command(device, APP, AZM, MC_SLEW_DONE, NULL, 0, reply)) {
			if (reply[5] == 0x00)
				continue;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
		if (nexstaraux_command(device, APP, ALT, MC_SLEW_DONE, NULL, 0, reply)) {
			if (reply[5] == 0x00)
				continue;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
		break;
	}
	indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
}

static void mount_abort_motion_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	if (nexstaraux_command_24(device, APP, AZM, MC_MOVE_POS, 0, reply) && nexstaraux_command_24(device, APP, ALT, MC_MOVE_POS, 0, reply)) {
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
	unsigned char reply[16] = { 0 };
	unsigned char rate = 0;
	commands direction = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		rate = 1;
	else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		rate = 2;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		rate = 5;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		rate = 9;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
		direction = MC_MOVE_POS;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value) {
		direction = MC_MOVE_NEG;
	} else {
		direction = MC_MOVE_POS;
		rate = 0;
	}
	if (MOUNT_MOTION_DEC_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (nexstaraux_command(device, APP, ALT, direction, &rate, 1, reply))
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	} else if (MOUNT_MOTION_RA_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (nexstaraux_command(device, APP, AZM, direction, &rate, 1, reply))
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	}
}

static void mount_guide_rate_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	unsigned char rate = MOUNT_GUIDE_RATE_RA_ITEM->number.target / 100.0 * 256;
	if (nexstaraux_command(device, APP, AZM, MC_SET_AUTOGUIDE_RATE, &rate, 1, reply)) {
		rate = MOUNT_GUIDE_RATE_DEC_ITEM->number.target / 100.0 * 256;
		if (nexstaraux_command(device, APP, ALT, MC_SET_AUTOGUIDE_RATE, &rate, 1, reply)) {
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
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
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_guide_rate_handler, NULL);
		}
		return INDIGO_OK;
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// --------------------------------------------------------------------------------

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = nexstaraux_open(device);
		if (result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
		nexstaraux_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_timer_ra_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	unsigned char rate = 1;
	unsigned duration = 0;
	commands direction = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		direction = MC_MOVE_POS;
		duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		direction = MC_MOVE_NEG;
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
	}
	if (nexstaraux_command(device, APP, AZM, direction, &rate, 1, reply)) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_usleep(duration * 1000);
		rate = 0;
		if (nexstaraux_command(device, APP, AZM, direction, &rate, 1, reply)) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_timer_dec_handler(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	unsigned char rate = 1;
	unsigned duration = 0;
	commands direction = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		direction = MC_MOVE_POS;
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		direction = MC_MOVE_NEG;
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
	}
	if (nexstaraux_command(device, APP, ALT, direction, &rate, 1, reply)) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_usleep(duration * 1000);
		rate = 0;
		if (nexstaraux_command(device, APP, ALT, direction, &rate, 1, reply)) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
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

static nexstaraux_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_nexstaraux(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_NEXSTARAUX_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);

	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_NEXSTARAUX_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "NexStar AUX Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(nexstaraux_private_data));
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
