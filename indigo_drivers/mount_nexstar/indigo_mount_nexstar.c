// Copyright (c) 2016-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_mount_nexstar.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_gps_driver.h>
#include "nexstar.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_nexstar.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300002C
#define DRIVER_NAME          "indigo_mount_nexstar"
#define DRIVER_LABEL         "Nexstar Mount"
#define MOUNT_DEVICE_NAME    "Mount Nexstar"
#define GUIDER_DEVICE_NAME   "Mount Nexstar (guider)"
#define PRIVATE_DATA         ((nexstar_private_data *)device->private_data)

//+ define

#define h2d(h)               (h * 15.0)
#define d2h(d)               (d / 15.0)
#define REFRESH_SECONDS      (0.5)
#define GPS_DEVICE_NAME      "Mount Nexstar (gps)"
#define NEXSTAR_AVX_MODEL_ID 20
#define NEXSTAR_CGX_MODEL_ID 23
#define NEXSTAR_MODERN_EQ_MODEL(id) ((id) == NEXSTAR_AVX_MODEL_ID || (id) == NEXSTAR_CGX_MODEL_ID)
#define WARN_PARKED_MSG      "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG "Mount parking is in progress, please wait until complete!"
#define is_connected         gp_bits

//- define

#pragma mark - Property definitions

#define TRACKING_MODE_PROPERTY         (PRIVATE_DATA->tracking_mode_property)
#define TRACKING_EQ_ITEM               (TRACKING_MODE_PROPERTY->items + 0)
#define TRACKING_AA_ITEM               (TRACKING_MODE_PROPERTY->items + 1)
#define TRACKING_AUTO_ITEM             (TRACKING_MODE_PROPERTY->items + 2)

#define TRACKING_MODE_PROPERTY_NAME    "TRACKING_MODE"
#define TRACKING_EQ_ITEM_NAME          "EQ"
#define TRACKING_AA_ITEM_NAME          "AA"
#define TRACKING_AUTO_ITEM_NAME        "AUTO"

#define COMMAND_GUIDE_RATE_PROPERTY      (PRIVATE_DATA->command_guide_rate_property)
#define GUIDE_50_ITEM                    (COMMAND_GUIDE_RATE_PROPERTY->items + 0)
#define GUIDE_100_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items + 1)

#define COMMAND_GUIDE_RATE_PROPERTY_NAME "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME               "GUIDE_50"
#define GUIDE_100_ITEM_NAME              "GUIDE_100"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *tracking_mode_property;
	indigo_property *command_guide_rate_property;
	//+ data
	char response[18];
	bool initialized;
	bool configured;
	int dev_id;
	int model_id;
	bool parked;
	bool park_in_progress;
	int slew_rate;
	int st4_ra_rate, st4_dec_rate;
	int vendor_id;
	uint32_t capabilities;
	pthread_mutex_t serial_mutex;
	int guide_rate;
	indigo_device *gps;
	bool guiding_in_progress;
	//- data
} nexstar_private_data;

#pragma mark - Low level code

//+ code

static void mount_equatorial_coordinates_handler(indigo_device *device);
static void mount_motion_dec_handler(indigo_device *device);
static void mount_motion_ra_handler(indigo_device *device);
static void mount_park_handler(indigo_device *device);
static void mount_park_finalizer(indigo_device *device);
static indigo_result gps_attach(indigo_device *device);
static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result gps_detach(indigo_device *device);

static int nexstar_get_autoguide_rate(int dev_id, int vendor_id, char axis) {
	if (vendor_id != VNDR_CELESTRON) {
		return tc_get_autoguide_rate(dev_id, axis);
	}
	char response[2];
	int destination = axis > 0 ? 16 : 17;
	int result = tc_pass_through_cmd(dev_id, 1, destination, 0x47, 0, 0, 0, 1, response);
	if (result != RC_OK || response[1] != '#') {
		return RC_FAILED;
	}
	return 100 * (unsigned char)response[0] / 256;
}

static int nexstar_set_autoguide_rate(int dev_id, int vendor_id, char axis, int rate) {
	if (vendor_id != VNDR_CELESTRON) {
		return tc_set_autoguide_rate(dev_id, axis, rate);
	}
	char response;
	int destination = axis > 0 ? 16 : 17;
	int encoded = rate == 0 ? 0 : rate == 99 ? 255 : 256 * rate / 100 + 1;
	int result = tc_pass_through_cmd(dev_id, 2, destination, 0x46, encoded, 0, 0, 0, &response);
	return result == RC_OK && response == '#' ? RC_OK : RC_FAILED;
}

