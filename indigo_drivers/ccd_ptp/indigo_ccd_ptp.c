// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO PTP DSLR driver
 \file indigo_ccd_ptp.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <pthread.h>

#include <libusb-1.0/libusb.h>

#include <indigo/indigo_usb_utils.h>

#include "indigo_ptp.h"
#include "indigo_ptp_canon.h"
#include "indigo_ptp_nikon.h"
#include "indigo_ptp_sony.h"
#include "indigo_ptp_fuji.h"
#include "indigo_ccd_ptp.h"

#define MAX_DEVICES    	4

#define CANON_VID	0x04a9
#define NIKON_VID	0x04B0
#define SONY_VID	0x054c
#define FUJI_VID  0x04cb

static indigo_device *devices[MAX_DEVICES];

#include "ptp_camera_model.h"


static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		CCD_MODE_PROPERTY->hidden = true;
		CCD_STREAMING_PROPERTY->hidden = PRIVATE_DATA->liveview == NULL;
		CCD_FRAME_PROPERTY->hidden = true;
		CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->model.width;
		CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->model.height;
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value =  CCD_INFO_PIXEL_SIZE_ITEM->number.value = PRIVATE_DATA->model.pixel_size;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
		CCD_IMAGE_FORMAT_PROPERTY = indigo_resize_property(CCD_IMAGE_FORMAT_PROPERTY, 3);
		indigo_init_switch_item(CCD_IMAGE_FORMAT_NATIVE_ITEM, CCD_IMAGE_FORMAT_NATIVE_ITEM_NAME, "Native", true);
		indigo_init_switch_item(CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM, CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM_NAME, "Native + AVI", false);
		indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_ITEM, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, "RAW", false);
		// -------------------------------------------------------------------------------- DSLR_DELETE_IMAGE
		DSLR_DELETE_IMAGE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_DELETE_IMAGE_PROPERTY_NAME, "DSLR", "Delete downloaded image", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DSLR_DELETE_IMAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(DSLR_DELETE_IMAGE_ON_ITEM, DSLR_ZOOM_PREVIEW_ON_ITEM_NAME, "On", true);
		indigo_init_switch_item(DSLR_DELETE_IMAGE_OFF_ITEM, DSLR_ZOOM_PREVIEW_OFF_ITEM_NAME, "Off", false);
		// -------------------------------------------------------------------------------- DSLR_MIRROR_LOCKUP
		DSLR_MIRROR_LOCKUP_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_MIRROR_LOCKUP_PROPERTY_NAME, "DSLR", "Use mirror lockup", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DSLR_MIRROR_LOCKUP_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(DSLR_MIRROR_LOCKUP_LOCK_ITEM, DSLR_MIRROR_LOCKUP_LOCK_ITEM_NAME, "Lock", false);
		indigo_init_switch_item(DSLR_MIRROR_LOCKUP_UNLOCK_ITEM, DSLR_MIRROR_LOCKUP_UNLOCK_ITEM_NAME, "Unlock", true);
		// -------------------------------------------------------------------------------- DSLR_ZOOM_PREVIEW
		DSLR_ZOOM_PREVIEW_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_ZOOM_PREVIEW_PROPERTY_NAME, "DSLR", "Zoom preview", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DSLR_ZOOM_PREVIEW_PROPERTY == NULL)
			return INDIGO_FAILED;
		DSLR_ZOOM_PREVIEW_PROPERTY->hidden = PRIVATE_DATA->zoom == NULL;
		indigo_init_switch_item(DSLR_ZOOM_PREVIEW_ON_ITEM, DSLR_ZOOM_PREVIEW_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(DSLR_ZOOM_PREVIEW_OFF_ITEM, DSLR_ZOOM_PREVIEW_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------------- DSLR_LOCK
		DSLR_LOCK_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_LOCK_PROPERTY_NAME, "DSLR", "Lock camera GUI", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DSLR_LOCK_PROPERTY == NULL)
			return INDIGO_FAILED;
		DSLR_LOCK_PROPERTY->hidden = PRIVATE_DATA->lock == NULL;
		indigo_init_switch_item(DSLR_LOCK_ITEM, DSLR_LOCK_ITEM_NAME, "Lock", false);
		indigo_init_switch_item(DSLR_UNLOCK_ITEM, DSLR_UNLOCK_ITEM_NAME, "Unlock", true);
		// -------------------------------------------------------------------------------- DSLR_AF
		DSLR_AF_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_AF_PROPERTY_NAME, "DSLR", "Autofocus", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (DSLR_AF_PROPERTY == NULL)
			return INDIGO_FAILED;
		DSLR_AF_PROPERTY->hidden = PRIVATE_DATA->af == NULL;
		indigo_init_switch_item(DSLR_AF_ITEM, DSLR_AF_ITEM_NAME, "Start autofocus", false);
		// -------------------------------------------------------------------------------- DSLR_SET_HOST_TIME
		DSLR_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_SET_HOST_TIME_PROPERTY_NAME, "DSLR", "Set host time", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (DSLR_SET_HOST_TIME_PROPERTY == NULL)
			return INDIGO_FAILED;
		DSLR_SET_HOST_TIME_PROPERTY->hidden = PRIVATE_DATA->set_host_time == NULL;
		indigo_init_switch_item(DSLR_SET_HOST_TIME_ITEM, DSLR_SET_HOST_TIME_ITEM_NAME, "Set host time", false);
		// --------------------------------------------------------------------------------
		PRIVATE_DATA->transaction_id = 0;
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->message_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(DSLR_DELETE_IMAGE_PROPERTY, property))
			indigo_define_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
		if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property))
			indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
		if (indigo_property_match(DSLR_ZOOM_PREVIEW_PROPERTY, property))
			indigo_define_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
		if (indigo_property_match(DSLR_LOCK_PROPERTY, property))
			indigo_define_property(device, DSLR_LOCK_PROPERTY, NULL);
		if (indigo_property_match(DSLR_AF_PROPERTY, property))
			indigo_define_property(device, DSLR_AF_PROPERTY, NULL);
		if (indigo_property_match(DSLR_SET_HOST_TIME_PROPERTY, property))
			indigo_define_property(device, DSLR_SET_HOST_TIME_PROPERTY, NULL);
		for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++)
			if (indigo_property_match(PRIVATE_DATA->properties[i].property, property))
				indigo_define_property(device, PRIVATE_DATA->properties[i].property, NULL);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static void handle_connection(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
		if (PRIVATE_DATA->handle == NULL) {
			bool result = true;
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				result = false;
			} else {
				result = ptp_open(device);
			}
			if (result) {
				PRIVATE_DATA->transaction_id = 0;
				PRIVATE_DATA->session_id = 0;
				result = ptp_transaction_1_1(device, ptp_operation_OpenSession, 1, &PRIVATE_DATA->session_id);
				if (!result && PRIVATE_DATA->last_error == ptp_response_SessionAlreadyOpen) {
					ptp_transaction_0_0(device, ptp_operation_CloseSession);
					result = ptp_transaction_1_1(device, ptp_operation_OpenSession, 1, &PRIVATE_DATA->session_id);
				}
				if (result) {
					if (PRIVATE_DATA->initialise(device)) {
						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					} else {
						ptp_close(device);
						indigo_global_unlock(device);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					}
				} else {
					ptp_close(device);
					indigo_global_unlock(device);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			} else {
				indigo_global_unlock(device);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (CONNECTION_PROPERTY->state == INDIGO_OK_STATE) {
				indigo_define_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
				indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
				indigo_define_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
				indigo_define_property(device, DSLR_LOCK_PROPERTY, NULL);
				indigo_define_property(device, DSLR_AF_PROPERTY, NULL);
				indigo_define_property(device, DSLR_SET_HOST_TIME_PROPERTY, NULL);
				for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++)
					indigo_define_property(device, PRIVATE_DATA->properties[i].property, NULL);
				if (PRIVATE_DATA->focuser)
					indigo_attach_device(PRIVATE_DATA->focuser);
			} else {
				for (int i = 0; PRIVATE_DATA->properties[i].property; i++)
					indigo_release_property(PRIVATE_DATA->properties[i].property);
				memset(PRIVATE_DATA->properties, 0, sizeof(PRIVATE_DATA->properties));
			}
		}
		pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
	} else {
		indigo_detach_device(PRIVATE_DATA->focuser);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->event_checker);
		ptp_transaction_0_0(device, ptp_operation_CloseSession);
		ptp_close(device);
		indigo_delete_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_LOCK_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_AF_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_SET_HOST_TIME_PROPERTY, NULL);
		for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
			indigo_delete_property(device, PRIVATE_DATA->properties[i].property, NULL);
			indigo_release_property(PRIVATE_DATA->properties[i].property);
		}
		memset(PRIVATE_DATA->properties, 0, sizeof(PRIVATE_DATA->properties));
		if (PRIVATE_DATA->image_buffer) {
			free(PRIVATE_DATA->image_buffer);
			PRIVATE_DATA->image_buffer = NULL;
		}
		indigo_global_unlock(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void handle_lock(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	DSLR_LOCK_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DSLR_LOCK_PROPERTY, NULL);
	if (PRIVATE_DATA->lock(device))
		DSLR_LOCK_PROPERTY->state = INDIGO_OK_STATE;
	else
		DSLR_LOCK_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DSLR_LOCK_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static void handle_af(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	DSLR_AF_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DSLR_AF_PROPERTY, NULL);
	if (PRIVATE_DATA->af(device))
		DSLR_AF_PROPERTY->state = INDIGO_OK_STATE;
	else
		DSLR_AF_PROPERTY->state = INDIGO_ALERT_STATE;
	DSLR_AF_ITEM->sw.value = false;
	indigo_update_property(device, DSLR_AF_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static void handle_set_host_time(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	DSLR_SET_HOST_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DSLR_SET_HOST_TIME_PROPERTY, NULL);
	if (PRIVATE_DATA->set_host_time(device))
		DSLR_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	else
		DSLR_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	DSLR_SET_HOST_TIME_ITEM->sw.value = false;
	indigo_update_property(device, DSLR_SET_HOST_TIME_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static void handle_zoom(indigo_device *device) {
	DSLR_ZOOM_PREVIEW_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
	if (PRIVATE_DATA->zoom(device))
		DSLR_ZOOM_PREVIEW_PROPERTY->state = INDIGO_OK_STATE;
	else
		DSLR_ZOOM_PREVIEW_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
}

static void handle_set_property(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	indigo_property *property = PRIVATE_DATA->properties[PRIVATE_DATA->message_property_index].property;
	if (property) {
		property->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, property, NULL);
		if (PRIVATE_DATA->set_property(device, PRIVATE_DATA->properties + PRIVATE_DATA->message_property_index))
			property->state = INDIGO_OK_STATE;
		else
			property->state = INDIGO_ALERT_STATE;
	} else {
		property->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, property, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static void handle_streaming(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	PRIVATE_DATA->abort_capture = false;
	CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	if (PRIVATE_DATA->liveview(device))
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	else {
		CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static void handle_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->message_mutex);
	PRIVATE_DATA->abort_capture = false;
	CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
	CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value)
		CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value)
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
	indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	if (PRIVATE_DATA->exposure(device))
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	else
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->message_mutex);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_connection, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_DELETE_IMAGE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_DELETE_IMAGE_PROPERTY
		indigo_property_copy_values(DSLR_DELETE_IMAGE_PROPERTY, property, false);
		DSLR_DELETE_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_MIRROR_LOCKUP
		indigo_property_copy_values(DSLR_MIRROR_LOCKUP_PROPERTY, property, false);
		DSLR_MIRROR_LOCKUP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_ZOOM_PREVIEW_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_ZOOM_PREVIEW
		indigo_property_copy_values(DSLR_ZOOM_PREVIEW_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_zoom, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_LOCK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_LOCK
		indigo_property_copy_values(DSLR_LOCK_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_lock, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_AF_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_AF
		indigo_property_copy_values(DSLR_AF_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_af, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(DSLR_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_AF
		indigo_property_copy_values(DSLR_SET_HOST_TIME_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_set_host_time, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_exposure, NULL);
		// -------------------------------------------------------------------------------- CCD_STREAMING
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		PRIVATE_DATA->abort_capture = false;
		indigo_set_timer(device, 0, handle_streaming, NULL);
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_ABORT_EXPOSURE_ITEM->sw.value) {
			CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
			PRIVATE_DATA->abort_capture = true;
		}
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_PREVIEW_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_PREVIEW
		indigo_property_copy_values(CCD_PREVIEW_PROPERTY, property, false);
		if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
			if (CCD_PREVIEW_IMAGE_PROPERTY->hidden) {
				CCD_PREVIEW_IMAGE_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			}
		} else {
			if (!CCD_PREVIEW_IMAGE_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
				CCD_PREVIEW_IMAGE_PROPERTY->hidden = true;
			}
		}
		CCD_PREVIEW_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_PREVIEW_PROPERTY, NULL);
		return INDIGO_OK;
	} else {
		for (int i = 0; i < PRIVATE_DATA->info_properties_supported[i]; i++) {
			if (indigo_property_match(PRIVATE_DATA->properties[i].property, property)) {
				indigo_property *definition = PRIVATE_DATA->properties[i].property;
				indigo_property_copy_values(definition, property, false);
				PRIVATE_DATA->message_property_index = i;
				indigo_set_timer(device, 0, handle_set_property, NULL);
				break;
			}
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_connection(device);
	}
	indigo_release_property(DSLR_DELETE_IMAGE_PROPERTY);
	indigo_release_property(DSLR_MIRROR_LOCKUP_PROPERTY);
	indigo_release_property(DSLR_ZOOM_PREVIEW_PROPERTY);
	indigo_release_property(DSLR_LOCK_PROPERTY);
	indigo_release_property(DSLR_AF_PROPERTY);
	indigo_release_property(DSLR_SET_HOST_TIME_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void handle_focus(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	if (PRIVATE_DATA->focus(device->master_device, (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1) * FOCUSER_STEPS_ITEM->number.value))
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	else
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			PRIVATE_DATA->focus(device->master_device, 0);
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_focus, NULL);
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);
	struct libusb_device_descriptor descriptor;
	pthread_mutex_lock(&device_mutex);
	int rc = libusb_get_device_descriptor(dev, &descriptor);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "searching for %04x:%04x", descriptor.idVendor, descriptor.idProduct);
	for (int i = 0; CAMERA[i].vendor; i++) {
		if (CAMERA[i].vendor == descriptor.idVendor && (CAMERA[i].product == descriptor.idProduct || CAMERA[i].product == 0xFFFF)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "found %s", CAMERA[i].name);
			ptp_private_data *private_data = indigo_safe_malloc(sizeof(ptp_private_data));
			private_data->dev = dev;
			private_data->model = CAMERA[i];
			if (descriptor.idVendor == CANON_VID) {
				private_data->operation_code_label = ptp_operation_canon_code_label;
				private_data->response_code_label = ptp_response_canon_code_label;
				private_data->event_code_label = ptp_event_canon_code_label;
				private_data->property_code_name = ptp_property_canon_code_name;
				private_data->property_code_label = ptp_property_canon_code_label;
				private_data->property_value_code_label = ptp_property_canon_value_code_label;
				private_data->initialise = ptp_canon_initialise;
				private_data->handle_event = NULL;
				private_data->fix_property = NULL;
				private_data->set_property = ptp_canon_set_property;
				private_data->exposure = ptp_canon_exposure;
				private_data->liveview = (CAMERA[i].flags && ptp_flag_lv) ? ptp_canon_liveview : NULL;
				private_data->lock = ptp_canon_lock;
				private_data->af = ptp_canon_af;
				private_data->zoom = (CAMERA[i].flags && ptp_flag_lv) ? ptp_canon_zoom : NULL;
				private_data->focus = (CAMERA[i].flags && ptp_flag_lv) ? ptp_canon_focus : NULL;
				private_data->set_host_time = ptp_canon_set_host_time;
				private_data->check_dual_compression = ptp_canon_check_dual_compression;
			} else if (descriptor.idVendor == NIKON_VID) {
				private_data->operation_code_label = ptp_operation_nikon_code_label;
				private_data->response_code_label = ptp_response_nikon_code_label;
				private_data->event_code_label = ptp_event_nikon_code_label;
				private_data->property_code_name = ptp_property_nikon_code_name;
				private_data->property_code_label = ptp_property_nikon_code_label;
				private_data->property_value_code_label = ptp_property_nikon_value_code_label;
				private_data->initialise = ptp_nikon_initialise;
				private_data->handle_event = ptp_nikon_handle_event;
				private_data->fix_property = ptp_nikon_fix_property;
				private_data->set_property = ptp_nikon_set_property;
				private_data->exposure = ptp_nikon_exposure;
				private_data->liveview = (CAMERA[i].flags && ptp_flag_lv) ? ptp_nikon_liveview : NULL;
				private_data->lock = ptp_nikon_lock;
				private_data->af = NULL;
				private_data->zoom = (CAMERA[i].flags && ptp_flag_lv) ? ptp_nikon_zoom : NULL;
				private_data->focus = (CAMERA[i].flags && ptp_flag_lv) ? ptp_nikon_focus : NULL;
				private_data->set_host_time = ptp_set_host_time;
				private_data->check_dual_compression = ptp_nikon_check_dual_compression;
			} else if (descriptor.idVendor == SONY_VID) {
				private_data->operation_code_label = ptp_operation_sony_code_label;
				private_data->response_code_label = ptp_response_code_label;
				private_data->event_code_label = ptp_event_sony_code_label;
				private_data->property_code_name = ptp_property_sony_code_name;
				private_data->property_code_label = ptp_property_sony_code_label;
				private_data->property_value_code_label = ptp_property_sony_value_code_label;
				private_data->initialise = ptp_sony_initialise;
				private_data->handle_event = ptp_sony_handle_event;
				private_data->fix_property = NULL;
				private_data->set_property = ptp_sony_set_property;
				private_data->exposure = ptp_sony_exposure;
				private_data->liveview = (CAMERA[i].flags && ptp_flag_lv) ? ptp_sony_liveview : NULL;
				private_data->lock = NULL;
				private_data->af = ptp_sony_af;
				private_data->zoom = NULL;
				private_data->focus = NULL;
				private_data->set_host_time = NULL;
				private_data->check_dual_compression = ptp_sony_check_dual_compression;
			} else if (descriptor.idVendor == FUJI_VID) {
				private_data->operation_code_label = ptp_operation_fuji_code_label;
				private_data->response_code_label = ptp_response_code_label;
				private_data->event_code_label = ptp_event_fuji_code_label;
				private_data->property_code_name = ptp_property_fuji_code_name;
				private_data->property_code_label = ptp_property_fuji_code_label;
				private_data->property_value_code_label = ptp_property_fuji_value_code_label;
				private_data->initialise = ptp_fuji_initialise;
				private_data->handle_event = NULL;
				private_data->fix_property = ptp_fuji_fix_property;
				private_data->set_property = ptp_fuji_set_property;
				private_data->exposure = ptp_fuji_exposure;
				private_data->liveview = (CAMERA[i].flags && ptp_flag_lv) ? ptp_fuji_liveview : NULL;
				private_data->lock = NULL;
				private_data->af = NULL;
				private_data->zoom = NULL;
				private_data->focus = NULL;
				private_data->set_host_time = ptp_set_host_time;
				private_data->check_dual_compression = ptp_fuji_check_dual_compression;
			} else {
				private_data->operation_code_label = ptp_operation_code_label;
				private_data->response_code_label = ptp_response_code_label;
				private_data->event_code_label = ptp_event_code_label;
				private_data->property_code_name = ptp_property_code_name;
				private_data->property_code_label = ptp_property_code_label;
				private_data->property_value_code_label = ptp_property_value_code_label;
				private_data->initialise = ptp_initialise;
				private_data->handle_event = ptp_handle_event;
				private_data->fix_property = NULL;
				private_data->set_property = ptp_set_property;
				private_data->exposure = ptp_exposure;
				private_data->liveview = NULL;
				private_data->lock = NULL;
				private_data->af = NULL;
				private_data->zoom = NULL;
				private_data->focus = NULL;
				private_data->set_host_time = ptp_set_host_time;
				private_data->check_dual_compression = NULL;
			}
			libusb_ref_device(dev);
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			device->master_device = device;
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(device->name, INDIGO_NAME_SIZE, "%s #%s", CAMERA[i].name, usb_path);
			device->private_data = private_data;
			if (private_data->focus) {
				indigo_device *focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
				focuser->master_device = device;
				snprintf(focuser->name, INDIGO_NAME_SIZE, "%s (focuser) #%s", CAMERA[i].name, usb_path);
				focuser->private_data = private_data;
				private_data->focuser = focuser;
			}
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
}


static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	ptp_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				private_data = PRIVATE_DATA;
				if (private_data->focuser) {
					indigo_detach_device(private_data->focuser);
					free(private_data->focuser);
					private_data->focuser = NULL;
				}
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		if (private_data->vendor_private_data)
			free(private_data->vendor_private_data);
		free(private_data);
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_ASYNC(process_plug_event, dev);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			process_unplug_event(dev);
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_ptp(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "PTP-over-USB Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
				}
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
