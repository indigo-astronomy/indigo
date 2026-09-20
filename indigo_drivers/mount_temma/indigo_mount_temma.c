// Copyright (c) 2017-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_mount_temma.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_temma.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000D
#define DRIVER_NAME          "indigo_mount_temma"
#define DRIVER_LABEL         "Takahashi Temma Mount"
#define MOUNT_DEVICE_NAME    "Takahashi Temma Mount"
#define GUIDER_DEVICE_NAME   "Takahashi Temma Mount (guider)"
#define PRIVATE_DATA         ((temma_private_data *)device->private_data)

//+ define

#define CCD_ADVANCED_GROUP   "Advanced"
#define TEMMA_MOTION_RA_EAST 0x02
#define TEMMA_MOTION_RA_WEST 0x04
#define TEMMA_MOTION_DEC_NORTH 0x08
#define TEMMA_MOTION_DEC_SOUTH 0x10

//- define

#pragma mark - Property definitions

#define CORRECTION_SPEED_PROPERTY      (PRIVATE_DATA->correction_speed_property)
#define CORRECTION_SPEED_RA_ITEM       (CORRECTION_SPEED_PROPERTY->items + 0)
#define CORRECTION_SPEED_DEC_ITEM      (CORRECTION_SPEED_PROPERTY->items + 1)

#define CORRECTION_SPEED_PROPERTY_NAME "X_TEMMA_CORRECTION_SPEED"
#define CORRECTION_SPEED_RA_ITEM_NAME  "RA"
#define CORRECTION_SPEED_DEC_ITEM_NAME "DEC"

#define HIGH_SPEED_PROPERTY            (PRIVATE_DATA->high_speed_property)
#define HIGH_SPEED_LOW_ITEM            (HIGH_SPEED_PROPERTY->items + 0)
#define HIGH_SPEED_HIGH_ITEM           (HIGH_SPEED_PROPERTY->items + 1)

#define HIGH_SPEED_PROPERTY_NAME       "X_TEMMA_HIGH_SPEED"
#define HIGH_SPEED_LOW_ITEM_NAME       "LOW"
#define HIGH_SPEED_HIGH_ITEM_NAME      "HIGH"

#define ZENITH_PROPERTY                (PRIVATE_DATA->zenith_property)
#define ZENITH_EAST_ITEM               (ZENITH_PROPERTY->items + 0)
#define ZENITH_WEST_ITEM               (ZENITH_PROPERTY->items + 1)

#define ZENITH_PROPERTY_NAME           "X_TEMMA_ZENITH"
#define ZENITH_EAST_ITEM_NAME          "EAST"
#define ZENITH_WEST_ITEM_NAME          "WEST"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *correction_speed_property;
	indigo_property *high_speed_property;
	indigo_property *zenith_property;
	//+ data
	double current_ra, current_dec;
	char telescope_side;
	bool is_busy, start_tracking, stop_tracking;
	unsigned char mount_motion_mask, guider_motion_mask;
	bool mount_high_speed;
	char response[128];
	//- data
} temma_private_data;

#pragma mark - Low level code

//+ code

static void mount_goto_finalizer(indigo_device *device);
static void mount_motion_finalizer(indigo_device *device);
static void guider_guide_ra_finalizer(indigo_device *device);
static void guider_guide_dec_finalizer(indigo_device *device);

static bool temma_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_config(DEVICE_PORT_ITEM->text.value, "19200-8E1", INDIGO_LOG_DEBUG);
	return PRIVATE_DATA->handle != NULL;
}

static void temma_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static bool temma_vcommand(indigo_device *device, bool reply, const char *format, va_list args) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, format, args);
	}
	if (result >= 0) {
		result = indigo_uni_printf(PRIVATE_DATA->handle, "\r\n");
	}
	if (!reply) {
		return result >= 0;
	}
	result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(0.3), INDIGO_DELAY(0.3));
	if (result <= 0) {
		return false;
	}
	while (result > 0 && (PRIVATE_DATA->response[result - 1] == '\r' || PRIVATE_DATA->response[result - 1] == '\n')) {
		result--;
	}
	PRIVATE_DATA->response[result] = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Temma %s -> %s", format, PRIVATE_DATA->response);
	return true;
}