static void nexstar_initialize_private_data(indigo_device *device) {
	if (!PRIVATE_DATA->initialized) {
		PRIVATE_DATA->initialized = true;
		PRIVATE_DATA->configured = false;
		PRIVATE_DATA->dev_id = -1;
		PRIVATE_DATA->model_id = -1;
		PRIVATE_DATA->vendor_id = -1;
		PRIVATE_DATA->guide_rate = 1;
		PRIVATE_DATA->slew_rate = 2;
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
	}
}

static bool nexstar_open(indigo_device *device) {
	int dev_id = open_telescope(DEVICE_PORT_ITEM->text.value);
	if (dev_id == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "open_telescope(%s) = %d (%s)", DEVICE_PORT_ITEM->text.value, dev_id, strerror(errno));
		PRIVATE_DATA->dev_id = -1;
		return false;
	}
	PRIVATE_DATA->dev_id = dev_id;
	PRIVATE_DATA->vendor_id = guess_mount_vendor(dev_id);
	int res = get_mount_capabilities(dev_id, &PRIVATE_DATA->capabilities, &PRIVATE_DATA->vendor_id);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_mount_capabilities(%d) = %d", dev_id, res);
	}
	PRIVATE_DATA->model_id = tc_get_model(dev_id);
	PRIVATE_DATA->capabilities &= ~(CAN_PULSE_GUIDE);
	// SynScan HC protocol 3.3 does not define the Celestron 0x46/0x47 ST4-rate passthrough commands.
	if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
		PRIVATE_DATA->capabilities &= ~(CAN_GET_SET_GUIDE_RATE);
	}
	return true;
}

static void nexstar_close(indigo_device *device) {
	if (device->master_device != NULL) {
		device = device->master_device;
	}
	if (PRIVATE_DATA->dev_id >= 0) {
		close_telescope(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
		PRIVATE_DATA->model_id = -1;
	}
	PRIVATE_DATA->configured = false;
}

static bool nexstar_stop_axis(indigo_device *device, int axis) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_slew_fixed(PRIVATE_DATA->dev_id, axis, TC_DIR_POSITIVE, 0);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

static void nexstar_attach_gps(indigo_device *device) {
	if (PRIVATE_DATA->gps != NULL) {
		return;
	}
	static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(GPS_DEVICE_NAME, gps_attach, indigo_gps_enumerate_properties, gps_change_property, NULL, gps_detach);
	PRIVATE_DATA->gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
	PRIVATE_DATA->gps->private_data = PRIVATE_DATA;
	PRIVATE_DATA->gps->master_device = device->master_device != NULL ? device->master_device : device;
	indigo_attach_device(PRIVATE_DATA->gps);
}

static void nexstar_detach_gps(indigo_device *device) {
	(void)device;
	if (PRIVATE_DATA->gps != NULL) {
		indigo_detach_device(PRIVATE_DATA->gps);
		indigo_safe_free(PRIVATE_DATA->gps);
		PRIVATE_DATA->gps = NULL;
	}
}

static bool nexstar_configure_mount(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	bool attach_gps = false;
	if (dev_id < 0) {
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (PRIVATE_DATA->vendor_id < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "guess_mount_vendor(%d) = %d (%s)", dev_id, PRIVATE_DATA->vendor_id, strerror(errno));
	} else if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
		INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher");
	} else if (PRIVATE_DATA->vendor_id == VNDR_CELESTRON) {
		INDIGO_COPY_VALUE(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron");
	}
	int model_id = PRIVATE_DATA->model_id;
	if (model_id < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_model(%d) = %d (%s)", dev_id, model_id, strerror(errno));
	} else {
		get_model_name(model_id, MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
	}
	if (enforce_protocol_version(dev_id, VER_AUTO) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_version(%d) = %d (%s)", dev_id, nexstar_proto_version, strerror(errno));
	} else if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
		snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "SynScan %2d.%02d.%02d", GET_RELEASE(nexstar_proto_version), GET_REVISION(nexstar_proto_version), GET_PATCH(nexstar_proto_version));
	} else {
		snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s %2d.%02d", nexstar_hc_type == HC_STARSENSE ? "StarSense" : "NexStar", GET_RELEASE(nexstar_proto_version), GET_REVISION(nexstar_proto_version));
	}
	if (PRIVATE_DATA->capabilities & CAN_GET_SET_GUIDE_RATE) {
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		int offset = PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER ? 0 : 1;
		int st4_ra_rate = nexstar_get_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_RA);
		if (st4_ra_rate < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_ra_rate, strerror(errno));
			MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
		} else {
			MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
			PRIVATE_DATA->st4_ra_rate = st4_ra_rate + offset;
			int st4_dec_rate = nexstar_get_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_DE);
			if (st4_dec_rate < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_dec_rate, strerror(errno));
			} else {
				MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
				PRIVATE_DATA->st4_dec_rate = st4_dec_rate + offset;
			}
		}
	} else {
		MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	}
	if (PRIVATE_DATA->capabilities & TRUE_EQ_MOUNT) {
		TRACKING_MODE_PROPERTY->hidden = true;
		indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_EQ_ITEM, true);
	} else {
		TRACKING_MODE_PROPERTY->hidden = false;
	}
	TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
	int mode = tc_get_tracking_mode(dev_id);
	if (mode < 0) {
		indigo_sleep(0.1);
		mode = tc_get_tracking_mode(dev_id);
	}
	if (mode < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_tracking_mode(%d) = %d (%s)", dev_id, mode, strerror(errno));
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (mode == TC_TRACK_OFF) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		if (TRACKING_AUTO_ITEM->sw.value) {
			TRACKING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, ALERT_PROPERTY, "Tracking mode can't be detected");
		}
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (TRACKING_AUTO_ITEM->sw.value) {
			if (mode == TC_TRACK_ALT_AZ) {
				indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_AA_ITEM, true);
			} else {
				indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_EQ_ITEM, true);
			}
			indigo_send_message(device, IDLE_PROPERTY, "Tracking mode detected");
		}
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	}
	PRIVATE_DATA->parked = false;
	PRIVATE_DATA->park_in_progress = false;
	indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
	if (PRIVATE_DATA->capabilities & CAN_GET_SIDE_OF_PIER) {
		int side_of_pier = tc_get_side_of_pier(dev_id);
		if (side_of_pier < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_side_of_pier(%d) = %d (%s)", dev_id, side_of_pier, strerror(errno));
		} else if (side_of_pier == 'W') {
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
		} else if (side_of_pier == 'E') {
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		}
	}
	attach_gps = PRIVATE_DATA->vendor_id == VNDR_CELESTRON;
	PRIVATE_DATA->configured = true;
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (attach_gps) {
		nexstar_attach_gps(device);
	}
	return true;
}

