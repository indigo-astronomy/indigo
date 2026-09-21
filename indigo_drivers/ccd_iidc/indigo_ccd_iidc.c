// Copyright (c) 2016-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_ccd_iidc.driver

// supported_architecture: defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <limits.h>
#include <math.h>
#include <dc1394/dc1394.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_iidc.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000012
#define DRIVER_NAME          "indigo_ccd_iidc"
#define DRIVER_LABEL         "IIDC Compatible Camera"
#define CCD_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((iidc_private_data *)device->private_data)

//+ define

#define IIDC_READOUT_TIMEOUT 30
#define IIDC_POLL_INTERVAL   0.01
#define IIDC_TEMPERATURE_INTERVAL 5
typedef struct {
	dc1394video_mode_t mode;
	dc1394color_coding_t coding;
	uint32_t width, height, width_unit, height_unit;
	unsigned bits_per_pixel;
} iidc_mode_data;

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	dc1394camera_t *camera;
	uint64_t guid;
	uint16_t unit;
	iidc_mode_data *modes;
	int mode_count, selected_mode;
	bool gain_present, gamma_present, temperature_present;
	bool capture_active, streaming;
	double capture_deadline;
	unsigned char *buffer;
	size_t buffer_size;
	//- data
} iidc_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

typedef struct {
	const char *name;
	unsigned bits_per_pixel;
} iidc_coding_info;

static const iidc_coding_info IIDC_CODINGS[] = {
	{ "MONO 8", 8 }, { "YUV 4:1:1", 24 }, { "YUV 4:2:2", 24 }, { "YUV 4:4:4", 24 }, { "RGB 8", 24 }, { "MONO 16", 16 }, { "RGB 16", 48 }, { "MONO 16S", 16 }, { "RGB 16S", 48 }, { "RAW 8", 8 }, { "RAW 16", 16 }
};

typedef struct {
	const char *name;
	unsigned width, height, bits_per_pixel;
} iidc_legacy_info;

static const iidc_legacy_info IIDC_LEGACY_MODES[] = {
	{ "YUV 4:4:4 160x120", 160, 120, 24 }, { "YUV 4:2:2 320x240", 320, 240, 24 }, { "YUV 4:1:1 640x480", 640, 480, 24 },
	{ "YUV 4:2:2 640x480", 640, 480, 24 }, { "RGB 8 640x480", 640, 480, 24 }, { "MONO 8 640x480", 640, 480, 8 },
	{ "MONO 16 640x480", 640, 480, 16 }, { "YUV 4:2:2 800x600", 800, 600, 24 }, { "RGB 8 800x600", 800, 600, 24 },
	{ "MONO 8 800x600", 800, 600, 8 }, { "YUV 4:2:2 1024x768", 1024, 768, 24 }, { "RGB 8 1024x768", 1024, 768, 24 },
	{ "MONO 8 1024x768", 1024, 768, 8 }, { "MONO 16 800x600", 800, 600, 16 }, { "MONO 16 1024x768", 1024, 768, 16 },
	{ "YUV 4:2:2 1280x960", 1280, 960, 24 }, { "RGB 8 1280x960", 1280, 960, 24 }, { "MONO 8 1280x960", 1280, 960, 8 },
	{ "YUV 4:2:2 1600x1200", 1600, 1200, 24 }, { "RGB 8 1600x1200", 1600, 1200, 24 }, { "MONO 8 1600x1200", 1600, 1200, 8 },
	{ "MONO 16 1280x960", 1280, 960, 16 }, { "MONO 16 1600x1200", 1600, 1200, 16 }
};

static dc1394_t *iidc_context;

static bool iidc_coding(dc1394color_coding_t coding, const iidc_coding_info **info) {
	int index = coding - DC1394_COLOR_CODING_MIN;
	if (index < 0 || index >= (int)(sizeof(IIDC_CODINGS) / sizeof(IIDC_CODINGS[0]))) {
		return false;
	}
	*info = IIDC_CODINGS + index;
	return true;
}

