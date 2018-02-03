// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO IIDC CCD driver
 \file indigo_ccd_iidc.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_iidc"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <dc1394/dc1394.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_iidc.h"

#define PRIVATE_DATA        ((iidc_private_data *)device->private_data)

struct {
	char *name;
	unsigned bits_per_pixel;
} COLOR_CODING[] = {
	{ "MONO 8", 8 },
	{ "YUV 4:1:1", 12 },
	{ "YUV 4:2:2", 16 },
	{ "YUV 4:4:4", 24 },
	{ "RGB 8", 8 },
	{ "MONO 16", 16 },
	{ "RGB 16", 16 },
	{ "MONO 16S", 16 },
	{ "RGB 16S", 16 },
	{ "RAW 8", 8 },
	{ "RAW 16", 16 }
};

typedef struct {
	dc1394video_mode_t mode;
	dc1394color_coding_t color_coding;
	unsigned width;
	unsigned height;
	unsigned bits_per_pixel;
	unsigned width_unit;
	unsigned height_unit;
} iidc_mode_data;

typedef struct {
	dc1394camera_t *camera;
	uint64_t guid;
	uint16_t unit;
	bool present;
	bool connected;
	iidc_mode_data mode_data[64];
	int mode_data_ix;
	int device_count;
	dc1394bool_t temperature_is_present, gain_is_present, gamma_is_present;
	indigo_timer *exposure_timer, *temperture_timer;
	unsigned char *buffer;
} iidc_private_data;

static void stop_camera(indigo_device *device) {
	if (PRIVATE_DATA->connected) {
    dc1394error_t err = dc1394_video_set_transmission(PRIVATE_DATA->camera, DC1394_OFF);
    INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_transmission() -> %s", dc1394_error_get_string(err));
		err = dc1394_capture_stop(PRIVATE_DATA->camera);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_stop() -> %s", dc1394_error_get_string(err));
	}
	PRIVATE_DATA->connected = false;
}

