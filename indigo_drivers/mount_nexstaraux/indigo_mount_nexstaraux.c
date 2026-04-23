// Copyright (c) 2020-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_mount_nexstaraux.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_nexstaraux.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000007
#define DRIVER_NAME          "indigo_mount_nexstaraux"
#define DRIVER_LABEL         "NexStar AUX Mount"
#define MOUNT_DEVICE_NAME    "Mount Nexstar AUX"
#define GUIDER_DEVICE_NAME   "Mount Nexstar AUX (guider)"
#define PRIVATE_DATA         ((nexstaraux_private_data *)device->private_data)

//+ define

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

//- define

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	//+ data
	bool stopping, slewing, centering, parking, parked;
	//- data
} nexstaraux_private_data;

#pragma mark - Low level code

//+ code

static bool nexstaraux_validate_handle(indigo_device *device);

static bool nexstaraux_command(indigo_device *device, targets src, targets dst, commands cmd, unsigned char *data, int length, unsigned char *reply) {
	if (!nexstaraux_validate_handle(device)) {
		return false;
	}
	unsigned char buffer[16] = { 0 };
	buffer[0] = 0x3b;
	buffer[1] = (length + 3);
	buffer[2] = src;
	buffer[3] = dst;
	buffer[4] = cmd;
	if (data) {
		memcpy(buffer + 5, data, length);
	}
	int checksum = 0;
	length += 3;
	for (int i = 1; i < length + 2; i++) {
		checksum += buffer[i];
	}
	buffer[length + 2] = (unsigned char)(((~checksum) + 1) & 0xFF);
	if (indigo_uni_write(PRIVATE_DATA->handle, (char *)buffer, length + 3) > 0) {
		while (true) {
			for (int i = 0; i < 10; i++) {
				if (indigo_uni_read(PRIVATE_DATA->handle, reply, 1) == 1) {
					if (*reply == 0x3b) {
						break;
					}
				} else {
					return false;
				}
			}
			if (*reply != 0x3b) {
				return false;
			}
			if (indigo_uni_read(PRIVATE_DATA->handle, reply + 1, 1) == 1) {
				if (indigo_uni_read(PRIVATE_DATA->handle, (char *)(reply + 2), reply[1] + 1) > 0) {
					if (buffer[4] != reply[4] || buffer[2] != reply[3] || buffer[3] != reply[2]) {
						continue;
					}
					return true;
				} else {
					return false;
				}
			}
		}
	}
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

static bool nexstaraux_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (strncmp(name, "nexstar://", 10) == 0) {
		char *host = name + 10;
		if (*host == 0) {
			char result[256], payload[1024];
			if (indigo_perform_passive_discovery(55555, 3, result, sizeof(result), payload, sizeof(payload))) {
				sprintf(name, "nexstar://%s:%d", result, 2000);
				DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DEVICE_PORT_PROPERTY, "Mount detected at %s (%s)", name, payload);
				PRIVATE_DATA->handle = indigo_uni_open_url(name, 2000, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG | BINARY_LOG);
			}
		} else {
			PRIVATE_DATA->handle = indigo_uni_open_url(name, 2000, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG | BINARY_LOG);
		}
	}
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_set_socket_read_timeout(PRIVATE_DATA->handle, 1000000);
		indigo_uni_set_socket_write_timeout(PRIVATE_DATA->handle, 1000000);
		unsigned char alt_version[16] = { 0 }, azm_version[16] = { 0 };
		if (nexstaraux_command(device, APP, ALT, MC_GET_VER, NULL, 0, alt_version) && nexstaraux_command(device, APP, AZM, MC_GET_VER, NULL, 0, azm_version)) {
			sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value,"%d.%d / %d.%d", alt_version[5], alt_version[6], azm_version[5], azm_version[6]);
			strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron");
			strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "NexStar AUX");
			strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			indigo_update_property(device, INFO_PROPERTY, NULL);
		} else {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
	}
	return PRIVATE_DATA->handle != NULL;
}

static void nexstaraux_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static bool nexstaraux_validate_handle(indigo_device *device) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!indigo_uni_is_valid(PRIVATE_DATA->handle)) {
		nexstaraux_close(device);
		indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		return false;
	}
	return true;
}

