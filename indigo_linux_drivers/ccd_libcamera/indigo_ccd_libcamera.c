// Copyright (c) 2026 CloudMakers, s. r. o.
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

// This file generated from indigo_ccd_libcamera.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <ctype.h>
#include <math.h>
#include "indigo_ccd_libcamera_bridge.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_libcamera.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000001
#define DRIVER_NAME          "indigo_ccd_libcamera"
#define DRIVER_LABEL         "libcamera Camera"
#define CCD_DEVICE_NAME      "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((libcamera_private_data *)device->private_data)

//+ define

#ifndef LIBCAMERA_READOUT_TIMEOUT
#define LIBCAMERA_READOUT_TIMEOUT 10
#endif
#define LIBCAMERA_POLL_INTERVAL 0.01

//- define

#pragma mark - Private data definition

typedef struct {
	void *sdkdev;
	//+ data
	char id[LCAM_ID_SIZE];
	char model[LCAM_MODEL_SIZE];
	lcam_camera *camera;
	lcam_properties properties;
	int selected_mode;
	bool capturing;
	double capture_deadline;
	unsigned char *buffer;
	size_t buffer_size;
	char error[LCAM_ERROR_SIZE];
	//- data
} libcamera_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static lcam_camera_info libcamera_cameras[LCAM_MAX_CAMERAS];

static void libcamera_stop(indigo_device *device) {
	if (PRIVATE_DATA->capturing) {
		lcam_stop(PRIVATE_DATA->camera);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_stop()");
	}
	PRIVATE_DATA->capturing = false;
}

static bool libcamera_select_mode(indigo_device *device, int index) {
	if (index < 0 || index >= PRIVATE_DATA->properties.mode_count) {
		return false;
	}
	lcam_mode *mode = PRIVATE_DATA->properties.modes + index;
	PRIVATE_DATA->selected_mode = index;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = mode->width;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = mode->height;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
	// Exposure limits depend on the sensor mode (an OV5647 exposes up to 3 s at full resolution and 4.6 s at 640x480).
	if (mode->exposure_max > mode->exposure_min && mode->exposure_min > 0) {
		CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = mode->exposure_min;
		CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = mode->exposure_max;
		CCD_EXPOSURE_ITEM->number.target = fmin(fmax(CCD_EXPOSURE_ITEM->number.target, mode->exposure_min), mode->exposure_max);
		CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target = fmin(fmax(CCD_STREAMING_EXPOSURE_ITEM->number.value, mode->exposure_min), mode->exposure_max);
	}
	return true;
}

static void libcamera_close(indigo_device *device);

static bool libcamera_open(indigo_device *device) {
	PRIVATE_DATA->camera = lcam_open(PRIVATE_DATA->id, &PRIVATE_DATA->properties, PRIVATE_DATA->error, sizeof(PRIVATE_DATA->error));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_open('%s') -> %s", PRIVATE_DATA->id, PRIVATE_DATA->camera ? "OK" : PRIVATE_DATA->error);
	if (PRIVATE_DATA->camera == NULL) {
		indigo_send_message(device, NULL, "%s", PRIVATE_DATA->error);
		return false;
	}
	lcam_properties *properties = &PRIVATE_DATA->properties;
	unsigned max_width = 0, max_height = 0;
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, properties->mode_count);
	for (int i = 0; i < properties->mode_count; i++) {
		lcam_mode *mode = properties->modes + i;
		char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
		snprintf(name, sizeof(name), "MODE_%d", i);
		if (mode->binning > 1) {
			snprintf(label, sizeof(label), "RAW %u %ux%u (bin %ux%u)", mode->bit_depth, mode->width, mode->height, mode->binning, mode->binning);
		} else {
			snprintf(label, sizeof(label), "RAW %u %ux%u", mode->bit_depth, mode->width, mode->height);
		}
		indigo_init_switch_item(CCD_MODE_PROPERTY->items + i, name, label, i == 0);
		max_width = max_width > mode->width ? max_width : mode->width;
		max_height = max_height > mode->height ? max_height : mode->height;
	}
	CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	libcamera_select_mode(device, 0);
	CCD_INFO_WIDTH_ITEM->number.value = max_width;
	CCD_INFO_HEIGHT_ITEM->number.value = max_height;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
	CCD_INFO_PIXEL_WIDTH_ITEM->number.value = properties->pixel_width;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = properties->pixel_height;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = properties->pixel_width > properties->pixel_height ? properties->pixel_width : properties->pixel_height;
	CCD_GAIN_PROPERTY->hidden = !(properties->gain_max > properties->gain_min);
	if (!CCD_GAIN_PROPERTY->hidden) {
		CCD_GAIN_ITEM->number.min = properties->gain_min;
		CCD_GAIN_ITEM->number.max = properties->gain_max;
		CCD_GAIN_ITEM->number.step = 0;
		if (CCD_GAIN_ITEM->number.value < properties->gain_min || CCD_GAIN_ITEM->number.value > properties->gain_max) {
			CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = properties->gain_default;
		}
	}
	if (max_width == 0 || max_height == 0 || (size_t)max_width > (SIZE_MAX - FITS_HEADER_SIZE) / max_height / 2) {
		libcamera_close(device);
		return false;
	}
	PRIVATE_DATA->buffer_size = FITS_HEADER_SIZE + (size_t)max_width * max_height * 2;
	PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (PRIVATE_DATA->buffer == NULL) {
		libcamera_close(device);
		return false;
	}
	return true;
}

