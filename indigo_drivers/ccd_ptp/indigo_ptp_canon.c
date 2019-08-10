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
#include <libusb-1.0/libusb.h>

#include "indigo_ptp.h"
#include "indigo_ptp_canon.h"

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

char *ptp_property_canon_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_canon_Aperture: return "Aperture_Canon";
		case ptp_property_canon_ShutterSpeed: return "ShutterSpeed_Canon";
		case ptp_property_canon_ISOSpeed: return "ISOSpeed_Canon";
		case ptp_property_canon_ExpCompensation: return "ExpCompensation_Canon";
		case ptp_property_canon_AutoExposureMode: return "AutoExposureMode_Canon";
		case ptp_property_canon_DriveMode: return "DriveMode_Canon";
		case ptp_property_canon_MeteringMode: return "MeteringMode_Canon";
		case ptp_property_canon_FocusMode: return "FocusMode_Canon";
		case ptp_property_canon_WhiteBalance: return "WhiteBalance_Canon";
		case ptp_property_canon_ColorTemperature: return "ColorTemperature_Canon";
		case ptp_property_canon_WhiteBalanceAdjustA: return "WhiteBalanceAdjustA_Canon";
		case ptp_property_canon_WhiteBalanceAdjustB: return "WhiteBalanceAdjustB_Canon";
		case ptp_property_canon_WhiteBalanceXA: return "WhiteBalanceXA_Canon";
		case ptp_property_canon_WhiteBalanceXB: return "WhiteBalanceXB_Canon";
		case ptp_property_canon_ColorSpace: return "ColorSpace_Canon";
		case ptp_property_canon_PictureStyle: return "PictureStyle_Canon";
		case ptp_property_canon_BatteryPower: return "BatteryPower_Canon";
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
		case ptp_property_canon_ImageFormat: return "ImageFormat_Canon";
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
		case ptp_property_canon_ExFlasgSyncSpeedInAvMode: return "ExFlasgSyncSpeedInAvMode_Canon";
		case ptp_property_canon_ExLongExposureNoiseReduction: return "ExLongExposureNoiseReduction_Canon";
		case ptp_property_canon_ExHighISONoiseReduction: return "ExHighISONoiseReduction_Canon";
		case ptp_property_canon_ExHHighlightTonePriority: return "ExHHighlightTonePriority_Canon";
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

bool ptp_canon_initialise(indigo_device *device) {
	if (ptp_initialise(device)) {
		void *buffer = NULL;
		if (ptp_request(device, ptp_operation_canon_GetDeviceInfoEx, 0) && ptp_read(device, NULL, &buffer, NULL) && ptp_response(device, NULL, 0)) {
			uint8_t *source = buffer + sizeof(uint32_t);
			uint32_t events[PTP_MAX_ELEMENTS], properties[PTP_MAX_ELEMENTS];
			source = ptp_copy_uint32_array(source, events, NULL);
			ptp_append_uint16_32_array(PRIVATE_DATA->info_events_supported, events);
			source = ptp_copy_uint32_array(source, properties, NULL);
			ptp_append_uint16_32_array(PRIVATE_DATA->info_properties_supported, properties);
			if (buffer)
				free(buffer);
			INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));
			INDIGO_LOG(indigo_log("events:"));
			for (uint32_t *event = events; *event; event++) {
				INDIGO_LOG(indigo_log("  %04x %s", *event, PRIVATE_DATA->event_code_label(*event)));
			}
			INDIGO_LOG(indigo_log("properties:"));
			for (uint32_t *property = properties; *property; property++) {
				INDIGO_LOG(indigo_log("  %04x %s", *property, PRIVATE_DATA->property_code_label(*property)));
			}
			return true;
		}
		if (buffer)
			free(buffer);
		return false;

	}
	return false;
}
