// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_ccd_uvc.driver

// supported_architecture: !defined(INDIGO_WINDOWS)
#if !defined(INDIGO_WINDOWS)

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <limits.h>
#include <math.h>
#include "indigo_ccd_uvc_libuvc.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_uvc.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001A
#define DRIVER_NAME          "indigo_ccd_uvc"
#define DRIVER_LABEL         "UVC Camera"
#define CCD_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((uvc_private_data *)device->private_data)

//+ define

#define UVC_CTRL_FLAG_SET_CUR (1 << 0)
#define UVC_CTRL_FLAG_GET_CUR (1 << 1)
#define UVC_READOUT_TIMEOUT  15
#define UVC_POLL_INTERVAL    0.001
typedef struct {
	enum uvc_frame_format format;
	int width, height, bits_per_pixel;
} uvc_mode_data;

//- define

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	//+ data
	uvc_device_t *dev;
	uvc_device_handle_t *handle;
	uvc_stream_ctrl_t ctrl;
	uvc_stream_handle_t *stream;
	uvc_mode_data *modes;
	int mode_count, selected_mode;
	char *buffer;
	size_t buffer_size;
	bool acquisition_active, streaming;
	double frame_deadline;
	//- data
} uvc_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

typedef struct {
	enum uvc_frame_format format;
	const char *fourcc;
	const char *label_format;
} uvc_format_info;

static const uvc_format_info UVC_FORMATS[] = {
	{ UVC_FRAME_FORMAT_YUYV, "YUY2", "YUV %dx%d" },
	{ UVC_FRAME_FORMAT_YUYV, "YUVY", "YUV %dx%d " },
	{ UVC_FRAME_FORMAT_RGB, "YUVY", "RGB %dx%d " },
	{ UVC_FRAME_FORMAT_GRAY8, "Y800", "MONO8  %dx%d" },
	{ UVC_FRAME_FORMAT_GRAY16, "Y16 ", "MONO16  %dx%d" },
	{ UVC_FRAME_FORMAT_BY8, "BY8 ", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_BA81, "BY81", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SGRBG8, "GRBG", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SGBRG8, "GBRG", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SRGGB8, "RGGB", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SBGGR8, "BGGR", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_ANY, "    ", "%dx%d" }
};

static uvc_context_t *uvc_context;

static bool uvc_native_8_bit(enum uvc_frame_format format) {
	return format == UVC_FRAME_FORMAT_GRAY8 || format == UVC_FRAME_FORMAT_BY8 || format == UVC_FRAME_FORMAT_BA81 || format == UVC_FRAME_FORMAT_SGRBG8 || format == UVC_FRAME_FORMAT_SGBRG8 || format == UVC_FRAME_FORMAT_SRGGB8 || format == UVC_FRAME_FORMAT_SBGGR8;
}

static bool uvc_converted_rgb(enum uvc_frame_format format) {
	return format == UVC_FRAME_FORMAT_RGB || format == UVC_FRAME_FORMAT_YUYV || format == UVC_FRAME_FORMAT_UYVY;
}

static bool uvc_add_mode(indigo_device *device, int format_index, int width, int height) {
	if (PRIVATE_DATA->mode_count == INT_MAX / (int)sizeof(uvc_mode_data)) {
		return false;
	}
	int count = PRIVATE_DATA->mode_count;
	uvc_mode_data *modes = indigo_safe_realloc(PRIVATE_DATA->modes, (size_t)(count + 1) * sizeof(uvc_mode_data));
	if (!modes) {
		return false;
	}
	PRIVATE_DATA->modes = modes;
	PRIVATE_DATA->modes[count] = (uvc_mode_data) { UVC_FORMATS[format_index].format, width, height, UVC_FORMATS[format_index].format == UVC_FRAME_FORMAT_GRAY16 ? 16 : 8 };
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, count + 1);
	char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
	snprintf(name, sizeof(name), "%d_%dx%d", format_index, width, height);
	snprintf(label, sizeof(label), UVC_FORMATS[format_index].label_format, width, height);
	indigo_init_switch_item(CCD_MODE_PROPERTY->items + count, name, label, count == 0);
	PRIVATE_DATA->mode_count++;
	return true;
}