static bool nexstaraux_set_tracking(indigo_device *device, bool on) {
	unsigned char reply[16] = { 0 };
	uint_fast16_t rate;
	if (!on) {
		rate = 0x0000;
	} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
		rate = 0xFFFD;
	} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
		rate = 0xFFFE;
	} else {
		rate = 0xFFFF;
	}
	commands set_guide_rate = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0 ? MC_SET_POS_GUIDERATE : MC_SET_NEG_GUIDERATE;
	return nexstaraux_command_16(device, APP, AZM, set_guide_rate, rate, reply);
}

static bool nexstaraux_get_slew_state(indigo_device *device, bool *in_propress) {
	unsigned char reply[16] = { 0 };
	if (nexstaraux_command(device, APP, AZM, MC_SLEW_DONE, NULL, 0, reply)) {
		if (reply[5] == 0x00) {
			*in_propress = true;
			return true;
		}
		if (nexstaraux_command(device, APP, ALT, MC_SLEW_DONE, NULL, 0, reply)) {
			if (reply[5] == 0x00) {
				*in_propress = true;
			} else {
				*in_propress = false;
			}
			return true;
		}
	}
	return false;
}

static bool nexstaraux_slew(indigo_device *device, double ra, double dec, bool fast) {
	unsigned char reply[16] = { 0 };
	double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	double ha = fmod(lst - ra + 24, 24);
	int32_t raw_ra = (int32_t)((fmod(ha + 12, 24) / 24.0) * 0x1000000) % 0x1000000;
	int32_t raw_dec = (int32_t)((dec / 360.0) * 0x1000000) % 0x1000000;
	if (nexstaraux_command_24(device, APP, AZM, fast ? MC_GOTO_FAST : MC_GOTO_SLOW, raw_ra, reply) && nexstaraux_command_24(device, APP, ALT, fast ? MC_GOTO_FAST : MC_GOTO_SLOW, raw_dec, reply)) {
		return true;
	}
	return false;
}

static bool nexstaraux_sync(indigo_device *device, double ra, double dec) {
	unsigned char reply[16] = { 0 };
	double lst = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	double ha = fmod(lst - ra + 24, 24);
	int32_t raw_ra = (int32_t)((fmod(ha + 12, 24) / 24.0) * 0x1000000) % 0x1000000;
	int32_t raw_dec = (int32_t)((dec / 360.0) * 0x1000000) % 0x1000000;
	if (nexstaraux_command_24(device, APP, AZM, MC_SET_POSITION, raw_ra, reply) && nexstaraux_command_24(device, APP, ALT, MC_SET_POSITION, raw_dec, reply)) {
		return true;
	}
	return false;
}

static bool nexstaraux_stop(indigo_device *device) {
	unsigned char reply[16] = { 0 };
	return nexstaraux_command_24(device, APP, AZM, MC_MOVE_POS, 0, reply) && nexstaraux_command_24(device, APP, ALT, MC_MOVE_POS, 0, reply);
}

static bool nextstar_get_coordinates(indigo_device *device, double *ra, double *dec) {
	unsigned char reply[16] = { 0 };
	if (nexstaraux_command(device, APP, ALT, MC_GET_POSITION, NULL, 0, reply)) {
		int raw_dec = reply[5] << 16 | reply[6] << 8 | reply[7];
		if (nexstaraux_command(device, APP, AZM, MC_GET_POSITION, NULL, 0, reply)) {
			int raw_ra = reply[5] << 16 | reply[6] << 8 | reply[7];
			double ha = fmod(((double)raw_ra / 0x1000000) * 24 + 12, 24);
			*ra = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - ha + 24, 24);
			*dec = fmod(((double)raw_dec / 0x1000000) * 360, 360);
			return true;
		}
	}
	return false;
}

static bool nexstar_get_guide_rate(indigo_device *device, double *ra, double *dec) {
	unsigned char reply[16] = { 0 };
	if (nexstaraux_command(device, APP, AZM, MC_GET_AUTOGUIDE_RATE, NULL, 0, reply)) {
		*ra = reply[5] * 100.0 / 256;
		if (nexstaraux_command(device, APP, ALT, MC_GET_AUTOGUIDE_RATE, NULL, 0, reply)) {
			*dec = reply[5] * 100.0 / 256;
			return true;
		}
	}
	return false;
}

static bool nexstar_set_guide_rate_handler(indigo_device *device, double ra, double dec) {
	unsigned char reply[16] = { 0 };
	unsigned char rate = (unsigned char)(ra / 100.0 * 256);
	if (nexstaraux_command(device, APP, AZM, MC_SET_AUTOGUIDE_RATE, &rate, 1, reply)) {
		rate = (unsigned char)(dec / 100.0 * 256);
		return nexstaraux_command(device, APP, ALT, MC_SET_AUTOGUIDE_RATE, &rate, 1, reply);
	}
	return false;
}

