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

char *ptp_operation_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_Undefined: return "Undefined";
		case ptp_operation_GetDeviceInfo: return "GetDeviceInfo";
		case ptp_operation_OpenSession: return "OpenSession";
		case ptp_operation_CloseSession: return "CloseSession";
		case ptp_operation_GetStorageIDs: return "GetStorageIDs";
		case ptp_operation_GetStorageInfo: return "GetStorageInfo";
		case ptp_operation_GetNumObjects: return "GetNumObjects";
		case ptp_operation_GetObjectHandles: return "GetObjectHandles";
		case ptp_operation_GetObjectInfo: return "GetObjectInfo";
		case ptp_operation_GetObject: return "GetObject";
		case ptp_operation_GetThumb: return "GetThumb";
		case ptp_operation_DeleteObject: return "DeleteObject";
		case ptp_operation_SendObjectInfo: return "SendObjectInfo";
		case ptp_operation_SendObject: return "SendObject";
		case ptp_operation_InitiateCapture: return "InitiateCapture";
		case ptp_operation_FormatStore: return "FormatStore";
		case ptp_operation_ResetDevice: return "ResetDevice";
		case ptp_operation_SelfTest: return "SelfTest";
		case ptp_operation_SetObjectProtection: return "SetObjectProtection";
		case ptp_operation_PowerDown: return "PowerDown";
		case ptp_operation_GetDevicePropDesc: return "GetDevicePropDesc";
		case ptp_operation_GetDevicePropValue: return "GetDevicePropValue";
		case ptp_operation_SetDevicePropValue: return "SetDevicePropValue";
		case ptp_operation_ResetDevicePropValue: return "ResetDevicePropValue";
		case ptp_operation_TerminateOpenCapture: return "TerminateOpenCapture";
		case ptp_operation_MoveObject: return "MoveObject";
		case ptp_operation_CopyObject: return "CopyObject";
		case ptp_operation_GetPartialObject: return "GetPartialObject";
		case ptp_operation_InitiateOpenCapture: return "InitiateOpenCapture";
		case ptp_operation_GetNumDownloadableObjects: return "GetNumDownloadableObjects";
		case ptp_operation_GetAllObjectInfo: return "GetAllObjectInfo";
		case ptp_operation_GetUserAssignedDeviceName: return "GetUserAssignedDeviceName";
		case ptp_operation_MTPGetObjectPropsSupported: return "MTPGetObjectPropsSupported";
		case ptp_operation_MTPGetObjectPropDesc: return "MTPGetObjectPropDesc";
		case ptp_operation_MTPGetObjectPropValue: return "MTPGetObjectPropValue";
		case ptp_operation_MTPSetObjectPropValue: return "MTPSetObjectPropValue";
		case ptp_operation_MTPGetObjPropList: return "MTPGetObjPropList";
		case ptp_operation_MTPSetObjPropList: return "MTPSetObjPropList";
		case ptp_operation_MTPGetInterdependendPropdesc: return "MTPGetInterdependendPropdesc";
		case ptp_operation_MTPSendObjectPropList: return "MTPSendObjectPropList";
		case ptp_operation_MTPGetObjectReferences: return "MTPGetObjectReferences";
		case ptp_operation_MTPSetObjectReferences: return "MTPSetObjectReferences";
		case ptp_operation_MTPUpdateDeviceFirmware: return "MTPUpdateDeviceFirmware";
		case ptp_operation_MTPSkip: return "MTPSkip";
	}
	return NULL;
}