static void libcamera_close(indigo_device *device) {
	libcamera_stop(device);
	if (PRIVATE_DATA->camera) {
		lcam_close(PRIVATE_DATA->camera);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_close()");
		PRIVATE_DATA->camera = NULL;
	}
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->buffer_size = 0;
}

static bool libcamera_start(indigo_device *device, bool streaming) {
	double exposure = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.value : CCD_EXPOSURE_ITEM->number.target;
	double gain = CCD_GAIN_PROPERTY->hidden ? PRIVATE_DATA->properties.gain_default : CCD_GAIN_ITEM->number.value;
	libcamera_stop(device);
	bool result = lcam_start(PRIVATE_DATA->camera, PRIVATE_DATA->selected_mode, exposure, gain, streaming, PRIVATE_DATA->error, sizeof(PRIVATE_DATA->error));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_start(%d, %g, %g, %s) -> %s", PRIVATE_DATA->selected_mode, exposure, gain, streaming ? "streaming" : "single", result ? "OK" : PRIVATE_DATA->error);
	if (result) {
		PRIVATE_DATA->capturing = true;
		PRIVATE_DATA->capture_deadline = indigo_monotonic_time() + exposure + LIBCAMERA_READOUT_TIMEOUT;
	}
	return result;
}

// Returns 1 when a frame was processed, 0 while it is pending and -1 on failure.
static int libcamera_process_frame(indigo_device *device, bool streaming) {
	unsigned left = (unsigned)CCD_FRAME_LEFT_ITEM->number.value;
	unsigned top = (unsigned)CCD_FRAME_TOP_ITEM->number.value;
	unsigned width = (unsigned)CCD_FRAME_WIDTH_ITEM->number.value;
	unsigned height = (unsigned)CCD_FRAME_HEIGHT_ITEM->number.value;
	lcam_frame frame = { 0 };
	int result = lcam_read(PRIVATE_DATA->camera, (uint16_t *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE, left, top, width, height, &frame, PRIVATE_DATA->error, sizeof(PRIVATE_DATA->error));
	if (result == 1) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_read() -> %ux%u, %u bits, exposure %gs, gain %g", frame.width, frame.height, frame.bit_depth, frame.exposure, frame.gain);
		if (frame.width != width || frame.height != height) {
			snprintf(PRIVATE_DATA->error, sizeof(PRIVATE_DATA->error), "Invalid image data");
			return -1;
		}
		if (PRIVATE_DATA->properties.bayer[0]) {
			indigo_fits_keyword keywords[] = { { INDIGO_FITS_STRING, "BAYERPAT", .string = PRIVATE_DATA->properties.bayer, "Bayer color pattern" }, { 0 } };
			indigo_process_image(device, PRIVATE_DATA->buffer, width, height, 16, true, true, keywords, streaming);
		} else {
			indigo_process_image(device, PRIVATE_DATA->buffer, width, height, 16, true, true, NULL, streaming);
		}
	} else if (result < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "lcam_read() -> %s", PRIVATE_DATA->error);
	}
	return result;
}

static void libcamera_acquisition_failure(indigo_device *device, indigo_property *property, const char *message) {
	libcamera_stop(device);
	indigo_ccd_failure_cleanup(device);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->capturing || CCD_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	int result = libcamera_process_frame(device, false);
	if (result == 0 && indigo_monotonic_time() < PRIVATE_DATA->capture_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, LIBCAMERA_POLL_INTERVAL, exposure_finalizer);
		return;
	}
	if (result != 1) {
		libcamera_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, result == 0 ? "Exposure timed out" : PRIVATE_DATA->error);
		return;
	}
	libcamera_stop(device);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void streaming_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->capturing || CCD_STREAMING_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	int result = libcamera_process_frame(device, true);
	if (result == 0 && indigo_monotonic_time() < PRIVATE_DATA->capture_deadline) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, LIBCAMERA_POLL_INTERVAL, streaming_finalizer);
		return;
	}
	if (result != 1) {
		libcamera_acquisition_failure(device, CCD_STREAMING_PROPERTY, result == 0 ? "Streaming timed out" : PRIVATE_DATA->error);
		indigo_finalize_video_stream(device);
		return;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		libcamera_stop(device);
		indigo_finalize_video_stream(device);
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		return;
	}
	PRIVATE_DATA->capture_deadline = indigo_monotonic_time() + CCD_STREAMING_EXPOSURE_ITEM->number.value + LIBCAMERA_READOUT_TIMEOUT;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
}

//- code

