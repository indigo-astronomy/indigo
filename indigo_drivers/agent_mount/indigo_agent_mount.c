// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Mount control agent
 \file indigo_agent_mount.c
 */

#define DRIVER_VERSION 0x0011
#define DRIVER_NAME	"indigo_agent_mount"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_align.h>

#include "indigo_agent_mount.h"

#define DEVICE_PRIVATE_DATA														((mount_agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA														((mount_agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_GEOGRAPHIC_COORDINATES_PROPERTY					(DEVICE_PRIVATE_DATA->agent_geographic_property)
#define AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)
#define AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)
#define AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

#define AGENT_SITE_DATA_SOURCE_PROPERTY								(DEVICE_PRIVATE_DATA->agent_site_data_source_property)
#define AGENT_SITE_DATA_SOURCE_HOST_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+0)
#define AGENT_SITE_DATA_SOURCE_MOUNT_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+1)
#define AGENT_SITE_DATA_SOURCE_DOME_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+2)
#define AGENT_SITE_DATA_SOURCE_GPS_ITEM  							(AGENT_SITE_DATA_SOURCE_PROPERTY->items+3)

#define AGENT_SET_HOST_TIME_PROPERTY									(DEVICE_PRIVATE_DATA->agent_set_host_time_property)
#define AGENT_SET_HOST_TIME_MOUNT_ITEM  							(AGENT_SET_HOST_TIME_PROPERTY->items+0)
#define AGENT_SET_HOST_TIME_DOME_ITEM  								(AGENT_SET_HOST_TIME_PROPERTY->items+1)

#define AGENT_ABORT_RELATED_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_related_processes_property)
#define AGENT_ABORT_IMAGER_ITEM  											(AGENT_ABORT_RELATED_PROCESS_PROPERTY->items+0)
#define AGENT_ABORT_GUIDER_ITEM  											(AGENT_ABORT_RELATED_PROCESS_PROPERTY->items+1)

#define AGENT_LX200_SERVER_PROPERTY										(DEVICE_PRIVATE_DATA->agent_lx200_server_property)
#define AGENT_LX200_SERVER_STOPPED_ITEM								(AGENT_LX200_SERVER_PROPERTY->items+0)
#define AGENT_LX200_SERVER_STARTED_ITEM								(AGENT_LX200_SERVER_PROPERTY->items+1)

#define AGENT_LX200_CONFIGURATION_PROPERTY						(DEVICE_PRIVATE_DATA->agent_lx200_configuration_property)
#define AGENT_LX200_CONFIGURATION_PORT_ITEM						(AGENT_LX200_CONFIGURATION_PROPERTY->items+0)
#define AGENT_LX200_CONFIGURATION_EPOCH_ITEM					(AGENT_LX200_CONFIGURATION_PROPERTY->items+1)

#define AGENT_LIMITS_PROPERTY													(DEVICE_PRIVATE_DATA->agent_limits_property)
#define AGENT_HA_TRACKING_LIMIT_ITEM									(AGENT_LIMITS_PROPERTY->items+0)
#define AGENT_LOCAL_TIME_LIMIT_ITEM										(AGENT_LIMITS_PROPERTY->items+1)
#define AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM			(AGENT_LIMITS_PROPERTY->items+2)

#define AGENT_MOUNT_FOV_PROPERTY											(DEVICE_PRIVATE_DATA->agent_fov_property)
#define AGENT_MOUNT_FOV_ANGLE_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+0)
#define AGENT_MOUNT_FOV_WIDTH_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+1)
#define AGENT_MOUNT_FOV_HEIGHT_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+2)

#define AGENT_MOUNT_TARGET_COORDINATES_PROPERTY				(DEVICE_PRIVATE_DATA->agent_target_coordinates_property)
#define AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM				(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY->items+0)
#define AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM				(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY->items+1)

#define AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY			(DEVICE_PRIVATE_DATA->agent_display_coordinates_property)
#define AGENT_MOUNT_DISPLAY_COORDINATES_RA_JNOW_ITEM	(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+0)
#define AGENT_MOUNT_DISPLAY_COORDINATES_DEC_JNOW_ITEM	(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+1)
#define AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+2)
#define AGENT_MOUNT_DISPLAY_COORDINATES_AZ_ITEM				(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+3)
#define AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM				(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+4)
#define AGENT_MOUNT_DISPLAY_COORDINATES_HA_ITEM				(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+5)
#define AGENT_MOUNT_DISPLAY_COORDINATES_RISE_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+6)
#define AGENT_MOUNT_DISPLAY_COORDINATES_TRANSIT_ITEM	(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+7)
#define AGENT_MOUNT_DISPLAY_COORDINATES_SET_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+8)
#define AGENT_MOUNT_DISPLAY_COORDINATES_TIME_TO_TRANSIT_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+9)
#define AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+10)
#define AGENT_MOUNT_DISPLAY_COORDINATES_DEROTATION_RATE_ITEM			(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->items+11)

#define AGENT_ABORT_PROCESS_PROPERTY									(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      								(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_START_PROCESS_PROPERTY									(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_MOUNT_START_SLEW_ITEM  									(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_MOUNT_START_SYNC_ITEM  									(AGENT_START_PROCESS_PROPERTY->items+1)

#define AGENT_PROCESS_FEATURES_PROPERTY								(DEVICE_PRIVATE_DATA->agent_process_features_property)
#define AGENT_MOUNT_ENABLE_HA_LIMIT_FEATURE_ITEM			(AGENT_PROCESS_FEATURES_PROPERTY->items+0)
#define AGENT_MOUNT_ENABLE_TIME_LIMIT_FEATURE_ITEM		(AGENT_PROCESS_FEATURES_PROPERTY->items+1)

#define AGENT_FIELD_DEROTATION_PROPERTY				(DEVICE_PRIVATE_DATA->agent_field_derotation_proeprety)
#define AGENT_FIELD_DEROTATION_ENABLED_ITEM			(AGENT_FIELD_DEROTATION_PROPERTY->items+0)
#define AGENT_FIELD_DEROTATION_DISABLED_ITEM		(AGENT_FIELD_DEROTATION_PROPERTY->items+1)

