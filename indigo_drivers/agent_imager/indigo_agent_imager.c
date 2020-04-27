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

/** INDIGO Imager agent
 \file indigo_agent_imager.c
 */

#define DRIVER_VERSION 0x000C
#define DRIVER_NAME	"indigo_agent_imager"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_guider_utils.h>

#include "indigo_agent_imager.h"

#define DEVICE_PRIVATE_DATA										((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_IMAGER_BATCH_PROPERTY						(DEVICE_PRIVATE_DATA->agent_imager_batch_property)
#define AGENT_IMAGER_BATCH_COUNT_ITEM    			(AGENT_IMAGER_BATCH_PROPERTY->items+0)
#define AGENT_IMAGER_BATCH_EXPOSURE_ITEM  		(AGENT_IMAGER_BATCH_PROPERTY->items+1)
#define AGENT_IMAGER_BATCH_DELAY_ITEM     		(AGENT_IMAGER_BATCH_PROPERTY->items+2)

#define AGENT_IMAGER_FOCUS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_imager_focus_property)
#define AGENT_IMAGER_FOCUS_INITIAL_ITEM    		(AGENT_IMAGER_FOCUS_PROPERTY->items+0)
#define AGENT_IMAGER_FOCUS_FINAL_ITEM  				(AGENT_IMAGER_FOCUS_PROPERTY->items+1)
#define AGENT_IMAGER_FOCUS_BACKLASH_ITEM     	(AGENT_IMAGER_FOCUS_PROPERTY->items+2)
#define AGENT_IMAGER_FOCUS_STACK_ITEM					(AGENT_IMAGER_FOCUS_PROPERTY->items+3)

#define AGENT_IMAGER_DITHERING_PROPERTY				(DEVICE_PRIVATE_DATA->agent_imager_dithering_property)
#define AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM (AGENT_IMAGER_DITHERING_PROPERTY->items+0)
#define AGENT_IMAGER_DITHERING_DELAY_ITEM  		(AGENT_IMAGER_DITHERING_PROPERTY->items+1)

#define AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_imager_download_file_property)
#define AGENT_IMAGER_DOWNLOAD_FILE_ITEM    		(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->items+0)

#define AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_files_property)
#define AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM    (AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items+0)
#define DOWNLOAD_MAX_COUNT										128

#define AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_image_property)
#define AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM    	(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->items+0)

#define AGENT_IMAGER_DELETE_FILE_PROPERTY			(DEVICE_PRIVATE_DATA->agent_imager_delete_file_property)
#define AGENT_IMAGER_DELETE_FILE_ITEM    			(AGENT_IMAGER_DELETE_FILE_PROPERTY->items+0)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_IMAGER_START_EXPOSURE_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_IMAGER_START_STREAMING_ITEM 		(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_IMAGER_START_FOCUSING_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+2)
#define AGENT_IMAGER_START_SEQUENCE_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+3)

#define AGENT_PAUSE_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_pause_process_property)
#define AGENT_PAUSE_PROCESS_ITEM      				(AGENT_PAUSE_PROCESS_PROPERTY->items+0)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_WHEEL_FILTER_PROPERTY						(DEVICE_PRIVATE_DATA->agent_wheel_filter_property)
#define FILTER_SLOT_COUNT											24

#define AGENT_IMAGER_STATS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stats_property)
#define AGENT_IMAGER_STATS_EXPOSURE_ITEM      (AGENT_IMAGER_STATS_PROPERTY->items+0)
#define AGENT_IMAGER_STATS_DELAY_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+1)
#define AGENT_IMAGER_STATS_FRAME_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+2)
#define AGENT_IMAGER_STATS_FRAMES_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+3)
#define AGENT_IMAGER_STATS_BATCH_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+4)
#define AGENT_IMAGER_STATS_BATCHES_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+5)
#define AGENT_IMAGER_STATS_DRIFT_X_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+6)
#define AGENT_IMAGER_STATS_DRIFT_Y_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+7)
#define AGENT_IMAGER_STATS_FWHM_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+8)
#define AGENT_IMAGER_STATS_HFD_ITEM      			(AGENT_IMAGER_STATS_PROPERTY->items+9)
#define AGENT_IMAGER_STATS_PEAK_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+10)

#define AGENT_IMAGER_SELECTION_PROPERTY				(DEVICE_PRIVATE_DATA->agent_selection_property)
#define AGENT_IMAGER_SELECTION_X_ITEM  				(AGENT_IMAGER_SELECTION_PROPERTY->items+0)
#define AGENT_IMAGER_SELECTION_Y_ITEM  				(AGENT_IMAGER_SELECTION_PROPERTY->items+1)

#define AGENT_IMAGER_SEQUENCE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_sequence)
#define AGENT_IMAGER_SEQUENCE_ITEM						(AGENT_IMAGER_SEQUENCE_PROPERTY->items+0)

#define SEQUENCE_SIZE			16

