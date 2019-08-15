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

/** INDIGO PTP Nikon implementation
 \file indigo_ptp_nikon.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <libusb-1.0/libusb.h>

#include "indigo_ptp.h"
#include "indigo_ptp_nikon.h"

char *ptp_operation_nikon_code_label(uint16_t code) {
	switch (code) {
		case ptp_operation_nikon_GetProfileAllData: return "GetProfileAllData_Nikon";
		case ptp_operation_nikon_SendProfileData: return "SendProfileData_Nikon";
		case ptp_operation_nikon_DeleteProfile: return "DeleteProfile_Nikon";
		case ptp_operation_nikon_SetProfileData: return "SetProfileData_Nikon";
		case ptp_operation_nikon_AdvancedTransfer: return "AdvancedTransfer_Nikon";
		case ptp_operation_nikon_GetFileInfoInBlock: return "GetFileInfoInBlock_Nikon";
		case ptp_operation_nikon_Capture: return "Capture_Nikon";
		case ptp_operation_nikon_AfDrive: return "AfDrive_Nikon";
		case ptp_operation_nikon_SetControlMode: return "SetControlMode_Nikon";
		case ptp_operation_nikon_DelImageSDRAM: return "DelImageSDRAM_Nikon";
		case ptp_operation_nikon_GetLargeThumb: return "GetLargeThumb_Nikon";
		case ptp_operation_nikon_CurveDownload: return "CurveDownload_Nikon";
		case ptp_operation_nikon_CurveUpload: return "CurveUpload_Nikon";
		case ptp_operation_nikon_CheckEvent: return "CheckEvent_Nikon";
		case ptp_operation_nikon_DeviceReady: return "DeviceReady_Nikon";
		case ptp_operation_nikon_SetPreWBData: return "SetPreWBData_Nikon";
		case ptp_operation_nikon_GetVendorPropCodes: return "GetVendorPropCodes_Nikon";
		case ptp_operation_nikon_AfCaptureSDRAM: return "AfCaptureSDRAM_Nikon";
		case ptp_operation_nikon_GetPictCtrlData: return "GetPictCtrlData_Nikon";
		case ptp_operation_nikon_SetPictCtrlData: return "SetPictCtrlData_Nikon";
		case ptp_operation_nikon_DelCstPicCtrl: return "DelCstPicCtrl_Nikon";
		case ptp_operation_nikon_GetPicCtrlCapability: return "GetPicCtrlCapability_Nikon";
		case ptp_operation_nikon_GetPreviewImg: return "GetPreviewImg_Nikon";
		case ptp_operation_nikon_StartLiveView: return "StartLiveView_Nikon";
		case ptp_operation_nikon_EndLiveView: return "EndLiveView_Nikon";
		case ptp_operation_nikon_GetLiveViewImg: return "GetLiveViewImg_Nikon";
		case ptp_operation_nikon_MfDrive: return "MfDrive_Nikon";
		case ptp_operation_nikon_ChangeAfArea: return "ChangeAfArea_Nikon";
		case ptp_operation_nikon_AfDriveCancel: return "AfDriveCancel_Nikon";
		case ptp_operation_nikon_InitiateCaptureRecInMedia: return "InitiateCaptureRecInMedia_Nikon";
		case ptp_operation_nikon_GetVendorStorageIDs: return "GetVendorStorageIDs_Nikon";
		case ptp_operation_nikon_StartMovieRecInCard: return "StartMovieRecInCard_Nikon";
		case ptp_operation_nikon_EndMovieRec: return "EndMovieRec_Nikon";
		case ptp_operation_nikon_TerminateCapture: return "TerminateCapture_Nikon";
		case ptp_operation_nikon_GetDevicePTPIPInfo: return "GetDevicePTPIPInfo_Nikon";
		case ptp_operation_nikon_GetPartialObjectHiSpeed: return "GetPartialObjectHiSpeed_Nikon";
	}
	return ptp_operation_code_label(code);
}

char *ptp_response_nikon_code_label(uint16_t code) {
	switch (code) {
		case ptp_response_nikon_HardwareError: return "HardwareError_Nikon";
		case ptp_response_nikon_OutOfFocus: return "OutOfFocus_Nikon";
		case ptp_response_nikon_ChangeCameraModeFailed: return "ChangeCameraModeFailed_Nikon";
		case ptp_response_nikon_InvalidStatus: return "InvalidStatus_Nikon";
		case ptp_response_nikon_SetPropertyNotSupported: return "SetPropertyNotSupported_Nikon";
		case ptp_response_nikon_WbResetError: return "WbResetError_Nikon";
		case ptp_response_nikon_DustReferenceError: return "DustReferenceError_Nikon";
		case ptp_response_nikon_ShutterSpeedBulb: return "ShutterSpeedBulb_Nikon";
		case ptp_response_nikon_MirrorUpSequence: return "MirrorUpSequence_Nikon";
		case ptp_response_nikon_CameraModeNotAdjustFNumber: return "CameraModeNotAdjustFNumber_Nikon";
		case ptp_response_nikon_NotLiveView: return "NotLiveView_Nikon";
		case ptp_response_nikon_MfDriveStepEnd: return "MfDriveStepEnd_Nikon";
		case ptp_response_nikon_MfDriveStepInsufficiency: return "MfDriveStepInsufficiency_Nikon";
		case ptp_response_nikon_AdvancedTransferCancel: return "AdvancedTransferCancel_Nikon";
	}
	return ptp_response_code_label(code);
}

char *ptp_event_nikon_code_label(uint16_t code) {
	switch (code) {
		case ptp_event_nikon_ObjectAddedInSDRAM: return "ObjectAddedInSDRAM_Nikon";
		case ptp_event_nikon_CaptureCompleteRecInSdram: return "CaptureCompleteRecInSdram_Nikon";
		case ptp_event_nikon_AdvancedTransfer: return "AdvancedTransfer_Nikon";
		case ptp_event_nikon_PreviewImageAdded: return "PreviewImageAdded_Nikon";
	}
	return ptp_event_code_label(code);
}

char *ptp_property_nikon_code_name(uint16_t code) {
	static char label[INDIGO_NAME_SIZE];
	sprintf(label, "%04x", code);
	return label;
}

char *ptp_property_nikon_code_label(uint16_t code) {
	switch (code) {
		case ptp_property_nikon_ShootingBank: return "ShootingBank_Nikon";
		case ptp_property_nikon_ShootingBankNameA: return "ShootingBankNameA_Nikon";
		case ptp_property_nikon_ShootingBankNameB: return "ShootingBankNameB_Nikon";
		case ptp_property_nikon_ShootingBankNameC: return "ShootingBankNameC_Nikon";
		case ptp_property_nikon_ShootingBankNameD: return "ShootingBankNameD_Nikon";
		case ptp_property_nikon_ResetBank0: return "ResetBank0_Nikon";
		case ptp_property_nikon_RawCompression: return "RawCompression_Nikon";
		case ptp_property_nikon_WhiteBalanceAutoBias: return "WhiteBalanceAutoBias_Nikon";
		case ptp_property_nikon_WhiteBalanceTungstenBias: return "WhiteBalanceTungstenBias_Nikon";
		case ptp_property_nikon_WhiteBalanceFluorescentBias: return "WhiteBalanceFluorescentBias_Nikon";
		case ptp_property_nikon_WhiteBalanceDaylightBias: return "WhiteBalanceDaylightBias_Nikon";
		case ptp_property_nikon_WhiteBalanceFlashBias: return "WhiteBalanceFlashBias_Nikon";
		case ptp_property_nikon_WhiteBalanceCloudyBias: return "WhiteBalanceCloudyBias_Nikon";
		case ptp_property_nikon_WhiteBalanceShadeBias: return "WhiteBalanceShadeBias_Nikon";
		case ptp_property_nikon_WhiteBalanceColorTemperature: return "WhiteBalanceColorTemperature_Nikon";
		case ptp_property_nikon_WhiteBalancePresetNo: return "WhiteBalancePresetNo_Nikon";
		case ptp_property_nikon_WhiteBalancePresetName0: return "WhiteBalancePresetName0_Nikon";
		case ptp_property_nikon_WhiteBalancePresetName1: return "WhiteBalancePresetName1_Nikon";
		case ptp_property_nikon_WhiteBalancePresetName2: return "WhiteBalancePresetName2_Nikon";
		case ptp_property_nikon_WhiteBalancePresetName3: return "WhiteBalancePresetName3_Nikon";
		case ptp_property_nikon_WhiteBalancePresetName4: return "WhiteBalancePresetName4_Nikon";
		case ptp_property_nikon_WhiteBalancePresetVal0: return "WhiteBalancePresetVal0_Nikon";
		case ptp_property_nikon_WhiteBalancePresetVal1: return "WhiteBalancePresetVal1_Nikon";
		case ptp_property_nikon_WhiteBalancePresetVal2: return "WhiteBalancePresetVal2_Nikon";
		case ptp_property_nikon_WhiteBalancePresetVal3: return "WhiteBalancePresetVal3_Nikon";
		case ptp_property_nikon_WhiteBalancePresetVal4: return "WhiteBalancePresetVal4_Nikon";
		case ptp_property_nikon_ImageSharpening: return "ImageSharpening_Nikon";
		case ptp_property_nikon_ToneCompensation: return "ToneCompensation_Nikon";
		case ptp_property_nikon_ColorModel: return "ColorModel_Nikon";
		case ptp_property_nikon_HueAdjustment: return "HueAdjustment_Nikon";
		case ptp_property_nikon_NonCPULensDataFocalLength: return "NonCPULensDataFocalLength_Nikon";
		case ptp_property_nikon_NonCPULensDataMaximumAperture: return "NonCPULensDataMaximumAperture_Nikon";
		case ptp_property_nikon_ShootingMode: return "ShootingMode_Nikon";
		case ptp_property_nikon_JPEGCompressionPolicy: return "JPEGCompressionPolicy_Nikon";
		case ptp_property_nikon_ColorSpace: return "ColorSpace_Nikon";
		case ptp_property_nikon_AutoDXCrop: return "AutoDXCrop_Nikon";
		case ptp_property_nikon_FlickerReduction: return "FlickerReduction_Nikon";
		case ptp_property_nikon_RemoteMode: return "RemoteMode_Nikon";
		case ptp_property_nikon_VideoMode: return "VideoMode_Nikon";
		case ptp_property_nikon_EffectMode: return "EffectMode_Nikon";
		case ptp_property_nikon_1Mode: return "1Mode_Nikon";
		case ptp_property_nikon_CSMMenuBankSelect: return "CSMMenuBankSelect_Nikon";
		case ptp_property_nikon_MenuBankNameA: return "MenuBankNameA_Nikon";
		case ptp_property_nikon_MenuBankNameB: return "MenuBankNameB_Nikon";
		case ptp_property_nikon_MenuBankNameC: return "MenuBankNameC_Nikon";
		case ptp_property_nikon_MenuBankNameD: return "MenuBankNameD_Nikon";
		case ptp_property_nikon_ResetBank: return "ResetBank_Nikon";
		case ptp_property_nikon_AFCModePriority: return "AFCModePriority_Nikon";
		case ptp_property_nikon_AFSModePriority: return "AFSModePriority_Nikon";
		case ptp_property_nikon_GroupDynamicAF: return "GroupDynamicAF_Nikon";
		case ptp_property_nikon_AFActivation: return "AFActivation_Nikon";
		case ptp_property_nikon_FocusAreaIllumManualFocus: return "FocusAreaIllumManualFocus_Nikon";
		case ptp_property_nikon_FocusAreaIllumContinuous: return "FocusAreaIllumContinuous_Nikon";
		case ptp_property_nikon_FocusAreaIllumWhenSelected: return "FocusAreaIllumWhenSelected_Nikon";
		case ptp_property_nikon_FocusAreaWrap: return "FocusAreaWrap_Nikon";
		case ptp_property_nikon_VerticalAFON: return "VerticalAFON_Nikon";
		case ptp_property_nikon_AFLockOn: return "AFLockOn_Nikon";
		case ptp_property_nikon_FocusAreaZone: return "FocusAreaZone_Nikon";
		case ptp_property_nikon_EnableCopyright: return "EnableCopyright_Nikon";
		case ptp_property_nikon_ISOAutoTime: return "ISOAutoTime_Nikon";
		case ptp_property_nikon_EVISOStep: return "EVISOStep_Nikon";
		case ptp_property_nikon_EVStep: return "EVStep_Nikon";
		case ptp_property_nikon_EVStepExposureComp: return "EVStepExposureComp_Nikon";
		case ptp_property_nikon_ExposureCompensation: return "ExposureCompensation_Nikon";
		case ptp_property_nikon_CenterWeightArea: return "CenterWeightArea_Nikon";
		case ptp_property_nikon_ExposureBaseMatrix: return "ExposureBaseMatrix_Nikon";
		case ptp_property_nikon_ExposureBaseCenter: return "ExposureBaseCenter_Nikon";
		case ptp_property_nikon_ExposureBaseSpot: return "ExposureBaseSpot_Nikon";
		case ptp_property_nikon_LiveViewAFArea: return "LiveViewAFArea_Nikon";
		case ptp_property_nikon_AELockMode: return "AELockMode_Nikon";
		case ptp_property_nikon_AELAFLMode: return "AELAFLMode_Nikon";
		case ptp_property_nikon_LiveViewAFFocus: return "LiveViewAFFocus_Nikon";
		case ptp_property_nikon_MeterOff: return "MeterOff_Nikon";
		case ptp_property_nikon_SelfTimer: return "SelfTimer_Nikon";
		case ptp_property_nikon_MonitorOff: return "MonitorOff_Nikon";
		case ptp_property_nikon_ISOSensitivity: return "ISOSensitivity_Nikon";
		case ptp_property_nikon_ImgConfTime: return "ImgConfTime_Nikon";
		case ptp_property_nikon_AutoOffTimers: return "AutoOffTimers_Nikon";
		case ptp_property_nikon_AngleLevel: return "AngleLevel_Nikon";
		case ptp_property_nikon_ShootingSpeed: return "ShootingSpeed_Nikon";
		case ptp_property_nikon_MaximumShots: return "MaximumShots_Nikon";
		case ptp_property_nikon_ExposureDelayMode: return "ExposureDelayMode_Nikon";
		case ptp_property_nikon_LongExposureNoiseReduction: return "LongExposureNoiseReduction_Nikon";
		case ptp_property_nikon_FileNumberSequence: return "FileNumberSequence_Nikon";
		case ptp_property_nikon_ControlPanelFinderRearControl: return "ControlPanelFinderRearControl_Nikon";
		case ptp_property_nikon_ControlPanelFinderViewfinder: return "ControlPanelFinderViewfinder_Nikon";
		case ptp_property_nikon_Illumination: return "Illumination_Nikon";
		case ptp_property_nikon_NrHighISO: return "NrHighISO_Nikon";
		case ptp_property_nikon_SHSETCHGUIDDISP: return "SHSETCHGUIDDISP_Nikon";
		case ptp_property_nikon_ArtistName: return "ArtistName_Nikon";
		case ptp_property_nikon_CopyrightInfo: return "CopyrightInfo_Nikon";
		case ptp_property_nikon_FlashSyncSpeed: return "FlashSyncSpeed_Nikon";
		case ptp_property_nikon_FlashShutterSpeed: return "FlashShutterSpeed_Nikon";
		case ptp_property_nikon_AAFlashMode: return "AAFlashMode_Nikon";
		case ptp_property_nikon_ModelingFlash: return "ModelingFlash_Nikon";
		case ptp_property_nikon_BracketSet: return "BracketSet_Nikon";
		case ptp_property_nikon_ManualModeBracketing: return "ManualModeBracketing_Nikon";
		case ptp_property_nikon_BracketOrder: return "BracketOrder_Nikon";
		case ptp_property_nikon_AutoBracketSelection: return "AutoBracketSelection_Nikon";
		case ptp_property_nikon_BracketingSet: return "BracketingSet_Nikon";
		case ptp_property_nikon_CenterButtonShootingMode: return "CenterButtonShootingMode_Nikon";
		case ptp_property_nikon_CenterButtonPlaybackMode: return "CenterButtonPlaybackMode_Nikon";
		case ptp_property_nikon_Multiselector: return "Fultiselector_Nikon";
		case ptp_property_nikon_PhotoInfoPlayback: return "PhotoInfoPlayback_Nikon";
		case ptp_property_nikon_AssignFuncButton: return "AssignFuncButton_Nikon";
		case ptp_property_nikon_CustomizeCommDials: return "CustomizeCommDials_Nikon";
		case ptp_property_nikon_ReverseCommandDial: return "ReverseCommandDial_Nikon";
		case ptp_property_nikon_ApertureSetting: return "ApertureSetting_Nikon";
		case ptp_property_nikon_MenusAndPlayback: return "MenusAndPlayback_Nikon";
		case ptp_property_nikon_ButtonsAndDials: return "ButtonsAndDials_Nikon";
		case ptp_property_nikon_NoCFCard: return "NoCFCard_Nikon";
		case ptp_property_nikon_CenterButtonZoomRatio: return "CenterButtonZoomRatio_Nikon";
		case ptp_property_nikon_FunctionButton2: return "FunctionButton2_Nikon";
		case ptp_property_nikon_AFAreaPoint: return "AFAreaPoint_Nikon";
		case ptp_property_nikon_NormalAFOn: return "NormalAFOn_Nikon";
		case ptp_property_nikon_CleanImageSensor: return "CleanImageSensor_Nikon";
		case ptp_property_nikon_ImageCommentString: return "ImageCommentString_Nikon";
		case ptp_property_nikon_ImageCommentEnable: return "ImageCommentEnable_Nikon";
		case ptp_property_nikon_ImageRotation: return "ImageRotation_Nikon";
		case ptp_property_nikon_ManualSetLensNo: return "ManualSetLensNo_Nikon";
		case ptp_property_nikon_MovScreenSize: return "MovScreenSize_Nikon";
		case ptp_property_nikon_MovVoice: return "MovVoice_Nikon";
		case ptp_property_nikon_MovMicrophone: return "MovMicrophone_Nikon";
		case ptp_property_nikon_MovFileSlot: return "MovFileSlot_Nikon";
		case ptp_property_nikon_MovRecProhibitCondition: return "MovRecProhibitCondition_Nikon";
		case ptp_property_nikon_ManualMovieSetting: return "ManualMovieSetting_Nikon";
		case ptp_property_nikon_MovHiQuality: return "MovHiQuality_Nikon";
		case ptp_property_nikon_MovMicSensitivity: return "MovMicSensitivity_Nikon";
		case ptp_property_nikon_MovWindNoiceReduction: return "MovWindNoiceReduction_Nikon";
		case ptp_property_nikon_LiveViewScreenDisplaySetting: return "LiveViewScreenDisplaySetting_Nikon";
		case ptp_property_nikon_MonitorOffDelay: return "MonitorOffDelay_Nikon";
		case ptp_property_nikon_Bracketing: return "Bracketing_Nikon";
		case ptp_property_nikon_AutoExposureBracketStep: return "AutoExposureBracketStep_Nikon";
		case ptp_property_nikon_AutoExposureBracketProgram: return "AutoExposureBracketProgram_Nikon";
		case ptp_property_nikon_AutoExposureBracketCount: return "AutoExposureBracketCount_Nikon";
		case ptp_property_nikon_WhiteBalanceBracketStep: return "WhiteBalanceBracketStep_Nikon";
		case ptp_property_nikon_WhiteBalanceBracketProgram: return "WhiteBalanceBracketProgram_Nikon";
		case ptp_property_nikon_LensID: return "LensID_Nikon";
		case ptp_property_nikon_LensSort: return "LensSort_Nikon";
		case ptp_property_nikon_LensType: return "LensType_Nikon";
		case ptp_property_nikon_FocalLengthMin: return "FocalLengthMin_Nikon";
		case ptp_property_nikon_FocalLengthMax: return "FocalLengthMax_Nikon";
		case ptp_property_nikon_MaxApAtMinFocalLength: return "MaxApAtMinFocalLength_Nikon";
		case ptp_property_nikon_MaxApAtMaxFocalLength: return "MaxApAtMaxFocalLength_Nikon";
		case ptp_property_nikon_FinderISODisp: return "FinderISODisp_Nikon";
		case ptp_property_nikon_AutoOffPhoto: return "AutoOffPhoto_Nikon";
		case ptp_property_nikon_AutoOffMenu: return "AutoOffMenu_Nikon";
		case ptp_property_nikon_AutoOffInfo: return "AutoOffInfo_Nikon";
		case ptp_property_nikon_SelfTimerShootNum: return "SelfTimerShootNum_Nikon";
		case ptp_property_nikon_VignetteCtrl: return "VignetteCtrl_Nikon";
		case ptp_property_nikon_AutoDistortionControl: return "AutoDistortionControl_Nikon";
		case ptp_property_nikon_SceneMode: return "SceneMode_Nikon";
		case ptp_property_nikon_SceneMode2: return "SceneMode2_Nikon";
		case ptp_property_nikon_SelfTimerInterval: return "SelfTimerInterval_Nikon";
		case ptp_property_nikon_ExposureTime: return "ExposureTime_Nikon";
		case ptp_property_nikon_ACPower: return "ACPower_Nikon";
		case ptp_property_nikon_WarningStatus: return "WarningStatus_Nikon";
		case ptp_property_nikon_RemainingShots: return "RemainingShots_Nikon";
		case ptp_property_nikon_AFLockStatus: return "AFLockStatus_Nikon";
		case ptp_property_nikon_AELockStatus: return "AELockStatus_Nikon";
		case ptp_property_nikon_FVLockStatus: return "FVLockStatus_Nikon";
		case ptp_property_nikon_AutofocusLCDTopMode2: return "AutofocusLCDTopMode2_Nikon";
		case ptp_property_nikon_AutofocusSensor: return "AutofocusSensor_Nikon";
		case ptp_property_nikon_FlexibleProgram: return "FlexibleProgram_Nikon";
		case ptp_property_nikon_LightMeter: return "LightMeter_Nikon";
		case ptp_property_nikon_SaveMedia: return "SaveMedia_Nikon";
		case ptp_property_nikon_USBSpeed: return "USBSpeed_Nikon";
		case ptp_property_nikon_CCDNumber: return "CCDNumber_Nikon";
		case ptp_property_nikon_CameraInclination: return "CameraInclination_Nikon";
		case ptp_property_nikon_GroupPtnType: return "GroupPtnType_Nikon";
		case ptp_property_nikon_FNumberLock: return "FNumberLock_Nikon";
		case ptp_property_nikon_ExposureApertureLock: return "ExposureApertureLock_Nikon";
		case ptp_property_nikon_TVLockSetting: return "TVLockSetting_Nikon";
		case ptp_property_nikon_AVLockSetting: return "AVLockSetting_Nikon";
		case ptp_property_nikon_IllumSetting: return "IllumSetting_Nikon";
		case ptp_property_nikon_FocusPointBright: return "FocusPointBright_Nikon";
		case ptp_property_nikon_ExternalFlashAttached: return "ExternalFlashAttached_Nikon";
		case ptp_property_nikon_ExternalFlashStatus: return "ExternalFlashStatus_Nikon";
		case ptp_property_nikon_ExternalFlashSort: return "ExternalFlashSort_Nikon";
		case ptp_property_nikon_ExternalFlashMode: return "ExternalFlashMode_Nikon";
		case ptp_property_nikon_ExternalFlashCompensation: return "ExternalFlashCompensation_Nikon";
		case ptp_property_nikon_NewExternalFlashMode: return "NewExternalFlashMode_Nikon";
		case ptp_property_nikon_FlashExposureCompensation: return "FlashExposureCompensation_Nikon";
		case ptp_property_nikon_HDRMode: return "HDRMode_Nikon";
		case ptp_property_nikon_HDRHighDynamic: return "HDRHighDynamic_Nikon";
		case ptp_property_nikon_HDRSmoothing: return "HDRSmoothing_Nikon";
		case ptp_property_nikon_OptimizeImage: return "OptimizeImage_Nikon";
		case ptp_property_nikon_Saturation: return "Saturation_Nikon";
		case ptp_property_nikon_BWFillerEffect: return "BWFillerEffect_Nikon";
		case ptp_property_nikon_BWSharpness: return "BWSharpness_Nikon";
		case ptp_property_nikon_BWContrast: return "BWContrast_Nikon";
		case ptp_property_nikon_BWSettingType: return "BWSettingType_Nikon";
		case ptp_property_nikon_Slot2SaveMode: return "Slot2SaveMode_Nikon";
		case ptp_property_nikon_RawBitMode: return "RawBitMode_Nikon";
		case ptp_property_nikon_ActiveDLighting: return "ActiveDLighting_Nikon";
		case ptp_property_nikon_FlourescentType: return "FlourescentType_Nikon";
		case ptp_property_nikon_TuneColourTemperature: return "TuneColourTemperature_Nikon";
		case ptp_property_nikon_TunePreset0: return "TunePreset0_Nikon";
		case ptp_property_nikon_TunePreset1: return "TunePreset1_Nikon";
		case ptp_property_nikon_TunePreset2: return "TunePreset2_Nikon";
		case ptp_property_nikon_TunePreset3: return "TunePreset3_Nikon";
		case ptp_property_nikon_TunePreset4: return "TunePreset4_Nikon";
		case ptp_property_nikon_BeepOff: return "BeepOff_Nikon";
		case ptp_property_nikon_AutofocusMode: return "AutofocusMode_Nikon";
		case ptp_property_nikon_AFAssist: return "AFAssist_Nikon";
		case ptp_property_nikon_PADVPMode: return "PADVPMode_Nikon";
		case ptp_property_nikon_ImageReview: return "ImageReview_Nikon";
		case ptp_property_nikon_AFAreaIllumination: return "AFAreaIllumination_Nikon";
		case ptp_property_nikon_FlashMode: return "FlashMode_Nikon";
		case ptp_property_nikon_FlashCommanderMode: return "FlashCommanderMode_Nikon";
		case ptp_property_nikon_FlashSign: return "FlashSign_Nikon";
		case ptp_property_nikon_ISOAuto: return "ISOAuto_Nikon";
		case ptp_property_nikon_RemoteTimeout: return "RemoteTimeout_Nikon";
		case ptp_property_nikon_GridDisplay: return "GridDisplay_Nikon";
		case ptp_property_nikon_FlashModeManualPower: return "FlashModeManualPower_Nikon";
		case ptp_property_nikon_FlashModeCommanderPower: return "FlashModeCommanderPower_Nikon";
		case ptp_property_nikon_AutoFP: return "AutoFP_Nikon";
		case ptp_property_nikon_DateImprintSetting: return "DateImprintSetting_Nikon";
		case ptp_property_nikon_DateCounterSelect: return "DateCounterSelect_Nikon";
		case ptp_property_nikon_DateCountData: return "DateCountData_Nikon";
		case ptp_property_nikon_DateCountDisplaySetting: return "DateCountDisplaySetting_Nikon";
		case ptp_property_nikon_RangeFinderSetting: return "RangeFinderSetting_Nikon";
		case ptp_property_nikon_CSMMenu: return "CSMMenu_Nikon";
		case ptp_property_nikon_WarningDisplay: return "WarningDisplay_Nikon";
		case ptp_property_nikon_BatteryCellKind: return "BatteryCellKind_Nikon";
		case ptp_property_nikon_ISOAutoHiLimit: return "ISOAutoHiLimit_Nikon";
		case ptp_property_nikon_DynamicAFArea: return "DynamicAFArea_Nikon";
		case ptp_property_nikon_ContinuousSpeedHigh: return "ContinuousSpeedHigh_Nikon";
		case ptp_property_nikon_InfoDispSetting: return "InfoDispSetting_Nikon";
		case ptp_property_nikon_PreviewButton: return "PreviewButton_Nikon";
		case ptp_property_nikon_PreviewButton2: return "PreviewButton2_Nikon";
		case ptp_property_nikon_AEAFLockButton2: return "AEAFLockButton2_Nikon";
		case ptp_property_nikon_IndicatorDisp: return "IndicatorDisp_Nikon";
		case ptp_property_nikon_CellKindPriority: return "CellKindPriority_Nikon";
		case ptp_property_nikon_BracketingFramesAndSteps: return "BracketingFramesAndSteps_Nikon";
		case ptp_property_nikon_LiveViewMode: return "LiveViewMode_Nikon";
		case ptp_property_nikon_LiveViewDriveMode: return "LiveViewDriveMode_Nikon";
		case ptp_property_nikon_LiveViewStatus: return "LiveViewStatus_Nikon";
		case ptp_property_nikon_LiveViewImageZoomRatio: return "LiveViewImageZoomRatio_Nikon";
		case ptp_property_nikon_LiveViewProhibitCondition: return "LiveViewProhibitCondition_Nikon";
		case ptp_property_nikon_MovieShutterSpeed: return "MovieShutterSpeed_Nikon";
		case ptp_property_nikon_MovieFNumber: return "MovieFNumber_Nikon";
		case ptp_property_nikon_MovieISO: return "MovieISO_Nikon";
		case ptp_property_nikon_LiveViewImageSize: return "LiveViewImageSize_Nikon";
		case ptp_property_nikon_BlinkingStatus: return "BlinkingStatus_Nikon";
		case ptp_property_nikon_ExposureIndicateStatus: return "ExposureIndicateStatus_Nikon";
		case ptp_property_nikon_InfoDispErrStatus: return "InfoDispErrStatus_Nikon";
		case ptp_property_nikon_ExposureIndicateLightup: return "ExposureIndicateLightup_Nikon";
		case ptp_property_nikon_FlashOpen: return "FlashOpen_Nikon";
		case ptp_property_nikon_FlashCharged: return "FlashCharged_Nikon";
		case ptp_property_nikon_FlashMRepeatValue: return "FlashMRepeatValue_Nikon";
		case ptp_property_nikon_FlashMRepeatCount: return "FlashMRepeatCount_Nikon";
		case ptp_property_nikon_FlashMRepeatInterval: return "FlashMRepeatInterval_Nikon";
		case ptp_property_nikon_FlashCommandChannel: return "FlashCommandChannel_Nikon";
		case ptp_property_nikon_FlashCommandSelfMode: return "FlashCommandSelfMode_Nikon";
		case ptp_property_nikon_FlashCommandSelfCompensation: return "FlashCommandSelfCompensation_Nikon";
		case ptp_property_nikon_FlashCommandSelfValue: return "FlashCommandSelfValue_Nikon";
		case ptp_property_nikon_FlashCommandAMode: return "FlashCommandAMode_Nikon";
		case ptp_property_nikon_FlashCommandACompensation: return "FlashCommandACompensation_Nikon";
		case ptp_property_nikon_FlashCommandAValue: return "FlashCommandAValue_Nikon";
		case ptp_property_nikon_FlashCommandBMode: return "FlashCommandBMode_Nikon";
		case ptp_property_nikon_FlashCommandBCompensation: return "FlashCommandBCompensation_Nikon";
		case ptp_property_nikon_FlashCommandBValue: return "FlashCommandBValue_Nikon";
		case ptp_property_nikon_ApplicationMode: return "ApplicationMode_Nikon";
		case ptp_property_nikon_ActiveSlot: return "ActiveSlot_Nikon";
		case ptp_property_nikon_ActivePicCtrlItem: return "ActivePicCtrlItem_Nikon";
		case ptp_property_nikon_ChangePicCtrlItem: return "ChangePicCtrlItem_Nikon";
		case ptp_property_nikon_MovieNrHighISO: return "MovieNrHighISO_Nikon";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_nikon_value_code_label(uint16_t property, uint64_t code) {
	return ptp_property_value_code_label(property, code);
}

bool ptp_nikon_initialise(indigo_device *device) {
	assert(0);
}

bool ptp_nikon_set_property(indigo_device *device, ptp_property *property) {
	assert(0);
}

bool ptp_nikon_liveview(indigo_device *device) {
	assert(0);
}