typedef struct {
	indigo_property *agent_geographic_property;
	indigo_property *agent_site_data_source_property;
	indigo_property *agent_set_host_time_property;
	indigo_property *agent_related_processes_property;
	indigo_property *agent_lx200_server_property;
	indigo_property *agent_lx200_configuration_property;
	indigo_property *agent_limits_property;
	indigo_property *agent_fov_property;
	indigo_property *agent_target_coordinates_property;
	indigo_property *agent_display_coordinates_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_process_features_property;
	indigo_property *agent_field_derotation_proeprety;
	double mount_latitude, mount_longitude, mount_elevation;
	double dome_latitude, dome_longitude, dome_elevation;
	double gps_latitude, gps_longitude, gps_elevation;
	indigo_property_state mount_eq_coordinates_state;
	double mount_ra, mount_dec;
	int mount_side_of_pier;
	double mount_target_ra, mount_target_dec;
	indigo_property_state rotator_position_state;
	double rotator_position;
	double initial_frame_rotation;
	indigo_uni_handle *server_handle;
	bool mount_unparked;
	bool dome_unparked;
	pthread_mutex_t mutex;
} mount_agent_private_data;

typedef struct {
	indigo_uni_handle *handle;
	indigo_device *device;
} handler_data;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_SITE_DATA_SOURCE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_LX200_CONFIGURATION_PROPERTY);
		indigo_save_property(device, NULL, AGENT_MOUNT_FOV_PROPERTY);
		indigo_save_property(device, NULL, AGENT_SET_HOST_TIME_PROPERTY);
		indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PROCESS_FEATURES_PROPERTY);
		double tmp_ha_tracking_limit = AGENT_HA_TRACKING_LIMIT_ITEM->number.value;
		AGENT_HA_TRACKING_LIMIT_ITEM->number.value = AGENT_HA_TRACKING_LIMIT_ITEM->number.target;
		double tmp_local_time_limit = AGENT_LOCAL_TIME_LIMIT_ITEM->number.value;
		AGENT_LOCAL_TIME_LIMIT_ITEM->number.value = AGENT_LOCAL_TIME_LIMIT_ITEM->number.target;
		indigo_save_property(device, NULL, AGENT_LIMITS_PROPERTY);
		AGENT_HA_TRACKING_LIMIT_ITEM->number.value = tmp_ha_tracking_limit;
		 AGENT_LOCAL_TIME_LIMIT_ITEM->number.value = tmp_local_time_limit;
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
	}
}

static void abort_process(indigo_device *device) {
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
}

static void mount_control(indigo_device *device, char *operation) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	if (!DEVICE_PRIVATE_DATA->mount_unparked) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
	}
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, operation, true);
	static const char *names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
	double values[] = { AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.target, AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.target };
	indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	for (int i = 0; i < 3000; i++) {
		if (DEVICE_PRIVATE_DATA->mount_eq_coordinates_state == INDIGO_BUSY_STATE) {
			break;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			break;
		}
		indigo_usleep(1000);
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
		indigo_debug("MOUNT_EQUATORIAL_COORDINATES didn't become BUSY in 3s");
	}
	for (int i = 0; i < 180000; i++) {
		if (DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
			break;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			break;
		}
		indigo_usleep(1000);
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_OK_STATE) {
		indigo_error("MOUNT_EQUATORIAL_COORDINATES didn't become OK in 180s");
	}
	AGENT_MOUNT_START_SLEW_ITEM->sw.value = AGENT_MOUNT_START_SYNC_ITEM->sw.value = false;
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	} else if (DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_OK_STATE) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	} else {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	}
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void slew_process(indigo_device *device) {
	mount_control(device, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME);
}

static void sync_process(indigo_device *device) {
	mount_control(device, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME);
}

static void lx200_server_worker_thread(indigo_uni_worker_data *data) {
	indigo_uni_handle *handle = data->handle;
	indigo_device *device = data->data;
	char buffer_in[128];
	char buffer_out[128];
	long result = 1;
	indigo_uni_set_socket_read_timeout(handle, 500000);
	INDIGO_DRIVER_TRACE(MOUNT_AGENT_NAME, "%d: CONNECTED", handle->index);
	while (true) {
		*buffer_in = 0;
		*buffer_out = 0;
		result = indigo_uni_read_available(handle, buffer_in, 1);
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
				result = indigo_uni_read_available(handle, buffer_in + i, 1);
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
				double ra = DEVICE_PRIVATE_DATA->mount_ra;
				double dec = DEVICE_PRIVATE_DATA->mount_dec;
				indigo_j2k_to_eq(AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				strcpy(buffer_out, indigo_dtos(ra, "%02d:%02d:%02d#"));
			} else if (strcmp(buffer_in, "GD") == 0) {
				double ra = DEVICE_PRIVATE_DATA->mount_ra;
				double dec = DEVICE_PRIVATE_DATA->mount_dec;
				indigo_j2k_to_eq(AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				strcpy(buffer_out, indigo_dtos(dec, "%+03d*%02d'%02d#"));
			} else if (strncmp(buffer_in, "Sr", 2) == 0) {
				int h = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &h, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->mount_target_ra = h + m/60.0 + s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &h, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->mount_target_ra = h + m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "Sd", 2) == 0) {
				int d = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &d, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->mount_target_dec = d > 0 ? d + m/60.0 + s/3600.0 : d - m/60.0 - s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &d, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->mount_target_dec = d > 0 ? d + m/60.0 : d - m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "MS", 2) == 0) {
				double ra = DEVICE_PRIVATE_DATA->mount_target_ra;
				double dec = DEVICE_PRIVATE_DATA->mount_target_dec;
				indigo_eq_to_j2k(AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.target = AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.value = ra;
				AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.target = AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.value = dec;
				indigo_update_property(device, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, NULL);
				if (INDIGO_FILTER_MOUNT_SELECTED && AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
					AGENT_MOUNT_START_SLEW_ITEM->sw.value = true;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
					indigo_set_timer(device, 0, slew_process, NULL);
				}
				strcpy(buffer_out, "0");
			} else if (strncmp(buffer_in, "CM", 2) == 0) {
				double ra = DEVICE_PRIVATE_DATA->mount_target_ra;
				double dec = DEVICE_PRIVATE_DATA->mount_target_dec;
				indigo_eq_to_j2k(AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.value, &ra, &dec);
				AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.target = AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.value = ra;
				AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.target = AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.value = dec;
				indigo_update_property(device, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, NULL);
				if (INDIGO_FILTER_MOUNT_SELECTED && AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
					AGENT_MOUNT_START_SLEW_ITEM->sw.value = true;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
					indigo_set_timer(device, 0, sync_process, NULL);
				}
				strcpy(buffer_out, "OK#");
			} else if (strcmp(buffer_in, "RG") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RC") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RM") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_FIND_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RS") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_MAX_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Sw1") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw2") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw3") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_FIND_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw4") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_MAX_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Mn") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qn") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Ms") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qs") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Mw") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qw") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Me") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qe") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Q") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
			} else if (strncmp(buffer_in, "SC", 2) == 0) {
				// http://www.dv-fansler.com/FTP%20Files/Astronomy/LX200%20Hand%20Controller%20Communications.pdf
				strcpy(buffer_out, "1Updating        planetary data. #                                #");
			} else if (strncmp(buffer_in, "S", 1) == 0) {
				strcpy(buffer_out, "1");
			}
			if (*buffer_out) {
				indigo_uni_write(handle, buffer_out, strlen(buffer_out));
				INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "%d: '%s' -> '%s'", handle->index, buffer_in, buffer_out);
			} else {
				INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "%d: '%s' -> ", handle->index, buffer_in);
			}
		}
	}
	INDIGO_DRIVER_TRACE(MOUNT_AGENT_NAME, "%d: DISCONNECTED", handle->index);
	indigo_uni_close(&handle);
	free(data);
}