static void setup_camera(indigo_device *device) {
	stop_camera(device);
	if (!PRIVATE_DATA->connected) {
		dc1394error_t err;
		uint32_t packet_size;
		dc1394video_mode_t mode = PRIVATE_DATA->mode_data[PRIVATE_DATA->mode_data_ix].mode;
		err = dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAIN, CCD_GAIN_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_absolute_value(DC1394_FEATURE_GAIN, %g) -> %s", CCD_GAIN_ITEM->number.value, dc1394_error_get_string(err));
		err = dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAMMA, CCD_GAMMA_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_absolute_value(DC1394_FEATURE_GAMMA, %g) -> %s", CCD_GAMMA_ITEM->number.value, dc1394_error_get_string(err));
		err = dc1394_format7_set_image_position(PRIVATE_DATA->camera, mode, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_image_position(%d, %d) -> %s", (int)CCD_FRAME_LEFT_ITEM->number.value, (int)CCD_FRAME_TOP_ITEM->number.value, dc1394_error_get_string(err));
		err = dc1394_format7_set_image_size(PRIVATE_DATA->camera, mode, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_image_size(%d, %d) -> %s", (int)CCD_FRAME_WIDTH_ITEM->number.value, (int)CCD_FRAME_HEIGHT_ITEM->number.value, dc1394_error_get_string(err));
		err = dc1394_format7_get_recommended_packet_size(PRIVATE_DATA->camera, mode, &packet_size);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_get_recommended_packet_size() -> %s", dc1394_error_get_string(err));
		err = dc1394_format7_set_packet_size(PRIVATE_DATA->camera, mode, packet_size);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_packet_size(%d) -> %s", packet_size, dc1394_error_get_string(err));
		err = dc1394_capture_setup(PRIVATE_DATA->camera, 2, DC1394_CAPTURE_FLAGS_DEFAULT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_setup() -> %s", dc1394_error_get_string(err));
		PRIVATE_DATA->connected = true;
	}
}

static bool setup_feature(indigo_device *device, indigo_item *item, dc1394feature_t feature) {
	dc1394feature_info_t info;
	info.id = feature;
	INDIGO_DEBUG_DRIVER(const char *f = dc1394_feature_get_string(feature));
	dc1394error_t err = dc1394_feature_get(PRIVATE_DATA->camera, &info);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_get(%s) -> %s", f, dc1394_error_get_string(err));
	if (err == DC1394_SUCCESS && info.available) {
		if (info.is_on != DC1394_ON) {
			err = dc1394_feature_set_power(PRIVATE_DATA->camera, feature, DC1394_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_power(%s, DC1394_ON) -> %s", f, dc1394_error_get_string(err));
		}
		if (info.current_mode != DC1394_FEATURE_MODE_MANUAL) {
			err = dc1394_feature_set_mode(PRIVATE_DATA->camera, feature, DC1394_FEATURE_MODE_MANUAL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_mode(%s, DC1394_FEATURE_MODE_MANUAL) -> %s", f, dc1394_error_get_string(err));
		}
		if (info.abs_control != DC1394_ON) {
			err = dc1394_feature_set_absolute_control(PRIVATE_DATA->camera, feature, DC1394_ON);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_absolute_control(%s, DC1394_ON) -> %s", f, dc1394_error_get_string(err));
		}
		item->number.value = info.abs_value;
		item->number.min = info.abs_min;
		if (item->number.value <  info.abs_min)
			item->number.value =  info.abs_min;
		item->number.max =  info.abs_max;
		if (item->number.value > info.abs_max)
			item->number.value = info.abs_max;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "feature %s: value=%g min=%g max=%g", f, info.abs_value, info.abs_min, info.abs_max);
		return true;
	}
	return false;
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		dc1394error_t err = dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_SHUTTER, CCD_EXPOSURE_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_absolute_value(DC1394_FEATURE_SHUTTER, %g) -> %s", CCD_EXPOSURE_ITEM->number.value, dc1394_error_get_string(err));
		setup_camera(device);
		err = dc1394_video_set_one_shot(PRIVATE_DATA->camera, DC1394_ON);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_one_shot(DC1394_ON) -> %s", dc1394_error_get_string(err));
		dc1394video_frame_t *frame;
		err = dc1394_capture_dequeue(PRIVATE_DATA->camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_dequeue() -> %s", dc1394_error_get_string(err));
		if (err == DC1394_SUCCESS) {
			void *data = frame->image;
			assert(data != NULL);
      int width = frame->size[0];
      int height = frame->size[1];
      int size = frame->image_bytes;
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, data, size);
      err = dc1394_capture_enqueue(PRIVATE_DATA->camera, frame);
      INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_enqueue() -> %s", dc1394_error_get_string(err));
			indigo_process_image(device, PRIVATE_DATA->buffer, width, height, frame->little_endian, NULL);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
  stop_camera(device);
}

static void streaming_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	dc1394error_t err = dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_SHUTTER, CCD_STREAMING_EXPOSURE_ITEM->number.value);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_absolute_value(DC1394_FEATURE_SHUTTER, %g) -> %s", CCD_STREAMING_EXPOSURE_ITEM->number.value, dc1394_error_get_string(err));
	setup_camera(device);
	err = dc1394_video_set_transmission(PRIVATE_DATA->camera, DC1394_ON);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_transmission(DC1394_ON) -> %s", dc1394_error_get_string(err));
	while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {
		dc1394video_frame_t *frame;
		err = dc1394_capture_dequeue(PRIVATE_DATA->camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_dequeue() -> %s", dc1394_error_get_string(err));
		if (err == DC1394_SUCCESS) {
			void *data = frame->image;
			assert(data != NULL);
      int width = frame->size[0];
      int height = frame->size[1];
      int size = frame->image_bytes;
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, data, size);
      err = dc1394_capture_enqueue(PRIVATE_DATA->camera, frame);
      INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_capture_enqueue() -> %s", dc1394_error_get_string(err));
			indigo_process_image(device, PRIVATE_DATA->buffer, width, height, frame->little_endian, NULL);
		} else {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
			break;
		}
		if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
			CCD_STREAMING_COUNT_ITEM->number.value -= 1;
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	}
	CCD_STREAMING_COUNT_ITEM->number.value = 0;
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
  stop_camera(device);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	uint32_t target_temperature, temperature;
	dc1394error_t err = dc1394_feature_temperature_get_value(PRIVATE_DATA->camera, &target_temperature, &temperature);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_temperature_get_value() -> %s (%u, %u)", dc1394_error_get_string(err),  target_temperature, temperature);
	if (err == DC1394_SUCCESS) {
		CCD_TEMPERATURE_ITEM->number.value = (temperature & 0xFFF)/10.0-273.15;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
	} else {
		PRIVATE_DATA->temperture_timer = NULL;
	}
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- CCD_MODE, CCD_INFO, CCD_FRAME
		dc1394error_t err;
		dc1394video_modes_t modes;
		char name[128], label[128];
		err = dc1394_video_get_supported_modes(PRIVATE_DATA->camera, &modes);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_get_supported_modes() -> %s", dc1394_error_get_string(err));
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 0;
		CCD_INFO_WIDTH_ITEM->number.value = 0;
		CCD_INFO_HEIGHT_ITEM->number.value = 0;
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = 4;
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 4;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = 4;
		CCD_INFO_WIDTH_ITEM->number.value = 0;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 0;
		iidc_mode_data *mode_data = PRIVATE_DATA->mode_data;
		for (int i = 0; i < modes.num; i++) {
			dc1394video_mode_t mode = modes.modes[i];
			if (mode >= DC1394_VIDEO_MODE_FORMAT7_MIN && mode <= DC1394_VIDEO_MODE_FORMAT7_MAX) {
				dc1394color_codings_t color_codings;
				dc1394_format7_get_color_codings(PRIVATE_DATA->camera, mode, &color_codings);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_get_color_codings(%d) -> %s", mode, dc1394_error_get_string(err));
				for (int j = 0; j < color_codings.num; j++) {
					mode_data->mode = mode;
					mode_data->color_coding = color_codings.codings[j];
					dc1394_format7_get_max_image_size(PRIVATE_DATA->camera, mode, &mode_data->width, &mode_data->height);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_get_max_image_size(%d) -> %s", mode, dc1394_error_get_string(err));
					dc1394_format7_get_unit_size(PRIVATE_DATA->camera, mode, &mode_data->width_unit, &mode_data->height_unit);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_get_unit_size(%d) -> %s", mode, dc1394_error_get_string(err));
					mode_data->bits_per_pixel = COLOR_CODING[mode_data->color_coding-DC1394_COLOR_CODING_MONO8].bits_per_pixel;
					snprintf(name, sizeof(name), "MODE_%d", CCD_MODE_PROPERTY->count);
					snprintf(label, sizeof(label), "%s %dx%d", COLOR_CODING[mode_data->color_coding-DC1394_COLOR_CODING_MONO8].name, mode_data->width, mode_data->height);
					indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, label, false);
					if (CCD_INFO_WIDTH_ITEM->number.value < mode_data->width)
						CCD_INFO_WIDTH_ITEM->number.value = mode_data->width;
					if (CCD_INFO_HEIGHT_ITEM->number.value < mode_data->height)
						CCD_INFO_HEIGHT_ITEM->number.value = mode_data->height;
					if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value < mode_data->bits_per_pixel)
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = mode_data->bits_per_pixel;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "MODE_%d: %s %dx%d [%dx%d]", CCD_MODE_PROPERTY->count, COLOR_CODING[mode_data->color_coding-DC1394_COLOR_CODING_MONO8].name, mode_data->width, mode_data->height, mode_data->width_unit, mode_data->height_unit);
					mode_data++;
					CCD_MODE_PROPERTY->count++;
				}
			}
		}
		mode_data = PRIVATE_DATA->mode_data;
		dc1394_format7_set_color_coding(PRIVATE_DATA->camera, mode_data->mode, mode_data->color_coding);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_color_coding(%d, %d) -> %s", mode_data->mode, mode_data->color_coding, dc1394_error_get_string(err));
		dc1394_video_set_mode(PRIVATE_DATA->camera, mode_data->mode);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_mode(%d) -> %s", mode_data->mode, dc1394_error_get_string(err));
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = mode_data->width;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = mode_data->height;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = mode_data->bits_per_pixel;
		CCD_MODE_ITEM->sw.value = true;
		PRIVATE_DATA->mode_data_ix = 0;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
    setup_feature(device, CCD_EXPOSURE_ITEM, DC1394_FEATURE_SHUTTER);
		CCD_STREAMING_EXPOSURE_ITEM->number.min = CCD_EXPOSURE_ITEM->number.min;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = CCD_EXPOSURE_ITEM->number.max;
		CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.value;
		err = dc1394_feature_set_power(PRIVATE_DATA->camera, DC1394_FEATURE_FRAME_RATE, DC1394_OFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_set_power(DC1394_FEATURE_FRAME_RATE, DC1394_OFF) -> %s", dc1394_error_get_string(err));
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- CCD_GAIN
		if (setup_feature(device, CCD_GAIN_ITEM, DC1394_FEATURE_GAIN)) {
			CCD_GAIN_PROPERTY->hidden = false;
			PRIVATE_DATA->gain_is_present = true;
		}
		// -------------------------------------------------------------------------------- CCD_GAMMA
		if (setup_feature(device, CCD_GAMMA_ITEM, DC1394_FEATURE_GAMMA)) {
			CCD_GAMMA_PROPERTY->hidden = false;
			PRIVATE_DATA->gamma_is_present = true;
		}
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		err = dc1394_feature_is_present(PRIVATE_DATA->camera, DC1394_FEATURE_TEMPERATURE, &PRIVATE_DATA->temperature_is_present);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_feature_is_present(DC1394_FEATURE_TEMPERATURE) -> %s", dc1394_error_get_string(err));
		if (err == DC1394_SUCCESS) {
			CCD_TEMPERATURE_PROPERTY->hidden = !PRIVATE_DATA->temperature_is_present;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
		}
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(FITS_HEADER_SIZE + 2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value);
			assert(PRIVATE_DATA->buffer != NULL);
			if (PRIVATE_DATA->temperature_is_present) {
				PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 0, ccd_temperature_callback);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperture_timer);
			stop_camera(device);
			if (PRIVATE_DATA->buffer != NULL) {
				free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_MODE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, "Camera is busy");
		} else {
			bool update_frame = false;
			indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
			stop_camera(device);
			for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
				indigo_item *item = &CCD_MODE_PROPERTY->items[i];
				if (item->sw.value) {
					iidc_mode_data *mode_data = &PRIVATE_DATA->mode_data[PRIVATE_DATA->mode_data_ix = i];
					dc1394error_t err = dc1394_format7_set_color_coding(PRIVATE_DATA->camera, mode_data->mode, mode_data->color_coding);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_color_coding(%d, %d) -> %s", mode_data->mode, mode_data->color_coding, dc1394_error_get_string(err));
					err = dc1394_video_set_mode(PRIVATE_DATA->camera, mode_data->mode);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_mode(%d) -> %s", mode_data->mode, dc1394_error_get_string(err));
					err = dc1394_format7_set_packet_size(PRIVATE_DATA->camera, mode_data->mode, DC1394_USE_MAX_AVAIL);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_format7_set_packet_size() -> %s", dc1394_error_get_string(err));
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = mode_data->width;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = mode_data->height;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = mode_data->bits_per_pixel;
					setup_feature(device, CCD_EXPOSURE_ITEM, DC1394_FEATURE_SHUTTER);
					CCD_STREAMING_EXPOSURE_ITEM->number.min = CCD_EXPOSURE_ITEM->number.min;
					CCD_STREAMING_EXPOSURE_ITEM->number.max = CCD_EXPOSURE_ITEM->number.max;
					CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.value;
					CCD_FRAME_PROPERTY->state = INDIGO_IDLE_STATE;
					indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
					indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
					CCD_EXPOSURE_PROPERTY->state = INDIGO_IDLE_STATE;
					indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
					indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
					CCD_STREAMING_PROPERTY->state = INDIGO_IDLE_STATE;
					indigo_delete_property(device, CCD_STREAMING_PROPERTY, NULL);
					indigo_define_property(device, CCD_STREAMING_PROPERTY, NULL);
					break;
				}
			}
			if (IS_CONNECTED) {
				CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		iidc_mode_data *mode_data = &PRIVATE_DATA->mode_data[PRIVATE_DATA->mode_data_ix];
		CCD_FRAME_LEFT_ITEM->number.value = (((int)CCD_FRAME_LEFT_ITEM->number.value) / mode_data->width_unit) * mode_data->width_unit;
		CCD_FRAME_TOP_ITEM->number.value = (((int)CCD_FRAME_TOP_ITEM->number.value) / mode_data->height_unit) * mode_data->height_unit;
		CCD_FRAME_WIDTH_ITEM->number.value = (((int)CCD_FRAME_WIDTH_ITEM->number.value) / mode_data->width_unit) * mode_data->width_unit;
		if (CCD_FRAME_WIDTH_ITEM->number.value == 0)
			CCD_FRAME_WIDTH_ITEM->number.value = mode_data->width_unit;
		CCD_FRAME_HEIGHT_ITEM->number.value = (((int)CCD_FRAME_HEIGHT_ITEM->number.value) / mode_data->height_unit) * mode_data->height_unit;
		if (CCD_FRAME_HEIGHT_ITEM->number.value == 0)
			CCD_FRAME_HEIGHT_ITEM->number.value = mode_data->height_unit;
		if (IS_CONNECTED) {
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 0, exposure_timer_callback);
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 0, streaming_timer_callback);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
      dc1394error_t err = dc1394_video_set_transmission(PRIVATE_DATA->camera, DC1394_OFF);
      INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_transmission(DC1394_OFF) -> %s", dc1394_error_get_string(err));
			stop_camera(device);
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
			indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			return INDIGO_OK;
		} else if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			stop_camera(device);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *devices[MAX_DEVICES];