char *ptp_response_code_label(uint16_t code) {
	switch (code) {
		case ptp_response_Undefined: return "Undefined";
		case ptp_response_OK: return "OK";
		case ptp_response_GeneralError: return "GeneralError";
		case ptp_response_SessionNotOpen: return "SessionNotOpen";
		case ptp_response_InvalidTransactionID: return "InvalidTransactionID";
		case ptp_response_OperationNotSupported: return "OperationNotSupported";
		case ptp_response_ParameterNotSupported: return "ParameterNotSupported";
		case ptp_response_IncompleteTransfer: return "IncompleteTransfer";
		case ptp_response_InvalidStorageID: return "InvalidStorageID";
		case ptp_response_InvalidObjectHandle: return "InvalidObjectHandle";
		case ptp_response_DevicePropNotSupported: return "DevicePropNotSupported";
		case ptp_response_InvalidObjectFormatCode: return "InvalidObjectFormatCode";
		case ptp_response_StoreFull: return "StoreFull";
		case ptp_response_ObjectWriteProtected: return "ObjectWriteProtected";
		case ptp_response_StoreReadOnly: return "StoreReadOnly";
		case ptp_response_AccessDenied: return "AccessDenied";
		case ptp_response_NoThumbnailPresent: return "NoThumbnailPresent";
		case ptp_response_SelfTestFailed: return "SelfTestFailed";
		case ptp_response_PartialDeletion: return "PartialDeletion";
		case ptp_response_StoreNotAvailable: return "StoreNotAvailable";
		case ptp_response_SpecificationByFormatUnsupported: return "SpecificationByFormatUnsupported";
		case ptp_response_NoValidObjectInfo: return "NoValidObjectInfo";
		case ptp_response_InvalidCodeFormat: return "InvalidCodeFormat";
		case ptp_response_UnknownVendorCode: return "UnknownVendorCode";
		case ptp_response_CaptureAlreadyTerminated: return "CaptureAlreadyTerminated";
		case ptp_response_DeviceBusy: return "DeviceBusy";
		case ptp_response_InvalidParentObject: return "InvalidParentObject";
		case ptp_response_InvalidDevicePropFormat: return "InvalidDevicePropFormat";
		case ptp_response_InvalidDevicePropValue: return "InvalidDevicePropValue";
		case ptp_response_InvalidParameter: return "InvalidParameter";
		case ptp_response_SessionAlreadyOpen: return "SessionAlreadyOpen";
		case ptp_response_TransactionCancelled: return "TransactionCancelled";
		case ptp_response_SpecificationOfDestinationUnsupported: return "SpecificationOfDestinationUnsupported";
		case ptp_response_MTPUndefined: return "MTPUndefined";
		case ptp_response_MTPInvalidObjectPropCode: return "MTPInvalidObjectPropCode";
		case ptp_response_MTPInvalidObjectProp_Format: return "MTPInvalidObjectProp_Format";
		case ptp_response_MTPInvalidObjectProp_Value: return "MTPInvalidObjectProp_Value";
		case ptp_response_MTPInvalidObjectReference: return "MTPInvalidObjectReference";
		case ptp_response_MTPInvalidDataset: return "MTPInvalidDataset";
		case ptp_response_MTPSpecificationByGroupUnsupported: return "MTPSpecificationByGroupUnsupported";
		case ptp_response_MTPSpecificationByDepthUnsupported: return "MTPSpecificationByDepthUnsupported";
		case ptp_response_MTPObjectTooLarge: return "MTPObjectTooLarge";
		case ptp_response_MTPObjectPropNotSupported: return "MTPObjectPropNotSupported";
	}
	return NULL;
}

char *ptp_event_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_Undefined: return "Undefined";
		case ptp_event_CancelTransaction: return "CancelTransaction";
		case ptp_event_ObjectAdded: return "ObjectAdded";
		case ptp_event_ObjectRemoved: return "ObjectRemoved";
		case ptp_event_StoreAdded: return "StoreAdded";
		case ptp_event_StoreRemoved: return "StoreRemoved";
		case ptp_event_DevicePropChanged: return "DevicePropChanged";
		case ptp_event_ObjectInfoChanged: return "ObjectInfoChanged";
		case ptp_event_DeviceInfoChanged: return "DeviceInfoChanged";
		case ptp_event_RequestObjectTransfer: return "RequestObjectTransfer";
		case ptp_event_StoreFull: return "StoreFull";
		case ptp_event_DeviceReset: return "DeviceReset";
		case ptp_event_StorageInfoChanged: return "StorageInfoChanged";
		case ptp_event_CaptureComplete: return "CaptureComplete";
		case ptp_event_UnreportedStatus: return "UnreportedStatus";
		case ptp_event_AppleDeviceUnlocked: return "AppleDeviceUnlocked";
		case ptp_event_AppleUserAssignedNameChanged: return "AppleUserAssignedNameChanged";
	}
	return NULL;
}

