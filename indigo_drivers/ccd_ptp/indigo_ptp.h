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

#ifndef indigo_ptp_h
#define indigo_ptp_h

#include <indigo/indigo_driver.h>

#define PRIVATE_DATA                ((ptp_private_data *)device->private_data)
#define DRIVER_VERSION              0x0018
#define DRIVER_NAME                 "indigo_ccd_ptp"

#define PTP_TIMEOUT                 10000
#define PTP_MAX_BULK_TRANSFER_SIZE  8388608

typedef enum {
	ptp_container_command =	0x0001,
	ptp_container_data = 0x0002,
	ptp_container_response = 0x0003,
	ptp_container_event = 0x0004
} ptp_contaner_code;

typedef enum {
	ptp_operation_Undefined = 0x1000,
	ptp_operation_GetDeviceInfo = 0x1001,
	ptp_operation_OpenSession = 0x1002,
	ptp_operation_CloseSession = 0x1003,
	ptp_operation_GetStorageIDs = 0x1004,
	ptp_operation_GetStorageInfo = 0x1005,
	ptp_operation_GetNumObjects = 0x1006,
	ptp_operation_GetObjectHandles = 0x1007,
	ptp_operation_GetObjectInfo = 0x1008,
	ptp_operation_GetObject = 0x1009,
	ptp_operation_GetThumb = 0x100A,
	ptp_operation_DeleteObject = 0x100B,
	ptp_operation_SendObjectInfo = 0x100C,
	ptp_operation_SendObject = 0x100D,
	ptp_operation_InitiateCapture = 0x100E,
	ptp_operation_FormatStore = 0x100F,
	ptp_operation_ResetDevice = 0x1010,
	ptp_operation_SelfTest = 0x1011,
	ptp_operation_SetObjectProtection = 0x1012,
	ptp_operation_PowerDown = 0x1013,
	ptp_operation_GetDevicePropDesc = 0x1014,
	ptp_operation_GetDevicePropValue = 0x1015,
	ptp_operation_SetDevicePropValue = 0x1016,
	ptp_operation_ResetDevicePropValue = 0x1017,
	ptp_operation_TerminateOpenCapture = 0x1018,
	ptp_operation_MoveObject = 0x1019,
	ptp_operation_CopyObject = 0x101A,
	ptp_operation_GetPartialObject = 0x101B,
	ptp_operation_InitiateOpenCapture = 0x101C,
	ptp_operation_GetNumDownloadableObjects = 0x9001,
	ptp_operation_GetAllObjectInfo = 0x9002,
	ptp_operation_GetUserAssignedDeviceName = 0x9003,
	ptp_operation_MTPGetObjectPropsSupported = 0x9801,
	ptp_operation_MTPGetObjectPropDesc = 0x9802,
	ptp_operation_MTPGetObjectPropValue = 0x9803,
	ptp_operation_MTPSetObjectPropValue = 0x9804,
	ptp_operation_MTPGetObjPropList = 0x9805,
	ptp_operation_MTPSetObjPropList = 0x9806,
	ptp_operation_MTPGetInterdependendPropdesc = 0x9807,
	ptp_operation_MTPSendObjectPropList = 0x9808,
	ptp_operation_MTPGetObjectReferences = 0x9810,
	ptp_operation_MTPSetObjectReferences = 0x9811,
	ptp_operation_MTPUpdateDeviceFirmware = 0x9812,
	ptp_operation_MTPSkip = 0x9820
} ptp_operation_code;

