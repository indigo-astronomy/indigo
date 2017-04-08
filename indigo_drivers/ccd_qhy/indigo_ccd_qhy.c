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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO QHY CCD driver
 \file indigo_ccd_qhy.c
 */

#define DRIVER_VERSION 0x0001

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

#include <libqhy/libqhy.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_qhy.h"


#define PRIVATE_DATA        ((qhy_private_data *)device->private_data)

typedef struct {
	libqhy_device_context *device_context;
	libusb_device *dev;
	int device_count;
	indigo_timer *exposure_timer, *temperture_timer, *guider_timer;
	double current_temperature;
	unsigned width, height, bits_per_pixel;
	double pixel_width, pixel_height;
	unsigned relay_mask;
	unsigned char *buffer;
} qhy_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (libqhy_read_pixels(PRIVATE_DATA->device_context, (unsigned short *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE))) {
			libqhy_stop(PRIVATE_DATA->device_context);
			indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->width, PRIVATE_DATA->height, true, NULL);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	double temperature;
	if (libqhy_get_temperature(PRIVATE_DATA->device_context, &temperature)) {
		CCD_TEMPERATURE_ITEM->number.value = temperature;
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
		// -------------------------------------------------------------------------------- CCD_GAIN
		CCD_GAIN_PROPERTY->hidden = false;
		CCD_GAIN_ITEM->number.max = CCD_GAIN_ITEM->number.value = 100;
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	libqhy_debug_level = DEBUG_ENABLED_ITEM->sw.value;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = libqhy_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context);
			}
			if (result) {
				libqhy_get_resolution(PRIVATE_DATA->device_context, &PRIVATE_DATA->width, &PRIVATE_DATA->height, &PRIVATE_DATA->bits_per_pixel);
				libqhy_get_pixel_size(PRIVATE_DATA->device_context, &PRIVATE_DATA->pixel_width, &PRIVATE_DATA->pixel_height);
				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = PRIVATE_DATA->bits_per_pixel;
				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = PRIVATE_DATA->width;
				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max  = PRIVATE_DATA->height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(PRIVATE_DATA->pixel_width * 100)/100;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(PRIVATE_DATA->pixel_height * 100) / 100;
				PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(FITS_HEADER_SIZE + 2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + 5);
				assert(PRIVATE_DATA->buffer != NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				ccd_temperature_callback(device);
			} else {
				indigo_cancel_timer(device, &PRIVATE_DATA->temperture_timer);
				if (PRIVATE_DATA->buffer != NULL) {
					free(PRIVATE_DATA->buffer);
					PRIVATE_DATA->buffer = NULL;
				}
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperture_timer);
			if (PRIVATE_DATA->buffer != NULL) {
				free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
			}
			if (--PRIVATE_DATA->device_count == 0) {
				libqhy_close(PRIVATE_DATA->device_context);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		libqhy_set_gain(PRIVATE_DATA->device_context, CCD_GAIN_ITEM->number.value);
		libqhy_set_exposure_time(PRIVATE_DATA->device_context, CCD_EXPOSURE_ITEM->number.value);
		libqhy_start(PRIVATE_DATA->device_context);
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		// libqhy_abort_exposure(PRIVATE_DATA->device_context);
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	libqhy_debug_level = DEBUG_ENABLED_ITEM->sw.value;
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = libqhy_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context);
			}
			if (result) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				libqhy_close(PRIVATE_DATA->device_context);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);

		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);

		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = {
		"", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		"", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libqhy_camera_type type;
			const char *name;
			bool is_guider;
			if (libqhy_camera(dev, &type, &name, &is_guider)) {
				qhy_private_data *private_data = malloc(sizeof(qhy_private_data));
				assert(private_data != NULL);
				memset(private_data, 0, sizeof(qhy_private_data));
				private_data->dev = dev;
				libusb_ref_device(dev);
				indigo_device *device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &ccd_template, sizeof(indigo_device));
				strcpy(device->name, name);
				device->private_data = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
						break;
					}
				}
				if (is_guider) {
					device = malloc(sizeof(indigo_device));
					assert(device != NULL);
					memcpy(device, &guider_template, sizeof(indigo_device));
					strcpy(device->name, name);
					strcat(device->name, " (guider)");
					device->private_data = private_data;
					for (int j = 0; j < MAX_DEVICES; j++) {
						if (devices[j] == NULL) {
							indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
							break;
						}
					}
				}
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			qhy_private_data *private_data = NULL;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					if (PRIVATE_DATA->dev == dev) {
						private_data = PRIVATE_DATA;
						indigo_detach_device(device);
						free(device);
						devices[j] = NULL;
					}
				}
			}
			if (private_data != NULL) {
				libusb_unref_device(dev);
				free(private_data);
			}
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
}

static libusb_hotplug_callback_handle callback_handle1, callback_handle2;

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	libqhy_use_syslog = indigo_use_syslog;
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "QHY Camera", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, QHY_VID1, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle1);
		if (rc >= 0)
			rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, QHY_VID2, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle2);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_qhy: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle1);
		libusb_hotplug_deregister_callback(NULL, callback_handle2);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_qhy: libusb_hotplug_deregister_callback [%d]", __LINE__));
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}