static void iidc_stop(indigo_device *device) {
	if (PRIVATE_DATA->capture_active) {
		dc1394error_t transmission = dc1394_video_set_transmission(PRIVATE_DATA->camera, DC1394_OFF);
		dc1394error_t capture = dc1394_capture_stop(PRIVATE_DATA->camera);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "dc1394_video_set_transmission(OFF) -> %s, dc1394_capture_stop() -> %s", dc1394_error_get_string(transmission), dc1394_error_get_string(capture));
	}
	PRIVATE_DATA->capture_active = PRIVATE_DATA->streaming = false;
}

static bool iidc_setup_feature(indigo_device *device, indigo_item *item, dc1394feature_t feature) {
	dc1394feature_info_t info = { .id = feature };
	if (dc1394_feature_get(PRIVATE_DATA->camera, &info) != DC1394_SUCCESS || !info.available || !isfinite(info.abs_min) || !isfinite(info.abs_max) || !isfinite(info.abs_value) || info.abs_min > info.abs_max) {
		return false;
	}
	if ((info.on_off_capable && info.is_on != DC1394_ON && dc1394_feature_set_power(PRIVATE_DATA->camera, feature, DC1394_ON) != DC1394_SUCCESS) || (info.current_mode != DC1394_FEATURE_MODE_MANUAL && dc1394_feature_set_mode(PRIVATE_DATA->camera, feature, DC1394_FEATURE_MODE_MANUAL) != DC1394_SUCCESS) || (info.abs_control != DC1394_ON && dc1394_feature_set_absolute_control(PRIVATE_DATA->camera, feature, DC1394_ON) != DC1394_SUCCESS)) {
		return false;
	}
	item->number.min = info.abs_min;
	item->number.max = info.abs_max;
	item->number.value = item->number.target = fmin(fmax(info.abs_value, info.abs_min), info.abs_max);
	return true;
}

static bool iidc_add_mode(indigo_device *device, iidc_mode_data mode, const char *label) {
	int count = PRIVATE_DATA->mode_count;
	if (count == INT_MAX / (int)sizeof(iidc_mode_data)) {
		return false;
	}
	iidc_mode_data *modes = indigo_safe_realloc(PRIVATE_DATA->modes, (size_t)(count + 1) * sizeof(iidc_mode_data));
	if (!modes) {
		return false;
	}
	PRIVATE_DATA->modes = modes;
	PRIVATE_DATA->modes[count] = mode;
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, count + 1);
	char name[32];
	snprintf(name, sizeof(name), "MODE_%d", count);
	indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, count == 0);
	PRIVATE_DATA->mode_count++;
	return true;
}

static bool iidc_discover_modes(indigo_device *device) {
	dc1394video_modes_t supported = { 0 };
	if (dc1394_video_get_supported_modes(PRIVATE_DATA->camera, &supported) != DC1394_SUCCESS || supported.num == 0) {
		return false;
	}
	PRIVATE_DATA->mode_count = 0;
	indigo_safe_free(PRIVATE_DATA->modes);
	PRIVATE_DATA->modes = NULL;
	CCD_MODE_PROPERTY->count = 0;
	for (uint32_t i = 0; i < supported.num; i++) {
		dc1394video_mode_t mode = supported.modes[i];
		if (mode >= DC1394_VIDEO_MODE_FORMAT7_MIN && mode <= DC1394_VIDEO_MODE_FORMAT7_MAX) {
			dc1394color_codings_t codings = { 0 };
			uint32_t width = 0, height = 0, width_unit = 0, height_unit = 0;
			if (dc1394_format7_get_color_codings(PRIVATE_DATA->camera, mode, &codings) != DC1394_SUCCESS || dc1394_format7_get_max_image_size(PRIVATE_DATA->camera, mode, &width, &height) != DC1394_SUCCESS || dc1394_format7_get_unit_size(PRIVATE_DATA->camera, mode, &width_unit, &height_unit) != DC1394_SUCCESS || width == 0 || height == 0 || width_unit == 0 || height_unit == 0) {
				continue;
			}
			for (uint32_t j = 0; j < codings.num; j++) {
				const iidc_coding_info *coding;
				if (!iidc_coding(codings.codings[j], &coding)) {
					continue;
				}
				char label[128];
				snprintf(label, sizeof(label), "%s %ux%u", coding->name, width, height);
				if (!iidc_add_mode(device, (iidc_mode_data) { mode, codings.codings[j], width, height, width_unit, height_unit, coding->bits_per_pixel }, label)) {
					return false;
				}
			}
		} else if (mode >= DC1394_VIDEO_MODE_160x120_YUV444 && mode < DC1394_VIDEO_MODE_EXIF) {
			int index = mode - DC1394_VIDEO_MODE_160x120_YUV444;
			if (index >= 0 && index < (int)(sizeof(IIDC_LEGACY_MODES) / sizeof(IIDC_LEGACY_MODES[0]))) {
				const iidc_legacy_info *legacy = IIDC_LEGACY_MODES + index;
				char label[128];
				snprintf(label, sizeof(label), "%s (legacy)", legacy->name);
				if (!iidc_add_mode(device, (iidc_mode_data) { mode, 0, legacy->width, legacy->height, legacy->width, legacy->height, legacy->bits_per_pixel }, label)) {
					return false;
				}
			}
		}
	}
	return PRIVATE_DATA->mode_count > 0;
}

