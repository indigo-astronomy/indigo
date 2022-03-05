// Copyright (c) 2016 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO MOUNT Nexstar (celestron & skywatcher) driver
 \file indigo_mount_nexstar.c
 */

#define DRIVER_VERSION 0x0015
#define DRIVER_NAME	"indigo_mount_nexstar"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_novas.h>

#include "indigo_mount_nexstar.h"
#include "nexstar.h"

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (0.5)

#define PRIVATE_DATA        ((nexstar_private_data *)device->private_data)


#define COMMAND_GUIDE_RATE_PROPERTY     (PRIVATE_DATA->command_guide_rate_property)
#define GUIDE_50_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items+0)
#define GUIDE_100_ITEM                  (COMMAND_GUIDE_RATE_PROPERTY->items+1)

#define COMMAND_GUIDE_RATE_PROPERTY_NAME   "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME                 "GUIDE_50"
#define GUIDE_100_ITEM_NAME                "GUIDE_100"

#define TRACKING_MODE_PROPERTY     			(PRIVATE_DATA->tracking_mode_property)
#define TRACKING_EQ_ITEM                (TRACKING_MODE_PROPERTY->items+0)
#define TRACKING_AA_ITEM                (TRACKING_MODE_PROPERTY->items+1)
#define TRACKING_AUTO_ITEM              (TRACKING_MODE_PROPERTY->items+2)

#define TRACKING_MODE_PROPERTY_NAME   	"TRACKING_MODE"
#define TRACKING_EQ_ITEM_NAME           "EQ"
#define TRACKING_AA_ITEM_NAME           "AA"
#define TRACKING_AUTO_ITEM_NAME         "AUTO"



#define WARN_PARKED_MSG                    "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG          "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	int dev_id;
	bool parked;
	bool park_in_progress;
	char tty_name[INDIGO_VALUE_SIZE];
	int count_open;
	int slew_rate;
	int st4_ra_rate, st4_dec_rate;
	int vendor_id;
	uint32_t capabilities;
	pthread_mutex_t serial_mutex;
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec, *park_timer;
	int guide_rate;
	indigo_property *command_guide_rate_property;
	indigo_property *tracking_mode_property;
	indigo_device *gps;
} nexstar_private_data;

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result gps_attach(indigo_device *device);
static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result gps_detach(indigo_device *device);

static bool mount_open(indigo_device *device) {
	if (device->is_connected)
		return false;
	if (PRIVATE_DATA->count_open++ == 0) {
		int dev_id = open_telescope(DEVICE_PORT_ITEM->text.value);
		if (dev_id == -1) {
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "open_telescope(%s) = %d (%s)", DEVICE_PORT_ITEM->text.value, dev_id, strerror(errno));
			PRIVATE_DATA->count_open--;
			return false;
		} else {
			PRIVATE_DATA->dev_id = dev_id;
			PRIVATE_DATA->vendor_id = guess_mount_vendor(dev_id);
			int res = get_mount_capabilities(dev_id, &PRIVATE_DATA->capabilities, &PRIVATE_DATA->vendor_id);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "get_mount_capabilities(%d) = %d", dev_id, res);
			}
			PRIVATE_DATA->capabilities &= ~(CAN_PULSE_GUIDE); // do not pulse guide natively
		}
	}
	return true;
}