char *ptp_property_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_Undefined: return "Undefined";
		case ptp_property_BatteryLevel: return "BatteryLevel";
		case ptp_property_FunctionalMode: return "FunctionalMode";
		case ptp_property_ImageSize: return "ImageSize";
		case ptp_property_CompressionSetting: return "CompressionSetting";
		case ptp_property_WhiteBalance: return "WhiteBalance";
		case ptp_property_RGBGain: return "RGBGain";
		case ptp_property_FNumber: return "FNumber";
		case ptp_property_FocalLength: return "FocalLength";
		case ptp_property_FocusDistance: return "FocusDistance";
		case ptp_property_FocusMode: return "FocusMode";
		case ptp_property_ExposureMeteringMode: return "ExposureMeteringMode";
		case ptp_property_FlashMode: return "FlashMode";
		case ptp_property_ExposureTime: return "ExposureTime";
		case ptp_property_ExposureProgramMode: return "ExposureProgramMode";
		case ptp_property_ExposureIndex: return "ExposureIndex";
		case ptp_property_ExposureBiasCompensation: return "ExposureBiasCompensation";
		case ptp_property_DateTime: return "DateTime";
		case ptp_property_CaptureDelay: return "CaptureDelay";
		case ptp_property_StillCaptureMode: return "StillCaptureMode";
		case ptp_property_Contrast: return "Contrast";
		case ptp_property_Sharpness: return "Sharpness";
		case ptp_property_DigitalZoom: return "DigitalZoom";
		case ptp_property_EffectMode: return "EffectMode";
		case ptp_property_BurstNumber: return "BurstNumber";
		case ptp_property_BurstInterval: return "BurstInterval";
		case ptp_property_TimelapseNumber: return "TimelapseNumber";
		case ptp_property_TimelapseInterval: return "TimelapseInterval";
		case ptp_property_FocusMeteringMode: return "FocusMeteringMode";
		case ptp_property_UploadURL: return "UploadURL";
		case ptp_property_Artist: return "Artist";
		case ptp_property_CopyrightInfo: return "CopyrightInfo";
		case ptp_property_SupportedStreams: return "SupportedStreams";
		case ptp_property_EnabledStreams: return "EnabledStreams";
		case ptp_property_VideoFormat: return "VideoFormat";
		case ptp_property_VideoResolution: return "VideoResolution";
		case ptp_property_VideoQuality: return "VideoQuality";
		case ptp_property_VideoFrameRate: return "VideoFrameRate";
		case ptp_property_VideoContrast: return "VideoContrast";
		case ptp_property_VideoBrightness: return "VideoBrightness";
		case ptp_property_AudioFormat: return "AudioFormat";
		case ptp_property_AudioBitrate: return "AudioBitrate";
		case ptp_property_AudioSamplingRate: return "AudioSamplingRate";
		case ptp_property_AudioBitPerSample: return "AudioBitPerSample";
		case ptp_property_AudioVolume: return "AudioVolume";
		case ptp_property_MTPSynchronizationPartner: return "MTPSynchronizationPartner";
		case ptp_property_MTPDeviceFriendlyName: return "MTPDeviceFriendlyName";
		case ptp_property_MTPVolumeLevel: return "MTPVolumeLevel";
		case ptp_property_MTPDeviceIcon: return "MTPDeviceIcon";
		case ptp_property_MTPSessionInitiatorInfo: return "MTPSessionInitiatorInfo";
		case ptp_property_MTPPerceivedDeviceType: return "MTPPerceivedDeviceType";
		case ptp_property_MTPPlaybackRate: return "MTPPlaybackRate";
		case ptp_property_MTPPlaybackObject: return "MTPPlaybackObject";
		case ptp_property_MTPPlaybackContainerIndex: return "MTPPlaybackContainerIndex";
		case ptp_property_MTPPlaybackPosition: return "MTPPlaybackPosition";	}
	return NULL;
}

