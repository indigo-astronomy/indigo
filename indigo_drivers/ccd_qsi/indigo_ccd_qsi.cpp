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

#define DRIVER_VERSION 0x0001
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

typedef struct {
	char serial[INDIGO_NAME_SIZE];
	bool available;
	QSICamera *cam;
	int count_open;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	unsigned short *buffer;
	pthread_mutex_t usb_mutex;
} qsi_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (!IS_CONNECTED)
		return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		try {
			bool ready = false;
			PRIVATE_DATA->cam->get_ImageReady(&ready);
			while (!ready) {
				usleep(5000);
				PRIVATE_DATA->cam->get_ImageReady(&ready);
			}
			long width, height;
			PRIVATE_DATA->cam->get_NumX(&width);
			PRIVATE_DATA->cam->get_NumY(&height);
			PRIVATE_DATA->cam->get_ImageArray(PRIVATE_DATA->buffer);
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)width, (int)height, false, NULL);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			PRIVATE_DATA->cam->get_LastError(last);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed  %s %s", text.c_str(), last.c_str());
		}
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	try {
		bool canGetPower;
		PRIVATE_DATA->cam->get_CCDTemperature(&CCD_TEMPERATURE_ITEM->number.value);
		CCD_TEMPERATURE_PROPERTY->state = fabs(CCD_TEMPERATURE_ITEM->number.value - CCD_TEMPERATURE_ITEM->number.target) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		PRIVATE_DATA->cam->get_CanGetCoolerPower(&canGetPower);
		if (canGetPower) {
			PRIVATE_DATA->cam->get_CoolerPower(&CCD_COOLER_POWER_ITEM->number.value);
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		}
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		PRIVATE_DATA->cam->get_LastError(last);
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Temperature check failed  %s %s", text.c_str(), last.c_str());
	}
	indigo_reschedule_timer(device, 10, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
		PRIVATE_DATA->cam = new QSICamera();
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s (%s) attached", device->name, PRIVATE_DATA->serial);
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
				std::string desc("");
				long width, height;
				double pixelWidth, pixelHeight;
				bool hasShutter, canSetTemp, hasFilterWheel, power2Binning;
				short maxBinX, maxBinY;
				int filterCount;
				char name[32], label[128];
				
				PRIVATE_DATA->cam->put_IsMainCamera(true);
				PRIVATE_DATA->cam->put_Connected(true);
				PRIVATE_DATA->cam->get_Description(desc);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s connected", desc.c_str());
				PRIVATE_DATA->cam->get_CameraXSize(&width);
				PRIVATE_DATA->cam->get_CameraYSize(&height);
				PRIVATE_DATA->cam->get_PixelSizeX(&pixelWidth);
				PRIVATE_DATA->cam->get_PixelSizeY(&pixelHeight);
				PRIVATE_DATA->cam->get_CanSetCCDTemperature(&canSetTemp);
				PRIVATE_DATA->cam->get_HasShutter(&hasShutter);
				PRIVATE_DATA->cam->get_HasFilterWheel(&hasFilterWheel);
				if (hasFilterWheel) {
					PRIVATE_DATA->cam->get_FilterCount(filterCount);
				} else {
					filterCount = 0;
				}
				PRIVATE_DATA->cam->get_MaxBinX(&maxBinX);
				PRIVATE_DATA->cam->get_MaxBinY(&maxBinY);
				PRIVATE_DATA->cam->get_PowerOfTwoBinning(&power2Binning);

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
					sprintf(label, "BIN_%dx%d", (int)(width / bin), (int)(height / bin));
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
				PRIVATE_DATA->cam->get_LastError(last);
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, "Connection failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			try {
				PRIVATE_DATA->cam->put_Connected(false);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				PRIVATE_DATA->cam->get_LastError(last);
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
			try {
				PRIVATE_DATA->cam->put_StartX(CCD_FRAME_LEFT_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
				PRIVATE_DATA->cam->put_StartY(CCD_FRAME_TOP_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
				PRIVATE_DATA->cam->put_NumX(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value);
				PRIVATE_DATA->cam->put_NumY(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);
				PRIVATE_DATA->cam->put_BinX(CCD_BIN_HORIZONTAL_ITEM->number.value);
				PRIVATE_DATA->cam->put_BinY(CCD_BIN_VERTICAL_ITEM->number.value);
				PRIVATE_DATA->cam->StartExposure(CCD_EXPOSURE_ITEM->number.value, !(CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value));
				CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				PRIVATE_DATA->cam->get_LastError(last);
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
				PRIVATE_DATA->cam->get_CanAbortExposure(&canAbort);
				if (canAbort) {
					PRIVATE_DATA->cam->AbortExposure();
				}
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			} catch (std::runtime_error err) {
				std::string text = err.what();
				std::string last("");
				PRIVATE_DATA->cam->get_LastError(last);
				CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, "Abort exposure failed  %s %s", text.c_str(), last.c_str());
				return INDIGO_OK;
			}
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		try {
			PRIVATE_DATA->cam->put_CoolerOn(CCD_COOLER_ON_ITEM->sw.value);
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			PRIVATE_DATA->cam->get_LastError(last);
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, "Cooler setup failed  %s %s", text.c_str(), last.c_str());
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		try {
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				PRIVATE_DATA->cam->put_CoolerOn(true);
				CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			PRIVATE_DATA->cam->put_SetCCDTemperature(CCD_TEMPERATURE_ITEM->number.value);
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} catch (std::runtime_error err) {
			std::string text = err.what();
			std::string last("");
			PRIVATE_DATA->cam->get_LastError(last);
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
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s (%s) detached.", device->name, PRIVATE_DATA->serial);
	delete PRIVATE_DATA->cam;
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_device *devices[QSICamera::MAXCAMERAS];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;

	pthread_mutex_lock(&device_mutex);
	libusb_get_device_descriptor(dev, &descriptor);
	if (descriptor.idVendor != QSI_VENDOR_ID || descriptor.idProduct != QSI_PRODUCT_ID) {
		pthread_mutex_unlock(&device_mutex);
		return 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hotplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			static indigo_device ccd_template = {
				"", false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
				ccd_attach,
				indigo_ccd_enumerate_properties,
				ccd_change_property,
				NULL,
				ccd_detach
			};
			std::string camSerial[QSICamera::MAXCAMERAS];
			std::string camDesc[QSICamera::MAXCAMERAS];
			char serial[INDIGO_NAME_SIZE];
			char desc[INDIGO_NAME_SIZE];
			int count;
			QSICamera cam;
			cam.get_AvailableCameras(camSerial, camDesc, count);
			for (int i = 0; i < count; i++) {
				strcpy(serial, camSerial[i].c_str());
				strcpy(desc, camDesc[i].c_str());
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "device[i]: desc = %s serial = %s", desc, serial);
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
				strcpy(private_data->serial, serial);
				indigo_device *device = (indigo_device *)malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &ccd_template, sizeof(indigo_device));
				strcpy(device->name, desc);
				device->private_data = private_data;
				for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *(*)(void *))indigo_attach_device, devices[j] = device);
						break;
					}
				}
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
				indigo_device *device = devices[j];
				if (device) {
					PRIVATE_DATA->available = false;
				}
			}
			std::string camSerial[QSICamera::MAXCAMERAS];
			std::string camDesc[QSICamera::MAXCAMERAS];
			char serial[INDIGO_NAME_SIZE];
			int count;
			QSICamera cam;
			cam.get_AvailableCameras(camSerial, camDesc, count);
			for (int i = 0; i < count; i++) {
				strcpy(serial, camSerial[i].c_str());
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "device[i]: serial = %s", serial);
				for (int j = 0; j < QSICamera::MAXCAMERAS; j++) {
					indigo_device *device = devices[j];
					if (device && !PRIVATE_DATA->available) {
						PRIVATE_DATA->available = !strcmp(serial, PRIVATE_DATA->serial);
						break;
					}
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
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};

static void remove_all_devices() {
	for (int i = 0; i < QSICamera::MAXCAMERAS; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_qsi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "QSI Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			for (int i = 0; i < QSICamera::MAXCAMERAS; i++) {
				devices[i] = NULL;
			}
			QSICamera cam;
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