static void start_lx200_server(indigo_device *device) {
	int port = (int)AGENT_LX200_CONFIGURATION_PORT_ITEM->number.value;
	AGENT_LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "Starting server on %d", (int)AGENT_LX200_CONFIGURATION_PORT_ITEM->number.value);
	indigo_uni_open_tcp_server_socket(&port, &DEVICE_PRIVATE_DATA->server_handle, lx200_server_worker_thread, device, NULL);
	AGENT_LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
	indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "Server finished");
}

static void stop_lx200_server(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->server_handle != NULL) {
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
		indigo_uni_close(&DEVICE_PRIVATE_DATA->server_handle);
	}
}

static void abort_imager_process(indigo_device *device, char *reason) {
	if (!AGENT_ABORT_IMAGER_ITEM->sw.value) {
		return;
	}
	indigo_send_message(device, "Aborting Imager agent process due to %s", reason);
	char *related_agent_name = indigo_filter_first_related_agent(device, "Imager Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
	}
}

static void abort_guider_process(indigo_device *device, char *reason) {
	if (!AGENT_ABORT_GUIDER_ITEM->sw.value) {
		return;
	}
	indigo_send_message(device, "Aborting Guider agent process due to %s", reason);
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
	}
}

static void reset_star_selection(indigo_device *device, char *reason) {
	// indigo_send_message(device, "Clearing star selection due to %s", reason);
	char *related_agent_name = indigo_filter_first_related_agent(device, "Imager Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_IMAGER_CLEAR_SELECTION_ITEM_NAME, true);
	}
	related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_CLEAR_SELECTION_ITEM_NAME, true);
	}
}

