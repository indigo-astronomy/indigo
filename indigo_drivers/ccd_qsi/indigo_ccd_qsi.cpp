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

// This file generated from indigo_ccd_qsi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <stdexcept>
#include <string>
#include "qsiapi.h"
#include "QSIError.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_qsi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000011
#define DRIVER_NAME          "indigo_ccd_qsi"
#define DRIVER_LABEL         "QSI Camera"
#define CCD_DEVICE_NAME      "%s"
#define WHEEL_DEVICE_NAME    "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((qsi_private_data *)device->private_data)

//+ define

#define QSI_VENDOR_ID        0x0403
#define QSI_PRODUCT_ID1      0xEB48
#define QSI_PRODUCT_ID2      0xEB49
#define QSI_TEMPERATURE_PERIOD 5
#define QSI_READOUT_POLL_PERIOD 0.005
#define QSI_READOUT_TIMEOUT  60
#define QSI_WHEEL_POLL_PERIOD 0.1
#define QSI_WHEEL_TIMEOUT    60

//- define

#pragma mark - Property definitions

#define X_QSI_READOUT_SPEED_PROPERTY      (PRIVATE_DATA->x_qsi_readout_speed_property)
#define X_QSI_READOUT_HQ_ITEM             (X_QSI_READOUT_SPEED_PROPERTY->items + 0)
#define X_QSI_READOUT_FAST_ITEM           (X_QSI_READOUT_SPEED_PROPERTY->items + 1)

#define X_QSI_READOUT_SPEED_PROPERTY_NAME "X_QSI_READOUT_SPEED"
#define X_QSI_READOUT_HQ_ITEM_NAME        "HIGH_QUALITY"
#define X_QSI_READOUT_FAST_ITEM_NAME      "FAST_READOUT"

#define X_QSI_ANTI_BLOOM_PROPERTY         (PRIVATE_DATA->x_qsi_anti_bloom_property)
#define X_QSI_ANTI_BLOOM_NORMAL_ITEM      (X_QSI_ANTI_BLOOM_PROPERTY->items + 0)
#define X_QSI_ANTI_BLOOM_HIGH_ITEM        (X_QSI_ANTI_BLOOM_PROPERTY->items + 1)

#define X_QSI_ANTI_BLOOM_PROPERTY_NAME    "X_QSI_ANTI_BLOOM"
#define X_QSI_ANTI_BLOOM_NORMAL_ITEM_NAME "NORMAL"
#define X_QSI_ANTI_BLOOM_HIGH_ITEM_NAME   "HIGH"

#define X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY               (PRIVATE_DATA->x_qsi_pre_exposure_flush_property)
#define X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM              (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items + 0)
#define X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM            (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items + 1)
#define X_QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM            (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items + 2)
#define X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM        (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items + 3)
#define X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM      (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items + 4)

#define X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY_NAME          "X_QSI_PRE_EXPOSURE_FLUSH"
#define X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM_NAME         "NONE"
#define X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM_NAME       "MODEST"
#define X_QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM_NAME       "NORMAL"
#define X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM_NAME   "AGGRESSIVE"
#define X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM_NAME "VERY_AGGRESSIVE"

#define X_QSI_FAN_MODE_PROPERTY        (PRIVATE_DATA->x_qsi_fan_mode_property)
#define X_QSI_FAN_MODE_OFF_ITEM        (X_QSI_FAN_MODE_PROPERTY->items + 0)
#define X_QSI_FAN_MODE_QUIET_ITEM      (X_QSI_FAN_MODE_PROPERTY->items + 1)
#define X_QSI_FAN_MODE_FULL_ITEM       (X_QSI_FAN_MODE_PROPERTY->items + 2)

#define X_QSI_FAN_MODE_PROPERTY_NAME   "X_QSI_FAN_MODE"
#define X_QSI_FAN_MODE_OFF_ITEM_NAME   "OFF"
#define X_QSI_FAN_MODE_QUIET_ITEM_NAME "QUIET"
#define X_QSI_FAN_MODE_FULL_ITEM_NAME  "FULL_SPEED"

#pragma mark - Private data definition

typedef struct {
	int count;
	libusb_device *usbdev;
	indigo_property *x_qsi_readout_speed_property;
	indigo_property *x_qsi_anti_bloom_property;
	indigo_property *x_qsi_pre_exposure_flush_property;
	indigo_property *x_qsi_fan_mode_property;
	//+ data
	char serial[INDIGO_NAME_SIZE];
	char model[INDIGO_NAME_SIZE];
	char wheel_name[INDIGO_NAME_SIZE];
	long width, height;
	double pixel_width, pixel_height;
	short max_bin_x, max_bin_y;
	double min_exposure, max_exposure;
	bool power_of_two_binning;
	bool has_shutter;
	bool can_set_temperature;
	bool can_get_cooler_power;
	bool can_set_gain;
	bool cooler_on;
	int gain;
	int fan_mode;
	int readout_speed;
	int anti_bloom;
	int pre_exposure_flush;
	bool has_filter_wheel;
	int filter_count;
	int wheel_slot;
	double wheel_deadline;
	bool can_check_temperature;
	bool exposure_active;
	double readout_deadline;
	int image_width, image_height;
	unsigned short *buffer;
	//- data
} qsi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

//+ code

// The vendor SDK exposes one global camera object and allows a single
// connected camera at a time; every logical device shares it.
static QSICamera cam;
static char qsi_last_error[INDIGO_VALUE_SIZE];