static void mount_close(indigo_device *device) {
	if (!device->is_connected)
		return;
	if (--PRIVATE_DATA->count_open == 0) {
		close_telescope(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
	}
}

static void position_timer_callback(indigo_device *device) {
	int res;
	double ra, dec, lon, lat;
	char side_of_pier = 0;
	int dev_id = PRIVATE_DATA->dev_id;
	if (dev_id < 0)
		return;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (
		tc_goto_in_progress(dev_id) ||
		MOUNT_MOTION_NORTH_ITEM->sw.value ||
		MOUNT_MOTION_SOUTH_ITEM->sw.value ||
		MOUNT_MOTION_EAST_ITEM->sw.value ||
		MOUNT_MOTION_WEST_ITEM->sw.value
	) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	res = tc_get_rade_p(dev_id, &ra, &dec);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_rade_p(%d) = %d (%s)", dev_id, res, strerror(errno));
	}
	res = tc_get_location(dev_id, &lon, &lat);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_location(%d) = %d (%s)", dev_id, res, strerror(errno));
	}
	if (lon < 0)
		lon += 360;
	time_t ttime;
	int tz, dst;
	res = (int)tc_get_time(dev_id, &ttime, &tz, &dst);
	if (res == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_time(%d) = %d (%s)", dev_id, res, strerror(errno));
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
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
				indigo_send_message(device, "Tracking mode detected");
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
	bool linked = false;
	if (PRIVATE_DATA->gps) {
		nexstar_private_data *private_data = PRIVATE_DATA;
		indigo_device *device = private_data->gps;
		if (IS_CONNECTED) {
			char response[3];
			if (tc_pass_through_cmd(dev_id, 1, 0xB0, 0x37, 0, 0, 0, 1, response) == RC_OK) {
				linked = response[0] > 0;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = d2h(ra);
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	indigo_update_coordinates(device, NULL);
	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	indigo_timetoisolocal(ttime - ((tz + dst) * 3600), MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
	snprintf(MOUNT_UTC_OFFSET_ITEM->text.value, INDIGO_VALUE_SIZE, "%d", tz + dst);
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	if (!TRACKING_MODE_PROPERTY->hidden)
		indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
	if (!MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
		if (side_of_pier == 'W' && MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		} else if (side_of_pier == 'E' && MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value) {
			indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		}
	}
	if (PRIVATE_DATA->gps) {
		nexstar_private_data *private_data = PRIVATE_DATA;
		indigo_device *device = private_data->gps;
		if (IS_CONNECTED) {
			if (linked) {
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
			} else {
				if (GPS_STATUS_NO_FIX_ITEM->light.value != INDIGO_ALERT_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				}
			}
		}
	}
	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->position_timer);
}

static void park_timer_callback(indigo_device *device) {
	int res;
	int dev_id = PRIVATE_DATA->dev_id;
	if (dev_id < 0)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (tc_goto_in_progress(dev_id)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->park_in_progress = true;
	} else {
		res = tc_set_tracking_mode(dev_id, TC_TRACK_OFF);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d (%s)", dev_id, res, strerror(errno));
		} else {
			MOUNT_TRACKING_OFF_ITEM->sw.value = true;
			MOUNT_TRACKING_ON_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_in_progress = false;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (PRIVATE_DATA->park_in_progress) {
		indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->park_timer);
	} else {
		PRIVATE_DATA->park_timer = NULL;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount Parked.");
	}
}

static void mount_handle_connect(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (mount_open(device)) {
				int dev_id = PRIVATE_DATA->dev_id;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				/* initialize info prop */
				if (PRIVATE_DATA->vendor_id < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "guess_mount_vendor(%d) = %d (%s)", dev_id, PRIVATE_DATA->vendor_id, strerror(errno));
				} else if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
					indigo_copy_value(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher");
				} else if (PRIVATE_DATA->vendor_id == VNDR_CELESTRON) {
					indigo_copy_value(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron");
				}
				int model_id = tc_get_model(dev_id);
				if (model_id < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_model(%d) = %d (%s)", dev_id, model_id, strerror(errno));
				} else {
					get_model_name(model_id, MOUNT_INFO_MODEL_ITEM->text.value,  INDIGO_VALUE_SIZE);
				}
				if (enforce_protocol_version(dev_id, VER_AUTO) < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_version(%d) = %d (%s)", dev_id, nexstar_proto_version, strerror(errno));
				} else {
					if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) {
						snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "SynScan %2d.%02d.%02d", GET_RELEASE(nexstar_proto_version), GET_REVISION(nexstar_proto_version), GET_PATCH(nexstar_proto_version));
					} else {
						snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%s %2d.%02d", nexstar_hc_type == HC_STARSENSE ? "StarSense" : "NexStar", GET_RELEASE(nexstar_proto_version), GET_REVISION(nexstar_proto_version));
					}
				}
				/* initialize guidingrate prop */
				if (PRIVATE_DATA->capabilities & CAN_GET_SET_GUIDE_RATE) {
					MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
					int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
					if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER)
						offset = 0; /* there is no offset for Sky-Watcher */
					int st4_ra_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
					if (st4_ra_rate < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_ra_rate, strerror(errno));
						MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
					} else {
						MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
						PRIVATE_DATA->st4_ra_rate = st4_ra_rate + offset;
						int st4_dec_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
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
				/* initialize tracking prop */
				if (PRIVATE_DATA->capabilities & TRUE_EQ_MOUNT) {
					TRACKING_MODE_PROPERTY->hidden = true;
					indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_EQ_ITEM, true);
				}
				TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
				int mode = tc_get_tracking_mode(dev_id);
				if (mode < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_tracking_mode(%d) = %d (%s)", dev_id, mode, strerror(errno));
					MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				} else if (mode == TC_TRACK_OFF) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					if (TRACKING_AUTO_ITEM->sw.value) {
						TRACKING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_send_message(device, "Tracking mode can't be detected");
					}
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					if (TRACKING_AUTO_ITEM->sw.value) {
						if (mode == TC_TRACK_ALT_AZ) {
							indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_AA_ITEM, true);
						} else {
							indigo_set_switch(TRACKING_MODE_PROPERTY, TRACKING_EQ_ITEM, true);
						}
						indigo_send_message(device, "Tracking mode detected");
					}
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				}
				PRIVATE_DATA->parked = false;
				PRIVATE_DATA->park_in_progress = false;
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				/* check for side of pier support & GPS */
				MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
				MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
				if (PRIVATE_DATA->capabilities & CAN_GET_SIDE_OF_PIER) {
					int side_of_pier = tc_get_side_of_pier(dev_id);
					if (side_of_pier < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_side_of_pier(%d) = %d (%s)", dev_id, side_of_pier, strerror(errno));
						MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
					} else {
						if (side_of_pier == 'W') {
							MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
							indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
						} else if (side_of_pier == 'E') {
							MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
							indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
						}
					}
				}
				if (PRIVATE_DATA->vendor_id == VNDR_CELESTRON) {
//					char response[3];
//					if (tc_pass_through_cmd(dev_id, 1, 0xB0, 0xFE, 0, 0, 0, 2, response) == RC_OK) {
//						sprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value + strlen(MOUNT_INFO_FIRMWARE_ITEM->text.value), " (GPS %d.%d)", response[0], response[1]);
						static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(
							MOUNT_NEXSTAR_GPS_NAME,
							gps_attach,
							indigo_gps_enumerate_properties,
							gps_change_property,
							NULL,
							gps_detach
						);
						PRIVATE_DATA->gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
						PRIVATE_DATA->gps->private_data = PRIVATE_DATA;
						indigo_attach_device(PRIVATE_DATA->gps);
//					}
				}
				device->is_connected = true;
				/* start updates */
				indigo_define_property(device, TRACKING_MODE_PROPERTY, NULL);
				indigo_set_timer(device, 0, position_timer_callback, &PRIVATE_DATA->position_timer);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
			if (PRIVATE_DATA->gps) {
				indigo_detach_device(PRIVATE_DATA->gps);
				free(PRIVATE_DATA->gps);
				PRIVATE_DATA->gps = NULL;
			}
			indigo_delete_property(device, TRACKING_MODE_PROPERTY, NULL);
			mount_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_handle_park(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		PRIVATE_DATA->parked = true;  /* a bit premature but need to cancel other movements from now on until unparked */
		PRIVATE_DATA->park_in_progress = true;
		/* Celestron and SkyWatcher do not go to real ALT and AZ on EQ mounts,
			 they infact go to HA and DEC. So we can use tc_goto_azalt_p() directly
		*/
		double dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
		double ha =  MOUNT_PARK_POSITION_HA_ITEM->number.value * 15;
		if (ha < 0)
			ha += 360.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Going to park position: HA = %.5f Dec = %.5f", ha, dec);
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = tc_goto_azalt_p(PRIVATE_DATA->dev_id, ha, dec);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_azalt_p(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->park_in_progress = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Failed to park");
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
			indigo_set_timer(device, 2, park_timer_callback, &PRIVATE_DATA->park_timer);
		}
	} else {
		PRIVATE_DATA->parked = false;
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount unparked");
	}
}