static bool nexstaraux_move_ra(indigo_device *device, int speed) {
	unsigned char reply[16];
	unsigned char rate = abs(speed);
	return nexstaraux_command(device, APP, AZM, speed < 0 ? MC_MOVE_NEG : MC_MOVE_POS, &rate, 1, reply);
}

static bool nexstaraux_move_dec(indigo_device *device, int speed) {
	unsigned char reply[16];
	unsigned char rate = abs(speed);
	return nexstaraux_command(device, APP, ALT, speed < 0 ? MC_MOVE_NEG : MC_MOVE_POS, &rate, 1, reply);
}

static bool nexstaraux_guide_ra(indigo_device *device, int direction) {
	unsigned char reply[16];
	unsigned char rate = abs(direction);
	return nexstaraux_command(device, APP, AZM, direction < 0 ? MC_MOVE_NEG : MC_MOVE_POS, &rate, 1, reply);
}

static bool nexstaraux_guide_dec(indigo_device *device, int direction) {
	unsigned char reply[16];
	unsigned char rate = abs(direction);
	return nexstaraux_command(device, APP, ALT, direction < 0 ? MC_MOVE_NEG : MC_MOVE_POS, &rate, 1, reply);
}

static void nexstar_update_mount_state(indigo_device *device) {
	double ra = 0, dec = 0;
	if (nextstar_get_coordinates(device, &ra, &dec)) {
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		indigo_timetoisogm(time(NULL), MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		indigo_update_coordinates(device, NULL);
	}
}

//- code

//+ mount.code

static void mount_slew_finalizer(indigo_device *device) {
	bool in_progress;
	if (nexstaraux_get_slew_state(device, &in_progress)) {
		if (in_progress) {
			indigo_execute_handler_in(device, 0.1, mount_slew_finalizer);
		} else {
			if (PRIVATE_DATA->stopping) {
				PRIVATE_DATA->centering = PRIVATE_DATA->slewing = PRIVATE_DATA->stopping = false;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
				MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_ALERT_STATE;
				if (PRIVATE_DATA->parking) {
					PRIVATE_DATA->parking = false;
					PRIVATE_DATA->parked = false;
					MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
					MOUNT_STATE_PARK_ITEM->light.value = INDIGO_ALERT_STATE;
				}
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			} else if (PRIVATE_DATA->centering) {
				if (!PRIVATE_DATA->parking) {
					if (nexstaraux_set_tracking(device, true)) {
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
						MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
					} else {
						indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
						MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
						MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_ALERT_STATE;
					}
				}
				PRIVATE_DATA->centering = PRIVATE_DATA->slewing = false;
				MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
				if (PRIVATE_DATA->parking) {
					PRIVATE_DATA->parking = false;
					PRIVATE_DATA->parked = true;
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
					MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
				}
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			} else {
				double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
				double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
				indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
				if (nexstaraux_slew(device, ra, dec, false)) {
					PRIVATE_DATA->centering = PRIVATE_DATA->slewing = true;
					indigo_execute_handler_in(device, 0.1, mount_slew_finalizer);
				}
			}
		}
	}
}

//- mount.code

//+ guider.code

static void guider_guide_dec_finalizer(indigo_device *device) {
	if (nexstaraux_guide_ra(device, 0)) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	if (nexstaraux_guide_dec(device, 0)) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_OK_STATE, NULL);
}