// The SDK defaults to structured exceptions (CCDCamera.cpp sets
// m_bStructuredExceptions, lib/wincompat.h throws from Error()), but the
// same calls report through their return code when that mode is off, so
// both channels are handled.
template<typename operation_type> static bool qsi_call(const char *operation, operation_type call) {
	try {
		int result = call();
		if (result != QSI_OK) {
			snprintf(qsi_last_error, sizeof(qsi_last_error), "%s failed (0x%08x)", operation, (unsigned)result);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
			return false;
		}
		return true;
	} catch (std::runtime_error &error) {
		snprintf(qsi_last_error, sizeof(qsi_last_error), "%s failed: %s", operation, error.what());
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
		return false;
	}
}

#define QSI_CALL(operation,  call) qsi_call(operation, [&]() -> int { return (call); })

static bool qsi_enumerate(std::string *serials, std::string *descriptions, int *count) {
	*count = 0;
	if (!QSI_CALL("get_AvailableCameras", cam.get_AvailableCameras(serials, descriptions, *count))) {
		return false;
	}
	if (*count < 0 || *count > QSICamera::MAXCAMERAS) {
		*count = 0;
		return false;
	}
	return true;
}

// Reads the shared capability set in the same order as the pre-migration
// driver, so the recorded SDK reference trace stays unchanged.
static bool qsi_read_capabilities(indigo_device *device) {
	std::string model("");
	if (!QSI_CALL("get_ModelNumber", cam.get_ModelNumber(model))) {
		return false;
	}
	INDIGO_COPY_NAME(PRIVATE_DATA->model, model.c_str());
	if (!QSI_CALL("get_CameraXSize", cam.get_CameraXSize(&PRIVATE_DATA->width)) || !QSI_CALL("get_CameraYSize", cam.get_CameraYSize(&PRIVATE_DATA->height))) {
		return false;
	}
	if (!QSI_CALL("get_PixelSizeX", cam.get_PixelSizeX(&PRIVATE_DATA->pixel_width)) || !QSI_CALL("get_PixelSizeY", cam.get_PixelSizeY(&PRIVATE_DATA->pixel_height))) {
		return false;
	}
	if (PRIVATE_DATA->width <= 0 || PRIVATE_DATA->height <= 0 || !isfinite(PRIVATE_DATA->pixel_width) || !isfinite(PRIVATE_DATA->pixel_height) || (size_t)PRIVATE_DATA->width > (SIZE_MAX - FITS_HEADER_SIZE) / 2 / (size_t)PRIVATE_DATA->height) {
		snprintf(qsi_last_error, sizeof(qsi_last_error), "Invalid camera geometry");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
		return false;
	}
	if (!QSI_CALL("get_CanSetCCDTemperature", cam.get_CanSetCCDTemperature(&PRIVATE_DATA->can_set_temperature)) || !QSI_CALL("get_HasShutter", cam.get_HasShutter(&PRIVATE_DATA->has_shutter))) {
		return false;
	}
	if (!QSI_CALL("get_MinExposureTime", cam.get_MinExposureTime(&PRIVATE_DATA->min_exposure)) || !QSI_CALL("get_MaxExposureTime", cam.get_MaxExposureTime(&PRIVATE_DATA->max_exposure))) {
		return false;
	}
	if (!QSI_CALL("get_HasFilterWheel", cam.get_HasFilterWheel(&PRIVATE_DATA->has_filter_wheel))) {
		return false;
	}
	PRIVATE_DATA->filter_count = 0;
	if (PRIVATE_DATA->has_filter_wheel && !QSI_CALL("get_FilterCount", cam.get_FilterCount(PRIVATE_DATA->filter_count))) {
		return false;
	}
	if (!QSI_CALL("get_CanSetGain", cam.get_CanSetGain(&PRIVATE_DATA->can_set_gain))) {
		return false;
	}
	PRIVATE_DATA->gain = QSICamera::CameraGainHigh;
	if (PRIVATE_DATA->can_set_gain) {
		QSICamera::CameraGain gain = QSICamera::CameraGainHigh;
		if (!QSI_CALL("get_CameraGain", cam.get_CameraGain(&gain))) {
			return false;
		}
		PRIVATE_DATA->gain = (int)gain;
	}
	if (!QSI_CALL("get_MaxBinX", cam.get_MaxBinX(&PRIVATE_DATA->max_bin_x)) || !QSI_CALL("get_MaxBinY", cam.get_MaxBinY(&PRIVATE_DATA->max_bin_y)) || !QSI_CALL("get_PowerOfTwoBinning", cam.get_PowerOfTwoBinning(&PRIVATE_DATA->power_of_two_binning))) {
		return false;
	}
	if (PRIVATE_DATA->max_bin_x < 1 || PRIVATE_DATA->max_bin_y < 1) {
		snprintf(qsi_last_error, sizeof(qsi_last_error), "Invalid binning limits");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
		return false;
	}
	QSICamera::FanMode fan_mode = QSICamera::fanQuiet;
	if (!QSI_CALL("get_FanMode", cam.get_FanMode(fan_mode))) {
		return false;
	}
	PRIVATE_DATA->fan_mode = (int)fan_mode;
	PRIVATE_DATA->cooler_on = false;
	if (PRIVATE_DATA->can_set_temperature && !QSI_CALL("get_CoolerOn", cam.get_CoolerOn(&PRIVATE_DATA->cooler_on))) {
		return false;
	}
	if (!QSI_CALL("get_CanGetCoolerPower", cam.get_CanGetCoolerPower(&PRIVATE_DATA->can_get_cooler_power))) {
		return false;
	}
	QSICamera::ReadoutSpeed readout_speed = QSICamera::HighImageQuality;
	if (!QSI_CALL("get_ReadoutSpeed", cam.get_ReadoutSpeed(readout_speed))) {
		return false;
	}
	PRIVATE_DATA->readout_speed = (int)readout_speed;
	QSICamera::AntiBloom anti_bloom = QSICamera::AntiBloomNormal;
	if (!QSI_CALL("get_AntiBlooming", cam.get_AntiBlooming(&anti_bloom))) {
		return false;
	}
	PRIVATE_DATA->anti_bloom = (int)anti_bloom;
	QSICamera::PreExposureFlush pre_exposure_flush = QSICamera::FlushNormal;
	if (!QSI_CALL("get_PreExposureFlush", cam.get_PreExposureFlush(&pre_exposure_flush))) {
		return false;
	}
	PRIVATE_DATA->pre_exposure_flush = (int)pre_exposure_flush;
	return true;
}

