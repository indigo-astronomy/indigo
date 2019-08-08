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

/** INDIGO PTP implementation
 \file indigo_ptp.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <libusb-1.0/libusb.h>

#include "indigo_ptp.h"

static char *ptp_request_code_label(ptp_request_code code) {
	switch (code) {
		case ptp_request_undefined: return "Undefined";
		case ptp_request_getdeviceinfo: return "GetDeviceInfo";
		case ptp_request_opensession: return "OpenSession";
		case ptp_request_closesession: return "CloseSession";
		case ptp_request_getstorageids: return "GetStorageIDs";
		case ptp_request_getstorageinfo: return "GetStorageInfo";
		case ptp_request_getnumobjects: return "GetNumObjects";
		case ptp_request_getobjecthandles: return "GetObjectHandles";
		case ptp_request_getobjectinfo: return "GetObjectInfo";
		case ptp_request_getobject: return "GetObject";
		case ptp_request_getthumb: return "GetThumb";
		case ptp_request_deleteobject: return "DeleteObject";
		case ptp_request_sendobjectinfo: return "SendObjectInfo";
		case ptp_request_sendobject: return "SendObject";
		case ptp_request_initiatecapture: return "InitiateCapture";
		case ptp_request_formatstore: return "FormatStore";
		case ptp_request_resetdevice: return "ResetDevice";
		case ptp_request_selftest: return "SelfTest";
		case ptp_request_setobjectprotection: return "SetObjectProtection";
		case ptp_request_powerdown: return "PowerDown";
		case ptp_request_getdevicepropdesc: return "GetDevicePropDesc";
		case ptp_request_getdevicepropvalue: return "GetDevicePropValue";
		case ptp_request_setdevicepropvalue: return "SetDevicePropValue";
		case ptp_request_resetdevicepropvalue: return "ResetDevicePropValue";
		case ptp_request_terminateopencapture: return "TerminateOpenCapture";
		case ptp_request_moveobject: return "MoveObject";
		case ptp_request_copyobject: return "CopyObject";
		case ptp_request_getpartialobject: return "GetPartialObject";
		case ptp_request_initiateopencapture: return "InitiateOpenCapture";
		case ptp_request_getnumdownloadableobjects: return "GetNumDownloadableObjects";
		case ptp_request_getallobjectinfo: return "GetAllObjectInfo";
		case ptp_request_getuserassigneddevicename: return "GetUserAssignedDeviceName";
		case ptp_request_mtpgetobjectpropssupported: return "MTPGetObjectPropsSupported";
		case ptp_request_mtpgetobjectpropdesc: return "MTPGetObjectPropDesc";
		case ptp_request_mtpgetobjectpropvalue: return "MTPGetObjectPropValue";
		case ptp_request_mtpsetobjectpropvalue: return "MTPSetObjectPropValue";
		case ptp_request_mtpgetobjproplist: return "MTPGetObjPropList";
		case ptp_request_mtpsetobjproplist: return "MTPSetObjPropList";
		case ptp_request_mtpgetinterdependendpropdesc: return "MTPGetInterdependendPropdesc";
		case ptp_request_mtpsendobjectproplist: return "MTPSendObjectPropList";
		case ptp_request_mtpgetobjectreferences: return "MTPGetObjectReferences";
		case ptp_request_mtpsetobjectreferences: return "MTPSetObjectReferences";
		case ptp_request_mtpupdatedevicefirmware: return "MTPUpdateDeviceFirmware";
		case ptp_request_mtpskip: return "MTPSkip";
	}
	return "Undefined";
}

void ptp_dump_container(int line, const char *function, ptp_container *container) {
	char buffer[256];
	int offset = 0;
	switch (container->type) {
		case ptp_container_command:
			offset = sprintf(buffer, "request %s (%04x) %08x [", ptp_request_code_label(container->code), container->code, container->transaction_id);
			break;
		case ptp_container_data:
			offset = sprintf(buffer, "data %s (%04x) %08x [", "???", container->code, container->transaction_id);
			break;
		case ptp_container_response:
			offset = sprintf(buffer, "response %s (%04x) %08x [", "???", container->code, container->transaction_id);
			break;
		case ptp_container_event:
			offset = sprintf(buffer, "event %s (%04x) %08x [", "???", container->code, container->transaction_id);
			break;
		default:
			offset = sprintf(buffer, "unknown %04x %08x [", container->code, container->transaction_id);
			break;
	}
	if (container->length > 12) {
		offset += sprintf(buffer + offset, "%08x", container->params[0]);
	}
	if (container->length > 16) {
		offset += sprintf(buffer + offset, ", %08x", container->params[1]);
	}
	if (container->length > 20) {
		offset += sprintf(buffer + offset, ", %08x", container->params[2]);
	}
	if (container->length > 24) {
		offset += sprintf(buffer + offset, ", %08x", container->params[3]);
	}
	if (container->length > 28) {
		offset += sprintf(buffer + offset, ", %08x", container->params[4]);
	}
	sprintf(buffer + offset, "]");
	indigo_debug("%s[%d, %s]: %s", DRIVER_NAME, line, function,  buffer);
}

bool ptp_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = 0;
	struct libusb_device_descriptor	device_descriptor;
	libusb_device *dev = PRIVATE_DATA->dev;
	rc = libusb_get_device_descriptor(dev, &device_descriptor);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_device_descriptor() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	rc = libusb_open(dev, &PRIVATE_DATA->handle);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_open() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc >= 0) {
		if (libusb_kernel_driver_active(handle, 0) == 1) {
			rc = libusb_detach_kernel_driver(handle, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_detach_kernel_driver() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		}
	}
	struct libusb_config_descriptor *config_descriptor = NULL;
	const struct libusb_interface *interface = NULL;
	for (int config = 0; config < device_descriptor.bNumConfigurations; config++) {
		rc = libusb_get_config_descriptor(dev, config, &config_descriptor);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_get_config_descriptor(%d) -> %s", config, rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc < 0)
			break;
		for (int iface = 0; iface < config_descriptor->bNumInterfaces; iface++) {
			interface = config_descriptor->interface + iface;
			if (interface->altsetting->bInterfaceClass == 0x06 && interface->altsetting->bInterfaceSubClass == 0x01 && interface->altsetting->bInterfaceProtocol == 0x01) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PTP CONFIG = %d IFACE = %d", config_descriptor->bConfigurationValue, interface->altsetting->bInterfaceNumber);
				break;
			}
			interface = NULL;
		}
		if (interface)
			break;
		libusb_free_config_descriptor(config_descriptor);
	}
	if (rc >= 0 && config_descriptor) {
		int configuration_value = config_descriptor->bConfigurationValue;
		rc = libusb_set_configuration(handle, configuration_value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_configuration(%d) -> %s", configuration_value, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0 && interface) {
		int interface_number = interface->altsetting->bInterfaceNumber;
		rc = libusb_claim_interface(handle, interface_number);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface(%d) -> %s", interface_number, rc < 0 ? libusb_error_name(rc) : "OK");
		int alt_settings = config_descriptor->interface->altsetting->bAlternateSetting;
		rc = libusb_set_interface_alt_setting(handle, interface_number, alt_settings);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_interface_alt_setting(%d, %d) -> %s", interface_number, alt_settings, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc >= 0 && interface) {
		const struct libusb_endpoint_descriptor *ep = interface->altsetting->endpoint;
		int count = interface->altsetting->bNumEndpoints;
		for (int i = 0; i < count; i++) {
			if (ep[i].bmAttributes == LIBUSB_TRANSFER_TYPE_BULK) {
				int address = ep[i].bEndpointAddress;
				if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
					PRIVATE_DATA->ep_in = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PTP EP IN = %02x", PRIVATE_DATA->ep_in);
				} else if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
					PRIVATE_DATA->ep_out = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PTP EP OUT = %02x", PRIVATE_DATA->ep_out);
				}
			} else if (ep[i].bmAttributes == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
				int address = ep[i].bEndpointAddress;
				if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
					PRIVATE_DATA->ep_int = address;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PTP EP INT = %02x", PRIVATE_DATA->ep_int);
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
	request.length = 2 * sizeof(uint16_t) + (2 + count) * sizeof(uint32_t);
	request.type = ptp_container_command;
	request.code = code;
	request.transaction_id = PRIVATE_DATA->transaction_id++;
	int length = 0;
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		request.params[i] = (va_arg(argp, uint32_t));
	va_end(argp);
	for (int i = count; i < 5; i++)
		request.params[i] = 0;
	PTP_DUMP_CONTAINER(&request);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, 2 * sizeof(uint16_t) + (2 + count) * sizeof(uint32_t), &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return rc >= 0;
}

bool ptp_read(indigo_device *device, void *data, int length, int *actual) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)data, length, actual, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)data, length, actual, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return rc >= 0;
}

bool ptp_response(indigo_device *device, uint16_t *code, int count, ...) {
	ptp_container response;
	int length = sizeof(response);
	memset(&response, 0, sizeof(response));
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, length, &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, length, &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (code)
		*code = response.code;
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		*va_arg(argp, uint32_t *) = response.params[i];
	va_end(argp);
	PTP_DUMP_CONTAINER(&response);
	return rc >= 0;
}

void ptp_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close()");
	PRIVATE_DATA->handle = NULL;
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}
