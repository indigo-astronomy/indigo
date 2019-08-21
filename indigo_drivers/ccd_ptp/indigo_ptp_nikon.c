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

#include <indigo/indigo_ccd_driver.h>

#include "indigo_ptp.h"
#include "indigo_ptp_nikon.h"

#define NIKON_PRIVATE_DATA	((nikon_private_data *)(PRIVATE_DATA->vendor_private_data))

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
	switch (code) {
		case ptp_property_ExposureProgramMode: return DSLR_PROGRAM_PROPERTY_NAME;
		case ptp_property_StillCaptureMode: return DSLR_CAPTURE_MODE_PROPERTY_NAME;
		case ptp_property_FNumber: return DSLR_APERTURE_PROPERTY_NAME;
		case ptp_property_ExposureTime: return DSLR_SHUTTER_PROPERTY_NAME;
		case ptp_property_ImageSize: return CCD_MODE_PROPERTY_NAME;
		case ptp_property_WhiteBalance: return DSLR_WHITE_BALANCE_PROPERTY_NAME;
		case ptp_property_ExposureIndex: return DSLR_ISO_PROPERTY_NAME;
		case ptp_property_ExposureMeteringMode: return DSLR_EXPOSURE_METERING_PROPERTY_NAME;
		case ptp_property_ExposureBiasCompensation: return DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME;
		case ptp_property_BatteryLevel: return DSLR_BATTERY_LEVEL_PROPERTY_NAME;
		case ptp_property_CompressionSetting: return DSLR_COMPRESSION_PROPERTY_NAME;
		case ptp_property_FocalLength: return DSLR_FOCAL_LENGTH_PROPERTY_NAME;
		case ptp_property_FlashMode: return DSLR_FLASH_MODE_PROPERTY_NAME;
		case ptp_property_nikon_AutofocusMode: return DSLR_FOCUS_MODE_PROPERTY_NAME;
		case ptp_property_nikon_EVStep: return DSLR_COMPENSATION_STEP_PROPERTY_NAME;
		case ptp_property_nikon_ActivePicCtrlItem: return DSLR_PICTURE_STYLE_PROPERTY_NAME;
	}
	return ptp_property_nikon_code_label(code);
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
		case ptp_property_nikon_EVStep: return "Compensation step";
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
		case ptp_property_nikon_ExternalFlashMode: return "Flash mode";
		case ptp_property_nikon_ExternalFlashCompensation: return "External flash compensation";
		case ptp_property_nikon_NewExternalFlashMode: return "NewExternalFlashMode_Nikon";
		case ptp_property_nikon_FlashExposureCompensation: return "Flash compensation";
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
		case ptp_property_nikon_AutofocusMode: return "AF mode";
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
		case ptp_property_nikon_ActivePicCtrlItem: return "Picture style";
		case ptp_property_nikon_ChangePicCtrlItem: return "ChangePicCtrlItem_Nikon";
		case ptp_property_nikon_MovieNrHighISO: return "MovieNrHighISO_Nikon";
	}
	return ptp_property_code_label(code);
}