typedef enum {
	ptp_response_Undefined = 0x2000,
	ptp_response_OK = 0x2001,
	ptp_response_GeneralError = 0x2002,
	ptp_response_SessionNotOpen = 0x2003,
	ptp_response_InvalidTransactionID = 0x2004,
	ptp_response_OperationNotSupported = 0x2005,
	ptp_response_ParameterNotSupported = 0x2006,
	ptp_response_IncompleteTransfer = 0x2007,
	ptp_response_InvalidStorageID = 0x2008,
	ptp_response_InvalidObjectHandle = 0x2009,
	ptp_response_DevicePropNotSupported = 0x200A,
	ptp_response_InvalidObjectFormatCode = 0x200B,
	ptp_response_StoreFull = 0x200C,
	ptp_response_ObjectWriteProtected = 0x200D,
	ptp_response_StoreReadOnly = 0x200E,
	ptp_response_AccessDenied = 0x200F,
	ptp_response_NoThumbnailPresent = 0x2010,
	ptp_response_SelfTestFailed = 0x2011,
	ptp_response_PartialDeletion = 0x2012,
	ptp_response_StoreNotAvailable = 0x2013,
	ptp_response_SpecificationByFormatUnsupported = 0x2014,
	ptp_response_NoValidObjectInfo = 0x2015,
	ptp_response_InvalidCodeFormat = 0x2016,
	ptp_response_UnknownVendorCode = 0x2017,
	ptp_response_CaptureAlreadyTerminated = 0x2018,
	ptp_response_DeviceBusy = 0x2019,
	ptp_response_InvalidParentObject = 0x201A,
	ptp_response_InvalidDevicePropFormat = 0x201B,
	ptp_response_InvalidDevicePropValue = 0x201C,
	ptp_response_InvalidParameter = 0x201D,
	ptp_response_SessionAlreadyOpen = 0x201E,
	ptp_response_TransactionCancelled = 0x201F,
	ptp_response_SpecificationOfDestinationUnsupported = 0x2020,
	ptp_response_MTPUndefined = 0xA800,
	ptp_response_MTPInvalidObjectPropCode = 0xA801,
	ptp_response_MTPInvalidObjectProp_Format = 0xA802,
	ptp_response_MTPInvalidObjectProp_Value = 0xA803,
	ptp_response_MTPInvalidObjectReference = 0xA804,
	ptp_response_MTPInvalidDataset = 0xA806,
	ptp_response_MTPSpecificationByGroupUnsupported = 0xA807,
	ptp_response_MTPSpecificationByDepthUnsupported = 0xA808,
	ptp_response_MTPObjectTooLarge = 0xA809,
	ptp_response_MTPObjectPropNotSupported = 0xA80A
} ptp_response_code;

typedef enum {
	ptp_event_Undefined = 0x4000,
	ptp_event_CancelTransaction = 0x4001,
	ptp_event_ObjectAdded = 0x4002,
	ptp_event_ObjectRemoved = 0x4003,
	ptp_event_StoreAdded = 0x4004,
	ptp_event_StoreRemoved = 0x4005,
	ptp_event_DevicePropChanged = 0x4006,
	ptp_event_ObjectInfoChanged = 0x4007,
	ptp_event_DeviceInfoChanged = 0x4008,
	ptp_event_RequestObjectTransfer = 0x4009,
	ptp_event_StoreFull = 0x400A,
	ptp_event_DeviceReset = 0x400B,
	ptp_event_StorageInfoChanged = 0x400C,
	ptp_event_CaptureComplete = 0x400D,
	ptp_event_UnreportedStatus = 0x400E,
	ptp_event_AppleDeviceUnlocked = 0xC001,
	ptp_event_AppleUserAssignedNameChanged = 0xC002
} ptp_event_code;

