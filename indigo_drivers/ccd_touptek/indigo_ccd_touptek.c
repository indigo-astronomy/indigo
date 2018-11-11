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

/** INDIGO ToupTek CCD driver
 \file indigo_ccd_touptek.c
 */

#define DRIVER_VERSION 0x0008
#define DRIVER_NAME "indigo_ccd_touptek"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <toupcam.h>

#include "indigo_usb_utils.h"
#include "indigo_driver_xml.h"

#include "indigo_ccd_touptek.h"

#define PRIVATE_DATA        ((touptek_private_data *)device->private_data)

#define X_CCD_ADVANCED_PROPERTY						(PRIVATE_DATA->advanced_property)
#define X_CCD_CONTRAST_ITEM								(X_CCD_ADVANCED_PROPERTY->items + 0)
#define X_CCD_HUE_ITEM										(X_CCD_ADVANCED_PROPERTY->items + 1)
#define X_CCD_SATURATION_ITEM							(X_CCD_ADVANCED_PROPERTY->items + 2)
#define X_CCD_BRIGHTNESS_ITEM							(X_CCD_ADVANCED_PROPERTY->items + 3)
#define X_CCD_GAMMA_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 4)
#define X_CCD_SPEED_ITEM									(X_CCD_ADVANCED_PROPERTY->items + 5)
#define X_CCD_FAN_SPEED_ITEM							(X_CCD_ADVANCED_PROPERTY->items + 6)

