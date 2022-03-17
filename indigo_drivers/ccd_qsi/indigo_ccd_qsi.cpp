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
//
// 23.08.2020 by Rumen.G.Bogdanovski <rumen@skyarchive.org>
// - major rework: made driver stable with access to QSI camera

/** INDIGO CCD driver for Quantum Scientific Imaging
 \file indigo_ccd_qsi.cpp
 */

#define DRIVER_VERSION    0x000C
#define DRIVER_NAME       "indigo_ccd_qsi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_ccd_qsi.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include "qsiapi.h"

#define QSI_VENDOR_ID             0x0403
#define QSI_PRODUCT_ID1           0xEB48
#define QSI_PRODUCT_ID2           0xEB49

#define QSI_READOUT_SPEED_PROPERTY                       (PRIVATE_DATA->qsi_readout_speed_property)
#define QSI_READOUT_SPEED_PROPERTY_NAME                  "QSI_READOUT_SPEED"
#define QSI_READOUT_HQ_ITEM                              (QSI_READOUT_SPEED_PROPERTY->items+0)
#define QSI_READOUT_HQ_ITEM_NAME                         "HIGH_QUALITY"
#define QSI_READOUT_FAST_ITEM                            (QSI_READOUT_SPEED_PROPERTY->items+1)
#define QSI_READOUT_FAST_ITEM_NAME                       "FAST_READOUT"

#define QSI_ANTI_BLOOM_PROPERTY                          (PRIVATE_DATA->qsi_anti_bloom_property)
#define QSI_ANTI_BLOOM_PROPERTY_NAME                     "QSI_ANTI_BLOOM"
#define QSI_ANTI_BLOOM_NORMAL_ITEM                       (QSI_ANTI_BLOOM_PROPERTY->items+0)
#define QSI_ANTI_BLOOM_NORMAL_ITEM_NAME                  "NORMAL"
#define QSI_ANTI_BLOOM_HIGH_ITEM                         (QSI_ANTI_BLOOM_PROPERTY->items+1)
#define QSI_ANTI_BLOOM_HIGH_ITEM_NAME                    "HIGH"

#define QSI_PRE_EXPOSURE_FLUSH_PROPERTY                  (PRIVATE_DATA->qsi_pre_exposure_flush_property)
#define QSI_PRE_EXPOSURE_FLUSH_PROPERTY_NAME             "QSI_PRE_EXPOSURE_FLUSH"
#define QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM                 (QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items+0)
#define QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM_NAME            "NONE"
#define QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM               (QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items+1)
#define QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM_NAME          "MODEST"
#define QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM               (QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items+2)
#define QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM_NAME          "NORMAL"
#define QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM           (QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items+3)
#define QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM_NAME      "AGGRESSIVE"
#define QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM         (QSI_PRE_EXPOSURE_FLUSH_PROPERTY->items+4)
#define QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM_NAME    "VERY_AGGRESSIVE"

#define QSI_FAN_MODE_PROPERTY                            (PRIVATE_DATA->qsi_fan_mode_property)
#define QSI_FAN_MODE_PROPERTY_NAME                       "QSI_FAN_MODE"
#define QSI_FAN_MODE_OFF_ITEM                            (QSI_FAN_MODE_PROPERTY->items+0)
#define QSI_FAN_MODE_OFF_ITEM_NAME                       "OFF"
#define QSI_FAN_MODE_QUIET_ITEM                          (QSI_FAN_MODE_PROPERTY->items+1)
#define QSI_FAN_MODE_QUIET_ITEM_NAME                     "QUIET"
#define QSI_FAN_MODE_FULL_ITEM                           (QSI_FAN_MODE_PROPERTY->items+2)
#define QSI_FAN_MODE_FULL_ITEM_NAME                      "FULL_SPEED"

#define PRIVATE_DATA                                     ((qsi_private_data *)device->private_data)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

static QSICamera cam;

