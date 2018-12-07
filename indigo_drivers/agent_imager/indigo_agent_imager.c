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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_imager"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_filter.h"
#include "indigo_agent_imager.h"

#define DEVICE_PRIVATE_DATA										((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_IMAGER_BATCH_PROPERTY						(DEVICE_PRIVATE_DATA->agent_ccd_batch_property)
#define AGENT_IMAGER_BATCH_COUNT_ITEM    			(AGENT_IMAGER_BATCH_PROPERTY->items+0)
#define AGENT_IMAGER_BATCH_EXPOSURE_ITEM  		(AGENT_IMAGER_BATCH_PROPERTY->items+1)
#define AGENT_IMAGER_BATCH_DELAY_ITEM     		(AGENT_IMAGER_BATCH_PROPERTY->items+2)

#define AGENT_IMAGER_PREVIEW_PROPERTY					(DEVICE_PRIVATE_DATA->agent_ccd_preview_property)
#define AGENT_IMAGER_PREVIEW_IMAGE_ITEM				(AGENT_IMAGER_PREVIEW_PROPERTY->items+0)
#define AGENT_IMAGER_PREVIEW_HISTO_ITEM				(AGENT_IMAGER_PREVIEW_PROPERTY->items+1)

#define AGENT_IMAGER_PREVIEW_SETUP_PROPERTY		(DEVICE_PRIVATE_DATA->agent_ccd_preview_setup_property)
#define AGENT_IMAGER_PREVIEW_BLACK_POINT_ITEM	(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY->items+0)
#define AGENT_IMAGER_PREVIEW_WHITE_POINT_ITEM	(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY->items+1)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_IMAGER_START_EXPOSURE_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_IMAGER_START_STREAMING_ITEM 		(AGENT_START_PROCESS_PROPERTY->items+1)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define FILTER_SLOT_COUNT											24