static void qsi_close(indigo_device *device) {
	indigo_lock_master_device(device);
	// The SDK connection and the blob buffer are released unconditionally,
	// so a failing disconnect cannot strand either of them.
	QSI_CALL("put_Connected(false)", cam.put_Connected(false));
	indigo_safe_free(PRIVATE_DATA->buffer);
	PRIVATE_DATA->buffer = NULL;
	PRIVATE_DATA->exposure_active = false;
	PRIVATE_DATA->can_check_temperature = true;
	indigo_unlock_master_device(device);
}

static bool qsi_open(indigo_device *device) {
	indigo_lock_master_device(device);
	bool connected = false;
	bool adopted = false;
	bool result = QSI_CALL("get_Connected", cam.get_Connected(&connected));
	if (result && connected) {
		// Only one QSI camera can be open at a time.
		std::string selected("");
		QSI_CALL("get_SelectCamera", cam.get_SelectCamera(selected));
		if (!strcmp(selected.c_str(), PRIVATE_DATA->serial)) {
			// A previous disconnect failed and left this camera claimed by
			// the SDK. Adopting that session is the only recovery short of
			// reloading the driver.
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Reusing the SDK session of camera #%s", PRIVATE_DATA->serial);
			adopted = true;
		} else {
			snprintf(qsi_last_error, sizeof(qsi_last_error), "Camera #%s is already connected, to use #%s disconnect it first.", selected.c_str(), PRIVATE_DATA->serial);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
			result = false;
		}
	}
	if (result && !adopted) {
		std::string serial(PRIVATE_DATA->serial);
		result = QSI_CALL("put_SelectCamera", cam.put_SelectCamera(serial)) && QSI_CALL("put_IsMainCamera", cam.put_IsMainCamera(true)) && QSI_CALL("put_Connected(true)", cam.put_Connected(true));
	}
	if (result) {
		result = qsi_read_capabilities(device);
		if (result) {
			PRIVATE_DATA->buffer = (unsigned short *)indigo_alloc_blob_buffer(2 * PRIVATE_DATA->width * PRIVATE_DATA->height + FITS_HEADER_SIZE);
			result = PRIVATE_DATA->buffer != NULL;
		}
		if (!result) {
			// Roll back the SDK connection, otherwise the camera stays
			// claimed and can never be opened again.
			QSI_CALL("put_Connected(false)", cam.put_Connected(false));
			indigo_safe_free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
	}
	PRIVATE_DATA->exposure_active = false;
	PRIVATE_DATA->can_check_temperature = true;
	indigo_unlock_master_device(device);
	return result;
}

// Discovery has to know whether a camera carries a filter wheel before the
// logical devices are attached. The SDK allows one open camera, so a camera
// arriving while another one is connected cannot be probed; it is then
// assumed to have a wheel and the wheel connection handler validates it.
static bool qsi_probe(qsi_private_data *private_data) {
	bool connected = false;
	if (!QSI_CALL("get_Connected", cam.get_Connected(&connected))) {
		return false;
	}
	if (connected) {
		private_data->has_filter_wheel = true;
		private_data->filter_count = 0;
		return true;
	}
	std::string serial(private_data->serial);
	if (!QSI_CALL("put_SelectCamera", cam.put_SelectCamera(serial)) || !QSI_CALL("put_Connected(true)", cam.put_Connected(true))) {
		return false;
	}
	bool has_filter_wheel = false;
	int filter_count = 0;
	bool result = QSI_CALL("get_HasFilterWheel", cam.get_HasFilterWheel(&has_filter_wheel));
	if (result && has_filter_wheel) {
		result = QSI_CALL("get_FilterCount", cam.get_FilterCount(filter_count));
	}
	QSI_CALL("put_Connected(false)", cam.put_Connected(false));
	if (!result) {
		return false;
	}
	private_data->has_filter_wheel = has_filter_wheel && filter_count > 0;
	private_data->filter_count = filter_count;
	return true;
}

//- code

//+ ccd.code

static void exposure_finalizer(indigo_device *device);

static void qsi_exposure_failed(indigo_device *device) {
	PRIVATE_DATA->exposure_active = false;
	PRIVATE_DATA->can_check_temperature = true;
	indigo_ccd_failure_cleanup(device);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "%s", qsi_last_error);
}

