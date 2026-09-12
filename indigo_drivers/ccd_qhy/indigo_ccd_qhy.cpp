// Copyright (c) 2017-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_ccd_qhy.driver

// supported_architecture: !defined(INDIGO_MACOS) || defined(__x86_64__)
#if !defined(INDIGO_MACOS) || defined(__x86_64__)

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include "qhyccd.h"
#include <indigo/indigo_client.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_qhy.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001E
#define DRIVER_NAME          "indigo_ccd_qhy"
#define DRIVER_LABEL         "QHY CCD (legacy) Camera"
#define CCD_DEVICE_NAME      "%s"
#define GUIDER_DEVICE_NAME   "%s (guider)"
#define WHEEL_DEVICE_NAME    "%s (wheel)"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((qhy_private_data *)device->private_data)

//+ define

#define QHY_BUFFER_LIMIT     (128U * 1024 * 1024)
#define CONFLICTING_DRIVER   "indigo_ccd_qhy2"

//- define

#pragma mark - Property definitions

#define X_PIXEL_FORMAT_PROPERTY        (PRIVATE_DATA->x_pixel_format_property)
#define RAW8_ITEM                      (X_PIXEL_FORMAT_PROPERTY->items + 0)
#define RAW16_ITEM                     (X_PIXEL_FORMAT_PROPERTY->items + 1)

#define X_PIXEL_FORMAT_PROPERTY_NAME   "X_PIXEL_FORMAT"
#define RAW8_ITEM_NAME                 "RAW 8"
#define RAW16_ITEM_NAME                "RAW 16"

#define X_ADVANCED_PROPERTY            (PRIVATE_DATA->x_advanced_property)

#define X_ADVANCED_PROPERTY_NAME       "X_ADVANCED"

#define X_READ_MODE_PROPERTY           (PRIVATE_DATA->x_read_mode_property)

#define X_READ_MODE_PROPERTY_NAME      "X_READ_MODE"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_pixel_format_property;
	indigo_property *x_advanced_property;
	indigo_property *x_read_mode_property;
	//+ data
	qhyccd_handle *handle;
	char sid[256], camera_name[INDIGO_NAME_SIZE], guider_name[INDIGO_NAME_SIZE], wheel_name[INDIGO_NAME_SIZE];
	bool has_guider, has_wheel, has_shutter, has_cooler, has_temperature;
	bool bins[4], acquiring, streaming, last_live;
	int last_bpp, sensor_bpp, selected_bpp, read_mode;
	uint32_t width, height, offset_x, offset_y, frame_width, frame_height;
	double pixel_width, pixel_height, duration, exposure_end, deadline, wheel_deadline;
	unsigned char *buffer;
	size_t buffer_size;
	CONTROL_ID advanced_controls[3];
	char wheel_reply[64];
	//- data
} qhy_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static bool sdk_initialized;
static void ccd_exposure_handler(indigo_device *device);
static void ccd_streaming_handler(indigo_device *device);
static void acquisition_finalizer(indigo_device *device);
static bool qhy_result(uint32_t result, const char *operation) {
	if (result != QHYCCD_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s = 0x%08x", operation, result);
		return false;
	}
	return true;
}

static bool qhy_geometry(indigo_device *device) {
	double cw = 0, ch = 0, pw = 0, ph = 0;
	uint32_t w = 0, h = 0, bpp = 0, x = 0, y = 0, ew = 0, eh = 0;
	if (!qhy_result(GetQHYCCDChipInfo(PRIVATE_DATA->handle, &cw, &ch, &w, &h, &pw, &ph, &bpp), "GetQHYCCDChipInfo") || w == 0 || h == 0 || w > 65535 || h > 65535 || !isfinite(pw) || !isfinite(ph) || pw <= 0 || ph <= 0 || bpp == 0 || bpp > 16) {
		return false;
	}
	if (!qhy_result(GetQHYCCDEffectiveArea(PRIVATE_DATA->handle, &x, &y, &ew, &eh), "GetQHYCCDEffectiveArea")) {
		return false;
	}
	// Some models return an empty effective area; their full sensor is usable.
	if (!ew || !eh) {
		x = y = 0;
		ew = w;
		eh = h;
	}
	if (x >= w || y >= h || ew > w - x || eh > h - y || (uint64_t)w * h * 2 > QHY_BUFFER_LIMIT) {
		return false;
	}
	PRIVATE_DATA->width = ew;
	PRIVATE_DATA->height = eh;
	PRIVATE_DATA->offset_x = x;
	PRIVATE_DATA->offset_y = y;
	PRIVATE_DATA->pixel_width = pw;
	PRIVATE_DATA->pixel_height = ph;
	PRIVATE_DATA->sensor_bpp = bpp;
	return true;
}

static void qhy_close(indigo_device *device) {
	if (PRIVATE_DATA->handle) {
		qhy_result(CloseQHYCCD(PRIVATE_DATA->handle), "CloseQHYCCD");
		PRIVATE_DATA->handle = NULL;
	}
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->buffer_size = 0;
	indigo_global_unlock(device);
}

static bool qhy_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		return false;
	}
	// Legacy QHY5L-II needs a rescan before every reopen (vendor SDK workaround).
	ScanQHYCCD();
	PRIVATE_DATA->handle = OpenQHYCCD(PRIVATE_DATA->sid);
	if (!PRIVATE_DATA->handle || !qhy_result(SetQHYCCDStreamMode(PRIVATE_DATA->handle, 0), "SetQHYCCDStreamMode") || !qhy_result(InitQHYCCD(PRIVATE_DATA->handle), "InitQHYCCD") || !qhy_geometry(device)) {
		qhy_close(device);
		return false;
	}
	PRIVATE_DATA->last_live = false;
	PRIVATE_DATA->last_bpp = 0;
	PRIVATE_DATA->acquiring = false;
	PRIVATE_DATA->buffer_size = QHY_BUFFER_LIMIT + FITS_HEADER_SIZE;
	PRIVATE_DATA->buffer = (unsigned char *)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
	if (!PRIVATE_DATA->buffer) {
		qhy_close(device);
		return false;
	}
	return true;
}