static void mount_handle_geo_coordinates(indigo_device *device) {
	int res;
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	double lon = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	if (lon > 180)
		lon -= 360.0;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_set_location(PRIVATE_DATA->dev_id, lon, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res == RC_FORBIDDEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
		if (nexstar_hc_type == HC_STARSENSE)
			indigo_send_message(device, "Can't set location to StarSense controller.");
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_handle_set_utc_from_host(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		struct tm tm_timenow;
		time_t timenow = time(NULL);
		if (timenow == -1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get host time");
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			localtime_r(&timenow, &tm_timenow);
			/* tm_gmtoff is is in seconds and is corrected for tm_isdst */
			int offset = (int)tm_timenow.tm_gmtoff/3600;
			int dst = 0;
			/* daylight is set by localtime_r() call */
			if (daylight != 0) {
				offset -=1;
				dst = 1;
			}
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			/* set mount time to local time and UTC offset */
			int res = tc_set_time(PRIVATE_DATA->dev_id, timenow, offset, dst);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "tc_set_time: '%02d/%02d/%04d %02d:%02d:%02d %+d'", tm_timenow.tm_mday, tm_timenow.tm_mon+1, tm_timenow.tm_year+1900, tm_timenow.tm_hour, tm_timenow.tm_min, tm_timenow.tm_sec, offset, res);
			if (res == RC_FORBIDDEN) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				if (nexstar_hc_type == HC_STARSENSE)
					indigo_send_message(device, "Can't set time to StarSense controller.");
			} else if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
	}
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_handle_eq_coordinates(indigo_device *device) {
	int res = RC_OK;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int aligned = tc_check_align(PRIVATE_DATA->dev_id);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (aligned < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_check_align(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
	} else if (aligned == 0) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is not aligned, please align it first.");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Mount is not aligned, please align it first.");
		return;
	}
	/* GOTO requested */
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = tc_goto_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_rade_p(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		}
	}
	/* SYNC requested */
	else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_sync_rade_p(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		}
	}
	indigo_update_coordinates(device, NULL);
}

