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
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>

#include <indigo/indigo_ccd_driver.h>

#include "indigo_ptp.h"

#define ADVANCED_GROUP
//#define UNKNOWN_GROUP

char *ptp_type_code_label(uint16_t code) {
	static char *scalar_type_label[] = { "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64", "int128", "uint128" };
	static char *array_type_label[] = { "int8[]", "uint8[]", "int16[]", "uint16[]", "int32[]", "uint32[]", "int64[]", "uint64[]", "int128[]", "uint128[]" };
	if (code == 0)
		return "undef";
	if (code <= ptp_uint128_type)
		return scalar_type_label[code - 1];
	if (code <= ptp_auint128_type)
		return array_type_label[code & 0xFF - 1];
	if (code == 0xFFFF)
		return "string";
	return "undef!";
}

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
	return "???";
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
	return "???";
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
	return "???";
}

char *ptp_property_code_name(uint16_t code) {
	static char label[INDIGO_NAME_SIZE];
	sprintf(label, "%04x", code);
	return label;
}

char *ptp_property_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_Undefined: return "Undefined";
		case ptp_property_BatteryLevel: return "Battery level";
		case ptp_property_FunctionalMode: return "Functional mode";
		case ptp_property_ImageSize: return "Image size";
		case ptp_property_CompressionSetting: return "Compression";
		case ptp_property_WhiteBalance: return "White balance";
		case ptp_property_RGBGain: return "RGBGain";
		case ptp_property_FNumber: return "Aperture";
		case ptp_property_FocalLength: return "Focal length";
		case ptp_property_FocusDistance: return "Focus distance";
		case ptp_property_FocusMode: return "Focus mode";
		case ptp_property_ExposureMeteringMode: return "Metering mode";
		case ptp_property_FlashMode: return "Flash mode";
		case ptp_property_ExposureTime: return "Shutter";
		case ptp_property_ExposureProgramMode: return "Program mode";
		case ptp_property_ExposureIndex: return "ISO";
		case ptp_property_ExposureBiasCompensation: return "Exposure compensation";
		case ptp_property_DateTime: return "DateTime";
		case ptp_property_CaptureDelay: return "CaptureDelay";
		case ptp_property_StillCaptureMode: return "Capture mode";
		case ptp_property_Contrast: return "Contrast";
		case ptp_property_Sharpness: return "Sharpness";
		case ptp_property_DigitalZoom: return "Digital zoom";
		case ptp_property_EffectMode: return "Effect mode";
		case ptp_property_BurstNumber: return "Burst number";
		case ptp_property_BurstInterval: return "Burst interval";
		case ptp_property_TimelapseNumber: return "Timelapse number";
		case ptp_property_TimelapseInterval: return "Timelapse interval";
		case ptp_property_FocusMeteringMode: return "Focus metering mode";
		case ptp_property_UploadURL: return "Upload URL";
		case ptp_property_Artist: return "Artist";
		case ptp_property_CopyrightInfo: return "Copyright info";
		case ptp_property_SupportedStreams: return "Supported streams";
		case ptp_property_EnabledStreams: return "Enabled streams";
		case ptp_property_VideoFormat: return "Video format";
		case ptp_property_VideoResolution: return "Video resolution";
		case ptp_property_VideoQuality: return "Video quality";
		case ptp_property_VideoFrameRate: return "Video frame rate";
		case ptp_property_VideoContrast: return "Video contrast";
		case ptp_property_VideoBrightness: return "Video brightness";
		case ptp_property_AudioFormat: return "Audio format";
		case ptp_property_AudioBitrate: return "Audio bitrate";
		case ptp_property_AudioSamplingRate: return "Audio sampling rate";
		case ptp_property_AudioBitPerSample: return "Audio bit per sample";
		case ptp_property_AudioVolume: return "Audiov olume";
		case ptp_property_MTPSynchronizationPartner: return "MTP synchronization partner";
		case ptp_property_MTPDeviceFriendlyName: return "MTP device friendly name";
		case ptp_property_MTPVolumeLevel: return "MTP bolume level";
		case ptp_property_MTPDeviceIcon: return "MTP device icon";
		case ptp_property_MTPSessionInitiatorInfo: return "MTP session initiator info";
		case ptp_property_MTPPerceivedDeviceType: return "MTP perceived device type";
		case ptp_property_MTPPlaybackRate: return "MTP playback rate";
		case ptp_property_MTPPlaybackObject: return "MTP playback object";
		case ptp_property_MTPPlaybackContainerIndex: return "MTP playback container index";
		case ptp_property_MTPPlaybackPosition: return "MTP playback position";
	}
	static char label[INDIGO_NAME_SIZE];
	sprintf(label, "%04x", code);
	return label;
}