static bool qhy_available(indigo_device *device, CONTROL_ID control) {
	return IsQHYCCDControlAvailable(PRIVATE_DATA->handle, control) == QHYCCD_SUCCESS;
}

static bool qhy_control_info(indigo_device *device, CONTROL_ID control, indigo_item *item) {
	double min = 0, max = 0, step = 0;
	if (!qhy_result(GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, control, &min, &max, &step), "GetQHYCCDParamMinMaxStep") || !isfinite(min) || !isfinite(max) || !isfinite(step) || min > max || step < 0) {
		return false;
	}
	double value = GetQHYCCDParam(PRIVATE_DATA->handle, control);
	if (!isfinite(value) || value < min || value > max || value == (double)QHYCCD_ERROR) {
		return false;
	}
	item->number.min = min;
	item->number.max = max;
	item->number.step = step;
	item->number.value = item->number.target = value;
	return true;
}

static bool qhy_write_control(indigo_device *device, CONTROL_ID control, indigo_item *item) {
	if (!PRIVATE_DATA->handle || !qhy_result(SetQHYCCDParam(PRIVATE_DATA->handle, control, item->number.target), "SetQHYCCDParam")) {
		item->number.target = item->number.value;
		return false;
	}
	double value = GetQHYCCDParam(PRIVATE_DATA->handle, control);
	if (!isfinite(value) || value < item->number.min || value > item->number.max || value == (double)QHYCCD_ERROR) {
		item->number.target = item->number.value;
		return false;
	}
	item->number.value = item->number.target = value;
	return true;
}

static void qhy_update_geometry(indigo_device *device) {
	CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->width;
	CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->height;
	CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = PRIVATE_DATA->pixel_width;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_height;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->sensor_bpp;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->width;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->height;
}

static void qhy_modes(indigo_device *device) {
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, 8);
	int count = 0;
	for (int b = 1; b <= 4; b++) {
		if (!PRIVATE_DATA->bins[b - 1]) {
			continue;
		}
		for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
			int bpp = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, "RAW 8") ? 8 : 16;
			char name[32], label[64];
			snprintf(name, sizeof(name), "RAW %d %dx%d", bpp, b, b);
			snprintf(label, sizeof(label), "RAW %d %dx%d", bpp, PRIVATE_DATA->width / b, PRIVATE_DATA->height / b);
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + count++, name, label, b == CCD_BIN_HORIZONTAL_ITEM->number.value && bpp == CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value);
		}
	}
	CCD_MODE_PROPERTY->count = count;
}

static bool qhy_initialize_ccd(indigo_device *device) {
	qhy_update_geometry(device);
	X_PIXEL_FORMAT_PROPERTY = indigo_resize_property(X_PIXEL_FORMAT_PROPERTY, 2);
	int formats = 0;
	if (qhy_available(device, CAM_8BITS)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + formats++, "RAW 8", "RAW 8", false);
	}
	if (qhy_available(device, CAM_16BITS)) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + formats++, "RAW 16", "RAW 16", false);
	}
	if (!formats) {
		indigo_init_switch_item(X_PIXEL_FORMAT_PROPERTY->items + formats++, PRIVATE_DATA->sensor_bpp > 8 ? "RAW 16" : "RAW 8", "Native depth", true);
	}
	X_PIXEL_FORMAT_PROPERTY->count = formats;
	X_PIXEL_FORMAT_PROPERTY->hidden = formats < 2;
	int selected = formats - 1;
	for (int i = 0; i < formats; i++) {
		if (!strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, PRIVATE_DATA->sensor_bpp > 8 ? "RAW 16" : "RAW 8")) {
			selected = i;
		}
	}
	indigo_set_switch(X_PIXEL_FORMAT_PROPERTY, X_PIXEL_FORMAT_PROPERTY->items + selected, true);
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[selected].name, "RAW 8") ? 8 : 16;
	PRIVATE_DATA->selected_bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[0].name, "RAW 8") ? 8 : 16;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[formats - 1].name, "RAW 8") ? 8 : 16;
	int max_bin = 0;
	const CONTROL_ID controls[] = { CAM_BIN1X1MODE, CAM_BIN2X2MODE, CAM_BIN3X3MODE, CAM_BIN4X4MODE };
	for (int i = 0; i < 4; i++) {
		PRIVATE_DATA->bins[i] = qhy_available(device, controls[i]);
		if (PRIVATE_DATA->bins[i]) {
			max_bin = i + 1;
		}
	}
	if (!PRIVATE_DATA->bins[0]) {
		return false;
	}
	CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = 1;
	CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;
	qhy_modes(device);
	indigo_property *properties[] = { CCD_GAIN_PROPERTY, CCD_OFFSET_PROPERTY, CCD_GAMMA_PROPERTY };
	const CONTROL_ID standard_controls[] = { CONTROL_GAIN, CONTROL_OFFSET, CONTROL_GAMMA };
	for (int i = 0; i < 3; i++) {
		properties[i]->hidden = !qhy_available(device, standard_controls[i]);
		if (!properties[i]->hidden && !qhy_control_info(device, standard_controls[i], properties[i]->items)) {
			return false;
		}
	}
	double min = 0, max = 0, step = 0;
	if (!qhy_result(GetQHYCCDParamMinMaxStep(PRIVATE_DATA->handle, CONTROL_EXPOSURE, &min, &max, &step), "Exposure range") || !isfinite(min) || !isfinite(max) || !isfinite(step) || min < 0 || max < min || step < 0) {
		return false;
	}
	CCD_EXPOSURE_ITEM->number.min = min / 1e6;
	// Preserve the legacy QHY6 range workaround; its SDK underreports long exposures.
	CCD_EXPOSURE_ITEM->number.max = fmax(max / 1e6, 900);
	CCD_EXPOSURE_ITEM->number.step = step / 1e6;
	PRIVATE_DATA->has_cooler = qhy_available(device, CONTROL_COOLER);
	PRIVATE_DATA->has_temperature = PRIVATE_DATA->has_cooler || qhy_available(device, CAM_CHIPTEMPERATURESENSOR_INTERFACE);
	PRIVATE_DATA->has_shutter = qhy_available(device, CAM_MECHANICALSHUTTER);
	CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = !PRIVATE_DATA->has_cooler;
	CCD_TEMPERATURE_PROPERTY->hidden = !PRIVATE_DATA->has_temperature;
	CCD_TEMPERATURE_PROPERTY->perm = PRIVATE_DATA->has_cooler ? INDIGO_RW_PERM : INDIGO_RO_PERM;
	CCD_TEMPERATURE_ITEM->number.min = -50;
	CCD_TEMPERATURE_ITEM->number.max = 40;
	X_ADVANCED_PROPERTY = indigo_resize_property(X_ADVANCED_PROPERTY, 3);
	int count = 0;
	const CONTROL_ID advanced_controls[] = { CONTROL_USBTRAFFIC, CONTROL_SPEED, CAM_SHUTTERMOTORHEATING_INTERFACE };
	const char *names[] = { "USBTRAFFIC", "USBSPEED", "SHUTTERMOTORHEATING" };
	const char *labels[] = { "USB Traffic", "USB Speed", "Shutter Motor Heating" };
	for (int i = 0; i < 3; i++) {
		if (qhy_available(device, advanced_controls[i])) {
			indigo_item *item = X_ADVANCED_PROPERTY->items + count;
			indigo_init_number_item(item, names[i], labels[i], 0, 0, 1, 0);
			if (!qhy_control_info(device, advanced_controls[i], item)) {
				return false;
			}
			PRIVATE_DATA->advanced_controls[count++] = advanced_controls[i];
			// QHY5L-II readout can hang above 15 s below USB traffic 40; retain floor 50 within SDK range.
			if (i == 0 && item->number.value < 50 && item->number.max >= 50) {
				item->number.target = 50;
				if (!qhy_write_control(device, advanced_controls[i], item)) {
					return false;
				}
			}
		}
	}
	X_ADVANCED_PROPERTY->count = count;
	X_ADVANCED_PROPERTY->hidden = count == 0;
	X_READ_MODE_PROPERTY->hidden = true;
	return true;
}