char *ptp_vendor_label(uint16_t code) {
	switch (code) {
		case ptp_vendor_eastman_kodak: return "Eastman Kodak";
		case ptp_vendor_seiko_epson: return "Seiko Epson";
		case ptp_vendor_agilent: return "Agilent";
		case ptp_vendor_polaroid: return "Polaroid";
		case ptp_vendor_agfa_gevaert: return "Agfa-Gevaert";
		case ptp_vendor_microsoft: return "Microsoft";
		case ptp_vendor_equinox: return "Equinox";
		case ptp_vendor_viewquest: return "Viewquest";
		case ptp_vendor_stmicroelectronics: return "STMicroelectronics";
		case ptp_vendor_nikon: return "Nikon";
		case ptp_vendor_canon: return "Canon";
		case ptp_vendor_fotonation: return "Fotonation";
		case ptp_vendor_pentax: return "Pentax";
		case ptp_vendor_fuji: return "Fuji";
		case ptp_vendor_ndd_medical_technologies: return "ndd Medical Technologies";
		case ptp_vendor_samsung: return "Samsung";
		case ptp_vendor_parrot: return "Parrot";
		case ptp_vendor_panasonic: return "Panasonic";
		case ptp_vendor_sony: return "Sony";
	}
	return NULL;
}

void ptp_dump_container(int line, const char *function, indigo_device *device, ptp_container *container) {
	char buffer[256];
	int offset = 0;
	switch (container->type) {
		case ptp_container_command:
			offset = sprintf(buffer, "request %s (%04x) %08x [", PRIVATE_DATA->operation_code_label(container->code), container->code, container->transaction_id);
			break;
		case ptp_container_data:
			offset = sprintf(buffer, "data %04x %08x +%u bytes", container->code, container->transaction_id, container->length - PTP_CONTAINER_HDR_SIZE);
			break;
		case ptp_container_response:
			offset = sprintf(buffer, "response %s (%04x) %08x [", PRIVATE_DATA->response_code_label(container->code), container->code, container->transaction_id);
			break;
		case ptp_container_event:
			offset = sprintf(buffer, "event %s (%04x) %08x [", PRIVATE_DATA->event_code_label(container->code), container->code, container->transaction_id);
			break;
		default:
			offset = sprintf(buffer, "unknown %04x %08x", container->code, container->transaction_id);
			break;
	}
	if (container->type == ptp_container_command || container->type == ptp_container_response || container->type == ptp_container_event) {
		if (container->length > 12) {
			offset += sprintf(buffer + offset, "%08x", container->payload.params[0]);
		}
		if (container->length > 16) {
			offset += sprintf(buffer + offset, ", %08x", container->payload.params[1]);
		}
		if (container->length > 20) {
			offset += sprintf(buffer + offset, ", %08x", container->payload.params[2]);
		}
		if (container->length > 24) {
			offset += sprintf(buffer + offset, ", %08x", container->payload.params[3]);
		}
		if (container->length > 28) {
			offset += sprintf(buffer + offset, ", %08x", container->payload.params[4]);
		}
		sprintf(buffer + offset, "]");
	}
	indigo_debug("%s[%d, %s]: %s", DRIVER_NAME, line, function,  buffer);
}

void ptp_dump_device_info(int line, const char *function, indigo_device *device, ptp_device_info *info) {
	indigo_debug("%s[%d, %s]: device info", DRIVER_NAME, line, function);
	indigo_debug("PTP %.2f + %s (%04x)", info->standard_version / 100.0, ptp_vendor_label(info->vendor_extension_id), info->vendor_extension_id);
	indigo_debug("%s [%s], %s, #%s", info->model, info->device_version, info->manufacturer, info->serial_number);
	indigo_debug("operations:");
	for (uint16_t *operation = info->operations_supported; *operation; operation++) {
		indigo_debug("  %04x %s", *operation, PRIVATE_DATA->operation_code_label(*operation));
	}
	indigo_debug("events:");
	for (uint16_t *event = info->events_supported; *event; event++) {
		indigo_debug("  %04x %s", *event, PRIVATE_DATA->event_code_label(*event));
	}
	indigo_debug("properties:");
	for (uint16_t *property = info->properties_supported; *property; property++) {
		indigo_debug("  %04x %s", *property, PRIVATE_DATA->property_code_label(*property));
	}
}

uint8_t *ptp_copy_string(uint8_t *source, char *target) {
	int length = *source++;
	for (int i = 0; i < length; i++) {
		*target++ = *source++;
		source++;
	}
	return source;
}

uint8_t *ptp_copy_uint8(uint8_t *source, uint8_t *target) {
	*target = *source++;
	return source;
}