typedef struct {
	ToupcamInstV2 cam;
	HToupCam handle;
	bool present;
	indigo_device *camera;
	indigo_device *guider;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer;
	unsigned char *buffer;
	int bits;
	pthread_mutex_t mutex;
	indigo_property *advanced_property;
} touptek_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void pull_callback(unsigned event, void* callbackCtx) {
	ToupcamFrameInfoV2 frameInfo;
	HRESULT result;
	indigo_device *device = (indigo_device *)callbackCtx;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pull_callback #%d", event);
	switch (event) {
		case TOUPCAM_EVENT_IMAGE: {
			pthread_mutex_lock(&PRIVATE_DATA->mutex);
			result = Toupcam_PullImageV2(PRIVATE_DATA->handle, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, PRIVATE_DATA->bits, &frameInfo);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_PullImageV2(%d, ->[%d x %d, %x, %d]) -> %08x", PRIVATE_DATA->bits, frameInfo.width, frameInfo.height, frameInfo.flag, frameInfo.seq, result);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			indigo_process_image(device, PRIVATE_DATA->buffer, frameInfo.width, frameInfo.height, PRIVATE_DATA->bits, PRIVATE_DATA->bits != 24, NULL);
			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
				CCD_EXPOSURE_ITEM->number.value = 0;
				CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
				if (--CCD_STREAMING_COUNT_ITEM->number.value == 0)
					CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			}
			break;
		}
		case TOUPCAM_EVENT_TIMEOUT:
		case TOUPCAM_EVENT_DISCONNECTED:
		case TOUPCAM_EVENT_ERROR: {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			break;
		}
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;
	short temperature;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	HRESULT result = Toupcam_get_Temperature(PRIVATE_DATA->handle, &temperature);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (result >= 0) {
		CCD_TEMPERATURE_ITEM->number.value = temperature / 10.0;
		if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM && fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 1.0) {
			if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Toupcam_get_Temperature() -> %08x", result);
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}

static void setup_exposure(indigo_device *device) {
	HRESULT result;
	PRIVATE_DATA->bits = 0;
	unsigned resolutionIndex = 0, currentResolutionIndex = 0;
	result = Toupcam_get_eSize(PRIVATE_DATA->handle, &currentResolutionIndex);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_eSize(->%d) -> %08x", currentResolutionIndex, result);
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		indigo_item *item = CCD_MODE_PROPERTY->items + i;
		if (item->sw.value) {
			if (strncmp(item->name, "RAW8", 4) == 0) {
				resolutionIndex = atoi(item->name + 5);
				if (resolutionIndex != currentResolutionIndex) {
					result = Toupcam_Stop(PRIVATE_DATA->handle);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Stop() -> %08x", result);
					result = Toupcam_put_eSize(PRIVATE_DATA->handle, resolutionIndex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_eSize(%d) -> %08x", resolutionIndex, result);
					result = Toupcam_StartPullModeWithCallback(PRIVATE_DATA->handle, pull_callback, device);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_StartPullModeWithCallback() -> %08x", result);
				}
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_RAW, 1);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_RAW, 1) -> %08x", result);
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_BITDEPTH, 0);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_BITDEPTH, 0) -> %08x", result);
				PRIVATE_DATA->bits = 8;
			} else if (strncmp(item->name, "RAW16", 4) == 0) {
				resolutionIndex = atoi(item->name + 6);
				if (resolutionIndex != currentResolutionIndex) {
					result = Toupcam_Stop(PRIVATE_DATA->handle);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Stop() -> %08x", result);
					result = Toupcam_put_eSize(PRIVATE_DATA->handle, resolutionIndex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_eSize(%d) -> %08x", resolutionIndex, result);
					result = Toupcam_StartPullModeWithCallback(PRIVATE_DATA->handle, pull_callback, device);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_StartPullModeWithCallback() -> %08x", result);
				}
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_RAW, 1);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_RAW, 1) -> %08x", result);
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_BITDEPTH, 1);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_BITDEPTH, 1) -> %08x", result);
				PRIVATE_DATA->bits = 16;
			} else if (strncmp(item->name, "RGB", 3) == 0) {
				resolutionIndex = atoi(item->name + 4);
				if (resolutionIndex != currentResolutionIndex) {
					result = Toupcam_Stop(PRIVATE_DATA->handle);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Stop() -> %08x", result);
					result = Toupcam_put_eSize(PRIVATE_DATA->handle, resolutionIndex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_eSize(%d) -> %08x", resolutionIndex, result);
					result = Toupcam_StartPullModeWithCallback(PRIVATE_DATA->handle, pull_callback, device);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_StartPullModeWithCallback() -> %08x", result);
				}
				PRIVATE_DATA->bits = 24;
			}
		}
	}
	if (PRIVATE_DATA->cam.model->flag & TOUPCAM_FLAG_ROI_HARDWARE) {
		unsigned left = 2 * ((unsigned)CCD_FRAME_LEFT_ITEM->number.value / (unsigned)CCD_BIN_HORIZONTAL_ITEM->number.value / 2);
		unsigned top = 2 * ((unsigned)CCD_FRAME_TOP_ITEM->number.value / (unsigned)CCD_BIN_VERTICAL_ITEM->number.value / 2);
		unsigned width = 2 * ((unsigned)CCD_FRAME_WIDTH_ITEM->number.value / (unsigned)CCD_BIN_HORIZONTAL_ITEM->number.value / 2);
		if (width < 16)
			width = 16;
		unsigned height = 2 * ((unsigned)CCD_FRAME_HEIGHT_ITEM->number.value / (unsigned)CCD_BIN_VERTICAL_ITEM->number.value / 2);
		if (height < 16)
			height = 16;
		result = Toupcam_put_Roi(PRIVATE_DATA->handle, left, top, width, height);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Roi(%d, %d, %d, %d) -> %08x", left, top, width, height, result);
	}
	result = Toupcam_put_AutoExpoEnable(PRIVATE_DATA->handle, false);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_AutoExpoEnable(false) -> %08x", result);
	result = Toupcam_Flush(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Flush() -> %08x", result);
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		unsigned long long flags = PRIVATE_DATA->cam.model->flag;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "flags = %0LX", flags);
		char name[128], label[128];
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->cam.model->xpixsz;
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->cam.model->ypixsz;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = (CCD_INFO_PIXEL_WIDTH_ITEM->number.value + CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) / 2.0;
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 0;
		CCD_INFO_WIDTH_ITEM->number.value = 0;
		CCD_INFO_HEIGHT_ITEM->number.value = 0;
		for (int i = 0; i < PRIVATE_DATA->cam.model->preview; i++) {
			int frame_width = PRIVATE_DATA->cam.model->res[i].width;
			int frame_height = PRIVATE_DATA->cam.model->res[i].height;
			if (frame_width > CCD_INFO_WIDTH_ITEM->number.value)
				CCD_INFO_WIDTH_ITEM->number.value = frame_width;
			if (frame_height > CCD_INFO_HEIGHT_ITEM->number.value)
				CCD_INFO_HEIGHT_ITEM->number.value = frame_height;
			if (flags & TOUPCAM_FLAG_RAW8) {
				snprintf(name, sizeof(name), "RAW8_%d", i);
				snprintf(label, sizeof(label), "RAW %d x %dx8", frame_width, frame_height);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
				CCD_MODE_PROPERTY->count++;
			}
			if (flags & TOUPCAM_FLAG_RAW16 || flags & TOUPCAM_FLAG_RAW14 || flags & TOUPCAM_FLAG_RAW12 || flags & TOUPCAM_FLAG_RAW10) {
				snprintf(name, sizeof(name), "RAW16_%d", i);
				snprintf(label, sizeof(label), "RAW %d x %dx16", frame_width, frame_height);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
				CCD_MODE_PROPERTY->count++;
			}
			if ((flags & TOUPCAM_FLAG_MONO) == 0) {
				snprintf(name, sizeof(name), "RGB_%d", i);
				snprintf(label, sizeof(label), "RGB %d x %d", frame_width, frame_height);
				indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
				CCD_MODE_PROPERTY->count++;
			}
		}
		CCD_MODE_ITEM->sw.value = true;
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 0;
		if (flags & TOUPCAM_FLAG_RAW8) {
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
		}
		if (flags & TOUPCAM_FLAG_RAW16 || flags & TOUPCAM_FLAG_RAW14 || flags & TOUPCAM_FLAG_RAW12 || flags & TOUPCAM_FLAG_RAW10) {
			if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min == 0)
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 16;
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
		}
		if ((flags & TOUPCAM_FLAG_ROI_HARDWARE) == 0) {
			CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		}
		if (PRIVATE_DATA->cam.model->flag & TOUPCAM_FLAG_GETTEMPERATURE) {
			CCD_TEMPERATURE_PROPERTY->hidden = false;
			if (PRIVATE_DATA->cam.model->flag & TOUPCAM_FLAG_TEC_ONOFF) {
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_COOLER_PROPERTY->hidden = false;
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
			} else {
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			}
		}
		CCD_BIN_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_STREAMING_PROPERTY->hidden = false;
		X_CCD_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, "X_CCD_ADVANCED_PROPERTY", CCD_MAIN_GROUP, "Advanced Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, flags & TOUPCAM_FLAG_FAN ? 7 : 6);
		if (X_CCD_ADVANCED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_CCD_CONTRAST_ITEM, "CONTRAST", "Contrast", TOUPCAM_CONTRAST_MIN, TOUPCAM_CONTRAST_MAX, 1, TOUPCAM_CONTRAST_DEF);
		indigo_init_number_item(X_CCD_HUE_ITEM, "HUE", "Hue", TOUPCAM_HUE_MIN, TOUPCAM_HUE_MAX, 1, TOUPCAM_HUE_DEF);
		indigo_init_number_item(X_CCD_SATURATION_ITEM, "SATURATION", "Saturation", TOUPCAM_SATURATION_MIN, TOUPCAM_SATURATION_MAX, 1, TOUPCAM_SATURATION_DEF);
		indigo_init_number_item(X_CCD_BRIGHTNESS_ITEM, "BRIGHTNESS", "Brightness", TOUPCAM_BRIGHTNESS_MIN, TOUPCAM_BRIGHTNESS_MAX, 1, TOUPCAM_BRIGHTNESS_DEF);
		indigo_init_number_item(X_CCD_GAMMA_ITEM, "GAMMA", "Gamma", TOUPCAM_GAMMA_MIN, TOUPCAM_GAMMA_MAX, 1, TOUPCAM_GAMMA_DEF);
		indigo_init_number_item(X_CCD_SPEED_ITEM, "SPEED", "Speed (framerate)", 0, 0, 1, 0);
		if (flags & TOUPCAM_FLAG_FAN)
			indigo_init_number_item(X_CCD_FAN_SPEED_ITEM, "FAN_SPEED", "Fan speed", 0, 0, 1, 0);
		
		PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(3 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_CCD_ADVANCED_PROPERTY, property))
			indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	HRESULT result;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (PRIVATE_DATA->handle == NULL) {
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				} else {
					PRIVATE_DATA->handle = Toupcam_Open(PRIVATE_DATA->cam.id);
				}
			}
			device->gp_bits = 1;
			if (PRIVATE_DATA->handle) {
				if (PRIVATE_DATA->cam.model->flag & TOUPCAM_FLAG_GETTEMPERATURE) {
					if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM) {
						int value;
						result = Toupcam_get_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_TEC, &value);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_Option(TOUPCAM_OPTION_TEC, ->%d) -> %08x", value, result);
						indigo_set_switch(CCD_COOLER_PROPERTY, value ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
						result = Toupcam_get_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_TECTARGET, &value);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_Option(TOUPCAM_OPTION_TECTARGET, ->%d) -> %08x", value, result);
						CCD_TEMPERATURE_ITEM->number.target = value / 10.0;
					}
					PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 5.0, ccd_temperature_callback);
				} else {
					PRIVATE_DATA->temperature_timer = NULL;
				}
				int rawMode;
				int bitDepth;
				unsigned resolutionIndex;
				char name[16];
				if (PRIVATE_DATA->cam.model->flag & TOUPCAM_FLAG_MONO) {
					rawMode = 1;
				} else {
					result = Toupcam_get_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_RAW, &rawMode);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_Option(TOUPCAM_OPTION_RAW, ->%d) -> %08x", rawMode, result);
				}
				if (rawMode) {
					result = Toupcam_get_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_BITDEPTH, &bitDepth);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_Option(TOUPCAM_OPTION_BITDEPTH, ->%d) -> %08x", bitDepth, result);
					result = Toupcam_get_eSize(PRIVATE_DATA->handle, &resolutionIndex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_eSize(->%d) -> %08x", resolutionIndex, result);
					sprintf(name, "RAW%d_%d", bitDepth ? 16 : 8, resolutionIndex);
				} else {
					result = Toupcam_get_eSize(PRIVATE_DATA->handle, &resolutionIndex);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_eSize(->%d) -> %08x", resolutionIndex, result);
					sprintf(name, "RGB_%d", resolutionIndex);
				}
				for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
					if (strcmp(name, CCD_MODE_PROPERTY->items[i].name) == 0) {
						indigo_set_switch(CCD_MODE_PROPERTY, CCD_MODE_PROPERTY->items + i, true);
					}
				}
				CCD_BIN_HORIZONTAL_ITEM->number.value = (int)(CCD_INFO_WIDTH_ITEM->number.value / PRIVATE_DATA->cam.model->res[resolutionIndex].width);
				CCD_BIN_VERTICAL_ITEM->number.value = (int)(CCD_INFO_HEIGHT_ITEM->number.value / PRIVATE_DATA->cam.model->res[resolutionIndex].height);
				uint32_t min, max, current;
				Toupcam_get_ExpTimeRange(PRIVATE_DATA->handle, &min, &max, &current);
				CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = min / 1000000.0;
				CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = max / 1000000.0;
				result = Toupcam_get_Speed(PRIVATE_DATA->handle, (unsigned short *)&current);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_get_Speed(->%d) -> %08x", current, result);
				X_CCD_SPEED_ITEM->number.max = Toupcam_get_MaxSpeed(PRIVATE_DATA->handle);
				if (X_CCD_ADVANCED_PROPERTY->count > 6)
					X_CCD_FAN_SPEED_ITEM->number.max = Toupcam_get_FanMaxSpeed(PRIVATE_DATA->handle);
				indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_TRIGGER, 1);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_TRIGGER) -> %08x", result);
				result = Toupcam_StartPullModeWithCallback(PRIVATE_DATA->handle, pull_callback, device);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_StartPullModeWithCallback() -> %08x", result);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				device->gp_bits = 0;
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			if (PRIVATE_DATA->buffer != NULL) {
				free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
			}
			if (PRIVATE_DATA->guider && PRIVATE_DATA->guider->gp_bits == 0) {
				if (((touptek_private_data *)PRIVATE_DATA->guider->private_data)->handle == NULL) {
					pthread_mutex_lock(&PRIVATE_DATA->mutex);
					Toupcam_Close(PRIVATE_DATA->handle);
					pthread_mutex_unlock(&PRIVATE_DATA->mutex);
				}
				PRIVATE_DATA->handle = NULL;
				indigo_global_unlock(device);
			}
			device->gp_bits = 0;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_MODE
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				char *underscore = strchr(item->name, '_');
				unsigned resolutionIndex = atoi(underscore + 1);
				CCD_BIN_HORIZONTAL_ITEM->number.value = (int)(CCD_INFO_WIDTH_ITEM->number.value / PRIVATE_DATA->cam.model->res[resolutionIndex].width);
				CCD_BIN_VERTICAL_ITEM->number.value = (int)(CCD_INFO_HEIGHT_ITEM->number.value / PRIVATE_DATA->cam.model->res[resolutionIndex].height);
				break;
			}
		}
		if (IS_CONNECTED) {
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		setup_exposure(device);
		result = Toupcam_put_ExpoTime(PRIVATE_DATA->handle, (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_ExpoTime(%u) -> %08x", (unsigned)(CCD_EXPOSURE_ITEM->number.target * 1000000), result);
		result = Toupcam_Trigger(PRIVATE_DATA->handle, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Trigger(1) -> %08x", result);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		setup_exposure(device);
		result = Toupcam_put_ExpoTime(PRIVATE_DATA->handle, (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_ExpoTime(%u) -> %08x", (unsigned)(CCD_STREAMING_EXPOSURE_ITEM->number.target * 1000000), result);
		result = Toupcam_Trigger(PRIVATE_DATA->handle, (int)CCD_STREAMING_COUNT_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Trigger(%d) -> %08x", (int)CCD_STREAMING_COUNT_ITEM->number.value);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_ABORT_EXPOSURE_ITEM->sw.value) {
			result = Toupcam_Trigger(PRIVATE_DATA->handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_Trigger(0) -> %08x", result);
			CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_TEC, CCD_COOLER_ON_ITEM->sw.value ? 1 : 0);
		if (result >= 0)
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		else {
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_TEC) -> %08x", result);
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		result = Toupcam_put_Temperature(PRIVATE_DATA->handle, (short)(CCD_TEMPERATURE_ITEM->number.target * 10));
		if (result >= 0) {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value) {
				result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_TEC, 1);
				if (result >= 0) {
					indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
					CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_TEC, 1) -> %08x", result);
				}
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Toupcam_put_Temperature() -> %08x", result);
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CCD_ADVANCED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(X_CCD_ADVANCED_PROPERTY, property, false);
		X_CCD_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		result = Toupcam_put_Contrast(PRIVATE_DATA->handle, (int)X_CCD_CONTRAST_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Contrast(%d) -> %08x", (int)X_CCD_CONTRAST_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		result = Toupcam_put_Hue(PRIVATE_DATA->handle, (int)X_CCD_HUE_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Hue(%d) -> %08x", (int)X_CCD_HUE_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		result = Toupcam_put_Saturation(PRIVATE_DATA->handle, (int)X_CCD_SATURATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Saturation(%d) -> %08x", (int)X_CCD_SATURATION_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		result = Toupcam_put_Brightness(PRIVATE_DATA->handle, (int)X_CCD_BRIGHTNESS_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Brightness(%d) -> %08x", (int)X_CCD_BRIGHTNESS_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		result = Toupcam_put_Gamma(PRIVATE_DATA->handle, (int)X_CCD_GAMMA_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Gamma(%d) -> %08x", (int)X_CCD_GAMMA_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		result = Toupcam_put_Speed(PRIVATE_DATA->handle, (int)X_CCD_SPEED_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Speed(%d) -> %08x", (int)X_CCD_SPEED_ITEM->number.value);
		if (result < 0)
			X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		if (X_CCD_ADVANCED_PROPERTY->count > 6) {
			result = Toupcam_put_Option(PRIVATE_DATA->handle, TOUPCAM_OPTION_FAN, (int)X_CCD_FAN_SPEED_ITEM->number.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Toupcam_put_Option(TOUPCAM_OPTION_FAN, %d) -> %08x", (int)X_CCD_FAN_SPEED_ITEM->number.value);
			if (result < 0)
				X_CCD_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	if (device == device->master_device)
		indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation


static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	indigo_release_property(X_CCD_ADVANCED_PROPERTY);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (PRIVATE_DATA->handle == NULL) {
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
				} else {
					PRIVATE_DATA->handle = Toupcam_Open(PRIVATE_DATA->cam.id);
				}
			}
			device->gp_bits = 1;
			if (PRIVATE_DATA->handle) {
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				device->gp_bits = 0;
			}
		} else {
			if (PRIVATE_DATA->buffer != NULL) {
				free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
			}
			if (PRIVATE_DATA->camera && PRIVATE_DATA->camera->gp_bits == 0) {
				if (((touptek_private_data *)PRIVATE_DATA->camera->private_data)->handle == NULL) {
					pthread_mutex_lock(&PRIVATE_DATA->mutex);
					Toupcam_Close(PRIVATE_DATA->handle);
					pthread_mutex_unlock(&PRIVATE_DATA->mutex);
					indigo_global_unlock(device);
				}
				PRIVATE_DATA->handle = NULL;
			}
			device->gp_bits = 0;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		HRESULT result = 0;
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0)
			result = Toupcam_ST4PlusGuide(PRIVATE_DATA->handle, 0, GUIDER_GUIDE_NORTH_ITEM->number.value);
		else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0)
			result = Toupcam_ST4PlusGuide(PRIVATE_DATA->handle, 1, GUIDER_GUIDE_SOUTH_ITEM->number.value);
		GUIDER_GUIDE_DEC_PROPERTY->state = SUCCEEDED(result) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		HRESULT result = 0;
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (GUIDER_GUIDE_EAST_ITEM->number.value > 0)
			result = Toupcam_ST4PlusGuide(PRIVATE_DATA->handle, 2, GUIDER_GUIDE_EAST_ITEM->number.value);
		else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0)
			result = Toupcam_ST4PlusGuide(PRIVATE_DATA->handle, 3, GUIDER_GUIDE_WEST_ITEM->number.value);
		GUIDER_GUIDE_RA_PROPERTY->state = SUCCEEDED(result) ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	if (device == device->master_device)
		indigo_global_unlock(device);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static bool hotplug_callback_initialized = false;
static indigo_device *devices[TOUPCAM_MAX];

static void hotplug_callback(void* pCallbackCtx) {
	for (int i = 0; i < TOUPCAM_MAX; i++) {
		indigo_device *device = devices[i];
		if (device)
			PRIVATE_DATA->present = false;
	}
	ToupcamInstV2 cams[TOUPCAM_MAX];
	int cnt = Toupcam_EnumV2(cams);
	for (int j = 0; j < cnt; j++) {
		ToupcamInstV2 cam = cams[j];
		bool found = false;
		for (int i = 0; i < TOUPCAM_MAX; i++) {
			indigo_device *device = devices[i];
			if (device && memcmp(PRIVATE_DATA->cam.id, cam.id, sizeof(64)) == 0) {
				found = true;
				PRIVATE_DATA->present = true;
				break;
			}
		}
		if (!found) {
			static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
				"",
				ccd_attach,
				ccd_enumerate_properties,
				ccd_change_property,
				NULL,
				ccd_detach
				);
			touptek_private_data *private_data = malloc(sizeof(touptek_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(touptek_private_data));
			private_data->cam = cam;
			private_data->present = true;
			indigo_device *camera = malloc(sizeof(indigo_device));
			assert(camera != NULL);
			memcpy(camera, &ccd_template, sizeof(indigo_device));
			snprintf(camera->name, INDIGO_NAME_SIZE, "ToupTek %s #%s", cam.displayname, cam.id);
			camera->private_data = private_data;
			private_data->camera = camera;
			for (int i = 0; i < TOUPCAM_MAX; i++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = camera);
					break;
				}
			}
			if (cam.model->flag & TOUPCAM_FLAG_ST4) {
				static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
				 "",
				 guider_attach,
				 indigo_guider_enumerate_properties,
				 guider_change_property,
				 NULL,
				 guider_detach
				 );
				indigo_device *guider = malloc(sizeof(indigo_device));
				assert(guider != NULL);
				memcpy(guider, &guider_template, sizeof(indigo_device));
				snprintf(guider->name, INDIGO_NAME_SIZE, "ToupTek %s (guider) #%s", cam.displayname, cam.id);
				guider->private_data = private_data;
				private_data->guider = guider;
				indigo_async((void *)(void *)indigo_attach_device, guider);
			}
		}
	}
	for (int i = 0; i < TOUPCAM_MAX; i++) {
		indigo_device *device = devices[i];
		if (device && !PRIVATE_DATA->present) {
			indigo_device *guider = PRIVATE_DATA->guider;
			if (guider) {
				indigo_detach_device(guider);
				free(guider);
			}
			indigo_detach_device(device);
			if (device->private_data)
				free(device->private_data);
			free(device);
			devices[i] = NULL;
		}
	}
}


indigo_result indigo_ccd_touptek(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, "ToupTek Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch(action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			for (int i = 0; i < TOUPCAM_MAX; i++)
				devices[i] = NULL;
			if (!hotplug_callback_initialized) {
				Toupcam_HotPlug(hotplug_callback, NULL);
				hotplug_callback_initialized = true;
			}
			INDIGO_DRIVER_LOG(DRIVER_NAME, "ToupTek SDK version %s", Toupcam_Version());
			hotplug_callback(NULL);
			break;
		}
		case INDIGO_DRIVER_SHUTDOWN:
			break;
			
		case INDIGO_DRIVER_INFO:
			break;
	}
	
	return INDIGO_OK;
}