typedef struct {
	char serial[INDIGO_NAME_SIZE];
	bool available;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	unsigned short *buffer;
	bool can_check_temperature;
	bool wheel_attached;
	indigo_device *wheel;
	int filter_count;
	indigo_property *qsi_readout_speed_property,
	                *qsi_anti_bloom_property,
	                *qsi_pre_exposure_flush_property,
	                *qsi_fan_mode_property;
} qsi_private_data;

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->wheel_attached = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_slot_callback(indigo_device *device) {
	try {
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			short slot;
			cam.get_Position(&slot);
			if (WHEEL_SLOT_ITEM->number.value - 1 == slot) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			} else {
				WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				cam.put_Position(WHEEL_SLOT_ITEM->number.value - 1);
				cam.get_Position(&slot);
				while(slot == -1) {
					indigo_usleep(100000);
					cam.get_Position(&slot);
				}
				WHEEL_SLOT_ITEM->number.value = slot + 1;
				WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
			}
		}
	} catch (std::runtime_error err) {
		std::string text = err.what();
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, text.c_str());
	}
}

static void wheel_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		assert(PRIVATE_DATA->filter_count > 0);
		WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->filter_count;
		try {
			short slot;
			cam.get_Position(&slot);
			WHEEL_SLOT_ITEM->number.value = slot + 1;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, "Connection failed: %s", text.c_str());
			return;
		}
	} else {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		indigo_set_timer(device, 0, wheel_slot_callback, NULL);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (!PRIVATE_DATA->wheel_attached) return INDIGO_FAILED;
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	PRIVATE_DATA->wheel_attached = false;
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (!IS_CONNECTED)
		return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		try {
			bool ready = false;
			cam.get_ImageReady(&ready);
			while (!ready) {
				indigo_usleep(5000);
				cam.get_ImageReady(&ready);
			}
			int width, height, depth;
			cam.get_ImageArraySize(width, height, depth);
			cam.get_ImageArray(PRIVATE_DATA->buffer + FITS_HEADER_SIZE / 2);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Image %ld x %ld", width, height);
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)width, (int)height, 16, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, text.c_str());
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	if (PRIVATE_DATA->can_check_temperature) {
		try {
			bool canGetPower;
			cam.get_CCDTemperature(&CCD_TEMPERATURE_ITEM->number.value);
			CCD_TEMPERATURE_PROPERTY->state = fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 0.2 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			cam.get_CanGetCoolerPower(&canGetPower);
			if (canGetPower) {
				cam.get_CoolerPower(&CCD_COOLER_POWER_ITEM->number.value);
				CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, text.c_str());
		}
	}
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}