typedef enum {
	ptp_property_Undefined = 0x5000,
	ptp_property_BatteryLevel = 0x5001,
	ptp_property_FunctionalMode = 0x5002,
	ptp_property_ImageSize = 0x5003,
	ptp_property_CompressionSetting = 0x5004,
	ptp_property_WhiteBalance = 0x5005,
	ptp_property_RGBGain = 0x5006,
	ptp_property_FNumber = 0x5007,
	ptp_property_FocalLength = 0x5008,
	ptp_property_FocusDistance = 0x5009,
	ptp_property_FocusMode = 0x500A,
	ptp_property_ExposureMeteringMode = 0x500B,
	ptp_property_FlashMode = 0x500C,
	ptp_property_ExposureTime = 0x500D,
	ptp_property_ExposureProgramMode = 0x500E,
	ptp_property_ExposureIndex = 0x500F,
	ptp_property_ExposureBiasCompensation = 0x5010,
	ptp_property_DateTime = 0x5011,
	ptp_property_CaptureDelay = 0x5012,
	ptp_property_StillCaptureMode = 0x5013,
	ptp_property_Contrast = 0x5014,
	ptp_property_Sharpness = 0x5015,
	ptp_property_DigitalZoom = 0x5016,
	ptp_property_EffectMode = 0x5017,
	ptp_property_BurstNumber = 0x5018,
	ptp_property_BurstInterval = 0x5019,
	ptp_property_TimelapseNumber = 0x501A,
	ptp_property_TimelapseInterval = 0x501B,
	ptp_property_FocusMeteringMode = 0x501C,
	ptp_property_UploadURL = 0x501D,
	ptp_property_Artist = 0x501E,
	ptp_property_CopyrightInfo = 0x501F,
	ptp_property_SupportedStreams = 0x5020,
	ptp_property_EnabledStreams = 0x5021,
	ptp_property_VideoFormat = 0x5022,
	ptp_property_VideoResolution = 0x5023,
	ptp_property_VideoQuality = 0x5024,
	ptp_property_VideoFrameRate = 0x5025,
	ptp_property_VideoContrast = 0x5026,
	ptp_property_VideoBrightness = 0x5027,
	ptp_property_AudioFormat = 0x5028,
	ptp_property_AudioBitrate = 0x5029,
	ptp_property_AudioSamplingRate = 0x502A,
	ptp_property_AudioBitPerSample = 0x502B,
	ptp_property_AudioVolume = 0x502C,
	ptp_property_MTPSynchronizationPartner = 0xD401,
	ptp_property_MTPDeviceFriendlyName = 0xD402,
	ptp_property_MTPVolumeLevel = 0xD403,
	ptp_property_MTPDeviceIcon = 0xD405,
	ptp_property_MTPSessionInitiatorInfo = 0xD406,
	ptp_property_MTPPerceivedDeviceType = 0xD407,
	ptp_property_MTPPlaybackRate = 0xD410,
	ptp_property_MTPPlaybackObject = 0xD411,
	ptp_property_MTPPlaybackContainerIndex = 0xD412,
	ptp_property_MTPPlaybackPosition = 0xD413
} ptp_property_code;

typedef enum {
	ptp_vendor_eastman_kodak = 0x0001,
	ptp_vendor_seiko_epson = 0x00002,
	ptp_vendor_agilent = 0x0003,
	ptp_vendor_polaroid = 0x0004,
	ptp_vendor_agfa_gevaert = 0x0005,
	ptp_vendor_microsoft = 0x0006,
	ptp_vendor_equinox = 0x0007,
	ptp_vendor_viewquest = 0x0008,
	ptp_vendor_stmicroelectronics = 0x0009,
	ptp_vendor_nikon = 0x000a,
	ptp_vendor_canon = 0x000b,
	ptp_vendor_fotonation = 0x000c,
	ptp_vendor_pentax = 0x000d,
	ptp_vendor_fuji = 0x000e,
	ptp_vendor_ndd_medical_technologies = 0x0012,
	ptp_vendor_samsung = 0x001a,
	ptp_vendor_parrot = 0x001b,
	ptp_vendor_panasonic = 0x001c,
	ptp_vendor_sony = 0x0011
} ptp_vendor_extension_id;

typedef enum {
	ptp_undef_type = 0x0000,
	ptp_int8_type = 0x0001,
	ptp_uint8_type = 0x0002,
	ptp_int16_type = 0x0003,
	ptp_uint16_type = 0x0004,
	ptp_int32_type = 0x0005,
	ptp_uint32_type = 0x0006,
	ptp_int64_type = 0x0007,
	ptp_uint64_type = 0x0008,
	ptp_int128_type = 0x0009,
	ptp_uint128_type = 0x000A,
	ptp_aint8_type = 0x4001,
	ptp_auint8_type = 0x4002,
	ptp_aint16_type = 0x4003,
	ptp_auint16_type = 0x4004,
	ptp_aint32_type = 0x4005,
	ptp_auint32_type = 0x4006,
	ptp_aint64_type = 0x4007,
	ptp_auint64_type = 0x4008,
	ptp_aint128_type = 0x4009,
	ptp_auint128_type = 0x400A,
	ptp_str_type = 0xFFFF
} ptp_type;