static bool qhy_stop(indigo_device *device) {
	if (!PRIVATE_DATA->acquiring) {
		return true;
	}
	bool ok = PRIVATE_DATA->handle && qhy_result(PRIVATE_DATA->streaming ? StopQHYCCDLive(PRIVATE_DATA->handle) : CancelQHYCCDExposingAndReadout(PRIVATE_DATA->handle), "Stop acquisition");
	if (ok) {
		PRIVATE_DATA->acquiring = false;
		if (PRIVATE_DATA->streaming) {
			PRIVATE_DATA->last_bpp = 0;
			ok = qhy_result(SetQHYCCDStreamMode(PRIVATE_DATA->handle, 0), "Reset stream mode") && qhy_result(InitQHYCCD(PRIVATE_DATA->handle), "Reset after streaming");
			if (ok) {
				PRIVATE_DATA->last_live = false;
			}
		}
	}
	return ok;
}

static void acquisition_finish(indigo_device *device, bool ok) {
	bool streaming = PRIVATE_DATA->streaming;
	ok = qhy_stop(device) && ok;
	if (streaming) {
		indigo_finalize_video_stream(device);
		CCD_STREAMING_EXPOSURE_ITEM->number.value = 0;
	} else {
		CCD_EXPOSURE_ITEM->number.value = 0;
	}
	if (!ok) {
		indigo_ccd_failure_cleanup(device);
	}
	indigo_property *property = streaming ? CCD_STREAMING_PROPERTY : CCD_EXPOSURE_PROPERTY;
	property->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, property, ok ? NULL : "Acquisition failed");
}

