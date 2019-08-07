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
#include <arpa/inet.h>

#include <libusb-1.0/libusb.h>

#include <indigo/indigo_usb_utils.h>

#include "indigo_ptp.h"
#include "indigo_ccd_ptp.h"

#define DRIVER_VERSION 	0x0001
#define DRIVER_NAME 		"indigo_ccd_ptp"

#define MAX_DEVICES    	4
#define PRIVATE_DATA    ((ptp_private_data *)device->private_data)

typedef struct {
	int vendor;
	int product;
	const char *name;
	indigo_device_interface iface;
} ptp_camera_model;

typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	int ep_in, ep_out, ep_int;
	int device_count;
	pthread_mutex_t mutex;
	uint32_t session_id;
	uint32_t transaction_id;
} ptp_private_data;

static indigo_device *devices[MAX_DEVICES];

static ptp_camera_model CAMERA[] = {
	{ 0x05ac, 0x12a8, "Apple iPhone", INDIGO_INTERFACE_CCD },
	{ 0x04a9, 0x3218, "Canon 600D", INDIGO_INTERFACE_CCD },
	{ 0, 0, NULL }
};

#define PTP_CONTAINER_COMMAND		0x0001
#define PTP_CONTAINER_DATA			0x0002
#define PTP_CONTAINER_RESPONSE	0x0003
#define PTP_CONTAINER_EVENT			0x0004

bool ptp_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = 0;
	struct libusb_config_descriptor *config_descriptor = NULL;
	libusb_device *dev = PRIVATE_DATA->dev;
	rc = libusb_open(dev, &PRIVATE_DATA->handle);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		if (libusb_kernel_driver_active(handle, 0) == 1) {
			rc = libusb_detach_kernel_driver(handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_detach_kernel_driver() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		}
	}
	if (rc >= 0) {
		rc = libusb_get_config_descriptor(dev, 0, &config_descriptor);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_config_descriptor() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0 && config_descriptor) {
		int configuration_value = config_descriptor->bConfigurationValue;
		rc = libusb_set_configuration(handle, configuration_value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_configuration(%d) -> %s", configuration_value, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0 && config_descriptor) {
		int interface_number = config_descriptor->interface->altsetting->bInterfaceNumber;
		rc = libusb_claim_interface(handle, interface_number);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface(%d) -> %s", interface_number, rc < 0 ? libusb_error_name(rc) : "OK");
//		int alt_settings = config_descriptor->interface->altsetting->bAlternateSetting;
//		rc = libusb_set_interface_alt_setting(handle, interface_number, alt_settings);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_interface_alt_setting(%d, %d) -> %s", interface_number, alt_settings, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0 && config_descriptor) {
		const struct libusb_endpoint_descriptor *ep = config_descriptor->interface->altsetting->endpoint;
		int count = config_descriptor->interface->altsetting->bNumEndpoints;
		for (int i = 0; i < count; i++) {
			if (ep[i].bmAttributes == LIBUSB_TRANSFER_TYPE_BULK) {
				int address = ep[i].bEndpointAddress;
				if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
					PRIVATE_DATA->ep_in = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EP IN = %02x", PRIVATE_DATA->ep_in);
				} else if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
					PRIVATE_DATA->ep_out = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EP OUT = %02x", PRIVATE_DATA->ep_out);
				}
			} else if (ep[i].bmAttributes == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
				int address = ep[i].bEndpointAddress;
				if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
					PRIVATE_DATA->ep_int = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EP INT = %02x", PRIVATE_DATA->ep_int);
				}
			}
		}
	}
	if (config_descriptor)
		libusb_free_config_descriptor(config_descriptor);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return true;
}

bool ptp_request(indigo_device *device, uint16_t code, int count, ...) {
	ptp_container request;
	int length = 2 * sizeof(uint16_t) + (2 + count) * sizeof(uint32_t);
	request.length = htonl(length);
	request.type = htons(PTP_CONTAINER_COMMAND);
	request.code = htons(code);
	request.transaction_id = htonl(PRIVATE_DATA->transaction_id++);
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		request.params[i] = htonl(va_arg(argp, uint32_t));
	va_end(argp);
	for (int i = count; i < 5; i++)
		request.params[i] = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "req: %08x %04x %04x %08x [%08x, %08x, %08x, %08x, %08x]", request.length, request.type, request.code, request.transaction_id, request.params[0], request.params[1], request.params[2], request.params[3], request.params[4]);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, length, &length, 5000);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return rc >= 0;
}

bool ptp_response(indigo_device *device, uint16_t *code, int count, ...) {
	ptp_container response;
	int length;
	memset(&response, 0, sizeof(response));
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, sizeof(response), &length, 5000);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (code)
		*code = ntohs(response.code);
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		*va_arg(argp, uint32_t *) = ntohl(response.params[i]);
	va_end(argp);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "resp: %08x %04x %04x %08x [%08x, %08x, %08x, %08x, %08x]", response.length, response.type, response.code, response.transaction_id, response.params[0], response.params[1], response.params[2], response.params[3], response.params[4]);
	return rc >= 0;
}

void ptp_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close()");
	PRIVATE_DATA->handle = NULL;
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

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
		PRIVATE_DATA->transaction_id = 0;
		if (ptp_request(device, PTP_REQ_OPEN_SESSION, 1, 0) && ptp_response(device, &code, 1, &PRIVATE_DATA->session_id) && code == PTP_RESP_OK)
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
				ptp_request(device, PTP_REQ_CLOSE_SESSION, 1, 0);
				ptp_response(device, NULL, 1, &PRIVATE_DATA->session_id);
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