#define SELECTION_RADIUS	9

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct {
	indigo_property *agent_imager_batch_property;
	indigo_property *agent_imager_focus_property;
	indigo_property *agent_imager_dithering_property;
	indigo_property *agent_imager_download_file_property;
	indigo_property *agent_imager_download_files_property;
	indigo_property *agent_imager_download_image_property;
	indigo_property *agent_imager_delete_file_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_pause_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_wheel_filter_property;
	indigo_property *agent_selection_property;
	indigo_property *agent_stats_property;
	indigo_property *agent_sequence;
	indigo_property *agent_sequence_state;
	char current_folder[INDIGO_VALUE_SIZE];
	void *image_buffer;
	int focuser_position;
	indigo_frame_digest reference;
	double drift_x, drift_y;
	int bin_x, bin_y;
	int stack_size;
	pthread_mutex_t mutex;
	double focus_exposure;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void save_config(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
	indigo_save_property(device, NULL, AGENT_IMAGER_BATCH_PROPERTY);
	indigo_save_property(device, NULL, AGENT_IMAGER_FOCUS_PROPERTY);
	indigo_save_property(device, NULL, AGENT_IMAGER_DITHERING_PROPERTY);
	indigo_save_property(device, NULL, AGENT_IMAGER_SEQUENCE_PROPERTY);
	if (DEVICE_CONTEXT->property_save_file_handle) {
		CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		close(DEVICE_CONTEXT->property_save_file_handle);
		DEVICE_CONTEXT->property_save_file_handle = 0;
	} else {
		CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	CONFIG_SAVE_ITEM->sw.value = false;
	indigo_update_property(device, CONFIG_PROPERTY, NULL);
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
}

static int save_switch_state(indigo_device *device, int index, char *name) {
	indigo_property *remote_property = indigo_filter_cached_property(device, index, name);
	for (int i = 0; i < remote_property->count; i++) {
		if (remote_property->items[i].sw.value)
			return i;
	}
	return -1;
}

static void restore_switch_state(indigo_device *device, int index, char *name, int state) {
	if (state >= 0) {
		indigo_property *remote_property = indigo_filter_cached_property(device, index, name);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, remote_property->items[state].name, true);
	}
}

static void set_headers(indigo_device *device) {
	if (!AGENT_WHEEL_FILTER_PROPERTY->hidden) {
		for (int i = 0; i < AGENT_WHEEL_FILTER_PROPERTY->count; i++) {
			indigo_item *item = AGENT_WHEEL_FILTER_PROPERTY->items + i;
			if (item->sw.value) {
				indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_FITS_HEADERS_PROPERTY_NAME, "HEADER_9", "FILTER='%s'", item->label);
				break;
			}
		}
	} else {
		indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_FITS_HEADERS_PROPERTY_NAME, "HEADER_9", "");
	}
	if (*FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX]) {
		indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_FITS_HEADERS_PROPERTY_NAME, "HEADER_10", "FOCUSPOS=%d", DEVICE_PRIVATE_DATA->focuser_position);
	} else {
		indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_FITS_HEADERS_PROPERTY_NAME, "HEADER_10", "");
	}
}

static bool capture_raw_frame(indigo_device *device) {
	indigo_property *remote_exposure_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_EXPOSURE_PROPERTY_NAME);
	if (remote_exposure_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE not found");
		return false;
	}
	indigo_property *remote_image_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_PROPERTY_NAME);
	if (remote_image_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_IMAGE not found");
		return false;
	}
	indigo_property *remote_format_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME);
	if (remote_format_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_IMAGE_FORMAT not found");
		return false;
	}
	for (int item_index = 0; item_index < remote_format_property->count; item_index++) {
		indigo_item *item = remote_format_property->items + item_index;
		if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_RAW_ITEM_NAME))
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_format_property->device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	}
	for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
		double exposure_time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
		while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			indigo_usleep(200000);
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_exposure_property->device, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure_time);
		for (int i = 0; i < 1000 && remote_exposure_property->state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			exposure_attempt--;
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (remote_exposure_property->state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in 1 second");
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		double reported_exposure_time = exposure_time;
		AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		while (remote_exposure_property->state == INDIGO_BUSY_STATE) {
			double c = ceil(reported_exposure_time);
			if (AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value > c) {
				AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = c;
				indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			}
			if (reported_exposure_time > 1) {
				reported_exposure_time -= 0.2;
				indigo_usleep(200000);
			} else {
				reported_exposure_time -= 0.01;
				indigo_usleep(10000);
			}
		}
		AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			exposure_attempt--;
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (remote_exposure_property->state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK");
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		break;
	}
	if (remote_exposure_property->state != INDIGO_OK_STATE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed");
		return false;
	}
	if (AGENT_IMAGER_SELECTION_X_ITEM->number.value > 0 && AGENT_IMAGER_SELECTION_X_ITEM->number.value > 0) {
		if (strchr(remote_image_property->device, '@'))
			indigo_populate_http_blob_item(remote_image_property->items);
		indigo_raw_header *header = (indigo_raw_header *)(remote_image_property->items->blob.value);
		if (header && (header->signature == INDIGO_RAW_MONO8 || header->signature == INDIGO_RAW_MONO16 || header->signature == INDIGO_RAW_RGB24 || header->signature == INDIGO_RAW_RGB48)) {
			if (AGENT_IMAGER_STATS_FRAME_ITEM->number.value == 0) {
				indigo_result result;
				indigo_delete_frame_digest(&DEVICE_PRIVATE_DATA->reference);
				result = indigo_selection_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), &AGENT_IMAGER_SELECTION_X_ITEM->number.value, &AGENT_IMAGER_SELECTION_Y_ITEM->number.value, SELECTION_RADIUS, header->width, header->height, &DEVICE_PRIVATE_DATA->reference);
				if (result == INDIGO_OK)
					indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
				if (result == INDIGO_OK) {
					AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
				} else {
					return false;
				}
			} else {
				indigo_frame_digest digest;
				indigo_result result;
				result = indigo_selection_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), &AGENT_IMAGER_SELECTION_X_ITEM->number.value, &AGENT_IMAGER_SELECTION_Y_ITEM->number.value, SELECTION_RADIUS, header->width, header->height, &digest);
				if (result == INDIGO_OK)
					indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
				if (result == INDIGO_OK) {
					result = indigo_calculate_drift(&DEVICE_PRIVATE_DATA->reference, &digest, &DEVICE_PRIVATE_DATA->drift_x, &DEVICE_PRIVATE_DATA->drift_y);
					if (result == INDIGO_OK) {
						AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
						AGENT_IMAGER_STATS_DRIFT_X_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_x) / 1000;
						AGENT_IMAGER_STATS_DRIFT_Y_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_y) / 1000;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift %.4gpx, %.4gpx", DEVICE_PRIVATE_DATA->drift_x, DEVICE_PRIVATE_DATA->drift_y);
					}
				} else {
					return false;
				}
				indigo_delete_frame_digest(&digest);
			}
			indigo_selection_psf(header->signature, (void*)header + sizeof(indigo_raw_header), AGENT_IMAGER_SELECTION_X_ITEM->number.value, AGENT_IMAGER_SELECTION_Y_ITEM->number.value, 4, header->width, header->height, &AGENT_IMAGER_STATS_FWHM_ITEM->number.value, &AGENT_IMAGER_STATS_HFD_ITEM->number.value, &AGENT_IMAGER_STATS_PEAK_ITEM->number.value);
		} else {
			indigo_send_message(device, "Invalid image format, only RAW is supported");
			return false;
		}
	}
	return true;
}