static void handle_mount_change(indigo_device *device) {
	time_t utc = time(NULL);
	double ra = DEVICE_PRIVATE_DATA->mount_ra;
	double dec = DEVICE_PRIVATE_DATA->mount_dec;
	double longitude = AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	double latitude = AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
	double elevation = AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value;
	double lst = indigo_lst(&utc, longitude);
	double values[] = { ra, dec, DEVICE_PRIVATE_DATA->mount_side_of_pier };
	// update target coordinates
	AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.value = ra;
	AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.value = dec;
	AGENT_MOUNT_TARGET_COORDINATES_PROPERTY->state = DEVICE_PRIVATE_DATA->mount_eq_coordinates_state;
	indigo_update_property(device, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, NULL);
	// update display coordinates
	indigo_j2k_to_jnow(&ra, &dec);
	AGENT_MOUNT_DISPLAY_COORDINATES_RA_JNOW_ITEM->number.value = ra;
	AGENT_MOUNT_DISPLAY_COORDINATES_DEC_JNOW_ITEM->number.value = dec;
	indigo_radec_to_altaz(ra, dec, &utc, latitude, longitude, elevation, &AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM->number.value, &AGENT_MOUNT_DISPLAY_COORDINATES_AZ_ITEM->number.value);
	AGENT_MOUNT_DISPLAY_COORDINATES_HA_ITEM->number.value = fmod((lst - ra + 24), 24);
	indigo_raise_set(UT2JD(utc), latitude, longitude, ra, dec, &AGENT_MOUNT_DISPLAY_COORDINATES_RISE_ITEM->number.value, &AGENT_MOUNT_DISPLAY_COORDINATES_TRANSIT_ITEM->number.value, &AGENT_MOUNT_DISPLAY_COORDINATES_SET_ITEM->number.value);
	AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM->number.value = indigo_airmass(AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM->number.value);
	AGENT_MOUNT_DISPLAY_COORDINATES_TIME_TO_TRANSIT_ITEM->number.value = indigo_time_to_transit(ra, lst);
	AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY->state = DEVICE_PRIVATE_DATA->mount_eq_coordinates_state;
	AGENT_MOUNT_DISPLAY_COORDINATES_DEROTATION_RATE_ITEM->number.value = indigo_derotation_rate(AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM->number.value, AGENT_MOUNT_DISPLAY_COORDINATES_AZ_ITEM->number.value, latitude);
	AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM->number.value = indigo_parallactic_angle(AGENT_MOUNT_DISPLAY_COORDINATES_HA_ITEM->number.value * 15, dec, latitude);
	indigo_update_property(device, AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY, NULL);
	// check limits
	double ha = fmod(lst - ra + 24, 24);
	time_t timer;
	time(&timer);
	struct tm *info = localtime(&timer);
	double now = info->tm_hour + info->tm_min / 60.0 + info->tm_sec / 3600.0;
	AGENT_HA_TRACKING_LIMIT_ITEM->number.value = ha;
	AGENT_LOCAL_TIME_LIMIT_ITEM->number.value = now;
	indigo_update_property(device, AGENT_LIMITS_PROPERTY, NULL);
	if (INDIGO_FILTER_MOUNT_SELECTED && DEVICE_PRIVATE_DATA->mount_unparked) {
		bool park = false;
		if (AGENT_MOUNT_ENABLE_HA_LIMIT_FEATURE_ITEM->sw.value) {
			double target = AGENT_HA_TRACKING_LIMIT_ITEM->number.target;
			if ((target < 12 && ha < 12 && ha > target) || ((target > 12 && target < 24) && ((ha > 12 &&  ha > target) || (ha < 12 && ha + 24 > target)))) {
				park = true;
				indigo_send_message(device, "Hour angle tracking limit reached");
			}
		}
		if (AGENT_MOUNT_ENABLE_TIME_LIMIT_FEATURE_ITEM->sw.value) {
			double target = AGENT_LOCAL_TIME_LIMIT_ITEM->number.target;
			if (now < 12 && target < 12 && now > target) {
				park = true;
				indigo_send_message(device, "Time limit reached");
			}
			if (now > 12 && target > 12 && now > target) {
				park = true;
				indigo_send_message(device, "Time limit reached");
			}
		}
		if (park) {
			abort_imager_process(device, "hitting limits");
			abort_guider_process(device, "hitting limits");
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true);
		}
	}
	// slave dome
	if (INDIGO_FILTER_DOME_SELECTED && DEVICE_PRIVATE_DATA->dome_unparked && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_ALERT_STATE) {
		static const char *names[] = { DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME, DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
	}
	// derotate field
	if (INDIGO_FILTER_ROTATOR_SELECTED && AGENT_FIELD_DEROTATION_ENABLED_ITEM->sw.value) {
		double target_rotator_position = AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM->number.value + DEVICE_PRIVATE_DATA->initial_frame_rotation;
		if (target_rotator_position < 0) {
			target_rotator_position += 360;
		} else if (target_rotator_position >= 360) {
			target_rotator_position -= 360;
		}
		double rotation_diff = fabs(indigo_angle_difference(DEVICE_PRIVATE_DATA->rotator_position, target_rotator_position));
		INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "Derotation: target_rotator_position = %g, rotator_position = %g, parallactic_angle = %g, rotation_diff = %g", target_rotator_position, DEVICE_PRIVATE_DATA->rotator_position, AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM->number.value, rotation_diff);
		if (rotation_diff >= 0.005 && DEVICE_PRIVATE_DATA->rotator_position_state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "Derotation: going to position %g", target_rotator_position);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, target_rotator_position);
		}
	}
	// set eq coordinates and airmass to FITS headers of related imager agent
	char *related_agent_name = indigo_filter_first_related_agent(device, "Imager Agent");
	if (related_agent_name) {
		if (AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM->number.value >= 1.0) {
			indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "AIRMASS", "%20.6f / air mass at DATE-OBS", AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM->number.value);
		} else {
			indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "AIRMASS");
		}
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "OBJCTRA", "'%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_ra), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 3600)) % 60);
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "OBJCTDEC", "'%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_dec), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 3600)) % 60);
	}
	// set eq coordinates to related guider agent
	related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		static const char *names[] = { AGENT_GUIDER_MOUNT_COORDINATES_RA_ITEM_NAME, AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM_NAME, AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM_NAME };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY_NAME, 3, names, values);
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "OBJCTRA", "'%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_ra), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 3600)) % 60);
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "OBJCTDEC", "'%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_dec), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 3600)) % 60);
	}
}

static void handle_site_change(indigo_device *device) {
	static const char *names[] = { GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME };
	double latitude = 0, longitude = 0, elevation = 0;
	// select coordinates source
	if (INDIGO_FILTER_MOUNT_SELECTED && AGENT_SITE_DATA_SOURCE_MOUNT_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->mount_latitude;
		longitude = DEVICE_PRIVATE_DATA->mount_longitude;
		elevation = DEVICE_PRIVATE_DATA->mount_elevation;
		double values[] = { latitude, longitude, elevation };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "DOME_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
	} else if (INDIGO_FILTER_DOME_SELECTED && AGENT_SITE_DATA_SOURCE_DOME_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->dome_latitude;
		longitude = DEVICE_PRIVATE_DATA->dome_longitude;
		elevation = DEVICE_PRIVATE_DATA->dome_elevation;
		double values[] = { latitude, longitude, elevation };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "MOUNT_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
	} else if (INDIGO_FILTER_GPS_SELECTED && AGENT_SITE_DATA_SOURCE_GPS_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->gps_latitude;
		longitude = DEVICE_PRIVATE_DATA->gps_longitude;
		elevation = DEVICE_PRIVATE_DATA->gps_elevation;
		double values[] = { latitude, longitude, elevation };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "MOUNT_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "DOME_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
	} else {
		latitude = AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target;
		longitude = AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target;
		elevation = AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.target;
		double values[] = { latitude, longitude, elevation };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "MOUNT_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, "DOME_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
	}
	// set host time if needed
	if (AGENT_SET_HOST_TIME_MOUNT_ITEM->sw.value)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SET_HOST_TIME_ITEM_NAME, true);
	if (AGENT_SET_HOST_TIME_DOME_ITEM->sw.value)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, DOME_SET_HOST_TIME_PROPERTY_NAME, DOME_SET_HOST_TIME_ITEM_NAME, true);
	AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
	AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
	AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
	AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	// set site coordinates to FITS headers of related imager agent
	char *related_agent_name = indigo_filter_first_related_agent(device, "Imager Agent");
	if (related_agent_name) {
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "SITELAT", "'%d %02d %02d'", (int)(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value), ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) * 60)) % 60, ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) * 3600)) % 60);
		indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, related_agent_name, "SITELONG", "'%d %02d %02d'", (int)(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value), ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) * 60)) % 60, ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) * 3600)) % 60);
	}
	// update display coordinates
	handle_mount_change(device);
}