char *ptp_property_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_ExposureProgramMode: {
			switch (code) { case 1: return "Manual"; case 2: return "Program"; case 3: return "Aperture priority"; case 4: return "Shutter priority"; }
			break;
		}
		case ptp_property_WhiteBalance: {
			switch (code) { case 1: return "Manual"; case 2: return "Auto"; case 3: return "One-push Auto"; case 4: return "Daylight"; case 5: return "Fluorescent"; case 6: return "Incandescent"; case 7: return "Flash"; }
			break;
		}
		case ptp_property_ExposureMeteringMode: {
			switch (code) { case 1: return "Average"; case 2: return "Center Weighted Average"; case 3: return "Multi-spot"; case 4: return "Center-spot"; }
			break;
		}
		case ptp_property_FNumber: {
			sprintf(label, "f/%g", code / 100.0);
			return label;
		}
		case ptp_property_ExposureTime: {
			if (code == 0xffffffff)
				return "Bulb";
			if (code == 0xfffffffd)
				return "Time";
			if (code == 1)
				return "1/8000s";
			if (code == 2)
				return "1/4000s";
			if (code == 3)
				return "1/3200s";
			if (code == 6)
				return "1/1600s";
			if (code == 12)
				return "1/800s";
			if (code == 13)
				return "1/750s";
			if (code == 15)
				return "1/640s";
			if (code == 28)
				return "1/350s";
			if (code == 80)
				return "1/125s";
			if (code < 100) {
				sprintf(label, "1/%gs", round(1000.0 / code) * 10);
				return label;
			}
			if (code < 10000) {
				double fraction = 10000.0 / code;
				double integral_part = 0.0;
				double fraction_part = modf(fraction, &integral_part);
				if (fraction_part >= 0.1 && integral_part < 10) {
					// for 1/2.5s, 1/1.6s, 1/1.3s
					sprintf(label, "1/%.1fs", fraction);
				} else {
					sprintf(label, "1/%gs", round(fraction));
				}
				return label;
			}
			sprintf(label, "%gs", code / 10000.0);
			return label;
		}
		case ptp_property_ExposureIndex: {
			sprintf(label, "%lld", code);
			return label;
		}
		case ptp_property_ExposureBiasCompensation: {
			sprintf(label, "%.1f", round((int)code / 100.0) / 10.0);
			return label;
		}
	}
	sprintf(label, "%llx", code);
	return label;
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
	return "???";
}

void ptp_dump_container(int line, const char *function, indigo_device *device, ptp_container *container) {
	char buffer[PTP_MAX_CHARS];
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
			offset = sprintf(buffer, "event %s (%04x) [", PRIVATE_DATA->event_code_label(container->code), container->code);
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
	indigo_debug("%s[%s:%d]: %s", DRIVER_NAME, function, line,  buffer);
}

void ptp_dump_device_info(int line, const char *function, indigo_device *device) {
	indigo_log("%s[%s:%d]: device info", DRIVER_NAME, function, line);
	indigo_log("PTP %.2f + %s (%04x), %s %.2f", PRIVATE_DATA->info_standard_version / 100.0, ptp_vendor_label(PRIVATE_DATA->info_vendor_extension_id), PRIVATE_DATA->info_vendor_extension_id, PRIVATE_DATA->info_vendor_extension_desc, PRIVATE_DATA->info_vendor_extension_version / 100.0);
	indigo_log("%s [%s], %s, #%s", PRIVATE_DATA->info_model, PRIVATE_DATA->info_device_version, PRIVATE_DATA->info_manufacturer, PRIVATE_DATA->info_serial_number);
	indigo_log("operations:");
	for (uint16_t *operation = PRIVATE_DATA->info_operations_supported; *operation; operation++) {
		indigo_log("  %04x %s", *operation, PRIVATE_DATA->operation_code_label(*operation));
	}
	indigo_log("events:");
	for (uint16_t *event = PRIVATE_DATA->info_events_supported; *event; event++) {
		indigo_debug("  %04x %s", *event, PRIVATE_DATA->event_code_label(*event));
	}
	indigo_log("properties:");
	for (uint16_t *property = PRIVATE_DATA->info_properties_supported; *property; property++) {
		indigo_log("  %04x %s", *property, PRIVATE_DATA->property_code_label(*property));
	}
}

uint8_t *ptp_encode_string(char *source, uint8_t *target) {
	uint8_t length = (uint8_t)strlen(source) + 1;
	*target++ = length;
	for (int i = 0; i < length; i++) {
		*target++ = *source++;
		*target++ = 0;
	}
	return target;
}

uint8_t *ptp_encode_uint8(uint8_t source, uint8_t *target) {
	*target++ = source;
	return target;
}

uint8_t *ptp_encode_uint16(uint16_t source, uint8_t *target) {
	*(uint16_t *)target = source;
	return target + sizeof(uint16_t);
}

uint8_t *ptp_encode_uint32(uint32_t source, uint8_t *target) {
	*(uint32_t *)target = source;
	return target + sizeof(uint32_t);
}

uint8_t *ptp_encode_uint64(uint64_t source, uint8_t *target) {
	*(uint64_t *)target = source;
	return target + sizeof(uint64_t);
}

uint8_t *ptp_decode_string(uint8_t *source, char *target) {
	uint8_t length = *source++;
	for (int i = 0; i < length; i++) {
		*target++ = *source++;
		source++;
	}
	return source;
}

uint8_t *ptp_decode_uint8(uint8_t *source, uint8_t *target) {
	*target = *source++;
	return source;
}

uint8_t *ptp_decode_uint16(uint8_t *source, uint16_t *target) {
	*target = *(uint16_t *)source;
	return source + sizeof(uint16_t);
}

uint8_t *ptp_decode_uint32(uint8_t *source, uint32_t *target) {
	*target = *(uint32_t *)source;
	return source + sizeof(uint32_t);
}

uint8_t *ptp_decode_uint64(uint8_t *source, uint64_t *target) {
	*target = *(uint64_t *)source;
	return source + sizeof(uint64_t);
}

uint8_t *ptp_decode_uint128(uint8_t *source, char *target) {
	uint32_t u32_1, u32_2, u32_3, u32_4;
	source = ptp_decode_uint32(source, &u32_1);
	source = ptp_decode_uint32(source, &u32_2);
	source = ptp_decode_uint32(source, &u32_3);
	source = ptp_decode_uint32(source, &u32_4);
	sprintf(target, "%04x%04x%04x%04x", u32_4, u32_3, u32_2, u32_1);
	return source;
}

uint8_t *ptp_decode_uint16_array(uint8_t *source, uint16_t *target, uint32_t *count) {
	uint32_t length;
	source = ptp_decode_uint32(source, &length);
	assert(length < PTP_MAX_ELEMENTS);
	for (int i = 0; i < length; i++) {
		source = ptp_decode_uint16(source, target++);
	}
	*target = 0;
	if (count)
		*count = length;
	return source;
}