static bool exposure_batch(indigo_device *device) {
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_property *remote_exposure_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_EXPOSURE_PROPERTY_NAME);
	if (remote_exposure_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE not found");
		return false;
	}
	set_headers(device);
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value++;
	for (int remaining_exposures = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target; remaining_exposures != 0; remaining_exposures--) {
		AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (remaining_exposures < 0)
			remaining_exposures = -1;
		for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
			double exposure_time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_exposure_property->device, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure_time);
			for (int i = 0; i < 1000 && remote_exposure_property->state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
				indigo_usleep(1000);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				exposure_attempt--;
				continue;
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			if (remote_exposure_property->state != INDIGO_BUSY_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in 1 second");
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			double reported_exposure_time = exposure_time;
			AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			while (remote_exposure_property->state == INDIGO_BUSY_STATE) {
				if (reported_exposure_time < floor(AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value)) {
					double c = ceil(reported_exposure_time);
					if (AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value > c) {
						AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = c;
						indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
					}
				}
				if (reported_exposure_time > 1) {
					reported_exposure_time -= 0.2;
					indigo_usleep(200000);
				} else {
					reported_exposure_time -= 0.01;
					indigo_usleep(10000);
				}
			}
			AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				exposure_attempt--;
				continue;
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			if (remote_exposure_property->state != INDIGO_OK_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK");
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			break;
		}
		if (remote_exposure_property->state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed");
			return false;
		}
		if (remaining_exposures > 1 || AGENT_IMAGER_DITHERING_DELAY_ITEM->number.target < 0) {
			double delay_time = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
			if (AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM->number.target != 0) {
				for (int item_index = 0; item_index < FILTER_DEVICE_CONTEXT->filter_related_agent_list_property->count; item_index++) {
					indigo_item *agent = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property->items + item_index;
					if (agent->sw.value && !strncmp(agent->name, "Guider Agent", 12)) {
						const char *item_names[] = { AGENT_GUIDER_SETTINGS_DITH_X_ITEM_NAME, AGENT_GUIDER_SETTINGS_DITH_Y_ITEM_NAME };
						double x_value = fabs(AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM->number.target) * (2 * drand48() - 1);
						double y_value = AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM->number.target > 0 ? AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM->number.target * (2 * drand48() - 1) : 0;
						double item_values[] = { x_value, y_value };
						indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, agent->name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 2, item_names, item_values);
						delay_time = MAX(AGENT_IMAGER_BATCH_DELAY_ITEM->number.target, fabs(AGENT_IMAGER_DITHERING_DELAY_ITEM->number.target));
						break;
					}
				}
			}
			double reported_delay_time = delay_time;
			AGENT_IMAGER_STATS_DELAY_ITEM->number.value = reported_delay_time;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			while (reported_delay_time > 0) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				if (reported_delay_time < floor(AGENT_IMAGER_STATS_DELAY_ITEM->number.value)) {
					double c = ceil(reported_delay_time);
					if (AGENT_IMAGER_STATS_DELAY_ITEM->number.value > c) {
						AGENT_IMAGER_STATS_DELAY_ITEM->number.value = c;
						indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
					}
				}
				if (reported_delay_time > 1) {
					reported_delay_time -= 0.2;
					indigo_usleep(200000);
				} else {
					reported_delay_time -= 0.01;
					indigo_usleep(10000);
				}
			}
			AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		}
	}
	return true;
}

static void exposure_batch_process(indigo_device *device) {
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 1;
	if (exposure_batch(device)) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Batch finished");
	} else {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Batch failed");
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
}

static bool streaming_batch(indigo_device *device) {
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_property *remote_streaming_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_STREAMING_PROPERTY_NAME);
	if (remote_streaming_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE not found");
		return false;
	}
	set_headers(device);
	int count_index = -1;
	for (int i = 0; i < remote_streaming_property->count; i++) {
		if (!strcmp(remote_streaming_property->items[i].name, CCD_STREAMING_COUNT_ITEM_NAME))
			count_index = i;
	}
	if (count_index == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_STREAMING_COUNT_ITEM not found in CCD_STREAMING_PROPERTY");
		return false;
	}
	char const *names[] = { AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME };
	double values[] = { AGENT_IMAGER_BATCH_COUNT_ITEM->number.target, AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target };
	indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, remote_streaming_property->device, CCD_STREAMING_PROPERTY_NAME, 2, names, values);
	for (int i = 0; i < 1000 && remote_streaming_property->state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
		indigo_usleep(1000);
	if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		return false;
	if (remote_streaming_property->state != INDIGO_BUSY_STATE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_STREAMING_PROPERTY didn't become busy in 1 second");
		return false;
	}
	while (remote_streaming_property->state == INDIGO_BUSY_STATE) {
		indigo_usleep(20000);
		int count = remote_streaming_property->items[count_index].number.value;
		if (count != AGENT_IMAGER_STATS_FRAME_ITEM->number.value) {
			AGENT_IMAGER_STATS_FRAME_ITEM->number.value = count;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		}
	}
	if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		return false;
	return true;
}

static void streaming_batch_process(indigo_device *device) {
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 1;
	if (streaming_batch(device)) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Streaming finished");
	} else {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Streaming failed");
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
}