static void acquisition_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->acquiring) {
		return;
	}
	double now = indigo_monotonic_time();
	if (now > PRIVATE_DATA->deadline) {
		acquisition_finish(device, false);
		return;
	}
	if (PRIVATE_DATA->streaming) {
		double remaining = ceil(fmax(0, PRIVATE_DATA->exposure_end - now));
		if (CCD_STREAMING_EXPOSURE_ITEM->number.value != remaining) {
			CCD_STREAMING_EXPOSURE_ITEM->number.value = remaining;
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		}
	}
	if (now < PRIVATE_DATA->exposure_end) {
		indigo_execute_handler_in(device, fmin(0.25, PRIVATE_DATA->exposure_end - now), acquisition_finalizer);
		return;
	}
	if (!PRIVATE_DATA->streaming) {
		uint32_t remaining = GetQHYCCDExposureRemaining(PRIVATE_DATA->handle);
		if (remaining == QHYCCD_ERROR) {
			acquisition_finish(device, false);
			return;
		}
		if (remaining > 100) {
			indigo_execute_handler_in(device, 0.05, acquisition_finalizer);
			return;
		}
	}
	uint32_t w = 0, h = 0, bpp = 0, channels = 0;
	uint32_t result = PRIVATE_DATA->streaming ? GetQHYCCDLiveFrame(PRIVATE_DATA->handle, &w, &h, &bpp, &channels, PRIVATE_DATA->buffer + FITS_HEADER_SIZE) : GetQHYCCDSingleFrame(PRIVATE_DATA->handle, &w, &h, &bpp, &channels, PRIVATE_DATA->buffer + FITS_HEADER_SIZE);
	if (result != QHYCCD_SUCCESS) {
		if (PRIVATE_DATA->streaming) {
			indigo_execute_handler_in(device, 0.02, acquisition_finalizer);
		} else {
			acquisition_finish(device, false);
		}
		return;
	}
	if (w != PRIVATE_DATA->frame_width || h != PRIVATE_DATA->frame_height || bpp != PRIVATE_DATA->last_bpp || channels != 1 || (uint64_t)w * h * (bpp / 8) > PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
		acquisition_finish(device, false);
		return;
	}
	const char *pattern = NULL;
	switch (IsQHYCCDControlAvailable(PRIVATE_DATA->handle, CAM_COLOR)) {
		case BAYER_GB: pattern = "GBRG"; break;
		case BAYER_GR: pattern = "GRBG"; break;
		case BAYER_BG: pattern = "BGGR"; break;
		case BAYER_RG: pattern = "RGGB"; break;
		default: break;
	}
	indigo_fits_keyword keywords[] = { { .type = INDIGO_FITS_STRING, .name = "BAYERPAT", .string = (char *)pattern, .comment = "Bayer color pattern" }, { .type = (indigo_fits_keyword_type)0 } };
	indigo_process_image(device, PRIVATE_DATA->buffer, w, h, bpp, true, true, pattern ? keywords : NULL, PRIVATE_DATA->streaming);
	if (!PRIVATE_DATA->streaming) {
		PRIVATE_DATA->acquiring = false;
		acquisition_finish(device, true);
		return;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		acquisition_finish(device, true);
		return;
	}
	PRIVATE_DATA->exposure_end = indigo_monotonic_time() + PRIVATE_DATA->duration;
	PRIVATE_DATA->deadline = PRIVATE_DATA->exposure_end + 10;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	indigo_execute_handler_in(device, fmin(0.25, PRIVATE_DATA->duration), acquisition_finalizer);
}

static bool qhy_setup(indigo_device *device, bool streaming) {
	int bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	if (!PRIVATE_DATA->handle || PRIVATE_DATA->last_bpp != bpp || PRIVATE_DATA->last_live != streaming) {
		// Reconfigure the current handle; close/reopen crashes some QHY SDKs.
		PRIVATE_DATA->last_bpp = 0;
		if (!PRIVATE_DATA->handle || !qhy_result(SetQHYCCDStreamMode(PRIVATE_DATA->handle, streaming ? 1 : 0), "SetQHYCCDStreamMode") || !qhy_result(InitQHYCCD(PRIVATE_DATA->handle), "InitQHYCCD") || (X_PIXEL_FORMAT_PROPERTY->count > 1 && !qhy_result(SetQHYCCDBitsMode(PRIVATE_DATA->handle, bpp), "SetQHYCCDBitsMode"))) {
			return false;
		}
		indigo_property *properties[] = { CCD_GAIN_PROPERTY, CCD_OFFSET_PROPERTY, CCD_GAMMA_PROPERTY };
		const CONTROL_ID controls[] = { CONTROL_GAIN, CONTROL_OFFSET, CONTROL_GAMMA };
		for (int i = 0; i < 3; i++) {
			if (!properties[i]->hidden && !qhy_write_control(device, controls[i], properties[i]->items)) {
				return false;
			}
		}
		for (int i = 0; i < X_ADVANCED_PROPERTY->count; i++) {
			if (!qhy_write_control(device, PRIVATE_DATA->advanced_controls[i], X_ADVANCED_PROPERTY->items + i)) {
				return false;
			}
		}
		PRIVATE_DATA->last_bpp = bpp;
		PRIVATE_DATA->last_live = streaming;
	}
	int bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	PRIVATE_DATA->frame_width = CCD_FRAME_WIDTH_ITEM->number.value / bin;
	PRIVATE_DATA->frame_height = CCD_FRAME_HEIGHT_ITEM->number.value / bin;
	if (!qhy_result(SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_EXPOSURE, PRIVATE_DATA->duration * 1e6), "Set exposure") || !qhy_result(SetQHYCCDBinMode(PRIVATE_DATA->handle, bin, bin), "Set bin") || !qhy_result(SetQHYCCDResolution(PRIVATE_DATA->handle, (PRIVATE_DATA->offset_x + CCD_FRAME_LEFT_ITEM->number.value) / bin, (PRIVATE_DATA->offset_y + CCD_FRAME_TOP_ITEM->number.value) / bin, PRIVATE_DATA->frame_width, PRIVATE_DATA->frame_height), "Set ROI")) {
		return false;
	}
	uint32_t length = GetQHYCCDMemLength(PRIVATE_DATA->handle);
	if (!length || length == QHYCCD_ERROR || length > PRIVATE_DATA->buffer_size - FITS_HEADER_SIZE) {
		return false;
	}
	if (PRIVATE_DATA->has_shutter && !qhy_result(ControlQHYCCDShutter(PRIVATE_DATA->handle, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value ? MACHANICALSHUTTER_CLOSE : MACHANICALSHUTTER_FREE), "Set shutter")) {
		return false;
	}
	return true;
}