static void qsi_abort_exposure(indigo_device *device) {
	bool can_abort = false;
	if (QSI_CALL("get_CanAbortExposure", cam.get_CanAbortExposure(&can_abort)) && can_abort) {
		QSI_CALL("AbortExposure", cam.AbortExposure());
	}
}

static void exposure_finalizer(indigo_device *device) {
	if (!IS_CONNECTED || !PRIVATE_DATA->exposure_active) {
		return;
	}
	bool ready = false;
	if (!QSI_CALL("get_ImageReady", cam.get_ImageReady(&ready))) {
		qsi_exposure_failed(device);
		return;
	}
	if (!ready) {
		if (indigo_monotonic_time() < PRIVATE_DATA->readout_deadline) {
			indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, QSI_READOUT_POLL_PERIOD, exposure_finalizer);
		} else {
			qsi_abort_exposure(device);
			snprintf(qsi_last_error, sizeof(qsi_last_error), "Image readout timed out");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
			qsi_exposure_failed(device);
		}
		return;
	}
	int width = 0, height = 0, element_size = 0;
	if (!QSI_CALL("get_ImageArraySize", cam.get_ImageArraySize(width, height, element_size))) {
		qsi_exposure_failed(device);
		return;
	}
	if (width != PRIVATE_DATA->image_width || height != PRIVATE_DATA->image_height) {
		snprintf(qsi_last_error, sizeof(qsi_last_error), "Unexpected image size %d x %d", width, height);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", qsi_last_error);
		qsi_exposure_failed(device);
		return;
	}
	if (!QSI_CALL("get_ImageArray", cam.get_ImageArray(PRIVATE_DATA->buffer + FITS_HEADER_SIZE / 2))) {
		qsi_exposure_failed(device);
		return;
	}
	PRIVATE_DATA->exposure_active = false;
	PRIVATE_DATA->can_check_temperature = true;
	CCD_EXPOSURE_ITEM->number.value = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Image %d x %d", width, height);
	indigo_process_image(device, PRIVATE_DATA->buffer, width, height, 16, true, true, NULL, false);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static bool qsi_initialize_ccd(indigo_device *device) {
	snprintf(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_NAME_SIZE, "QSI %s", PRIVATE_DATA->model);
	indigo_update_property(device, INFO_PROPERTY, NULL);
	CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->width;
	CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = PRIVATE_DATA->height;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.target = 0;
	CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.target = 0;
	CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_SIZE_ITEM->number.value = PRIVATE_DATA->pixel_width;
	CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->pixel_height;
	CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = PRIVATE_DATA->max_bin_x;
	CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = PRIVATE_DATA->max_bin_y;
	CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;
	CCD_EXPOSURE_ITEM->number.min = PRIVATE_DATA->min_exposure;
	CCD_EXPOSURE_ITEM->number.max = PRIVATE_DATA->max_exposure;
	CCD_BIN_PROPERTY->perm = CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
	CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
	CCD_BIN_HORIZONTAL_ITEM->number.max = PRIVATE_DATA->max_bin_x;
	CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.min = 1;
	CCD_BIN_VERTICAL_ITEM->number.max = PRIVATE_DATA->max_bin_y;
	int max_bin = PRIVATE_DATA->max_bin_x > PRIVATE_DATA->max_bin_y ? PRIVATE_DATA->max_bin_y : PRIVATE_DATA->max_bin_x;
	CCD_MODE_PROPERTY = indigo_resize_property(CCD_MODE_PROPERTY, max_bin);
	CCD_MODE_PROPERTY->count = 0;
	for (int bin = 1; bin <= max_bin; bin = PRIVATE_DATA->power_of_two_binning ? (bin * 2) : (bin + 1)) {
		char name[32], label[128];
		snprintf(name, sizeof(name), "BIN_%dx%d", bin, bin);
		snprintf(label, sizeof(label), "RAW 16 %dx%d", (int)(PRIVATE_DATA->width / bin), (int)(PRIVATE_DATA->height / bin));
		indigo_init_switch_item(CCD_MODE_ITEM + (CCD_MODE_PROPERTY->count++), name, label, bin == 1);
	}
	CCD_GAIN_PROPERTY->hidden = !PRIVATE_DATA->can_set_gain;
	if (PRIVATE_DATA->can_set_gain) {
		CCD_GAIN_ITEM->number.min = QSICamera::CameraGainHigh;
		CCD_GAIN_ITEM->number.max = QSICamera::CameraGainAuto;
		CCD_GAIN_ITEM->number.step = 1;
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = PRIVATE_DATA->gain;
	}
	CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = !PRIVATE_DATA->can_set_temperature;
	if (PRIVATE_DATA->can_set_temperature) {
		CCD_TEMPERATURE_PROPERTY->perm = CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_TEMPERATURE_ITEM->number.min = -60;
		CCD_TEMPERATURE_ITEM->number.max = 60;
		CCD_TEMPERATURE_ITEM->number.step = 1;
		indigo_set_switch(CCD_COOLER_PROPERTY, PRIVATE_DATA->cooler_on ? CCD_COOLER_ON_ITEM : CCD_COOLER_OFF_ITEM, true);
	}
	CCD_COOLER_POWER_PROPERTY->hidden = !PRIVATE_DATA->can_get_cooler_power;
	X_QSI_FAN_MODE_PROPERTY->hidden = false;
	switch (PRIVATE_DATA->fan_mode) {
		case QSICamera::fanOff:
			indigo_set_switch(X_QSI_FAN_MODE_PROPERTY, X_QSI_FAN_MODE_OFF_ITEM, true);
			break;
		case QSICamera::fanQuiet:
			indigo_set_switch(X_QSI_FAN_MODE_PROPERTY, X_QSI_FAN_MODE_QUIET_ITEM, true);
			break;
		case QSICamera::fanFull:
			indigo_set_switch(X_QSI_FAN_MODE_PROPERTY, X_QSI_FAN_MODE_FULL_ITEM, true);
			break;
		default:
			X_QSI_FAN_MODE_PROPERTY->hidden = true;
			break;
	}
	X_QSI_READOUT_SPEED_PROPERTY->hidden = false;
	switch (PRIVATE_DATA->readout_speed) {
		case QSICamera::HighImageQuality:
			indigo_set_switch(X_QSI_READOUT_SPEED_PROPERTY, X_QSI_READOUT_HQ_ITEM, true);
			break;
		case QSICamera::FastReadout:
			indigo_set_switch(X_QSI_READOUT_SPEED_PROPERTY, X_QSI_READOUT_FAST_ITEM, true);
			break;
		default:
			X_QSI_READOUT_SPEED_PROPERTY->hidden = true;
			break;
	}
	X_QSI_ANTI_BLOOM_PROPERTY->hidden = false;
	switch (PRIVATE_DATA->anti_bloom) {
		case QSICamera::AntiBloomNormal:
			indigo_set_switch(X_QSI_ANTI_BLOOM_PROPERTY, X_QSI_ANTI_BLOOM_NORMAL_ITEM, true);
			break;
		case QSICamera::AntiBloomHigh:
			indigo_set_switch(X_QSI_ANTI_BLOOM_PROPERTY, X_QSI_ANTI_BLOOM_HIGH_ITEM, true);
			break;
		default:
			X_QSI_ANTI_BLOOM_PROPERTY->hidden = true;
			break;
	}
	X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->hidden = false;
	switch (PRIVATE_DATA->pre_exposure_flush) {
		case QSICamera::FlushNone:
			indigo_set_switch(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM, true);
			break;
		case QSICamera::FlushModest:
			indigo_set_switch(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM, true);
			break;
		case QSICamera::FlushNormal:
			indigo_set_switch(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, X_QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM, true);
			break;
		case QSICamera::FlushAggressive:
			indigo_set_switch(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM, true);
			break;
		case QSICamera::FlushVeryAggressive:
			indigo_set_switch(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM, true);
			break;
		default:
			X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->hidden = true;
			break;
	}
	return true;
}