static bool autofocus(indigo_device *device) {
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	double last_quality = 0;
	double steps = AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.value;
	double steps_with_backlash = steps + AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
	bool moving_out = true, first_move = true;
	indigo_property *remote_upload_mode_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_UPLOAD_MODE_PROPERTY_NAME);
	if (remote_upload_mode_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_UPLOAD_MODE_PROPERTY_NAME not found");
		return false;
	}
	indigo_property *remote_steps_property = indigo_filter_cached_property(device, INDIGO_FILTER_FOCUSER_INDEX, FOCUSER_STEPS_PROPERTY_NAME);
	if (remote_steps_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FOCUSER_STEPS not found");
		return false;
	}
	indigo_property *remote_direction_property = indigo_filter_cached_property(device, INDIGO_FILTER_FOCUSER_INDEX, FOCUSER_DIRECTION_PROPERTY_NAME);
	if (remote_direction_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FOCUSER_DIRECTION_PROPERTY_NAME not found");
		return false;
	}
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_upload_mode_property->device, remote_upload_mode_property->name, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, true);
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_direction_property->device, remote_direction_property->name, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
	while (true) {
		double quality = 0;
		int frame_count = 0;
		for (int i = 0; i < AGENT_IMAGER_FOCUS_STACK_ITEM->number.value; i++) {
			if (!capture_raw_frame(device))
				return false;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Peak = %g, HFD = %g,  FWHM = %g", AGENT_IMAGER_STATS_PEAK_ITEM->number.value, AGENT_IMAGER_STATS_HFD_ITEM->number.value, AGENT_IMAGER_STATS_FWHM_ITEM->number.value);
			if (AGENT_IMAGER_STATS_HFD_ITEM->number.value == 0 || AGENT_IMAGER_STATS_FWHM_ITEM->number.value == 0)
				continue;
			quality += AGENT_IMAGER_STATS_PEAK_ITEM->number.value / AGENT_IMAGER_STATS_FWHM_ITEM->number.value / AGENT_IMAGER_STATS_HFD_ITEM->number.value;
			frame_count++;
		}
		if (quality == 0) {
			indigo_send_message(device, "Failed to evaluate quality");
			return false;
		}
		quality /= frame_count;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Quality = %g", quality);
		if (quality > last_quality) {
			if (moving_out)
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving out %d steps", (int)steps);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving in %d steps", (int)steps);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_steps_property->device, remote_steps_property->name, FOCUSER_STEPS_ITEM_NAME, steps);
		} else if (steps <= AGENT_IMAGER_FOCUS_FINAL_ITEM->number.value) {
			moving_out = !moving_out;
			if (moving_out) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving out %d steps to final position", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_direction_property->device, remote_direction_property->name, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving in %d steps to final position", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_direction_property->device, remote_direction_property->name, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			}
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_steps_property->device, remote_steps_property->name, FOCUSER_STEPS_ITEM_NAME, steps_with_backlash);
			break;
		} else {
			moving_out = !moving_out;
			if (!first_move) {
				steps = round(steps / 2);
				if (steps < 1)
					steps = 1;
				steps_with_backlash = steps + AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
			}
			first_move = false;
			if (moving_out) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving out %d steps", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_direction_property->device, remote_direction_property->name, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving in %d steps", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_direction_property->device, remote_direction_property->name, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			}
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_steps_property->device, remote_steps_property->name, FOCUSER_STEPS_ITEM_NAME, steps_with_backlash);
		}
		for (int i = 0; i < 1000 && remote_steps_property->state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (remote_steps_property->state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FOCUSER_STEPS_PROPERTY didn't become busy in 1 second");
			return false;
		}
		while (remote_steps_property->state == INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		last_quality = quality;
	}
	while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		indigo_usleep(200000);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		return false;
	return capture_raw_frame(device);
}

static void autofocus_process(indigo_device *device) {
	int upload_mode = save_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_UPLOAD_MODE_PROPERTY_NAME);
	int image_format = save_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME);
	if (autofocus(device)) {
			AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Focusing finished");
		} else {
			AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Focusing failed");
	}
	restore_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	restore_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
}

static void set_property(indigo_device *device, char *name, char *value) {
	indigo_property *remote_property = NULL;
	if (!strcasecmp(name, "focus")) {
		DEVICE_PRIVATE_DATA->focus_exposure = atof(value);
	} else if (!strcasecmp(name, "count")) {
		AGENT_IMAGER_BATCH_COUNT_ITEM->number.target = atoi(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "exposure")) {
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = atof(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "delay")) {
		AGENT_IMAGER_BATCH_DELAY_ITEM->number.target = atof(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "filter")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_WHEEL_INDEX, WHEEL_SLOT_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < AGENT_WHEEL_FILTER_PROPERTY->count; j++) {
				indigo_item *item = AGENT_WHEEL_FILTER_PROPERTY->items + j;
				if (!strcasecmp(value, item->label) || !strcasecmp(value, item->name)) {
					indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, WHEEL_SLOT_ITEM_NAME, j + 1);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "mode")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_MODE_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "name")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_LOCAL_MODE_PROPERTY_NAME);
		if (remote_property)
			indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_LOCAL_MODE_PREFIX_ITEM_NAME, value);
	} else if (!strcasecmp(name, "gain")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_GAIN_PROPERTY_NAME);
		if (remote_property)
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_GAIN_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "offset")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_OFFSET_PROPERTY_NAME);
		if (remote_property)
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_OFFSET_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "gamma")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_GAMMA_PROPERTY_NAME);
		if (remote_property)
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_GAMMA_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "temperature")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_TEMPERATURE_PROPERTY_NAME);
		if (remote_property)
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_TEMPERATURE_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "cooler")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_COOLER_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "frame")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_FRAME_TYPE_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "aperture")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_APERTURE_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "shutter")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_SHUTTER_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "iso")) {
		remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_ISO_PROPERTY_NAME);
		if (remote_property) {
			for (int j = 0; j < remote_property->count; j++) {
				indigo_item *item = remote_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, item->name, true);
					break;
				}
			}
		}
	}
	if (remote_property) {
		indigo_usleep(200000);
		while (remote_property->state == INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
	}
}

