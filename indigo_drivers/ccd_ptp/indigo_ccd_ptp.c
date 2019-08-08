	// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO PTP DSLR driver
 \file indigo_ccd_ptp.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <pthread.h>

#include <libusb-1.0/libusb.h>

#include <indigo/indigo_usb_utils.h>

#include "indigo_ptp.h"
#include "indigo_ptp_canon.h"
#include "indigo_ccd_ptp.h"

#define MAX_DEVICES    	4


static indigo_device *devices[MAX_DEVICES];

static ptp_camera_model CAMERA[] = {
	{ 0x05ac, 0x12a8, "Apple iPhone", INDIGO_INTERFACE_CCD },
	{ 0x045e, 0x0a00, "Microsoft Lumia", INDIGO_INTERFACE_CCD },
	{ 0x04a9, 0x3218, "Canon 600D", INDIGO_INTERFACE_CCD },
	{ 0, 0, NULL }
};


static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		// TBD
		// --------------------------------------------------------------------------------
		PRIVATE_DATA->transaction_id = 0;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		// --------------------------------------------------------------------------------
		// TBD
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static void handle_connection(indigo_device *device) {
	bool result = true;
	if (PRIVATE_DATA->device_count++ == 0) {
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		if (indigo_try_global_lock(device) != INDIGO_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			result = false;
		} else {
			result = ptp_open(device);
		}
	}
	if (result) {
		uint16_t code;
		void *buffer;
		int length;
		PRIVATE_DATA->transaction_id = 0;
		PRIVATE_DATA->session_id = 0;
		if (ptp_request(device, ptp_operation_GetDeviceInfo, 0) && ptp_read(device, &code, &buffer, &length) && ptp_response(device, &code, 1, &PRIVATE_DATA->session_id) && code == ptp_response_OK) {
			ptp_device_info device_info;
			ptp_copy_device_info(buffer, &device_info);
			PTP_DUMP_DEVICE_INFO(&device_info);
			free(buffer);
		}
		PRIVATE_DATA->transaction_id = 0;
		if (ptp_request(device, ptp_operation_OpenSession, 1, 1) && ptp_response(device, &code, 1, &PRIVATE_DATA->session_id) && code == ptp_response_OK)
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		else
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_set_timer(device, 0, handle_connection);
			return INDIGO_OK;
		} else {
			// TBD
			if (--PRIVATE_DATA->device_count == 0) {
				ptp_request(device, ptp_operation_CloseSession, 0);
				ptp_response(device, NULL, 0);
				ptp_close(device);
				indigo_global_unlock(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	// TBD
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	// --------------------------------------------------------------------------------
	// TBD
	// --------------------------------------------------------------------------------
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
		);
	
	struct libusb_device_descriptor descriptor;
	
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			int rc = libusb_get_device_descriptor(dev, &descriptor);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			for (int i = 0; CAMERA[i].name; i++) {
				if (CAMERA[i].vendor == descriptor.idVendor && CAMERA[i].product == descriptor.idProduct) {
					ptp_private_data *private_data = malloc(sizeof(ptp_private_data));
					assert(private_data != NULL);
					memset(private_data, 0, sizeof(ptp_private_data));
					private_data->dev = dev;
					if (descriptor.idVendor == 0x04a9) {
						private_data->operation_code_label = ptp_operation_canon_code_label;
						private_data->response_code_label = ptp_response_canon_code_label;
						private_data->event_code_label = ptp_event_canon_code_label;
						private_data->property_code_label = ptp_property_canon_code_label;
					} else {
						private_data->operation_code_label = ptp_operation_code_label;
						private_data->response_code_label = ptp_response_code_label;
						private_data->event_code_label = ptp_event_code_label;
						private_data->property_code_label = ptp_property_code_label;
					}
					libusb_ref_device(dev);
					indigo_device *device = malloc(sizeof(indigo_device));
					indigo_device *master_device = device;
					assert(device != NULL);
					memcpy(device, &ccd_template, sizeof(indigo_device));
					device->master_device = master_device;
					char usb_path[INDIGO_NAME_SIZE];
					indigo_get_usb_path(dev, usb_path);
					snprintf(device->name, INDIGO_NAME_SIZE, "%s #%s", CAMERA[i].name, usb_path);
					device->private_data = private_data;
					for (int j = 0; j < MAX_DEVICES; j++) {
						if (devices[j] == NULL) {
							indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			ptp_private_data *private_data = NULL;
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
	return 0;
};


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_ptp(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, "PTP-over-USB Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
			
		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
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