static bool iidc_select_mode(indigo_device *device, int index) {
	if (index < 0 || index >= PRIVATE_DATA->mode_count) {
		return false;
	}
	iidc_stop(device);
	iidc_mode_data *mode = PRIVATE_DATA->modes + index;
	if (dc1394_video_set_mode(PRIVATE_DATA->camera, mode->mode) != DC1394_SUCCESS) {
		return false;
	}
	if (mode->mode >= DC1394_VIDEO_MODE_FORMAT7_MIN && mode->mode <= DC1394_VIDEO_MODE_FORMAT7_MAX && (dc1394_format7_set_image_position(PRIVATE_DATA->camera, mode->mode, 0, 0) != DC1394_SUCCESS || dc1394_format7_set_image_size(PRIVATE_DATA->camera, mode->mode, mode->width, mode->height) != DC1394_SUCCESS || dc1394_format7_set_color_coding(PRIVATE_DATA->camera, mode->mode, mode->coding) != DC1394_SUCCESS)) {
		return false;
	}
	PRIVATE_DATA->selected_mode = index;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = mode->width;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = mode->height;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = mode->bits_per_pixel;
	return true;
}

static bool iidc_initialize(indigo_device *device) {
	if (!iidc_discover_modes(device) || !iidc_select_mode(device, 0) || !iidc_setup_feature(device, CCD_EXPOSURE_ITEM, DC1394_FEATURE_SHUTTER)) {
		return false;
	}
	CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_STREAMING_EXPOSURE_ITEM->number.min = CCD_EXPOSURE_ITEM->number.min;
	CCD_STREAMING_EXPOSURE_ITEM->number.max = CCD_EXPOSURE_ITEM->number.max;
	CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target = CCD_EXPOSURE_ITEM->number.value;
	if (dc1394_feature_set_power(PRIVATE_DATA->camera, DC1394_FEATURE_FRAME_RATE, DC1394_OFF) != DC1394_SUCCESS) {
		return false;
	}
	PRIVATE_DATA->gain_present = iidc_setup_feature(device, CCD_GAIN_ITEM, DC1394_FEATURE_GAIN);
	PRIVATE_DATA->gamma_present = iidc_setup_feature(device, CCD_GAMMA_ITEM, DC1394_FEATURE_GAMMA);
	dc1394bool_t temperature = DC1394_FALSE;
	PRIVATE_DATA->temperature_present = dc1394_feature_is_present(PRIVATE_DATA->camera, DC1394_FEATURE_TEMPERATURE, &temperature) == DC1394_SUCCESS && temperature;
	CCD_GAIN_PROPERTY->hidden = !PRIVATE_DATA->gain_present;
	CCD_GAMMA_PROPERTY->hidden = !PRIVATE_DATA->gamma_present;
	CCD_TEMPERATURE_PROPERTY->hidden = !PRIVATE_DATA->temperature_present;
	CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
	uint32_t max_width = 0, max_height = 0;
	unsigned max_bpp = 0;
	for (int i = 0; i < PRIVATE_DATA->mode_count; i++) {
		max_width = max_width > PRIVATE_DATA->modes[i].width ? max_width : PRIVATE_DATA->modes[i].width;
		max_height = max_height > PRIVATE_DATA->modes[i].height ? max_height : PRIVATE_DATA->modes[i].height;
		max_bpp = max_bpp > PRIVATE_DATA->modes[i].bits_per_pixel ? max_bpp : PRIVATE_DATA->modes[i].bits_per_pixel;
	}
	if (max_width == 0 || max_height == 0 || max_bpp == 0 || (size_t)max_width > (SIZE_MAX - FITS_HEADER_SIZE) / max_height / 6) {
		return false;
	}
	CCD_INFO_WIDTH_ITEM->number.value = max_width;
	CCD_INFO_HEIGHT_ITEM->number.value = max_height;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = max_bpp;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 0;
	PRIVATE_DATA->buffer_size = FITS_HEADER_SIZE + (size_t)max_width * max_height * 6;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	return PRIVATE_DATA->buffer != NULL;
}