static void acquisition_start(indigo_device *device, bool streaming) {
	if (!qhy_stop(device)) {
		indigo_ccd_failure_cleanup(device);
		indigo_property *property = streaming ? CCD_STREAMING_PROPERTY : CCD_EXPOSURE_PROPERTY;
		property->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, property, "Previous acquisition could not be stopped");
		return;
	}
	PRIVATE_DATA->streaming = streaming;
	PRIVATE_DATA->duration = streaming ? CCD_STREAMING_EXPOSURE_ITEM->number.target : CCD_EXPOSURE_ITEM->number.target;
	if (!qhy_setup(device, streaming)) {
		acquisition_finish(device, false);
		return;
	}
	uint32_t result = streaming ? BeginQHYCCDLive(PRIVATE_DATA->handle) : ExpQHYCCDSingleFrame(PRIVATE_DATA->handle);
	if (result != QHYCCD_SUCCESS && result != QHYCCD_READ_DIRECTLY) {
		acquisition_finish(device, false);
		return;
	}
	PRIVATE_DATA->acquiring = true;
	PRIVATE_DATA->exposure_end = indigo_monotonic_time() + PRIVATE_DATA->duration;
	PRIVATE_DATA->deadline = PRIVATE_DATA->exposure_end + 10;
	indigo_execute_handler_in(device, fmin(0.25, PRIVATE_DATA->duration), acquisition_finalizer);
}

static void qhy_temperature(indigo_device *device) {
	if (!PRIVATE_DATA->has_temperature || !PRIVATE_DATA->handle || (PRIVATE_DATA->acquiring && (PRIVATE_DATA->streaming || PRIVATE_DATA->exposure_end - indigo_monotonic_time() <= 4))) {
		return;
	}
	bool ok = true;
	if (PRIVATE_DATA->has_cooler) {
		ok = qhy_result(CCD_COOLER_ON_ITEM->sw.value ? ControlQHYCCDTemp(PRIVATE_DATA->handle, CCD_TEMPERATURE_ITEM->number.target) : SetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_MANULPWM, 0), "Set cooling");
		double power = GetQHYCCDParam(PRIVATE_DATA->handle, CONTROL_CURPWM);
		if (!isfinite(power) || power < 0 || power > 255) {
			ok = false;
		} else {
			CCD_COOLER_POWER_ITEM->number.value = CCD_COOLER_ON_ITEM->sw.value ? power / 2.55 : 0;
		}
	}
	double temperature = GetQHYCCDParam(PRIVATE_DATA->handle, PRIVATE_DATA->has_cooler ? CONTROL_CURTEMP : CAM_CHIPTEMPERATURESENSOR_INTERFACE);
	if (!isfinite(temperature) || temperature < -100 || temperature > 100 || temperature == (double)QHYCCD_ERROR) {
		ok = false;
	} else {
		CCD_TEMPERATURE_ITEM->number.value = temperature;
	}
	CCD_TEMPERATURE_PROPERTY->state = !ok ? INDIGO_ALERT_STATE : PRIVATE_DATA->has_cooler && CCD_COOLER_ON_ITEM->sw.value && fabs(temperature - CCD_TEMPERATURE_ITEM->number.target) > 0.3 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	CCD_COOLER_PROPERTY->state = CCD_COOLER_POWER_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	if (PRIVATE_DATA->has_cooler) {
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
}

static bool qhy_busy(indigo_device *device, indigo_property *property) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		property->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, property, "Acquisition in progress");
		return true;
	}
	return false;
}

static void guider_ra_finalizer(indigo_device *device) {
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void qhy_guide(indigo_device *device, bool ra) {
	indigo_property *property = ra ? GUIDER_GUIDE_RA_PROPERTY : GUIDER_GUIDE_DEC_PROPERTY;
	double duration = fmax(property->items[0].number.value, property->items[1].number.value);
	if (duration == 0) {
		(ra ? guider_ra_finalizer : guider_dec_finalizer)(device);
		return;
	}
	if (property->items[0].number.value > 0 && property->items[1].number.value > 0) {
		property->items[0].number.value = property->items[1].number.value = 0;
		property->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, property, "Opposed guide directions");
		return;
	}
	int direction = ra ? (GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? 0 : 3) : (GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? 1 : 2);
	property->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, property, NULL);
	double end = indigo_monotonic_time() + duration / 1000;
	// The vendor call may block for the pulse; no separate relay ON/OFF API is provided.
	if (!PRIVATE_DATA->handle || !qhy_result(ControlQHYCCDGuide(PRIVATE_DATA->handle, direction, (uint16_t)duration), "ControlQHYCCDGuide")) {
		property->items[0].number.value = property->items[1].number.value = 0;
		property->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, property, NULL);
		return;
	}
	indigo_execute_handler_in(device, fmax(0, end - indigo_monotonic_time()), ra ? guider_ra_finalizer : guider_dec_finalizer);
}

static void wheel_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	memset(PRIVATE_DATA->wheel_reply, 0, sizeof(PRIVATE_DATA->wheel_reply));
	if (!PRIVATE_DATA->handle || !qhy_result(GetQHYCCDCFWStatus(PRIVATE_DATA->handle, PRIVATE_DATA->wheel_reply), "GetQHYCCDCFWStatus")) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (PRIVATE_DATA->wheel_reply[0] >= '0' && PRIVATE_DATA->wheel_reply[0] < '0' + 8) {
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->wheel_reply[0] - '0' + 1;
		if (WHEEL_SLOT_ITEM->number.value == WHEEL_SLOT_ITEM->number.target) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else if (indigo_monotonic_time() < PRIVATE_DATA->wheel_deadline) {
			indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
			return;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static indigo_result qhy_generated_entry(indigo_driver_action action, indigo_driver_info *info);
extern "C" {
INDIGO_EXTERN indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info);
}

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	indigo_result result = qhy_generated_entry(action, info);
	// Failed generated queue setup must also roll back the SDK initialized by on_init.
	if (action == INDIGO_DRIVER_INIT && result != INDIGO_OK && sdk_initialized) {
		qhy_result(ReleaseQHYCCDResource(), "ReleaseQHYCCDResource after failed INIT");
		sdk_initialized = false;
	}
	if (info) {
		snprintf(info->name, sizeof(info->name), "%s", DRIVER_NAME);
	}
	return result;
}