#pragma mark - High level code (ccd)

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = libcamera_open(device);
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
		libcamera_stop(device);
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_finalize_video_stream(device);
		}
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_ccd_failure_cleanup(device);
		}
		//- ccd.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			CCD_MODE_PROPERTY,
			CCD_FRAME_PROPERTY,
			CCD_EXPOSURE_PROPERTY,
			CCD_STREAMING_PROPERTY,
			CCD_ABORT_EXPOSURE_PROPERTY,
			CCD_GAIN_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		libcamera_close(device);
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
	if (!libcamera_select_mode(device, selected)) {
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		// The new mode has other frame and exposure limits, which clients receive only with a definition.
		indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_STREAMING_PROPERTY, NULL);
		indigo_define_property(device, CCD_STREAMING_PROPERTY, NULL);
	}
	//- ccd.CCD_MODE.on_change
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	// A subframe of a colour sensor starts and ends on a Bayer cell boundary, so BAYERPAT stays valid.
	lcam_mode *mode = PRIVATE_DATA->properties.modes + PRIVATE_DATA->selected_mode;
	unsigned unit = PRIVATE_DATA->properties.bayer[0] ? 2 : 1;
	unsigned left = (unsigned)CCD_FRAME_LEFT_ITEM->number.value / unit * unit;
	unsigned top = (unsigned)CCD_FRAME_TOP_ITEM->number.value / unit * unit;
	unsigned width = (unsigned)CCD_FRAME_WIDTH_ITEM->number.value / unit * unit;
	unsigned height = (unsigned)CCD_FRAME_HEIGHT_ITEM->number.value / unit * unit;
	if (left >= mode->width || top >= mode->height) {
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		width = width < unit ? unit : width;
		height = height < unit ? unit : height;
		width = width < mode->width - left ? width : (mode->width - left) / unit * unit;
		height = height < mode->height - top ? height : (mode->height - top) / unit * unit;
		CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = left;
		CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = top;
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = width;
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = height;
		CCD_FRAME_PROPERTY->state = width && height ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	if (libcamera_start(device, false)) {
		indigo_ccd_exposure_setup(device);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, exposure_finalizer);
	} else {
		libcamera_acquisition_failure(device, CCD_EXPOSURE_PROPERTY, PRIVATE_DATA->error);
	}
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
	if (libcamera_start(device, true)) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, 0, streaming_finalizer);
	} else {
		libcamera_acquisition_failure(device, CCD_STREAMING_PROPERTY, PRIVATE_DATA->error);
		indigo_finalize_video_stream(device);
	}
	//- ccd.CCD_STREAMING.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, exposure_finalizer);
	indigo_cancel_pending_handler(device, streaming_finalizer);
	libcamera_stop(device);
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
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model[0] ? PRIVATE_DATA->model : DRIVER_LABEL);
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->id);
		CCD_BIN_PROPERTY->hidden = true;
		CCD_BIN_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_TEMPERATURE_PROPERTY->hidden = true;
		CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		//- ccd.on_attach
		CCD_MODE_PROPERTY->hidden = false;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
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
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress");
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
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
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

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	void *dev = data;
	bool dev_ref_transferred = false;
	libcamera_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (libcamera_private_data *)indigo_safe_malloc(sizeof(libcamera_private_data));
	private_data->sdkdev = dev;
	if (plug_result) {
		//+ sdk.plug
		lcam_camera_info *info = (lcam_camera_info *)dev;
		snprintf(private_data->id, sizeof(private_data->id), "%s", info->id);
		snprintf(private_data->model, sizeof(private_data->model), "%s", info->model);
		if (info->model[0]) {
			int i = 0;
			for (; info->model[i] && i < INDIGO_NAME_SIZE - 1; i++) {
				name[i] = (char)toupper((unsigned char)info->model[i]);
			}
			name[i] = 0;
		}
		plug_result = indigo_make_name_unique(name, NULL);
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
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	void *dev = data;
	libcamera_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->sdkdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		indigo_safe_free(private_data);
	}
}

static void discover_devices_handler(indigo_device *device) {
	//+ sdk.discover
	bool available = lcam_initialize();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "lcam_initialize() -> %s", available ? "OK" : "not available");
	int count = available ? lcam_list(libcamera_cameras, MAX_DEVICES < LCAM_MAX_CAMERAS ? MAX_DEVICES : LCAM_MAX_CAMERAS) : 0;
	for (int i = 0; i < count; i++) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera '%s' (%s)", libcamera_cameras[i].id, libcamera_cameras[i].model);
		process_plug_event_handler(NULL, libcamera_cameras + i);
	}
	//- sdk.discover
}

#pragma mark - Main code

indigo_result indigo_ccd_libcamera(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_queue_add(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, discover_devices_handler, &driver_queue_mutex);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			pthread_mutex_lock(&driver_queue_mutex);
			indigo_result shutdown_result = verify_devices_disconnected();
			pthread_mutex_unlock(&driver_queue_mutex);
			if (shutdown_result != INDIGO_OK) {
				return shutdown_result;
			}
			last_action = action;
			indigo_queue_drain(driver_queue);
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, PRIVATE_DATA->sdkdev);
				}
			}
			indigo_queue_delete(&driver_queue);
		//+ on_shutdown
		lcam_terminate();
		//- on_shutdown
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