typedef enum {
	ptp_none_form = 0x00,
	ptp_range_form = 0x01,
	ptp_enum_form = 0x02
} ptp_form;

typedef enum {
	ptp_flag_lv = 1
} ptp_capbility_mask;

#define PTP_CONTAINER_HDR_SIZE 							((uint32_t)(2 * sizeof(uint16_t) + 2 * sizeof(uint32_t)))
#define PTP_CONTAINER_COMMAND_SIZE(count) 	((uint32_t)(2 * sizeof(uint16_t) + (2 + count) * sizeof(uint32_t)))
#define PTP_MAX_ELEMENTS										1024
#define PTP_MAX_CHARS												256

typedef struct {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t transaction_id;
	union {
		uint32_t params[5];
		uint8_t data[1024 - PTP_CONTAINER_HDR_SIZE];
	} payload;
} ptp_container;

typedef struct {
	int vendor;
	int product;
	const char *name;
	uint32_t flags;
	int width;
	int height;
	float pixel_size;
} ptp_camera_model;

typedef struct {
	uint16_t code;
	uint16_t type;
	uint8_t form;
	uint8_t writable;
	int count;
	union {
		struct {
			int64_t value, min, max, step;
		} number;
		struct {
			char value[PTP_MAX_CHARS];
		} text;
		struct {
			int64_t value;
			int64_t values[PTP_MAX_ELEMENTS];
		} sw;
		struct {
			char value[PTP_MAX_CHARS];
			char values[16][PTP_MAX_CHARS];
		} sw_str;
	} value;
	indigo_property *property;
} ptp_property;

#define DSLR_DELETE_IMAGE_PROPERTY      (PRIVATE_DATA->dslr_delete_image_property)
#define DSLR_DELETE_IMAGE_ON_ITEM       (DSLR_DELETE_IMAGE_PROPERTY->items + 0)
#define DSLR_DELETE_IMAGE_OFF_ITEM      (DSLR_DELETE_IMAGE_PROPERTY->items + 1)
#define DSLR_MIRROR_LOCKUP_PROPERTY     (PRIVATE_DATA->dslr_mirror_lockup_property)
#define DSLR_MIRROR_LOCKUP_LOCK_ITEM    (DSLR_MIRROR_LOCKUP_PROPERTY->items + 0)
#define DSLR_MIRROR_LOCKUP_UNLOCK_ITEM  (DSLR_MIRROR_LOCKUP_PROPERTY->items + 1)
#define DSLR_ZOOM_PREVIEW_PROPERTY      (PRIVATE_DATA->dslr_zoom_preview_property)
#define DSLR_ZOOM_PREVIEW_ON_ITEM       (DSLR_ZOOM_PREVIEW_PROPERTY->items + 0)
#define DSLR_ZOOM_PREVIEW_OFF_ITEM      (DSLR_ZOOM_PREVIEW_PROPERTY->items + 1)

#define DSLR_LOCK_PROPERTY              (PRIVATE_DATA->dslr_lock_property)
#define DSLR_LOCK_ITEM                  (DSLR_LOCK_PROPERTY->items + 0)
#define DSLR_UNLOCK_ITEM                (DSLR_LOCK_PROPERTY->items + 1)
#define DSLR_AF_PROPERTY                (PRIVATE_DATA->dslr_af_property)
#define DSLR_AF_ITEM                    (DSLR_AF_PROPERTY->items + 0)
#define DSLR_SET_HOST_TIME_PROPERTY     (PRIVATE_DATA->dslr_set_host_time_property)
#define DSLR_SET_HOST_TIME_ITEM         (DSLR_SET_HOST_TIME_PROPERTY->items + 0)