static void mount_handle_tracking(indigo_device *device) {
	int res = RC_OK;
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	TRACKING_MODE_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		int tracking_mode = 0;
		if (TRACKING_EQ_ITEM->sw.value || PRIVATE_DATA->capabilities & TRUE_EQ_MOUNT)
			tracking_mode = TC_TRACK_EQ;
		else if (TRACKING_AA_ITEM->sw.value)
			tracking_mode = TC_TRACK_ALT_AZ;
		if (tracking_mode) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			res = tc_set_tracking_mode(PRIVATE_DATA->dev_id, tracking_mode);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				TRACKING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d (%s)", PRIVATE_DATA->dev_id, tracking_mode, res, strerror(errno));
			}
		} else {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			TRACKING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Tracking mode is not set");
		}
	} else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = tc_set_tracking_mode(PRIVATE_DATA->dev_id, TC_TRACK_OFF);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			TRACKING_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		}
	}
	indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_handle_st4_guiding_rate(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;
	int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
	if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER)
		offset = 0; /* there is no offset for Sky-Watcher */
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value) != PRIVATE_DATA->st4_ra_rate) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = tc_set_autoguide_rate(dev_id, TC_AXIS_RA, (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value)-1);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d (%s)", dev_id, res, strerror(errno));
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_ra_rate = (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value);
		}
	}
	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value) != PRIVATE_DATA->st4_dec_rate) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		res = tc_set_autoguide_rate(dev_id, TC_AXIS_DE, (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value)-1);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d (%s)", dev_id, res, strerror(errno));
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_dec_rate = (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value);
		}
	}
	/* read set values as Sky-Watcher rounds to 12, 25 ,50, 75 and 100 % */
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int st4_ra_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (st4_ra_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_ra_rate, strerror(errno));
	} else {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	int st4_dec_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (st4_dec_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d (%s)", dev_id, st4_dec_rate, strerror(errno));
	} else {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
	}
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void mount_handle_slew_rate(indigo_device *device) {
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
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}

static void mount_handle_motion_ns(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;
	if (PRIVATE_DATA->slew_rate == 0)
		mount_handle_slew_rate(device);
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", dev_id, res, strerror(errno));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_handle_motion_ne(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;
	if (PRIVATE_DATA->slew_rate == 0)
		mount_handle_slew_rate(device);
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if(MOUNT_MOTION_EAST_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", dev_id, res, strerror(errno));
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_handle_utc(indigo_device *device) {
	time_t utc_time = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		return;
	}
	int offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
	int dst = 0;
	tzset(); /* make sure daylight is set */
	if (daylight != 0) {
		offset -=1;
		dst = 1;
	}
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	/* set mount time to local time */
	int res = tc_set_time(PRIVATE_DATA->dev_id, utc_time, offset, dst);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res == RC_FORBIDDEN) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = RC_FORBIDDEN", PRIVATE_DATA->dev_id);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		if (nexstar_hc_type == HC_STARSENSE)
			indigo_send_message(device, "Can't set time to StarSense controller.");
	} else if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Can't set mount date/time.");
	} else {
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	return;
}