static bool temma_command(indigo_device *device, bool reply, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = temma_vcommand(device, reply, format, args);
	va_end(args);
	return result;
}

static bool temma_command_ack(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = temma_vcommand(device, true, format, args);
	va_end(args);
	return result && !strcmp(PRIVATE_DATA->response, "R1");
}

static unsigned char temma_motion_byte(indigo_device *device) {
	unsigned char motion_mask = PRIVATE_DATA->mount_motion_mask | PRIVATE_DATA->guider_motion_mask;
	return motion_mask == 0 ? 'A' : 0x40 | motion_mask | (PRIVATE_DATA->mount_motion_mask != 0 && PRIVATE_DATA->mount_high_speed ? 1 : 0);
}

static bool temma_update_motion(indigo_device *device) {
	indigo_device *mount = device->master_device == NULL ? device : device->master_device;
	return temma_command_ack(mount, "M%c", temma_motion_byte(mount));
}

static bool temma_update_position(indigo_device *device) {
	if (!temma_command(device, true, "E") || strlen(PRIVATE_DATA->response) != 15 || PRIVATE_DATA->response[0] != 'E' || (PRIVATE_DATA->response[7] != '+' && PRIVATE_DATA->response[7] != '-') || (PRIVATE_DATA->response[13] != 'E' && PRIVATE_DATA->response[13] != 'W') || (PRIVATE_DATA->response[14] != '0' && PRIVATE_DATA->response[14] != '1')) {
		return false;
	}
	for (int index = 1; index <= 12; index++) {
		if (index != 7 && !isdigit((unsigned char)PRIVATE_DATA->response[index])) {
			return false;
		}
	}
	int degrees, minutes, seconds;
	if (sscanf(PRIVATE_DATA->response + 1, "%02d%02d%02d", &degrees, &minutes, &seconds) != 3 || degrees > 23 || minutes > 59 || seconds > 59) {
		return false;
	}
	PRIVATE_DATA->current_ra = degrees + minutes / 60.0 + seconds / 3600.0;
	if (sscanf(PRIVATE_DATA->response + 8, "%02d%02d%01d", &degrees, &minutes, &seconds) != 3 || degrees > 90 || minutes > 59 || (degrees == 90 && (minutes != 0 || seconds != 0))) {
		return false;
	}
	PRIVATE_DATA->current_dec = degrees + minutes / 60.0 + seconds / 600.0;
	if (PRIVATE_DATA->response[7] == '-') {
		PRIVATE_DATA->current_dec = -PRIVATE_DATA->current_dec;
	}
	indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &PRIVATE_DATA->current_ra, &PRIVATE_DATA->current_dec);
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->current_ra;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->current_dec;
	PRIVATE_DATA->telescope_side = PRIVATE_DATA->response[13];
	indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, PRIVATE_DATA->telescope_side == 'W' ? MOUNT_SIDE_OF_PIER_EAST_ITEM : MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
	return true;
}

static bool temma_set_lst(indigo_device *device) {
	time_t utc = indigo_get_mount_utc(device);
	double lst = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
	return temma_command_ack(device, "T%02d%02d%02d", (int)lst, ((int)(lst * 60)) % 60, ((int)(lst * 3600)) % 60);
}

static bool temma_set_latitude(indigo_device *device) {
	double latitude = fabs(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value);
	int degrees = latitude;
	int minutes = (latitude - degrees) * 60;
	int tenths = ((int)(latitude * 600)) % 10;
	return temma_command_ack(device, "I%c%02d%02d%d", MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0 ? '-' : '+', degrees, minutes, tenths);
}