char *ptp_property_nikon_value_code_label(indigo_device *device, uint16_t property, uint64_t code) {
	static char label[PTP_MAX_CHARS];
	switch (property) {
		case ptp_property_CompressionSetting: {
			if (PRIVATE_DATA->model.product == 0x043a || PRIVATE_DATA->model.product == 0x043c || PRIVATE_DATA->model.product == 0x0440 || PRIVATE_DATA->model.product == 0x0441 || PRIVATE_DATA->model.product == 0x0442 || PRIVATE_DATA->model.product == 0x0443) {
				switch (code) { case 0: return "JPEG basic"; case 1: return "JPEG basic *"; case 2: return "JPEG normal"; case 3: return "JPEG normal *"; case 4: return "JPEG fine"; case 5: return "JPEG  fine *"; case 6: return "TIFF (RGB)"; case 7: return "NEF"; case 8: return "NEF + JPEG basic"; case 9: return "NEF + JPEG basic *"; case 10: return "NEF + JPEG normal"; case 11: return "NEF + JPEG normal *"; case 12: return "NEF + JPEG fine"; case 13: return "NEF + JPEG fine *"; }
			} else {
				switch (code) { case 0: return "JPEG Basic"; case 1: return "JPEG Norm"; case 2: return "JPEG Fine"; case 3:return "TIFF-RGB"; case 4: return "RAW"; case 5: return "RAW + JPEG Basic"; case 6: return "RAW + JPEG Norm"; case 7: return "RAW + JPEG Fine"; }
			}
			break;
		}
		case ptp_property_ExposureProgramMode: {
			switch (code) { case 1: return "Manual"; case 2: return "Program"; case 3: return "Aperture priority"; case 4: return "Shutter priority"; case 32784: return "Auto"; case 32785: return "Portrait"; case 32786: return "Landscape"; case 32787:return "Macro"; case 32788: return "Sport"; case 32789: return "Night portrait"; case 32790:return "Night landscape"; case 32791: return "Child"; case 32792: return "Scene"; case 32793: return "Effects"; case 0x8050: return "U1"; case 0x8051: return "U2"; case 0x8052: return "U3"; }
			break;
		}
		case ptp_property_FNumber: {
			sprintf(label, "f/%g", code / 100.0);
			return label;
		}
		case ptp_property_StillCaptureMode: {
			switch (code) { case 1: return "Single shot"; case 2: return "Continuous"; case 3:return "Timelapse"; case 32784: return "Continuous low speed"; case 32785: return "Timer"; case 32786: return "Mirror up"; case 32787: return "Remote"; case 32788: return "Timer + Remote"; case 32789: return "Delayed remote"; case 32790: return "Quiet shutter release"; }
			break;
		}
		case ptp_property_FocusMeteringMode: {
			switch (code) { case 1: return "Center-spot"; case 2: return "Multi-spot"; case 32784: return "Single Area"; case 32785: return "Auto area"; case 32786: return "3D tracking"; case 32787: return "21 points"; case 32788: return "39 points"; }
			break;
		}
		case ptp_property_FlashMode: {
			switch (code) { case 0: return "Undefined"; case 1: return "Automatic flash"; case 2: return "Flash off"; case 3: return "Fill flash"; case 4: return "Automatic Red-eye Reduction"; case 5: return "Red-eye fill flash"; case 6: return "External sync"; case 32784: return "Auto"; case 32785: return "Auto Slow Sync"; case 32786: return "Rear Curtain Sync + Slow Sync"; case 32787: return "Red-eye Reduction + Slow Sync"; }
			break;
		}
		case ptp_property_WhiteBalance: {
			switch (code) { case 1: return "Manual"; case 2: return "Auto"; case 3: return "One-push Auto"; case 4: return "Daylight"; case 5: return "Fluorescent"; case 6: return "Incandescent"; case 7: return "Flash"; case 32784: return "Cloudy"; case 32785: return "Shade"; case 32786: return "Color Temperature"; case 32787: return "Preset"; }
			break;
		}
		case ptp_property_nikon_ExternalFlashMode:
		case ptp_property_nikon_ColorSpace: {
			switch (code) { case 0: return "sRGB"; case 1: return "Adobe RGB"; }
			break;
		}
		case ptp_property_nikon_LiveViewImageZoomRatio: {
			switch (code) { case 0: return "1x"; case 1: return "2x"; case 2: return "3x"; case 3: return "4x"; case 4: return "6x"; case 5: return "8x"; }
			break;
		}
		case ptp_property_nikon_VignetteCtrl: {
			switch (code) { case 0: return "High"; case 1: return "Normal"; case 2: return "Low"; case 3: return "Off"; }
			break;
		}
		case ptp_property_nikon_BlinkingStatus: {
			switch (code) { case 0: return "None"; case 1: return "Shutter"; case 2: return "Aperture"; case 3: return "Shutter + Aperture"; }
			break;
		}
		case ptp_property_nikon_CleanImageSensor: {
			switch (code) { case 0: return "At startup"; case 1: return "At shutdown"; case 2: return "At startup + shutdown"; case 3: return "Off"; }
			break;
		}
		case ptp_property_nikon_HDRMode: {
			switch (code) { case 0: return "Off"; case 1: return "Low"; case 2: return "Normal"; case 3: return "High"; case 4: return "Extra high"; case 5: return "Auto"; }
			break;
		}
		case ptp_property_nikon_MovScreenSize: {
			switch (code) { case 0: return "1920x1080 50p"; case 1: return "1920x1080 25p"; case 2: return "1920x1080 24p"; case 3: return "1280x720 50p"; case 4: return "640x424 25p"; case 5: return "1920x1080 25p"; case 6: return "1920x1080 24p"; case 7: return "1280x720 50p"; }
			break;
		}
		case ptp_property_nikon_EnableCopyright:
		case ptp_property_nikon_AutoDistortionControl:
		case ptp_property_nikon_AELockStatus:
		case ptp_property_nikon_AFLockStatus:
		case ptp_property_nikon_ExternalFlashAttached:
		case ptp_property_nikon_ISOAutoTime:
		case ptp_property_nikon_MovHiQuality:
		case ptp_property_nikon_ImageRotation:
		case ptp_property_nikon_ImageCommentEnable:
		case ptp_property_nikon_ManualMovieSetting:
		case ptp_property_nikon_ResetBank:
		case ptp_property_nikon_ResetBank0:
		case ptp_property_nikon_LongExposureNoiseReduction:
		case ptp_property_nikon_MovWindNoiceReduction:
		case ptp_property_nikon_Bracketing:
		case ptp_property_nikon_FocusAreaWrap:
		case ptp_property_nikon_NoCFCard: {
			switch (code) { case 0: return "Off"; case 1: return "On"; }
			break;
		}
		case ptp_property_nikon_AFCModePriority: {
			switch (code) { case 0: return "Release"; case 1: return "Focus"; }
			break;
		}
		case ptp_property_nikon_AFSModePriority: {
			switch (code) { case 1: return "Release"; case 0: return "Focus"; }
			break;
		}
		case ptp_property_nikon_AFLockOn: {
			switch (code) { case 0: return "5 (long)"; case 1: return "4"; case 2: return "3 (normal)"; case 3: return "2"; case 4: return "1 (short)"; case 5: return "Off"; }
			break;
		}
		case ptp_property_nikon_AFAreaPoint: {
			switch (code) { case 0: return "AF51"; case 1: return "AF11"; }
			break;
		}
		case ptp_property_nikon_AFAssist: {
			switch (code) { case 0: return "On"; case 1: return "Off"; }
			break;
		}
		case ptp_property_nikon_EVISOStep:
		case ptp_property_nikon_EVStep: {
			switch (code) { case 0: return "1/3"; case 1: return "1/2"; }
			break;
		}
		case ptp_property_nikon_CameraInclination: {
			switch (code) { case 0: return "Level"; case 1: return "Grip is top"; case 2: return "Grip is bottom"; case 3: return "Up Down"; }
			break;
		}
		case ptp_property_nikon_LiveViewImageSize: {
			switch (code) { case 1: return "QVGA"; case 2: return "VGA"; }
			break;
		}
		case ptp_property_nikon_RawBitMode: {
			switch (code) { case 0: return "12 bit"; case 1: return "14 bit"; }
			break;
		}
		case ptp_property_nikon_SaveMedia: {
			switch (code) { case 0: return "Card"; case 1: return "SDRAM"; case 2: return "Card + SDRAM"; }
			break;
		}
		case ptp_property_nikon_LiveViewAFArea: {
			switch (code) { case 0: return "Face priority"; case 1: return "Wide area"; case 2: return "Normal area"; }
			break;
		}
		case ptp_property_nikon_FlourescentType: {
			switch (code) { case 0: return "Sodium-vapor"; case 1: return "Warm-white"; case 2: return "White"; case 3: return "Cool-white"; case 4: return "Day white"; case 5: return "Daylight"; case 6: return "High temp. mercury-vapor"; }
			break;
		}
		case ptp_property_nikon_ActiveDLighting: {
			switch (code) { case 0: return "High"; case 1: return "Normal"; case 2: return "Low"; case 3: return "Off"; case 4: return "Extra high"; case 5: return "Auto"; }
			break;
		}
		case ptp_property_nikon_ActivePicCtrlItem: {
			switch (code) { case 0: return "Undefined"; case 1: return "Standard"; case 2: return "Neutral"; case 3: return "Vivid"; case 4: return "Monochrome"; case 5: return "Portrait"; case 6: return "Landscape"; case 7: return "Flat"; case 201: return "Custom 1"; case 202: return "Custom 2"; case 203: return "Custom 3"; case 204: return "Custom 4"; case 205: return "Custom 5"; case 206: return "Custom 6"; case 207: return "Custom 7"; case 208: return "Custom 8"; case 209: return "Custom 9"; }
			break;
		}
		case ptp_property_nikon_EffectMode: {
			switch (code) { case 0: return "Night Vision"; case 1: return "Color Sketch"; case 2: return "Miniature Effect"; case 3: return "Selective Color"; case 4: return "Silhouette"; case 5: return "High Key"; case 6: return "Low Key"; }
			break;
		}
		case ptp_property_nikon_SceneMode: {
			switch (code) { case 0: return "NightLandscape"; case 1: return "PartyIndoor"; case 2: return "BeachSnow"; case 3: return "Sunset"; case 4: return "Duskdawn"; case 5: return "Petportrait"; case 6: return "Candlelight"; case 7: return "Blossom"; case 8: return "AutumnColors"; case 9: return "Food"; case 10: return "Silhouette"; case 11: return "Highkey"; case 12: return "Lowkey"; case 13: return "Portrait"; case 14: return "Landscape"; case 15: return "Child"; case 16: return "Sports"; case 17: return "Closeup"; case 18: return "NightPortrait"; }
			break;
		}
		case ptp_property_FocusMode: {
			switch (code) { case 1: return "Manual"; case 2: return "Automatic"; case 3: return "Macro"; case 32784: return "AF-S"; case 32785: return "AF-C"; case 32786: return "AF-A"; case 32787: return "M"; }
			break;
		}
		case ptp_property_ExposureMeteringMode: {
			switch (code) { case 1: return "Average"; case 2: return "Center Weighted Average"; case 3: return "Multi-spot"; case 4: return "Center-spot"; case 0x8010: return "Center-spot *"; }
			break;
		}
		case ptp_property_nikon_AutofocusMode: {
			switch (code) { case 0: return "AF-S"; case 1: return "AF-C"; case 2: return "AF-A"; case 3: return "M (fixed)"; case 4: return "M"; }
			break;
		}
		case ptp_property_nikon_ExposureDelayMode: {
			ptp_property *prop = ptp_property_supported(device, ptp_property_nikon_ExposureDelayMode);
			if (prop) {
				if (prop->value.number.max == 1) {
					switch (code) { case 0: return "Off"; case 1: return "On"; }
				} else {
					switch (code) { case 0: return "3s"; case 1: return "2s"; case 2: return "1s"; case 3: return "Off";  }
				}
			}
			break;
		}
	}
	return ptp_property_value_code_label(device, property, code);
}