//- ccd.code

//+ wheel.code

// Handlers and finalizers already run with the master device mutex held
// by the handler queue, so they must not lock it again.
static void wheel_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	short position = -1;
	bool result = QSI_CALL("get_Position", cam.get_Position(&position));
	if (!result) {
		WHEEL_SLOT_ITEM->number.target = WHEEL_SLOT_ITEM->number.value;
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "%s", qsi_last_error);
		return;
	}
	if (position < 0) {
		if (indigo_monotonic_time() < PRIVATE_DATA->wheel_deadline) {
			indigo_execute_handler_in(device, QSI_WHEEL_POLL_PERIOD, wheel_move_finalizer);
		} else {
			WHEEL_SLOT_ITEM->number.target = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Filter wheel did not reach the requested slot");
		}
		return;
	}
	PRIVATE_DATA->wheel_slot = position + 1;
	WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->wheel_slot;
	WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

//- wheel.code

#pragma mark - High level code (ccd)

static void ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ ccd.on_timer
	// The pre-migration driver deliberately kept the SDK idle between
	// exposure start and image handoff; that is preserved.
	if (PRIVATE_DATA->can_check_temperature) {
		double temperature = 0;
		if (QSI_CALL("get_CCDTemperature", cam.get_CCDTemperature(&temperature))) {
			CCD_TEMPERATURE_ITEM->number.value = temperature;
			// BUSY means "cooling towards the target". The generated change
			// dispatch rejects a request while the property is BUSY, so an
			// idle cooler must leave the property in OK.
			CCD_TEMPERATURE_PROPERTY->state = !CCD_COOLER_PROPERTY->hidden && CCD_COOLER_ON_ITEM->sw.value && fabs(temperature - CCD_TEMPERATURE_ITEM->number.target) > 0.2 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "%s", qsi_last_error);
		}
		bool can_get_cooler_power = false;
		if (QSI_CALL("get_CanGetCoolerPower", cam.get_CanGetCoolerPower(&can_get_cooler_power)) && can_get_cooler_power) {
			double power = 0;
			if (QSI_CALL("get_CoolerPower", cam.get_CoolerPower(&power))) {
				CCD_COOLER_POWER_ITEM->number.value = power;
				CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			} else {
				CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, "%s", qsi_last_error);
			}
		}
	}
	indigo_execute_handler_in(device, QSI_TEMPERATURE_PERIOD, ccd_timer_callback);
	//- ccd.on_timer
}