static dc1394_t *context;


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dc1394camera_list_t *list;
			dc1394error_t err=dc1394_camera_enumerate(context, &list);
			if (err != DC1394_SUCCESS) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_camera_enumerate() -> %s (%d)", dc1394_error_get_string(err),  list->num);
			} else {
				for (int i = 0; i < list->num; i++) {
					uint64_t guid = list->ids[i].guid;
					uint16_t unit = list->ids[i].unit;
					for (int j = 0; j < MAX_DEVICES; j++) {
						indigo_device *device = devices[j];
						if (device!= NULL && PRIVATE_DATA->guid == guid && PRIVATE_DATA->unit == unit) {
							guid = 0;
							break;
						}
					}
					if (guid == 0)
						continue;
					dc1394camera_t *camera = dc1394_camera_new_unit(context, guid, unit);
					if (camera) {
						INDIGO_DRIVER_LOG(DRIVER_NAME, "Camera %s detected", camera->model);
						if (strstr(camera->model, "CMLN") != NULL || strstr(camera->model, "FMVU") != NULL) {
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  forced DC1394_OPERATION_MODE_LEGACY");
							camera->bmode_capable = false;
						}
						if (camera->bmode_capable) {
							err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_operation_mode(DC1394_OPERATION_MODE_1394B) -> %s", dc1394_error_get_string(err));
							err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_iso_speed ->(DC1394_ISO_SPEED_800) -> %s", dc1394_error_get_string(err));
						} else {
							err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_LEGACY);
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_operation_mode(DC1394_OPERATION_MODE_LEGACY) -> %s", dc1394_error_get_string(err));
							err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_iso_speed(DC1394_ISO_SPEED_400) -> %s", dc1394_error_get_string(err));
						}
						INDIGO_DEBUG_DRIVER(dc1394_camera_print_info(camera, stderr));
						INDIGO_DEBUG_DRIVER(dc1394featureset_t features);
						INDIGO_DEBUG_DRIVER(dc1394_feature_get_all(camera, &features));
						INDIGO_DEBUG_DRIVER(dc1394_feature_print_all(&features, stderr));

						iidc_private_data *private_data = malloc(sizeof(iidc_private_data));
						assert(private_data != NULL);
						memset(private_data, 0, sizeof(iidc_private_data));
						private_data->camera = camera;
						private_data->guid = guid;
						private_data->unit = unit;
						indigo_device *device = malloc(sizeof(indigo_device));
						assert(device != NULL);
						memcpy(device, &ccd_template, sizeof(indigo_device));
						strncpy(device->name, camera->model, INDIGO_NAME_SIZE);
						device->private_data = private_data;
						for (int j = 0; j < MAX_DEVICES; j++) {
							if (devices[j] == NULL) {
								indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
								break;
							}
						}
					}
				}
				dc1394_camera_free_list(list);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			for (int j = 0; j < MAX_DEVICES; j++) {
				indigo_device *device = devices[j];
				if (device != NULL)
					PRIVATE_DATA->present = false;
			}
			dc1394camera_list_t *list;
			dc1394error_t err=dc1394_camera_enumerate(context, &list);
			if (err != DC1394_SUCCESS) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_camera_enumerate() -> %s (%d)", dc1394_error_get_string(err),  list->num);
			} else {
				for (int i = 0; i < list->num; i++) {
					uint64_t guid = list->ids[i].guid;
					for (int j = 0; j < MAX_DEVICES; j++) {
						indigo_device *device = devices[j];
						if (device != NULL && PRIVATE_DATA->guid == guid) {
							PRIVATE_DATA->present = true;
							break;
						}
					}
				}
				dc1394_camera_free_list(list);
			}
			iidc_private_data *private_data = NULL;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					if (!PRIVATE_DATA->present) {
						private_data = PRIVATE_DATA;
						INDIGO_DRIVER_LOG(DRIVER_NAME, "Camera %s removed", private_data->camera->model);
						indigo_detach_device(device);
						dc1394_camera_free(private_data->camera);
						free(private_data);
						free(device);
						devices[j] = NULL;
					}
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