static void ptp_check_event(indigo_device *device) {
	void *buffer = NULL;
	if (ptp_operation_supported(device, ptp_operation_nikon_CheckEvent)) {
		if (ptp_transaction_0_0_i(device, ptp_operation_nikon_CheckEvent, &buffer, NULL)) {
			uint8_t *source = buffer;
			uint16_t count, code;
			uint32_t param;
			source = ptp_decode_uint16(source, &count);
			for (int i = 0; i < count; i++) {
				source = ptp_decode_uint16(source, &code);
				source = ptp_decode_uint32(source, &param);
				ptp_nikon_handle_event(device, code, &param);
			}
		}
		if (buffer)
			free(buffer);
	} else {
		for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
			if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, PRIVATE_DATA->info_properties_supported[i], &buffer, NULL)) {
				ptp_decode_property(buffer, device, PRIVATE_DATA->properties + i);
			}
			if (buffer)
				free(buffer);
			buffer = NULL;
		}
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->event_checker);
}

bool ptp_nikon_initialise(indigo_device *device) {
	PRIVATE_DATA->vendor_private_data = malloc(sizeof(nikon_private_data));
	memset(NIKON_PRIVATE_DATA, 0, sizeof(nikon_private_data));
	if (!ptp_initialise(device))
		return false;	
	INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));
	if (PRIVATE_DATA->model.product == 0x0427 || PRIVATE_DATA->model.product == 0x042c || PRIVATE_DATA->model.product == 0x0433 || PRIVATE_DATA->model.product == 0x043d || PRIVATE_DATA->model.product == 0x0445) {
		static uint32_t operations[] = { ptp_operation_nikon_GetVendorPropCodes, ptp_operation_nikon_CheckEvent, ptp_operation_nikon_Capture, ptp_operation_nikon_AfDrive, ptp_operation_nikon_SetControlMode, ptp_operation_nikon_DeviceReady, ptp_operation_nikon_AfCaptureSDRAM, ptp_operation_nikon_DelImageSDRAM, ptp_operation_nikon_GetPreviewImg, ptp_operation_nikon_StartLiveView, ptp_operation_nikon_EndLiveView, ptp_operation_nikon_GetLiveViewImg, ptp_operation_nikon_MfDrive, ptp_operation_nikon_ChangeAfArea, ptp_operation_nikon_AfDriveCancel };
		ptp_append_uint16_32_array(PRIVATE_DATA->info_operations_supported, operations);
		INDIGO_LOG(indigo_log("operations:"));
		for (uint32_t *operation = operations; *operation; operation++) {
			INDIGO_LOG(indigo_log("  %04x %s", *operation, ptp_operation_nikon_code_label(*operation)));
		}
	}
	if (ptp_operation_supported(device, ptp_operation_nikon_GetVendorPropCodes)) {
		void *buffer;
		if (ptp_transaction_0_0_i(device, ptp_operation_nikon_GetVendorPropCodes, &buffer, NULL)) {
			uint16_t properties[PTP_MAX_ELEMENTS];
			ptp_decode_uint16_array(buffer, properties, NULL);
			free(buffer);
			buffer = NULL;
			uint16_t *target = PRIVATE_DATA->info_properties_supported;
			int index = 0;
			for (index = 0; target[index]; index++)
				;
			for (int i = 0; properties[i]; i++) {
				target[index] = properties[i];
				if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, properties[index], &buffer, NULL)) {
					ptp_decode_property(buffer, device, PRIVATE_DATA->properties + index);
				}
				if (buffer)
					free(buffer);
				buffer = NULL;
				index++;
			}
			target[index] = 0;
		}
		if (buffer)
			free(buffer);
	}
	PRIVATE_DATA->event_checker = indigo_set_timer(device, 0.5, ptp_check_event);
	return true;
}