static void sequence_process(indigo_device *device) {
	char sequence_text[INDIGO_VALUE_SIZE], *sequence_text_pnt, *value;
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 0;
	DEVICE_PRIVATE_DATA->focus_exposure = 0;
	strncpy(sequence_text, AGENT_IMAGER_SEQUENCE_ITEM->text.value, INDIGO_VALUE_SIZE);
	bool autofocus_requested = strstr(sequence_text, "focus") != NULL;
	for (char *token = strtok_r(sequence_text, ";", &sequence_text_pnt); token; token = strtok_r(NULL, ";", &sequence_text_pnt)) {
		if (strchr(token, '='))
			continue;
		int batch_index = atoi(token);
		if (batch_index < 1 || batch_index > SEQUENCE_SIZE)
			continue;
		AGENT_IMAGER_STATS_BATCHES_ITEM->number.value++;
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "focus") != NULL) {
			autofocus_requested = true;
		}
	}
	if (autofocus_requested) {
		if (AGENT_IMAGER_SELECTION_X_ITEM->number.value == 0 && AGENT_IMAGER_SELECTION_Y_ITEM->number.value == 0) {
			AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Autofocus requested, but no star is selected!");
		}
		if (*FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX] == 0) {
			AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Autofocus requested, but no focuser is selected!");
		}
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_ALERT_STATE) {
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			return;
		}
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	strncpy(sequence_text, AGENT_IMAGER_SEQUENCE_ITEM->text.value, INDIGO_VALUE_SIZE);
	for (char *token = strtok_r(sequence_text, ";", &sequence_text_pnt); token; token = strtok_r(NULL, ";", &sequence_text_pnt)) {
		value = strchr(token, '=');
		if (value) {
			*value++ = 0;
			set_property(device, token, value);
			continue;
		}
		int batch_index = atoi(token);
		if (batch_index < 1 || batch_index > SEQUENCE_SIZE)
			continue;
		char batch_text[INDIGO_VALUE_SIZE], *batch_text_pnt;
		strncpy(batch_text, AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, INDIGO_VALUE_SIZE);
		for (char *token = strtok_r(batch_text, ";", &batch_text_pnt); token; token = strtok_r(NULL, ";", &batch_text_pnt)) {
			value = strchr(token, '=');
			if (value == NULL) {
				continue;
			}
			*value++ = 0;
			set_property(device, token, value);
		}
		AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
		AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (DEVICE_PRIVATE_DATA->focus_exposure > 0) {
			int upload_mode = save_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_UPLOAD_MODE_PROPERTY_NAME);
			int image_format = save_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME);
			double exposure = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = DEVICE_PRIVATE_DATA->focus_exposure;
			indigo_send_message(device, "Autofocus started");
			if (autofocus(device)) {
				indigo_send_message(device, "Autofocus finished");
			} else {
				indigo_send_message(device, "Autofocus failed");
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			restore_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
			restore_switch_state(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
			AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = exposure;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			DEVICE_PRIVATE_DATA->focus_exposure = 0;
		}		
		if (exposure_batch(device)) {
			indigo_send_message(device, "Batch %d finished", batch_index);
		} else {
			indigo_send_message(device, "Batch %d failed", batch_index);
			AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
	}
	if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Sequence finished");
	} else {
		indigo_send_message(device, "Sequence failed");
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
}

static void abort_process(indigo_device *device) {
	indigo_property *remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_ABORT_EXPOSURE_PROPERTY_NAME);
	if (remote_property)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
	remote_property = indigo_filter_cached_property(device, INDIGO_FILTER_FOCUSER_INDEX, FOCUSER_ABORT_MOTION_PROPERTY_NAME);
	if (remote_property)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_property->device, remote_property->name, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
}