static void errorlog_handler(dc1394log_t type, const char *message, void* user) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
}

static void debuglog_handler(dc1394log_t type, const char *message, void* user) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
}

//#ifdef INDIGO_MACOS
//
//#include <IOKit/IOKitLib.h>
//#include <IOKit/IOMessage.h>
//#include <IOKit/IOCFPlugIn.h>
//#include <IOKit/usb/IOUSBLib.h>
//
//static CFRunLoopRef runloop;
//
//static void firewire_devices_attached(void *ptr, io_iterator_t firewire_add_device_iterator) {
//	io_service_t device;
//	bool empty = true;
//	while ((device = IOIteratorNext(firewire_add_device_iterator)) != 0) {
//		IOObjectRelease(device);
//		empty = false;
//	}
//	if (!empty) {
//		INDIGO_DRIVER_LOG(DRIVER_NAME, "indigo_ccd_iidc: FireWire device attached");
//		hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
//	}
//}
//
//static void firewire_devices_detached(void *ptr, io_iterator_t firewire_remove_device_iterator) {
//	io_service_t device;
//	bool empty = true;
//	while ((device = IOIteratorNext (firewire_remove_device_iterator)) != 0) {
//		IOObjectRelease(device);
//		empty = false;
//	}
//	if (!empty) {
//		INDIGO_DRIVER_LOG(DRIVER_NAME, "indigo_ccd_iidc: FireWire device detached");
//		hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
//	}
//}
//
//static void *firewire_hotplug_thread(void *arg0) {
//	IOReturn kresult;
//	CFRunLoopSourceRef firewire_notification_cfsource;
//	IONotificationPortRef firewire_notification_port;
//	io_iterator_t firewire_remove_device_iterator;
//	io_iterator_t firewire_add_device_iterator;
//
//	pthread_setname_np("indigo.firewire-hotplug");
//
//	CFRetain(runloop = CFRunLoopGetCurrent());
//	CFRunLoopAddSource(runloop, firewire_notification_cfsource = IONotificationPortGetRunLoopSource (firewire_notification_port = IONotificationPortCreate(kIOMasterPortDefault)), kCFRunLoopDefaultMode);
//
//	kresult = IOServiceAddMatchingNotification(firewire_notification_port, kIOTerminatedNotification, IOServiceMatching("IOFireWireDevice"), firewire_devices_detached, NULL, &firewire_remove_device_iterator);
//	if (kresult != kIOReturnSuccess) {
//		indigo_error("indigo_ccd_iidc: Could not add hot-unplug event source: 0x%08x", kresult);
//		return NULL;
//	}
//	firewire_devices_detached(NULL, firewire_remove_device_iterator);
//
//	kresult = IOServiceAddMatchingNotification(firewire_notification_port, kIOMatchedNotification, IOServiceMatching("IOFireWireDevice"), firewire_devices_attached, NULL, &firewire_add_device_iterator);
//	if (kresult != kIOReturnSuccess) {
//		indigo_error("indigo_ccd_iidc: Could not add hot-plug event source: 0x%08x", kresult);
//		return NULL;
//	}
//	firewire_devices_attached(NULL, firewire_add_device_iterator);
//
//	INDIGO_DRIVER_LOG(DRIVER_NAME, "indigo_ccd_iidc: RunLoop for FireWire devices detection started");
//	CFRunLoopRun();
//	INDIGO_DRIVER_LOG(DRIVER_NAME, "indigo_ccd_iidc: RunLoop for FireWire devices detection finished");
//
//	CFRunLoopRemoveSource(runloop, firewire_notification_cfsource, kCFRunLoopDefaultMode);
//	IONotificationPortDestroy(firewire_notification_port);
//	IOObjectRelease(firewire_remove_device_iterator);
//	IOObjectRelease(firewire_add_device_iterator);
//	CFRelease(runloop);
//	return NULL;
//}
//
//#endif

indigo_result indigo_ccd_iidc(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "IIDC Compatible Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		context = dc1394_new();
		if (context != NULL) {
			dc1394_log_register_handler(DC1394_LOG_ERROR, errorlog_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_WARNING, errorlog_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_DEBUG, debuglog_handler, NULL);
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
//#ifdef INDIGO_MACOS
//			pthread_t hotplug_thread_handle;
//			pthread_create(&hotplug_thread_handle, NULL, firewire_hotplug_thread, NULL);
//#endif
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, /* LIBUSB_HOTPLUG_NO_FLAGS */ 0, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			hotplug_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback() ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
		}
		last_action = action;
		return INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
//#ifdef INDIGO_MACOS
//			CFRunLoopStop(runloop);
//#endif
		for (int j = 0; j < MAX_DEVICES; j++) {
			indigo_device *device = devices[j];
			if (device != NULL) {
				if (PRIVATE_DATA != NULL) {
					free(PRIVATE_DATA);
				}
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
		dc1394_free(context);
		context = NULL;
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