bool ptp_nikon_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	return ptp_handle_event(device, code, params);
}

bool ptp_nikon_fix_property(indigo_device *device, ptp_property *property) {
	switch (property->code) {
		case ptp_property_ImageSize: {
			return true;
		}
		case ptp_property_nikon_AutofocusMode: {
			if (property->value.number.value == 3) {
				property->count = 1;
				property->value.sw.values[0] = 3;
				property->writable = false;
			} else {
				property->count = (int)property->value.number.max;
				property->value.sw.values[0] = 0;
				property->value.sw.values[1] = 1;
				property->value.sw.values[2] = 2;
				property->value.sw.values[3] = 4;
			}
			return true;
		}
		case ptp_property_nikon_EVStep: {
			property->count = 2;
			property->value.sw.values[0] = 0;
			property->value.sw.values[1] = 1;
			return true;
		}
//		case ptp_property_nikon_ActivePicCtrlItem: return DSLR_PICTURE_STYLE_PROPERTY_NAME;
	}
	return false;
}

bool ptp_nikon_set_property(indigo_device *device, ptp_property *property) {
	return ptp_set_property(device, property);
}

bool ptp_nikon_exposure(indigo_device *device) {
	ptp_property *property = ptp_property_supported(device, ptp_property_nikon_ExposureDelayMode);
	bool result = true;
	if (property) {
		uint8_t value = DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value ? 1 : (property->value.number.max == 1 ? 0 : 3);
		result = result && ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_ExposureDelayMode, &value, sizeof(uint8_t));
	}
	if (ptp_operation_supported(device, ptp_operation_nikon_InitiateCaptureRecInMedia)) {
		result = result && ptp_transaction_2_0(device, ptp_operation_nikon_InitiateCaptureRecInMedia, -1, 0);
	} else {
		result = result && ptp_transaction_2_0(device, ptp_operation_InitiateCapture, 0, 0);
	}
	property = ptp_property_supported(device, ptp_property_ExposureTime);
	if (property->value.sw.value == 0xffffffff) {
		CCD_EXPOSURE_ITEM->number.value += DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value ? 2 : 0;
		while (!PRIVATE_DATA->abort_capture && CCD_EXPOSURE_ITEM->number.value > 1) {
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_usleep(ONE_SECOND_DELAY);
			CCD_EXPOSURE_ITEM->number.value -= 1;
		}
		if (!PRIVATE_DATA->abort_capture && CCD_EXPOSURE_ITEM->number.value > 0) {
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_usleep(ONE_SECOND_DELAY * CCD_EXPOSURE_ITEM->number.value);
		}
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		result = result && ptp_transaction_2_0(device, ptp_operation_nikon_TerminateCapture, 0, 0);
	}
	return result;
}