static void snoop_changes(indigo_client *client, indigo_device *device, indigo_property *property) {
	if (!strcmp(property->name, FILTER_MOUNT_LIST_PROPERTY_NAME)) { // Snoop mount
		if (INDIGO_FILTER_MOUNT_SELECTED) {
			handle_site_change(device);
		} else {
			DEVICE_PRIVATE_DATA->mount_eq_coordinates_state = INDIGO_IDLE_STATE;
			if (AGENT_FIELD_DEROTATION_ENABLED_ITEM->sw.value) {
				indigo_set_switch(AGENT_FIELD_DEROTATION_PROPERTY, AGENT_FIELD_DEROTATION_DISABLED_ITEM, true);
				AGENT_FIELD_DEROTATION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_FIELD_DEROTATION_PROPERTY, "Mount disconnected - derotation disabled");
			}
		}
	} else if (!strcmp(property->name, "MOUNT_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		bool changed = false;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->mount_latitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->mount_latitude = item->number.value;
				}
			} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->mount_longitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->mount_longitude = item->number.value;
				}
			} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
				changed = changed || CLIENT_PRIVATE_DATA->mount_elevation != item->number.value;
				CLIENT_PRIVATE_DATA->mount_elevation = item->number.value;
			}
		}
		if (changed && AGENT_SITE_DATA_SOURCE_MOUNT_ITEM->sw.value) {
			handle_site_change(device);
		}
	} else if (!strcmp(property->name, MOUNT_SIDE_OF_PIER_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->mount_side_of_pier = 0;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (item->sw.value && !strcmp(item->name, MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME))
				CLIENT_PRIVATE_DATA->mount_side_of_pier = -1;
			else if (item->sw.value && !strcmp(item->name, MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME))
				CLIENT_PRIVATE_DATA->mount_side_of_pier = 1;
		}
		handle_mount_change(device);
	} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)) {
				CLIENT_PRIVATE_DATA->mount_ra = item->number.value;
			} else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)) {
				CLIENT_PRIVATE_DATA->mount_dec = item->number.value;
			}
		}
		if (CLIENT_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE && property->state == INDIGO_BUSY_STATE) {
			abort_imager_process(device, "slewing");
			abort_guider_process(device, "slewing");
		}
		if (CLIENT_PRIVATE_DATA->mount_eq_coordinates_state == INDIGO_BUSY_STATE && property->state != INDIGO_BUSY_STATE) {
			reset_star_selection(device, "slewing");
		}
		CLIENT_PRIVATE_DATA->mount_eq_coordinates_state = property->state;
		handle_mount_change(device);
	} else if (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->mount_unparked = false;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_PARK_PARKED_ITEM_NAME) && item->sw.value) {
					indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, device->name, DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME, true);
					indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, device->name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true);
					abort_imager_process(device, "parking");
					abort_guider_process(device, "parking");
				} else if (!strcmp(property->items[i].name, MOUNT_PARK_UNPARKED_ITEM_NAME) && item->sw.value) {
					CLIENT_PRIVATE_DATA->mount_unparked = true;
					indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, device->name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true);
					indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, device->name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true);
				}
			}
		}
	} else if (!strcmp(property->name, FILTER_DOME_LIST_PROPERTY_NAME)) { // Snoop dome
		if (INDIGO_FILTER_DOME_SELECTED) {
			handle_site_change(device);
		}
	} else if (!strcmp(property->name, "DOME_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		bool changed = false;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->dome_latitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->dome_latitude = item->number.value;
				}
			} else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->dome_longitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->dome_longitude = item->number.value;
				}
			} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
				changed = changed || CLIENT_PRIVATE_DATA->dome_elevation != item->number.value;
				CLIENT_PRIVATE_DATA->dome_elevation = item->number.value;
			}
		}
		if (changed && AGENT_SITE_DATA_SOURCE_DOME_ITEM->sw.value) {
			handle_site_change(device);
		}
	} else if (!strcmp(property->name, DOME_PARK_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->dome_unparked = false;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, DOME_PARK_UNPARKED_ITEM_NAME)) {
					CLIENT_PRIVATE_DATA->dome_unparked = item->sw.value;
					break;
				}
			}
		}
	} else if (!strcmp(property->name, "GPS_" GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) { // Snoop gps
		bool changed = false;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->gps_latitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->gps_latitude = property->items[i].number.value;
				}
			} else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
				changed = changed || fabs(CLIENT_PRIVATE_DATA->gps_longitude - item->number.value) > AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM->number.value;
				if (changed) {
					CLIENT_PRIVATE_DATA->gps_longitude = item->number.value;
				}
			} else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
				changed = changed || CLIENT_PRIVATE_DATA->gps_elevation != item->number.value;
				CLIENT_PRIVATE_DATA->gps_elevation = item->number.value;
			}
		}
		if (changed && AGENT_SITE_DATA_SOURCE_GPS_ITEM->sw.value) {
			handle_site_change(device);
		}
	} else if (!strcmp(property->name, FILTER_ROTATOR_LIST_PROPERTY_NAME)) { // Snoop rotator
		if (!INDIGO_FILTER_ROTATOR_SELECTED) {
			DEVICE_PRIVATE_DATA->rotator_position_state = INDIGO_IDLE_STATE;
			if (AGENT_FIELD_DEROTATION_ENABLED_ITEM->sw.value) {
				indigo_set_switch(AGENT_FIELD_DEROTATION_PROPERTY, AGENT_FIELD_DEROTATION_DISABLED_ITEM, true);
				AGENT_FIELD_DEROTATION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_FIELD_DEROTATION_PROPERTY, "Rotator disconnected - derotation disabled");
			}
		}
	} else if (!strcmp(property->name, ROTATOR_POSITION_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->rotator_position_state = property->state;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, ROTATOR_POSITION_ITEM_NAME)) {
					CLIENT_PRIVATE_DATA->rotator_position = item->number.value;
					break;
				}
			}
		}
	}
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_MOUNT | INDIGO_INTERFACE_DOME | INDIGO_INTERFACE_GPS | INDIGO_INTERFACE_AUX_JOYSTICK) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_MOUNT_LIST_PROPERTY->hidden = false;
		FILTER_DOME_LIST_PROPERTY->hidden = false;
		FILTER_ROTATOR_LIST_PROPERTY->hidden = false;
		FILTER_GPS_LIST_PROPERTY->hidden = false;
		FILTER_JOYSTICK_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- GEOGRAPHIC_COORDINATES
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, "Agent", "Location", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
		return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", -180, 360, 0, 0);
		indigo_init_number_item(AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", -400, 8000, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_SITE_DATA_SOURCE
		AGENT_SITE_DATA_SOURCE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SITE_DATA_SOURCE_PROPERTY_NAME, "Agent", "Location coordinates source", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_SITE_DATA_SOURCE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_HOST_ITEM, AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME, "Use agent coordinates", true);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_MOUNT_ITEM, AGENT_SITE_DATA_SOURCE_MOUNT_ITEM_NAME, "Use mount coordinates", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_DOME_ITEM, AGENT_SITE_DATA_SOURCE_DOME_ITEM_NAME, "Use dome coordinates", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_GPS_ITEM, AGENT_SITE_DATA_SOURCE_GPS_ITEM_NAME, "Use GPS coordinates", false);
		// -------------------------------------------------------------------------------- AGENT_SET_HOST_TIME
		AGENT_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SET_HOST_TIME_PROPERTY_NAME, "Agent", "Use host time", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_SET_HOST_TIME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_SET_HOST_TIME_MOUNT_ITEM, AGENT_SET_HOST_TIME_MOUNT_ITEM_NAME, "Use host time for mount", true);
		indigo_init_switch_item(AGENT_SET_HOST_TIME_DOME_ITEM, AGENT_SET_HOST_TIME_DOME_ITEM_NAME, "Use host time for dome", true);
		// -------------------------------------------------------------------------------- AGENT_ABORT_RELATED_PROCESS
		AGENT_ABORT_RELATED_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, "Agent", "Allow to abort related process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_ABORT_RELATED_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_IMAGER_ITEM, AGENT_ABORT_IMAGER_ITEM_NAME, "Imaging", false);
		indigo_init_switch_item(AGENT_ABORT_GUIDER_ITEM, AGENT_ABORT_GUIDER_ITEM_NAME, "Guiding", false);
		// -------------------------------------------------------------------------------- AGENT_LX200_SERVER
		AGENT_LX200_SERVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_LX200_SERVER_PROPERTY_NAME, "Agent", "LX200 Server state", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_LX200_SERVER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_LX200_SERVER_STARTED_ITEM, AGENT_LX200_SERVER_STARTED_ITEM_NAME, "Start LX200 server", false);
		indigo_init_switch_item(AGENT_LX200_SERVER_STOPPED_ITEM, AGENT_LX200_SERVER_STOPPED_ITEM_NAME, "Stop LX200 server", true);
		AGENT_LX200_CONFIGURATION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_LX200_CONFIGURATION_PROPERTY_NAME, "Agent", "LX200 Server configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_LX200_CONFIGURATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_LX200_CONFIGURATION_PORT_ITEM, AGENT_LX200_CONFIGURATION_PORT_ITEM_NAME, "Server port", 0, 0xFFFF, 0, 4030);
		indigo_init_number_item(AGENT_LX200_CONFIGURATION_EPOCH_ITEM, AGENT_LX200_CONFIGURATION_EPOCH_ITEM_NAME, "Epoch (0=JNow, 2000=J2k)", 0, 2050, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_LIMITS
		AGENT_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_LIMITS_PROPERTY_NAME, "Agent", "Limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_LIMITS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_HA_TRACKING_LIMIT_ITEM, AGENT_HA_TRACKING_LIMIT_ITEM_NAME, "HA tracking limit (0 to 24 hrs)", 0, 24, 0, 24);
		indigo_init_sexagesimal_number_item(AGENT_LOCAL_TIME_LIMIT_ITEM, AGENT_LOCAL_TIME_LIMIT_ITEM_NAME, "Time limit (0 to 24 hrs)", 0, 24, 0, 12);
		indigo_init_sexagesimal_number_item(AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM, AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM_NAME, "Coordinate propagation threshold (°)", 0, 360, 0, 5.0/3600.0);
		// -------------------------------------------------------------------------------- AGENT_MOUNT_FOV
		AGENT_MOUNT_FOV_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_MOUNT_FOV_PROPERTY_NAME, "Agent", "FOV", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_MOUNT_FOV_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_ANGLE_ITEM, AGENT_MOUNT_FOV_ANGLE_ITEM_NAME, "Angle (°)", -360, 360, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_WIDTH_ITEM, AGENT_MOUNT_FOV_WIDTH_ITEM_NAME, "Width (°)", 0, 360, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_HEIGHT_ITEM, AGENT_MOUNT_FOV_HEIGHT_ITEM_NAME, "Height (°)", 0, 360, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_MOUNT_TARGET_COORDINATES
		AGENT_MOUNT_TARGET_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY_NAME, "Agent", "Target coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_MOUNT_TARGET_COORDINATES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM, AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM, AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM_NAME, "Declination (-90° to +90°)", -90, 90, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_MOUNT_DISPLAY_COORDINATES
		AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY_NAME, "Agent", "Display coordinates", INDIGO_OK_STATE, INDIGO_RO_PERM, 12);
		if (AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_RA_JNOW_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_RA_JNOW_ITEM_NAME, "Right ascension JNow (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_DEC_JNOW_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_DEC_JNOW_ITEM_NAME, "Declination JNow (-90° to +90°)", -90, 90, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 180°)", 0, 180, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_AZ_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_AZ_ITEM_NAME, "Azimuth (0° to 360°)", -90, 360, 0, 0);
		indigo_init_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_AIRMASS_ITEM_NAME, "Airmass (1 to ∞)", 1, 10, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_HA_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_HA_ITEM_NAME, "Hour angle (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_RISE_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_RISE_ITEM_NAME, "Raise time (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_TRANSIT_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_TRANSIT_ITEM_NAME, "Transit time (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_SET_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_SET_ITEM_NAME, "Set time (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_TIME_TO_TRANSIT_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_TIME_TO_TRANSIT_ITEM_NAME, "Time to transit (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM_NAME, "Parallactic angle (-180 to 180°)", -180, 180, 0, 0);
		indigo_init_number_item(AGENT_MOUNT_DISPLAY_COORDINATES_DEROTATION_RATE_ITEM, AGENT_MOUNT_DISPLAY_COORDINATES_DEROTATION_RATE_ITEM_NAME, "Derotation rate (\"/s)", -1000, 1000, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_FIELD_DEROTATION
		AGENT_FIELD_DEROTATION_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_FIELD_DEROTATION_PROPERTY_NAME, "Agent", "Derotate field for Alt/Az mounts", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_FIELD_DEROTATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_FIELD_DEROTATION_ENABLED_ITEM, AGENT_FIELD_DEROTATION_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(AGENT_FIELD_DEROTATION_DISABLED_ITEM, AGENT_FIELD_DEROTATION_DISABLED_ITEM_NAME, "Disabled", true);
		// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_MOUNT_START_SLEW_ITEM, AGENT_MOUNT_START_SLEW_ITEM_NAME, "Slew", false);
		indigo_init_switch_item(AGENT_MOUNT_START_SYNC_ITEM, AGENT_MOUNT_START_SYNC_ITEM_NAME, "Sync", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort", false);
		AGENT_PROCESS_FEATURES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PROCESS_FEATURES_PROPERTY_NAME, "Agent", "Process features", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_PROCESS_FEATURES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_MOUNT_ENABLE_HA_LIMIT_FEATURE_ITEM, AGENT_MOUNT_ENABLE_HA_LIMIT_FEATURE_ITEM_NAME, "Enable HA limit", false);
		indigo_init_switch_item(AGENT_MOUNT_ENABLE_TIME_LIMIT_FEATURE_ITEM, AGENT_MOUNT_ENABLE_TIME_LIMIT_FEATURE_ITEM_NAME, "Enable time limit", false);
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->mutex, NULL);
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	indigo_define_matching_property(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_define_matching_property(AGENT_SITE_DATA_SOURCE_PROPERTY);
	indigo_define_matching_property(AGENT_SET_HOST_TIME_PROPERTY);
	indigo_define_matching_property(AGENT_ABORT_RELATED_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_LX200_SERVER_PROPERTY);
	indigo_define_matching_property(AGENT_LX200_CONFIGURATION_PROPERTY);
	indigo_define_matching_property(AGENT_LIMITS_PROPERTY);
	indigo_define_matching_property(AGENT_MOUNT_FOV_PROPERTY);
	indigo_define_matching_property(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY);
	indigo_define_matching_property(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY);
	indigo_define_matching_property(AGENT_FIELD_DEROTATION_PROPERTY);
	indigo_define_matching_property(AGENT_START_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_PROCESS_FEATURES_PROPERTY);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(AGENT_SITE_DATA_SOURCE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_SITE_DATA_SOURCE
		indigo_property_copy_values(AGENT_SITE_DATA_SOURCE_PROPERTY, property, false);
		handle_site_change(device);
		AGENT_SITE_DATA_SOURCE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_SITE_DATA_SOURCE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SET_HOST_TIME_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_SET_HOST_TIME
		indigo_property_copy_values(AGENT_SET_HOST_TIME_PROPERTY, property, false);
		if (AGENT_SET_HOST_TIME_MOUNT_ITEM->sw.value && INDIGO_FILTER_MOUNT_SELECTED)
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SET_HOST_TIME_ITEM_NAME, true);
		if (AGENT_SET_HOST_TIME_DOME_ITEM->sw.value && INDIGO_FILTER_DOME_SELECTED)
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, DOME_SET_HOST_TIME_PROPERTY_NAME, DOME_SET_HOST_TIME_ITEM_NAME, true);
		AGENT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ABORT_RELATED_PROCESS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_ABORT_RELATED_PROCESS
		indigo_property_copy_values(AGENT_ABORT_RELATED_PROCESS_PROPERTY, property, false);
		indigo_update_property(device, AGENT_ABORT_RELATED_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		handle_site_change(device);
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LX200_SERVER_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- LX200_SERVER
		indigo_property_copy_values(AGENT_LX200_SERVER_PROPERTY, property, false);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
		if (AGENT_LX200_SERVER_STARTED_ITEM->sw.value) {
			indigo_set_timer(device, 0, start_lx200_server, NULL);
		} else {
			indigo_set_timer(device, 0, stop_lx200_server, NULL);
		}
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LX200_CONFIGURATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_LX200_CONFIGURATION
		indigo_property_copy_values(AGENT_LX200_CONFIGURATION_PROPERTY, property, false);
		if (AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.target != 0 && AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.target != 2000) {
			AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.value = AGENT_LX200_CONFIGURATION_EPOCH_ITEM->number.target = 0;
			indigo_send_message(device, "Warning! Valid values are 0 or 2000 only, value adjusted to 0");
		}
		AGENT_LX200_CONFIGURATION_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_LX200_CONFIGURATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_LIMITS
		indigo_property_copy_values(AGENT_LIMITS_PROPERTY, property, false);
		AGENT_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_FIELD_DEROTATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_FIELD_DEROTATION
		indigo_property_copy_values(AGENT_FIELD_DEROTATION_PROPERTY, property, false);
		AGENT_FIELD_DEROTATION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_FIELD_DEROTATION_PROPERTY, NULL);
		if (INDIGO_FILTER_ROTATOR_SELECTED) {
			if (AGENT_FIELD_DEROTATION_ENABLED_ITEM->sw.value) {
				DEVICE_PRIVATE_DATA->initial_frame_rotation = DEVICE_PRIVATE_DATA->rotator_position - AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM->number.value;
				INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "Derotation started: initial_frame_rotation = %g, rotator_position = %g, parallactic_angle = %f", DEVICE_PRIVATE_DATA->initial_frame_rotation, DEVICE_PRIVATE_DATA->rotator_position, AGENT_MOUNT_DISPLAY_COORDINATES_PARALLACTIC_ANGLE_ITEM->number.value);
			} else {
				DEVICE_PRIVATE_DATA->initial_frame_rotation = 0;
				INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME,"Derotation stopped");
			}
			AGENT_FIELD_DEROTATION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_FIELD_DEROTATION_PROPERTY, NULL);
		} else {
			indigo_set_switch(AGENT_FIELD_DEROTATION_PROPERTY, AGENT_FIELD_DEROTATION_DISABLED_ITEM, true);
			AGENT_FIELD_DEROTATION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_FIELD_DEROTATION_PROPERTY, "No rotator selected");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_MOUNT_FOV_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_MOUNT_FOV
		indigo_property_copy_values(AGENT_MOUNT_FOV_PROPERTY, property, false);
		AGENT_MOUNT_FOV_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_MOUNT_FOV_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_MOUNT_TARGET_COORDINATES
		double ra = AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.value;
		double dec = AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, property, false);
		AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM->number.value = ra;
		AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM->number.value = dec;
		if (AGENT_MOUNT_TARGET_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
			AGENT_MOUNT_TARGET_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->mount_eq_coordinates_state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			if (INDIGO_FILTER_MOUNT_SELECTED) {
				if (AGENT_MOUNT_START_SLEW_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, slew_process, NULL);
				} else if (AGENT_MOUNT_START_SYNC_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, sync_process, NULL);
				}
			} else {
				AGENT_MOUNT_START_SLEW_ITEM->sw.value =
				AGENT_MOUNT_START_SYNC_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No mount is selected");
			}
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			abort_process(device);
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PROCESS_FEATURES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_PROCESS_FEATURES
		indigo_property_copy_values(AGENT_PROCESS_FEATURES_PROPERTY, property, false);
		AGENT_PROCESS_FEATURES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PROCESS_FEATURES_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		if (indigo_filter_change_property(device, client, property) == INDIGO_OK) {
			save_config(device);
		}
		return INDIGO_OK;
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	save_config(device);
	indigo_release_property(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_SITE_DATA_SOURCE_PROPERTY);
	indigo_release_property(AGENT_SET_HOST_TIME_PROPERTY);
	indigo_release_property(AGENT_ABORT_RELATED_PROCESS_PROPERTY);
	indigo_release_property(AGENT_LX200_SERVER_PROPERTY);
	indigo_release_property(AGENT_LX200_CONFIGURATION_PROPERTY);
	indigo_release_property(AGENT_LIMITS_PROPERTY);
	indigo_release_property(AGENT_MOUNT_FOV_PROPERTY);
	indigo_release_property(AGENT_MOUNT_TARGET_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_FIELD_DEROTATION_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_release_property(AGENT_PROCESS_FEATURES_PROPERTY);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME)) {
			if (property->count > 0) {
				indigo_send_message(FILTER_CLIENT_CONTEXT->device, "There are active saved alignment points. Make sure you want to use them.");
			}
		} else {
			snoop_changes(client, device, property);
		}
	} else {
		char *related_imager_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Imager Agent");
		if (related_imager_agent_name && !strcmp(property->device, related_imager_agent_name)) {
			if (!strcmp(property->name, CCD_SET_FITS_HEADER_PROPERTY_NAME)) {
				handle_site_change(FILTER_CLIENT_CONTEXT->device);
			}
		} else {
			char *related_guider_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Guider Agent");
			if (related_guider_agent_name && !strcmp(property->device, related_guider_agent_name)) {
				if (!strcmp(property->name, AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY_NAME)) {
					handle_mount_change(FILTER_CLIENT_CONTEXT->device);
				}
			}
		}
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, "JOYSTICK_" MOUNT_MOTION_DEC_PROPERTY_NAME) || !strcmp(property->name, "JOYSTICK_" MOUNT_MOTION_RA_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				// forward property even if no item is on
				indigo_filter_forward_change_property(client, property, NULL, property->name + 9);
			}
		} else if (!strcmp(property->name, "JOYSTICK_" MOUNT_PARK_PROPERTY_NAME) || !strcmp(property->name, "JOYSTICK_" MOUNT_HOME_PROPERTY_NAME) || !strcmp(property->name, "JOYSTICK_" MOUNT_SLEW_RATE_PROPERTY_NAME) || !strcmp(property->name, "JOYSTICK_" MOUNT_TRACKING_PROPERTY_NAME) || !strcmp(property->name, "JOYSTICK_" MOUNT_ABORT_MOTION_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				// forward property only if some item is on
				for (int i = 0; i < property->count; i++) {
					if (property->items[i].sw.value) {
						indigo_filter_forward_change_property(client, property, NULL, property->name + 9);
						break;
					}
				}
			}
		} else if (!strcmp(property->name, "JOYSTICK_" FOCUSER_CONTROL_PROPERTY_NAME)) {
			char *related_imager_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Imager Agent");
			if (related_imager_agent_name) {
				static const char *names[] = { FOCUSER_FOCUS_IN_ITEM_NAME, FOCUSER_FOCUS_OUT_ITEM_NAME };
				bool values[] = { false, false };
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (!strcmp(item->name, FOCUSER_FOCUS_IN_ITEM_NAME)) {
						values[0] = item->sw.value;
					} else if (!strcmp(item->name, FOCUSER_FOCUS_OUT_ITEM_NAME)) {
						values[1] = item->sw.value;
					}
				}
				indigo_change_switch_property(client, related_imager_agent_name, "AGENT_" FOCUSER_CONTROL_PROPERTY_NAME, 2, names, values);
			}
		} else {
			snoop_changes(client, device, property);
		}
	}
	return indigo_filter_update_property(client, device, property, message);
}

// -------------------------------------------------------------------------------- Initialization

static mount_agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_mount(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		MOUNT_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		indigo_filter_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, MOUNT_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(mount_agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
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