#define indigo_ccd_qhy       qhy_generated_entry

//- code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	qhy_temperature(device);
	indigo_execute_handler_in(device, 1, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = qhy_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			connection_result = qhy_initialize_ccd(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
			indigo_define_property(device, X_ADVANCED_PROPERTY, NULL);
			indigo_define_property(device, X_READ_MODE_PROPERTY, NULL);
			indigo_execute_handler(device, ccd_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				qhy_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		qhy_stop(device);
		indigo_finalize_video_stream(device);
		indigo_ccd_failure_cleanup(device);
		CCD_EXPOSURE_ITEM->number.value = CCD_STREAMING_COUNT_ITEM->number.value = 0;
		CCD_EXPOSURE_PROPERTY->state = CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		//- ccd.on_disconnect
		indigo_delete_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
		indigo_delete_property(device, X_ADVANCED_PROPERTY, NULL);
		indigo_delete_property(device, X_READ_MODE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			qhy_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	acquisition_start(device, false); // acquisition_finalizer publishes completion.
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_streaming_handler(indigo_device *device) {
	//+ ccd.CCD_STREAMING.on_change
	indigo_use_shortest_exposure_if_bias(device);
	if (CCD_STREAMING_COUNT_ITEM->number.target == 0) {
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		return;
	}
	CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
	}
	acquisition_start(device, true); // acquisition_finalizer publishes completion.
	//- ccd.CCD_STREAMING.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, ccd_exposure_handler);
	indigo_cancel_pending_handler(device, ccd_streaming_handler);
	indigo_cancel_pending_handler(device, acquisition_finalizer);
	bool ok = qhy_stop(device);
	indigo_finalize_video_stream(device);
	indigo_ccd_failure_cleanup(device);
	CCD_EXPOSURE_ITEM->number.value = CCD_STREAMING_COUNT_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = CCD_STREAMING_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	CCD_ABORT_EXPOSURE_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	//+ ccd.CCD_COOLER.on_change
	CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	//+ ccd.CCD_TEMPERATURE.on_change
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	if (!qhy_write_control(device, CONTROL_GAIN, CCD_GAIN_ITEM)) {
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_offset_handler(indigo_device *device) {
	CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_OFFSET.on_change
	if (!qhy_write_control(device, CONTROL_OFFSET, CCD_OFFSET_ITEM)) {
		CCD_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_OFFSET.on_change
	indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
}

static void ccd_gamma_handler(indigo_device *device) {
	CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAMMA.on_change
	if (!qhy_write_control(device, CONTROL_GAMMA, CCD_GAMMA_ITEM)) {
		CCD_GAMMA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_GAMMA.on_change
	indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
}

static void ccd_frame_handler(indigo_device *device) {
	CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_FRAME.on_change
	int bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int left = CCD_FRAME_LEFT_ITEM->number.target;
	int top = CCD_FRAME_TOP_ITEM->number.target;
	int width = CCD_FRAME_WIDTH_ITEM->number.target;
	int height = CCD_FRAME_HEIGHT_ITEM->number.target;
	if (width < 64 * bin || height < 64 * bin || left + width > PRIVATE_DATA->width || top + height > PRIVATE_DATA->height || width % bin || height % bin) {
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_FRAME_LEFT_ITEM->number.value = left;
		CCD_FRAME_TOP_ITEM->number.value = top;
		CCD_FRAME_WIDTH_ITEM->number.value = width;
		CCD_FRAME_HEIGHT_ITEM->number.value = height;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = PRIVATE_DATA->selected_bpp;
	}
	//- ccd.CCD_FRAME.on_change
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

static void ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_BIN.on_change
	int bin = CCD_BIN_HORIZONTAL_ITEM->number.target != CCD_BIN_HORIZONTAL_ITEM->number.value ? CCD_BIN_HORIZONTAL_ITEM->number.target : CCD_BIN_VERTICAL_ITEM->number.target;
	if (!PRIVATE_DATA->bins[bin - 1] || CCD_FRAME_WIDTH_ITEM->number.value / bin < 64 || CCD_FRAME_HEIGHT_ITEM->number.value / bin < 64 || (int)CCD_FRAME_WIDTH_ITEM->number.value % bin || (int)CCD_FRAME_HEIGHT_ITEM->number.value % bin) {
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = bin;
		qhy_modes(device);
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	}
	//- ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void ccd_mode_handler(indigo_device *device) {
	CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_MODE.on_change
	for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
		if (CCD_MODE_PROPERTY->items[i].sw.value) {
			int bpp = 0, bin = 0;
			if (sscanf(CCD_MODE_PROPERTY->items[i].name, "RAW %d %dx", &bpp, &bin) == 2) {
				PRIVATE_DATA->selected_bpp = bpp;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = bpp;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = bin;
				qhy_update_geometry(device);
				for (int j = 0; j < X_PIXEL_FORMAT_PROPERTY->count; j++) {
					X_PIXEL_FORMAT_PROPERTY->items[j].sw.value = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[j].name, bpp == 8 ? "RAW 8" : "RAW 16");
				}
				indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
				indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
			}
			break;
		}
	}
	//- ccd.CCD_MODE.on_change
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
}

static void ccd_x_pixel_format_handler(indigo_device *device) {
	X_PIXEL_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_PIXEL_FORMAT.on_change
	for (int i = 0; i < X_PIXEL_FORMAT_PROPERTY->count; i++) {
		if (X_PIXEL_FORMAT_PROPERTY->items[i].sw.value) {
			PRIVATE_DATA->selected_bpp = !strcmp(X_PIXEL_FORMAT_PROPERTY->items[i].name, "RAW 8") ? 8 : 16;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = PRIVATE_DATA->selected_bpp;
		}
	}
	qhy_modes(device);
	indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
	indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
	//- ccd.X_PIXEL_FORMAT.on_change
	indigo_update_property(device, X_PIXEL_FORMAT_PROPERTY, NULL);
}

static void ccd_x_advanced_handler(indigo_device *device) {
	X_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_ADVANCED.on_change
	for (int i = 0; i < X_ADVANCED_PROPERTY->count; i++) {
		if (!qhy_write_control(device, PRIVATE_DATA->advanced_controls[i], X_ADVANCED_PROPERTY->items + i)) {
			X_ADVANCED_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- ccd.X_ADVANCED.on_change
	indigo_update_property(device, X_ADVANCED_PROPERTY, NULL);
}

static void ccd_x_read_mode_handler(indigo_device *device) {
	//+ ccd.X_READ_MODE.on_change
	X_READ_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	//- ccd.X_READ_MODE.on_change
	indigo_update_property(device, X_READ_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = 4;
		CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		//- ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_OFFSET_PROPERTY->hidden = false;
		CCD_GAMMA_PROPERTY->hidden = false;
		CCD_FRAME_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		CCD_MODE_PROPERTY->hidden = false;
		X_PIXEL_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PIXEL_FORMAT_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Pixel Format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_PIXEL_FORMAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(RAW8_ITEM, RAW8_ITEM_NAME, "RAW 8", false);
		indigo_init_switch_item(RAW16_ITEM, RAW16_ITEM_NAME, "RAW 16", true);
		X_ADVANCED_PROPERTY = indigo_init_number_property(NULL, device->name, X_ADVANCED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Advanced", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (X_ADVANCED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		X_READ_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_READ_MODE_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Read mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
		if (X_READ_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		X_READ_MODE_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PIXEL_FORMAT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ADVANCED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_READ_MODE_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, ccd_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		//+ ccd.CCD_EXPOSURE.on_change_request
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		//- ccd.CCD_EXPOSURE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		//+ ccd.CCD_STREAMING.on_change_request
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		//- ccd.CCD_STREAMING.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, ccd_temperature_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		//+ ccd.CCD_GAIN.on_change_request
		if (qhy_busy(device, CCD_GAIN_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_GAIN.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_OFFSET_PROPERTY, property)) {
		//+ ccd.CCD_OFFSET.on_change_request
		if (qhy_busy(device, CCD_OFFSET_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_OFFSET.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_OFFSET_PROPERTY, ccd_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAMMA_PROPERTY, property)) {
		//+ ccd.CCD_GAMMA.on_change_request
		if (qhy_busy(device, CCD_GAMMA_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_GAMMA.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAMMA_PROPERTY, ccd_gamma_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		//+ ccd.CCD_FRAME.on_change_request
		if (qhy_busy(device, CCD_FRAME_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_FRAME.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_FRAME_PROPERTY, ccd_frame_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		//+ ccd.CCD_BIN.on_change_request
		if (qhy_busy(device, CCD_BIN_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_BIN.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		//+ ccd.CCD_MODE.on_change_request
		if (qhy_busy(device, CCD_MODE_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.CCD_MODE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_MODE_PROPERTY, ccd_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PIXEL_FORMAT_PROPERTY, property)) {
		//+ ccd.X_PIXEL_FORMAT.on_change_request
		if (qhy_busy(device, X_PIXEL_FORMAT_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.X_PIXEL_FORMAT.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PIXEL_FORMAT_PROPERTY, ccd_x_pixel_format_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ADVANCED_PROPERTY, property)) {
		//+ ccd.X_ADVANCED.on_change_request
		if (qhy_busy(device, X_ADVANCED_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.X_ADVANCED.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(X_ADVANCED_PROPERTY, ccd_x_advanced_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_READ_MODE_PROPERTY, property)) {
		//+ ccd.X_READ_MODE.on_change_request
		if (qhy_busy(device, X_READ_MODE_PROPERTY)) {
			return INDIGO_OK;
		}
		//- ccd.X_READ_MODE.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_READ_MODE_PROPERTY, ccd_x_read_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_PIXEL_FORMAT_PROPERTY);
			indigo_save_property(device, NULL, X_ADVANCED_PROPERTY);
			indigo_save_property(device, NULL, X_READ_MODE_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(X_PIXEL_FORMAT_PROPERTY);
	indigo_release_property(X_ADVANCED_PROPERTY);
	indigo_release_property(X_READ_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = qhy_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				qhy_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			qhy_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	qhy_guide(device, true); /* guider_ra_finalizer publishes completion. */
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	qhy_guide(device, false); /* guider_dec_finalizer publishes completion. */
	//- guider.GUIDER_GUIDE_DEC.on_change
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		GUIDER_GUIDE_EAST_ITEM->number.max = GUIDER_GUIDE_WEST_ITEM->number.max = GUIDER_GUIDE_NORTH_ITEM->number.max = GUIDER_GUIDE_SOUTH_ITEM->number.max = 65535;
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, guider_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = qhy_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ wheel.on_connect
			char target = '0';
			connection_result = qhy_result(SendOrder2QHYCCDCFW(PRIVATE_DATA->handle, &target, 1), "Initialize CFW");
			if (connection_result) {
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = 8;
				WHEEL_SLOT_ITEM->number.target = 1;
				WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + 90;
				indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
			}
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				qhy_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			qhy_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	char target = '0' + (int)WHEEL_SLOT_ITEM->number.target - 1;
	if (!PRIVATE_DATA->handle || !qhy_result(SendOrder2QHYCCDCFW(PRIVATE_DATA->handle, &target, 1), "SendOrder2QHYCCDCFW")) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + 90;
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	//- wheel.WHEEL_SLOT.on_change
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, wheel_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - Device templates

static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(CCD_DEVICE_NAME, ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

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
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	qhy_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (qhy_private_data *)indigo_safe_malloc(sizeof(qhy_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (libusb_get_device_descriptor(dev, &descriptor) != LIBUSB_SUCCESS) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		if (descriptor.idVendor != 0x1618 && descriptor.idVendor != 0x16c0 && descriptor.idVendor != 0x1856 && descriptor.idVendor != 0x04b4 && descriptor.idVendor != 0x0547) {
			plug_result = false;
		} else {
			uint32_t count = ScanQHYCCD();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Discovery ScanQHYCCD = %u", count);
			plug_result = false;
			if (count <= 256) {
				for (uint32_t i = 0; i < count; i++) {
					char sid[256] = { 0 };
					if (!qhy_result(GetQHYCCDId(i, sid), "GetQHYCCDId") || !memchr(sid, 0, sizeof(sid)) || !sid[0]) {
						break;
					}
					bool duplicate = false;
					int available = 0;
					for (int j = 0; j < MAX_DEVICES; j++) {
						if (!devices[j]) {
							available++;
						} else if (!strcmp(((qhy_private_data *)devices[j]->private_data)->sid, sid)) {
							duplicate = true;
						}
					}
					if (duplicate) {
						continue;
					}
					char model[256] = { 0 };
					if (!qhy_result(GetQHYCCDModel(sid, model), "GetQHYCCDModel") || !memchr(model, 0, sizeof(model)) || !model[0]) {
						break;
					}
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Discovery probe %s (%s)", sid, model);
					qhyccd_handle *handle = OpenQHYCCD(sid);
					if (!handle) {
						break;
					}
					private_data->has_guider = IsQHYCCDControlAvailable(handle, CONTROL_ST4PORT) == QHYCCD_SUCCESS;
					private_data->has_wheel = IsQHYCCDControlAvailable(handle, CONTROL_CFWPORT) == QHYCCD_SUCCESS;
					bool closed = qhy_result(CloseQHYCCD(handle), "Close discovery probe");
					if (!closed || available < 1 + private_data->has_guider + private_data->has_wheel) {
						break;
					}
					snprintf(private_data->sid, sizeof(private_data->sid), "%s", sid);
					snprintf(private_data->camera_name, sizeof(private_data->camera_name), "%s", model);
					indigo_make_name_unique(private_data->camera_name, "%s", sid);
					snprintf(private_data->guider_name, sizeof(private_data->guider_name), "%s (guider)", model);
					indigo_make_name_unique(private_data->guider_name, "%s", sid);
					snprintf(private_data->wheel_name, sizeof(private_data->wheel_name), "%s (wheel)", model);
					indigo_make_name_unique(private_data->wheel_name, "%s", sid);
					plug_result = true;
					break;
				}
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		ccd->private_data = private_data;
		snprintf(ccd->name, INDIGO_NAME_SIZE, "%s", private_data->camera_name);
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
		if (ccd_attached && private_data->has_guider) {
		indigo_device *guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
		guider->private_data = private_data;
		guider->master_device = ccd;
		snprintf(guider->name, INDIGO_NAME_SIZE, "%s", private_data->guider_name);
		bool guider_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = guider;
				if (indigo_attach_device(guider) == INDIGO_OK) {
					dev_ref_transferred = true;
					guider_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!guider_attached) {
			indigo_safe_free(guider);
		}
		}
		if (ccd_attached && private_data->has_wheel) {
		indigo_device *wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
		wheel->private_data = private_data;
		wheel->master_device = ccd;
		snprintf(wheel->name, INDIGO_NAME_SIZE, "%s", private_data->wheel_name);
		bool wheel_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = wheel;
				if (indigo_attach_device(wheel) == INDIGO_OK) {
					dev_ref_transferred = true;
					wheel_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!wheel_attached) {
			indigo_safe_free(wheel);
		}
		}
	}
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	qhy_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				indigo_safe_free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		indigo_safe_free(private_data);
	}
	libusb_unref_device(dev);
}

static void discover_devices_handler(indigo_device *device) {
	libusb_device **list = NULL;
	ssize_t count = libusb_get_device_list(NULL, &list);
	if (count < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initial USB enumeration failed: %s", libusb_error_name((int)count));
		return;
	}
	for (ssize_t i = 0; i < count; i++) {
		process_plug_event_handler(NULL, libusb_ref_device(list[i]));
	}
	libusb_free_device_list(list, 1);
}

#pragma mark - Main code

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			if (indigo_driver_initialized((char *)CONFLICTING_DRIVER)) {
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			SetQHYCCDLogLevel(6);
			if (!qhy_result(InitQHYCCDResource(), "InitQHYCCDResource")) {
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			sdk_initialized = true;
			#ifdef INDIGO_MACOS
			char firmware_path[1024];
			snprintf(firmware_path, sizeof(firmware_path), "%s", getenv("INDIGO_FIRMWARE_BASE") ? getenv("INDIGO_FIRMWARE_BASE") : "/usr/local/lib/qhy");
			qhy_result(OSXInitQHYCCDFirmware(firmware_path), "OSXInitQHYCCDFirmware");
			#endif
			//- on_init
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
			indigo_start_usb_event_handler();
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
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
		//+ on_shutdown
		if (sdk_initialized) {
			qhy_result(ReleaseQHYCCDResource(), "ReleaseQHYCCDResource");
			sdk_initialized = false;
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
#include "indigo_ccd_qhy.h"

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	SET_DRIVER_INFO(info, "QHY CCD (legacy) Camera", __FUNCTION__, 0x0300001E, false, INDIGO_DRIVER_SHUTDOWN);
	return action == INDIGO_DRIVER_INFO ? INDIGO_OK : INDIGO_UNSUPPORTED_ARCH;
}
#endif