static void mount_goto_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !temma_update_position(device) || !temma_command(device, true, "s") || (strcmp(PRIVATE_DATA->response, "s0") && strcmp(PRIVATE_DATA->response, "s1"))) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->response[1] == '1') {
		indigo_update_coordinates(device, NULL);
		indigo_execute_handler_in(device, 0.5, mount_goto_finalizer);
		return;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		if (PRIVATE_DATA->start_tracking) {
			if (!temma_command_ack(device, "STN-OFF")) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			PRIVATE_DATA->start_tracking = false;
		}
		if (PRIVATE_DATA->stop_tracking) {
			if (!temma_command_ack(device, "STN-ON")) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			PRIVATE_DATA->stop_tracking = false;
		}
	}
	indigo_update_coordinates(device, NULL);
}

static void mount_motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || PRIVATE_DATA->mount_motion_mask == 0) {
		return;
	}
	if (temma_update_motion(device)) {
		indigo_execute_handler_in(device, 0.25, mount_motion_finalizer);
	} else {
		PRIVATE_DATA->mount_motion_mask = 0;
		MOUNT_MOTION_RA_PROPERTY->state = MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	}
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_RA_EAST | TEMMA_MOTION_RA_WEST);
	bool ok = temma_update_motion(device);
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_DEC_NORTH | TEMMA_MOTION_DEC_SOUTH);
	bool ok = temma_update_motion(device);
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