static void nexstar_update_position(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	if (dev_id < 0) {
		return;
	}
	double ra = 0, dec = 0, lon = 0, lat = 0;
	char side_of_pier = 0;
	time_t ttime = 0;
	int tz = 0, dst = 0;
	bool linked = false;
	bool position_valid = false;
	bool location_valid = false;
	bool time_valid = false;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (!PRIVATE_DATA->guiding_in_progress) {
		int goto_in_progress = tc_goto_in_progress(dev_id);
		if (goto_in_progress < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_in_progress(%d) = %d (%s)", dev_id, goto_in_progress, strerror(errno));
		}
		int res = tc_get_rade_p(dev_id, &ra, &dec);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_rade_p(%d) = %d (%s)", dev_id, res, strerror(errno));
		}
		if (goto_in_progress < 0 || res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			ra = d2h(ra);
			indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
			position_valid = true;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = (goto_in_progress || MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		}
		res = tc_get_location(dev_id, &lon, &lat);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_location(%d) = %d (%s)", dev_id, res, strerror(errno));
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			location_valid = true;
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
				MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		if (lon < 0) {
			lon += 360;
		}
		res = (int)tc_get_time(dev_id, &ttime, &tz, &dst);
		if (res == -1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_time(%d) = %d (%s)", dev_id, res, strerror(errno));
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			time_valid = true;
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE && MOUNT_TRACKING_OFF_ITEM->sw.value) {
			int mode = tc_get_tracking_mode(dev_id);
			if (mode < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_tracking_mode(%d) = %d (%s)", dev_id, mode, strerror(errno));
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (mode != TC_TRACK_OFF) {
				if (!TRACKING_MODE_PROPERTY->hidden && TRACKING_AUTO_ITEM->sw.value) {
					if (mode == TC_TRACK_ALT_AZ) {
						indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_AA_ITEM, true);
					} else {
						indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_EQ_ITEM, true);
					}
					TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
					indigo_send_message(device, IDLE_PROPERTY, "Tracking mode detected");
				}
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		if (!MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
			res = tc_get_side_of_pier(dev_id);
			if (res < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_side_of_pier(%d) = %d (%s)", dev_id, res, strerror(errno));
			} else {
				side_of_pier = res;
			}
		}
		if (PRIVATE_DATA->gps && PRIVATE_DATA->gps->gp_bits) {
			char response[3];
			if (tc_pass_through_cmd(dev_id, 1, 0xB0, 0x37, 0, 0, 0, 1, response) == RC_OK) {
				linked = response[0] > 0;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (!PRIVATE_DATA->guiding_in_progress) {
		if (position_valid) {
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		}
		indigo_update_coordinates(device, NULL);
		if (location_valid) {
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = lon;
				MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = lat;
			}
		}
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		if (time_valid) {
			indigo_timetoisolocal(ttime - ((tz + dst) * 3600), MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", tz + dst);
		}
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		if (!TRACKING_MODE_PROPERTY->hidden) {
			indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
		}
		if (!MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
			if (side_of_pier == 'W' && MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			} else if (side_of_pier == 'E' && MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
				indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			}
		}
		if (PRIVATE_DATA->gps && PRIVATE_DATA->gps->gp_bits) {
			indigo_device *gps_device = PRIVATE_DATA->gps;
			indigo_device *device = gps_device;
			if (linked && location_valid && time_valid) {
				if (GPS_STATUS_3D_FIX_ITEM->light.value != INDIGO_OK_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				}
				GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
				GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
				indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
				indigo_timetoisolocal(ttime - ((tz + dst) * 3600), GPS_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
				snprintf(GPS_UTC_OFFEST_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", tz + dst);
				indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			} else if (GPS_STATUS_NO_FIX_ITEM->light.value != INDIGO_ALERT_STATE) {
				GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
				GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
			}
		}
	}
}

static bool nexstar_set_location(indigo_device *device) {
	double lon = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target;
	if (lon > 180) {
		lon -= 360.0;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_set_location(PRIVATE_DATA->dev_id, lon, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res == RC_FORBIDDEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
		if (nexstar_hc_type == HC_STARSENSE) {
			indigo_send_message(device, ALERT_PROPERTY, "Can't set location to StarSense controller.");
		}
		return false;
	}
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target;
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target;
	return true;
}

static bool nexstar_set_host_time(indigo_device *device) {
	if (!MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		return true;
	}
	struct tm tm_timenow;
	time_t timenow = time(NULL);
	if (timenow == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get host time");
		return false;
	}
	localtime_r(&timenow, &tm_timenow);
	int offset = (int)tm_timenow.tm_gmtoff / 3600;
	int dst = 0;
	if (tm_timenow.tm_isdst != 0) {
		offset -= 1;
		dst = 1;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_set_time(PRIVATE_DATA->dev_id, timenow, offset, dst);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "tc_set_time: '%02d/%02d/%04d %02d:%02d:%02d %+d'", tm_timenow.tm_mday, tm_timenow.tm_mon + 1, tm_timenow.tm_year + 1900, tm_timenow.tm_hour, tm_timenow.tm_min, tm_timenow.tm_sec, offset, res);
	MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
	if (res == RC_FORBIDDEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
		if (nexstar_hc_type == HC_STARSENSE) {
			indigo_send_message(device, IDLE_PROPERTY, "Can't set time to StarSense controller.");
		}
		return false;
	}
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

static bool nexstar_set_coordinates(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int aligned = tc_check_align(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (aligned < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_check_align(%d) = %d (%s)", PRIVATE_DATA->dev_id, aligned, strerror(errno));
		return false;
	}
	if (aligned == 0) {
		indigo_send_message(device, ALERT_PROPERTY, "Mount is not aligned, please align it first.");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Mount is not aligned, please align it first.");
		return false;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value ? tc_goto_rade_p(PRIVATE_DATA->dev_id, h2d(ra), dec) : tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(ra), dec);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s(%d) = %d (%s)", MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value ? "tc_goto_rade_p" : "tc_sync_rade_p", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

static bool nexstar_get_park_axes(indigo_device *device, double *ha, double *dec) {
	char *response = PRIVATE_DATA->response;
	if (write_telescope(PRIVATE_DATA->dev_id, "z", 1) != 1 || read_telescope(PRIVATE_DATA->dev_id, response, 18) != 18 || response[8] != ',' || response[17] != '#') {
		return false;
	}
	for (int i = 0; i < 17; i++) {
		if (i != 8 && !isxdigit((unsigned char)response[i])) {
			return false;
		}
	}
	response[8] = response[17] = 0;
	*ha = strtoul(response, NULL, 16) * 24.0 / 4294967296.0 - 12;
	*dec = strtoul(response + 9, NULL, 16) * 360.0 / 4294967296.0;
	if (*dec > 180) {
		*dec -= 360;
	}
	return *dec >= -90 && *dec <= 90;
}

static bool nexstar_set_tracking(indigo_device *device) {
	int tracking_mode = TC_TRACK_OFF;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		if (TRACKING_EQ_ITEM->sw.value || (PRIVATE_DATA->capabilities & TRUE_EQ_MOUNT)) {
			tracking_mode = TC_TRACK_EQ;
		} else if (TRACKING_AA_ITEM->sw.value) {
			tracking_mode = TC_TRACK_ALT_AZ;
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Tracking mode is not set");
			return false;
		}
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_set_tracking_mode(PRIVATE_DATA->dev_id, tracking_mode);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
	return true;
}

static bool nexstar_set_st4_guiding_rate(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int offset = PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER ? 0 : 1;
	bool ok = true;
	if ((int)MOUNT_GUIDE_RATE_RA_ITEM->number.value != PRIVATE_DATA->st4_ra_rate) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = nexstar_set_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_RA, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.value - offset);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d (%s)", dev_id, res, strerror(errno));
			ok = false;
		} else {
			PRIVATE_DATA->st4_ra_rate = (int)MOUNT_GUIDE_RATE_RA_ITEM->number.value;
		}
	}
	if ((int)MOUNT_GUIDE_RATE_DEC_ITEM->number.value != PRIVATE_DATA->st4_dec_rate) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = nexstar_set_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_DE, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.value - offset);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d (%s)", dev_id, res, strerror(errno));
			ok = false;
		} else {
			PRIVATE_DATA->st4_dec_rate = (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.value;
		}
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int st4_ra_rate = nexstar_get_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_RA);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (st4_ra_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_ra_rate, strerror(errno));
		ok = false;
	} else {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int st4_dec_rate = nexstar_get_autoguide_rate(dev_id, PRIVATE_DATA->vendor_id, TC_AXIS_DE);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (st4_dec_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_dec_rate, strerror(errno));
		ok = false;
	} else {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
	}
	return ok;
}

static void nexstar_update_slew_rate(indigo_device *device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 2;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 4;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 6;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 9;
	} else {
		MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value = true;
		PRIVATE_DATA->slew_rate = 2;
	}
}

static bool nexstar_move_axis(indigo_device *device, int axis, bool positive, bool negative) {
	int res = RC_OK;
	if (PRIVATE_DATA->slew_rate == 0) {
		nexstar_update_slew_rate(device);
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (positive) {
		res = tc_slew_fixed(PRIVATE_DATA->dev_id, axis, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
	} else if (negative) {
		res = tc_slew_fixed(PRIVATE_DATA->dev_id, axis, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
	} else {
		res = tc_slew_fixed(PRIVATE_DATA->dev_id, axis, TC_DIR_POSITIVE, 0);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

static bool nexstar_set_utc(indigo_device *device) {
	time_t utc_time = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (utc_time == -1) {
		indigo_send_message(device, ALERT_PROPERTY, "Wrong date/time format!");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		return false;
	}
	int offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
	int dst = 0;
	tzset();
	if (indigo_get_dst_state() != 0) {
		offset -= 1;
		dst = 1;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_set_time(PRIVATE_DATA->dev_id, utc_time, offset, dst);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res == RC_FORBIDDEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
		if (nexstar_hc_type == HC_STARSENSE) {
			indigo_send_message(device, ALERT_PROPERTY, "Can't set time to StarSense controller.");
		}
		return false;
	}
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		indigo_send_message(device, ALERT_PROPERTY, "Failed to set date/time.");
		return false;
	}
	return true;
}

static bool nexstar_abort_motion(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_goto_cancel(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	bool ok = res == RC_OK;
	if (!ok) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_cancel(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
	}
	ok = nexstar_stop_axis(device, TC_AXIS_RA) && ok;
	ok = nexstar_stop_axis(device, TC_AXIS_DE) && ok;
	return ok;
}

static void gps_handle_connect(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		char response[3];
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = tc_pass_through_cmd(PRIVATE_DATA->dev_id, 1, 0xB0, 0xFE, 0, 0, 0, 2, response);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res == RC_OK) {
			device->gp_bits = 1;
			snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value), "%d.%d", response[0], response[1]);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			device->gp_bits = 0;
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_send_message(device, ALERT_PROPERTY, "No GPS unit detected");
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		device->gp_bits = 0;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Celestron GPS");
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 2;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_execute_handler(device, gps_handle_connect);
	}
	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_handle_connect(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

//- code

//+ mount.MOUNT_PARK.code

static void mount_park_finalizer(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value || !PRIVATE_DATA->park_in_progress) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int in_progress = tc_goto_in_progress(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (in_progress < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_in_progress(%d) = %d (%s)", PRIVATE_DATA->dev_id, in_progress, strerror(errno));
		PRIVATE_DATA->parked = false;
		PRIVATE_DATA->park_in_progress = false;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		return;
	}
	if (in_progress) {
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, REFRESH_SECONDS, mount_park_finalizer);
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int res = tc_set_tracking_mode(PRIVATE_DATA->dev_id, TC_TRACK_OFF);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res == RC_OK) {
		MOUNT_TRACKING_OFF_ITEM->sw.value = true;
		MOUNT_TRACKING_ON_ITEM->sw.value = false;
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		PRIVATE_DATA->parked = false;
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	PRIVATE_DATA->park_in_progress = false;
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

//- mount.MOUNT_PARK.code

//+ guider.code

static void guider_guide_ra_finalizer(indigo_device *device) {
	bool ok = true;
	if (CONNECTION_CONNECTED_ITEM->sw.value && !(PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE)) {
		ok = nexstar_stop_axis(device->master_device, TC_AXIS_RA);
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	PRIVATE_DATA->guiding_in_progress = GUIDER_GUIDE_DEC_PROPERTY->state == INDIGO_BUSY_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	bool ok = true;
	if (CONNECTION_CONNECTED_ITEM->sw.value && !(PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE)) {
		ok = nexstar_stop_axis(device->master_device, TC_AXIS_DE);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	PRIVATE_DATA->guiding_in_progress = GUIDER_GUIDE_RA_PROPERTY->state == INDIGO_BUSY_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static bool guider_start_ra(indigo_device *device, int *duration) {
	int res = RC_OK;
	*duration = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	if (*duration > 0) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) ? tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_EAST, PRIVATE_DATA->guide_rate * 50, *duration) : tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	} else {
		*duration = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
		if (*duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			res = (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) ? tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_WEST, PRIVATE_DATA->guide_rate * 50, *duration) : tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		}
	}
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed/tc_guide_pulse(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

static bool guider_start_dec(indigo_device *device, int *duration) {
	int res = RC_OK;
	*duration = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (*duration > 0) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) ? tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_NORTH, PRIVATE_DATA->guide_rate, *duration) : tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	} else {
		*duration = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
		if (*duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			res = (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) ? tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_SOUTH, PRIVATE_DATA->guide_rate, *duration) : tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		}
	}
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed/tc_guide_pulse(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		return false;
	}
	return true;
}

//- guider.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	nexstar_update_position(device);
	indigo_execute_handler_in(device, REFRESH_SECONDS, mount_timer_callback);
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = nexstar_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ mount.on_connect
			connection_result = nexstar_configure_mount(device);
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, TRACKING_MODE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				nexstar_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
		indigo_cancel_pending_handler(device, mount_park_finalizer);
		nexstar_detach_gps(device);
		PRIVATE_DATA->guiding_in_progress = false;
		PRIVATE_DATA->park_in_progress = false;
		//- mount.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY,
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY,
			MOUNT_SET_HOST_TIME_PROPERTY,
			MOUNT_UTC_TIME_PROPERTY,
			MOUNT_TRACKING_PROPERTY,
			TRACKING_MODE_PROPERTY,
			MOUNT_GUIDE_RATE_PROPERTY,
			MOUNT_SLEW_RATE_PROPERTY,
			MOUNT_MOTION_DEC_PROPERTY,
			MOUNT_MOTION_RA_PROPERTY,
			MOUNT_PARK_SET_PROPERTY,
			MOUNT_PARK_PROPERTY,
			MOUNT_ABORT_MOTION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, TRACKING_MODE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			nexstar_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, mount_timer_callback);
	}
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	if (nexstar_set_coordinates(device)) {
		if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = nexstar_set_location(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state == INDIGO_ALERT_STATE) {
		MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
	}
	//- mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_set_host_time_handler(indigo_device *device) {
	//+ mount.MOUNT_SET_HOST_TIME.on_change
	MOUNT_SET_HOST_TIME_PROPERTY->state = nexstar_set_host_time(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_SET_HOST_TIME.on_change
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_handler(indigo_device *device) {
	//+ mount.MOUNT_UTC_TIME.on_change
	MOUNT_UTC_TIME_PROPERTY->state = nexstar_set_utc(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_UTC_TIME.on_change
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	bool ok = nexstar_set_tracking(device);
	if (!ok) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int tracking_mode = tc_get_tracking_mode(PRIVATE_DATA->dev_id);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (tracking_mode >= 0) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, tracking_mode == TC_TRACK_OFF ? MOUNT_TRACKING_OFF_ITEM : MOUNT_TRACKING_ON_ITEM, true);
		}
	}
	MOUNT_TRACKING_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_tracking_mode_handler(indigo_device *device) {
	TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.TRACKING_MODE.on_change
	if (IS_CONNECTED && !TRACKING_MODE_PROPERTY->hidden) {
		if (TRACKING_AUTO_ITEM->sw.value) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		}
		TRACKING_MODE_PROPERTY->state = nexstar_set_tracking(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	}
	//- mount.TRACKING_MODE.on_change
	indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	//+ mount.MOUNT_GUIDE_RATE.on_change
	MOUNT_GUIDE_RATE_PROPERTY->state = nexstar_set_st4_guiding_rate(device) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void mount_slew_rate_handler(indigo_device *device) {
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SLEW_RATE.on_change
	nexstar_update_slew_rate(device);
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//- mount.MOUNT_SLEW_RATE.on_change
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
		return;
	}
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_DEC.on_change
	if (nexstar_move_axis(device, TC_AXIS_DE, MOUNT_MOTION_NORTH_ITEM->sw.value, MOUNT_MOTION_SOUTH_ITEM->sw.value)) {
		MOUNT_MOTION_DEC_PROPERTY->state = (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_DEC_PROPERTY);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
		return;
	}
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MOTION_RA.on_change
	if (nexstar_move_axis(device, TC_AXIS_RA, MOUNT_MOTION_EAST_ITEM->sw.value, MOUNT_MOTION_WEST_ITEM->sw.value)) {
		MOUNT_MOTION_RA_PROPERTY->state = (MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	} else {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_mount_commit_motion_client(device, MOUNT_MOTION_RA_PROPERTY);
}

static void mount_park_set_handler(indigo_device *device) {
	MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK_SET.on_change
	bool update_position = false;
	if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
		double ha, dec;
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		bool valid = nexstar_get_park_axes(device, &ha, &dec);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (valid) {
			MOUNT_PARK_POSITION_HA_ITEM->number.value = MOUNT_PARK_POSITION_HA_ITEM->number.target = ha;
			MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_PARK_POSITION_DEC_ITEM->number.target = dec;
			update_position = true;
		} else {
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_PARK_SET_DEFAULT_ITEM->sw.value) {
		MOUNT_PARK_POSITION_HA_ITEM->number.value = MOUNT_PARK_POSITION_HA_ITEM->number.target = 6;
		MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_PARK_POSITION_DEC_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value > 0 ? 90 : -90;
		update_position = true;
	}
	MOUNT_PARK_SET_CURRENT_ITEM->sw.value = MOUNT_PARK_SET_DEFAULT_ITEM->sw.value = false;
	if (update_position) {
		MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
	}
	//- mount.MOUNT_PARK_SET.on_change
	indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		PRIVATE_DATA->parked = true;
		PRIVATE_DATA->park_in_progress = true;
		double dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
		double ha = (MOUNT_PARK_POSITION_HA_ITEM->number.value + 12) * 15;
		if (ha < 0) {
			ha += 360.0;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Going to park position: HA = %.5f Dec = %.5f", ha, dec);
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = tc_goto_azalt_p(PRIVATE_DATA->dev_id, ha, dec);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res == RC_OK) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_execute_handler_in(device, 2, mount_park_finalizer);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_azalt_p(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->park_in_progress = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		}
	} else {
		PRIVATE_DATA->parked = false;
		PRIVATE_DATA->park_in_progress = false;
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	}
	//- mount.MOUNT_PARK.on_change
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, mount_equatorial_coordinates_handler);
	indigo_cancel_pending_handler(device, mount_motion_ra_handler);
	indigo_cancel_pending_handler(device, mount_motion_dec_handler);
	indigo_cancel_pending_handler(device, mount_park_handler);
	indigo_cancel_pending_handler(device, mount_park_finalizer);
	bool ok = nexstar_abort_motion(device);
	PRIVATE_DATA->park_in_progress = false;
	if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->parked = false;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	MOUNT_MOTION_WEST_ITEM->sw.value = MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		MOUNT_ABORT_MOTION_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_mount_commit_motion_client(device, MOUNT_ABORT_MOTION_PROPERTY);
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
		nexstar_initialize_private_data(device);
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		strncpy(MOUNT_GUIDE_RATE_PROPERTY->label, "ST4 guide rate", INDIGO_VALUE_SIZE);
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		//- mount.on_attach
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		TRACKING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, TRACKING_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Tracking mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (TRACKING_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(TRACKING_EQ_ITEM, TRACKING_EQ_ITEM_NAME, "EQ mode", false);
		indigo_init_switch_item(TRACKING_AA_ITEM, TRACKING_AA_ITEM_NAME, "Alt/Az mode", false);
		indigo_init_switch_item(TRACKING_AUTO_ITEM, TRACKING_AUTO_ITEM_NAME, "Automatic mode", true);
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(TRACKING_MODE_PROPERTY);
	}
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(TRACKING_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(TRACKING_MODE_PROPERTY, mount_tracking_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SLEW_RATE_PROPERTY, mount_slew_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		indigo_mount_record_motion_client(device, client, property);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_SET_PROPERTY, mount_park_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, TRACKING_MODE_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(TRACKING_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = nexstar_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ guider.on_connect
			if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
				INDIGO_COPY_VALUE(GUIDE_50_ITEM->label, "HC fixed rate 1 (nominal 1x sidereal)");
				INDIGO_COPY_VALUE(GUIDE_100_ITEM->label, "HC fixed rate 2 (nominal 8x sidereal)");
			} else if (NEXSTAR_MODERN_EQ_MODEL(PRIVATE_DATA->model_id)) {
				INDIGO_COPY_VALUE(GUIDE_50_ITEM->label, "HC fixed rate 1 (nominal 2x sidereal)");
				INDIGO_COPY_VALUE(GUIDE_100_ITEM->label, "HC fixed rate 2 (nominal 4x sidereal)");
			} else {
				INDIGO_COPY_VALUE(GUIDE_50_ITEM->label, "50% sidereal");
				INDIGO_COPY_VALUE(GUIDE_100_ITEM->label, "100% sidereal");
			}
			//- guider.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				nexstar_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
		indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
		PRIVATE_DATA->guiding_in_progress = false;
		//- guider.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			GUIDER_GUIDE_RA_PROPERTY,
			GUIDER_GUIDE_DEC_PROPERTY,
			COMMAND_GUIDE_RATE_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			nexstar_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	int duration = 0;
	if (guider_start_ra(device, &duration)) {
		if (duration > 0) {
			PRIVATE_DATA->guiding_in_progress = true;
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_ra_finalizer);
		} else {
			guider_guide_ra_finalizer(device);
		}
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	int duration = 0;
	if (guider_start_dec(device, &duration)) {
		if (duration > 0) {
			PRIVATE_DATA->guiding_in_progress = true;
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_dec_finalizer);
		} else {
			guider_guide_dec_finalizer(device);
		}
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_command_guide_rate_handler(indigo_device *device) {
	COMMAND_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.COMMAND_GUIDE_RATE.on_change
	if (GUIDE_50_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 1;
	} else if (GUIDE_100_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 2;
	}
	COMMAND_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER && PRIVATE_DATA->guide_rate == 1) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Fixed HC rate 1 selected (manual nominal 1x sidereal; physical correction unverified).");
	} else if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER && PRIVATE_DATA->guide_rate == 2) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Fixed HC rate 2 selected (manual nominal 8x sidereal; physical correction unverified).");
	} else if (NEXSTAR_MODERN_EQ_MODEL(PRIVATE_DATA->model_id) && PRIVATE_DATA->guide_rate == 1) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Fixed HC rate 1 selected (manual nominal 2x sidereal; physical correction unverified).");
	} else if (NEXSTAR_MODERN_EQ_MODEL(PRIVATE_DATA->model_id) && PRIVATE_DATA->guide_rate == 2) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Fixed HC rate 2 selected (manual nominal 4x sidereal; physical correction unverified).");
	} else if (PRIVATE_DATA->guide_rate == 1) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 7.5\"/s (1/2 sidereal).");
	} else if (PRIVATE_DATA->guide_rate == 2) {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 15\"/s (sidereal).");
	} else {
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set.");
	}
	//- guider.COMMAND_GUIDE_RATE.on_change
	indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		nexstar_initialize_private_data(device->master_device);
		PRIVATE_DATA->guide_rate = 1;
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		COMMAND_GUIDE_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (COMMAND_GUIDE_RATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(GUIDE_50_ITEM, GUIDE_50_ITEM_NAME, "50% sidereal", true);
		indigo_init_switch_item(GUIDE_100_ITEM, GUIDE_100_ITEM_NAME, "100% sidereal", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(COMMAND_GUIDE_RATE_PROPERTY);
	}
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(COMMAND_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(COMMAND_GUIDE_RATE_PROPERTY, guider_command_guide_rate_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	indigo_release_property(COMMAND_GUIDE_RATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_nexstar(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nexstar_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "USB-Serial Controller D");
			strcpy(patterns[0].vendor_string, "Prolific");
			INDIGO_REGISER_MATCH_PATTERNS(mount_template, patterns, 1);
			private_data = (nexstar_private_data *)indigo_safe_malloc(sizeof(nexstar_private_data));
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