typedef struct {
	void *vendor_private_data;
	indigo_device *focuser;
	libusb_device *dev;
	libusb_device_handle *handle;
	uint8_t ep_in, ep_out, ep_int;
	indigo_property *dslr_delete_image_property;
	indigo_property *dslr_mirror_lockup_property;
	indigo_property *dslr_zoom_preview_property;
	indigo_property *dslr_lock_property;
	indigo_property *dslr_af_property;
	indigo_property *dslr_set_host_time_property;
	ptp_camera_model model;
	pthread_mutex_t usb_mutex;
	uint32_t session_id;
	uint32_t transaction_id;
	uint16_t info_standard_version;
	uint32_t info_vendor_extension_id;
	uint16_t info_vendor_extension_version;
	char info_vendor_extension_desc[PTP_MAX_CHARS];
	char info_manufacturer[PTP_MAX_CHARS];
	char info_model[PTP_MAX_CHARS];
	char info_device_version[PTP_MAX_CHARS];
	char info_serial_number[PTP_MAX_CHARS];
	uint16_t info_functional_mode;
	uint16_t info_operations_supported[PTP_MAX_ELEMENTS];
	uint16_t info_events_supported[PTP_MAX_ELEMENTS];
	uint16_t info_capture_formats_supported[PTP_MAX_ELEMENTS];
	uint16_t info_image_formats_supported[PTP_MAX_ELEMENTS];
	uint16_t info_properties_supported[PTP_MAX_ELEMENTS];
	ptp_property properties[PTP_MAX_ELEMENTS];
	char *(* operation_code_label)(uint16_t code);
	char *(* response_code_label)(uint16_t code);
	char *(* event_code_label)(uint16_t code);
	char *(* property_code_name)(uint16_t code);
	char *(* property_code_label)(uint16_t code);
	char *(* property_value_code_label)(indigo_device *device, uint16_t property, uint64_t code);
	bool (* initialise)(indigo_device *device);
	bool (* handle_event)(indigo_device *device, ptp_event_code code, uint32_t *params);
	bool (* fix_property)(indigo_device *device, ptp_property *property);
	bool (* set_property)(indigo_device *device, ptp_property *property);
	bool (* exposure)(indigo_device *device);
	bool (* liveview)(indigo_device *device);
	bool (* lock)(indigo_device *device);
	bool (* af)(indigo_device *device);
	bool (* zoom)(indigo_device *device);
	bool (* focus)(indigo_device *device, int steps);
	bool (* set_host_time)(indigo_device *device);
	bool (* check_dual_compression)(indigo_device *device);
	indigo_timer *event_checker;
	pthread_mutex_t message_mutex;
	int message_property_index;
	bool abort_capture;
	uint32_t last_error;
	void *image_buffer;
} ptp_private_data;

extern void ptp_dump_container(int line, const char *function, indigo_device *device, ptp_container *container);
extern void ptp_dump_device_info(int line, const char *function, indigo_device *device);

#define PTP_DUMP_CONTAINER(c) INDIGO_DEBUG(ptp_dump_container(__LINE__, __FUNCTION__, device, c))
#define PTP_DUMP_DEVICE_INFO() INDIGO_LOG(ptp_dump_device_info(__LINE__, __FUNCTION__, device))

extern char *ptp_type_code_label(uint16_t code);
extern char *ptp_operation_code_label(uint16_t code);
extern char *ptp_response_code_label(uint16_t code);
extern char *ptp_event_code_label(uint16_t code);
extern char *ptp_property_code_name(uint16_t code);
extern char *ptp_property_code_label(uint16_t code);
extern char *ptp_property_value_code_label(indigo_device *device, uint16_t property, uint64_t code);
extern char *ptp_vendor_label(uint16_t code);