static bool uvc_select_mode(indigo_device *device, int selected) {
	if (selected < 0 || selected >= PRIVATE_DATA->mode_count) {
		return false;
	}
	uvc_mode_data *mode = PRIVATE_DATA->modes + selected;
	uvc_error_t result = uvc_get_stream_ctrl_format_size(PRIVATE_DATA->handle, &PRIVATE_DATA->ctrl, mode->format, mode->width, mode->height, 0);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_stream_ctrl_format_size(..., %d, %d, %d, 0) -> %s", mode->format, mode->width, mode->height, uvc_strerror(result));
	if (result != UVC_SUCCESS) {
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "stream ctrl: format=%u frame=%u interval=%u frame_size=%u payload=%u", PRIVATE_DATA->ctrl.bFormatIndex, PRIVATE_DATA->ctrl.bFrameIndex, PRIVATE_DATA->ctrl.dwFrameInterval, PRIVATE_DATA->ctrl.dwMaxVideoFrameSize, PRIVATE_DATA->ctrl.dwMaxPayloadTransferSize);
	PRIVATE_DATA->selected_mode = selected;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = mode->width;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = mode->height;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = mode->bits_per_pixel;
	return true;
}

static bool uvc_discover_modes(indigo_device *device) {
	PRIVATE_DATA->mode_count = 0;
	PRIVATE_DATA->selected_mode = -1;
	indigo_safe_free(PRIVATE_DATA->modes);
	PRIVATE_DATA->modes = NULL;
	CCD_MODE_PROPERTY->count = 0;
	CCD_INFO_WIDTH_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value = 0;
	const uvc_format_desc_t *format = uvc_get_format_descs(PRIVATE_DATA->handle);
	while (format) {
		int format_index = 0;
		while (UVC_FORMATS[format_index].format != UVC_FRAME_FORMAT_ANY && strncmp((char *)format->fourccFormat, UVC_FORMATS[format_index].fourcc, 4)) {
			format_index++;
		}
		if (format->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED || format->bDescriptorSubtype == UVC_VS_FORMAT_FRAME_BASED) {
			for (uvc_frame_desc_t *frame = format->frame_descs; frame; frame = frame->next) {
				if (!uvc_add_mode(device, format_index, frame->wWidth, frame->wHeight)) {
					return false;
				}
				if (CCD_INFO_WIDTH_ITEM->number.value < frame->wWidth) {
					CCD_INFO_WIDTH_ITEM->number.value = frame->wWidth;
				}
				if (CCD_INFO_HEIGHT_ITEM->number.value < frame->wHeight) {
					CCD_INFO_HEIGHT_ITEM->number.value = frame->wHeight;
				}
			}
		}
		format = format->next;
	}
	if (PRIVATE_DATA->mode_count == 0 || !uvc_select_mode(device, 0)) {
		return false;
	}
	uvc_error_t result = uvc_set_ae_mode(PRIVATE_DATA->handle, 1);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_ae_mode(1) -> %s", uvc_strerror(result));
	return result == UVC_SUCCESS;
}

static void uvc_setup_control(indigo_device *device, indigo_property *property, indigo_item *item, uvc_error_t (*getter)(uvc_device_handle_t *, uint16_t *, enum uvc_req_code)) {
	uint16_t value = 0;
	uvc_error_t result = getter(PRIVATE_DATA->handle, &value, UVC_GET_INFO);
	if (result != UVC_SUCCESS || !(value & UVC_CTRL_FLAG_GET_CUR)) {
		property->hidden = true;
		return;
	}
	property->hidden = false;
	property->perm = value & UVC_CTRL_FLAG_SET_CUR ? INDIGO_RW_PERM : INDIGO_RO_PERM;
	if (getter(PRIVATE_DATA->handle, &value, UVC_GET_CUR) == UVC_SUCCESS) {
		item->number.value = item->number.target = value;
	}
	if (getter(PRIVATE_DATA->handle, &value, UVC_GET_MIN) == UVC_SUCCESS) {
		item->number.min = value;
	}
	if (getter(PRIVATE_DATA->handle, &value, UVC_GET_MAX) == UVC_SUCCESS) {
		item->number.max = value;
	}
}