static void ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = qsi_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ ccd.on_connect
			connection_result = qsi_initialize_ccd(device);
			//- ccd.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_QSI_READOUT_SPEED_PROPERTY, NULL);
			indigo_define_property(device, X_QSI_ANTI_BLOOM_PROPERTY, NULL);
			indigo_define_property(device, X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
			indigo_define_property(device, X_QSI_FAN_MODE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				qsi_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ ccd.on_disconnect
		if (PRIVATE_DATA->exposure_active) {
			qsi_abort_exposure(device);
			PRIVATE_DATA->exposure_active = false;
		}
		indigo_ccd_failure_cleanup(device);
		CCD_EXPOSURE_ITEM->number.value = 0;
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		//- ccd.on_disconnect
		indigo_delete_property(device, X_QSI_READOUT_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, X_QSI_ANTI_BLOOM_PROPERTY, NULL);
		indigo_delete_property(device, X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
		indigo_delete_property(device, X_QSI_FAN_MODE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			qsi_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, ccd_timer_callback);
	}
}

static void ccd_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	int bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
	int bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;
	PRIVATE_DATA->image_width = (int)CCD_FRAME_WIDTH_ITEM->number.value / bin_x;
	PRIVATE_DATA->image_height = (int)CCD_FRAME_HEIGHT_ITEM->number.value / bin_y;
	bool light = !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value);
	PRIVATE_DATA->can_check_temperature = false;
	if (!QSI_CALL("put_StartX", cam.put_StartX((long)(CCD_FRAME_LEFT_ITEM->number.value / bin_x))) || !QSI_CALL("put_StartY", cam.put_StartY((long)(CCD_FRAME_TOP_ITEM->number.value / bin_y))) || !QSI_CALL("put_NumX", cam.put_NumX(PRIVATE_DATA->image_width)) || !QSI_CALL("put_NumY", cam.put_NumY(PRIVATE_DATA->image_height)) || !QSI_CALL("put_BinX", cam.put_BinX((short)bin_x)) || !QSI_CALL("put_BinY", cam.put_BinY((short)bin_y)) || !QSI_CALL("StartExposure", cam.StartExposure(CCD_EXPOSURE_ITEM->number.target, light))) {
		qsi_exposure_failed(device);
		return;
	}
	PRIVATE_DATA->exposure_active = true;
	PRIVATE_DATA->readout_deadline = indigo_monotonic_time() + CCD_EXPOSURE_ITEM->number.target + QSI_READOUT_TIMEOUT;
	indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, CCD_EXPOSURE_ITEM->number.target, exposure_finalizer); // exposure_finalizer publishes completion.
	//- ccd.CCD_EXPOSURE.on_change
}

static void ccd_abort_exposure_handler(indigo_device *device) {
	//+ ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, ccd_exposure_handler);
	indigo_cancel_pending_handler(device, exposure_finalizer);
	if (PRIVATE_DATA->exposure_active) {
		qsi_abort_exposure(device);
		PRIVATE_DATA->exposure_active = false;
	}
	PRIVATE_DATA->can_check_temperature = true;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_ccd_abort_exposure_cleanup(device);
	}
	//- ccd.CCD_ABORT_EXPOSURE.on_change
}

