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

/** INDIGO PTP Canon implementation
 \file indigo_ptp_canon.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <float.h>
#include <libusb-1.0/libusb.h>

#ifdef INDIGO_MACOS
#include <malloc/malloc.h>
#endif
#ifdef INDIGO_LINUX
#include <malloc.h>
#endif

#include "indigo_ptp.h"
#include "indigo_ptp_canon.h"

#define CANON_PRIVATE_DATA	((canon_private_data *)(PRIVATE_DATA->vendor_private_data))

char *ptp_operation_canon_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_canon_GetStorageIDs: return "GetStorageIDs_Canon";
		case ptp_operation_canon_GetStorageInfo: return "GetStorageInfo_Canon";
		case ptp_operation_canon_GetObjectInfo: return "GetObjectInfo_Canon";
		case ptp_operation_canon_GetObject: return "GetObject_Canon";
		case ptp_operation_canon_DeleteObject: return "DeleteObject_Canon";
		case ptp_operation_canon_FormatStore: return "FormatStore_Canon";
		case ptp_operation_canon_GetPartialObject: return "GetPartialObject_Canon";
		case ptp_operation_canon_GetDeviceInfoEx: return "GetDeviceInfoEx_Canon";
		case ptp_operation_canon_GetObjectInfoEx: return "GetObjectInfoEx_Canon";
		case ptp_operation_canon_GetThumbEx: return "GetThumbEx_Canon";
		case ptp_operation_canon_SendPartialObject: return "SendPartialObject_Canon";
		case ptp_operation_canon_SetObjectAttributes: return "SetObjectAttributes_Canon";
		case ptp_operation_canon_GetObjectTime: return "GetObjectTime_Canon";
		case ptp_operation_canon_SetObjectTime: return "SetObjectTime_Canon";
		case ptp_operation_canon_RemoteRelease: return "RemoteRelease_Canon";
		case ptp_operation_canon_SetDevicePropValueEx: return "SetDevicePropValueEx_Canon";
		case ptp_operation_canon_GetRemoteMode: return "GetRemoteMode_Canon";
		case ptp_operation_canon_SetRemoteMode: return "SetRemoteMode_Canon";
		case ptp_operation_canon_SetEventMode: return "SetEventMode_Canon";
		case ptp_operation_canon_GetEvent: return "GetEvent_Canon";
		case ptp_operation_canon_TransferComplete: return "TransferComplete_Canon";
		case ptp_operation_canon_CancelTransfer: return "CancelTransfer_Canon";
		case ptp_operation_canon_ResetTransfer: return "ResetTransfer_Canon";
		case ptp_operation_canon_PCHDDCapacity: return "PCHDDCapacity_Canon";
		case ptp_operation_canon_SetUILock: return "SetUILock_Canon";
		case ptp_operation_canon_ResetUILock: return "ResetUILock_Canon";
		case ptp_operation_canon_KeepDeviceOn: return "KeepDeviceOn_Canon";
		case ptp_operation_canon_SetNullPacketMode: return "SetNullPacketMode_Canon";
		case ptp_operation_canon_UpdateFirmware: return "UpdateFirmware_Canon";
		case ptp_operation_canon_TransferCompleteDT: return "TransferCompleteDT_Canon";
		case ptp_operation_canon_CancelTransferDT: return "CancelTransferDT_Canon";
		case ptp_operation_canon_SetWftProfile: return "SetWftProfile_Canon";
		case ptp_operation_canon_GetWftProfile: return "GetWftProfile_Canon";
		case ptp_operation_canon_SetProfileToWft: return "SetProfileToWft_Canon";
		case ptp_operation_canon_BulbStart: return "BulbStart_Canon";
		case ptp_operation_canon_BulbEnd: return "BulbEnd_Canon";
		case ptp_operation_canon_RequestDevicePropValue: return "RequestDevicePropValue_Canon";
		case ptp_operation_canon_RemoteReleaseOn: return "RemoteReleaseOn_Canon";
		case ptp_operation_canon_RemoteReleaseOff: return "RemoteReleaseOff_Canon";
		case ptp_operation_canon_RegistBackgroundImage: return "RegistBackgroundImage_Canon";
		case ptp_operation_canon_ChangePhotoStudioMode: return "ChangePhotoStudioMode_Canon";
		case ptp_operation_canon_GetPartialObjectEx: return "GetPartialObjectEx_Canon";
		case ptp_operation_canon_ResetMirrorLockupState: return "ResetMirrorLockupState_Canon";
		case ptp_operation_canon_PopupBuiltinFlash: return "PopupBuiltinFlash_Canon";
		case ptp_operation_canon_EndGetPartialObjectEx: return "EndGetPartialObjectEx_Canon";
		case ptp_operation_canon_MovieSelectSWOn: return "MovieSelectSWOn_Canon";
		case ptp_operation_canon_MovieSelectSWOff: return "MovieSelectSWOff_Canon";
		case ptp_operation_canon_GetCTGInfo: return "GetCTGInfo_Canon";
		case ptp_operation_canon_GetLensAdjust: return "GetLensAdjust_Canon";
		case ptp_operation_canon_SetLensAdjust: return "SetLensAdjust_Canon";
		case ptp_operation_canon_GetMusicInfo: return "GetMusicInfo_Canon";
		case ptp_operation_canon_CreateHandle: return "CreateHandle_Canon";
		case ptp_operation_canon_SendPartialObjectEx: return "SendPartialObjectEx_Canon";
		case ptp_operation_canon_EndSendPartialObjectEx: return "EndSendPartialObjectEx_Canon";
		case ptp_operation_canon_SetCTGInfo: return "SetCTGInfo_Canon";
		case ptp_operation_canon_SetRequestOLCInfoGroup: return "SetRequestOLCInfoGroup_Canon";
		case ptp_operation_canon_SetRequestRollingPitchingLevel: return "SetRequestRollingPitchingLevel_Canon";
		case ptp_operation_canon_GetCameraSupport: return "GetCameraSupport_Canon";
		case ptp_operation_canon_SetRating: return "SetRating_Canon";
		case ptp_operation_canon_RequestInnerDevelopStart: return "RequestInnerDevelopStart_Canon";
		case ptp_operation_canon_RequestInnerDevelopParamChange: return "RequestInnerDevelopParamChange_Canon";
		case ptp_operation_canon_RequestInnerDevelopEnd: return "RequestInnerDevelopEnd_Canon";
		case ptp_operation_canon_GpsLoggingDataMode: return "GpsLoggingDataMode_Canon";
		case ptp_operation_canon_GetGpsLogCurrentHandle: return "GetGpsLogCurrentHandle_Canon";
		case ptp_operation_canon_InitiateViewfinder: return "InitiateViewfinder_Canon";
		case ptp_operation_canon_TerminateViewfinder: return "TerminateViewfinder_Canon";
		case ptp_operation_canon_GetViewFinderData: return "GetViewFinderData_Canon";
		case ptp_operation_canon_DoAf: return "DoAf_Canon";
		case ptp_operation_canon_DriveLens: return "DriveLens_Canon";
		case ptp_operation_canon_DepthOfFieldPreview: return "DepthOfFieldPreview_Canon";
		case ptp_operation_canon_ClickWB: return "ClickWB_Canon";
		case ptp_operation_canon_Zoom: return "Zoom_Canon";
		case ptp_operation_canon_ZoomPosition: return "ZoomPosition_Canon";
		case ptp_operation_canon_SetLiveAfFrame: return "SetLiveAfFrame_Canon";
		case ptp_operation_canon_TouchAfPosition: return "TouchAfPosition_Canon";
		case ptp_operation_canon_SetLvPcFlavoreditMode: return "SetLvPcFlavoreditMode_Canon";
		case ptp_operation_canon_SetLvPcFlavoreditParam: return "SetLvPcFlavoreditParam_Canon";
		case ptp_operation_canon_AfCancel: return "AfCancel_Canon";
		case ptp_operation_canon_SetDefaultCameraSetting: return "SetDefaultCameraSetting_Canon";
		case ptp_operation_canon_GetAEData: return "GetAEData_Canon";
		case ptp_operation_canon_NotifyNetworkError: return "NotifyNetworkError_Canon";
		case ptp_operation_canon_AdapterTransferProgress: return "AdapterTransferProgress_Canon";
		case ptp_operation_canon_TransferComplete2: return "TransferComplete2_Canon";
		case ptp_operation_canon_CancelTransfer2: return "CancelTransfer2_Canon";
		case ptp_operation_canon_FAPIMessageTX: return "FAPIMessageTX_Canon";
		case ptp_operation_canon_FAPIMessageRX: return "FAPIMessageRX_Canon";	}
	return ptp_operation_code_label(code);
}

char *ptp_response_canon_code_label(uint16_t code) {
	switch (code) {
		case ptp_response_canon_UnknownCommand: return "UnknownCommand_Canon";
		case ptp_response_canon_OperationRefused: return "OperationRefused_Canon";
		case ptp_response_canon_LensCover: return "LensCover_Canon";
		case ptp_response_canon_BatteryLow: return "BatteryLow_Canon";
		case ptp_response_canon_NotReady: return "NotReady_Canon";
	}
	return ptp_response_code_label(code);
}

char *ptp_event_canon_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_canon_RequestGetEvent: return "RequestGetEvent_Canon";
		case ptp_event_canon_ObjectAddedEx: return "ObjectAddedEx_Canon";
		case ptp_event_canon_ObjectAddedEx2: return "ObjectAddedEx2_Canon";
		case ptp_event_canon_ObjectRemoved: return "ObjectRemoved_Canon";
		case ptp_event_canon_RequestGetObjectInfoEx: return "RequestGetObjectInfoEx_Canon";
		case ptp_event_canon_StorageStatusChanged: return "StorageStatusChanged_Canon";
		case ptp_event_canon_StorageInfoChanged: return "StorageInfoChanged_Canon";
		case ptp_event_canon_RequestObjectTransfer: return "RequestObjectTransfer_Canon";
		case ptp_event_canon_ObjectInfoChangedEx: return "ObjectInfoChangedEx_Canon";
		case ptp_event_canon_ObjectContentChanged: return "ObjectContentChanged_Canon";
		case ptp_event_canon_PropValueChanged: return "PropValueChanged_Canon";
		case ptp_event_canon_AvailListChanged: return "AvailListChanged_Canon";
		case ptp_event_canon_CameraStatusChanged: return "CameraStatusChanged_Canon";
		case ptp_event_canon_WillSoonShutdown: return "WillSoonShutdown_Canon";
		case ptp_event_canon_ShutdownTimerUpdated: return "ShutdownTimerUpdated_Canon";
		case ptp_event_canon_RequestCancelTransfer: return "RequestCancelTransfer_Canon";
		case ptp_event_canon_RequestObjectTransferDT: return "RequestObjectTransferDT_Canon";
		case ptp_event_canon_RequestCancelTransferDT: return "RequestCancelTransferDT_Canon";
		case ptp_event_canon_StoreAdded: return "StoreAdded_Canon";
		case ptp_event_canon_StoreRemoved: return "StoreRemoved_Canon";
		case ptp_event_canon_BulbExposureTime: return "BulbExposureTime_Canon";
		case ptp_event_canon_RecordingTime: return "RecordingTime_Canon";
		case ptp_event_canon_RequestObjectTransferTS: return "RequestObjectTransferTS_Canon";
		case ptp_event_canon_AfResult: return "AfResult_Canon";
		case ptp_event_canon_CTGInfoCheckComplete: return "CTGInfoCheckComplete_Canon";
		case ptp_event_canon_OLCInfoChanged: return "OLCInfoChanged_Canon";
		case ptp_event_canon_RequestObjectTransferFTP: return "RequestObjectTransferFTP_Canon";
	}
	return ptp_event_code_label(code);
}

char *ptp_property_canon_code_name(uint16_t code) {
	switch (code) {
		case ptp_property_canon_AutoExposureMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_canon_DriveMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		case ptp_property_canon_Aperture: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_canon_ShutterSpeed: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_canon_ImageFormat: return CCD_MODE_PROPERTY_NAME;
		case ptp_property_canon_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		case ptp_property_canon_ISOSpeed: return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_canon_MeteringMode: return DSLR_EXPOSURE_METERING_PROPERTY_NAME;
		case ptp_property_canon_FocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_canon_ExpCompensation: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_canon_BatteryPower: return DSLR_BATTERY_LEVEL_PROPERTY_NAME;
	}
	return ptp_property_canon_code_label(code);
}

char *ptp_property_canon_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_canon_AutoExposureMode: return "Exposure program";
		case ptp_property_canon_DriveMode: return "Capture mode";
		case ptp_property_canon_Aperture: return "Aperture";
		case ptp_property_canon_ShutterSpeed: return "Shutter";
		case ptp_property_canon_ImageFormat: return "Capture mode";
		case ptp_property_canon_WhiteBalance: return "White balance";
		case ptp_property_canon_ISOSpeed: return "ISO";
		case ptp_property_canon_MeteringMode: return "Exposure metering";
		case ptp_property_canon_FocusMode: return "Focus mode";
		case ptp_property_canon_ExpCompensation: return "Exposure compensation";
		case ptp_property_canon_BatteryPower: return "Battery level";
		case ptp_property_canon_ColorTemperature: return "ColorTemperature_Canon";
		case ptp_property_canon_WhiteBalanceAdjustA: return "WhiteBalanceAdjustA_Canon";
		case ptp_property_canon_WhiteBalanceAdjustB: return "WhiteBalanceAdjustB_Canon";
		case ptp_property_canon_WhiteBalanceXA: return "WhiteBalanceXA_Canon";
		case ptp_property_canon_WhiteBalanceXB: return "WhiteBalanceXB_Canon";
		case ptp_property_canon_ColorSpace: return "ColorSpace_Canon";
		case ptp_property_canon_PictureStyle: return "PictureStyle_Canon";
		case ptp_property_canon_BatterySelect: return "BatterySelect_Canon";
		case ptp_property_canon_CameraTime: return "CameraTime_Canon";
		case ptp_property_canon_AutoPowerOff: return "AutoPowerOff_Canon";
		case ptp_property_canon_Owner: return "Owner_Canon";
		case ptp_property_canon_ModelID: return "ModelID_Canon";
		case ptp_property_canon_PTPExtensionVersion: return "PTPExtensionVersion_Canon";
		case ptp_property_canon_DPOFVersion: return "DPOFVersion_Canon";
		case ptp_property_canon_AvailableShots: return "AvailableShots_Canon";
		case ptp_property_canon_CaptureDestination: return "CaptureDestination_Canon";
		case ptp_property_canon_BracketMode: return "BracketMode_Canon";
		case ptp_property_canon_CurrentStorage: return "CurrentStorage_Canon";
		case ptp_property_canon_CurrentFolder: return "CurrentFolder_Canon";
		case ptp_property_canon_ImageFormatCF: return "ImageFormatCF_Canon";
		case ptp_property_canon_ImageFormatSD: return "ImageFormatSD_Canon";
		case ptp_property_canon_ImageFormatExtHD: return "ImageFormatExtHD_Canon";
		case ptp_property_canon_CompressionS: return "CompressionS_Canon";
		case ptp_property_canon_CompressionM1: return "CompressionM1_Canon";
		case ptp_property_canon_CompressionM2: return "CompressionM2_Canon";
		case ptp_property_canon_CompressionL: return "CompressionL_Canon";
		case ptp_property_canon_AEModeDial: return "AEModeDial_Canon";
		case ptp_property_canon_AEModeCustom: return "AEModeCustom_Canon";
		case ptp_property_canon_MirrorUpSetting: return "MirrorUpSetting_Canon";
		case ptp_property_canon_HighlightTonePriority: return "HighlightTonePriority_Canon";
		case ptp_property_canon_AFSelectFocusArea: return "AFSelectFocusArea_Canon";
		case ptp_property_canon_HDRSetting: return "HDRSetting_Canon";
		case ptp_property_canon_PCWhiteBalance1: return "PCWhiteBalance1_Canon";
		case ptp_property_canon_PCWhiteBalance2: return "PCWhiteBalance2_Canon";
		case ptp_property_canon_PCWhiteBalance3: return "PCWhiteBalance3_Canon";
		case ptp_property_canon_PCWhiteBalance4: return "PCWhiteBalance4_Canon";
		case ptp_property_canon_PCWhiteBalance5: return "PCWhiteBalance5_Canon";
		case ptp_property_canon_MWhiteBalance: return "MWhiteBalance_Canon";
		case ptp_property_canon_MWhiteBalanceEx: return "MWhiteBalanceEx_Canon";
		case ptp_property_canon_UnknownPropD14D: return "UnknownPropD14D_Canon";
		case ptp_property_canon_PictureStyleStandard: return "PictureStyleStandard_Canon";
		case ptp_property_canon_PictureStylePortrait: return "PictureStylePortrait_Canon";
		case ptp_property_canon_PictureStyleLandscape: return "PictureStyleLandscape_Canon";
		case ptp_property_canon_PictureStyleNeutral: return "PictureStyleNeutral_Canon";
		case ptp_property_canon_PictureStyleFaithful: return "PictureStyleFaithful_Canon";
		case ptp_property_canon_PictureStyleBlackWhite: return "PictureStyleBlackWhite_Canon";
		case ptp_property_canon_PictureStyleAuto: return "PictureStyleAuto_Canon";
		case ptp_property_canon_PictureStyleUserSet1: return "PictureStyleUserSet1_Canon";
		case ptp_property_canon_PictureStyleUserSet2: return "PictureStyleUserSet2_Canon";
		case ptp_property_canon_PictureStyleUserSet3: return "PictureStyleUserSet3_Canon";
		case ptp_property_canon_PictureStyleParam1: return "PictureStyleParam1_Canon";
		case ptp_property_canon_PictureStyleParam2: return "PictureStyleParam2_Canon";
		case ptp_property_canon_PictureStyleParam3: return "PictureStyleParam3_Canon";
		case ptp_property_canon_HighISOSettingNoiseReduction: return "HighISOSettingNoiseReduction_Canon";
		case ptp_property_canon_MovieServoAF: return "MovieServoAF_Canon";
		case ptp_property_canon_ContinuousAFValid: return "ContinuousAFValid_Canon";
		case ptp_property_canon_Attenuator: return "Attenuator_Canon";
		case ptp_property_canon_UTCTime: return "UTCTime_Canon";
		case ptp_property_canon_Timezone: return "Timezone_Canon";
		case ptp_property_canon_Summertime: return "Summertime_Canon";
		case ptp_property_canon_FlavorLUTParams: return "FlavorLUTParams_Canon";
		case ptp_property_canon_CustomFunc1: return "CustomFunc1_Canon";
		case ptp_property_canon_CustomFunc2: return "CustomFunc2_Canon";
		case ptp_property_canon_CustomFunc3: return "CustomFunc3_Canon";
		case ptp_property_canon_CustomFunc4: return "CustomFunc4_Canon";
		case ptp_property_canon_CustomFunc5: return "CustomFunc5_Canon";
		case ptp_property_canon_CustomFunc6: return "CustomFunc6_Canon";
		case ptp_property_canon_CustomFunc7: return "CustomFunc7_Canon";
		case ptp_property_canon_CustomFunc8: return "CustomFunc8_Canon";
		case ptp_property_canon_CustomFunc9: return "CustomFunc9_Canon";
		case ptp_property_canon_CustomFunc10: return "CustomFunc10_Canon";
		case ptp_property_canon_CustomFunc11: return "CustomFunc11_Canon";
		case ptp_property_canon_CustomFunc12: return "CustomFunc12_Canon";
		case ptp_property_canon_CustomFunc13: return "CustomFunc13_Canon";
		case ptp_property_canon_CustomFunc14: return "CustomFunc14_Canon";
		case ptp_property_canon_CustomFunc15: return "CustomFunc15_Canon";
		case ptp_property_canon_CustomFunc16: return "CustomFunc16_Canon";
		case ptp_property_canon_CustomFunc17: return "CustomFunc17_Canon";
		case ptp_property_canon_CustomFunc18: return "CustomFunc18_Canon";
		case ptp_property_canon_CustomFunc19: return "CustomFunc19_Canon";
		case ptp_property_canon_InnerDevelop: return "InnerDevelop_Canon";
		case ptp_property_canon_MultiAspect: return "MultiAspect_Canon";
		case ptp_property_canon_MovieSoundRecord: return "MovieSoundRecord_Canon";
		case ptp_property_canon_MovieRecordVolume: return "MovieRecordVolume_Canon";
		case ptp_property_canon_WindCut: return "WindCut_Canon";
		case ptp_property_canon_ExtenderType: return "ExtenderType_Canon";
		case ptp_property_canon_OLCInfoVersion: return "OLCInfoVersion_Canon";
		case ptp_property_canon_UnknownPropD19A: return "UnknownPropD19A_Canon";
		case ptp_property_canon_UnknownPropD19C: return "UnknownPropD19C_Canon";
		case ptp_property_canon_UnknownPropD19D: return "UnknownPropD19D_Canon";
		case ptp_property_canon_CustomFuncEx: return "CustomFuncEx_Canon";
		case ptp_property_canon_MyMenu: return "MyMenu_Canon";
		case ptp_property_canon_MyMenuList: return "MyMenuList_Canon";
		case ptp_property_canon_WftStatus: return "WftStatus_Canon";
		case ptp_property_canon_WftInputTransmission: return "WftInputTransmission_Canon";
		case ptp_property_canon_HDDirectoryStructure: return "HDDirectoryStructure_Canon";
		case ptp_property_canon_BatteryInfo: return "BatteryInfo_Canon";
		case ptp_property_canon_AdapterInfo: return "AdapterInfo_Canon";
		case ptp_property_canon_LensStatus: return "LensStatus_Canon";
		case ptp_property_canon_QuickReviewTime: return "QuickReviewTime_Canon";
		case ptp_property_canon_CardExtension: return "CardExtension_Canon";
		case ptp_property_canon_TempStatus: return "TempStatus_Canon";
		case ptp_property_canon_ShutterCounter: return "ShutterCounter_Canon";
		case ptp_property_canon_SpecialOption: return "SpecialOption_Canon";
		case ptp_property_canon_PhotoStudioMode: return "PhotoStudioMode_Canon";
		case ptp_property_canon_SerialNumber: return "SerialNumber_Canon";
		case ptp_property_canon_EVFOutputDevice: return "EVFOutputDevice_Canon";
		case ptp_property_canon_EVFMode: return "EVFMode_Canon";
		case ptp_property_canon_DepthOfFieldPreview: return "DepthOfFieldPreview_Canon";
		case ptp_property_canon_EVFSharpness: return "EVFSharpness_Canon";
		case ptp_property_canon_EVFWBMode: return "EVFWBMode_Canon";
		case ptp_property_canon_EVFClickWBCoeffs: return "EVFClickWBCoeffs_Canon";
		case ptp_property_canon_EVFColorTemp: return "EVFColorTemp_Canon";
		case ptp_property_canon_ExposureSimMode: return "ExposureSimMode_Canon";
		case ptp_property_canon_EVFRecordStatus: return "EVFRecordStatus_Canon";
		case ptp_property_canon_LvAfSystem: return "LvAfSystem_Canon";
		case ptp_property_canon_MovSize: return "MovSize_Canon";
		case ptp_property_canon_LvViewTypeSelect: return "LvViewTypeSelect_Canon";
		case ptp_property_canon_MirrorDownStatus: return "MirrorDownStatus_Canon";
		case ptp_property_canon_MovieParam: return "MovieParam_Canon";
		case ptp_property_canon_MirrorLockupState: return "MirrorLockupState_Canon";
		case ptp_property_canon_FlashChargingState: return "FlashChargingState_Canon";
		case ptp_property_canon_AloMode: return "AloMode_Canon";
		case ptp_property_canon_FixedMovie: return "FixedMovie_Canon";
		case ptp_property_canon_OneShotRawOn: return "OneShotRawOn_Canon";
		case ptp_property_canon_ErrorForDisplay: return "ErrorForDisplay_Canon";
		case ptp_property_canon_AEModeMovie: return "AEModeMovie_Canon";
		case ptp_property_canon_BuiltinStroboMode: return "BuiltinStroboMode_Canon";
		case ptp_property_canon_StroboDispState: return "StroboDispState_Canon";
		case ptp_property_canon_StroboETTL2Metering: return "StroboETTL2Metering_Canon";
		case ptp_property_canon_ContinousAFMode: return "ContinousAFMode_Canon";
		case ptp_property_canon_MovieParam2: return "MovieParam2_Canon";
		case ptp_property_canon_StroboSettingExpComposition: return "StroboSettingExpComposition_Canon";
		case ptp_property_canon_MovieParam3: return "MovieParam3_Canon";
		case ptp_property_canon_LVMedicalRotate: return "LVMedicalRotate_Canon";
		case ptp_property_canon_Artist: return "Artist_Canon";
		case ptp_property_canon_Copyright: return "Copyright_Canon";
		case ptp_property_canon_BracketValue: return "BracketValue_Canon";
		case ptp_property_canon_FocusInfoEx: return "FocusInfoEx_Canon";
		case ptp_property_canon_DepthOfField: return "DepthOfField_Canon";
		case ptp_property_canon_Brightness: return "Brightness_Canon";
		case ptp_property_canon_LensAdjustParams: return "LensAdjustParams_Canon";
		case ptp_property_canon_EFComp: return "EFComp_Canon";
		case ptp_property_canon_LensName: return "LensName_Canon";
		case ptp_property_canon_AEB: return "AEB_Canon";
		case ptp_property_canon_StroboSetting: return "StroboSetting_Canon";
		case ptp_property_canon_StroboWirelessSetting: return "StroboWirelessSetting_Canon";
		case ptp_property_canon_StroboFiring: return "StroboFiring_Canon";
		case ptp_property_canon_LensID: return "LensID_Canon";
		case ptp_property_canon_LCDBrightness: return "LCDBrightness_Canon";
		case ptp_property_canon_CADarkBright: return "CADarkBright_Canon";
		case ptp_property_canon_ExExposureLevelIncrements: return "ExExposureLevelIncrements_Canon";
		case ptp_property_canon_ExISOExpansion: return "ExISOExpansion_Canon";
		case ptp_property_canon_ExFlasSyncSpeedInAvMode: return "ExFlasgSyncSpeedInAvMode_Canon";
		case ptp_property_canon_ExLongExposureNoiseReduction: return "ExLongExposureNoiseReduction_Canon";
		case ptp_property_canon_ExHighISONoiseReduction: return "ExHighISONoiseReduction_Canon";
		case ptp_property_canon_ExHighlightTonePriority: return "ExHHighlightTonePriority_Canon";
		case ptp_property_canon_ExAutoLightingOptimizer: return "ExAutoLightingOptimizer_Canon";
		case ptp_property_canon_ExAFAssistBeamFiring: return "ExAFAssistBeamFiring_Canon";
		case ptp_property_canon_ExAFDuringLiveView: return "ExAFDuringLiveView_Canon";
		case ptp_property_canon_ExMirrorLockup: return "ExMirrorLockup_Canon";
		case ptp_property_canon_ExShutterAELockButton: return "ExShutterAELockButton_Canon";
		case ptp_property_canon_ExSetButtonWhenShooting: return "ExSetButtonWhenShooting_Canon";
		case ptp_property_canon_ExLCDDisplayWhenPowerOn: return "ExLCDDisplayWhenPowerOn_Canon";
		case ptp_property_canon_ExAddOriginalDecisionData: return "ExAddOriginalDecisionData_Canon";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_canon_value_code_label(uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_canon_Aperture: {
			switch (code) {
				case 0x08: return "f/1"; case 0x0B: return "f/1.1"; case 0x0C: return "f/1.2"; case 0x0D: return "f/1.2"; case 0x10: return "f/1.4"; case 0x13: return "f/1.6"; case 0x14: return "f/1.8"; case 0x15: return "f/1.8"; case 0x18: return "f/2"; case 0x1B: return "f/2.2"; case 0x1C: return "f/2.5"; case 0x1D: return "f/2.5"; case 0x20: return "f/2.8"; case 0x23: return "f/3.2"; case 0x24: return "f/3.5"; case 0x25: return "f/3.5"; case 0x28: return "f/4"; case 0x2B: return "f/4.5"; case 0x2C: return "f/4.5"; case 0x2D: return "f/5.0"; case 0x30: return "f/5.6"; case 0x33: return "f/6.3"; case 0x34: return "f/6.7"; case 0x35: return "f/7.1"; case 0x38: return "f/8"; case 0x3B: return "f/9"; case 0x3C: return "f/9.5"; case 0x3D: return "f/10"; case 0x40: return "f/11"; case 0x43: return "f/13"; case 0x44: return "f/13"; case 0x45: return "f/14"; case 0x48: return "f/16"; case 0x4B: return "f/18"; case 0x4C: return "f/19"; case 0x4D: return "f/20"; case 0x50: return "f/22"; case 0x53: return "f/25"; case 0x54: return "f/27"; case 0x55: return "f/29"; case 0x58: return "f/32"; case 0x5B: return "f/36"; case 0x5C: return "f/38"; case 0x5D: return "f/40"; case 0x60: return "f/45"; case 0x63: return "f/51"; case 0x64: return "f/54"; case 0x65: return "f/57"; case 0x68: return "f/64"; case 0x6B: return "f/72"; case 0x6C: return "f/76"; case 0x6D: return "f/80"; case 0x70: return "f/91";
			}
			break;
		}
		case ptp_property_canon_ShutterSpeed: {
			switch (code) {
				case 0x0C: return "Bulb"; case 0x10: return "30s"; case 0x13: return "25s"; case 0x14: return "20s"; case 0x15: return "20s"; case 0x18: return "15s"; case 0x1B: return "13s"; case 0x1C: return "10s"; case 0x1D: return "10s"; case 0x20: return "8s"; case 0x23: return "6s"; case 0x24: return "6s"; case 0x25: return "5s"; case 0x28: return "4s"; case 0x2B: return "3.2s"; case 0x2C: return "3s"; case 0x2D: return "2.5s"; case 0x30: return "2s"; case 0x33: return "1.6s"; case 0x34: return "15s"; case 0x35: return "1.3s"; case 0x38: return "1s"; case 0x3B: return "0.8s"; case 0x3C: return "0.7s"; case 0x3D: return "0.6s"; case 0x40: return "0.5s"; case 0x43: return "0.4s"; case 0x44: return "0.3s"; case 0x45: return "0.3s"; case 0x48: return "1/4s"; case 0x4B: return "1/5s"; case 0x4C: return "1/6s"; case 0x4D: return "1/6s"; case 0x50: return "1/8s"; case 0x53: return "1/10s"; case 0x54: return "1/10s"; case 0x55: return "1/13s"; case 0x58: return "1/15s"; case 0x5B: return "1/20s"; case 0x5C: return "1/20s"; case 0x5D: return "1/25s"; case 0x60: return "1/30s"; case 0x63: return "1/40s"; case 0x64: return "1/45s"; case 0x65: return "1/50s"; case 0x68: return "1/60s"; case 0x6B: return "1/80s"; case 0x6C: return "1/90s"; case 0x6D: return "1/100s"; case 0x70: return "1/125s"; case 0x73: return "1/160s"; case 0x74: return "1/180s"; case 0x75: return "1/200s"; case 0x78: return "1/250s"; case 0x7B: return "1/320s"; case 0x7C: return "1/350s"; case 0x7D: return "1/400s"; case 0x80: return "1/500s"; case 0x83: return "1/640s"; case 0x84: return "1/750s"; case 0x85: return "1/800s"; case 0x88: return "1/1000s"; case 0x8B: return "1/1250s"; case 0x8C: return "1/1500s"; case 0x8D: return "1/1600s"; case 0x90: return "1/2000s"; case 0x93: return "1/2500s"; case 0x94: return "1/3000s"; case 0x95: return "1/3200s"; case 0x98: return "1/4000s"; case 0x9B: return "1/5000s"; case 0x9C: return "1/6000s"; case 0x9D: return "1/6400s"; case 0xA0: return "1/8000s";
			}
			break;
		}
		case ptp_property_canon_ISOSpeed: {
			switch (code) {
				case 0x00: return "Auto"; case 0x40: return "50"; case 0x48: return "100"; case 0x4b: return "125"; case 0x4d: return "160"; case 0x50: return "200"; case 0x53: return "250"; case 0x55: return "320"; case 0x58: return "400"; case 0x5b: return "500"; case 0x5d: return "640"; case 0x60: return "800"; case 0x63: return "1000"; case 0x65: return "1250"; case 0x68: return "1600"; case 0x6b: return "2000"; case 0x6d: return "2500"; case 0x70: return "3200"; case 0x73: return "4000"; case 0x75: return "5000"; case 0x78: return "6400"; case 0x7b: return "8000"; case 0x7d: return "10000"; case 0x80: return "12800"; case 0x83: return "16000"; case 0x85: return "20000"; case 0x88: return "25600"; case 0x90: return "51200"; case 0x98: return "102400";
			}
			break;
		}
		case ptp_property_canon_ExpCompensation: {
			switch (code) {
				case 0x28: return "+5"; case 0x25: return "+4 2/3"; case 0x24: return "+4 1/2"; case 0x23: return "+4 1/3"; case 0x20: return "+4"; case 0x1D: return "+3 2/3"; case 0x1C: return "+3 1/2"; case 0x1B: return "+3 1/3"; case 0x18: return "+3"; case 0x15: return "+2 2/3"; case 0x14: return "+2 1/2"; case 0x13: return "+2 1/3"; case 0x10: return "+2"; case 0x0D: return "+1 2/3"; case 0x0C: return "+1 1/2"; case 0x0B: return "+1 1/3"; case 0x08: return "+1"; case 0x05: return "+2/3"; case 0x04: return "+1/2"; case 0x03: return "+1/3"; case 0x00: return "0"; case 0xFD: return "-1/3"; case 0xFC: return "-1/2"; case 0xFB: return "-2/3"; case 0xF8: return "-1"; case 0xF5: return "-1 1/3"; case 0xF4: return "-1 1/2"; case 0xF3: return "-1 2/3"; case 0xF0: return "-2"; case 0xED: return "-2 1/3"; case 0xEC: return "-2 1/2"; case 0xEB: return "-2 2/3"; case 0xE8: return "-3"; case 0xE5: return "-3 1/3"; case 0xE4: return "-3 1/2"; case 0xE3: return "-3 2/3"; case 0xE0: return "-4"; case 0xDD: return "-4 1/3"; case 0xDC: return "-4 1/2"; case 0xDB: return "-4 2/3"; case 0xD8: return "-5";
			}
			break;
		}
		case ptp_property_canon_AEB: {
			switch (code) {
				case 0x00: return "Off"; case 0x03: return "±1/3"; case 0x04: return "±1/2"; case 0x05: return "±2/3"; case 0x08: return "±1"; case 0x0B: return "±1 1/3"; case 0x0C: return "±1 1/2"; case 0x0D: return "±1 2/3"; case 0x10: return "±2"; case 0x13: return "±2 1/3"; case 0x14: return "±2 1/3"; case 0x15: return "±2 2/3"; case 0x18: return "±3";
			}
			break;
		}
		case ptp_property_canon_MeteringMode: {
			switch (code) {
				case 1: return "Spot"; case 3: return "Evaluative"; case 4: return "Partial"; case 5: return "Center-weighted";
			}
			break;
		}
		case ptp_property_canon_BuiltinStroboMode: {
			switch (code) {
				case 0: return "Normal"; case 1: return "Easy Wireless"; case 2: return "Custom Wireless";
			}
		}
		case ptp_property_canon_AutoExposureMode: {
			switch (code) {
				case 0: return "Program AE"; case 1: return "Shutter Priority AE"; case 2: return "Aperture Priority AE"; case 3: return "Manual Exposure"; case 4: return "Bulb"; case 5: return "Auto DEP AE"; case 6: return "DEP AE"; case 8: return "Lock"; case 9: return "Auto"; case 10: return "Night Scene Portrait"; case 11: return "Sports"; case 12: return "Portrait"; case 13: return "Landscape"; case 14: return "Close-Up"; case 15: return "Flash Off"; case 19: return "Creative Auto"; case 22: return "Scene Intelligent Auto";
			}
			break;
		}
		case ptp_property_canon_EVFWBMode:
		case ptp_property_canon_WhiteBalance: {
			switch (code) {
				case 0: return "Auto"; case 1: return "Daylight"; case 2: return "Cloudy"; case 3: return "Tungsten"; case 4: return "Fluorescent"; case 5: return "Flash"; case 6: return "Manual"; case 8: return "Shade"; case 9: return "Color temperature"; case 10: return "Custom white balance: PC-1"; case 11: return "Custom white balance: PC-2"; case 12: return "Custom white balance: PC-3"; case 15: return "Manual 2"; case 16: return "Manual 3"; case 18: return "Manual 4"; case 19: return "Manual 5"; case 20: return "Custom white balance: PC-4"; case 21: return "Custom white balance: PC-5";
			}
			break;
		}
		case ptp_property_canon_FocusMode: {
			switch (code) {
				case 0: return "One-Shot AF"; case 1: return "AI Servo AF"; case 2: return "AI Focus AF"; case 3: return "Manual";
			}
			break;
		}
		case ptp_property_canon_PictureStyle: {
			switch (code) {
				case 0x81: return "Standard"; case 0x82: return "Portrait"; case 0x83: return "Landscape"; case 0x84: return "Neutral"; case 0x85: return "Faithful"; case 0x86: return "Monochrome"; case 0x87:return "Auto"; case 0x88: return "Fine detail"; case 0x21: return "User 1"; case 0x22: return "User 2"; case 0x23: return "User 3"; case 0x41: return "PC 1"; case 0x42: return "PC 2"; case 0x43: return "PC 3";
			}
			break;
		}
		case ptp_property_canon_ColorSpace: {
			switch (code) {
				case 0x01: return "sRGB"; case 0x02: return "Adobe";
			}
			break;
		}
		case ptp_property_canon_DriveMode: {
			switch (code) {
				case 0x00: return "Single shot"; case 0x01: return "Continuos"; case 0x02: return "Video"; case 0x04: return "High speed continuous"; case 0x05: return "Low speed continuous"; case 0x06: return "Silent"; case 0x07: return "10s self timer + continuous"; case 0x10: return "10s self timer"; case 0x11: return "2s self timer"; case 0x12: return "14fps high speed"; case 0x13: return "Silent single shot"; case 0x14: return "Silent continuous"; case 0x15: return "Silent high speed continuous"; case 0x16: return "Silent low speed continuous";
			}
			break;
		}
		case ptp_property_canon_ImageFormat:
		case ptp_property_canon_ImageFormatCF:
		case ptp_property_canon_ImageFormatSD:
		case ptp_property_canon_ImageFormatExtHD: {
			switch ((code >> 32) & 0xFFFFFFFF) {
				case 0:
					strcpy(label,"");
					break;
				case 0x10010003:
					strcpy(label, "Large fine JPEG + ");
					break;
				case 0x10010002:
					strcpy(label, "Large JPEG + ");
					break;
				case 0x10010103:
					strcpy(label, "Medium fine JPEG + ");
					break;
				case 0x10010102:
					strcpy(label, "Medium JPEG + ");
					break;
				case 0x10010203:
					strcpy(label, "Small fine JPEG + ");
					break;
				case 0x10010202:
					strcpy(label, "Small JPEG + ");
					break;
				case 0x10010503:
					strcpy(label, "M1 fine JPEG + ");
					break;
				case 0x10010502:
					strcpy(label, "M1 JPEG + ");
					break;
				case 0x10010603:
					strcpy(label, "M2 fine JPEG + ");
					break;
				case 0x10010602:
					strcpy(label, "M2 JPEG + ");
					break;
				case 0x10010e03:
					strcpy(label, "S1 fine JPEG + ");
					break;
				case 0x10010e02:
					strcpy(label, "S1 JPEG + ");
					break;
				case 0x10010f03:
					strcpy(label, "S2 fine JPEG + ");
					break;
				case 0x10010f02:
					strcpy(label, "S2 JPEG + ");
					break;
				case 0x10011003:
					strcpy(label, "S3 fine JPEG + ");
					break;
				case 0x10011002:
					strcpy(label, "S3 JPEG + ");
					break;
				case 0x10060002:
					strcpy(label, "CRW + ");
					break;
				case 0x10060004:
					strcpy(label, "RAW + ");
					break;
				case 0x10060104:
					strcpy(label, "MRAW + ");
					break;
				case 0x10060204:
					strcpy(label, "SRAW + ");
					break;
				case 0x10060006:
					strcpy(label, "CR2 + ");
					break;
			}
			switch (code & 0xFFFFFFFF) {
				case 0x10010003:
					strcat(label, "Large fine JPEG");
					break;
				case 0x10010002:
					strcat(label, "Large JPEG");
					break;
				case 0x10010103:
					strcat(label, "Medium fine JPEG");
					break;
				case 0x10010102:
					strcat(label, "Medium JPEG");
					break;
				case 0x10010203:
					strcat(label, "Small fine JPEG");
					break;
				case 0x10010202:
					strcat(label, "Small JPEG");
					break;
				case 0x10010503:
					strcat(label, "M1 fine JPEG");
					break;
				case 0x10010502:
					strcat(label, "M1 JPEG");
					break;
				case 0x10010603:
					strcat(label, "M2 fine JPEG");
					break;
				case 0x10010602:
					strcat(label, "M2 JPEG");
					break;
				case 0x10010e03:
					strcat(label, "S1 fine JPEG");
					break;
				case 0x10010e02:
					strcat(label, "S1 JPEG");
					break;
				case 0x10010f03:
					strcat(label, "S2 fine JPEG");
					break;
				case 0x10010f02:
					strcat(label, "S2 JPEG");
					break;
				case 0x10011003:
					strcat(label, "S3 fine JPEG");
					break;
				case 0x10011002:
					strcat(label, "S3 JPEG");
					break;
				case 0x10060002:
					strcat(label, "CRW");
					break;
				case 0x10060004:
					strcat(label, "RAW");
					break;
				case 0x10060104:
					strcat(label, "MRAW");
					break;
				case 0x10060204:
					strcat(label, "SRAW");
					break;
				case 0x10060006:
					strcat(label, "CR2");
					break;
			}
			return label;
			break;
		}
		case ptp_property_canon_ExExposureLevelIncrements: {
			switch (code) {
				case 0x0: return "1/3"; case 0x1: return "1/2";
			}
			break;
		}
		case ptp_property_canon_ExFlasSyncSpeedInAvMode: {
			switch (code) {
				case 0x0: return "Auto"; case 0x1: return "1/200s - 1/60s auto"; case 0x2: return "1/200s fixed";
			}
			break;
		}
		case ptp_property_canon_StroboETTL2Metering: {
			switch (code) {
				case 0x0: return "Evaluative"; case 0x1: return "Average";
			}
			break;
		}
		case ptp_property_canon_ExLongExposureNoiseReduction: {
			switch (code) {
				case 0x0: return "Off"; case 0x1: return "Auto"; case 0x2: return "On";
			}
			break;
		}
		case ptp_property_canon_ExAFAssistBeamFiring: {
			switch (code) {
				case 0x0: return "Enable"; case 0x1: return "Disable"; case 0x2: return "Enable external flash only"; case 0x3: return "AF assist beam only";
			}
			break;
		}
		case ptp_property_canon_ExAFDuringLiveView: {
			switch (code) {
				case 0x0: return "Disable"; case 0x1: return "Quick mode"; case 0x2: return "Live mode";
			}
			break;
		}
		case ptp_property_canon_ExShutterAELockButton: {
			switch (code) {
				case 0x0: return "AF/AE Lock"; case 0x1: return "AE Lock/AF"; case 0x2: return "AF/AF lock, no AE lock"; case 0x3: return "AE/AF, no AE lock";
			}
			break;
		}
		case ptp_property_canon_ExSetButtonWhenShooting: {
			switch (code) {
				case 0x0: return "Normal (disabled)";
				case 0x1: return "Image quality";
				case 0x2: return "Flash exposure compensation";
				case 0x3: return "LCD monitor On/Off";
				case 0x4: return "Menu display";
				case 0x5: return "ISO speed";
			}
			break;
		}
		case ptp_property_canon_ExLCDDisplayWhenPowerOn: {
			switch (code) {
				case 0x0: return "Display on"; case 0x1: return "Previous display status";
			}
			break;
		}
		case ptp_property_canon_QuickReviewTime: {
			switch (code) {
				case 0x0: return "None"; case 0x2: return "2 s"; case 0x4: return "4 s"; case 0x8: return "8 s"; case 0xFF: return "Hold";
			}
			break;
		}
		case ptp_property_canon_CaptureDestination: {
			switch (code) {
				case 0x1: return "Card"; case 0x4: return "RAM";
			}
			break;
		}
		case ptp_property_canon_EVFOutputDevice: {
			switch (code) {
				case 0x1: return "TFT"; case 0x2: return "PC";
			}
			break;
		}
		case ptp_property_canon_ExISOExpansion:
		case ptp_property_canon_ExAddOriginalDecisionData: {
			switch (code) {
				case 0x0: return "Off"; case 0x1: return "On";
			}
			break;
		}
		case ptp_property_canon_ExAutoLightingOptimizer:
		case ptp_property_canon_MirrorUpSetting:
		case ptp_property_canon_StroboFiring: {
			switch (code) {
				case 0x0: return "Enable"; case 0x1: return "Disable";
			}
			break;
		}
		case ptp_property_canon_ExHighlightTonePriority:
		case ptp_property_canon_ExMirrorLockup:
		case ptp_property_canon_EVFMode: {
			switch (code) {
				case 0x0: return "Disable"; case 0x1: return "Enable";
			}
			break;
		}
		case ptp_property_canon_ExHighISONoiseReduction:
		case ptp_property_canon_AloMode: {
			switch (code) {
				case 0x0: return "Standard"; case 0x1: return "Low"; case 0x2: return "Strong"; case 0x3: return "Disable";
			}
			break;
		}
		case ptp_property_canon_MultiAspect: {
			switch (code) {
				case 0x0: return "3:2"; case 0x1: return "1:1"; case 0x2: return "4:3"; case 0x7: return "16:9";
			}
			break;
		}
		case ptp_property_FNumber: {
			sprintf(label, "f/%g", code / 100.0);
			return label;
		}
		case ptp_property_canon_WhiteBalanceAdjustA:
		case ptp_property_canon_WhiteBalanceAdjustB: {
			sprintf(label, "%d", (int16_t)code);
			return label;
		}
	}
	return ptp_property_value_code_label(property, code);
}

static uint8_t *ptp_copy_image_format(uint8_t *source, uint64_t *target) {
	uint32_t count, size, format, quality, compression;
	source = ptp_decode_uint32(source, &count);
	source = ptp_decode_uint32(source, &size);
	source = ptp_decode_uint32(source, &format);
	source = ptp_decode_uint32(source, &quality);
	source = ptp_decode_uint32(source, &compression);
	uint64_t value = size << 24 | format << 16 | quality << 8 | compression;
	if (count == 2) {
		source = ptp_decode_uint32(source, &size);
		source = ptp_decode_uint32(source, &format);
		source = ptp_decode_uint32(source, &quality);
		source = ptp_decode_uint32(source, &compression);
		value = value << 32 | size << 24 | format << 16 | quality << 8 | compression;
	}
	*target = value;
	return source;
}

static void ptp_canon_get_event(indigo_device *device) {
	void *buffer = NULL;
	if (ptp_transaction_0_0_i(device, ptp_operation_canon_GetEvent, &buffer)) {
#ifdef INDIGO_MACOS
		unsigned long max_size = malloc_size(buffer);
#endif
#ifdef INDIGO_LINUX
		unsigned long max_size = malloc_usable_size(buffer);
#endif
		uint8_t *record = buffer;
		ptp_property *updated[PTP_MAX_ELEMENTS] = { NULL }, **next_updated = updated;
		while (true) {
			if (record - (uint8_t *)buffer >= max_size)
				break;
			uint8_t *source = record;
			uint32_t size, type;
			source = ptp_decode_uint32(source, &size);
			source = ptp_decode_uint32(source, &type);
			if (size <= 8 || type == 0)
				break;
			switch (type) {
				case ptp_event_canon_PropValueChanged: {
					uint32_t code;
					source = ptp_decode_uint32(source, &code);
					INDIGO_DRIVER_LOG(DRIVER_NAME, "PropValueChanged %04x (%s)", code, PRIVATE_DATA->property_code_label(code));
					ptp_property *property = NULL;
					for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
						if (PRIVATE_DATA->info_properties_supported[i] == code) {
							property = PRIVATE_DATA->properties + i;
							break;
						}
					}
					if (property == NULL)
						break;
					property->code = code;
					switch (code) {
						case ptp_property_canon_BatteryPower:
						case ptp_property_canon_BatterySelect:
						case ptp_property_canon_ModelID:
						case ptp_property_canon_PTPExtensionVersion:
						case ptp_property_canon_DPOFVersion:
						case ptp_property_canon_AvailableShots:
						case ptp_property_canon_CurrentStorage:
						case ptp_property_canon_CurrentFolder:
						case ptp_property_canon_MyMenu:
						case ptp_property_canon_MyMenuList:
						case ptp_property_canon_HDDirectoryStructure:
						case ptp_property_canon_BatteryInfo:
						case ptp_property_canon_AdapterInfo:
						case ptp_property_canon_AutoExposureMode:
						case ptp_property_canon_LensStatus:
						case ptp_property_canon_CardExtension:
						case ptp_property_canon_TempStatus:
						case ptp_property_canon_ShutterCounter:
						case ptp_property_canon_SerialNumber:
						case ptp_property_canon_DepthOfFieldPreview:
						case ptp_property_canon_EVFRecordStatus:
						case ptp_property_canon_LvAfSystem:
						case ptp_property_canon_FocusInfoEx:
						case ptp_property_canon_DepthOfField:
						case ptp_property_canon_Brightness:
						case ptp_property_canon_EFComp:
						case ptp_property_canon_LensName:
						case ptp_property_canon_LensID:
						case ptp_property_canon_CustomFunc1:
						case ptp_property_canon_CustomFunc2:
						case ptp_property_canon_CustomFunc3:
						case ptp_property_canon_CustomFunc4:
						case ptp_property_canon_CustomFunc5:
						case ptp_property_canon_CustomFunc6:
						case ptp_property_canon_CustomFunc7:
						case ptp_property_canon_CustomFunc8:
						case ptp_property_canon_CustomFunc9:
						case ptp_property_canon_CustomFunc10:
						case ptp_property_canon_CustomFunc11:
						case ptp_property_canon_CustomFunc12:
						case ptp_property_canon_CustomFunc13:
						case ptp_property_canon_CustomFunc14:
						case ptp_property_canon_CustomFunc15:
						case ptp_property_canon_CustomFunc16:
						case ptp_property_canon_CustomFunc17:
						case ptp_property_canon_CustomFunc18:
						case ptp_property_canon_CustomFunc19:
						case ptp_property_canon_CustomFuncEx:
						case ptp_property_canon_AutoPowerOff:
						case ptp_property_canon_DriveMode:
						case ptp_property_canon_EVFMode:
						case ptp_property_canon_EVFOutputDevice:
							property->writable = false;
							break;
						default:
							property->writable = true;
							break;
					}
					switch (code) {
						case ptp_property_canon_PictureStyle:
						case ptp_property_canon_WhiteBalance:
						case ptp_property_canon_MeteringMode:
						case ptp_property_canon_ExpCompensation:
							property->type = ptp_uint8_type;
							break;
						case ptp_property_canon_Aperture:
						case ptp_property_canon_ShutterSpeed:
						case ptp_property_canon_ISOSpeed:
						case ptp_property_canon_FocusMode:
						case ptp_property_canon_ColorSpace:
						case ptp_property_canon_BatteryPower:
						case ptp_property_canon_BatterySelect:
						case ptp_property_canon_PTPExtensionVersion:
						case ptp_property_canon_DriveMode:
						case ptp_property_canon_AEB:
						case ptp_property_canon_BracketMode:
						case ptp_property_canon_QuickReviewTime:
						case ptp_property_canon_EVFMode:
						case ptp_property_canon_EVFOutputDevice:
						case ptp_property_canon_AutoPowerOff:
						case ptp_property_canon_EVFRecordStatus:
						case ptp_property_canon_AutoExposureMode:
						case ptp_property_canon_MirrorUpSetting:
						case ptp_property_canon_MirrorLockupState:
							property->type = ptp_uint16_type;
							break;
						case ptp_property_canon_WhiteBalanceAdjustA:
						case ptp_property_canon_WhiteBalanceAdjustB:
							property->type = ptp_int32_type;
							break;
						case ptp_property_canon_CameraTime:
						case ptp_property_canon_UTCTime:
						case ptp_property_canon_Summertime:
						case ptp_property_canon_AvailableShots:
						case ptp_property_canon_CaptureDestination:
						case ptp_property_canon_WhiteBalanceXA:
						case ptp_property_canon_WhiteBalanceXB:
						case ptp_property_canon_CurrentStorage:
						case ptp_property_canon_CurrentFolder:
						case ptp_property_canon_ShutterCounter:
						case ptp_property_canon_ModelID:
						case ptp_property_canon_LensID:
						case ptp_property_canon_StroboFiring:
						case ptp_property_canon_AFSelectFocusArea:
						case ptp_property_canon_ContinousAFMode:
						case ptp_property_canon_ColorTemperature:
						case ptp_property_canon_WftStatus:
						case ptp_property_canon_LensStatus:
						case ptp_property_canon_CardExtension:
						case ptp_property_canon_TempStatus:
						case ptp_property_canon_PhotoStudioMode:
						case ptp_property_canon_DepthOfFieldPreview:
						case ptp_property_canon_EVFSharpness:
						case ptp_property_canon_EVFWBMode:
						case ptp_property_canon_EVFClickWBCoeffs:
						case ptp_property_canon_EVFColorTemp:
						case ptp_property_canon_ExposureSimMode:
						case ptp_property_canon_LvAfSystem:
						case ptp_property_canon_MovSize:
						case ptp_property_canon_DepthOfField:
						case ptp_property_canon_LvViewTypeSelect:
						case ptp_property_canon_AloMode:
						case ptp_property_canon_Brightness:
						case ptp_property_canon_BuiltinStroboMode:
						case ptp_property_canon_StroboETTL2Metering:
						case ptp_property_canon_MultiAspect:
							property->type = ptp_uint32_type;
							break;
						case ptp_property_canon_Owner:
						case ptp_property_canon_Artist:
						case ptp_property_canon_Copyright:
						case ptp_property_canon_SerialNumber:
						case ptp_property_canon_LensName:
							property->type = ptp_str_type;
							break;
						default:
							property->type = ptp_undef_type;
							break;
					}
					switch (property->type) {
						case ptp_uint8_type: {
							uint8_t value = 0;
							source = ptp_decode_uint8(source, &value);
							property->value.number.value = value;
							break;
						}
						case ptp_uint16_type: {
							uint16_t value = 0;
							source = ptp_decode_uint16(source, &value);
							property->value.number.value = value;
							break;
						}
						case ptp_int16_type: {
							int16_t value = 0;
							source = ptp_decode_uint16(source, (uint16_t *)&value);
							property->value.number.value = value;
							break;
						}
						case ptp_uint32_type: {
							uint32_t value = 0;
							source = ptp_decode_uint32(source, &value);
							property->value.number.value = value;
							break;
						}
						case ptp_int32_type: {
							int32_t value = 0;
							source = ptp_decode_uint32(source, (uint32_t *)&value);
							property->value.number.value = value;
							break;
						}
						case ptp_str_type: {
							strncpy((char *)property->value.text.value, (char *)source, PTP_MAX_CHARS);
							break;
						}
						case ptp_undef_type: {
							switch (code) {
								case ptp_property_canon_ImageFormat:
								case ptp_property_canon_ImageFormatCF:
								case ptp_property_canon_ImageFormatSD:
								case ptp_property_canon_ImageFormatExtHD: {
									uint64_t value = 0;
									source = ptp_copy_image_format(source, &value);
									property->value.number.value = value;
									break;
								}
								case ptp_property_canon_CustomFuncEx: {
									property = NULL;
									source += sizeof(uint32_t);
									unsigned int *source_uint32 = (unsigned int *)source;
									int offset = 0;
									unsigned int group_count = source_uint32[offset++];
									assert(group_count <= 16);
									for (int i = 0; i < group_count; i++) {
										unsigned int group = source_uint32[offset++] - 1;
										unsigned int group_size = source_uint32[offset++];
										assert(group_size <= 1024);
										CANON_PRIVATE_DATA->ex_func_group[group][0] = group_size + 12; // total size
										CANON_PRIVATE_DATA->ex_func_group[group][1] = 1; // group count
										CANON_PRIVATE_DATA->ex_func_group[group][2] = group; // group
										CANON_PRIVATE_DATA->ex_func_group[group][3] = group_size; // group size
										memcpy(CANON_PRIVATE_DATA->ex_func_group[group] + 4, source_uint32 + offset, group_size - 4);
										unsigned int item_count = source_uint32[offset++];
										for (int j = 0; j < item_count; j++) {
											unsigned short item_code = source_uint32[offset++] | 0x8000;
											unsigned int value_size = source_uint32[offset++];
											int index = 0;
											for (index = 0; PRIVATE_DATA->info_properties_supported[index]; index++) {
												if (PRIVATE_DATA->info_properties_supported[index] == item_code)
													break;
											}
											if (PRIVATE_DATA->info_properties_supported[index] == 0) {
												PRIVATE_DATA->info_properties_supported[index] = item_code;
												PRIVATE_DATA->info_properties_supported[index + 1] = 0;
											}
											ptp_property *ex_property = PRIVATE_DATA->properties + index;
											ex_property->code = item_code;
											ex_property->type = ptp_int32_type;
											ex_property->count = 0;
											ex_property->writable = true;
											ex_property->value.sw.value = source_uint32[offset];
											offset += value_size;
											*next_updated++ = ex_property;
											INDIGO_DRIVER_LOG(DRIVER_NAME, "PropValueChanged %04x (%s)", item_code, PRIVATE_DATA->property_code_label(item_code));
										}
									}
									break;
								}
							}
							break;
						}
					}
					if (code == ptp_property_canon_AutoExposureMode) {
						CANON_PRIVATE_DATA->mode = (int)property->value.sw.value;
					}
					if (code == ptp_property_canon_BatteryPower) {
						switch (property->value.number.value) {
							case 1:
								property->value.number.value = 50;
								break;
							case 2:
								property->value.number.value = 100;
								break;
							case 4:
								property->value.number.value = 75;
								break;
							case 5:
								property->value.number.value = 25;
								break;
							default:
								property->value.number.value = 0;
								break;
						}
					}
					if (property)
						*next_updated++ = property;
					break;
				}
				case ptp_event_canon_AvailListChanged: {
					uint32_t code, type, count;
					source = ptp_decode_uint32(source, &code);
					INDIGO_DRIVER_LOG(DRIVER_NAME, "AvailListChanged %04x (%s)", code, PRIVATE_DATA->property_code_label(code));
					ptp_property *property = NULL;
					for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
						if (PRIVATE_DATA->info_properties_supported[i] == code) {
							property = PRIVATE_DATA->properties + i;
							break;
						}
					}
					if (property == NULL)
						break;
					property->code = code;
					source = ptp_decode_uint32(source, &type);
					source = ptp_decode_uint32(source, &count);
					if (count >= PTP_MAX_ELEMENTS) {
						break;
					}
					property->count = count;
					if (property->count > 0) {
						if (code == ptp_property_canon_ImageFormat || code == ptp_property_canon_ImageFormatCF || code == ptp_property_canon_ImageFormatSD || code == ptp_property_canon_ImageFormatExtHD) {
							for (int i = 0; i < count; i++)
								source = ptp_copy_image_format(source, (uint64_t *)&property->value.sw.values[i]);
						} else {
							if (type == 1) {
								for (int i = 0; i < count; i++) {
									property->value.sw.values[i] = 0;
									source = ptp_decode_uint16(source, (uint16_t *)&property->value.sw.values[i]);
								}
							} else if (type == 3) {
								for (int i = 0; i < count; i++) {
									property->value.sw.values[i] = 0;
									source = ptp_decode_uint32(source, (uint32_t *)&property->value.sw.values[i]);
								}
							} else {
								INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unsupported type %04x", type);
							}
						}
					}
					*next_updated++ = property;
					break;
				}
				default:
					INDIGO_DRIVER_LOG(DRIVER_NAME, "%04x +%d skipped", type, size);
			}
			
			record += size;
		}
		for (ptp_property **property = updated; *property; property++) {
			switch ((*property)->code) {
				case ptp_property_canon_AutoExposureMode:
				case ptp_property_canon_DriveMode: {
					if (!(*property)->writable) {
						(*property)->count = 1;
						(*property)->value.sw.values[0] = (*property)->value.sw.value;
					}
					break;
				}
				case ptp_property_canon_Aperture: {
					if ((CANON_PRIVATE_DATA->mode != 2 && CANON_PRIVATE_DATA->mode != 3 && CANON_PRIVATE_DATA->mode != 4) || (*property)->value.sw.value == 0)
						(*property)->count = -1;
					break;
				}
				case ptp_property_canon_ShutterSpeed: {
					if (CANON_PRIVATE_DATA->mode == 4) {
						(*property)->count = 1;
						(*property)->value.sw.value = (*property)->value.sw.values[0] = 0x0C; // BULB
					} else  if (CANON_PRIVATE_DATA->mode != 1 && CANON_PRIVATE_DATA->mode != 3) {
						(*property)->count = -1;
					}
					break;
				}
				case ptp_property_canon_ISOSpeed:
				case ptp_property_canon_MeteringMode:
				case ptp_property_canon_EVFWBMode:
				case ptp_property_canon_WhiteBalance:
				case ptp_property_canon_PictureStyle: {
					if (CANON_PRIVATE_DATA->mode >= 8)
						(*property)->count = -1;
					break;
				}
				case ptp_property_canon_FocusMode: {
					if (CANON_PRIVATE_DATA->mode >= 8 || (*property)->value.sw.value == 3)
						(*property)->count = -1;
					else
						(*property)->count = 3; // AvailListChanged is not sent again
					break;
				}
				case ptp_property_canon_ExpCompensation: {
					if (CANON_PRIVATE_DATA->mode >= 8 || CANON_PRIVATE_DATA->mode == 3)
						(*property)->count = -1;
					break;
				}
				case ptp_property_canon_ExExposureLevelIncrements:
				case ptp_property_canon_ExLCDDisplayWhenPowerOn:
				case ptp_property_canon_ExHighlightTonePriority:
				case ptp_property_canon_ExAddOriginalDecisionData:
				case ptp_property_canon_ExISOExpansion:
				case ptp_property_canon_ExAutoLightingOptimizer:
				case ptp_property_canon_ExMirrorLockup: {
					(*property)->count = 2;
					for (int i = 0; i < 2; i++)
						(*property)->value.sw.values[i] = i;
					break;
				}
				case ptp_property_canon_ExFlasSyncSpeedInAvMode:
				case ptp_property_canon_ExLongExposureNoiseReduction:
				case ptp_property_canon_ExAFDuringLiveView: {
					(*property)->count = 3;
					for (int i = 0; i < 3; i++)
						(*property)->value.sw.values[i] = i;
					break;
				}
				case ptp_property_canon_ExAFAssistBeamFiring:
				case ptp_property_canon_ExHighISONoiseReduction:
				case ptp_property_canon_ExShutterAELockButton: {
					(*property)->count = 4;
					for (int i = 0; i < 4; i++)
						(*property)->value.sw.values[i] = i;
					break;
				}
				case ptp_property_canon_ExSetButtonWhenShooting: {
					(*property)->count = 5;
					for (int i = 0; i < 5; i++)
						(*property)->value.sw.values[i] = i;
					break;
				}
			}
			ptp_update_property(device, *property);
		}
	}
	if (buffer)
		free(buffer);
	buffer = NULL;
}

static void ptp_canon_check_event(indigo_device *device) {
	ptp_canon_get_event(device);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->event_checker);
}

bool ptp_canon_initialise(indigo_device *device) {
	PRIVATE_DATA->vendor_private_data = malloc(sizeof(canon_private_data));
	memset(CANON_PRIVATE_DATA, 0, sizeof(canon_private_data));
	if (!ptp_initialise(device))
		return false;
	void *buffer = NULL;
	if (!ptp_transaction_0_0_i(device, ptp_operation_canon_GetDeviceInfoEx, &buffer)) {
		if (buffer)
			free(buffer);
		return false;
	}
	uint8_t *source = buffer + sizeof(uint32_t);
	uint32_t events[PTP_MAX_ELEMENTS], properties[PTP_MAX_ELEMENTS];
	source = ptp_decode_uint32_array(source, events, NULL);
	ptp_append_uint16_32_array(PRIVATE_DATA->info_events_supported, events);
	source = ptp_decode_uint32_array(source, properties, NULL);
	ptp_append_uint16_32_array(PRIVATE_DATA->info_properties_supported, properties);
	if (buffer)
		free(buffer);
	buffer = NULL;
	INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));
	INDIGO_LOG(indigo_log("events:"));
	for (uint32_t *event = events; *event; event++) {
		INDIGO_LOG(indigo_log("  %04x %s", *event, PRIVATE_DATA->event_code_label(*event)));
	}
	INDIGO_LOG(indigo_log("properties:"));
	for (uint32_t *property = properties; *property; property++) {
		INDIGO_LOG(indigo_log("  %04x %s", *property, PRIVATE_DATA->property_code_label(*property)));
	}
	ptp_transaction_1_0(device, ptp_operation_canon_SetRemoteMode, 1);
	ptp_transaction_1_0(device, ptp_operation_canon_SetEventMode, 1);
	ptp_canon_get_event(device);
	PRIVATE_DATA->event_checker = indigo_set_timer(device, 0.5, ptp_canon_check_event);
	return true;
}

bool ptp_canon_set_property(indigo_device *device, ptp_property *property) {
	uint8_t buffer[1024], *target = buffer + sizeof(uint32_t);
	uint32_t code = property->code;
	assert(property->property != NULL);
	switch (property->property->type) {
		case INDIGO_TEXT_VECTOR:
			ptp_encode_string(property->property->items[0].text.value, buffer);
			break;
		case INDIGO_SWITCH_VECTOR:
			for (int i = 0; i < property->property->count; i++)
				if (property->property->items->sw.value) {
					property->value.sw.value = property->value.sw.values[i];
					break;
				}
		case INDIGO_NUMBER_VECTOR:
			if (property->code == ptp_property_canon_ImageFormat || property->code == ptp_property_canon_ImageFormatCF || property->code == ptp_property_canon_ImageFormatSD || property->code == ptp_property_canon_ImageFormatExtHD) {
				uint64_t l = property->value.number.value;
				uint64_t i1 = (l >> 32) & 0xFFFFFFFF;
				uint64_t i2 = l & 0xFFFFFFFF;
				int count = i1 == 0 ? 1 : 2;
				target = ptp_encode_uint32((uint32_t)count, target);
				if (count == 2) {
					target = ptp_encode_uint32((uint32_t)((i1 >> 24) & 0xFF), target);
					target = ptp_encode_uint32((uint32_t)((i1 >> 16) & 0xFF), target);
					target = ptp_encode_uint32((uint32_t)((i1 >> 8) & 0xFF), target);
					target = ptp_encode_uint32((uint32_t)(i1 & 0xFF), target);
				}
				target = ptp_encode_uint32((uint32_t)((i2 >> 24) & 0xFF), target);
				target = ptp_encode_uint32((uint32_t)((i2 >> 16) & 0xFF), target);
				target = ptp_encode_uint32((uint32_t)((i2 >> 8) & 0xFF), target);
				target = ptp_encode_uint32((uint32_t)(i2 & 0xFF), target);
			} else if ((property->code & 0xF000) == 0x8000) {
				for (int i = 0; CANON_PRIVATE_DATA->ex_func_group[i][0]; i++) {
					
					int offset = 4;
					unsigned int item_count = CANON_PRIVATE_DATA->ex_func_group[i][offset++];
					for (int j = 0; j < item_count; j++) {
						unsigned int item = CANON_PRIVATE_DATA->ex_func_group[i][offset++];
						unsigned int value_size = CANON_PRIVATE_DATA->ex_func_group[i][offset++];
						if ((item | 0x8000) == property->code) {
							 CANON_PRIVATE_DATA->ex_func_group[i][offset++] = (uint32_t)property->value.sw.value;
							memcpy(buffer + 8, CANON_PRIVATE_DATA->ex_func_group[i], CANON_PRIVATE_DATA->ex_func_group[i][0]);
							code = ptp_property_canon_CustomFuncEx;
							break;
						} else {
							offset += value_size;
						}
					}
					if (code == ptp_property_canon_CustomFuncEx)
						break;
				}
			} else if (property->type && property->type <= ptp_uint32_type) {
				target = ptp_encode_uint32((uint32_t)property->value.number.value, target);
			}
		default:
			assert(false);
	}
	*((uint32_t *)buffer) = (uint32_t)(target - buffer);
	*((uint32_t *)buffer + 1) = code;
	if (ptp_transaction_0_0_o(device, ptp_operation_canon_SetDevicePropValueEx, buffer))
		property->property->state = INDIGO_OK_STATE;
	else
		property->property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property->property, NULL);
	return true;
}