bool ptp_nikon_liveview(indigo_device *device) {
	if (ptp_transaction_0_0(device, ptp_operation_nikon_StartLiveView)) {
		uint8_t *buffer = NULL;
		uint32_t size;
		while (!PRIVATE_DATA->abort_capture) {
			if (ptp_transaction_0_0_i(device, ptp_operation_nikon_GetLiveViewImg, (void **)&buffer, &size)) {
				if ((buffer[64] & 0xFF) == 0xFF && (buffer[65] & 0xFF) == 0xD8) {
					indigo_process_dslr_image(device, (void *)buffer + 64, size - 64, ".jprg");
				} else if ((buffer[128] & 0xFF) == 0xFF && (buffer[129] & 0xFF) == 0xD8) {
					indigo_process_dslr_image(device, (void *)buffer + 128, size - 128, ".jprg");
				} else if ((buffer[384] & 0xFF) == 0xFF && (buffer[385] & 0xFF) == 0xD8) {
					indigo_process_dslr_image(device, (void *)buffer + 384, size - 384, ".jprg");
				}
			}
			if (buffer)
				free(buffer);
			buffer = NULL;
			indigo_usleep(100000);
		}
		ptp_transaction_0_0(device, ptp_operation_nikon_EndLiveView);
	}
	return true;
}

bool ptp_nikon_lock(indigo_device *device) {
	if (ptp_operation_supported(device, ptp_operation_nikon_SetControlMode)) {
		if (DSLR_LOCK_ITEM->sw.value)
			return ptp_transaction_1_0(device, ptp_operation_nikon_SetControlMode, 1);
		return ptp_transaction_1_0(device, ptp_operation_nikon_SetControlMode, 0);
	}
	return false;
}

bool ptp_nikon_zoom(indigo_device *device) {
	ptp_property *property = ptp_property_supported(device, ptp_property_nikon_LiveViewImageZoomRatio);
	if (property) {
		uint8_t value = DSLR_ZOOM_PREVIEW_ON_ITEM->sw.value ? 5 : 0;
		return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_LiveViewImageZoomRatio, &value, sizeof(uint8_t));
	}
	return false;
}

bool ptp_nikon_focus(indigo_device *device, int steps) {
	assert(0);
}