static void uvc_stop(indigo_device *device) {
	if (PRIVATE_DATA->stream) {
		uvc_stream_close(PRIVATE_DATA->stream);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_close()");
		PRIVATE_DATA->stream = NULL;
	}
	PRIVATE_DATA->acquisition_active = PRIVATE_DATA->streaming = false;
}

static void uvc_close(indigo_device *device);

static bool uvc_open(indigo_device *device) {
	uvc_error_t result = uvc_sdk_open(PRIVATE_DATA->dev, &PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_open() -> %s", uvc_strerror(result));
	if (result != UVC_SUCCESS) {
		PRIVATE_DATA->handle = NULL;
		return false;
	}
	uvc_print_diag(PRIVATE_DATA->handle, NULL);
	if (!uvc_discover_modes(device)) {
		uvc_close(device);
		return false;
	}
	uint32_t exposure = 0;
	if (uvc_get_exposure_abs(PRIVATE_DATA->handle, &exposure, UVC_GET_MIN) == UVC_SUCCESS) {
		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = exposure / 10000.0;
	}
	if (uvc_get_exposure_abs(PRIVATE_DATA->handle, &exposure, UVC_GET_MAX) == UVC_SUCCESS) {
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = exposure / 10000.0;
	}
	uvc_setup_control(device, CCD_GAIN_PROPERTY, CCD_GAIN_ITEM, uvc_get_gain);
	uvc_setup_control(device, CCD_GAMMA_PROPERTY, CCD_GAMMA_ITEM, uvc_get_gamma);
	if (CCD_INFO_WIDTH_ITEM->number.value <= 0 || CCD_INFO_HEIGHT_ITEM->number.value <= 0 || (size_t)CCD_INFO_WIDTH_ITEM->number.value > (SIZE_MAX - FITS_HEADER_SIZE) / (size_t)CCD_INFO_HEIGHT_ITEM->number.value / 6) {
		uvc_close(device);
		return false;
	}
	PRIVATE_DATA->buffer_size = FITS_HEADER_SIZE + (size_t)CCD_INFO_WIDTH_ITEM->number.value * (size_t)CCD_INFO_HEIGHT_ITEM->number.value * 6;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (!PRIVATE_DATA->buffer) {
		uvc_close(device);
		return false;
	}
	return true;
}

static void uvc_close(indigo_device *device) {
	uvc_stop(device);
	if (PRIVATE_DATA->handle) {
		uvc_sdk_close(PRIVATE_DATA->handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_close()");
		PRIVATE_DATA->handle = NULL;
	}
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->buffer_size = 0;
	indigo_safe_free(PRIVATE_DATA->modes);
	PRIVATE_DATA->modes = NULL;
	PRIVATE_DATA->mode_count = 0;
}

static bool uvc_start_acquisition(indigo_device *device, bool streaming) {
	uvc_error_t result = uvc_set_ae_mode(PRIVATE_DATA->handle, 1);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_ae_mode(1) -> %s", uvc_strerror(result));
	double exposure = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.value : CCD_EXPOSURE_ITEM->number.value;
	if (result == UVC_SUCCESS) {
		result = uvc_set_exposure_abs(PRIVATE_DATA->handle, (uint32_t)(10000 * exposure));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_exposure_abs(%u) -> %s", (uint32_t)(10000 * exposure), uvc_strerror(result));
	}
	if (result == UVC_SUCCESS && !CCD_GAIN_PROPERTY->hidden) {
		result = uvc_set_gain(PRIVATE_DATA->handle, (uint16_t)CCD_GAIN_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gain(%u) -> %s", (uint16_t)CCD_GAIN_ITEM->number.value, uvc_strerror(result));
	}
	if (result == UVC_SUCCESS && !CCD_GAMMA_PROPERTY->hidden) {
		result = uvc_set_gamma(PRIVATE_DATA->handle, (uint16_t)CCD_GAMMA_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gamma(%u) -> %s", (uint16_t)CCD_GAMMA_ITEM->number.value, uvc_strerror(result));
	}
	if (result == UVC_SUCCESS) {
		result = uvc_stream_open_ctrl(PRIVATE_DATA->handle, &PRIVATE_DATA->stream, &PRIVATE_DATA->ctrl);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_open_ctrl() -> %s", uvc_strerror(result));
	}
	if (result == UVC_SUCCESS) {
		result = uvc_stream_start(PRIVATE_DATA->stream, NULL, device, 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_start() -> %s", uvc_strerror(result));
	}
	if (result != UVC_SUCCESS) {
		uvc_stop(device);
		return false;
	}
	PRIVATE_DATA->acquisition_active = true;
	PRIVATE_DATA->streaming = streaming;
	PRIVATE_DATA->frame_deadline = indigo_monotonic_time() + exposure + UVC_READOUT_TIMEOUT;
	return true;
}

static bool uvc_process_frame(indigo_device *device, uvc_frame_t *frame, bool streaming) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "frame=%p data=%p format=%d width=%u height=%u bytes=%zu", frame, frame ? frame->data : NULL, frame ? frame->frame_format : UVC_FRAME_FORMAT_UNKNOWN, frame ? frame->width : 0, frame ? frame->height : 0, frame ? frame->data_bytes : 0);
	if (!frame || !frame->data || frame->width == 0 || frame->height == 0 || frame->width > SIZE_MAX / frame->height) {
		return false;
	}
	size_t pixels = (size_t)frame->width * frame->height;
	if (uvc_native_8_bit(frame->frame_format)) {
		if (frame->data_bytes < pixels || pixels > PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
			return false;
		}
		memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, pixels);
		indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 8, true, true, NULL, streaming);
		return true;
	}
	if (frame->frame_format == UVC_FRAME_FORMAT_GRAY16) {
		if (pixels > SIZE_MAX / 2 || frame->data_bytes < 2 * pixels || 2 * pixels > PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
			return false;
		}
		memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, 2 * pixels);
		indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 16, true, true, NULL, streaming);
		return true;
	}
	if (uvc_converted_rgb(frame->frame_format) && pixels <= SIZE_MAX / 3 && 3 * pixels <= PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
		size_t source_size = frame->frame_format == UVC_FRAME_FORMAT_RGB ? 3 * pixels : 2 * pixels;
		if (frame->data_bytes < source_size) {
			return false;
		}
		uvc_frame_t *rgb = uvc_allocate_frame(3 * pixels);
		if (!rgb) {
			return false;
		}
		uvc_error_t result = uvc_any2rgb(frame, rgb);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_any2rgb(...) -> %s", uvc_strerror(result));
		if (result == UVC_SUCCESS && rgb->data && rgb->data_bytes >= 3 * pixels) {
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, rgb->data, 3 * pixels);
			indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 24, true, true, NULL, streaming);
		}
		uvc_free_frame(rgb);
		return result == UVC_SUCCESS;
	}
	return false;
}

