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

/** INDIGO GPS Simulator driver
 \file indigo_gps_simulator.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"idnigo_gps_simulator"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"

#include "indigo_gps_simulator.h"

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (0.5)

#define PRIVATE_DATA        ((simulator_private_data *)device->private_data)


#define COMMAND_GUIDE_RATE_PROPERTY     (PRIVATE_DATA->command_guide_rate_property)
#define GUIDE_50_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items+0)
#define GUIDE_100_ITEM                  (COMMAND_GUIDE_RATE_PROPERTY->items+1)

#define COMMAND_GUIDE_RATE_PROPERTY_NAME   "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME                 "GUIDE_50"
#define GUIDE_100_ITEM_NAME                "GUIDE_100"

#define WARN_PARKED_MSG                    "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG          "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	int count_open;
	indigo_timer *gps_timer;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO GPS device implementation

static bool gps_open(indigo_device *device) {
	if (device->is_connected) return false;
	if (PRIVATE_DATA->count_open++ == 0) { }
	return true;
}


static bool gps_set_utc_from_host(indigo_device *device) {
	time_t utc_time = indigo_utc(NULL);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get host UT");
		return false;
	}
	return true;
}


static void gps_close(indigo_device *device) {
	if (!device->is_connected) return;
	if (--PRIVATE_DATA->count_open == 0) {
	}
}


static void gps_timer_callback(indigo_device *device) {
	int res;
	double ra, dec, lon, lat;

	GPS_EQUATORIAL_COORDINATES_RA_ITEM->number.value = d2h(ra);
	GPS_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	indigo_update_property(device, GPS_EQUATORIAL_COORDINATES_PROPERTY, NULL);

	GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
	GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
	indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);

	indigo_timetoiso(ttime - 3600 * (tz + dst), GPS_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
	indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);

	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->gps_timer);
}


static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------

		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 2; // we can not set elevation from the protocol
		GPS_UTC_TIME_PROPERTY->hidden = false;

		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
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
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (gps_open(device)) {
					int dev_id = PRIVATE_DATA->dev_id;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

					/* initialize info prop */
					strncpy(GPS_INFO_VENDOR_ITEM->text.value, "GPS Simulator", INDIGO_VALUE_SIZE);
					strncpy(GPS_INFO_MODEL_ITEM->text.value, "SPS Simularor", INDIGO_VALUE_SIZE);
					snprintf(GPS_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d", 0, 0);

					device->is_connected = true;
					/* start updates */
					PRIVATE_DATA->gps_timer = indigo_set_timer(device, 0, gps_timer_callback);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->ut_timer);
				gps_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_GEOGRAPTHIC_COORDINATES
		indigo_property_copy_values(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		if (gps_set_location(device)) {
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GPS_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(GPS_SET_HOST_TIME_PROPERTY, property, false);
		if(GPS_SET_HOST_TIME_ITEM->sw.value) {
			if(gps_set_utc_from_host(device)) {
				GPS_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				GPS_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		GPS_SET_HOST_TIME_ITEM->sw.value = false;
		GPS_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GPS_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GPS_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_UTC_TIME_PROPERTY
		indigo_property_copy_values(GPS_UTC_TIME_PROPERTY, property, false);
		gps_handle_utc(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_gps_change_property(device, client, property);
}


static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
	return indigo_gps_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;

static indigo_device *gps = NULL;
static indigo_device *gps_guider = NULL;

indigo_result indigo_gps_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device gps_template = {
		GPS_NEXSTAR_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		gps_attach,
		indigo_gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
	};
	static indigo_device gps_guider_template = {
		GPS_NEXSTAR_GUIDER_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		simulator_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "GPS Simulator", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(simulator_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(simulator_private_data));
		private_data->dev_id = -1;
		private_data->count_open = 0;
		gps = malloc(sizeof(indigo_device));
		assert(gps != NULL);
		memcpy(gps, &gps_template, sizeof(indigo_device));
		gps->private_data = private_data;
		indigo_attach_device(gps);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		if (gps != NULL) {
			indigo_detach_device(gps);
			free(gps);
			gps = NULL;
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