//- guider.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	nexstar_update_mount_state(device);
	indigo_execute_handler_in(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = nexstaraux_open(device);
		}
		if (connection_result) {
			//+ mount.on_connect
			if (nexstaraux_set_tracking(device, false)) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			} else {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (nexstar_get_guide_rate(device, &MOUNT_GUIDE_RATE_RA_ITEM->number.value, &MOUNT_GUIDE_RATE_DEC_ITEM->number.value)) {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, mount_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			nexstaraux_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	if (!nexstaraux_set_tracking(device, MOUNT_TRACKING_ON_ITEM->sw.value)) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	MOUNT_STATE_TRACKING_ITEM->light.value = MOUNT_TRACKING_PROPERTY->state == INDIGO_ALERT_STATE ? INDIGO_ALERT_STATE : MOUNT_TRACKING_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_track_rate_handler(indigo_device *device) {
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACK_RATE.on_change
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (!nexstaraux_set_tracking(device, true)) {
			MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (nexstaraux_set_tracking(device, false) && nexstaraux_slew(device, ra, dec, true)) {
			MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			PRIVATE_DATA->centering = PRIVATE_DATA->parking = PRIVATE_DATA->parked = false;
			PRIVATE_DATA->slewing = true;
			indigo_execute_handler_in(device, 0.1, mount_slew_finalizer);
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (nexstaraux_sync(device, ra, dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	double ra = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) + 24, 24);
	double dec = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value > 0 ? 90 : -90;
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = ra;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = dec;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (nexstaraux_set_tracking(device, false) && nexstaraux_slew(device, ra, dec, true)) {
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_OK_STATE;
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		PRIVATE_DATA->centering = PRIVATE_DATA->parked = false;
		PRIVATE_DATA->slewing = PRIVATE_DATA->parking = true;
		indigo_execute_handler_in(device, 0.1, mount_slew_finalizer);
	} else {
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_PARK.on_change
}

static void mount_abort_motion_handler(indigo_device *device) {
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_ABORT_MOTION.on_change
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		unsigned char reply[16] = { 0 };
		if (nexstaraux_command_24(device, APP, AZM, MC_MOVE_POS, 0, reply) && nexstaraux_command_24(device, APP, ALT, MC_MOVE_POS, 0, reply)) {
			if (MOUNT_MOTION_WEST_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value) {
				MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
				MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			}
			if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) {
				MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
				MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			}
			if (PRIVATE_DATA->slewing) {
				PRIVATE_DATA->stopping = true;
			}
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_RA.on_change
	unsigned char rate = 0;
	int speed = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		rate = 1;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		rate = 2;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		rate = 5;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		rate = 9;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		speed = rate;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		speed = -rate;
	} else {
		speed = 0;
	}
	if (nexstaraux_move_ra(device, speed)) {
		MOUNT_MOTION_RA_PROPERTY->state = speed ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	unsigned char rate = 0;
	int speed = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		rate = 1;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		rate = 2;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		rate = 5;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		rate = 9;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		speed = rate;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		speed = -rate;
	} else {
		speed = 0;
	}
	if (nexstaraux_move_dec(device, speed)) {
		MOUNT_MOTION_DEC_PROPERTY->state = speed ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	if (!nexstar_set_guide_rate_handler(device, MOUNT_GUIDE_RATE_RA_ITEM->number.target, MOUNT_GUIDE_RATE_DEC_ITEM->number.target)) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		//+ mount.on_attach
		INFO_PROPERTY->count = 6;
		strcpy(DEVICE_PORT_ITEM->text.value, "nexstar://");
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_GUIDE_RATE_PROPERTY->count = 2;
		//- mount.on_attach
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		MOUNT_STATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, mount_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = nexstaraux_open(device->master_device);
		}
		if (connection_result) {
			//+ guider.on_connect
			if (nexstar_get_guide_rate(device, &GUIDER_RATE_ITEM->number.value, &GUIDER_DEC_RATE_ITEM->number.value)) {
				GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			nexstaraux_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	unsigned duration = 0;
	int direction = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		direction = 1;
		duration = (unsigned)GUIDER_GUIDE_EAST_ITEM->number.value;
	} else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		direction = -1;
		duration = (unsigned)GUIDER_GUIDE_WEST_ITEM->number.value;
	}
	if (nexstaraux_guide_ra(device, direction)) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, duration / 1000, guider_guide_ra_finalizer);
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	unsigned duration = 0;
	int direction = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		direction = 1;
		duration = (unsigned)GUIDER_GUIDE_NORTH_ITEM->number.value;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		direction = -1;
		duration = (unsigned)GUIDER_GUIDE_SOUTH_ITEM->number.value;
	}
	if (nexstaraux_guide_dec(device, direction)) {
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, duration / 1000, guider_guide_dec_finalizer);
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_rate_handler(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_RATE.on_change
	if (!nexstar_set_guide_rate_handler(device, GUIDER_RATE_ITEM->number.target, GUIDER_DEC_RATE_ITEM->number.target)) {
		GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- guider.GUIDER_RATE.on_change
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		GUIDER_RATE_PROPERTY->count = 2;
		GUIDER_RATE_PROPERTY->hidden = false;
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, guider_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_nexstaraux(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nexstaraux_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(nexstaraux_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
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
