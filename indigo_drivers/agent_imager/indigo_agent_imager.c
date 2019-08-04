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

#define DRIVER_VERSION 0x0003
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

#define AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_imager_download_file_property)
#define AGENT_IMAGER_DOWNLOAD_FILE_ITEM    		(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->items+0)

#define AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_files_property)
#define AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM    (AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items+0)
#define DOWNLOAD_MAX_COUNT										128

#define AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_image_property)
#define AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM    	(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->items+0)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_IMAGER_START_EXPOSURE_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_IMAGER_START_STREAMING_ITEM 		(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_IMAGER_START_FOCUSING_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+2)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_WHEEL_FILTER_PROPERTY						(DEVICE_PRIVATE_DATA->agent_wheel_filter_property)
#define FILTER_SLOT_COUNT											24

#define AGENT_IMAGER_SELECTION_PROPERTY				(DEVICE_PRIVATE_DATA->agent_selection_property)
#define AGENT_IMAGER_SELECTION_X_ITEM  				(AGENT_IMAGER_SELECTION_PROPERTY->items+0)
#define AGENT_IMAGER_SELECTION_Y_ITEM  				(AGENT_IMAGER_SELECTION_PROPERTY->items+1)

#define AGENT_IMAGER_STATS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stats_property)
#define AGENT_IMAGER_STATS_FRAME_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+0)
#define AGENT_IMAGER_STATS_DRIFT_X_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+1)
#define AGENT_IMAGER_STATS_DRIFT_Y_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+2)
#define AGENT_IMAGER_STATS_FWHM_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+3)
#define AGENT_IMAGER_STATS_HFD_ITEM      			(AGENT_IMAGER_STATS_PROPERTY->items+4)
#define AGENT_IMAGER_STATS_PEAK_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+5)

#define SELECTION_RADIUS	9

typedef struct {
	indigo_property *agent_imager_batch_property;
	indigo_property *agent_imager_focus_property;
	indigo_property *agent_imager_download_file_property;
	indigo_property *agent_imager_download_files_property;
	indigo_property *agent_imager_download_image_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_wheel_filter_property;
	indigo_property *agent_selection_property;
	indigo_property *agent_stats_property;
	char current_folder[INDIGO_VALUE_SIZE], current_type[16];
	void *image_buffer;
	int focuser_position;
	indigo_frame_digest reference;
	double drift_x, drift_y;
	int stack_size;
	pthread_mutex_t mutex;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void save_config(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
	indigo_save_property(device, NULL, AGENT_IMAGER_BATCH_PROPERTY);
	indigo_save_property(device, NULL, AGENT_IMAGER_FOCUS_PROPERTY);
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

static indigo_property_state capture_raw_frame(indigo_device *device) {
	indigo_property *remote_exposure_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_EXPOSURE_PROPERTY_NAME);
	indigo_property *remote_image_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_PROPERTY_NAME);
	if (remote_exposure_property == NULL || remote_image_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY or CCD_IMAGE_PROPERTY not found");
		return INDIGO_ALERT_STATE;
	} else {
		indigo_property *remote_format_property = indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME);
		for (int i = 0; i < remote_format_property->count; i++) {
			indigo_item *item = remote_format_property->items + i;
			if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_RAW_ITEM_NAME)) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, remote_format_property->device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
			}
		}
		double time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_exposure_property->device, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, time);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
		for (int i = 0; remote_exposure_property->state != INDIGO_BUSY_STATE && i < 1000 && AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (remote_exposure_property->state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in 1 second");
			return INDIGO_ALERT_STATE;
		}
		while (remote_exposure_property->state == INDIGO_BUSY_STATE) {
			if (time > 1) {
				indigo_usleep(ONE_SECOND_DELAY);
				time -= 1;
			} else {
				indigo_usleep(10000);
				time -= 0.01;
			}
		}
		if (remote_exposure_property->state == INDIGO_OK_STATE && AGENT_IMAGER_SELECTION_X_ITEM->number.value >0 && AGENT_IMAGER_SELECTION_X_ITEM->number.value > 0) {
			indigo_raw_header *header = (indigo_raw_header *)(remote_image_property->items->blob.value);
			if (header->signature == INDIGO_RAW_MONO8 || header->signature == INDIGO_RAW_MONO16 || header->signature == INDIGO_RAW_RGB24 || header->signature == INDIGO_RAW_RGB48) {
				if (AGENT_IMAGER_STATS_FRAME_ITEM->number.value == 0) {
					indigo_result result;
					indigo_delete_frame_digest(&DEVICE_PRIVATE_DATA->reference);
					result = indigo_selection_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), &AGENT_IMAGER_SELECTION_X_ITEM->number.value, &AGENT_IMAGER_SELECTION_Y_ITEM->number.value, SELECTION_RADIUS, header->width, header->height, &DEVICE_PRIVATE_DATA->reference);
					if (result == INDIGO_OK)
						indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
					if (result == INDIGO_OK) {
						AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
					} else {
						return INDIGO_ALERT_STATE;
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
						return INDIGO_ALERT_STATE;
					}
					indigo_delete_frame_digest(&digest);
				}
				indigo_selection_psf(header->signature, (void*)header + sizeof(indigo_raw_header), AGENT_IMAGER_SELECTION_X_ITEM->number.value, AGENT_IMAGER_SELECTION_Y_ITEM->number.value, 4, header->width, header->height, &AGENT_IMAGER_STATS_FWHM_ITEM->number.value, &AGENT_IMAGER_STATS_HFD_ITEM->number.value, &AGENT_IMAGER_STATS_PEAK_ITEM->number.value);
			} else {
				indigo_send_message(device, "Invalid image format, only RAW is supported");
				return INDIGO_ALERT_STATE;
			}
		}
		return remote_exposure_property->state;
	}
}