static void setup_download(indigo_device *device) {
	if (*DEVICE_PRIVATE_DATA->current_folder) {
		indigo_delete_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
		DIR *folder;
		struct dirent *entry;
		folder = opendir(DEVICE_PRIVATE_DATA->current_folder);
		if (folder) {
			int index = 1;
			while ((entry = readdir(folder)) != NULL && index <= DOWNLOAD_MAX_COUNT) {
				if (strstr(entry->d_name, ".fits") || strstr(entry->d_name, ".xisf") || strstr(entry->d_name, ".raw") || strstr(entry->d_name, ".jpeg")) {
					indigo_init_switch_item(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + index, entry->d_name, entry->d_name, false);
					index++;
				}
			}
			AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count = index;
			closedir(folder);
		}
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->hidden = false;
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
	}
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_CCD_LIST_PROPERTY->hidden = false;
		FILTER_WHEEL_LIST_PROPERTY->hidden = false;
		FILTER_FOCUSER_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- Batch properties
		AGENT_IMAGER_BATCH_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_BATCH_PROPERTY_NAME, "Agent", "Batch settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_IMAGER_BATCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_BATCH_COUNT_ITEM, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, "Frame count", -1, 0xFFFF, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_EXPOSURE_ITEM, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME, "Exposure time", 0, 0xFFFF, 0, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_DELAY_ITEM, AGENT_IMAGER_BATCH_DELAY_ITEM_NAME, "Delay after each exposure", 0, 0xFFFF, 0, 0);
		// -------------------------------------------------------------------------------- Focus properties
		AGENT_IMAGER_FOCUS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_FOCUS_PROPERTY_NAME, "Agent", "Autofocus settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AGENT_IMAGER_FOCUS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_FOCUS_INITIAL_ITEM, AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME, "Initial step", 0, 0xFFFF, 1, 20);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_FINAL_ITEM, AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME, "Final step", 0, 0xFFFF, 1, 5);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_BACKLASH_ITEM, AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME, "Backlash", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_STACK_ITEM, AGENT_IMAGER_FOCUS_STACK_ITEM_NAME, "Stacking", 1, 5, 1, 3);
		// -------------------------------------------------------------------------------- Dithering properties
		AGENT_IMAGER_DITHERING_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_DITHERING_PROPERTY_NAME, "Agent", "Dithering settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_IMAGER_DITHERING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM, AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM_NAME, "Aggressivity (px)", -10, 10, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_DITHERING_DELAY_ITEM, AGENT_IMAGER_DITHERING_DELAY_ITEM_NAME, "Settle down delay (s)", -60, 60, 1, 5);
		// -------------------------------------------------------------------------------- Process properties
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_START_PROCESS_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_IMAGER_START_EXPOSURE_ITEM, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME, "Start batch", false);
		indigo_init_switch_item(AGENT_IMAGER_START_STREAMING_ITEM, AGENT_IMAGER_START_STREAMING_ITEM_NAME, "Start streaming", false);
		indigo_init_switch_item(AGENT_IMAGER_START_FOCUSING_ITEM, AGENT_IMAGER_START_FOCUSING_ITEM_NAME, "Start focusing", false);
		indigo_init_switch_item(AGENT_IMAGER_START_SEQUENCE_ITEM, AGENT_IMAGER_START_SEQUENCE_ITEM_NAME, "Start sequence", false);
		AGENT_PAUSE_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PAUSE_PROCESS_PROPERTY_NAME, "Agent", "Pause/Resume process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_PAUSE_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_PAUSE_PROCESS_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_PAUSE_PROCESS_ITEM, AGENT_PAUSE_PROCESS_ITEM_NAME, "Pause/resume process", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_ABORT_PROCESS_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort process", false);
		// -------------------------------------------------------------------------------- Download properties
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME, "Agent", "Download image", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->hidden = true;
		indigo_init_text_item(AGENT_IMAGER_DOWNLOAD_FILE_ITEM, AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME, "File name", "");
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY_NAME, "Agent", "Download image list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, DOWNLOAD_MAX_COUNT + 1);
		if (AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM, AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM_NAME, "Refresh", false);
		AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME, "Agent", "Download image data", INDIGO_OK_STATE, 1);
		if (AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->hidden = true;
		indigo_init_blob_item(AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM_NAME, "Image");
		AGENT_IMAGER_DELETE_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_DELETE_FILE_PROPERTY_NAME, "Agent", "Delete image", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_DELETE_FILE_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_IMAGER_DELETE_FILE_PROPERTY->hidden = true;
		indigo_init_text_item(AGENT_IMAGER_DELETE_FILE_ITEM, AGENT_IMAGER_DELETE_FILE_ITEM_NAME, "File name", "");
		// -------------------------------------------------------------------------------- Wheel helpers
		AGENT_WHEEL_FILTER_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_WHEEL_FILTER_PROPERTY_NAME, "Agent", "Selected filter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, FILTER_SLOT_COUNT);
		if (AGENT_WHEEL_FILTER_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 0; i < FILTER_SLOT_COUNT; i++) {
			char name[8], label[32];
			sprintf(name, "%d", i + 1);
			sprintf(label, "Filter #%d", i + 1);
			indigo_init_switch_item(AGENT_WHEEL_FILTER_PROPERTY->items + i, name, label, false);
		}
		AGENT_WHEEL_FILTER_PROPERTY->count = 0;
		AGENT_WHEEL_FILTER_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- Selected star
		AGENT_IMAGER_SELECTION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_SELECTION_PROPERTY_NAME, "Agent", "Selection", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_IMAGER_SELECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_SELECTION_X_ITEM, AGENT_IMAGER_SELECTION_X_ITEM_NAME, "Selection X (px)", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_Y_ITEM, AGENT_IMAGER_SELECTION_Y_ITEM_NAME, "Selection Y (px)", 0, 0xFFFF, 0, 0);
		// -------------------------------------------------------------------------------- Focusing stats
		AGENT_IMAGER_STATS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_STATS_PROPERTY_NAME, "Agent", "Stats", INDIGO_OK_STATE, INDIGO_RO_PERM, 11);
		if (AGENT_IMAGER_STATS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_STATS_EXPOSURE_ITEM, AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME, "Elapsed exposure", 0, 3600, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DELAY_ITEM, AGENT_IMAGER_STATS_DELAY_ITEM_NAME, "Elapsed delay", 0, 3600, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAME_ITEM, AGENT_IMAGER_STATS_FRAME_ITEM_NAME, "Current frame", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAMES_ITEM, AGENT_IMAGER_STATS_FRAMES_ITEM_NAME, "Frame count", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BATCH_ITEM, AGENT_IMAGER_STATS_BATCH_ITEM_NAME, "Current batch", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BATCHES_ITEM, AGENT_IMAGER_STATS_BATCHES_ITEM_NAME, "Batch count", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_X_ITEM, AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME, "Drift X", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_Y_ITEM, AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME, "Drift Y", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FWHM_ITEM, AGENT_IMAGER_STATS_FWHM_ITEM_NAME, "FWHM", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_HFD_ITEM, AGENT_IMAGER_STATS_HFD_ITEM_NAME, "HFD", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_PEAK_ITEM, AGENT_IMAGER_STATS_PEAK_ITEM_NAME, "Peak", 0, 0xFFFF, 0, 0);
		// -------------------------------------------------------------------------------- Sequencer
		AGENT_IMAGER_SEQUENCE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_SEQUENCE_PROPERTY_NAME, "Agent", "Sequence", INDIGO_OK_STATE, INDIGO_RW_PERM, 1 + SEQUENCE_SIZE);
		if (AGENT_IMAGER_SEQUENCE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_IMAGER_SEQUENCE_ITEM, AGENT_IMAGER_SEQUENCE_ITEM_NAME, "Sequence", "");
		for (int i = 1; i <= SEQUENCE_SIZE; i++) {
			char name[32], label[32];
			sprintf(name, "%02d", i);
			sprintf(label, "Batch #%d", i);
			indigo_init_text_item(AGENT_IMAGER_SEQUENCE_PROPERTY->items + i, name, label, "");
		}
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
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
	if (indigo_property_match(AGENT_IMAGER_BATCH_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_FOCUS_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_FOCUS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DITHERING_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DITHERING_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DELETE_FILE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PAUSE_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_SELECTION_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_STATS_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_SEQUENCE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_SEQUENCE_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_IMAGER_BATCH_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_IMAGER_BATCH
		indigo_property_copy_values(AGENT_IMAGER_BATCH_PROPERTY, property, false);
		AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_FOCUS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_FOCUS
		indigo_property_copy_values(AGENT_IMAGER_FOCUS_PROPERTY, property, false);
		AGENT_IMAGER_FOCUS_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_FOCUS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_DITHERING_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- AGENT_DITHERING
		indigo_property_copy_values(AGENT_IMAGER_DITHERING_PROPERTY, property, false);
		AGENT_IMAGER_DITHERING_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_DITHERING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_SELECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_SELECTION
		indigo_property_copy_values(AGENT_IMAGER_SELECTION_PROPERTY, property, false);
		DEVICE_PRIVATE_DATA->stack_size = 0;
		AGENT_IMAGER_SELECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, exposure_batch_process, NULL);
			} else if (AGENT_IMAGER_START_STREAMING_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, streaming_batch_process, NULL);
			} else if (AGENT_IMAGER_START_FOCUSING_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, autofocus_process, NULL);
			} else if (AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, sequence_process, NULL);
			}
		}
		AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = AGENT_IMAGER_START_STREAMING_ITEM->sw.value = AGENT_IMAGER_START_FOCUSING_ITEM->sw.value = AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_PAUSE_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_PAUSE_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_PAUSE_PROCESS_PROPERTY, property, false);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
				abort_process(device);
			}
		} else {
			AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		AGENT_PAUSE_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
			}
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			abort_process(device);
		} else {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILE
	} else 	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property, false);
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
			if (!strcmp(item->name, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value)) {
				char file_name[INDIGO_VALUE_SIZE + INDIGO_NAME_SIZE];
				struct stat file_stat;
				strcpy(file_name, DEVICE_PRIVATE_DATA->current_folder);
				strcat(file_name, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value);
				if (stat(file_name, &file_stat) < 0) {
					break;
				}
				if (DEVICE_PRIVATE_DATA->image_buffer)
					DEVICE_PRIVATE_DATA->image_buffer = realloc(DEVICE_PRIVATE_DATA->image_buffer, file_stat.st_size);
				else
					DEVICE_PRIVATE_DATA->image_buffer = malloc(file_stat.st_size);
				int fd = open(file_name, O_RDONLY, 0);
				if (fd == -1) {
					break;
				}
				int result = indigo_read(fd, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.value = DEVICE_PRIVATE_DATA->image_buffer, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.size = file_stat.st_size);
				close(fd);
				if (result == -1) {
					AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
					break;
				}
				*AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.url = 0;
				*AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.format = 0;
				char *file_type = strrchr(file_name, '.');
				if (file_type)
					strcpy(AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.format, file_type);
				AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
				AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DELETE_FILE
	} else 	if (indigo_property_match(AGENT_IMAGER_DELETE_FILE_PROPERTY, property)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_property_copy_values(AGENT_IMAGER_DELETE_FILE_PROPERTY, property, false);
		AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
		AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
			if (!strcmp(item->name, AGENT_IMAGER_DELETE_FILE_ITEM->text.value)) {
				char file_name[INDIGO_VALUE_SIZE + INDIGO_NAME_SIZE];
				struct stat file_stat;
				strcpy(file_name, DEVICE_PRIVATE_DATA->current_folder);
				strcat(file_name, AGENT_IMAGER_DELETE_FILE_ITEM->text.value);
				if (stat(file_name, &file_stat) < 0) {
					break;
				}
				indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
				if (unlink(file_name) == -1) {
					break;
				}
				AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		setup_download(device);
		indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILES
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property, false);
		if (AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value) {
			AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value = false;
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
			setup_download(device);
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		} else {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
			indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
			for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
				indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
				if (item->sw.value) {
					item->sw.value = false;
					AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
					pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
					return indigo_change_text_property_1(client, device->name, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME, AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME, item->name);
				}
			}
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- AGENT_WHEEL_FILTER
	} else 	if (indigo_property_match(AGENT_WHEEL_FILTER_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_WHEEL_FILTER_PROPERTY, property, false);
		for (int i = 0; i < AGENT_WHEEL_FILTER_PROPERTY->count; i++) {
			if (AGENT_WHEEL_FILTER_PROPERTY->items[i].sw.value) {
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX], WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, i + 1);
				break;
			}
		}
		AGENT_WHEEL_FILTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_WHEEL_FILTER_PROPERTY,NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- AGENT_IMAGER_SEQUENCE
	} else 	if (indigo_property_match(AGENT_IMAGER_SEQUENCE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_SEQUENCE_PROPERTY, property, false);
		AGENT_IMAGER_SEQUENCE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_SEQUENCE_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_IMAGER_BATCH_PROPERTY);
	indigo_release_property(AGENT_IMAGER_FOCUS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DITHERING_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DELETE_FILE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SELECTION_PROPERTY);
	indigo_release_property(AGENT_IMAGER_STATS_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_PAUSE_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SEQUENCE_PROPERTY);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	if (DEVICE_PRIVATE_DATA->image_buffer)
		free(DEVICE_PRIVATE_DATA->image_buffer);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX])) {
		if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_LOCAL_MODE_PROPERTY_NAME)) {
			*CLIENT_PRIVATE_DATA->current_folder = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_LOCAL_MODE_DIR_ITEM_NAME) == 0) {
					strncpy(CLIENT_PRIVATE_DATA->current_folder, item->text.value, INDIGO_VALUE_SIZE);
					break;
				}
			}
			CLIENT_PRIVATE_DATA->agent_imager_download_file_property->hidden = false;
			indigo_define_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_download_file_property, NULL);
			CLIENT_PRIVATE_DATA->agent_imager_download_image_property->hidden = false;
			indigo_define_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_download_image_property, NULL);
			CLIENT_PRIVATE_DATA->agent_imager_delete_file_property->hidden = false;
			indigo_define_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_delete_file_property, NULL);
			pthread_mutex_lock(&CLIENT_PRIVATE_DATA->mutex);
			setup_download(FILTER_CLIENT_CONTEXT->device);
			pthread_mutex_unlock(&CLIENT_PRIVATE_DATA->mutex);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME) == 0) {
					CLIENT_PRIVATE_DATA->bin_x = item->number.value;
				} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
					CLIENT_PRIVATE_DATA->bin_y = item->number.value;
				}
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX]) && !strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		indigo_property *agent_wheel_filter_property = CLIENT_PRIVATE_DATA->agent_wheel_filter_property;
		agent_wheel_filter_property->count = property->count;
		for (int i = 0; i < property->count; i++)
			strcpy(agent_wheel_filter_property->items[i].label, property->items[i].text.value);
		agent_wheel_filter_property->hidden = false;
		indigo_define_property(FILTER_CLIENT_CONTEXT->device, agent_wheel_filter_property, NULL);
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX]) && !strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		indigo_property *agent_wheel_filter_property = CLIENT_PRIVATE_DATA->agent_wheel_filter_property;
		int value = property->items->number.value;
		if (value)
			indigo_set_switch(agent_wheel_filter_property, agent_wheel_filter_property->items + value - 1, true);
		else
			indigo_set_switch(agent_wheel_filter_property, agent_wheel_filter_property->items, false);
		agent_wheel_filter_property->state = property->state;
		indigo_update_property(FILTER_CLIENT_CONTEXT->device, agent_wheel_filter_property, NULL);
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX]) && !strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->focuser_position = property->items[0].number.value;
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->device, IMAGER_AGENT_NAME) && !strcmp(property->name, FILTER_CCD_LIST_PROPERTY_NAME)) {
		if (property->items->sw.value) {
			abort_process(device);
			if (!AGENT_START_PROCESS_PROPERTY->hidden) {
				indigo_delete_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				AGENT_START_PROCESS_PROPERTY->hidden = true;
			}
			if (!AGENT_PAUSE_PROCESS_PROPERTY->hidden) {
				indigo_delete_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
				AGENT_PAUSE_PROCESS_PROPERTY->hidden = true;
			}
			if (!AGENT_ABORT_PROCESS_PROPERTY->hidden) {
				indigo_delete_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
				AGENT_ABORT_PROCESS_PROPERTY->hidden = true;
			}
		} else {
			if (AGENT_START_PROCESS_PROPERTY->hidden) {
				AGENT_START_PROCESS_PROPERTY->hidden = false;
				indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->hidden) {
				AGENT_PAUSE_PROCESS_PROPERTY->hidden = false;
				indigo_define_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->hidden) {
				AGENT_ABORT_PROCESS_PROPERTY->hidden = false;
				indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX])) {
		if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "TBD: plate solve etc...");
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_FILE_PROPERTY_NAME)) {
			pthread_mutex_lock(&CLIENT_PRIVATE_DATA->mutex);
			setup_download(FILTER_CLIENT_CONTEXT->device);
			pthread_mutex_unlock(&CLIENT_PRIVATE_DATA->mutex);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_LOCAL_MODE_PROPERTY_NAME)) {
			*CLIENT_PRIVATE_DATA->current_folder = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_LOCAL_MODE_DIR_ITEM_NAME) == 0) {
					strncpy(CLIENT_PRIVATE_DATA->current_folder, item->text.value, INDIGO_VALUE_SIZE);
					break;
				}
			}
			pthread_mutex_lock(&CLIENT_PRIVATE_DATA->mutex);
			setup_download(FILTER_CLIENT_CONTEXT->device);
			pthread_mutex_unlock(&CLIENT_PRIVATE_DATA->mutex);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
			indigo_property *agent_selection_property = CLIENT_PRIVATE_DATA->agent_selection_property;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME) == 0) {
					double ratio = CLIENT_PRIVATE_DATA->bin_x / item->number.target;
					agent_selection_property->items[0].number.value = agent_selection_property->items[0].number.target *= ratio;
					CLIENT_PRIVATE_DATA->bin_x = item->number.value;
				} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
					double ratio = CLIENT_PRIVATE_DATA->bin_y / item->number.target;
					agent_selection_property->items[1].number.value = agent_selection_property->items[1].number.target *= ratio;
					CLIENT_PRIVATE_DATA->bin_y = item->number.value;
				}
			}
			indigo_update_property(FILTER_CLIENT_CONTEXT->device, agent_selection_property, NULL);
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX]) && !strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		indigo_property *agent_wheel_filter_property = CLIENT_PRIVATE_DATA->agent_wheel_filter_property;
		agent_wheel_filter_property->count = property->count;
		for (int i = 0; i < property->count; i++)
			strcpy(agent_wheel_filter_property->items[i].label, property->items[i].text.value);
		agent_wheel_filter_property->hidden = false;
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, agent_wheel_filter_property, NULL);
		indigo_define_property(FILTER_CLIENT_CONTEXT->device, agent_wheel_filter_property, NULL);
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX]) && !strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		indigo_property *agent_wheel_filter_property = CLIENT_PRIVATE_DATA->agent_wheel_filter_property;
		int value = property->items->number.value;
		if (value)
			indigo_set_switch(agent_wheel_filter_property, agent_wheel_filter_property->items + value - 1, true);
		else
			indigo_set_switch(agent_wheel_filter_property, agent_wheel_filter_property->items, false);
		agent_wheel_filter_property->state = property->state;
		indigo_update_property(FILTER_CLIENT_CONTEXT->device, agent_wheel_filter_property, NULL);
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX]) && !strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->focuser_position = property->items[0].number.value;
	}
	return indigo_filter_update_property(client, device, property, message);
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]) && (!strcmp(property->name, CCD_LOCAL_MODE_PROPERTY_NAME) || !strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME))) {
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_download_file_property, NULL);
		CLIENT_PRIVATE_DATA->agent_imager_download_file_property->hidden = true;
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_download_files_property, NULL);
		CLIENT_PRIVATE_DATA->agent_imager_download_files_property->hidden = true;
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_download_image_property, NULL);
		CLIENT_PRIVATE_DATA->agent_imager_download_image_property->hidden = true;
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_imager_delete_file_property, NULL);
		CLIENT_PRIVATE_DATA->agent_imager_delete_file_property->hidden = true;
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_WHEEL_INDEX]) && !strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_wheel_filter_property, NULL);
	}
	return indigo_filter_delete_property(client, device, property, message);
}
// -------------------------------------------------------------------------------- Initialization

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_imager(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		IMAGER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		IMAGER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, IMAGER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(agent_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(agent_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
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