//- code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	bool ok = temma_update_position(device) && temma_command(device, true, "s") && (!strcmp(PRIVATE_DATA->response, "s0") || !strcmp(PRIVATE_DATA->response, "s1"));
	if (ok) {
		PRIVATE_DATA->is_busy = !strcmp(PRIVATE_DATA->response, "s1");
	}
	if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	}
	indigo_update_coordinates(device, NULL);
	indigo_execute_handler_in(device, 0.5, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = temma_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			connection_result = temma_command(device, true, "v") && PRIVATE_DATA->response[0] == 'v' && PRIVATE_DATA->response[1] != 0;
			if (connection_result) {
				INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "Takahashi");
				INDIGO_COPY_VALUE(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->response + 1);
				INDIGO_COPY_VALUE(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
				connection_result = temma_command_ack(device, "v1");
				if (connection_result) {
					PRIVATE_DATA->mount_motion_mask = PRIVATE_DATA->guider_motion_mask = 0;
					PRIVATE_DATA->mount_high_speed = false;
					temma_update_position(device);
					if (temma_command(device, true, "lg") && strlen(PRIVATE_DATA->response) == 8 && PRIVATE_DATA->response[0] == 'l' && PRIVATE_DATA->response[1] == 'g' && isdigit((unsigned char)PRIVATE_DATA->response[2]) && isdigit((unsigned char)PRIVATE_DATA->response[3]) && PRIVATE_DATA->response[4] == 'D' && isdigit((unsigned char)PRIVATE_DATA->response[5]) && isdigit((unsigned char)PRIVATE_DATA->response[6]) && (PRIVATE_DATA->response[7] == 'N' || PRIVATE_DATA->response[7] == 'S')) {
						int ra_correction = atoi(PRIVATE_DATA->response + 2);
						int dec_correction = atoi(PRIVATE_DATA->response + 5);
						if (ra_correction >= 10 && ra_correction <= 90 && dec_correction >= 10 && dec_correction <= 90) {
							CORRECTION_SPEED_RA_ITEM->number.value = ra_correction;
							CORRECTION_SPEED_DEC_ITEM->number.value = dec_correction;
						}
					}
				}
			}
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, CORRECTION_SPEED_PROPERTY, NULL);
			indigo_define_property(device, HIGH_SPEED_PROPERTY, NULL);
			indigo_define_property(device, ZENITH_PROPERTY, NULL);
			indigo_execute_handler(device, mount_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				temma_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		indigo_cancel_pending_handlers(device);
		PRIVATE_DATA->mount_motion_mask = 0;
		PRIVATE_DATA->mount_high_speed = false;
		if (PRIVATE_DATA->handle != NULL) {
			temma_update_motion(device);
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
				temma_command_ack(device, "PS");
			}
		}
		//- mount.on_disconnect
		indigo_delete_property(device, CORRECTION_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, HIGH_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, ZENITH_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			temma_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_correction_speed_handler(indigo_device *device) {
	CORRECTION_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.CORRECTION_SPEED.on_change
	bool ok = temma_command_ack(device, "LA%02d", (int)CORRECTION_SPEED_RA_ITEM->number.value) && temma_command_ack(device, "LB%02d", (int)CORRECTION_SPEED_DEC_ITEM->number.value);
	CORRECTION_SPEED_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.CORRECTION_SPEED.on_change
	indigo_update_property(device, CORRECTION_SPEED_PROPERTY, NULL);
}

static void mount_high_speed_handler(indigo_device *device) {
	//+ mount.HIGH_SPEED.on_change
	HIGH_SPEED_PROPERTY->state = temma_command_ack(device, HIGH_SPEED_HIGH_ITEM->sw.value ? "v2" : "v1") ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.HIGH_SPEED.on_change
	indigo_update_property(device, HIGH_SPEED_PROPERTY, NULL);
}

static void mount_zenith_handler(indigo_device *device) {
	ZENITH_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.ZENITH.on_change
	bool ok = temma_command_ack(device, "Z");
	ZENITH_EAST_ITEM->sw.value = false;
	ZENITH_WEST_ITEM->sw.value = false;
	ZENITH_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.ZENITH.on_change
	indigo_update_property(device, ZENITH_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	int ra_seconds = ra * 3600;
	int dec_tenths = dec * 600;
	char dec_sign = dec_tenths < 0 ? '-' : '+';
	dec_tenths = abs(dec_tenths);
	char command[32];
	snprintf(command, sizeof(command), "%c%02d%02d%02d%c%02d%02d%d", MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value ? 'D' : 'P', ra_seconds / 3600, (ra_seconds / 60) % 60, ra_seconds % 60, dec_sign, dec_tenths / 600, (dec_tenths / 10) % 60, dec_tenths % 10);
	if (temma_command_ack(device, "%s", command)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->start_tracking = MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value;
		PRIVATE_DATA->stop_tracking = MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value;
		indigo_update_coordinates(device, NULL);
		indigo_execute_handler_in(device, 0.1, mount_goto_finalizer);
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, mount_goto_finalizer);
	indigo_cancel_pending_handler(device, mount_motion_finalizer);
	PRIVATE_DATA->mount_motion_mask = 0;
	bool ok = temma_update_motion(device) && temma_command_ack(device, "PS");
	MOUNT_MOTION_RA_PROPERTY->state = MOUNT_MOTION_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_coordinates(device, NULL);
	if (!ok) {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (MOUNT_ABORT_MOTION_PROPERTY->state != INDIGO_ALERT_STATE) {
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	//- mount.MOUNT_ABORT_MOTION.on_change
}

static void mount_track_rate_handler(indigo_device *device) {
	//+ mount.MOUNT_TRACK_RATE.on_change
	MOUNT_TRACK_RATE_PROPERTY->state = temma_command_ack(device, MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value ? "LK" : "LL") ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_TRACK_RATE.on_change
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_TRACKING.on_change
	MOUNT_TRACKING_PROPERTY->state = temma_command_ack(device, MOUNT_TRACKING_ON_ITEM->sw.value ? "STN-OFF" : "STN-ON") ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_DEC.on_change
	PRIVATE_DATA->mount_motion_mask &= ~(TEMMA_MOTION_DEC_NORTH | TEMMA_MOTION_DEC_SOUTH);
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		PRIVATE_DATA->mount_motion_mask |= TEMMA_MOTION_DEC_NORTH;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		PRIVATE_DATA->mount_motion_mask |= TEMMA_MOTION_DEC_SOUTH;
	}
	PRIVATE_DATA->mount_high_speed = !(MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value);
	indigo_cancel_pending_handler(device, mount_motion_finalizer);
	if (PRIVATE_DATA->mount_motion_mask) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_execute_handler(device, mount_motion_finalizer);
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = temma_update_motion(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	}
	//- mount.MOUNT_MOTION_DEC.on_change
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_RA.on_change
	PRIVATE_DATA->mount_motion_mask &= ~(TEMMA_MOTION_RA_EAST | TEMMA_MOTION_RA_WEST);
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		PRIVATE_DATA->mount_motion_mask |= TEMMA_MOTION_RA_WEST;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		PRIVATE_DATA->mount_motion_mask |= TEMMA_MOTION_RA_EAST;
	}
	PRIVATE_DATA->mount_high_speed = !(MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value || MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value);
	indigo_cancel_pending_handler(device, mount_motion_finalizer);
	if (PRIVATE_DATA->mount_motion_mask) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_execute_handler(device, mount_motion_finalizer);
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = temma_update_motion(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	}
	//- mount.MOUNT_MOTION_RA.on_change
}

static void mount_side_of_pier_handler(indigo_device *device) {
	MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SIDE_OF_PIER.on_change
	bool switch_side = (MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value && PRIVATE_DATA->telescope_side == 'E') || (MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value && PRIVATE_DATA->telescope_side == 'W');
	MOUNT_SIDE_OF_PIER_PROPERTY->state = !switch_side || temma_command_ack(device, "PT") ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (MOUNT_SIDE_OF_PIER_PROPERTY->state == INDIGO_OK_STATE) {
		temma_update_position(device);
	}
	//- mount.MOUNT_SIDE_OF_PIER.on_change
	indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK.on_change
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		time_t utc = indigo_get_mount_utc(device);
		double ra = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_PARK_POSITION_HA_ITEM->number.value;
		int ra_seconds = (ra < 0 ? ra + 24 : ra) * 3600;
		int dec_tenths = MOUNT_PARK_POSITION_DEC_ITEM->number.value * 600;
		char dec_sign = dec_tenths < 0 ? '-' : '+';
		dec_tenths = abs(dec_tenths);
		bool ok = temma_set_lst(device) && temma_command_ack(device, "P%02d%02d%02d%c%02d%02d%d", ra_seconds / 3600, (ra_seconds / 60) % 60, ra_seconds % 60, dec_sign, dec_tenths / 600, (dec_tenths / 10) % 60, dec_tenths % 10) && temma_command_ack(device, "STN-ON");
		MOUNT_PARK_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
	}
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0) {
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
	}
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = temma_set_latitude(device) && temma_set_lst(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ mount.on_attach
		MOUNT_PARK_PROPERTY->count = 1;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RW_PERM;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		//- mount.on_attach
		CORRECTION_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, CORRECTION_SPEED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Correction speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (CORRECTION_SPEED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(CORRECTION_SPEED_RA_ITEM, CORRECTION_SPEED_RA_ITEM_NAME, "RA speed (10% - 90%)", 10, 90, 1, 50);
		indigo_init_number_item(CORRECTION_SPEED_DEC_ITEM, CORRECTION_SPEED_DEC_ITEM_NAME, "Dec speed (10% - 90%)", 10, 90, 1, 50);
		HIGH_SPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, HIGH_SPEED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "High-speed or High-voltage config", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (HIGH_SPEED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(HIGH_SPEED_LOW_ITEM, HIGH_SPEED_LOW_ITEM_NAME, "12V or Low-speed", true);
		indigo_init_switch_item(HIGH_SPEED_HIGH_ITEM, HIGH_SPEED_HIGH_ITEM_NAME, "24V or High-speed", false);
		ZENITH_PROPERTY = indigo_init_switch_property(NULL, device->name, ZENITH_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Sync zenith", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (ZENITH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ZENITH_EAST_ITEM, ZENITH_EAST_ITEM_NAME, "East zenith", false);
		indigo_init_switch_item(ZENITH_WEST_ITEM, ZENITH_WEST_ITEM_NAME, "West zenith", false);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(CORRECTION_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(HIGH_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(ZENITH_PROPERTY);
	}
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
	} else if (indigo_property_match_changeable(CORRECTION_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CORRECTION_SPEED_PROPERTY, mount_correction_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(HIGH_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(HIGH_SPEED_PROPERTY, mount_high_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ZENITH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ZENITH_PROPERTY, mount_zenith_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_EQUATORIAL_COORDINATES_PROPERTY->count; i++) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items[i].do_update = true;
			}
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACK_RATE_PROPERTY, mount_track_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_TRACKING_PROPERTY->count; i++) {
				MOUNT_TRACKING_PROPERTY->items[i].do_update = true;
			}
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_MOTION_DEC_PROPERTY->count; i++) {
				MOUNT_MOTION_DEC_PROPERTY->items[i].do_update = true;
			}
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
			for (int i = 0; i < MOUNT_MOTION_RA_PROPERTY->count; i++) {
				MOUNT_MOTION_RA_PROPERTY->items[i].do_update = true;
			}
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SIDE_OF_PIER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SIDE_OF_PIER_PROPERTY, mount_side_of_pier_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CORRECTION_SPEED_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(CORRECTION_SPEED_PROPERTY);
	indigo_release_property(HIGH_SPEED_PROPERTY);
	indigo_release_property(ZENITH_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = temma_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				temma_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		PRIVATE_DATA->guider_motion_mask = 0;
		if (PRIVATE_DATA->handle != NULL) {
			temma_update_motion(device);
		}
		//- guider.on_disconnect
		if (--PRIVATE_DATA->count == 0) {
			temma_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_DEC_NORTH | TEMMA_MOTION_DEC_SOUTH);
	double duration = 0;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		PRIVATE_DATA->guider_motion_mask |= TEMMA_MOTION_DEC_NORTH;
		duration = GUIDER_GUIDE_NORTH_ITEM->number.value / 1000.0;
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		PRIVATE_DATA->guider_motion_mask |= TEMMA_MOTION_DEC_SOUTH;
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value / 1000.0;
	}
	bool ok = temma_update_motion(device);
	if (duration > 0 && ok) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration, guider_guide_dec_finalizer);
	} else {
		if (!ok) {
			PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_DEC_NORTH | TEMMA_MOTION_DEC_SOUTH);
		}
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_RA_EAST | TEMMA_MOTION_RA_WEST);
	double duration = 0;
	if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		PRIVATE_DATA->guider_motion_mask |= TEMMA_MOTION_RA_WEST;
		duration = GUIDER_GUIDE_WEST_ITEM->number.value / 1000.0;
	} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		PRIVATE_DATA->guider_motion_mask |= TEMMA_MOTION_RA_EAST;
		duration = GUIDER_GUIDE_EAST_ITEM->number.value / 1000.0;
	}
	bool ok = temma_update_motion(device);
	if (duration > 0 && ok) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration, guider_guide_ra_finalizer);
	} else {
		if (!ok) {
			PRIVATE_DATA->guider_motion_mask &= ~(TEMMA_MOTION_RA_EAST | TEMMA_MOTION_RA_WEST);
		}
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
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
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
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

indigo_result indigo_mount_temma(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static temma_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (temma_private_data *)indigo_safe_malloc(sizeof(temma_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (guider != NULL) {
				indigo_detach_device(guider);
				indigo_safe_free(guider);
				guider = NULL;
			}
			if (mount != NULL) {
				indigo_detach_device(mount);
				indigo_safe_free(mount);
				mount = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