static void iidc_close(indigo_device *device);

static bool iidc_open(indigo_device *device) {
	bool result = PRIVATE_DATA->camera != NULL && iidc_initialize(device);
	if (!result) {
		iidc_close(device);
	}
	return result;
}

static void iidc_close(indigo_device *device) {
	iidc_stop(device);
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->buffer_size = 0;
	indigo_safe_free(PRIVATE_DATA->modes);
	PRIVATE_DATA->modes = NULL;
	PRIVATE_DATA->mode_count = 0;
}

static bool iidc_setup_capture(indigo_device *device, bool streaming) {
	iidc_mode_data *mode = PRIVATE_DATA->modes + PRIVATE_DATA->selected_mode;
	double exposure = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.value : CCD_EXPOSURE_ITEM->number.target;
	if (dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_SHUTTER, exposure) != DC1394_SUCCESS) {
		return false;
	}
	if (PRIVATE_DATA->gain_present && dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAIN, CCD_GAIN_ITEM->number.value) != DC1394_SUCCESS) {
		return false;
	}
	if (PRIVATE_DATA->gamma_present && dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAMMA, CCD_GAMMA_ITEM->number.value) != DC1394_SUCCESS) {
		return false;
	}
	if (mode->mode >= DC1394_VIDEO_MODE_FORMAT7_MIN && mode->mode <= DC1394_VIDEO_MODE_FORMAT7_MAX) {
		uint32_t packet_size = 0;
		if (dc1394_format7_set_image_position(PRIVATE_DATA->camera, mode->mode, 0, 0) != DC1394_SUCCESS || dc1394_format7_set_image_size(PRIVATE_DATA->camera, mode->mode, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value) != DC1394_SUCCESS || dc1394_format7_set_image_position(PRIVATE_DATA->camera, mode->mode, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value) != DC1394_SUCCESS || dc1394_format7_get_recommended_packet_size(PRIVATE_DATA->camera, mode->mode, &packet_size) != DC1394_SUCCESS || packet_size == 0 || dc1394_format7_set_packet_size(PRIVATE_DATA->camera, mode->mode, packet_size) != DC1394_SUCCESS) {
			return false;
		}
	}
	if (dc1394_capture_setup(PRIVATE_DATA->camera, 5, DC1394_CAPTURE_FLAGS_DEFAULT) != DC1394_SUCCESS) {
		return false;
	}
	PRIVATE_DATA->capture_active = true;
	PRIVATE_DATA->streaming = streaming;
	dc1394error_t result = streaming ? dc1394_video_set_transmission(PRIVATE_DATA->camera, DC1394_ON) : dc1394_video_set_one_shot(PRIVATE_DATA->camera, DC1394_ON);
	if (result != DC1394_SUCCESS) {
		iidc_stop(device);
		return false;
	}
	PRIVATE_DATA->capture_deadline = indigo_monotonic_time() + exposure + IIDC_READOUT_TIMEOUT;
	return true;
}