static void exposure_batch(indigo_device *device) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	indigo_property *remote_exposure_property = NULL;
	set_headers(device);
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if (cache[j] && !strcmp(cache[j]->device, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]) && !strcmp(cache[j]->name, CCD_EXPOSURE_PROPERTY_NAME)) {
			remote_exposure_property = cache[j];
			AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			int count = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
			for (int i = count; AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && i != 0; i--) {
				if (i < 0)
					i = -1;
				AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = i;
				double time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
				AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, remote_exposure_property->device, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, time);
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				
				for (int i = 0; remote_exposure_property->state != INDIGO_BUSY_STATE && i < 1000; i++)
					indigo_usleep(1000);
				if (remote_exposure_property->state != INDIGO_BUSY_STATE) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in 1 second");
					break;
				}
				while (remote_exposure_property->state == INDIGO_BUSY_STATE) {
					if (time > 1) {
							indigo_usleep(ONE_SECOND_DELAY);
						time -= 1;
						AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
						indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
					} else {
						indigo_usleep(10000);
						time -= 0.01;
					}
				}
				AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = 0;
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				if (i > 1) {
					time = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
					AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = time;
					indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
					while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && time > 0) {
						if (time > 1) {
								indigo_usleep(ONE_SECOND_DELAY);
							time -= 1;
							AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = time;
							indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
						} else {
							indigo_usleep(10000);
							time -= 0.01;
						}
					}
					AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = 0;
					indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				}
			}
			AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
			AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
			AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE)
				indigo_send_message(device, "Batch finished");
			else
				indigo_send_message(device, "Batch failed");
			return;
		}
	}
	if (remote_exposure_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY not found");
	}
	AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
	AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
	AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	indigo_send_message(device, "Batch failed");
}

static void streaming_batch(indigo_device *device) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	indigo_property *remote_streaming_property = NULL;
	set_headers(device);
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if (cache[j] && !strcmp(cache[j]->device, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]) && !strcmp(cache[j]->name, CCD_STREAMING_PROPERTY_NAME)) {
			remote_streaming_property = cache[j];
			int exposure_index = -1;
			int count_index = -1;
			for (int i = 0; i < remote_streaming_property->count; i++) {
				if (!strcmp(remote_streaming_property->items[i].name, CCD_STREAMING_EXPOSURE_ITEM_NAME))
					exposure_index = i;
				else if (!strcmp(remote_streaming_property->items[i].name, CCD_STREAMING_COUNT_ITEM_NAME))
					count_index = i;
			}
			if (exposure_index == -1 || count_index == -1) {
				indigo_send_message(device, "%s: CCD_STREAMING_EXPOSURE_ITEM or CCD_STREAMING_COUNT_ITEM not found in CCD_STREAMING_PROPERTY", IMAGER_AGENT_NAME);
				break;
			}
			AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			char const *names[] = { AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME };
			double values[] = { AGENT_IMAGER_BATCH_COUNT_ITEM->number.target, AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target };
			indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, remote_streaming_property->device, CCD_STREAMING_PROPERTY_NAME, 2, names, values);
			for (int i = 0; remote_streaming_property->state != INDIGO_BUSY_STATE && i < 1000; i++)
				indigo_usleep(1000);
			if (remote_streaming_property->state != INDIGO_BUSY_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_STREAMING_PROPERTY didn't become busy in 1 second");
				break;
			}
			while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && remote_streaming_property->state == INDIGO_BUSY_STATE) {
				double time = remote_streaming_property->items[exposure_index].number.value;
				if (time > 1) {
					AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
					AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = remote_streaming_property->items[count_index].number.value;
					indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
						indigo_usleep(ONE_SECOND_DELAY);
				} else {
					indigo_usleep(10000);
				}
			}
			AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = 0;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
			AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
			AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
			if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE)
				indigo_send_message(device, "Batch finished");
			else
				indigo_send_message(device, "Batch failed");
			return;
		}
	}
	if (remote_streaming_property == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_STREAMING_PROPERTY not found");
	}
	AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
	AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
	AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	indigo_send_message(device, "Batch failed");
}