static void ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_COOLER.on_change
	if (!QSI_CALL("put_CoolerOn", cam.put_CoolerOn(CCD_COOLER_ON_ITEM->sw.value))) {
		CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_TEMPERATURE.on_change
	if (!CCD_COOLER_PROPERTY->hidden && CCD_COOLER_OFF_ITEM->sw.value && QSI_CALL("put_CoolerOn", cam.put_CoolerOn(true))) {
		indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
		CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
	}
	if (QSI_CALL("put_SetCCDTemperature", cam.put_SetCCDTemperature(CCD_TEMPERATURE_ITEM->number.target))) {
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

static void ccd_gain_handler(indigo_device *device) {
	CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.CCD_GAIN.on_change
	if (QSI_CALL("put_CameraGain", cam.put_CameraGain((QSICamera::CameraGain)(int)CCD_GAIN_ITEM->number.target))) {
		CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target;
		PRIVATE_DATA->gain = (int)CCD_GAIN_ITEM->number.value;
	} else {
		CCD_GAIN_ITEM->number.target = CCD_GAIN_ITEM->number.value;
		CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.CCD_GAIN.on_change
	indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
}

static void ccd_x_qsi_readout_speed_handler(indigo_device *device) {
	X_QSI_READOUT_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_QSI_READOUT_SPEED.on_change
	QSICamera::ReadoutSpeed readout_speed = X_QSI_READOUT_FAST_ITEM->sw.value ? QSICamera::FastReadout : QSICamera::HighImageQuality;
	if (QSI_CALL("put_ReadoutSpeed", cam.put_ReadoutSpeed(readout_speed))) {
		PRIVATE_DATA->readout_speed = (int)readout_speed;
	} else {
		X_QSI_READOUT_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.X_QSI_READOUT_SPEED.on_change
	indigo_update_property(device, X_QSI_READOUT_SPEED_PROPERTY, NULL);
}

static void ccd_x_qsi_anti_bloom_handler(indigo_device *device) {
	X_QSI_ANTI_BLOOM_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_QSI_ANTI_BLOOM.on_change
	QSICamera::AntiBloom anti_bloom = X_QSI_ANTI_BLOOM_HIGH_ITEM->sw.value ? QSICamera::AntiBloomHigh : QSICamera::AntiBloomNormal;
	if (QSI_CALL("put_AntiBlooming", cam.put_AntiBlooming(anti_bloom))) {
		PRIVATE_DATA->anti_bloom = (int)anti_bloom;
	} else {
		X_QSI_ANTI_BLOOM_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.X_QSI_ANTI_BLOOM.on_change
	indigo_update_property(device, X_QSI_ANTI_BLOOM_PROPERTY, NULL);
}

static void ccd_x_qsi_pre_exposure_flush_handler(indigo_device *device) {
	X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_QSI_PRE_EXPOSURE_FLUSH.on_change
	QSICamera::PreExposureFlush pre_exposure_flush = QSICamera::FlushNormal;
	if (X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM->sw.value) {
		pre_exposure_flush = QSICamera::FlushNone;
	} else if (X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM->sw.value) {
		pre_exposure_flush = QSICamera::FlushModest;
	} else if (X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM->sw.value) {
		pre_exposure_flush = QSICamera::FlushAggressive;
	} else if (X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM->sw.value) {
		pre_exposure_flush = QSICamera::FlushVeryAggressive;
	}
	if (QSI_CALL("put_PreExposureFlush", cam.put_PreExposureFlush(pre_exposure_flush))) {
		PRIVATE_DATA->pre_exposure_flush = (int)pre_exposure_flush;
	} else {
		X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.X_QSI_PRE_EXPOSURE_FLUSH.on_change
	indigo_update_property(device, X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
}

static void ccd_x_qsi_fan_mode_handler(indigo_device *device) {
	X_QSI_FAN_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ ccd.X_QSI_FAN_MODE.on_change
	QSICamera::FanMode fan_mode = QSICamera::fanQuiet;
	if (X_QSI_FAN_MODE_OFF_ITEM->sw.value) {
		fan_mode = QSICamera::fanOff;
	} else if (X_QSI_FAN_MODE_FULL_ITEM->sw.value) {
		fan_mode = QSICamera::fanFull;
	}
	if (QSI_CALL("put_FanMode", cam.put_FanMode(fan_mode))) {
		PRIVATE_DATA->fan_mode = (int)fan_mode;
	} else {
		X_QSI_FAN_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ccd.X_QSI_FAN_MODE.on_change
	indigo_update_property(device, X_QSI_FAN_MODE_PROPERTY, NULL);
}

#pragma mark - Device API (ccd)

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ ccd.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_NAME(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->serial);
		//- ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_GAIN_PROPERTY->hidden = false;
		X_QSI_READOUT_SPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, X_QSI_READOUT_SPEED_PROPERTY_NAME, CCD_ADVANCED_GROUP, "CCD readout speed", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (X_QSI_READOUT_SPEED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_QSI_READOUT_HQ_ITEM, X_QSI_READOUT_HQ_ITEM_NAME, "High Quality", false);
		indigo_init_switch_item(X_QSI_READOUT_FAST_ITEM, X_QSI_READOUT_FAST_ITEM_NAME, "Fast Readout", false);
		X_QSI_ANTI_BLOOM_PROPERTY = indigo_init_switch_property(NULL, device->name, X_QSI_ANTI_BLOOM_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Antiblooming", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (X_QSI_ANTI_BLOOM_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_QSI_ANTI_BLOOM_NORMAL_ITEM, X_QSI_ANTI_BLOOM_NORMAL_ITEM_NAME, "Normal", false);
		indigo_init_switch_item(X_QSI_ANTI_BLOOM_HIGH_ITEM, X_QSI_ANTI_BLOOM_HIGH_ITEM_NAME, "High", false);
		X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY = indigo_init_switch_property(NULL, device->name, X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY_NAME, CCD_ADVANCED_GROUP, "Pre-exposure flush", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 5);
		if (X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM, X_QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM_NAME, "Off", false);
		indigo_init_switch_item(X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM, X_QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM_NAME, "Modest", false);
		indigo_init_switch_item(X_QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM, X_QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM_NAME, "Normal", false);
		indigo_init_switch_item(X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM, X_QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM_NAME, "Aggressive", false);
		indigo_init_switch_item(X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM, X_QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM_NAME, "Very aggressive", false);
		X_QSI_FAN_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_QSI_FAN_MODE_PROPERTY_NAME, CCD_COOLER_GROUP, "Fan mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (X_QSI_FAN_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_QSI_FAN_MODE_OFF_ITEM, X_QSI_FAN_MODE_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(X_QSI_FAN_MODE_QUIET_ITEM, X_QSI_FAN_MODE_QUIET_ITEM_NAME, "Quiet", false);
		indigo_init_switch_item(X_QSI_FAN_MODE_FULL_ITEM, X_QSI_FAN_MODE_FULL_ITEM_NAME, "Full speed", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_QSI_READOUT_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_QSI_ANTI_BLOOM_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_QSI_FAN_MODE_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, ccd_exposure_handler);
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
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_GAIN_PROPERTY, ccd_gain_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_QSI_READOUT_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_QSI_READOUT_SPEED_PROPERTY, ccd_x_qsi_readout_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_QSI_ANTI_BLOOM_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_QSI_ANTI_BLOOM_PROPERTY, ccd_x_qsi_anti_bloom_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY, ccd_x_qsi_pre_exposure_flush_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_QSI_FAN_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_QSI_FAN_MODE_PROPERTY, ccd_x_qsi_fan_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_QSI_READOUT_SPEED_PROPERTY);
			indigo_save_property(device, NULL, X_QSI_ANTI_BLOOM_PROPERTY);
			indigo_save_property(device, NULL, X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY);
			indigo_save_property(device, NULL, X_QSI_FAN_MODE_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connection_handler(device);
	}
	indigo_release_property(X_QSI_READOUT_SPEED_PROPERTY);
	indigo_release_property(X_QSI_ANTI_BLOOM_PROPERTY);
	indigo_release_property(X_QSI_PRE_EXPOSURE_FLUSH_PROPERTY);
	indigo_release_property(X_QSI_FAN_MODE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = qsi_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ wheel.on_connect
			indigo_lock_master_device(device);
			snprintf(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_NAME_SIZE, "QSI %s", PRIVATE_DATA->model);
			int filter_count = PRIVATE_DATA->filter_count;
			connection_result = filter_count > 0 && filter_count <= WHEEL_SLOT_NAME_PROPERTY->allocated_count;
			if (!connection_result) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera #%s has no usable filter wheel", PRIVATE_DATA->serial);
			} else {
				short position = -1;
				connection_result = QSI_CALL("get_Position", cam.get_Position(&position));
				if (connection_result) {
					WHEEL_SLOT_ITEM->number.min = 1;
					WHEEL_SLOT_ITEM->number.max = filter_count;
					WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = filter_count;
					PRIVATE_DATA->wheel_slot = position >= 0 ? position + 1 : 1;
					WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->wheel_slot;
				}
			}
			indigo_unlock_master_device(device);
			//- wheel.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				qsi_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			qsi_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	int slot = (int)WHEEL_SLOT_ITEM->number.target;
	short position = -1;
	bool result = QSI_CALL("get_Position", cam.get_Position(&position));
	if (result && position == slot - 1) {
		PRIVATE_DATA->wheel_slot = slot;
		WHEEL_SLOT_ITEM->number.value = slot;
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else if (result && QSI_CALL("put_Position", cam.put_Position((short)(slot - 1)))) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->wheel_deadline = indigo_monotonic_time() + QSI_WHEEL_TIMEOUT;
		indigo_execute_handler_in(device, QSI_WHEEL_POLL_PERIOD, wheel_move_finalizer); // wheel_move_finalizer publishes completion.
	} else {
		WHEEL_SLOT_ITEM->number.target = WHEEL_SLOT_ITEM->number.value;
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	//- wheel.WHEEL_SLOT.on_change
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_NAME(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->serial);
		//- wheel.on_attach
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
		INDIGO_PROCESS_QUEUED_CONNECT(driver_queue, &driver_queue_mutex, wheel_connection_handler);
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

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static indigo_result verify_devices_disconnected(void) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		VERIFY_NOT_CONNECTED(devices[i]);
	}
	return INDIGO_OK;
}

#define SDK_DISCOVERY_RETRIES (6)
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
	qsi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = (qsi_private_data *)indigo_safe_malloc(sizeof(qsi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == QSI_VENDOR_ID)) {
		plug_result = false;
	}
	bool discovery_eligible = plug_result;
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		if (descriptor.idProduct == QSI_PRODUCT_ID1 || descriptor.idProduct == QSI_PRODUCT_ID2) {
			std::string serials[QSICamera::MAXCAMERAS];
			std::string descriptions[QSICamera::MAXCAMERAS];
			int count = 0;
			if (qsi_enumerate(serials, descriptions, &count)) {
				for (int i = 0; i < count; i++) {
					char serial[INDIGO_NAME_SIZE];
					INDIGO_COPY_NAME(serial, serials[i].c_str());
					bool duplicate = false;
					int available = 0;
					for (int j = 0; j < MAX_DEVICES; j++) {
						if (devices[j] == NULL) {
							available++;
						} else if (!strcmp(((qsi_private_data *)devices[j]->private_data)->serial, serial)) {
							duplicate = true;
						}
					}
					if (duplicate) {
						continue;
					}
					INDIGO_COPY_NAME(private_data->serial, serial);
					if (!qsi_probe(private_data)) {
						break;
					}
					if (available < 1 + (private_data->has_filter_wheel ? 1 : 0)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "No free device slot for camera #%s", serial);
						break;
					}
					char description[INDIGO_NAME_SIZE];
					INDIGO_COPY_NAME(description, descriptions[i].c_str());
					snprintf(name, INDIGO_NAME_SIZE, "%s #%s", description, serial);
					snprintf(private_data->wheel_name, INDIGO_NAME_SIZE, "%s (wheel) #%s", description, serial);
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
		if (ccd_attached && private_data->has_filter_wheel) {
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
	update_sdk_discovery_retry(dev, discovery_eligible && !dev_ref_transferred);
	if (!dev_ref_transferred) {
		indigo_safe_free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	update_sdk_discovery_retry(dev, false);
	qsi_private_data *private_data = NULL;
	qsi_private_data *removed[MAX_DEVICES];
	int removed_count = 0;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			private_data = PRIVATE_DATA;
			bool unplug_result = private_data->usbdev == dev;
			if (!unplug_result && last_action != INDIGO_DRIVER_SHUTDOWN) {
				//+ sdk.unplug_match
				std::string serials[QSICamera::MAXCAMERAS];
				std::string descriptions[QSICamera::MAXCAMERAS];
				int count = 0;
				unplug_result = false;
				if (qsi_enumerate(serials, descriptions, &count)) {
					bool found = false;
					for (int i = 0; i < count; i++) {
						if (!strcmp(serials[i].c_str(), private_data->serial)) {
							found = true;
							break;
						}
					}
					// A failed or incomplete enumeration is inconclusive, never proof
					// that the camera is gone.
					unplug_result = !found;
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

indigo_result indigo_ccd_qsi(indigo_driver_action action, indigo_driver_info *info) {

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			std::string info("");
			QSI_CALL("get_DriverInfo", cam.get_DriverInfo(info));
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "QSIAPI version: %s", info.c_str());
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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, QSI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