static bool iidc_process_frame(indigo_device *device, dc1394video_frame_t *frame, bool streaming) {
	bool valid = frame && frame->image && frame->size[0] > 0 && frame->size[1] > 0 && frame->image_bytes > 0;
	uint32_t width = frame ? frame->size[0] : 0;
	uint32_t height = frame ? frame->size[1] : 0;
	dc1394bool_t little_endian = frame ? frame->little_endian : DC1394_FALSE;
	unsigned bpp = frame ? frame->data_depth : 0;
	bool yuv = frame && (frame->color_coding == DC1394_COLOR_CODING_YUV411 || frame->color_coding == DC1394_COLOR_CODING_YUV422 || frame->color_coding == DC1394_COLOR_CODING_YUV444);
	const iidc_coding_info *coding = NULL;
	if (frame && iidc_coding(frame->color_coding, &coding)) {
		bpp = coding->bits_per_pixel;
	}
	size_t output_size = valid && bpp > 0 && (size_t)frame->size[0] <= SIZE_MAX / frame->size[1] / bpp ? (size_t)frame->size[0] * frame->size[1] * bpp / 8 : SIZE_MAX;
	valid = valid && output_size <= PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE;
	if (valid && yuv) {
		valid = dc1394_convert_to_RGB8(frame->image, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->size[0], frame->size[1], frame->yuv_byte_order, frame->color_coding, 0) == DC1394_SUCCESS;
		bpp = 24;
	} else if (valid && frame->image_bytes == output_size && frame->image_bytes <= PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
		memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->image, frame->image_bytes);
	} else {
		valid = false;
	}
	if (frame && dc1394_capture_enqueue(PRIVATE_DATA->camera, frame) != DC1394_SUCCESS) {
		valid = false;
	}
	if (valid) {
		indigo_process_image(device, PRIVATE_DATA->buffer, width, height, bpp, little_endian, true, NULL, streaming);
	}
	return valid;
}

static void iidc_acquisition_failure(indigo_device *device, indigo_property *property, const char *message) {
	iidc_stop(device);
	indigo_ccd_failure_cleanup(device);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->capture_active || CCD_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	dc1394video_frame_t *frame = NULL;
	dc1394error_t result = dc1394_capture_dequeue(PRIVATE_DATA->camera, DC1394_CAPTURE_POLICY_POLL, &frame);
	if (result == DC1394_SUCCESS && frame == NULL && indigo_monotonic_time() < PRIVATE_DATA->capture_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, IIDC_POLL_INTERVAL, exposure_finalizer);
		return;
	}
	if (result != DC1394_SUCCESS || !iidc_process_frame(device, frame, false)) {
		iidc_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, frame ? "Invalid image data" : "Exposure timed out or readout failed");
		return;
	}
	iidc_stop(device);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void streaming_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->capture_active || CCD_STREAMING_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	dc1394video_frame_t *frame = NULL;
	dc1394error_t result = dc1394_capture_dequeue(PRIVATE_DATA->camera, DC1394_CAPTURE_POLICY_POLL, &frame);
	if (result == DC1394_SUCCESS && frame == NULL && indigo_monotonic_time() < PRIVATE_DATA->capture_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, IIDC_POLL_INTERVAL, streaming_finalizer);
		return;
	}
	if (result != DC1394_SUCCESS || !iidc_process_frame(device, frame, true)) {
		iidc_acquisition_failure(device, CCD_STREAMING_PROPERTY, frame ? "Invalid streaming frame" : "Streaming timed out or readout failed");
		indigo_finalize_video_stream(device);
		return;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		iidc_stop(device);
		indigo_finalize_video_stream(device);
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->capture_deadline = indigo_monotonic_time() + CCD_STREAMING_EXPOSURE_ITEM->number.value + IIDC_READOUT_TIMEOUT;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
}

static void iidc_log_handler(dc1394log_t type, const char *message, void *user) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libdc1394: %s", message);
}

//- code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	if (PRIVATE_DATA->temperature_present) {
		uint32_t target = 0, temperature = 0;
		if (dc1394_feature_temperature_get_value(PRIVATE_DATA->camera, &target, &temperature) == DC1394_SUCCESS) {
			CCD_TEMPERATURE_ITEM->number.value = (temperature & 0xFFF) / 10.0 - 273.15;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, IIDC_TEMPERATURE_INTERVAL, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = iidc_open(device);
		if (connection_result) {
			//+ ccd.on_connect
			connection_result = PRIVATE_DATA->buffer != NULL;
			//- ccd.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		iidc_stop(device);
		//- ccd.on_disconnect
		iidc_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, ccd_timer_callback);
	}
}

