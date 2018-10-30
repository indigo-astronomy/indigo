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


/** INDIGO CCD driver for Quantum Scientific Imaging
 \file indigo_ccd_qsi.cpp
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME		"indigo_ccd_qsi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "qsiapi.h"
#include "indigo_driver_xml.h"
#include "indigo_ccd_qsi.h"

#define QSI_VENDOR_ID             0x0403
#define QSI_PRODUCT_ID						0xEB48

#define PRIVATE_DATA              ((qsi_private_data *)device->private_data)

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
	indigo_device *wheel;
	int filter_count;
} qsi_private_data;

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static void wheel_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	try {
		short slot;
		cam.get_Position(&slot);
		if (slot == -1) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 0.5, wheel_timer_callback);
		} else {
			WHEEL_SLOT_ITEM->number.value = slot;
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		cam.get_LastError(last);
		WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Get position failed  %s %s", text.c_str(), last.c_str());
	}
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			assert(PRIVATE_DATA->filter_count > 0);
			WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->filter_count;
			try {
				short slot;
				cam.get_Position(&slot);
				WHEEL_SLOT_ITEM->number.value = slot;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				cam.get_LastError(last);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, "Connection failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		} else {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		try {
			if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				short slot;
				cam.get_Position(&slot);
				if (WHEEL_SLOT_ITEM->number.value == slot) {
					WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
					cam.put_Position(slot);
					indigo_set_timer(device, 0.5, wheel_timer_callback);
				}
			}
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			cam.get_LastError(last);
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, WHEEL_SLOT_PROPERTY, "Set position failed  %s %s", text.c_str(), last.c_str());
			return INDIGO_OK;
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
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
				usleep(5000);
				cam.get_ImageReady(&ready);
			}
			long width, height;
			cam.get_NumX(&width);
			cam.get_NumY(&height);
			cam.get_ImageArray(PRIVATE_DATA->buffer + FITS_HEADER_SIZE);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Image %ld x %ld", width, height);
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)width, (int)height, 16, false, NULL);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			cam.get_LastError(last);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed  %s %s", text.c_str(), last.c_str());
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
			CCD_TEMPERATURE_PROPERTY->state = fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			cam.get_CanGetCoolerPower(&canGetPower);
			if (canGetPower) {
				cam.get_CoolerPower(&CCD_COOLER_POWER_ITEM->number.value);
				CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			cam.get_LastError(last);
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Temperature check failed  %s %s", text.c_str(), last.c_str());
		}
	}
	indigo_reschedule_timer(device, 10, &PRIVATE_DATA->temperature_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->can_check_temperature = true;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' (%s) attached", device->name, PRIVATE_DATA->serial);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			try {
				std::string serial(PRIVATE_DATA->serial);
				std::string desc("");
				long width, height;
				double pixelWidth, pixelHeight;
				bool hasShutter, canSetTemp, hasFilterWheel, power2Binning;
				short maxBinX, maxBinY;
				char name[32], label[128];
				cam.put_SelectCamera(serial);
				cam.put_IsMainCamera(true);
				cam.put_Connected(true);
				cam.get_Description(desc);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s connected", desc.c_str());
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
					strcat(wheel->name, " (wheel)");
					wheel->private_data = PRIVATE_DATA;
					PRIVATE_DATA->wheel = wheel;
					indigo_async((void *(*)(void *))indigo_attach_device, wheel);
				} else {
					PRIVATE_DATA->filter_count = 0;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hasn't filter wheel");
					PRIVATE_DATA->wheel = NULL;
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
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_TEMPERATURE_ITEM->number.min = -60;
					CCD_TEMPERATURE_ITEM->number.max = 60;
					CCD_TEMPERATURE_ITEM->number.step = 0;
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_COOLER_ON_ITEM->sw.value = false;
					CCD_COOLER_OFF_ITEM->sw.value = true;
				}
				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, ccd_temperature_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				cam.get_LastError(last);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, "Connection failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			try {
				if (PRIVATE_DATA->wheel) {
					wheel_detach(PRIVATE_DATA->wheel);
					free(PRIVATE_DATA->wheel);
					PRIVATE_DATA->wheel = NULL;
				}
				cam.put_Connected(false);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				cam.get_LastError(last);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, "Disconnection failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		}
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_use_shortest_exposure_if_bias(device);
			try {
				cam.put_StartX(CCD_FRAME_LEFT_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
				cam.put_StartY(CCD_FRAME_TOP_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
				cam.put_NumX(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
				cam.put_NumY(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
				cam.put_BinX(CCD_BIN_HORIZONTAL_ITEM->number.value);
				cam.put_BinY(CCD_BIN_VERTICAL_ITEM->number.value);
				cam.StartExposure(CCD_EXPOSURE_ITEM->number.value, !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value));
				CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				cam.get_LastError(last);
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Start exposure failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		}
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			try {
				bool canAbort;
				cam.get_CanAbortExposure(&canAbort);
				if (canAbort) {
					cam.AbortExposure();
				}
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				cam.get_LastError(last);
				CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, "Abort exposure failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		try {
			cam.put_CoolerOn(CCD_COOLER_ON_ITEM->sw.value);
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			cam.get_LastError(last);
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, "Cooler setup failed  %s %s", text.c_str(), last.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		try {
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				cam.put_CoolerOn(true);
				CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			cam.put_SetCCDTemperature(CCD_TEMPERATURE_ITEM->number.value);
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			cam.get_LastError(last);
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Temperature setup failed  %s %s", text.c_str(), last.c_str());
		}
		return INDIGO_OK;
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' (%s) detached.", device->name, PRIVATE_DATA->serial);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *devices[QSICamera::MAXCAMERAS];

static void process_plug_event(indigo_device *unused) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
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
	sleep(3);
	try {
		cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		cam.get_LastError(last);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot plug failed  %s %s", text.c_str(), last.c_str());
		pthread_mutex_unlock(&device_mutex);
		return;
	}
	for (int i = 0; i < count; i++) {
		strncpy(serial, camSerial[i].c_str(), INDIGO_NAME_SIZE);
		strncpy(desc, camDesc[i].c_str(), INDIGO_NAME_SIZE);
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
		if (found)
			continue;
		qsi_private_data *private_data = (qsi_private_data *)malloc(sizeof(qsi_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(qsi_private_data));
		strncpy(private_data->serial, serial, INDIGO_NAME_SIZE);
		indigo_device *device = (indigo_device *)malloc(sizeof(indigo_device));
		assert(device != NULL);
		memcpy(device, &ccd_template, sizeof(indigo_device));
		strncpy(device->name, desc, INDIGO_NAME_SIZE);
		device->private_data = private_data;
		for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
			if (devices[j] == NULL) {
				indigo_async((void *(*)(void *))indigo_attach_device, devices[j] = device);
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
	sleep(3);
	try {
		cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		cam.get_LastError(last);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot unplug failed  %s %s", text.c_str(), last.c_str());
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
		strncpy(serial, camSerial[i].c_str(), INDIGO_NAME_SIZE);
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
			indigo_set_timer(NULL, 0.5, process_plug_event);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot-unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
			indigo_set_timer(NULL, 0.5, process_unplug_event);
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

static libusb_hotplug_callback_handle callback_handle;

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
			int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, QSI_VENDOR_ID, QSI_PRODUCT_ID, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
		}
		case INDIGO_DRIVER_SHUTDOWN: {
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
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