static void ccd_exposure_callback(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_use_shortest_exposure_if_bias(device);
		try {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			cam.put_StartX(CCD_FRAME_LEFT_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
			cam.put_StartY(CCD_FRAME_TOP_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
			cam.put_NumX(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
			cam.put_NumY(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
			cam.put_BinX(CCD_BIN_HORIZONTAL_ITEM->number.value);
			cam.put_BinY(CCD_BIN_VERTICAL_ITEM->number.value);
			cam.StartExposure(CCD_EXPOSURE_ITEM->number.value, !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value));
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, text.c_str());
		}
	}
	indigo_ccd_change_property(device, NULL, CCD_EXPOSURE_PROPERTY);
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(QSI_READOUT_SPEED_PROPERTY, property))
			indigo_define_property(device, QSI_READOUT_SPEED_PROPERTY, NULL);
		if (indigo_property_match(QSI_ANTI_BLOOM_PROPERTY, property))
			indigo_define_property(device, QSI_ANTI_BLOOM_PROPERTY, NULL);
		if (indigo_property_match(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, property))
			indigo_define_property(device, QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
		if (indigo_property_match(QSI_FAN_MODE_PROPERTY, property))
			indigo_define_property(device, QSI_FAN_MODE_PROPERTY, NULL);
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->can_check_temperature = true;
		INFO_PROPERTY->count = 8;
		snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_NAME_SIZE, "%s", PRIVATE_DATA->serial);
		// -------------------------------------------------------------------------------- QSI_READOUT_SPEED
		QSI_READOUT_SPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, QSI_READOUT_SPEED_PROPERTY_NAME, "Advanced", "CCD readout speed", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (QSI_READOUT_SPEED_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(QSI_READOUT_HQ_ITEM, QSI_READOUT_HQ_ITEM_NAME, "High Quality", false);
		indigo_init_switch_item(QSI_READOUT_FAST_ITEM, QSI_READOUT_FAST_ITEM_NAME, "Fast Readout", false);
		// -------------------------------------------------------------------------------- QSI_ANTI_BLOOM
		QSI_ANTI_BLOOM_PROPERTY = indigo_init_switch_property(NULL, device->name, QSI_ANTI_BLOOM_PROPERTY_NAME, "Advanced", "Antiblooming", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (QSI_ANTI_BLOOM_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(QSI_ANTI_BLOOM_NORMAL_ITEM, QSI_ANTI_BLOOM_NORMAL_ITEM_NAME, "Normal", false);
		indigo_init_switch_item(QSI_ANTI_BLOOM_HIGH_ITEM, QSI_ANTI_BLOOM_HIGH_ITEM_NAME, "High", false);
		// -------------------------------------------------------------------------------- QSI_PRE_EXPOSURE_FLUSH
		QSI_PRE_EXPOSURE_FLUSH_PROPERTY = indigo_init_switch_property(NULL, device->name, QSI_PRE_EXPOSURE_FLUSH_PROPERTY_NAME, "Advanced", "Pre-exposure flush", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 5);
		if (QSI_PRE_EXPOSURE_FLUSH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM, QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM_NAME, "Off", false);
		indigo_init_switch_item(QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM, QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM_NAME, "Modest", false);
		indigo_init_switch_item(QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM, QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM_NAME, "Normal", false);
		indigo_init_switch_item(QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM, QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM_NAME, "Aggressive", false);
		indigo_init_switch_item(QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM, QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM_NAME, "Verry aggressive", false);
		// -------------------------------------------------------------------------------- QSI_FAN_MODE
		QSI_FAN_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, QSI_FAN_MODE_PROPERTY_NAME, CCD_COOLER_GROUP, "Fan mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (QSI_FAN_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(QSI_FAN_MODE_OFF_ITEM, QSI_FAN_MODE_OFF_ITEM_NAME, "Off", false);
		indigo_init_switch_item(QSI_FAN_MODE_QUIET_ITEM, QSI_FAN_MODE_QUIET_ITEM_NAME, "Quiet", false);
		indigo_init_switch_item(QSI_FAN_MODE_FULL_ITEM, QSI_FAN_MODE_FULL_ITEM_NAME, "Full speed", false);

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		try {
			std::string serial(PRIVATE_DATA->serial);
			std::string selectedCamera("");
			std::string modelNumber("");
			std::string driverInfo("");
			long width, height;
			double pixelWidth, pixelHeight;
			bool hasShutter, canSetTemp, canGetCoolerPower, canSetGain, hasFilterWheel, power2Binning, isConnected;
			short maxBinX, maxBinY;
			char name[32], label[128];

			// Second connect is not allowed by the SDK, so handle this nicely
			cam.get_Connected(&isConnected);
			if (isConnected) {
				cam.get_SelectCamera(selectedCamera);
				char message[INDIGO_VALUE_SIZE];
				snprintf(message, INDIGO_VALUE_SIZE, "Camera #%s is already connected, to use #%s disconnect it first.", selectedCamera.c_str(), PRIVATE_DATA->serial);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, message);
				indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
				return;
			}
			cam.put_SelectCamera(serial);
			cam.put_IsMainCamera(true);
			cam.put_Connected(true);
			cam.get_ModelNumber(modelNumber);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (Model: %s) connected.", device->name, modelNumber.c_str());
			snprintf(INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_NAME_SIZE, "QSI %s", modelNumber.c_str());
			indigo_update_property(device, INFO_PROPERTY, NULL);
			cam.get_CameraXSize(&width);
			cam.get_CameraYSize(&height);
			cam.get_PixelSizeX(&pixelWidth);
			cam.get_PixelSizeY(&pixelHeight);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Resolution %ld x %ld, pixel size  %g x %g", width, height, pixelWidth, pixelHeight);
			PRIVATE_DATA->buffer = (unsigned short *)indigo_alloc_blob_buffer(2 * width * height + FITS_HEADER_SIZE);
			assert(PRIVATE_DATA->buffer != NULL);
			cam.get_CanSetCCDTemperature(&canSetTemp);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s set temperature", canSetTemp ? "Can" : "Can't");
			cam.get_HasShutter(&hasShutter);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s shutter", hasShutter ? "Has" : "Hasn't");
			cam.get_MinExposureTime(&CCD_EXPOSURE_ITEM->number.min);
			cam.get_MaxExposureTime(&CCD_EXPOSURE_ITEM->number.max);
			cam.get_HasFilterWheel(&hasFilterWheel);
			if (hasFilterWheel) {
				cam.get_FilterCount(PRIVATE_DATA->filter_count);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Has filter wheel with %ld positions", PRIVATE_DATA->filter_count);
				static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
					"",
					wheel_attach,
					indigo_wheel_enumerate_properties,
					wheel_change_property,
					NULL,
					wheel_detach
				);
				indigo_device *wheel = (indigo_device *)malloc(sizeof(indigo_device));
				assert(wheel != NULL);
				memcpy(wheel, &wheel_template, sizeof(indigo_device));
				strncpy(wheel->name, device->name, INDIGO_NAME_SIZE - 10);
				char *p = strrchr(wheel->name, '#');
				if (p != NULL) *p = 0;
				strcat(wheel->name, "(wheel)");
				strcat(wheel->name, " #");
				strcat(wheel->name, PRIVATE_DATA->serial);
				wheel->private_data = PRIVATE_DATA;
				PRIVATE_DATA->wheel = wheel;
				//indigo_async((void *(*)(void *))indigo_attach_device, PRIVATE_DATA->wheel);
				indigo_attach_device(PRIVATE_DATA->wheel);
			} else {
				PRIVATE_DATA->filter_count = 0;
				PRIVATE_DATA->wheel_attached = false;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hasn't filter wheel");
				PRIVATE_DATA->wheel = NULL;
			}
			cam.get_CanSetGain(&canSetGain);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s set gain", canSetGain ? "Can" : "Can't");
			if (canSetGain) {
				QSICamera::CameraGain gain;
				CCD_GAIN_PROPERTY->hidden = false;
				CCD_GAIN_ITEM->number.min = 0;
				CCD_GAIN_ITEM->number.max = 2;
				cam.get_CameraGain(&gain);
				CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = (double)gain;
			}
			cam.get_MaxBinX(&maxBinX);
			cam.get_MaxBinY(&maxBinY);
			cam.get_PowerOfTwoBinning(&power2Binning);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Max binning %d x %d %s", maxBinX, maxBinY, power2Binning ? ", power2Binning" : "");

			CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = width;
			CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = height;
			CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_SIZE_ITEM->number.value = pixelWidth;
			CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = pixelHeight;
			CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = maxBinX;
			CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = maxBinY;
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 16;

			QSICamera::FanMode fanMode;
			cam.get_FanMode(fanMode);
			switch (fanMode) {
				case QSICamera::fanOff:
					indigo_set_switch(QSI_FAN_MODE_PROPERTY, QSI_FAN_MODE_OFF_ITEM, true);
					break;
				case QSICamera::fanQuiet:
					indigo_set_switch(QSI_FAN_MODE_PROPERTY, QSI_FAN_MODE_QUIET_ITEM, true);
					break;
				case QSICamera::fanFull:
					indigo_set_switch(QSI_FAN_MODE_PROPERTY, QSI_FAN_MODE_FULL_ITEM, true);
					break;
				default:
					QSI_FAN_MODE_PROPERTY->hidden = true;
					break;
			}
			indigo_define_property(device, QSI_FAN_MODE_PROPERTY, NULL);

			int maxBin = maxBinX > maxBinY ? maxBinY : maxBinX;
			CCD_MODE_PROPERTY->count = 0;
			for (int bin = 1; bin <= maxBin; bin = power2Binning ? (bin * 2) : (bin + 1)) {
				sprintf(label, "RAW 16 %dx%d", (int)(width / bin), (int)(height / bin));
				sprintf(name, "BIN_%dx%d", bin, bin);
				indigo_init_switch_item(CCD_MODE_ITEM + (CCD_MODE_PROPERTY->count++), name, label, bin == 1);
			}
			CCD_BIN_PROPERTY->perm = CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
			CCD_BIN_HORIZONTAL_ITEM->number.max = maxBinX;
			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
			CCD_BIN_VERTICAL_ITEM->number.max = maxBinY;
			if (canSetTemp) {
				bool coolerOn;
				CCD_TEMPERATURE_PROPERTY->hidden = false;
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_TEMPERATURE_ITEM->number.min = -60;
				CCD_TEMPERATURE_ITEM->number.max = 60;
				CCD_TEMPERATURE_ITEM->number.step = 1;
				CCD_COOLER_PROPERTY->hidden = false;
				CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
				cam.get_CoolerOn(&coolerOn);
				if (coolerOn) {
					CCD_COOLER_ON_ITEM->sw.value = true;
					CCD_COOLER_OFF_ITEM->sw.value = false;
				} else {
					CCD_COOLER_ON_ITEM->sw.value = false;
					CCD_COOLER_OFF_ITEM->sw.value = true;
				}
			}
			cam.get_CanGetCoolerPower(&canGetCoolerPower);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s get cooler power", canGetCoolerPower ? "Can" : "Can't");
			if (canGetCoolerPower) {
				CCD_COOLER_POWER_PROPERTY->hidden = false;
			}

			QSICamera::ReadoutSpeed readoutSpeed;
			cam.get_ReadoutSpeed(readoutSpeed);
			switch (readoutSpeed) {
				case QSICamera::HighImageQuality:
					indigo_set_switch(QSI_READOUT_SPEED_PROPERTY, QSI_READOUT_HQ_ITEM, true);
					break;
				case QSICamera::FastReadout:
					indigo_set_switch(QSI_READOUT_SPEED_PROPERTY, QSI_READOUT_FAST_ITEM, true);
					break;
				default:
					QSI_READOUT_SPEED_PROPERTY->hidden = true;
					break;
			}
			indigo_define_property(device, QSI_READOUT_SPEED_PROPERTY, NULL);

			QSICamera::AntiBloom antiBloom;
			cam.get_AntiBlooming(&antiBloom);
			switch (antiBloom) {
				case QSICamera::AntiBloomNormal:
					indigo_set_switch(QSI_ANTI_BLOOM_PROPERTY, QSI_ANTI_BLOOM_NORMAL_ITEM, true);
					break;
				case QSICamera::AntiBloomHigh:
					indigo_set_switch(QSI_ANTI_BLOOM_PROPERTY, QSI_ANTI_BLOOM_HIGH_ITEM, true);
					break;
				default:
					QSI_ANTI_BLOOM_PROPERTY->hidden = true;
					break;
			}
			indigo_define_property(device, QSI_ANTI_BLOOM_PROPERTY, NULL);

			QSICamera::PreExposureFlush peFlush;
			cam.get_PreExposureFlush(&peFlush);
			switch (peFlush) {
				case QSICamera::FlushNone:
					indigo_set_switch(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM, true);
					break;
				case QSICamera::FlushModest:
					indigo_set_switch(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM, true);
					break;
				case QSICamera::FlushNormal:
					indigo_set_switch(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM, true);
					break;
				case QSICamera::FlushAggressive:
					indigo_set_switch(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM, true);
					break;
				case QSICamera::FlushVeryAggressive:
					indigo_set_switch(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM, true);
					break;
				default:
					QSI_PRE_EXPOSURE_FLUSH_PROPERTY->hidden = true;
					break;
			}
			indigo_define_property(device, QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);

			indigo_set_timer(device, 0, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} catch (std::runtime_error err) {
			std::string text = err.what();
			indigo_send_message(device, text.c_str());
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
		indigo_delete_property(device, QSI_READOUT_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, QSI_ANTI_BLOOM_PROPERTY, NULL);
		indigo_delete_property(device, QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
		indigo_delete_property(device, QSI_FAN_MODE_PROPERTY, NULL);
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			try {
				bool canAbort;
				cam.get_CanAbortExposure(&canAbort);
				if (canAbort) {
					indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
					cam.AbortExposure();
				}
			} catch (std::runtime_error err) {
				std::string text = err.what();
				indigo_send_message(device, text.c_str());
			}
		}
		try {
			if (PRIVATE_DATA->wheel) {
				if (indigo_detach_device(PRIVATE_DATA->wheel) == INDIGO_OK) {
					free(PRIVATE_DATA->wheel);
					PRIVATE_DATA->wheel = NULL;
				}
			}
			cam.put_Connected(false);
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} catch (std::runtime_error err) {
			std::string text = err.what();
			indigo_send_message(device, "Disconnect failed: %s", text.c_str());
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		//cam.StartExposure() may take up to 10 secinds to return, so it should be aync
		indigo_set_timer(device, 0, ccd_exposure_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			try {
				bool canAbort;
				cam.get_CanAbortExposure(&canAbort);
				if (canAbort) {
					indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
					cam.AbortExposure();
				}
				PRIVATE_DATA->can_check_temperature = true;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, text.c_str());
				return INDIGO_OK;
			}
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		try {
			cam.put_CoolerOn(CCD_COOLER_ON_ITEM->sw.value);
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		double temperature = CCD_TEMPERATURE_ITEM->number.value;
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		CCD_TEMPERATURE_ITEM->number.value = temperature;
		try {
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				cam.put_CoolerOn(true);
				CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			cam.put_SetCCDTemperature(CCD_TEMPERATURE_ITEM->number.target);
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", CCD_TEMPERATURE_ITEM->number.target);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		try {
			int gain = (int)CCD_GAIN_ITEM->number.value;
			cam.put_CameraGain((QSICamera::CameraGain)gain);
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, "Gain = %d", gain);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			CCD_GAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(QSI_READOUT_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- QSI_READOUT_SPEED
		indigo_property_copy_values(QSI_READOUT_SPEED_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		QSICamera::ReadoutSpeed requestedSpeed = QSICamera::HighImageQuality;
		if (QSI_READOUT_HQ_ITEM->sw.value) {
			requestedSpeed = QSICamera::HighImageQuality;
		} else if (QSI_READOUT_FAST_ITEM->sw.value) {
			requestedSpeed = QSICamera::FastReadout;
		}
		try {
			cam.put_ReadoutSpeed(requestedSpeed);
			QSI_READOUT_SPEED_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cam.put_ReadoutSpeed(%d)", requestedSpeed);
			indigo_update_property(device, QSI_READOUT_SPEED_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			QSI_READOUT_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QSI_READOUT_SPEED_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(QSI_ANTI_BLOOM_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- QSI_ANTI_BLOOM
		indigo_property_copy_values(QSI_ANTI_BLOOM_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		QSICamera::AntiBloom requestedAntiBloom = QSICamera::AntiBloomNormal;
		if (QSI_ANTI_BLOOM_NORMAL_ITEM->sw.value) {
			requestedAntiBloom = QSICamera::AntiBloomNormal;
		} else if (QSI_ANTI_BLOOM_HIGH_ITEM->sw.value) {
			requestedAntiBloom = QSICamera::AntiBloomHigh;
		}
		try {
			cam.put_AntiBlooming(requestedAntiBloom);
			QSI_ANTI_BLOOM_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cam.put_AntiBlooming(%d)", requestedAntiBloom);
			indigo_update_property(device, QSI_ANTI_BLOOM_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			QSI_ANTI_BLOOM_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QSI_ANTI_BLOOM_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- QSI_PRE_EXPOSURE_FLUSH
		indigo_property_copy_values(QSI_PRE_EXPOSURE_FLUSH_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		QSICamera::PreExposureFlush requestedPEFlush = QSICamera::FlushNormal;
		if (QSI_PRE_EXPOSURE_FLUSH_NONE_ITEM->sw.value) {
			requestedPEFlush = QSICamera::FlushNone;
		} else if (QSI_PRE_EXPOSURE_FLUSH_MODEST_ITEM->sw.value) {
			requestedPEFlush = QSICamera::FlushModest;
		} else if (QSI_PRE_EXPOSURE_FLUSH_NORMAL_ITEM->sw.value) {
			requestedPEFlush = QSICamera::FlushNormal;
		} else if (QSI_PRE_EXPOSURE_FLUSH_AGGRESSIVE_ITEM->sw.value) {
			requestedPEFlush = QSICamera::FlushAggressive;
		} else if (QSI_PRE_EXPOSURE_FLUSH_V_AGGRESSIVE_ITEM->sw.value) {
			requestedPEFlush = QSICamera::FlushVeryAggressive;
		}
		try {
			cam.put_PreExposureFlush(requestedPEFlush);
			QSI_PRE_EXPOSURE_FLUSH_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cam.put_PreExposureFlush(%d)", requestedPEFlush);
			indigo_update_property(device, QSI_PRE_EXPOSURE_FLUSH_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			QSI_PRE_EXPOSURE_FLUSH_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QSI_PRE_EXPOSURE_FLUSH_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(QSI_FAN_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- QSI_FAN_MODE
		indigo_property_copy_values(QSI_FAN_MODE_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		QSICamera::FanMode requestedFanMode = QSICamera::fanQuiet;
		if (QSI_FAN_MODE_OFF_ITEM->sw.value) {
			requestedFanMode = QSICamera::fanOff;
		} else if (QSI_FAN_MODE_QUIET_ITEM->sw.value) {
			requestedFanMode = QSICamera::fanQuiet;
		} else if (QSI_FAN_MODE_FULL_ITEM->sw.value) {
			requestedFanMode = QSICamera::fanFull;
		}
		try {
			cam.put_FanMode(requestedFanMode);
			QSI_FAN_MODE_PROPERTY->state = INDIGO_OK_STATE;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cam.put_FanMode(%d)", requestedFanMode);
			indigo_update_property(device, QSI_FAN_MODE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			QSI_FAN_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, QSI_FAN_MODE_PROPERTY, text.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, QSI_READOUT_SPEED_PROPERTY);
			indigo_save_property(device, NULL, QSI_ANTI_BLOOM_PROPERTY);
			indigo_save_property(device, NULL, QSI_PRE_EXPOSURE_FLUSH_PROPERTY);
			indigo_save_property(device, NULL, QSI_FAN_MODE_PROPERTY);
		}
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	indigo_release_property(QSI_READOUT_SPEED_PROPERTY);
	indigo_release_property(QSI_ANTI_BLOOM_PROPERTY);
	indigo_release_property(QSI_PRE_EXPOSURE_FLUSH_PROPERTY);
	indigo_release_property(QSI_FAN_MODE_PROPERTY);

	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *devices[QSICamera::MAXCAMERAS];

static void process_plug_event(indigo_device *unused) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	std::string camSerial[QSICamera::MAXCAMERAS];
	std::string camDesc[QSICamera::MAXCAMERAS];
	char serial[INDIGO_NAME_SIZE];
	char desc[INDIGO_NAME_SIZE];
	int count;
	pthread_mutex_lock(&device_mutex);
	indigo_usleep(1 * ONE_SECOND_DELAY);
	try {
		cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot plug failed: %s", text.c_str());
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	for (int i = 0; i < count; i++) {
		indigo_copy_name(serial, camSerial[i].c_str());
		indigo_copy_name(desc, camDesc[i].c_str());
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera[%d]: desc = %s serial = %s", i, desc, serial);
		bool found = false;
		for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
			indigo_device *device = devices[j];
			if (device) {
				if (!strcmp(serial, PRIVATE_DATA->serial)) {
					found = true;
					break;
				}
			}
		}
		if (found) {
			continue;
		}
		qsi_private_data *private_data = (qsi_private_data *)indigo_safe_malloc(sizeof(qsi_private_data));
		indigo_copy_name(private_data->serial, serial);
		indigo_device *device = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		snprintf(device->name, INDIGO_NAME_SIZE, "%s #%s", desc, serial);
		device->private_data = private_data;
		for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
			if (devices[j] == NULL) {
				indigo_attach_device(devices[j] = device);
				break;
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	std::string camSerial[QSICamera::MAXCAMERAS];
	std::string camDesc[QSICamera::MAXCAMERAS];
	char serial[INDIGO_NAME_SIZE];
	int count;
	pthread_mutex_lock(&device_mutex);
	indigo_usleep(1 * ONE_SECOND_DELAY);
	try {
		cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot unplug failed: %s", text.c_str());
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device) {
			PRIVATE_DATA->available = false;
		}
	}
	for (int i = 0; i < count; i++) {
		indigo_copy_name(serial, camSerial[i].c_str());
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera[%d]: serial = %s", i, serial);
		for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
			indigo_device *device = devices[j];
			if (!device || strcmp(serial, PRIVATE_DATA->serial))
				continue;
			PRIVATE_DATA->available = true;
			break;
		}
	}
	for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device && !PRIVATE_DATA->available) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
			devices[j] = NULL;
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	libusb_get_device_descriptor(dev, &descriptor);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot-plug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 0.1, process_plug_event, NULL);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot-unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 0.1, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};

static void remove_all_devices() {
	for (int i = 0; i < QSICamera::MAXCAMERAS; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		if (((qsi_private_data *)(*device)->private_data)->buffer)
			free(((qsi_private_data *)(*device)->private_data)->buffer);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}

static libusb_hotplug_callback_handle callback_handle1;
static libusb_hotplug_callback_handle callback_handle2;

indigo_result indigo_ccd_qsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "QSI Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			for (int i = 0; i < QSICamera::MAXCAMERAS; i++) {
				devices[i] = NULL;
			}
			std::string info("");
			cam.get_DriverInfo(info);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "QSIAPI version: %s", info.c_str());
			last_action = action;
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, QSI_VENDOR_ID, QSI_PRODUCT_ID1, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle1);
			if (rc >= 0)
				rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, QSI_VENDOR_ID, QSI_PRODUCT_ID2, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle2);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
		}
		case INDIGO_DRIVER_SHUTDOWN: {
			for (int i = 0; i < QSICamera::MAXCAMERAS; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle1);
			libusb_hotplug_deregister_callback(NULL, callback_handle2);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			remove_all_devices();
			break;
		}
		case INDIGO_DRIVER_INFO: {
			break;
		}
	}

	return INDIGO_OK;
}

#else

indigo_result indigo_ccd_qsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "QSI Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	switch(action) {
		case INDIGO_DRIVER_INIT:
		case INDIGO_DRIVER_SHUTDOWN:
			return INDIGO_UNSUPPORTED_ARCH;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}

#endif