static void autofocus(indigo_device *device) {
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_property_state result;
	double last_quality = 0;
	double steps = AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.value;
	double steps_with_backlash = steps + AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
	char *device_name = FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_FOCUSER_INDEX];
	bool moving_out = true, first_move = true;
	indigo_property *remote_steps_property = indigo_filter_cached_property(device, INDIGO_FILTER_FOCUSER_INDEX, FOCUSER_STEPS_PROPERTY_NAME);
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
	while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		double quality = 0;
		for (int i = 0; i < AGENT_IMAGER_FOCUS_STACK_ITEM->number.value; i++) {
			result = capture_raw_frame(device);
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
				goto finished;
			}
			if (result != INDIGO_OK_STATE) {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				goto finished;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Peak = %g, HFD = %g,  FWHM = %g", AGENT_IMAGER_STATS_PEAK_ITEM->number.value, AGENT_IMAGER_STATS_HFD_ITEM->number.value, AGENT_IMAGER_STATS_FWHM_ITEM->number.value);
			if (AGENT_IMAGER_STATS_HFD_ITEM->number.value == 0 || AGENT_IMAGER_STATS_FWHM_ITEM->number.value == 0) {
				indigo_send_message(device, "Invalid HFD or FWHM");
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				goto finished;
			}
			quality += AGENT_IMAGER_STATS_PEAK_ITEM->number.value / AGENT_IMAGER_STATS_FWHM_ITEM->number.value / AGENT_IMAGER_STATS_HFD_ITEM->number.value;
		}
		quality /= AGENT_IMAGER_FOCUS_STACK_ITEM->number.value;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Quality = %g", quality);
		if (quality > last_quality) {
			if (moving_out)
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving out %d steps", (int)steps);
			else
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving in %d steps", (int)steps);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, steps);
		} else if (steps <= AGENT_IMAGER_FOCUS_FINAL_ITEM->number.value) {
			moving_out = !moving_out;
			if (moving_out) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving out %d steps to final position", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving in %d steps to final position", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			}
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, steps_with_backlash);
			indigo_send_message(device, "Automatic focusing is done");
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
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
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving in %d steps", (int)steps_with_backlash);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			}
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, steps_with_backlash);
		}
		indigo_usleep(500000);
		while (remote_steps_property->state == INDIGO_BUSY_STATE) {
			indigo_usleep(500000);
		}
		last_quality = quality;
	}
	capture_raw_frame(device);
finished:
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- Process properties
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_START_PROCESS_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_IMAGER_START_EXPOSURE_ITEM, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME, "Start batch", false);
		indigo_init_switch_item(AGENT_IMAGER_START_STREAMING_ITEM, AGENT_IMAGER_START_STREAMING_ITEM_NAME, "Start streaming", false);
		indigo_init_switch_item(AGENT_IMAGER_START_FOCUSING_ITEM, AGENT_IMAGER_START_FOCUSING_ITEM_NAME, "Start focusing", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_ABORT_PROCESS_PROPERTY->hidden = true;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort batch", false);
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
		AGENT_IMAGER_STATS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_STATS_PROPERTY_NAME, "Agent", "Stats", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AGENT_IMAGER_STATS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAME_ITEM, AGENT_IMAGER_STATS_FRAME_ITEM_NAME, "Frame #", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_X_ITEM, AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME, "Drift X", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_Y_ITEM, AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME, "Drift Y", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FWHM_ITEM, AGENT_IMAGER_STATS_FWHM_ITEM_NAME, "FWHM", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_HFD_ITEM, AGENT_IMAGER_STATS_HFD_ITEM_NAME, "HFD", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_PEAK_ITEM, AGENT_IMAGER_STATS_PEAK_ITEM_NAME, "Peak", 0, 0xFFFF, 0, 0);
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
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_SELECTION_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_STATS_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

static void abort_batch(indigo_device *device) {
	if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	}
}

