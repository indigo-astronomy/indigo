// Copyright (c) 2018 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>


/** INDIGO CCD driver for Apogee
 \file indigo_ccd_apogee.cpp
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME		"indigo_ccd_apogee"

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

#include <libapogee/Alta.h>
#include <libapogee/AltaF.h>
#include <libapogee/Ascent.h>
#include <libapogee/Aspen.h>
#include <libapogee/Quad.h>
#include <libapogee/ApgLogger.h>
#include <libapogee/CamHelpers.h>
#include <libapogee/versionNo.h>

#include "indigo_driver_xml.h"
#include "indigo_ccd_apogee.h"

#define MAXCAMERAS				5

#define PRIVATE_DATA              ((apogee_private_data *)device->private_data)

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c

typedef struct {
	bool available;
	indigo_timer *exposure_timer, *temperature_timer;
	long int buffer_size;
	unsigned short *buffer;
	char serial[255];
	bool can_check_temperature;
} apogee_private_data;

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->can_check_temperature = true;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' attached", device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_device *devices[MAXCAMERAS];

static void hotplug(void *param) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	std::string camSerial[MAXCAMERAS];
	std::string camDesc[MAXCAMERAS];
	char serial[INDIGO_NAME_SIZE];
	char desc[INDIGO_NAME_SIZE];
	int count;
	sleep(3);
	try {
		//cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		//cam.get_LastError(last);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot plug failed  %s %s", text.c_str(), last.c_str());
		return;
	}
	for (int i = 0; i < count; i++) {
		strncpy(serial, camSerial[i].c_str(), INDIGO_NAME_SIZE);
		strncpy(desc, camDesc[i].c_str(), INDIGO_NAME_SIZE);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera[%d]: desc = %s serial = %s", i, desc, serial);
		bool found = false;
		for (int j = 0; j < MAXCAMERAS; j++) {
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
		apogee_private_data *private_data = (apogee_private_data *)malloc(sizeof(apogee_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(apogee_private_data));
		strncpy(private_data->serial, serial, INDIGO_NAME_SIZE);
		indigo_device *device = (indigo_device *)malloc(sizeof(indigo_device));
		assert(device != NULL);
		memcpy(device, &ccd_template, sizeof(indigo_device));
		strncpy(device->name, desc, INDIGO_NAME_SIZE);
		device->private_data = private_data;
		for (int j = 0; j < MAXCAMERAS; j++) {
			if (devices[j] == NULL) {
				indigo_async((void *(*)(void *))indigo_attach_device, devices[j] = device);
				break;
			}
		}
	}
}

static void hotunplug(void *param) {
	std::string camSerial[MAXCAMERAS];
	std::string camDesc[MAXCAMERAS];
	char serial[INDIGO_NAME_SIZE];
	int count;
	sleep(3);
	try {
		//cam.get_AvailableCameras(camSerial, camDesc, count);
	} catch (std::runtime_error err) {
		std::string text = err.what();
		std::string last("");
		//cam.get_LastError(last);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Hot unplug failed  %s %s", text.c_str(), last.c_str());
		return;
	}
	for (int j = 0; j < MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device) {
			PRIVATE_DATA->available = false;
		}
	}
	for (int i = 0; i < count; i++) {
		strncpy(serial, camSerial[i].c_str(), INDIGO_NAME_SIZE);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "camera[%d]: serial = %s", i, serial);
		for (int j = 0; j < MAXCAMERAS; j++) {
			indigo_device *device = devices[j];
			if (!device || strcmp(serial, PRIVATE_DATA->serial))
				continue;
			PRIVATE_DATA->available = true;
			break;
		}
	}
	for (int j = 0; j < MAXCAMERAS; j++) {
		indigo_device *device = devices[j];
		if (device && !PRIVATE_DATA->available) {
			indigo_detach_device(device);
			free(device->private_data);
			free(device);
			devices[j] = NULL;
		}
	}
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	pthread_mutex_lock(&device_mutex);
	libusb_get_device_descriptor(dev, &descriptor);
	if (descriptor.idVendor == UsbFrmwr::APOGEE_VID) {
		switch (event) {
			case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot-plug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
				indigo_async((void *(*)(void *))hotplug, NULL);
				break;
			}
			case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Hot-unplug: vid=%x pid=%x", descriptor.idVendor, descriptor.idProduct);
				indigo_async((void *(*)(void *))hotunplug, NULL);
				break;
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};

static void remove_all_devices() {
	for (int i = 0; i < MAXCAMERAS; i++) {
		indigo_device **device = &devices[i];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		if (((apogee_private_data *)(*device)->private_data)->buffer)
			free(((apogee_private_data *)(*device)->private_data)->buffer);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_apogee(indigo_driver_action action, indigo_driver_info *info) {
		static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

		SET_DRIVER_INFO(info, "Apogee Camera", __FUNCTION__, DRIVER_VERSION, last_action);

		if (action == last_action)
			return INDIGO_OK;

		switch (action) {
			case INDIGO_DRIVER_INIT: {
				for (int i = 0; i < MAXCAMERAS; i++) {
					devices[i] = NULL;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libapogee version: %d.%d.%d", APOGEE_MAJOR_VERSION, APOGEE_MINOR_VERSION, APOGEE_PATCH_VERSION);
				last_action = action;
				indigo_start_usb_event_handler();
				int rc = libusb_hotplug_register_callback(NULL, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), LIBUSB_HOTPLUG_ENUMERATE, UsbFrmwr::APOGEE_VID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
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