static void mount_cancel_slew(indigo_device *device) {
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = tc_goto_cancel(PRIVATE_DATA->dev_id);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_cancel(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		}
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
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	}
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- TRACKING_MODE
		TRACKING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, TRACKING_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Tracking mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (TRACKING_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(TRACKING_EQ_ITEM, TRACKING_EQ_ITEM_NAME, "EQ mode", false);
		indigo_init_switch_item(TRACKING_AA_ITEM, TRACKING_AA_ITEM_NAME, "Alt/Az mode", false);
		indigo_init_switch_item(TRACKING_AUTO_ITEM, TRACKING_AUTO_ITEM_NAME, "Automatic mode", true);
		// --------------------------------------------------------------------------------
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->count = 2; // we can not set elevation from the protocol
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		strncpy(MOUNT_GUIDE_RATE_PROPERTY->label,"ST4 guide rate", INDIGO_VALUE_SIZE);
		MOUNT_TRACK_RATE_PROPERTY->hidden = true;
		MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, mount_handle_connect, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		if (PRIVATE_DATA->park_in_progress) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, WARN_PARKING_PROGRESS_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_park, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPTHIC_COORDINATES
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
			if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
				MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_handle_geo_coordinates, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_handle_set_utc_from_host, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (PRIVATE_DATA->parked) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_eq_coordinates, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_utc, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (PRIVATE_DATA->parked) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_tracking, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(TRACKING_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- TRACKING_MODE
		if (IS_CONNECTED && !TRACKING_MODE_PROPERTY->hidden) { // TBD - I don't understand how it works!!?!?
			indigo_property_copy_values(TRACKING_MODE_PROPERTY, property, false);
			if (TRACKING_AUTO_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			}
			TRACKING_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, TRACKING_MODE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_handle_tracking, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_handle_st4_guiding_rate, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_set_timer(device, 0, mount_handle_slew_rate, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_motion_ns, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		if(PRIVATE_DATA->parked) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_handle_motion_ne, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_cancel_slew, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, TRACKING_MODE_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_handle_connect(device);
	}
	indigo_release_property(TRACKING_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_handle_ra(indigo_device *device) {
	int res = RC_OK;
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	if (duration > 0) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
			res = tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_EAST, PRIVATE_DATA->guide_rate * 50, duration);
		} else {
			res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	} else {
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
				res = tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_WEST, PRIVATE_DATA->guide_rate * 50, duration);
			} else {
				res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		}
	}

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed/tc_guide_pulse(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		if (duration > 0) {
			indigo_usleep(duration * 1000);
			if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
				int status;
				do {
					pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
					status = tc_get_guide_status(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_EAST);
					pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
					if (status > 0) {
						indigo_usleep(10000); /* celestron can do 1/100s */
					} else if (status < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_guide_status(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
						GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
				} while (status > 0);
			} else {
				pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
				res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // stop
				//res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, 0);
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				if (res != RC_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
					GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
		}
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_handle_dec(indigo_device *device) {
	int res = RC_OK;
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	if (duration > 0) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
			res = tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_NORTH, PRIVATE_DATA->guide_rate, duration);
		} else {
			res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	} else {
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
				res = tc_guide_pulse(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_SOUTH, PRIVATE_DATA->guide_rate, duration);
			} else {
				res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		}
	}

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_guide_pulse(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		if (duration > 0) {
			int status;
			indigo_usleep(duration * 1000);
			if (PRIVATE_DATA->capabilities & CAN_PULSE_GUIDE) {
				do {
					pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
					status = tc_get_guide_status(PRIVATE_DATA->dev_id, TC_AUX_GUIDE_NORTH);
					pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
					if (status > 0) {
						indigo_usleep(10000);
					} else if (status < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_guide_status(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
						GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
						break;
					}
				} while (status > 0);
			} else {
				pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
				res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
				//res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, 0);
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				if (res != RC_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d (%s)", PRIVATE_DATA->dev_id, res, strerror(errno));
					GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
		}
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_handle_connect(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (mount_open(device)) {
				device->is_connected = true;
				indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
				PRIVATE_DATA->guider_timer_ra = NULL;
				PRIVATE_DATA->guider_timer_dec = NULL;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
			mount_close(device);
			indigo_delete_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result nexstar_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property))
			indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		PRIVATE_DATA->guide_rate = 1; /* 1 -> 0.5 siderial rate , 2 -> siderial rate */
		COMMAND_GUIDE_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (COMMAND_GUIDE_RATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(GUIDE_50_ITEM, GUIDE_50_ITEM_NAME, "50% sidereal", true);
		indigo_init_switch_item(GUIDE_100_ITEM, GUIDE_100_ITEM_NAME, "100% sidereal", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
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
		indigo_set_timer(device, 0, guider_handle_connect, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		if (GUIDER_GUIDE_DEC_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_handle_dec, &PRIVATE_DATA->guider_timer_dec);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		if (GUIDER_GUIDE_RA_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_handle_ra, &PRIVATE_DATA->guider_timer_ra);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COMMAND_GUIDE_RATE
		indigo_property_copy_values(COMMAND_GUIDE_RATE_PROPERTY, property, false);
		if (GUIDE_50_ITEM->sw.value) {
			PRIVATE_DATA->guide_rate = 1;
		} else if (GUIDE_100_ITEM->sw.value) {
			PRIVATE_DATA->guide_rate = 2;
		}
		COMMAND_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		if (PRIVATE_DATA->guide_rate == 1)
			indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 7.5\"/s (1/2 sidereal).");
		else if (PRIVATE_DATA->guide_rate == 2)
			indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 15\"/s (sidereal).");
		else
			indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set.");
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_handle_connect(device);
	}
	indigo_release_property(COMMAND_GUIDE_RATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO gps device implementation

static void gps_handle_connect(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		char response[3];
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		int res = tc_pass_through_cmd(PRIVATE_DATA->dev_id, 1, 0xB0, 0xFE, 0, 0, 0, 2, response);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		if (res == RC_OK) {
			sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%d.%d", response[0], response[1]);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_send_message(device, "No GPS unit detected");
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Celestron GPS");
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
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, gps_handle_connect, NULL);
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
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static nexstar_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_nexstar(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_NEXSTAR_NAME,
		mount_attach,
		indigo_mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_NEXSTAR_GUIDER_NAME,
		guider_attach,
		nexstar_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Nexstar Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	//INDIGO_DEBUG(tc_debug = indigo_debug);

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = indigo_safe_malloc(sizeof(nexstar_private_data));
		private_data->dev_id = -1;
		private_data->count_open = 0;
		mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
		mount->private_data = private_data;
		indigo_attach_device(mount);
		mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
		mount_guider->private_data = private_data;
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