uint8_t *ptp_copy_uint16(uint8_t *source, uint16_t *target) {
	*target = *(uint16_t *)source;
	return source + sizeof(uint16_t);
}

uint8_t *ptp_copy_uint32(uint8_t *source, uint32_t *target) {
	*target = *(uint32_t *)source;
	return source + sizeof(uint32_t);
}

uint8_t *ptp_copy_uint16_array(uint8_t *source, uint16_t **target, uint32_t *count) {
	uint32_t length;
	source = ptp_copy_uint32(source, &length);
	uint16_t *buffer = malloc((length + 1) * sizeof(uint16_t));
	*target = buffer;
	for (int i = 0; i < length; i++) {
		source = ptp_copy_uint16(source, buffer++);
	}
	*buffer++ = 0;
	if (count)
		*count = length;
	return source;
}

uint8_t *ptp_copy_device_info(uint8_t *source, ptp_device_info *target) {
	source = ptp_copy_uint16(source, &target->standard_version);
	source = ptp_copy_uint32(source, &target->vendor_extension_id);
	source = ptp_copy_uint16(source, &target->vendor_extension_version);
	source = ptp_copy_string(source, target->vendor_extension_desc);
	source = ptp_copy_uint16(source, &target->functional_mode);
	source = ptp_copy_uint16_array(source, &target->operations_supported, NULL);
	source = ptp_copy_uint16_array(source, &target->events_supported, NULL);
	source = ptp_copy_uint16_array(source, &target->properties_supported, NULL);
	source = ptp_copy_uint16_array(source, &target->capture_formats_supported, NULL);
	source = ptp_copy_uint16_array(source, &target->image_formats_supported, NULL);
	source = ptp_copy_string(source, target->manufacturer);
	source = ptp_copy_string(source, target->model);
	source = ptp_copy_string(source, target->device_version);
	source = ptp_copy_string(source, target->serial_number);
	return source;
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
				} else if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
					PRIVATE_DATA->ep_out = address;
				}
			} else if (ep[i].bmAttributes == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
				int address = ep[i].bEndpointAddress;
				if ((address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
					PRIVATE_DATA->ep_int = address;
				}
			}
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PTP EP OUT = %02x IN = %02x INT = %02x", PRIVATE_DATA->ep_out, PRIVATE_DATA->ep_in, PRIVATE_DATA->ep_int);

	}
	if (config_descriptor)
		libusb_free_config_descriptor(config_descriptor);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return true;
}

bool ptp_request(indigo_device *device, uint16_t code, int count, ...) {
	ptp_container request;
	request.length = PTP_CONTAINER_COMMAND_SIZE(count);
	request.type = ptp_container_command;
	request.code = code;
	request.transaction_id = PRIVATE_DATA->transaction_id++;
	int length = 0;
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		request.payload.params[i] = (va_arg(argp, uint32_t));
	va_end(argp);
	for (int i = count; i < 5; i++)
		request.payload.params[i] = 0;
	PTP_DUMP_CONTAINER(&request);
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return rc >= 0;
}

bool ptp_read(indigo_device *device, uint16_t *code, void **data, int *size) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ptp_container header;
	int length = 0;
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&header, sizeof(header), &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
	PTP_DUMP_CONTAINER(&header);
	int total = header.length - PTP_CONTAINER_HDR_SIZE;
	if (code)
		*code = header.code;
	if (size)
		*size = total;
	unsigned char *buffer = malloc(total);
	memcpy(buffer, &header.payload, length);
	int offset = length;
	total -= length;
	while (total > 0) {
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, buffer + offset, total, &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc < 0) {
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			free(buffer);
			return false;
		}
		offset += length;
		total -= length;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	*data = buffer;
	return true;
}

bool ptp_response(indigo_device *device, uint16_t *code, int count, ...) {
	ptp_container response;
	int length = 0;
	memset(&response, 0, sizeof(response));
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, sizeof(response), &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, sizeof(response), &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	if (rc < 0)
		return false;
	PTP_DUMP_CONTAINER(&response);
	if (code)
		*code = response.code;
	va_list argp;
	va_start(argp, count);
	for (int i = 0; i < count; i++)
		*va_arg(argp, uint32_t *) = response.payload.params[i];
	va_end(argp);
	return true;
}

void ptp_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close()");
	PRIVATE_DATA->handle = NULL;
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}