uint8_t *ptp_decode_uint32_array(uint8_t *source, uint32_t *target, uint32_t *count) {
	uint32_t length;
	source = ptp_decode_uint32(source, &length);
	assert(length < PTP_MAX_ELEMENTS);
	for (int i = 0; i < length; i++) {
		source = ptp_decode_uint32(source, target++);
	}
	*target = 0;
	if (count)
		*count = length;
	return source;
}

void ptp_append_uint16_16_array(uint16_t *target, uint16_t *source) {
	int index = 0;
	for (index = 0; target[index]; index++)
		;
	for (int i = 0; source[i]; i++)
		target[index++] = source[i];
	target[index] = 0;
}

void ptp_append_uint16_32_array(uint16_t *target, uint32_t *source) {
	int index = 0;
	for (index = 0; target[index]; index++)
		;
	for (int i = 0; source[i]; i++)
		target[index++] = source[i];
	target[index] = 0;
}

uint8_t *ptp_decode_device_info(uint8_t *source, indigo_device *device) {
	source = ptp_decode_uint16(source, &PRIVATE_DATA->info_standard_version);
	source = ptp_decode_uint32(source, &PRIVATE_DATA->info_vendor_extension_id);
	source = ptp_decode_uint16(source, &PRIVATE_DATA->info_vendor_extension_version);
	source = ptp_decode_string(source, PRIVATE_DATA->info_vendor_extension_desc);
	source = ptp_decode_uint16(source, &PRIVATE_DATA->info_functional_mode);
	source = ptp_decode_uint16_array(source, PRIVATE_DATA->info_operations_supported, NULL);
	source = ptp_decode_uint16_array(source, PRIVATE_DATA->info_events_supported, NULL);
	source = ptp_decode_uint16_array(source, PRIVATE_DATA->info_properties_supported, NULL);
	source = ptp_decode_uint16_array(source, PRIVATE_DATA->info_capture_formats_supported, NULL);
	source = ptp_decode_uint16_array(source, PRIVATE_DATA->info_image_formats_supported, NULL);
	source = ptp_decode_string(source, PRIVATE_DATA->info_manufacturer);
	source = ptp_decode_string(source, PRIVATE_DATA->info_model);
	source = ptp_decode_string(source, PRIVATE_DATA->info_device_version);
	source = ptp_decode_string(source, PRIVATE_DATA->info_serial_number);
	if (PRIVATE_DATA->info_vendor_extension_id == ptp_vendor_microsoft) {
		if (strstr(PRIVATE_DATA->info_manufacturer, "Nikon")) {
			PRIVATE_DATA->info_vendor_extension_id = ptp_vendor_nikon;
			PRIVATE_DATA->info_vendor_extension_version = 100;
			strncpy(PRIVATE_DATA->info_vendor_extension_desc, "Nikon & Microsoft PTP Extensions", PTP_MAX_CHARS);
		} else if (strstr(PRIVATE_DATA->info_manufacturer, "Canon")) {
			PRIVATE_DATA->info_vendor_extension_id = ptp_vendor_canon;
			PRIVATE_DATA->info_vendor_extension_version = 100;
			strncpy(PRIVATE_DATA->info_vendor_extension_desc, "Canon & Microsoft PTP Extensions", PTP_MAX_CHARS);
		}
	} else if (strstr(PRIVATE_DATA->info_manufacturer, "Nikon")) {
		PRIVATE_DATA->info_vendor_extension_id = ptp_vendor_nikon;
		PRIVATE_DATA->info_vendor_extension_version = 100;
		strncpy(PRIVATE_DATA->info_vendor_extension_desc, "Nikon Extension", PTP_MAX_CHARS);
	} else if (strstr(PRIVATE_DATA->info_manufacturer, "Sony")) {
		PRIVATE_DATA->info_vendor_extension_id = ptp_vendor_nikon;
		PRIVATE_DATA->info_vendor_extension_version = 100;
		strncpy(PRIVATE_DATA->info_vendor_extension_desc, "Sony Extension", PTP_MAX_CHARS);
	}
	return source;
}

#define CHECK_SENTINEL() if (sentinel <= source) { INDIGO_LOG(indigo_log("check_sentinel failed.")); return sentinel; }

