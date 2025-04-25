// Copyright (c) 2025 Rumen G. Bogdanovski
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_gps_simulator.driver

// version history
// 3.0 Rumen G. Bogdanovski

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_gps_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_gps_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0009
#define DRIVER_NAME          "indigo_gps_simulator"
#define DRIVER_LABEL         "GPS Simulator"
#define GPS_DEVICE_NAME      "GPS Simulator"
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)
#define SIM_LONGITUDE        22.675
#define SIM_LATITUDE         43.625
#define SIM_ELEVATION        650
#define SIM_SV_IN_USE        3
#define SIM_SV_IN_VIEW       7
#define SIM_PDOP             2
#define SIM_HDOP             3
#define SIM_VDOP             3
#define REFRESH_SECONDS      1.0
#define TICKS_TO_2D_FIX      10
#define TICKS_TO_3D_FIX      20

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;

	// Custom code below

	int timer_ticks;

	// Custom code above

	indigo_timer *gps_timer;
} simulator_private_data;

#pragma mark - Low level code

static bool simulator_open(indigo_device *device) {
	return true;
}

static void simulator_close(indigo_device *device) {
}

#pragma mark - High level code (gps)

// gps state checking timer callback

static void gps_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);

	// Custom code below

	if (PRIVATE_DATA->timer_ticks >= TICKS_TO_2D_FIX) {
		GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = SIM_LONGITUDE + rand() / ((double)(RAND_MAX) * 1000);
		GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = SIM_LATITUDE + rand() / ((double)(RAND_MAX) * 1000);
		GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = (int)(SIM_ELEVATION + 0.5 + (double)(rand()) / RAND_MAX);
		GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM->number.value = (int)(5 + 0.5 + (double)(rand()) / RAND_MAX);
		time_t ttime = time(NULL);
		indigo_timetoisogm(ttime, GPS_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = (int)(SIM_SV_IN_USE + 0.5 + (double)(rand()) / RAND_MAX);
		GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = (int)(SIM_SV_IN_VIEW + 0.5 + (double)(rand()) / RAND_MAX);
		GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = (SIM_PDOP + (double)(rand()) / RAND_MAX);
		GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = (SIM_HDOP + (double)(rand()) / RAND_MAX);
		GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = (SIM_VDOP + (double)(rand()) / RAND_MAX);
		if (PRIVATE_DATA->timer_ticks == TICKS_TO_2D_FIX) {
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->timer_ticks == TICKS_TO_3D_FIX) {
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
		if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
			indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
		}
	} else {
		GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
		GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
		GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
		GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM->number.value = 0;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		time_t ttime = 0;
		GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_timetoisogm(ttime, GPS_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
		GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = (int)(0.5 + (double)(rand())/RAND_MAX);
		GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = (int)(0.5 + (double)(rand())/RAND_MAX);
		GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = 0;
		GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = 0;
		GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = 0;
		indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
		if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
			indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->timer_ticks == 0) {
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
		}
	}
	PRIVATE_DATA->timer_ticks++;
	indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->gps_timer);

	// Custom code above

	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// CONNECTION change handler

static void gps_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = simulator_open(device);
		if (connection_result) {

			// Custom code below

			srand((unsigned)time(NULL));
			PRIVATE_DATA->timer_ticks = 0;
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;

			// Custom code above

			indigo_set_timer(device, 0, gps_timer_callback, &PRIVATE_DATA->gps_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->gps_timer);
		simulator_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_unlock_master_device(device);
}

#pragma mark - Device API (gps)

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// gps attach API callback

static indigo_result gps_attach(indigo_device *device) {
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;

		// Custom code below

		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 4;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;

		// Custom code above

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// gps enumerate API callback

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_gps_enumerate_properties(device, NULL, NULL);
}

// gps change property API callback

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {

  // CONNECTION change handling

	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, gps_connection_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_gps_change_property(device, client, property);
}

// gps detach API callback

static indigo_result gps_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_gps_detach(device);
}

#pragma mark - Device templates

static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(GPS_DEVICE_NAME, gps_attach, gps_enumerate_properties, gps_change_property, NULL, gps_detach);

#pragma mark - Main code

// GPS Simulator driver entry point

indigo_result indigo_gps_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *gps = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
			gps->private_data = private_data;
			indigo_attach_device(gps);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(gps);
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