static void ccd_mode_handler(indigo_device *device) {
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_MODE.on_change
	int selected = -1;
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		if (CCD_MODE_PROPERTY->items[i].sw.value) {
			selected = i;
			break;
		}
	}
	if (!iidc_select_mode(device, selected) || !iidc_setup_feature(device, CCD_EXPOSURE_ITEM, DC1394_FEATURE_SHUTTER)) {
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_STREAMING_EXPOSURE_ITEM->number.min = CCD_EXPOSURE_ITEM->number.min;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = CCD_EXPOSURE_ITEM->number.max;
		CCD_FRAME_PROPERTY->state = CCD_EXPOSURE_PROPERTY->state = CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	}
	//- ccd.CCD_MODE.on_change
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	iidc_mode_data *mode = PRIVATE_DATA->modes + PRIVATE_DATA->selected_mode;
	if (mode->mode < DC1394_VIDEO_MODE_FORMAT7_MIN || mode->mode > DC1394_VIDEO_MODE_FORMAT7_MAX) {
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		uint32_t left = (uint32_t)CCD_FRAME_LEFT_ITEM->number.value / mode->width_unit * mode->width_unit;
		uint32_t top = (uint32_t)CCD_FRAME_TOP_ITEM->number.value / mode->height_unit * mode->height_unit;
		uint32_t requested_width = (uint32_t)CCD_FRAME_WIDTH_ITEM->number.value / mode->width_unit * mode->width_unit;
		uint32_t requested_height = (uint32_t)CCD_FRAME_HEIGHT_ITEM->number.value / mode->height_unit * mode->height_unit;
		uint32_t width = mode->width_unit > requested_width ? mode->width_unit : requested_width;
		uint32_t height = mode->height_unit > requested_height ? mode->height_unit : requested_height;
		if (left >= mode->width || top >= mode->height) {
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			uint32_t available_width = (mode->width - left) / mode->width_unit * mode->width_unit;
			uint32_t available_height = (mode->height - top) / mode->height_unit * mode->height_unit;
			width = width < available_width ? width : available_width;
			height = height < available_height ? height : available_height;
			CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = left;
			CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = top;
			CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = width;
			CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = height;
			CCD_FRAME_PROPERTY->state = width && height ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		}
	}
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	if (iidc_setup_capture(device, false)) {
		indigo_ccd_exposure_setup(device);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, exposure_finalizer);
	} else {
		iidc_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, "Capture setup failed");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
	if (iidc_setup_capture(device, true)) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
	} else {
		iidc_acquisition_failure(device, CCD_STREAMING_PROPERTY, "Streaming setup failed");
		indigo_finalize_video_stream(device);
	}
	//- ccd.CCD_STREAMING.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, exposure_finalizer);
	indigo_cancel_pending_handler(device, streaming_finalizer);
	iidc_stop(device);
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_finalize_video_stream(device);
	}
	indigo_ccd_abort_exposure_cleanup(device);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	if (dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAIN, CCD_GAIN_ITEM->number.target) != DC1394_SUCCESS) {
		CCD_GAIN_ITEM->number.target = CCD_GAIN_ITEM->number.value;
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target;
	}
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_gamma_handler(indigo_device *device) {
	CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAMMA.on_change
	if (dc1394_feature_set_absolute_value(PRIVATE_DATA->camera, DC1394_FEATURE_GAMMA, CCD_GAMMA_ITEM->number.target) != DC1394_SUCCESS) {
		CCD_GAMMA_ITEM->number.target = CCD_GAMMA_ITEM->number.value;
		CCD_GAMMA_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_GAMMA_ITEM->number.value = CCD_GAMMA_ITEM->number.target;
	}
	//- ccd.CCD_GAMMA.on_change
	indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->camera->model && PRIVATE_DATA->camera->model[0] ? PRIVATE_DATA->camera->model : DRIVER_LABEL);
		snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE, "%016llx-%u", (unsigned long long)PRIVATE_DATA->guid, PRIVATE_DATA->unit);
		CCD_BIN_PROPERTY->hidden = true;
		CCD_BIN_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		//- ccd.on_attach
		CCD_MODE_PROPERTY->hidden = false;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_GAMMA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_MODE_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_MODE_PROPERTY, ccd_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_FRAME_PROPERTY, "Acquisition in progress");
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		//+ ccd.CCD_EXPOSURE.on_change_request
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) { return INDIGO_OK; }
		//- ccd.CCD_EXPOSURE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		//+ ccd.CCD_STREAMING.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) { return INDIGO_OK; }
		//- ccd.CCD_STREAMING.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAMMA_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAMMA_PROPERTY, ccd_gamma_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	//+ ccd.on_detach
	if (PRIVATE_DATA->camera) {
		dc1394_camera_free(PRIVATE_DATA->camera);
		PRIVATE_DATA->camera = NULL;
	}
	//- ccd.on_detach
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