typedef struct {
	indigo_property *agent_ccd_batch_property;
	indigo_property *agent_ccd_preview_property;
	indigo_property *agent_ccd_preview_setup_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_abort_process_property;
	char filter_names[FILTER_SLOT_COUNT][INDIGO_NAME_SIZE];
	int filter_slot;
	int focuser_position;
	double site_lat, site_long;
	double mount_ra, mount_dec;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void set_headers(indigo_device *device) {
	char item_1[INDIGO_NAME_SIZE], item_2[INDIGO_NAME_SIZE], item_3[INDIGO_NAME_SIZE], item_4[INDIGO_NAME_SIZE], item_5[INDIGO_NAME_SIZE], item_6[INDIGO_NAME_SIZE];
	char *items[] = { item_1, item_2, item_3, item_4, item_5, item_6 };
	char value_1[INDIGO_NAME_SIZE], value_2[INDIGO_NAME_SIZE], value_3[INDIGO_NAME_SIZE], value_4[INDIGO_NAME_SIZE], value_5[INDIGO_NAME_SIZE], value_6[INDIGO_NAME_SIZE];
	char *values[] = { value_1, value_2, value_3, value_4, value_5, value_6 };

	
	for (int i = 0; i < 6; i++) {
		sprintf(items[i], CCD_FITS_HEADER_ITEM_NAME, i + 5);
		*values[i] = 0;
	}
	
	if (*FILTER_DEVICE_CONTEXT->related_mount_name) {
		sprintf(values[0], "SITELAT='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->site_lat), ((int)(fabs(DEVICE_PRIVATE_DATA->site_lat) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->site_lat) * 3600)) % 60);
		sprintf(values[1], "SITELONG='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->site_long), ((int)(fabs(DEVICE_PRIVATE_DATA->site_long) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->site_long) * 3600)) % 60);
		sprintf(values[2], "OBJCTRA='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_ra), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 3600)) % 60);
		sprintf(values[3], "OBJCTDEC='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_dec), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 3600)) % 60);
	}

	if (*FILTER_DEVICE_CONTEXT->related_wheel_name && DEVICE_PRIVATE_DATA->filter_slot > 0) {
		sprintf(values[4], "FILTER='%s'", DEVICE_PRIVATE_DATA->filter_names[DEVICE_PRIVATE_DATA->filter_slot - 1]);
	}

	if (*FILTER_DEVICE_CONTEXT->related_focuser_name) {
		sprintf(values[5], "FOCUSPOS=%d", DEVICE_PRIVATE_DATA->focuser_position);
	}
	
	indigo_change_text_property(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name, CCD_FITS_HEADERS_PROPERTY_NAME, 6, (const char **)items, (const char **)values);
}

static void exposure_batch(indigo_device *device) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	indigo_property *remote_exposure_property = NULL;
	set_headers(device);
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if (cache[j] && !strcmp(cache[j]->device, FILTER_DEVICE_CONTEXT->device_name) && !strcmp(cache[j]->name, CCD_EXPOSURE_PROPERTY_NAME)) {
			remote_exposure_property = cache[j];
			indigo_property *local_exposure_property = indigo_init_number_property(NULL, remote_exposure_property->device, remote_exposure_property->name, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, remote_exposure_property->count);
			if (local_exposure_property) {
				memcpy(local_exposure_property, remote_exposure_property, sizeof(indigo_property) + remote_exposure_property->count * sizeof(indigo_item));
				AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				int count = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
				for (int i = count; AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && i != 0; i--) {
					if (i < 0)
						i = -1;
					AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = i;
					double time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
					AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
					indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
					local_exposure_property->items[0].number.value = time;
					indigo_change_property(FILTER_DEVICE_CONTEXT->client, local_exposure_property);
					for (int i = 0; remote_exposure_property->state != INDIGO_BUSY_STATE && i < 1000; i++)
						usleep(1000);
					if (remote_exposure_property->state != INDIGO_BUSY_STATE) {
						indigo_send_message(device, "%s: CCD_EXPOSURE_PROPERTY didn't become busy in 1s", IMAGER_AGENT_NAME);
						break;
					}
					while (remote_exposure_property->state == INDIGO_BUSY_STATE && time > 0) {
						if (time > 1) {
							usleep(1000000);
							time -= 1;
							AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
							indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
						} else {
							usleep(10000);
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
								usleep(1000000);
								time -= 1;
								AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = time;
								indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
							} else {
								usleep(10000);
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
				indigo_release_property(local_exposure_property);
			}
			return;
		}
	}
	if (remote_exposure_property == NULL) {
		indigo_send_message(device, "%s: CCD_EXPOSURE_PROPERTY not found", IMAGER_AGENT_NAME);
	}
	AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
	AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
	AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
}

static void streaming_batch(indigo_device *device) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	indigo_property *remote_streaming_property = NULL;
	set_headers(device);
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if (cache[j] && !strcmp(cache[j]->device, FILTER_DEVICE_CONTEXT->device_name) && !strcmp(cache[j]->name, CCD_STREAMING_PROPERTY_NAME)) {
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
			indigo_property *local_streaming_property = indigo_init_number_property(NULL, remote_streaming_property->device, remote_streaming_property->name, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, remote_streaming_property->count);
			if (local_streaming_property) {
				memcpy(local_streaming_property, remote_streaming_property, sizeof(indigo_property) + remote_streaming_property->count * sizeof(indigo_item));
				AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				local_streaming_property->items[exposure_index].number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
				local_streaming_property->items[count_index].number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
				indigo_change_property(FILTER_DEVICE_CONTEXT->client, local_streaming_property);
				for (int i = 0; remote_streaming_property->state != INDIGO_BUSY_STATE && i < 1000; i++)
					usleep(1000);
				if (remote_streaming_property->state != INDIGO_BUSY_STATE) {
					indigo_send_message(device, "%s: CCD_STREAMING_PROPERTY didn't become busy in 1s", IMAGER_AGENT_NAME);
					break;
				}
				while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && remote_streaming_property->state == INDIGO_BUSY_STATE) {
					double time = remote_streaming_property->items[exposure_index].number.value;
					if (time > 1) {
						AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = time;
						AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = remote_streaming_property->items[count_index].number.value;
						indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
						usleep(1000000);
					} else {
						usleep(10000);
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
				indigo_release_property(local_streaming_property);
			}
			return;
		}
	}
	if (remote_streaming_property == NULL) {
		indigo_send_message(device, "%s: CCD_STREAMING_PROPERTY not found", IMAGER_AGENT_NAME);
	}
	AGENT_IMAGER_BATCH_COUNT_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
	AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
	AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		strcpy(FILTER_DEVICE_LIST_PROPERTY->name, FILTER_CCD_LIST_PROPERTY_NAME);
		strcpy(FILTER_DEVICE_LIST_PROPERTY->label, "Camera list");
		// -------------------------------------------------------------------------------- Batch properties
		AGENT_IMAGER_BATCH_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_BATCH_PROPERTY_NAME, "Batch", "Batch settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_IMAGER_BATCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_BATCH_COUNT_ITEM, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, "Frame count", -1, 999999, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_EXPOSURE_ITEM, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME, "Exposure time", 0, 3600, 0, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_DELAY_ITEM, AGENT_IMAGER_BATCH_DELAY_ITEM_NAME, "Delay after each exposure", 0, 3600, 0, 0);
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Batch", "Start batch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_START_EXPOSURE_ITEM, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME, "Start batch", false);
		indigo_init_switch_item(AGENT_IMAGER_START_STREAMING_ITEM, AGENT_IMAGER_START_STREAMING_ITEM_NAME, "Start streaming", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Batch", "Abort batch", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort batch", false);
		// -------------------------------------------------------------------------------- Preview properties
		AGENT_IMAGER_PREVIEW_SETUP_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_PREVIEW_SETUP_PROPERTY_NAME, "Preview", "Preview settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_IMAGER_BATCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_PREVIEW_BLACK_POINT_ITEM, AGENT_IMAGER_PREVIEW_BLACK_POINT_ITEM_NAME, "Black point", -1, 0xFFFF, 0, -1);
		indigo_init_number_item(AGENT_IMAGER_PREVIEW_WHITE_POINT_ITEM, AGENT_IMAGER_PREVIEW_WHITE_POINT_ITEM_NAME, "White point", -1, 0xFFFF, 0, -1);
		AGENT_IMAGER_PREVIEW_PROPERTY = indigo_init_blob_property(NULL, device->name, AGENT_IMAGER_PREVIEW_PROPERTY_NAME, "Preview", "Preview image data", INDIGO_OK_STATE, 2);
		if (AGENT_IMAGER_PREVIEW_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_blob_item(AGENT_IMAGER_PREVIEW_IMAGE_ITEM, AGENT_IMAGER_PREVIEW_IMAGE_ITEM_NAME, "Image preview");
		indigo_init_blob_item(AGENT_IMAGER_PREVIEW_HISTO_ITEM, AGENT_IMAGER_PREVIEW_HISTO_ITEM_NAME, "Image histogram");
		// -------------------------------------------------------------------------------- Related decvices properties
		FILTER_RELATED_WHEEL_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_FOCUSER_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_MOUNT_LIST_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		DEVICE_PRIVATE_DATA->filter_slot = -1;
		for (int i = 0; i < FILTER_SLOT_COUNT; i++)
			sprintf(DEVICE_PRIVATE_DATA->filter_names[i], "Filter #%d", i + 1);
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
	if (indigo_property_match(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_PREVIEW_SETUP_PROPERTY, NULL);
	if (indigo_property_match(AGENT_IMAGER_PREVIEW_PROPERTY, property))
		indigo_define_property(device, AGENT_IMAGER_PREVIEW_PROPERTY, NULL);
	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_IMAGER_BATCH_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_BATCH_PROPERTY, property, false);
		AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else 	if (indigo_property_match(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY, property, false);
		AGENT_IMAGER_PREVIEW_SETUP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_PREVIEW_SETUP_PROPERTY, NULL);
	} else 	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
		if (*FILTER_DEVICE_CONTEXT->device_name) {
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
				}
			}
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		} else {
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "%s: No CCD is selected", IMAGER_AGENT_NAME);
		}
	} else 	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
		if (*FILTER_DEVICE_CONTEXT->device_name) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_property *abort_property = indigo_init_switch_property(NULL, FILTER_DEVICE_CONTEXT->device_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
				if (abort_property) {
					indigo_init_switch_item(abort_property->items, CCD_ABORT_EXPOSURE_ITEM_NAME, "", true);
					indigo_change_property(FILTER_DEVICE_CONTEXT->client, abort_property);
					indigo_release_property(abort_property);
				}
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			}
			AGENT_ABORT_PROCESS_ITEM->sw.value = false;
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		} else {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, "%s: No CCD is selected", IMAGER_AGENT_NAME);
		}
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_IMAGER_BATCH_PROPERTY);
	indigo_release_property(AGENT_IMAGER_PREVIEW_SETUP_PROPERTY);
	indigo_release_property(AGENT_IMAGER_PREVIEW_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->related_wheel_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_wheel_name) && !strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++)
			strcpy(CLIENT_PRIVATE_DATA->filter_names[i], property->items[i].text.value);
	} else if (*FILTER_CLIENT_CONTEXT->related_wheel_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_wheel_name) && !strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->filter_slot = property->items[0].number.value;
	} else if (*FILTER_CLIENT_CONTEXT->related_focuser_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_focuser_name) && !strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->focuser_position = property->items[0].number.value;
	} else if (*FILTER_CLIENT_CONTEXT->related_mount_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_mount_name)) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME))
					CLIENT_PRIVATE_DATA->site_lat = item->number.value;
				else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME))
					CLIENT_PRIVATE_DATA->site_long = item->number.value;
			}
		} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_ra = item->number.value;
				else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_dec = item->number.value;
			}
		}
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name) && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
		if (property->state == INDIGO_OK_STATE) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "TBD: make histo & preview");
		} else {
			CLIENT_PRIVATE_DATA->agent_ccd_preview_property->state = property->state;
			indigo_update_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_ccd_preview_property, NULL);
		}
	} else if (*FILTER_CLIENT_CONTEXT->related_wheel_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_wheel_name) && !strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++)
			strcpy(CLIENT_PRIVATE_DATA->filter_names[i], property->items[i].text.value);
	} else if (*FILTER_CLIENT_CONTEXT->related_wheel_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_wheel_name) && !strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->filter_slot = property->items[0].number.value;
	} else if (*FILTER_CLIENT_CONTEXT->related_focuser_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_focuser_name) && !strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		CLIENT_PRIVATE_DATA->focuser_position = property->items[0].number.value;
	} else if (*FILTER_CLIENT_CONTEXT->related_mount_name && !strcmp(property->device, FILTER_CLIENT_CONTEXT->related_mount_name)) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME))
					CLIENT_PRIVATE_DATA->site_lat = item->number.value;
				else if (!strcmp(item->name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME))
					CLIENT_PRIVATE_DATA->site_long = item->number.value;
			}
		} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_ra = item->number.value;
				else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_dec = item->number.value;
			}
		}
	}
	return indigo_filter_update_property(client, device, property, message);
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
		indigo_filter_delete_property,
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