extern uint8_t *ptp_decode_string(uint8_t *source, char *target);
extern uint8_t *ptp_decode_uint8(uint8_t *source, uint8_t *target);
extern uint8_t *ptp_decode_uint16(uint8_t *source, uint16_t *target);
extern uint8_t *ptp_decode_uint32(uint8_t *source, uint32_t *target);
extern uint8_t *ptp_decode_uint64(uint8_t *source, uint64_t *target);
extern uint8_t *ptp_decode_uint128(uint8_t *source, char *target);
extern uint8_t *ptp_decode_uint16_array(uint8_t *source, uint16_t *target, uint32_t *count);
extern uint8_t *ptp_decode_uint32_array(uint8_t *source, uint32_t *target, uint32_t *count);
extern uint8_t *ptp_decode_device_info(uint8_t *source, indigo_device *device);
extern uint8_t *ptp_decode_property(uint8_t *source, uint32_t size, indigo_device *device, ptp_property *target);
extern uint8_t *ptp_decode_property_value(uint8_t *source, indigo_device *device, ptp_property *target);

extern uint8_t *ptp_encode_string(char *source, uint8_t *target);
extern uint8_t *ptp_encode_uint8(uint8_t source, uint8_t *target);
extern uint8_t *ptp_encode_uint16(uint16_t source, uint8_t *target);
extern uint8_t *ptp_encode_uint32(uint32_t source, uint8_t *target);
extern uint8_t *ptp_encode_uint64(uint64_t source, uint8_t *target);

extern void ptp_append_uint16_16_array(uint16_t *target, uint16_t *source);
extern void ptp_append_uint16_32_array(uint16_t *target, uint32_t *source);

extern ptp_property *ptp_property_supported(indigo_device *device, uint16_t code);
extern bool ptp_operation_supported(indigo_device *device, uint16_t code);

extern bool ptp_open(indigo_device *device);
extern bool ptp_transaction(indigo_device *device, uint16_t code, int count, uint32_t out_1, uint32_t out_2, uint32_t out_3, uint32_t out_4, uint32_t out_5, void *data_out, uint32_t data_out_size, uint32_t *in_1, uint32_t *in_2, uint32_t *in_3, uint32_t *in_4, uint32_t *in_5, void **data_in, uint32_t *data_in_sizee);
extern void ptp_close(indigo_device *device);
extern bool ptp_update_property(indigo_device *device, ptp_property *property);
extern bool ptp_refresh_property(indigo_device *device, ptp_property *property);

extern bool ptp_initialise(indigo_device *device);
extern bool ptp_get_event(indigo_device *device);
extern bool ptp_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params);
extern bool ptp_set_property(indigo_device *device, ptp_property *property);
extern bool ptp_exposure(indigo_device *device);
extern bool ptp_set_host_time(indigo_device *device);
extern bool ptp_check_jpeg_ext(const char *ext);

extern void ptp_blob_exposure_timer(indigo_device *device);

#define ptp_transaction_0_0(device, code) ptp_transaction(device, code, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_1_0(device, code, out_1) ptp_transaction(device, code, 1, out_1, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_2_0(device, code, out_1, out_2) ptp_transaction(device, code, 2, out_1, out_2, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_3_0(device, code, out_1, out_2, out_3) ptp_transaction(device, code, 3, out_1, out_2, out_3, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_3_0_i(device, code, out_1, out_2, out_3, data_in, data_size) ptp_transaction(device, code, 3, out_1, out_2, out_3, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, data_in, data_size)
#define ptp_transaction_0_0_i(device, code, data_in, data_size) ptp_transaction(device, code, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, data_in, data_size)
#define ptp_transaction_1_0_i(device, code, out_1, data_in, data_size) ptp_transaction(device, code, 1, out_1, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, data_in, data_size)
#define ptp_transaction_1_1(device, code, out_1, in_1) ptp_transaction(device, code, 1, out_1, 0, 0, 0, 0, NULL, 0, in_1, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_0_0_o(device, code, data_out, data_size) ptp_transaction(device, code, 0, 0, 0, 0, 0, 0, data_out, data_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
#define ptp_transaction_0_1_o(device, code, out_1, data_out, data_size) ptp_transaction(device, code, 1, out_1, 0, 0, 0, 0, data_out, data_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL)

#endif /* indigo_ptp_h */