#define SDK_DISCOVERY_RETRIES (3)
typedef struct sdk_discovery_retry {
	libusb_device *dev;
	int remaining;
	bool active, queued;
	struct sdk_discovery_retry *next;
} sdk_discovery_retry;

static sdk_discovery_retry *sdk_discovery_retries;
static bool sdk_discovery_stopping;
static void process_plug_event_handler(indigo_device *device, void *data);
static void process_sdk_retry_handler(indigo_device *device, void *data);

static void update_sdk_discovery_retry(libusb_device *dev, bool retry) {
	sdk_discovery_retry *entry = sdk_discovery_retries;
	while (entry && entry->dev != dev) {
		entry = entry->next;
	}
	if (!retry || sdk_discovery_stopping) {
		if (entry) {
			entry->active = false;
		}
		return;
	}
	if (!entry && SDK_DISCOVERY_RETRIES <= 0) {
		return;
	}
	if (!entry) {
		entry = (sdk_discovery_retry *)indigo_safe_malloc(sizeof(*entry));
		entry->dev = libusb_ref_device(dev);
		entry->remaining = SDK_DISCOVERY_RETRIES;
		entry->active = true;
		entry->next = sdk_discovery_retries;
		sdk_discovery_retries = entry;
	}
	if (entry->active && !entry->queued && entry->remaining > 0) {
		entry->remaining--;
		entry->queued = true;
		indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0.5, process_sdk_retry_handler, entry, &driver_queue_mutex);
	}
}