static bool uvc_frame_is_complete(indigo_device *device, uvc_frame_t *frame) {
	if (!frame || !frame->data || frame->width == 0 || frame->height == 0 || frame->width > SIZE_MAX / frame->height) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "discarding invalid frame: frame=%p data=%p width=%u height=%u", frame, frame ? frame->data : NULL, frame ? frame->width : 0, frame ? frame->height : 0);
		return false;
	}
	uvc_mode_data *mode = PRIVATE_DATA->modes + PRIVATE_DATA->selected_mode;
	if (frame->width != mode->width || frame->height != mode->height) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "discarding stale frame: received=%ux%u selected=%dx%d bytes=%zu", frame->width, frame->height, mode->width, mode->height, frame->data_bytes);
		return false;
	}
	size_t pixels = (size_t)frame->width * frame->height;
	size_t required = 0;
	if (uvc_native_8_bit(frame->frame_format)) {
		required = pixels;
	}
	if (frame->frame_format == UVC_FRAME_FORMAT_GRAY16 || frame->frame_format == UVC_FRAME_FORMAT_YUYV || frame->frame_format == UVC_FRAME_FORMAT_UYVY) {
		required = pixels <= SIZE_MAX / 2 ? 2 * pixels : SIZE_MAX;
	}
	if (frame->frame_format == UVC_FRAME_FORMAT_RGB) {
		required = pixels <= SIZE_MAX / 3 ? 3 * pixels : SIZE_MAX;
	}
	if (required && frame->data_bytes < required) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "discarding incomplete frame: format=%d size=%ux%u bytes=%zu required=%zu", frame->frame_format, frame->width, frame->height, frame->data_bytes, required);
		return false;
	}
	return true;
}