static void setup_download(indigo_device *device) {
	pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
	if (*DEVICE_PRIVATE_DATA->current_folder && *DEVICE_PRIVATE_DATA->current_type) {
		indigo_delete_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
		DIR *folder;
		struct dirent *entry;
		folder = opendir(DEVICE_PRIVATE_DATA->current_folder);
		if (folder) {
			int index = 1;
			while ((entry = readdir(folder)) != NULL && index <= DOWNLOAD_MAX_COUNT) {
				if (strstr(entry->d_name, DEVICE_PRIVATE_DATA->current_type)) {
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
	pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
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
	} else if (indigo_property_match(AGENT_IMAGER_SELECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_SELECTION
		indigo_property_copy_values(AGENT_IMAGER_SELECTION_PROPERTY, property, false);
		DEVICE_PRIVATE_DATA->stack_size = 0;
		AGENT_IMAGER_SELECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		if (*FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
				if (AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value) {
					AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, exposure_batch);
				} else if (AGENT_IMAGER_START_STREAMING_ITEM->sw.value) {
					AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, streaming_batch);
				} else if (AGENT_IMAGER_START_FOCUSING_ITEM->sw.value) {
					AGENT_IMAGER_START_FOCUSING_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, autofocus);
				}
			}
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		} else {
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "%s: No CCD is selected", IMAGER_AGENT_NAME);
		}
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (*FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			abort_batch(device);
			AGENT_ABORT_PROCESS_ITEM->sw.value = false;
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		} else {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, "%s: No CCD is selected", IMAGER_AGENT_NAME);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILE
	} else 	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property, false);
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
			if (!strcmp(item->name, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value)) {
				char file_name[INDIGO_VALUE_SIZE + INDIGO_NAME_SIZE];
				struct stat file_stat;
				strcpy(file_name, DEVICE_PRIVATE_DATA->current_folder);
				strcat(file_name, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value);
				if (stat(file_name, &file_stat) < 0) {
					AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				if (DEVICE_PRIVATE_DATA->image_buffer)
					DEVICE_PRIVATE_DATA->image_buffer = realloc(DEVICE_PRIVATE_DATA->image_buffer, file_stat.st_size);
				else
					DEVICE_PRIVATE_DATA->image_buffer = malloc(file_stat.st_size);
				int fd = open(file_name, O_RDONLY, 0);
				if (fd == -1) {
					AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				int result = indigo_read(fd, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.value = DEVICE_PRIVATE_DATA->image_buffer, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.size = file_stat.st_size);
				close(fd);
				if (result == -1) {
					AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
					AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				*AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.url = 0;
				strcpy(AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.format, DEVICE_PRIVATE_DATA->current_type);
				AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
				if (unlink(file_name) == -1) {
					AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		setup_download(device);
		return INDIGO_OK;
	} else 	if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILES
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property, false);
		if (AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value) {
			AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value = false;
			setup_download(device);
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
	} else 	if (indigo_property_match(AGENT_WHEEL_FILTER_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_WHEEL_FILTER
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
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_IMAGER_BATCH_PROPERTY);
	indigo_release_property(AGENT_IMAGER_FOCUS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SELECTION_PROPERTY);
	indigo_release_property(AGENT_IMAGER_STATS_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
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
			setup_download(FILTER_CLIENT_CONTEXT->device);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
			*CLIENT_PRIVATE_DATA->current_type = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_RAW_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".raw");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_FITS_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".fits");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_XISF_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".xisf");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_JPEG_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".jpeg");
					break;
				}
			}
			setup_download(FILTER_CLIENT_CONTEXT->device);
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
			abort_batch(device);
			if (!AGENT_START_PROCESS_PROPERTY->hidden) {
				indigo_delete_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				AGENT_START_PROCESS_PROPERTY->hidden = true;
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
			if (AGENT_ABORT_PROCESS_PROPERTY->hidden) {
				AGENT_ABORT_PROCESS_PROPERTY->hidden = false;
				indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX])) {
		if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "TBD: plate solve etc...");
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_FILE_PROPERTY_NAME)) {
			setup_download(FILTER_CLIENT_CONTEXT->device);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_LOCAL_MODE_PROPERTY_NAME)) {
			*CLIENT_PRIVATE_DATA->current_folder = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_LOCAL_MODE_DIR_ITEM_NAME) == 0) {
					strncpy(CLIENT_PRIVATE_DATA->current_folder, item->text.value, INDIGO_VALUE_SIZE);
					break;
				}
			}
			setup_download(FILTER_CLIENT_CONTEXT->device);
		} else if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
			*CLIENT_PRIVATE_DATA->current_type = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_RAW_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".raw");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_FITS_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".fits");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_XISF_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".xisf");
					break;
				} else if (item->sw.value && strcmp(item->name, CCD_IMAGE_FORMAT_JPEG_ITEM_NAME) == 0) {
					strcpy(CLIENT_PRIVATE_DATA->current_type, ".jpeg");
					break;
				}
			}
			setup_download(FILTER_CLIENT_CONTEXT->device);
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