static void process_sdk_retry_handler(indigo_device *device, void *data) {
	sdk_discovery_retry *entry = (sdk_discovery_retry *)data;
	entry->queued = false;
	if (entry->active && !sdk_discovery_stopping) {
		process_plug_event_handler(NULL, libusb_ref_device(entry->dev));
	}
	if (!entry->queued) {
		sdk_discovery_retry **link = &sdk_discovery_retries;
		while (*link != entry) {
			link = &(*link)->next;
		}
		*link = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}

static void clear_sdk_discovery_retries(void) {
	while (sdk_discovery_retries) {
		sdk_discovery_retry *entry = sdk_discovery_retries;
		sdk_discovery_retries = entry->next;
		libusb_unref_device(entry->dev);
		indigo_safe_free(entry);
	}
}
static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	if (sdk_discovery_stopping) {
		libusb_unref_device(dev);
		return;
	}
	bool dev_ref_transferred = false;
	iidc_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (iidc_private_data *)indigo_safe_malloc(sizeof(iidc_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) != LIBUSB_SUCCESS) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		if (!iidc_context) {
			iidc_context = dc1394_new();
		}
		dc1394camera_list_t *list = NULL;
		if (iidc_context && dc1394_camera_enumerate(iidc_context, &list) == DC1394_SUCCESS && list) {
			for (uint32_t index = 0; index < list->num; index++) {
				uint64_t guid = list->ids[index].guid;
				uint16_t unit = list->ids[index].unit;
				bool attached = false;
				for (int slot = 0; slot < MAX_DEVICES; slot++) {
					if (devices[slot] && ((iidc_private_data *)devices[slot]->private_data)->guid == guid && ((iidc_private_data *)devices[slot]->private_data)->unit == unit) {
						attached = true;
						break;
					}
				}
				if (attached) {
					continue;
				}
				private_data->camera = dc1394_camera_new_unit(iidc_context, guid, unit);
				if (!private_data->camera) {
					continue;
				}
				private_data->guid = guid;
				private_data->unit = unit;
				if (strstr(private_data->camera->model, "CMLN") || strstr(private_data->camera->model, "FMVU")) {
					private_data->camera->bmode_capable = false;
				}
				dc1394error_t mode_result = dc1394_video_set_operation_mode(private_data->camera, private_data->camera->bmode_capable ? DC1394_OPERATION_MODE_1394B : DC1394_OPERATION_MODE_LEGACY);
				dc1394error_t speed_result = dc1394_video_set_iso_speed(private_data->camera, private_data->camera->bmode_capable ? DC1394_ISO_SPEED_800 : DC1394_ISO_SPEED_400);
				if (mode_result != DC1394_SUCCESS || speed_result != DC1394_SUCCESS) {
					dc1394_camera_free(private_data->camera);
					private_data->camera = NULL;
					continue;
				}
				snprintf(name, INDIGO_NAME_SIZE, "%s", private_data->camera->model && private_data->camera->model[0] ? private_data->camera->model : DRIVER_LABEL);
				indigo_make_name_unique(name, "%016llx-%u", (unsigned long long)guid, unit);
				plug_result = true;
				break;
			}
			dc1394_camera_free_list(list);
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		ccd->private_data = private_data;
		snprintf(ccd->name, INDIGO_NAME_SIZE, "%s", name);
		bool ccd_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = ccd;
				if (indigo_attach_device(ccd) == INDIGO_OK) {
					dev_ref_transferred = true;
					ccd_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!ccd_attached) {
			indigo_safe_free(ccd);
		}
	}
	update_sdk_discovery_retry(dev, discovery_eligible && !dev_ref_transferred);
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	update_sdk_discovery_retry(dev, false);
	iidc_private_data *private_data = NULL;
	iidc_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				dc1394camera_list_t *list = NULL;
				dc1394error_t result = iidc_context ? dc1394_camera_enumerate(iidc_context, &list) : DC1394_FAILURE;
				unplug_result = false;
				if (result == DC1394_SUCCESS && list) {
					unplug_result = true;
					for (uint32_t index = 0; index < list->num; index++) {
						if (list->ids[index].guid == private_data->guid && list->ids[index].unit == private_data->unit) {
							unplug_result = false;
							break;
						}
					}
					dc1394_camera_free_list(list);
				}
				//- sdk.unplug_match
			}
			if (unplug_result) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
				bool recorded = false;
				for (int k = 0; k < removed_count; k++) {
					if (removed[k] == private_data) {
						recorded = true;
						break;
					}
				}
				if (!recorded) {
					removed[removed_count++] = private_data;
				}
			}
		}
	}
	for (int k = 0; k < removed_count; k++) {
		libusb_unref_device(removed[k]->usbdev);
		indigo_safe_free(removed[k]);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_ccd_iidc(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			dc1394_log_register_handler(DC1394_LOG_ERROR, iidc_log_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_WARNING, iidc_log_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_DEBUG, iidc_log_handler, NULL);
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			sdk_discovery_stopping = false;
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc < 0) {
				indigo_queue_delete(&driver_queue);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			if (shutdown_result == INDIGO_OK) {
				sdk_discovery_stopping = true;
			}
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			indigo_queue_remove(driver_queue, NULL, (indigo_timer_callback)process_sdk_retry_handler);
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			clear_sdk_discovery_retries();
		//+ on_shutdown
		if (iidc_context) {
			dc1394_free(iidc_context);
		}
		iidc_context = NULL;
		//- on_shutdown
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
#else
#include "indigo_ccd_iidc.h"

indigo_result indigo_ccd_iidc(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "IIDC Compatible Camera", __FUNCTION__, 0x03000012, true, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