static void uvc_acquisition_failure(indigo_device *device, indigo_property *property, const char *message) {
	bool streaming = PRIVATE_DATA->streaming;
	uvc_stop(device);
	if (streaming) {
		indigo_finalize_video_stream(device);
	}
	indigo_ccd_failure_cleanup(device);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquisition_active || CCD_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	uvc_frame_t *frame = NULL;
	uvc_error_t result = uvc_stream_get_frame(PRIVATE_DATA->stream, &frame, 1000);
	if (result != UVC_ERROR_TIMEOUT) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_get_frame(...) -> %s", uvc_strerror(result));
	}
	bool incomplete = result == UVC_SUCCESS && frame && !uvc_frame_is_complete(device, frame);
	if ((result == UVC_ERROR_TIMEOUT || (result == UVC_SUCCESS && !frame) || incomplete) && indigo_monotonic_time() < PRIVATE_DATA->frame_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, UVC_POLL_INTERVAL, exposure_finalizer);
		return;
	}
	if (result != UVC_SUCCESS || incomplete || !uvc_process_frame(device, frame, false)) {
		uvc_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, result == UVC_ERROR_TIMEOUT || !frame ? "Exposure timed out" : "Exposure readout failed");
		return;
	}
	uvc_stop(device);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void streaming_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquisition_active || CCD_STREAMING_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	uvc_frame_t *frame = NULL;
	uvc_error_t result = uvc_stream_get_frame(PRIVATE_DATA->stream, &frame, 1000);
	if (result != UVC_ERROR_TIMEOUT) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_get_frame(...) -> %s", uvc_strerror(result));
	}
	bool incomplete = result == UVC_SUCCESS && frame && !uvc_frame_is_complete(device, frame);
	if ((result == UVC_ERROR_TIMEOUT || (result == UVC_SUCCESS && !frame) || incomplete) && indigo_monotonic_time() < PRIVATE_DATA->frame_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, UVC_POLL_INTERVAL, streaming_finalizer);
		return;
	}
	if (result != UVC_SUCCESS || incomplete || !uvc_process_frame(device, frame, true)) {
		uvc_acquisition_failure(device, CCD_STREAMING_PROPERTY, result == UVC_ERROR_TIMEOUT || !frame ? "Streaming timed out" : "Streaming readout failed");
		return;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		uvc_stop(device);
		indigo_finalize_video_stream(device);
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->frame_deadline = indigo_monotonic_time() + CCD_STREAMING_EXPOSURE_ITEM->number.value + UVC_READOUT_TIMEOUT;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
}

//- code