uint8_t *ptp_decode_property(uint8_t *source, uint32_t size, indigo_device *device, ptp_property *target) {
	uint8_t *sentinel = source + size;
	source = ptp_decode_uint16(source, &target->code);
	source = ptp_decode_uint16(source, &target->type);
	source = ptp_decode_uint8(source, &target->writable);
	switch (target->type) {
		case ptp_undef_type: {
			return source;
		}
		case ptp_uint8_type: {
			uint8_t value;
			source = ptp_decode_uint8(source + sizeof(uint8_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int8_type: {
			int8_t value;
			source = ptp_decode_uint8(source + sizeof(uint8_t), (uint8_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint16_type: {
			uint16_t value;
			source = ptp_decode_uint16(source + sizeof(uint16_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int16_type: {
			int16_t value;
			source = ptp_decode_uint16(source + sizeof(uint16_t), (uint16_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint32_type: {
			uint32_t value;
			source = ptp_decode_uint32(source + sizeof(uint32_t), &value);
			target->value.number.value = value;
			break;
		}
		case ptp_int32_type: {
			int32_t value;
			source = ptp_decode_uint32(source + sizeof(uint32_t), (uint32_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint64_type:
		case ptp_int64_type: {
			int64_t value;
			source = ptp_decode_uint64(source + 2 * sizeof(uint32_t), (uint64_t *)&value);
			target->value.number.value = value;
			break;
		}
		case ptp_uint128_type:
		case ptp_int128_type: {
			source = ptp_decode_uint128(source + 4 * sizeof(uint32_t), target->value.text.value);
			break;
		}
		case ptp_str_type: {
			source += *source *2 + 1;
			source = ptp_decode_string(source, target->value.text.value);
			break;
		}
		case ptp_aint8_type:
		case ptp_auint8_type:
		case ptp_aint16_type:
		case ptp_auint16_type:
		case ptp_aint32_type:
		case ptp_auint32_type:
		case ptp_aint64_type:
		case ptp_auint64_type:
		case ptp_aint128_type:
		case ptp_auint128_type:
			INDIGO_LOG(indigo_log("code:%x is array type(%x)", target->code, target->type));
			return source;
		default:
			INDIGO_LOG(indigo_log("Unsupported type=%x", target->type));
			assert(false);
	}
	// Some models are not given a form flag.
	CHECK_SENTINEL();
	source = ptp_decode_uint8(source, &target->form);
	switch (target->form) {
		case ptp_none_form:
			if (target->type <= ptp_uint64_type) {
				target->value.number.min = LONG_MIN;
				target->value.number.max = LONG_MAX;
				target->value.number.step = 0;
			}
			break;
		case ptp_range_form:
			switch (target->type) {
				case ptp_uint8_type: {
					uint8_t min, max, step;
					source = ptp_decode_uint8(source, &min);
					source = ptp_decode_uint8(source, &max);
					source = ptp_decode_uint8(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int8_type: {
					int8_t min, max, step;
					source = ptp_decode_uint8(source, (uint8_t *)&min);
					source = ptp_decode_uint8(source, (uint8_t *)&max);
					source = ptp_decode_uint8(source, (uint8_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint16_type: {
					uint16_t min, max, step;
					source = ptp_decode_uint16(source, &min);
					source = ptp_decode_uint16(source, &max);
					source = ptp_decode_uint16(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int16_type: {
					int16_t min, max, step;
					source = ptp_decode_uint16(source, (uint16_t *)&min);
					source = ptp_decode_uint16(source, (uint16_t *)&max);
					source = ptp_decode_uint16(source, (uint16_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint32_type: {
					uint32_t min, max, step;
					source = ptp_decode_uint32(source, &min);
					source = ptp_decode_uint32(source, &max);
					source = ptp_decode_uint32(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int32_type: {
					int32_t min, max, step;
					source = ptp_decode_uint32(source, (uint32_t *)&min);
					source = ptp_decode_uint32(source, (uint32_t *)&max);
					source = ptp_decode_uint32(source, (uint32_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint64_type: {
					uint64_t min, max, step;
					source = ptp_decode_uint64(source, &min);
					source = ptp_decode_uint64(source, &max);
					source = ptp_decode_uint64(source, &step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_int64_type: {
					int64_t min, max, step;
					source = ptp_decode_uint64(source, (uint64_t *)&min);
					source = ptp_decode_uint64(source, (uint64_t *)&max);
					source = ptp_decode_uint64(source, (uint64_t *)&step);
					target->value.number.min = min;
					target->value.number.max = max;
					target->value.number.step = step;
					break;
				}
				case ptp_uint128_type:
				case ptp_int128_type:
					source += 3 * 4 * sizeof(uint32_t);
					break;
				default:
					assert(false);
			}
			break;
		case ptp_enum_form: {
			uint16_t count;
			source = ptp_decode_uint16(source, &count);
			target->count = count;
			for (int i = 0; i < target->count; i++) {
				switch (target->type) {
					case ptp_uint8_type: {
						uint8_t value;
						source = ptp_decode_uint8(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int8_type: {
						int8_t value;
						source = ptp_decode_uint8(source, (uint8_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_uint16_type: {
						uint16_t value;
						source = ptp_decode_uint16(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int16_type: {
						int16_t value;
						source = ptp_decode_uint16(source, (uint16_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_uint32_type: {
						uint32_t value;
						source = ptp_decode_uint32(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int32_type: {
						int32_t value;
						source = ptp_decode_uint32(source, (uint32_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_uint64_type: {
						uint64_t value;
						source = ptp_decode_uint64(source, &value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_int64_type: {
						int64_t value;
						source = ptp_decode_uint64(source, (uint64_t *)&value);
						target->value.sw.values[i] = value;
						break;
					}
					case ptp_str_type: {
						if (i < 16)
							source = ptp_decode_string(source, target->value.sw_str.values[i]);
						break;
					}
					default:
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unknown target type: %d code=%x", target->type, target->code);
						assert(false);
				}
			}
			break;
		}
	}
	if (PRIVATE_DATA->fix_property)
		PRIVATE_DATA->fix_property(device, target);
	ptp_update_property(device, target);
	return source;
}

uint8_t *ptp_decode_property_value(uint8_t *source, indigo_device *device, ptp_property *target) {
	switch (target->type) {
		case ptp_undef_type: {
			return source;
		}
		case ptp_uint8_type: {
			uint8_t value;
			source = ptp_decode_uint8(source, &value);
			target->value.number.value = value;
			return source;
		}
		case ptp_int8_type: {
			int8_t value;
			source = ptp_decode_uint8(source, (uint8_t *)&value);
			target->value.number.value = value;
			return source;
		}
		case ptp_uint16_type: {
			uint16_t value;
			source = ptp_decode_uint16(source, &value);
			target->value.number.value = value;
			return source;
		}
		case ptp_int16_type: {
			int16_t value;
			source = ptp_decode_uint16(source, (uint16_t *)&value);
			target->value.number.value = value;
			return source;
		}
		case ptp_uint32_type: {
			uint32_t value;
			source = ptp_decode_uint32(source, &value);
			target->value.number.value = value;
			return source;
		}
		case ptp_int32_type: {
			int32_t value;
			source = ptp_decode_uint32(source, (uint32_t *)&value);
			target->value.number.value = value;
			return source;
		}
		case ptp_uint64_type:
		case ptp_int64_type: {
			int64_t value;
			source = ptp_decode_uint64(source, (uint64_t *)&value);
			target->value.number.value = value;
			return source;
		}
		case ptp_uint128_type:
		case ptp_int128_type: {
			source = ptp_decode_uint128(source, target->value.text.value);
			return source;
		}
		case ptp_str_type: {
			source = ptp_decode_string(source, target->value.text.value);
			return source;
		}
		default:
			assert(false);
	}
}

ptp_property *ptp_property_supported(indigo_device *device, uint16_t code) {
	for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++)
		if (PRIVATE_DATA->info_properties_supported[i] == code)
			return PRIVATE_DATA->properties + i;
	return NULL;
}

bool ptp_operation_supported(indigo_device *device, uint16_t code) {
	for (int i = 0; PRIVATE_DATA->info_operations_supported[i]; i++)
		if (PRIVATE_DATA->info_operations_supported[i] == code)
			return true;
	return false;
}

bool ptp_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
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
		config_descriptor = NULL;
	}
// Already sent OpenSession
//	if (rc >= 0 && config_descriptor) {
//		int configuration_value = config_descriptor->bConfigurationValue;
//		rc = libusb_set_configuration(handle, configuration_value);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_configuration(%d) -> %s", configuration_value, rc < 0 ? libusb_error_name(rc) : "OK");
//	}
	if (rc >= 0 && interface) {
		int interface_number = interface->altsetting->bInterfaceNumber;
		for (int i = 0; i < 5; i++) {
			rc = libusb_claim_interface(handle, interface_number);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_claim_interface(%d) -> %s", interface_number, rc < 0 ? libusb_error_name(rc) : "OK");
			if (rc == LIBUSB_ERROR_ACCESS) {
				indigo_send_message(device, "Failed to connect, the camera is probably used by another program!");
				indigo_usleep((i + 1) * ONE_SECOND_DELAY);
			} else {
				break;
			}
		}
// Doesn't work with Sony!!!
//		int alt_settings = config_descriptor->interface->altsetting->bAlternateSetting;
//		rc = libusb_set_interface_alt_setting(handle, interface_number, alt_settings);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_set_interface_alt_setting(%d, %d) -> %s", interface_number, alt_settings, rc < 0 ? libusb_error_name(rc) : "OK");
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
	if (rc < 0 && PRIVATE_DATA->handle) {
			libusb_close(PRIVATE_DATA->handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close()");
			PRIVATE_DATA->handle = NULL;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return rc >= 0;
}

uint32_t ptp_type_size(ptp_type type) {
	switch (type) {
	case ptp_int8_type:
	case ptp_uint8_type:
		return 1;
	case ptp_int16_type:
	case ptp_uint16_type:
		return 2;
	case ptp_int32_type:
	case ptp_uint32_type:
		return 4;
	case ptp_int64_type:
	case ptp_uint64_type:
		return 8;
	case ptp_int128_type:
	case ptp_uint128_type:
		return 16;
	// array of integers
	case ptp_aint8_type:
	case ptp_auint8_type:
	case ptp_aint16_type:
	case ptp_auint16_type:
	case ptp_aint32_type:
	case ptp_auint32_type:
	case ptp_aint64_type:
	case ptp_auint64_type:
	case ptp_aint128_type:
	case ptp_auint128_type:
	default:
		return 0;
	}
}

bool ptp_transaction(indigo_device *device, uint16_t code, int count, uint32_t out_1, uint32_t out_2, uint32_t out_3, uint32_t out_4, uint32_t out_5, void *data_out, uint32_t data_out_size, uint32_t *in_1, uint32_t *in_2, uint32_t *in_3, uint32_t *in_4, uint32_t *in_5, void **data_in, uint32_t *data_in_size) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->handle == NULL)
		return false;
	ptp_container request, response;
	int length = 0;
	memset(&request, 0, sizeof(request));
	request.length = PTP_CONTAINER_COMMAND_SIZE(count);
	request.type = ptp_container_command;
	request.code = code;
	request.transaction_id = PRIVATE_DATA->transaction_id++;
	request.payload.params[0] = out_1;
	request.payload.params[1] = out_2;
	request.payload.params[2] = out_3;
	request.payload.params[3] = out_4;
	request.payload.params[4] = out_5;
	PTP_DUMP_CONTAINER(&request);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
	}
	if (rc < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to send request -> %s", libusb_error_name(rc));
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return false;
	}
	if (data_out) {
		int size = data_out_size;
		request.length = PTP_CONTAINER_HDR_SIZE + data_out_size;
		request.type = ptp_container_data;
		PTP_DUMP_CONTAINER(&request);
		if (size < sizeof(ptp_container) - PTP_CONTAINER_HDR_SIZE) {
			memcpy(request.payload.data, data_out, size);
			rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, request.length, &length, PTP_TIMEOUT);
		} else {
			memcpy(request.payload.data, data_out, sizeof(ptp_container) - PTP_CONTAINER_HDR_SIZE);
			rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, sizeof(ptp_container), &length, PTP_TIMEOUT);
		}
		size -= length - PTP_CONTAINER_HDR_SIZE;
		while (rc >=0 && size > 0) {
			rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_out, (unsigned char *)&request, size, &length, PTP_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer(%d) -> %s", length, rc < 0 ? libusb_error_name(rc) : "OK");
			size -= length;
		}
		if (rc < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to send request -> %s", libusb_error_name(rc));
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			return false;
		}
	}
	while (true) {
		memset(&response, 0, sizeof(response));
		length = 0;
		rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, sizeof(response), &length, PTP_TIMEOUT);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s, %d", rc < 0 ? libusb_error_name(rc) : "OK", length);
		if (rc < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read response -> %s", libusb_error_name(rc));
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			return false;
		}
		if (length == 0)
			continue;
		break;
	}
	PTP_DUMP_CONTAINER(&response);
	if (response.type == ptp_container_data) {
		length -= PTP_CONTAINER_HDR_SIZE;
		int total = response.length - PTP_CONTAINER_HDR_SIZE;
		unsigned char *buffer = indigo_safe_malloc(total);
		memcpy(buffer, &response.payload, length);
		int offset = length;
		if (data_in_size)
			*data_in_size = total;
		total -= length;
		while (total > 0) {
			rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, buffer + offset, total > PTP_MAX_BULK_TRANSFER_SIZE ? PTP_MAX_BULK_TRANSFER_SIZE : total, &length, PTP_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s, %d", rc < 0 ? libusb_error_name(rc) : "OK", length);
			if (rc < 0) {
				free(buffer);
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				return false;
			}
			offset += length;
			total -= length;
		}
		if (data_in)
			*data_in = buffer;
		while (true) {
			memset(&response, 0, sizeof(response));
			length = 0;
			rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_in, (unsigned char *)&response, sizeof(response), &length, PTP_TIMEOUT);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s, %d", rc < 0 ? libusb_error_name(rc) : "OK", length);
			if (rc < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read response -> %s", libusb_error_name(rc));
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				return false;
			}
			if (length == 0)
				continue;
			break;
		}
		PTP_DUMP_CONTAINER(&response);
	}
	if (in_1)
		*in_1 = response.payload.params[0];
	if (in_2)
		*in_2 = response.payload.params[1];
	if (in_3)
		*in_3 = response.payload.params[2];
	if (in_4)
		*in_4 = response.payload.params[3];
	if (in_5)
		*in_5 = response.payload.params[4];
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	PRIVATE_DATA->last_error = response.code;
	return rc >= 0 && response.code == ptp_response_OK;
}

void ptp_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close()");
	PRIVATE_DATA->handle = NULL;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

bool ptp_update_property(indigo_device *device, ptp_property *property) {
	bool define = false, delete = false, update = false;
	if (property->property) {
		if ((property->property->type == INDIGO_SWITCH_VECTOR && property->count == 0) || (property->property->type != INDIGO_SWITCH_VECTOR && property->count != 0)) {
			indigo_release_property(property->property);
			property->property = NULL;
			delete = true;
		}
	}
	if (property->property == NULL) {
		if (property->count >= 0) {
			define = true;
			char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE], group[16];
			indigo_copy_name(name, PRIVATE_DATA->property_code_name(property->code));
			indigo_copy_name(label, PRIVATE_DATA->property_code_label(property->code));
			if (!strncmp(name, "DSLR_", 5)) {
				strcpy(group, "DSLR");
			} else if (!strncmp(name, "CCD_", 4)) {
				strcpy(group, "Camera");
			} else if (!strncmp(name, "ADV_", 4)) {
#ifdef ADVANCED_GROUP
				strcpy(group, "Advanced");
#else
				property->count = -1;
#endif
			} else {
#ifdef UNKNOWN_GROUP
				strcpy(group, "Unknown");
#else
				property->count = -1;
#endif
			}
			indigo_property_perm perm = property->writable ? INDIGO_RW_PERM : INDIGO_RO_PERM;
			if (property->count == 0) {
				if (property->type == ptp_str_type) {
					property->property = indigo_init_text_property(NULL, device->name, name, group, label, INDIGO_OK_STATE, perm, 1);
					indigo_init_text_item(property->property->items, "VALUE", "Value", property->value.text.value);
				} else {
					property->property = indigo_init_number_property(NULL, device->name, name, group, label, INDIGO_OK_STATE, perm, 1);
					indigo_init_number_item(property->property->items, "VALUE", "Value", property->value.number.min, property->value.number.max, property->value.number.step, property->value.number.value);
				}
			} else if (property->count > 0) {
				property->property = indigo_init_switch_property(NULL, device->name, name, group, label, INDIGO_OK_STATE, perm, INDIGO_ONE_OF_MANY_RULE, property->count);
				char str[INDIGO_VALUE_SIZE];
				for (int i = 0; i < property->count; i++) {
					if (property->type == ptp_str_type) {
						strcpy(str, property->value.sw_str.values[i]);
						indigo_init_switch_item(property->property->items + i, str, str, !strcmp(property->value.sw_str.value, str));
					} else {
						indigo_item *item = property->property->items + i;
						sprintf(str, "%llx", property->value.sw.values[i]);
						indigo_init_switch_item(item, str, PRIVATE_DATA->property_value_code_label(device, property->code, property->value.sw.values[i]), property->value.sw.value == property->value.sw.values[i]);
						if (!strcmp(item->label, "+") || !strcmp(item->label, "-")) {
							strcpy(item->name, item->label);
						}
					}
				}
			}
		}
	} else {
		delete = true;
		if (property->count == 0) {
			if (property->type == ptp_str_type) {
				if (strncmp(property->property->items->text.value, property->value.text.value, INDIGO_NAME_SIZE)) {
					indigo_copy_name(property->property->items->text.value, property->value.text.value);
					update = true;
				}
			} else if (property->value.number.min == property->property->items->number.min && property->value.number.max == property->property->items->number.max && property->value.number.step == property->property->items->number.step) {
				if (property->property->items->number.value != property->value.number.value) {
					property->property->items->number.value = property->value.number.value;
					update = true;
				}
			} else {
				property->property->items->number.min = property->value.number.min;
				property->property->items->number.max = property->value.number.max;
				property->property->items->number.step = property->value.number.step;
				property->property->items->number.value = property->value.number.value;
				define = true;
			}
		} else {
			if (property->count > 0) {
				if (property->property->count != property->count) {
					if (property->count > property->property->count)
						property->property = indigo_resize_property(property->property, property->count);
					else
						property->property->count = property->count;
					define = true;
				}
				char str[INDIGO_NAME_SIZE];
				for (int i = 0; i < property->count; i++) {
					if (property->type == ptp_str_type) {
						strcpy(str, property->value.sw_str.values[i]);
						indigo_copy_value(property->property->items[i].label, str);
					} else {
						sprintf(str, "%llx", property->value.sw.values[i]);
						indigo_copy_value(property->property->items[i].label, PRIVATE_DATA->property_value_code_label(device, property->code, property->value.sw.values[i]));
					}
					if (strncmp(property->property->items[i].name, str, INDIGO_NAME_SIZE)) {
						indigo_copy_name(property->property->items[i].name, str);
						define = true;
					}
					if (property->type == ptp_str_type) {
						if (!strcmp(property->value.sw_str.value, property->value.sw_str.values[i]) && !property->property->items[i].sw.value)
							update = true;
						property->property->items[i].sw.value = !strcmp(property->value.sw_str.value, property->value.sw_str.values[i]);
					} else {
						if (property->value.sw.value == property->value.sw.values[i] && !property->property->items[i].sw.value)
							update = true;
						property->property->items[i].sw.value = (property->value.sw.value == property->value.sw.values[i]);
					}
				}
			} else {
				if (IS_CONNECTED) {
					indigo_delete_property(device, property->property, NULL);
					indigo_release_property(property->property);
					property->property = NULL;
					return true;
				}
			}
		}
		if (!property->writable == (property->property->perm == INDIGO_RW_PERM)) {
			property->property->perm = property->writable ? INDIGO_RW_PERM : INDIGO_RO_PERM;
			define = true;
		}
	}
	if (property->type == ptp_str_type)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = str -> %s ('%s')", PRIVATE_DATA->property_code_label(property->code), property->code, property->property ? property->property->name : "NONE", property->value.text.value);
	else if (property->form == ptp_none_form)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld)", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.number.value);
	else if (property->form == ptp_range_form)
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld<%lld<%lld)", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.number.min, property->value.number.value, property->value.number.max);
	else if (property->form == ptp_enum_form) {
		switch (property->count) {
			case -1:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [/])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value);
				break;
			case 0:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value);
				break;
			case 1:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0]);
				break;
			case 2:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1]);
				break;
			case 3:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld, %lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1], property->value.sw.values[2]);
				break;
			case 4:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld, %lld, %lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1], property->value.sw.values[2], property->value.sw.values[3]);
				break;
			case 5:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld, %lld, %lld, %lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1], property->value.sw.values[2], property->value.sw.values[3], property->value.sw.values[4]);
				break;
			case 6:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld, %lld, %lld, %lld, %lld])", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1], property->value.sw.values[2], property->value.sw.values[3], property->value.sw.values[4], property->value.sw.values[4]);
				break;
			default:
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x), type = %s -> %s (%lld: [%lld, %lld, %lld, ..., %lld, %lld, %lld]<%d>)", PRIVATE_DATA->property_code_label(property->code), property->code, ptp_type_code_label(property->type), property->property ? property->property->name : "NONE", property->value.sw.value, property->value.sw.values[0], property->value.sw.values[1], property->value.sw.values[2], property->value.sw.values[property->count - 3], property->value.sw.values[property->count - 2], property->value.sw.values[property->count - 1], property->count);
				break;
		}
	}
	if (IS_CONNECTED) {
		if (define) {
			if (delete)
				indigo_delete_property(device, property->property, NULL);
			indigo_define_property(device, property->property, NULL);
		} else if (update) {
			indigo_update_property(device, property->property, NULL);
		}
	}
	return true;
}

bool ptp_refresh_property(indigo_device *device, ptp_property *property) {
	bool result = false;
	if (property) {
		void *buffer = NULL;
		uint32_t size = 0;
		if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, property->code, &buffer, &size)) {
			result = ptp_decode_property(buffer, size, device, property);
		}
		if (buffer)
			free(buffer);
	}
	return result;
}

bool ptp_get_event(indigo_device *device) {
	ptp_container event;
	int length = 0;
	memset(&event, 0, sizeof(event));
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int rc = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int, (unsigned char *)&event, sizeof(event), &length, PTP_TIMEOUT);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_bulk_transfer() -> %s, %d", rc < 0 ? libusb_error_name(rc) : "OK", length);
	if (rc < 0) {
		rc = libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->ep_int);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_clear_halt() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		return false;
	}
	PTP_DUMP_CONTAINER(&event);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	PRIVATE_DATA->handle_event(device, event.code, event.payload.params);
	return true;
}

static void ptp_check_event(indigo_device *device) {
	ptp_get_event(device);
	indigo_reschedule_timer(device, 0, &PRIVATE_DATA->event_checker);
}

bool ptp_initialise(indigo_device *device) {
	void *buffer = NULL;
	if (ptp_transaction_0_0_i(device, ptp_operation_GetDeviceInfo, &buffer, NULL)) {
		ptp_decode_device_info(buffer, device);
		PTP_DUMP_DEVICE_INFO();
		if (buffer)
			free(buffer);
		buffer = NULL;
		uint16_t *properties = PRIVATE_DATA->info_properties_supported;
		uint32_t size = 0;
		for (int i = 0; properties[i]; i++) {
			if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, properties[i], &buffer, &size)) {
				ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + i);
			}
			if (buffer)
				free(buffer);
			buffer = NULL;
		}
		if (PRIVATE_DATA->initialise == ptp_initialise) {
			indigo_set_timer(device, 0.5, ptp_check_event, &PRIVATE_DATA->event_checker);
		}
		return true;
	}
	if (buffer)
		free(buffer);
	return false;
}

bool ptp_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	switch (code) {
		case ptp_event_ObjectAdded: {
			void *buffer = NULL;
			if (ptp_transaction_1_0_i(device, ptp_operation_GetObjectInfo, params[0], &buffer, NULL)) {
				uint32_t size;
				char filename[PTP_MAX_CHARS];
				uint8_t *source = buffer;
				source = ptp_decode_uint32(source + 8, &size);
				source = ptp_decode_string(source + 40 , filename);
				free(buffer);
				buffer = NULL;
				INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_event_ObjectAdded: handle = %08x, size = %u, name = '%s'", params[0], size, filename);
				if (size && ptp_transaction_1_0_i(device, ptp_operation_GetObject, params[0], &buffer, NULL)) {
					const char *ext = strchr(filename, '.');
					if (PRIVATE_DATA->check_dual_compression != NULL && PRIVATE_DATA->check_dual_compression(device) && ptp_check_jpeg_ext(ext)) {
						if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
							indigo_process_dslr_preview_image(device, buffer, size);
						}
					} else {
						indigo_process_dslr_image(device, buffer, size, ext, false);
						if (PRIVATE_DATA->image_buffer)
							free(PRIVATE_DATA->image_buffer);
						PRIVATE_DATA->image_buffer = buffer;
						buffer = NULL;
					}
					if (DSLR_DELETE_IMAGE_ON_ITEM->sw.value)
						ptp_transaction_1_0(device, ptp_operation_DeleteObject, params[0]);
				}
			}
			if (buffer)
				free(buffer);
			return true;
		}
		case ptp_event_DevicePropChanged: {
			void *buffer = NULL;
			code = params[0];
			INDIGO_DRIVER_LOG(DRIVER_NAME, "ptp_event_DevicePropChanged: code=%s (%04x)", PRIVATE_DATA->property_code_name(code), code);
			uint32_t size = 0;
			for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
				if (PRIVATE_DATA->info_properties_supported[i] == code) {
					if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, code, &buffer, &size)) {
						ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + i);
					}
					break;
				}
			}
			if (buffer)
				free(buffer);
		}
		default:
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%04x)", PRIVATE_DATA->event_code_label(code), code);
			return false;
	}
}

bool ptp_set_property(indigo_device *device, ptp_property *property) {
	switch (property->property->type) {
		case INDIGO_TEXT_VECTOR: {
			strncpy(property->value.text.value, property->property->items->text.value, PTP_MAX_CHARS);
			uint8_t buffer[PTP_MAX_CHARS * 2 + 2];
			uint32_t size = (uint32_t)(ptp_encode_string(property->value.text.value, buffer) - buffer);
			return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, buffer, size);
		}
		case INDIGO_SWITCH_VECTOR: {
			for (int i = 0; i < property->property->count; i++) {
				if (property->property->items[i].sw.value) {
					if (property->type == ptp_str_type) {
						strncpy(property->value.sw_str.value, property->value.sw_str.values[i], PTP_MAX_CHARS);
					} else {
						property->value.sw.value = property->value.sw.values[i];
					}
					break;
				}
			}
			if (property->type == ptp_str_type) {
				uint8_t buffer[2 * PTP_MAX_CHARS + 2];
				uint8_t *end = ptp_encode_string(property->value.sw_str.value, buffer);
				return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, buffer, (uint32_t)(end - buffer));
			}
			return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, &property->value.number.value, ptp_type_size(property->type));
		}
		case INDIGO_NUMBER_VECTOR: {
			property->value.number.value = property->property->items->number.target;
			int64_t value = (int64_t)property->value.number.value;
			switch (property->type) {
				case ptp_int8_type:
				case ptp_uint8_type:
					return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, &value, sizeof(uint8_t));
				case ptp_int16_type:
				case ptp_uint16_type:
					return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, &value, sizeof(uint16_t));
				case ptp_int32_type:
				case ptp_uint32_type:
					return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, &value, sizeof(uint32_t));
				case ptp_int64_type:
				case ptp_uint64_type:
					return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, property->code, &value, sizeof(uint64_t));
			}
		}
		default:
			return false;
	}
}

bool ptp_exposure(indigo_device *device) {
	assert(0);
}

bool ptp_set_host_time(indigo_device *device) {
	if (ptp_property_supported(device, ptp_property_DateTime)) {
		time_t secs = time(NULL);
		struct tm tm = *localtime_r(&secs, &tm);
		char iso_time[16];
		sprintf(iso_time, "%02d%02d%02dT%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		uint8_t buffer[2 * PTP_MAX_CHARS + 2];
		uint8_t *end = ptp_encode_string(iso_time, buffer);
		return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_DateTime, buffer, (uint32_t)(end - buffer));
	}
	return false;
}

bool ptp_check_jpeg_ext(const char *ext) {
	return strcmp(ext, ".JPG") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".JPEG") == 0 || strcmp(ext, ".jpeg") == 0;
}

double timestamp(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void ptp_blob_exposure_timer(indigo_device *device) {
	double finish = timestamp() + (int)CCD_EXPOSURE_ITEM->number.value;
	double remains = finish;
	while (!PRIVATE_DATA->abort_capture && remains > 0) {
		indigo_usleep(10000);
		remains = finish - timestamp();
		if (remains < 0)
			remains = 0;
		double remains_ceil = ceil(remains);
		if (remains_ceil != CCD_EXPOSURE_ITEM->number.value) {
			CCD_EXPOSURE_ITEM->number.value = remains_ceil;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		}
	}
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}