#pragma mark - High level code (ccd)

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = uvc_open(device);
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
		indigo_cancel_pending_handler(device, exposure_finalizer);
		indigo_cancel_pending_handler(device, streaming_finalizer);
		uvc_stop(device);
		//- ccd.on_disconnect
		uvc_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
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
	if (!uvc_select_mode(device, selected)) {
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
	}
	//- ccd.CCD_MODE.on_change
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	if (uvc_start_acquisition(device, false)) {
		indigo_ccd_exposure_setup(device);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, exposure_finalizer);
	} else {
		uvc_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, "Exposure setup failed");
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
	if (uvc_start_acquisition(device, true)) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
	} else {
		uvc_acquisition_failure(device, CCD_STREAMING_PROPERTY, "Streaming setup failed");
		indigo_finalize_video_stream(device);
	}
	//- ccd.CCD_STREAMING.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, exposure_finalizer);
	indigo_cancel_pending_handler(device, streaming_finalizer);
	uvc_stop(device);
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_finalize_video_stream(device);
	}
	indigo_ccd_abort_exposure_cleanup(device);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		CCD_BIN_PROPERTY->hidden = true;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 0;
		CCD_EXPOSURE_ITEM->number.min = 0.001;
		CCD_STREAMING_EXPOSURE_ITEM->number.min = 0.001;
		CCD_INFO_PROPERTY->count = 2;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
		//- ccd.on_attach
		CCD_MODE_PROPERTY->hidden = false;
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
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAMMA_PROPERTY, property)) {
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);
		CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
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
	if (PRIVATE_DATA->dev) {
		uvc_unref_device(PRIVATE_DATA->dev);
		PRIVATE_DATA->dev = NULL;
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
	uvc_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (uvc_private_data *)indigo_safe_malloc(sizeof(uvc_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) != LIBUSB_SUCCESS) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		uvc_device_t **list = NULL;
		uvc_error_t result = uvc_context ? UVC_SUCCESS : uvc_init(&uvc_context, NULL);
		if (result == UVC_SUCCESS) {
			result = uvc_get_device_list(uvc_context, &list);
		}
		if (result == UVC_SUCCESS && list) {
			uvc_device_t *selected = NULL;
			for (int i = 0; list[i]; i++) {
				uvc_device_t *candidate = list[i];
				bool matches = uvc_get_bus_number(candidate) == libusb_get_bus_number(dev) && uvc_get_device_address(candidate) == libusb_get_device_address(dev);
				bool attached = false;
				if (matches) {
					for (int slot = 0; slot < MAX_DEVICES; slot++) {
						if (devices[slot] && uvc_get_bus_number(((uvc_private_data *)devices[slot]->private_data)->dev) == uvc_get_bus_number(candidate) && uvc_get_device_address(((uvc_private_data *)devices[slot]->private_data)->dev) == uvc_get_device_address(candidate)) {
							attached = true;
							break;
						}
					}
				}
				if (matches && !attached && !selected) {
					uvc_device_descriptor_t *uvc_descriptor = NULL;
					result = uvc_get_device_descriptor(candidate, &uvc_descriptor);
					if (result == UVC_SUCCESS && uvc_descriptor) {
						selected = candidate;
						private_data->dev = candidate;
						snprintf(name, INDIGO_NAME_SIZE, "%s%s%s", uvc_descriptor->manufacturer ? uvc_descriptor->manufacturer : "", uvc_descriptor->manufacturer && uvc_descriptor->product ? " " : "", uvc_descriptor->product ? uvc_descriptor->product : DRIVER_LABEL);
						char usb_path[INDIGO_NAME_SIZE];
						indigo_get_usb_path(dev, usb_path);
						indigo_make_name_unique(name, "%s", usb_path);
					}
					if (uvc_descriptor) {
						uvc_free_device_descriptor(uvc_descriptor);
					}
				}
				if (candidate != selected) {
					uvc_unref_device(candidate);
				}
			}
			uvc_free_device_list(list, 0);
			plug_result = selected != NULL;
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
	uvc_private_data *private_data = NULL;
	uvc_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				unplug_result = private_data->dev && uvc_get_bus_number(private_data->dev) == libusb_get_bus_number(dev) && uvc_get_device_address(private_data->dev) == libusb_get_device_address(dev);
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

indigo_result indigo_ccd_uvc(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			uvc_context_t *probe_context = NULL;
			uvc_error_t result = uvc_init(&probe_context, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libuvc %s, uvc_init() -> %s", LIBUVC_VERSION_STR, uvc_strerror(result));
			if (result != UVC_SUCCESS) {
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			uvc_exit(probe_context);
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
		if (uvc_context) {
			uvc_exit(uvc_context);
			uvc_context = NULL;
		}
		//- on_shutdown
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
#else
#include "indigo_ccd_uvc.h"

indigo_result indigo_ccd_uvc(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "UVC Camera", __FUNCTION__, 0x0300001A, true, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
