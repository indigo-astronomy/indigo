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

// Nikon D3XXX series
#define NIKON_PRODUCT_D3000 0x0424
#define NIKON_PRODUCT_D3100 0x0427
#define NIKON_PRODUCT_D3200 0x042c
#define NIKON_PRODUCT_D3300 0x0433
#define NIKON_PRODUCT_D3400 0x043d
#define NIKON_PRODUCT_D3500 0x0445

// Nikon EXPEED 5 series
#define NIKON_PRODUCT_D5    0x043a
#define NIKON_PRODUCT_D500  0x043c
#define NIKON_PRODUCT_D7500 0x0440
#define NIKON_PRODUCT_D850  0x0441
// Nikon EXPEED 6 series
#define NIKON_PRODUCT_Z7    0x0442
#define NIKON_PRODUCT_Z6    0x0443
#define NIKON_PRODUCT_Z50   0x0444
#define NIKON_PRODUCT_D780	0x0446
#define NIKON_PRODUCT_D6    0x0447
#define NIKON_PRODUCT_Z5    0x0448
#define NIKON_PRODUCT_Z7II  0x044B
#define NIKON_PRODUCT_Z6II  0x044C
// Nikon EXPEED 7 series
#define NIKON_PRODUCT_Z9    0x0450

#define IS_NIKON_D3000_OR_D3100() \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3000 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3100

// TODO: there is a doubt if 3200 and 3300 can really zoom in LV and control the lens!!!

#define IS_NIKON_D3200_OR_LATER() \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3200 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3300 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3400 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D3500

#define IS_NIKON_EXPEED5_SERIES() \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D5 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D500 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D7500 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D850

#define IS_NIKON_EXPEED6_SERIES() \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z7 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z6 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z7II || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z6II || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z50 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z5 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D780 || \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_D6

#define IS_NIKON_EXPEED7_SERIES() \
	PRIVATE_DATA->model.product == NIKON_PRODUCT_Z9

#define IS_NIKON_EXPEED5_OR_LATER() \
	IS_NIKON_EXPEED5_SERIES() || \
	IS_NIKON_EXPEED6_SERIES() || \
	IS_NIKON_EXPEED7_SERIES()

#define IS_NIKON_EXPEED6_OR_LATER() \
	IS_NIKON_EXPEED6_SERIES() || \
	IS_NIKON_EXPEED7_SERIES()

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
		case ptp_property_nikon_ColorSpace: return DSLR_COLOR_SPACE_PROPERTY_NAME;
		case ptp_property_FocusMeteringMode: return DSLR_FOCUS_METERING_PROPERTY_NAME;
		case ptp_property_nikon_AutoDXCrop: return "ADV_AutoDXCrop";
		case ptp_property_nikon_ArtistName: return "ADV_ArtistName";
		case ptp_property_nikon_CopyrightInfo: return "ADV_CopyrightInfo";
		case ptp_property_nikon_LensID: return "ADV_LensID";
		case ptp_property_nikon_FocalLengthMin: return "ADV_FocalLengthMin";
		case ptp_property_nikon_FocalLengthMax: return "ADV_FocalLengthMax";
		case ptp_property_nikon_LongExposureNoiseReduction: return "ADV_LongExposureNoiseReduction";
		case ptp_property_nikon_NrHighISO: return "ADV_NrHighISO";
		case ptp_property_nikon_VignetteCtrl: return "ADV_VignetteCtrl";
		case ptp_property_nikon_ActiveDLighting: return "ADV_ActiveDLighting";
		case ptp_property_nikon_ImageCommentString: return "ADV_ImageCommentString";
		case ptp_property_nikon_ImageCommentEnable: return "ADV_ImageCommentEnable";
		case ptp_property_nikon_EnableCopyright: return "ADV_EnableCopyright";
		case ptp_property_nikon_AFAssist: return "ADV_AFAssist";
		case ptp_property_nikon_AFCModePriority: return "ADV_AFCModePriority";
		case ptp_property_nikon_AFSModePriority: return "ADV_AFSModePriority";
		case ptp_property_nikon_AFAreaPoint: return "ADV_AFAreaPoint";
		case ptp_property_nikon_AFLockOn: return "ADV_AFLockOn";
		case ptp_property_nikon_EVISOStep: return "ADV_EVISOStep";
		case ptp_property_nikon_ImageRotation: return "ADV_ImageRotation";
		case ptp_property_nikon_RawBitMode: return "ADV_RawBitMode";
		case ptp_property_nikon_RawCompression: return "ADV_RawCompression";
		case ptp_property_nikon_CameraInclination: return "ADV_CameraInclination";
		case ptp_property_nikon_ExposureIndexHi: return "ADV_ExposureIndexHi";
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
		case ptp_property_nikon_RawCompression: return "NEF compression";
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
		case ptp_property_nikon_ColorSpace: return "Color space";
		case ptp_property_nikon_AutoDXCrop: return "Auto DX Crop";
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
		case ptp_property_nikon_AFCModePriority: return "AFC mode priority";
		case ptp_property_nikon_AFSModePriority: return "AFS mode priority";
		case ptp_property_nikon_GroupDynamicAF: return "GroupDynamicAF_Nikon";
		case ptp_property_nikon_AFActivation: return "AFActivation_Nikon";
		case ptp_property_nikon_FocusAreaIllumManualFocus: return "FocusAreaIllumManualFocus_Nikon";
		case ptp_property_nikon_FocusAreaIllumContinuous: return "FocusAreaIllumContinuous_Nikon";
		case ptp_property_nikon_FocusAreaIllumWhenSelected: return "FocusAreaIllumWhenSelected_Nikon";
		case ptp_property_nikon_FocusAreaWrap: return "FocusAreaWrap_Nikon";
		case ptp_property_nikon_VerticalAFON: return "VerticalAFON_Nikon";
		case ptp_property_nikon_AFLockOn: return "AF tracking with lock-on";
		case ptp_property_nikon_FocusAreaZone: return "FocusAreaZone_Nikon";
		case ptp_property_nikon_EnableCopyright: return "Copyright enable";
		case ptp_property_nikon_ISOAutoTime: return "ISOAutoTime_Nikon";
		case ptp_property_nikon_EVISOStep: return "ISO EV step";
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
		case ptp_property_nikon_ExposureIndexHi: return "ExposureIndexHi_Nikon";
		case ptp_property_nikon_ISOSensitivity: return "ISOSensitivity_Nikon";
		case ptp_property_nikon_ImgConfTime: return "ImgConfTime_Nikon";
		case ptp_property_nikon_AutoOffTimers: return "AutoOffTimers_Nikon";
		case ptp_property_nikon_AngleLevel: return "AngleLevel_Nikon";
		case ptp_property_nikon_ShootingSpeed: return "ShootingSpeed_Nikon";
		case ptp_property_nikon_MaximumShots: return "MaximumShots_Nikon";
		case ptp_property_nikon_ExposureDelayMode: return "ExposureDelayMode_Nikon";
		case ptp_property_nikon_LongExposureNoiseReduction: return "Long exposure NR";
		case ptp_property_nikon_FileNumberSequence: return "FileNumberSequence_Nikon";
		case ptp_property_nikon_ControlPanelFinderRearControl: return "ControlPanelFinderRearControl_Nikon";
		case ptp_property_nikon_ControlPanelFinderViewfinder: return "ControlPanelFinderViewfinder_Nikon";
		case ptp_property_nikon_Illumination: return "Illumination_Nikon";
		case ptp_property_nikon_NrHighISO: return "High ISO NR";
		case ptp_property_nikon_SHSETCHGUIDDISP: return "SHSETCHGUIDDISP_Nikon";
		case ptp_property_nikon_ArtistName: return "Artist";
		case ptp_property_nikon_CopyrightInfo: return "Copyright";
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
		case ptp_property_nikon_AFAreaPoint: return "AF area points";
		case ptp_property_nikon_NormalAFOn: return "NormalAFOn_Nikon";
		case ptp_property_nikon_CleanImageSensor: return "CleanImageSensor_Nikon";
		case ptp_property_nikon_ImageCommentString: return "Comment";
		case ptp_property_nikon_ImageCommentEnable: return "Comment enable";
		case ptp_property_nikon_ImageRotation: return "Image rotation";
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
		case ptp_property_nikon_LensID: return "Lens type";
		case ptp_property_nikon_LensSort: return "LensSort_Nikon";
		case ptp_property_nikon_LensType: return "LensType";
		case ptp_property_nikon_FocalLengthMin: return "Focal length min";
		case ptp_property_nikon_FocalLengthMax: return "Focal length max";
		case ptp_property_nikon_MaxApAtMinFocalLength: return "MaxApAtMinFocalLength_Nikon";
		case ptp_property_nikon_MaxApAtMaxFocalLength: return "MaxApAtMaxFocalLength_Nikon";
		case ptp_property_nikon_FinderISODisp: return "FinderISODisp_Nikon";
		case ptp_property_nikon_AutoOffPhoto: return "AutoOffPhoto_Nikon";
		case ptp_property_nikon_AutoOffMenu: return "AutoOffMenu_Nikon";
		case ptp_property_nikon_AutoOffInfo: return "AutoOffInfo_Nikon";
		case ptp_property_nikon_SelfTimerShootNum: return "SelfTimerShootNum_Nikon";
		case ptp_property_nikon_VignetteCtrl: return "Vignette control";
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
		case ptp_property_nikon_CameraInclination: return "Camera inclination";
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
		case ptp_property_nikon_RawBitMode: return "NEF bit depth";
		case ptp_property_nikon_ActiveDLighting: return "Active DLighting";
		case ptp_property_nikon_FlourescentType: return "FlourescentType_Nikon";
		case ptp_property_nikon_TuneColourTemperature: return "TuneColourTemperature_Nikon";
		case ptp_property_nikon_TunePreset0: return "TunePreset0_Nikon";
		case ptp_property_nikon_TunePreset1: return "TunePreset1_Nikon";
		case ptp_property_nikon_TunePreset2: return "TunePreset2_Nikon";
		case ptp_property_nikon_TunePreset3: return "TunePreset3_Nikon";
		case ptp_property_nikon_TunePreset4: return "TunePreset4_Nikon";
		case ptp_property_nikon_BeepOff: return "BeepOff_Nikon";
		case ptp_property_nikon_AutofocusMode: return "AF mode";
		case ptp_property_nikon_AFAssist: return "AF assist";
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
		case ptp_property_nikon_LiveViewZoomArea: return "LiveViewZoomArea_Nikon";
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
			if (IS_NIKON_EXPEED5_OR_LATER()) {
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
			switch (code) { case 1: return "Single shot"; case 2: return "Continuous"; case 3:return "Timelapse"; case 32784: return "Continuous low speed"; case 32785: return "Timer"; case 32786: return "Mirror up"; case 32787: return "Remote"; case 32788: return "Timer + Remote"; case 32789: return "Delayed remote"; case 32790: return "Quiet shutter release"; case 32793: return "Continuous *"; case 33024: return "Quick release-mode selection"; }
			break;
		}
		case ptp_property_FocusMeteringMode: {
			switch (code) { case 1: return "Center-spot"; case 2: return "Multi-spot"; case 32784: return "Single Area"; case 32785: return "Auto area"; case 32786: return "3D tracking"; case 32787: return "21 points"; case 32788: return "39/51 points"; case 0x8015: return "Expansion"; case 0x8017: return "Pinpoint"; case 0x8018: return "Wide area (S)"; case 0x8019: return "Wide area (L)"; case 0x801c: return "Group-area AF (C1)"; case 0x801d: return "Group-area AF (C2)"; }
			break;
		}
		case ptp_property_FlashMode: {
			switch (code) { case 0: return "Undefined"; case 1: return "Automatic flash"; case 2: return "Flash off"; case 3: return "Fill flash"; case 4: return "Automatic Red-eye Reduction"; case 5: return "Red-eye fill flash"; case 6: return "External sync"; case 32784: return "Auto"; case 32785: return "Auto Slow Sync"; case 32786: return "Rear Curtain Sync + Slow Sync"; case 32787: return "Red-eye Reduction + Slow Sync"; }
			break;
		}
		case ptp_property_WhiteBalance: {
			switch (code) { case 1: return "Manual"; case 2: return "Auto"; case 3: return "One-push Auto"; case 4: return "Daylight"; case 5: return "Fluorescent"; case 6: return "Incandescent"; case 7: return "Flash"; case 32784: return "Cloudy"; case 32785: return "Shade"; case 32786: return "Color Temperature"; case 32787: return "Preset"; case 32790: return "Natural light auto"; }
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
		case ptp_property_nikon_LiveViewZoomArea: {
			switch (code) { case 0: return "full"; case 0x200: case 0x280: return "200%"; case 0x400: case 0x500: return "100%"; case 0x800: case 0xa00: return "50%"; }
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
			switch (code) { case 0: return "1920x1080 50p"; case 1: return "1920x1080 25p"; case 2: return "1920x1080 24p"; case 3: return "1280x720 50p"; case 4: return "640x424 25p"; case 5: return "1920x1080 25p"; case 6: return "1920x1080 24p"; case 7: return "1280x720 50p"; case 0x0f000870001e0000: return "3840x2160 30p"; case 0x0f00087000190000: return "3840x2160 25p"; case 0x0f00087000180000: return "3840x2160 24p"; case 0x07800438003c0000: return "1920x1080 60p"; case 0x0780043800320000: return "1920x1080 50p"; case 0x07800438001e0000: return "1920x1080 30p"; case 0x0780043800190000: return "1920x1080 25p"; case 0x0780043800180000: return "1920x1080 24p"; }
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
		case ptp_property_nikon_AutoDXCrop:
		case ptp_property_nikon_RawCompression:
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
			switch (code) { case 0: return "Undefined"; case 1: return "Standard"; case 2: return "Neutral"; case 3: return "Vivid"; case 4: return "Monochrome"; case 5: return "Portrait"; case 6: return "Landscape"; case 7: return "Flat"; case 8: return "Auto"; case 101: return "Dream"; case 102: return "Morning"; case 103: return "Pop"; case 104: return "Sunday"; case 105: return "Somber"; case 106: return "Dramatic"; case 107: return "Silence"; case 108: return "Bleached"; case 109: return "Melancholic"; case 110: return "Pure"; case 111: return "Denim"; case 112: return "Toy"; case 113: return "Sepia"; case 114: return "Blue"; case 115: return "Red"; case 116: return "Pink"; case 117: return "Charcoal"; case 118: return "Graphite"; case 119: return "Binary"; case 120: return "Carbon"; case 201: return "Custom 1"; case 202: return "Custom 2"; case 203: return "Custom 3"; case 204: return "Custom 4"; case 205: return "Custom 5"; case 206: return "Custom 6"; case 207: return "Custom 7"; case 208: return "Custom 8"; case 209: return "Custom 9"; }
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
				if (prop->form == ptp_enum_form && prop->count == 6) {
					switch (code) { case 0: return "Off"; case 2: return "0.2s"; case 5: return "0.5s"; case 10: return "1s"; case 20: return "2s"; case 30: return "3s"; }
				} else if (prop->form == ptp_range_form && prop->value.number.max == 3) {
					switch (code) { case 0: return "3s"; case 1: return "2s"; case 2: return "1s"; case 3: return "Off";  }
				} else if (prop->form == ptp_range_form && prop->value.number.max == 5) {
					switch (code) { case 0: return "Off"; case 1: return "3s"; case 2: return "2s"; case 3: return "1s"; case 4: return "0.5s"; case 5: return "0.2s";  }
				} else {
					switch (code) { case 0: return "Off"; case 1: return "On"; }
				}
			}
			break;
		}
		case ptp_property_nikon_ExposureIndexHi: {
			// same as ptp_property_ExposureIndex
			sprintf(label, "%lld", code);
			return label;
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
		uint32_t size = 0;
		ptp_get_event(device);
		for (int i = 0; PRIVATE_DATA->info_properties_supported[i]; i++) {
			if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, PRIVATE_DATA->info_properties_supported[i], &buffer, &size)) {
				ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + i);
			}
			if (buffer)
				free(buffer);
			buffer = NULL;
		}
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->event_checker);
}

bool ptp_nikon_initialise(indigo_device *device) {
	PRIVATE_DATA->vendor_private_data = indigo_safe_malloc(sizeof(nikon_private_data));
	if (!ptp_initialise(device))
		return false;
	INDIGO_LOG(indigo_log("%s[%d, %s]: device ext_info", DRIVER_NAME, __LINE__, __FUNCTION__));
	if (IS_NIKON_D3000_OR_D3100()) {
		static uint32_t operations[] = { ptp_operation_nikon_DeviceReady, ptp_operation_nikon_GetPreviewImg, 0 };
		ptp_append_uint16_32_array(PRIVATE_DATA->info_operations_supported, operations);
		INDIGO_LOG(indigo_log("operations (D3000-3100):"));
		for (uint32_t *operation = operations; *operation; operation++) {
			INDIGO_LOG(indigo_log("  %04x %s", *operation, ptp_operation_nikon_code_label(*operation)));
		}
	} else if (IS_NIKON_D3200_OR_LATER()) {
		static uint32_t operations[] = { ptp_operation_nikon_GetVendorPropCodes, ptp_operation_nikon_CheckEvent, ptp_operation_nikon_Capture, ptp_operation_nikon_AfDrive, ptp_operation_nikon_SetControlMode, ptp_operation_nikon_DeviceReady, ptp_operation_nikon_AfCaptureSDRAM, ptp_operation_nikon_DelImageSDRAM, ptp_operation_nikon_GetPreviewImg, ptp_operation_nikon_StartLiveView, ptp_operation_nikon_EndLiveView, ptp_operation_nikon_GetLiveViewImg, ptp_operation_nikon_MfDrive, ptp_operation_nikon_ChangeAfArea, ptp_operation_nikon_AfDriveCancel, 0 };
		ptp_append_uint16_32_array(PRIVATE_DATA->info_operations_supported, operations);
		INDIGO_LOG(indigo_log("operations (D3200-3500):"));
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
			uint32_t size = 0;
			for (int i = 0; properties[i]; i++) {
				target[index] = properties[i];
				if (ptp_transaction_1_0_i(device, ptp_operation_GetDevicePropDesc, target[index], &buffer, &size)) {
					ptp_decode_property(buffer, size, device, PRIVATE_DATA->properties + index);
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
	indigo_set_timer(device, 0.5, ptp_check_event, &PRIVATE_DATA->event_checker);
	return true;
}

bool ptp_nikon_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params) {
	return ptp_handle_event(device, code, params);
}

bool ptp_nikon_fix_property(indigo_device *device, ptp_property *property) {
	switch (property->code) {
		case ptp_property_ImageSize: {
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_nikon_AutoDXCrop));
			return true;
		}
		case ptp_property_nikon_ACPower: {
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_BatteryLevel));
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
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_ExposureBiasCompensation));
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_nikon_FlashExposureCompensation));
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_nikon_ExternalFlashCompensation));
			return true;
		}
		case ptp_property_nikon_EVISOStep: {
			property->count = 2;
			property->value.sw.values[0] = 0;
			property->value.sw.values[1] = 1;
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_nikon_ISOSensitivity));
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_ExposureIndex));
			ptp_refresh_property(device, ptp_property_supported(device, ptp_property_nikon_ExposureIndexHi));
			return true;
		}
		case ptp_property_nikon_ImageCommentEnable:
		case ptp_property_nikon_EnableCopyright:
		case ptp_property_nikon_AFAssist:
		case ptp_property_nikon_AFCModePriority:
		case ptp_property_nikon_AFSModePriority:
		case ptp_property_nikon_AFAreaPoint:
		case ptp_property_nikon_RawBitMode:
		case ptp_property_nikon_RawCompression:
		case ptp_property_nikon_AutoDXCrop: {
			property->count = 2;
			property->value.sw.values[0] = 0;
			property->value.sw.values[1] = 1;
			return true;
		}
		case ptp_property_nikon_CameraInclination:
		case ptp_property_nikon_VignetteCtrl: {
			property->count = 4;
			property->value.sw.values[0] = 0;
			property->value.sw.values[1] = 1;
			property->value.sw.values[2] = 2;
			property->value.sw.values[3] = 3;
			return true;
		}
		case ptp_property_nikon_ActiveDLighting:
		case ptp_property_nikon_AFLockOn: {
			property->count = 6;
			property->value.sw.values[0] = 0;
			property->value.sw.values[1] = 1;
			property->value.sw.values[2] = 2;
			property->value.sw.values[3] = 3;
			property->value.sw.values[4] = 4;
			property->value.sw.values[5] = 5;
			return true;
		}
		case ptp_property_nikon_LensID: {
			property->type = ptp_str_type;
			switch (property->value.number.value) {
				case 0x0000: strcpy(property->value.text.value, "Fisheye Nikkor 8mm f/2.8 AiS"); break;
				case 0x0001: strcpy(property->value.text.value, "AF Nikkor 50mm f/1.8"); break;
				case 0x0002: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-70mm f/3.3-4.5"); break;
				case 0x0003: strcpy(property->value.text.value, "AF Zoom-Nikkor 70-210mm f/4"); break;
				case 0x0004: strcpy(property->value.text.value, "AF Nikkor 28mm f/2.8"); break;
				case 0x0005: strcpy(property->value.text.value, "AF Nikkor 50mm f/1.4"); break;
				case 0x0006: strcpy(property->value.text.value, "AF Micro-Nikkor 55mm f/2.8"); break;
				case 0x0007: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-85mm f/3.5-4.5"); break;
				case 0x0008: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-105mm f/3.5-4.5"); break;
				case 0x0009: strcpy(property->value.text.value, "AF Nikkor 24mm f/2.8"); break;
				case 0x000A: strcpy(property->value.text.value, "AF Nikkor 300mm f/2.8 IF-ED"); break;
				case 0x000B: strcpy(property->value.text.value, "AF Nikkor 180mm f/2.8 IF-ED"); break;
				case 0x000D: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-135mm f/3.5-4.5"); break;
				case 0x000E: strcpy(property->value.text.value, "AF Zoom-Nikkor 70-210mm f/4"); break;
				case 0x000F: strcpy(property->value.text.value, "AF Nikkor 50mm f/1.8 N"); break;
				case 0x0010: strcpy(property->value.text.value, "AF Nikkor 300mm f/4 IF-ED"); break;
				case 0x0011: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-70mm f/2.8"); break;
				case 0x0012: strcpy(property->value.text.value, "AF Nikkor 70-210mm f/4-5.6"); break;
				case 0x0013: strcpy(property->value.text.value, "AF Zoom-Nikkor 24-50mm f/3.3-4.5"); break;
				case 0x0014: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/2.8 ED"); break;
				case 0x0015: strcpy(property->value.text.value, "AF Nikkor 85mm f/1.8"); break;
				case 0x0017: strcpy(property->value.text.value, "Nikkor 500mm f/4 P ED IF"); break;
				case 0x0018: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-135mm f/3.5-4.5 N"); break;
				case 0x001A: strcpy(property->value.text.value, "AF Nikkor 35mm f/2"); break;
				case 0x001B: strcpy(property->value.text.value, "AF Zoom-Nikkor 75-300mm f/4.5-5.6"); break;
				case 0x001C: strcpy(property->value.text.value, "AF Nikkor 20mm f/2.8"); break;
				case 0x001D: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-70mm f/3.3-4.5 N"); break;
				case 0x001E: strcpy(property->value.text.value, "AF Micro-Nikkor 60mm f/2.8"); break;
				case 0x001F: strcpy(property->value.text.value, "AF Micro-Nikkor 105mm f/2.8"); break;
				case 0x0020: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/2.8 ED"); break;
				case 0x0021: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-70mm f/3.5-4.5"); break;
				case 0x0022: strcpy(property->value.text.value, "AF DC-Nikkor 135mm f/2"); break;
				case 0x0023: strcpy(property->value.text.value, "Zoom-Nikkor 1200-1700mm f/5.6-8 P ED IF"); break;
				case 0x0024: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/2.8D ED"); break;
				case 0x0025: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-70mm f/2.8D"); break;
				case 0x0026: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-70mm f/3.5-4.5D"); break;
				case 0x0027: strcpy(property->value.text.value, "AF-I Nikkor 300mm f/2.8D IF-ED"); break;
				case 0x0028: strcpy(property->value.text.value, "AF-I Nikkor 600mm f/4D IF-ED"); break;
				case 0x002A: strcpy(property->value.text.value, "AF Nikkor 28mm f/1.4D"); break;
				case 0x002B: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-80mm f/4-5.6D"); break;
				case 0x002C: strcpy(property->value.text.value, "AF DC-Nikkor 105mm f/2D"); break;
				case 0x002D: strcpy(property->value.text.value, "AF Micro-Nikkor 200mm f/4D IF-ED"); break;
				case 0x002E: strcpy(property->value.text.value, "AF Nikkor 70-210mm f/4-5.6D"); break;
				case 0x002F: strcpy(property->value.text.value, "AF Zoom-Nikkor 20-35mm f/2.8D IF"); break;
				case 0x0030: strcpy(property->value.text.value, "AF-I Nikkor 400mm f/2.8D IF-ED"); break;
				case 0x0031: strcpy(property->value.text.value, "AF Micro-Nikkor 60mm f/2.8D"); break;
				case 0x0032: strcpy(property->value.text.value, "AF Micro-Nikkor 105mm f/2.8D"); break;
				case 0x0033: strcpy(property->value.text.value, "AF Nikkor 18mm f/2.8D"); break;
				case 0x0034: strcpy(property->value.text.value, "AF Fisheye Nikkor 16mm f/2.8D"); break;
				case 0x0035: strcpy(property->value.text.value, "AF-I Nikkor 500mm f/4D IF-ED"); break;
				case 0x0036: strcpy(property->value.text.value, "AF Nikkor 24mm f/2.8D"); break;
				case 0x0037: strcpy(property->value.text.value, "AF Nikkor 20mm f/2.8D"); break;
				case 0x0038: strcpy(property->value.text.value, "AF Nikkor 85mm f/1.8D"); break;
				case 0x003A: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-70mm f/3.5-4.5D"); break;
				case 0x003B: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-70mm f/2.8D N"); break;
				case 0x003C: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/2.8D ED"); break;
				case 0x003D: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-80mm f/4-5.6D"); break;
				case 0x003E: strcpy(property->value.text.value, "AF Nikkor 28mm f/2.8D"); break;
				case 0x003F: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-105mm f/3.5-4.5D"); break;
				case 0x0041: strcpy(property->value.text.value, "AF Nikkor 180mm f/2.8D IF-ED"); break;
				case 0x0042: strcpy(property->value.text.value, "AF Nikkor 35mm f/2D"); break;
				case 0x0043: strcpy(property->value.text.value, "AF Nikkor 50mm f/1.4D"); break;
				case 0x0044: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/4.5-5.6D"); break;
				case 0x0045: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-80mm f/3.5-5.6D"); break;
				case 0x0046: strcpy(property->value.text.value, "AF Zoom-Nikkor 35-80mm f/4-5.6D N"); break;
				case 0x0047: strcpy(property->value.text.value, "AF Zoom-Nikkor 24-50mm f/3.3-4.5D"); break;
				case 0x0048: strcpy(property->value.text.value, "AF-S Nikkor 300mm f/2.8D IF-ED"); break;
				case 0x0049: strcpy(property->value.text.value, "AF-S Nikkor 600mm f/4D IF-ED"); break;
				case 0x004A: strcpy(property->value.text.value, "AF Nikkor 85mm f/1.4D IF"); break;
				case 0x004B: strcpy(property->value.text.value, "AF-S Nikkor 500mm f/4D IF-ED"); break;
				case 0x004C: strcpy(property->value.text.value, "AF Zoom-Nikkor 24-120mm f/3.5-5.6D IF"); break;
				case 0x004D: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-200mm f/3.5-5.6D IF"); break;
				case 0x004E: strcpy(property->value.text.value, "AF DC-Nikkor 135mm f/2D"); break;
				case 0x004F: strcpy(property->value.text.value, "IX-Nikkor 24-70mm f/3.5-5.6"); break;
				case 0x0050: strcpy(property->value.text.value, "IX-Nikkor 60-180mm f/4-5.6"); break;
				case 0x0053: strcpy(property->value.text.value, "AF Zoom-Nikkor 80-200mm f/2.8D ED"); break;
				case 0x0054: strcpy(property->value.text.value, "AF Zoom-Micro Nikkor 70-180mm f/4.5-5.6D ED"); break;
				case 0x0056: strcpy(property->value.text.value, "AF Zoom-Nikkor 70-300mm f/4-5.6D ED"); break;
				case 0x0059: strcpy(property->value.text.value, "AF-S Nikkor 400mm f/2.8D IF-ED"); break;
				case 0x005A: strcpy(property->value.text.value, "IX-Nikkor 30-60mm f/4-5.6"); break;
				case 0x005B: strcpy(property->value.text.value, "IX-Nikkor 60-180mm f/4.5-5.6"); break;
				case 0x005D: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 28-70mm f/2.8D IF-ED"); break;
				case 0x005E: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 80-200mm f/2.8D IF-ED"); break;
				case 0x005F: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-105mm f/3.5-4.5D IF"); break;
				case 0x0060: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-80mm f/3.5-5.6D"); break;
				case 0x0061: strcpy(property->value.text.value, "AF Zoom-Nikkor 75-240mm f/4.5-5.6D"); break;
				case 0x0063: strcpy(property->value.text.value, "AF-S Nikkor 17-35mm f/2.8D IF-ED"); break;
				case 0x0064: strcpy(property->value.text.value, "PC Micro-Nikkor 85mm f/2.8D"); break;
				case 0x0065: strcpy(property->value.text.value, "AF VR Zoom-Nikkor 80-400mm f/4.5-5.6D ED"); break;
				case 0x0066: strcpy(property->value.text.value, "AF Zoom-Nikkor 18-35mm f/3.5-4.5D IF-ED"); break;
				case 0x0067: strcpy(property->value.text.value, "AF Zoom-Nikkor 24-85mm f/2.8-4D IF"); break;
				case 0x0068: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-80mm f/3.3-5.6G"); break;
				case 0x0069: strcpy(property->value.text.value, "AF Zoom-Nikkor 70-300mm f/4-5.6G"); break;
				case 0x006A: strcpy(property->value.text.value, "AF-S Nikkor 300mm f/4D IF-ED"); break;
				case 0x006B: strcpy(property->value.text.value, "AF Nikkor ED 14mm f/2.8D"); break;
				case 0x006D: strcpy(property->value.text.value, "AF-S Nikkor 300mm f/2.8D IF-ED II"); break;
				case 0x006E: strcpy(property->value.text.value, "AF-S Nikkor 400mm f/2.8D IF-ED II"); break;
				case 0x006F: strcpy(property->value.text.value, "AF-S Nikkor 500mm f/4D IF-ED II"); break;
				case 0x0070: strcpy(property->value.text.value, "AF-S Nikkor 600mm f/4D IF-ED II"); break;
				case 0x0072: strcpy(property->value.text.value, "Nikkor 45mm f/2.8 P"); break;
				case 0x0074: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 24-85mm f/3.5-4.5G IF-ED"); break;
				case 0x0075: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-100mm f/3.5-5.6G"); break;
				case 0x0076: strcpy(property->value.text.value, "AF Nikkor 50mm f/1.8D"); break;
				case 0x0077: strcpy(property->value.text.value, "AF-S VR Zoom-Nikkor 70-200mm f/2.8G IF-ED"); break;
				case 0x0078: strcpy(property->value.text.value, "AF-S VR Zoom-Nikkor 24-120mm f/3.5-5.6G IF-ED"); break;
				case 0x0079: strcpy(property->value.text.value, "AF Zoom-Nikkor 28-200mm f/3.5-5.6G IF-ED"); break;
				case 0x007A: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 12-24mm f/4G IF-ED"); break;
				case 0x007B: strcpy(property->value.text.value, "AF-S VR Zoom-Nikkor 200-400mm f/4G IF-ED"); break;
				case 0x007D: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 17-55mm f/2.8G IF-ED"); break;
				case 0x007F: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 18-70mm f/3.5-4.5G IF-ED"); break;
				case 0x0080: strcpy(property->value.text.value, "AF DX Fisheye-Nikkor 10.5mm f/2.8G ED"); break;
				case 0x0081: strcpy(property->value.text.value, "AF-S VR Nikkor 200mm f/2G IF-ED"); break;
				case 0x0082: strcpy(property->value.text.value, "AF-S VR Nikkor 300mm f/2.8G IF-ED"); break;
				case 0x0083: strcpy(property->value.text.value, "FSA-L2, EDG 65, 800mm F13 G"); break;
				case 0x0089: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 55-200mm f/4-5.6G ED"); break;
				case 0x008A: strcpy(property->value.text.value, "AF-S VR Micro-Nikkor 105mm f/2.8G IF-ED"); break;
				case 0x008B: strcpy(property->value.text.value, "AF-S DX VR Zoom-Nikkor 18-200mm f/3.5-5.6G IF-ED"); break;
				case 0x008C: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 18-55mm f/3.5-5.6G ED"); break;
				case 0x008D: strcpy(property->value.text.value, "AF-S VR Zoom-Nikkor 70-300mm f/4.5-5.6G IF-ED"); break;
				case 0x008F: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 18-135mm f/3.5-5.6G IF-ED"); break;
				case 0x0090: strcpy(property->value.text.value, "AF-S DX VR Zoom-Nikkor 55-200mm f/4-5.6G IF-ED"); break;
				case 0x0092: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 14-24mm f/2.8G ED"); break;
				case 0x0093: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 24-70mm f/2.8G ED"); break;
				case 0x0094: strcpy(property->value.text.value, "AF-S DX Zoom-Nikkor 18-55mm f/3.5-5.6G ED II"); break;
				case 0x0095: strcpy(property->value.text.value, "PC-E Nikkor 24mm f/3.5D ED"); break;
				case 0x0096: strcpy(property->value.text.value, "AF-S VR Nikkor 400mm f/2.8G ED"); break;
				case 0x0097: strcpy(property->value.text.value, "AF-S VR Nikkor 500mm f/4G ED"); break;
				case 0x0098: strcpy(property->value.text.value, "AF-S VR Nikkor 600mm f/4G ED"); break;
				case 0x0099: strcpy(property->value.text.value, "AF-S DX VR Zoom-Nikkor 16-85mm f/3.5-5.6G ED"); break;
				case 0x009A: strcpy(property->value.text.value, "AF-S DX VR Zoom-Nikkor 18-55mm f/3.5-5.6G"); break;
				case 0x009B: strcpy(property->value.text.value, "PC-E Micro Nikkor 45mm f/2.8D ED"); break;
				case 0x009C: strcpy(property->value.text.value, "AF-S Micro Nikkor 60mm f/2.8G ED"); break;
				case 0x009D: strcpy(property->value.text.value, "PC-E Micro Nikkor 85mm f/2.8D"); break;
				case 0x009E: strcpy(property->value.text.value, "AF-S DX VR Zoom-Nikkor 18-105mm f/3.5-5.6G ED"); break;
				case 0x009F: strcpy(property->value.text.value, "AF-S DX Nikkor 35mm f/1.8G"); break;
				case 0x00A0: strcpy(property->value.text.value, "AF-S Nikkor 50mm f/1.4G"); break;
				case 0x00A1: strcpy(property->value.text.value, "AF-S DX Nikkor 10-24mm f/3.5-4.5G ED"); break;
				case 0x00A2: strcpy(property->value.text.value, "AF-S Nikkor 70-200mm f/2.8G ED VR II"); break;
				case 0x00A3: strcpy(property->value.text.value, "AF-S Nikkor 16-35mm f/4G ED VR"); break;
				case 0x00A4: strcpy(property->value.text.value, "AF-S Nikkor 24mm f/1.4G ED"); break;
				case 0x00A5: strcpy(property->value.text.value, "AF-S Nikkor 28-300mm f/3.5-5.6G ED VR"); break;
				case 0x00A6: strcpy(property->value.text.value, "AF-S Nikkor 300mm f/2.8G IF-ED VR II"); break;
				case 0x00A7: strcpy(property->value.text.value, "AF-S DX Micro Nikkor 85mm f/3.5G ED VR"); break;
				case 0x00A8: strcpy(property->value.text.value, "AF-S Zoom-Nikkor 200-400mm f/4G IF-ED VR II"); break;
				case 0x00A9: strcpy(property->value.text.value, "AF-S Nikkor 200mm f/2G ED VR II"); break;
				case 0x00AA: strcpy(property->value.text.value, "AF-S Nikkor 24-120mm f/4G ED VR"); break;
				case 0x00AC: strcpy(property->value.text.value, "AF-S DX Nikkor 55-300mm f/4.5-5.6G ED VR"); break;
				case 0x00AD: strcpy(property->value.text.value, "AF-S DX Nikkor 18-300mm f/3.5-5.6G ED VR"); break;
				case 0x00AE: strcpy(property->value.text.value, "AF-S Nikkor 85mm f/1.4G"); break;
				case 0x00AF: strcpy(property->value.text.value, "AF-S Nikkor 35mm f/1.4G"); break;
				case 0x00B0: strcpy(property->value.text.value, "AF-S Nikkor 50mm f/1.8G"); break;
				case 0x00B1: strcpy(property->value.text.value, "AF-S DX Micro Nikkor 40mm f/2.8G"); break;
				case 0x00B2: strcpy(property->value.text.value, "AF-S Nikkor 70-200mm f/4G ED VR"); break;
				case 0x00B3: strcpy(property->value.text.value, "AF-S Nikkor 85mm f/1.8G"); break;
				case 0x00B4: strcpy(property->value.text.value, "AF-S Nikkor 24-85mm f/3.5-4.5G ED VR"); break;
				case 0x00B5: strcpy(property->value.text.value, "AF-S Nikkor 28mm f/1.8G"); break;
				case 0x00B6: strcpy(property->value.text.value, "AF-S VR Nikkor 800mm f/5.6E FL ED"); break;
				case 0x00B7: strcpy(property->value.text.value, "AF-S Nikkor 80-400mm f/4.5-5.6G ED VR"); break;
				case 0x00B8: strcpy(property->value.text.value, "AF-S Nikkor 18-35mm f/3.5-4.5G ED"); break;
				case 0x01A0: strcpy(property->value.text.value, "AF-S DX Nikkor 18-140mm f/3.5-5.6G ED VR"); break;
				case 0x01A1: strcpy(property->value.text.value, "AF-S Nikkor 58mm f/1.4G"); break;
				case 0x01A2: strcpy(property->value.text.value, "AF-S DX Nikkor 18-55mm f/3.5-5.6G VR II"); break;
				case 0x01A4: strcpy(property->value.text.value, "AF-S DX Nikkor 18-300mm f/3.5-6.3G ED VR"); break;
				case 0x01A5: strcpy(property->value.text.value, "AF-S Nikkor 35mm f/1.8G ED"); break;
				case 0x01A6: strcpy(property->value.text.value, "AF-S Nikkor 400mm f/2.8E FL ED VR"); break;
				case 0x01A7: strcpy(property->value.text.value, "AF-S DX Nikkor 55-200mm f/4-5.6G ED VR II"); break;
				case 0x01A8: strcpy(property->value.text.value, "AF-S Nikkor 300mm f/4E PF ED VR"); break;
				case 0x01A9: strcpy(property->value.text.value, "AF-S Nikkor 20mm f/1.8G ED"); break;
				case 0x02AA: strcpy(property->value.text.value, "AF-S Nikkor 24-70mm f/2.8E ED VR"); break;
				case 0x02AB: strcpy(property->value.text.value, "AF-S Nikkor 500mm f/4E FL ED VR"); break;
				case 0x02AC: strcpy(property->value.text.value, "AF-S Nikkor 600mm f/4E FL ED VR"); break;
				case 0x02AD: strcpy(property->value.text.value, "AF-S DX Nikkor 16-80mm f/2.8-4E ED VR"); break;
				case 0x02AE: strcpy(property->value.text.value, "AF-S Nikkor 200-500mm f/5.6E ED VR"); break;
				case 0x03A0: strcpy(property->value.text.value, "AF-P DX Nikkor 18-55mm f/3.5-5.6G VR"); break;
				case 0x03A3: strcpy(property->value.text.value, "AF-P DX Nikkor 70–300mm f/4.5–6.3G ED VR"); break;
				case 0x03A4: strcpy(property->value.text.value, "AF-S Nikkor 70-200mm f/2.8E FL ED VR"); break;
				case 0x03A5: strcpy(property->value.text.value, "AF-S Nikkor 105mm f/1.4E ED"); break;
				case 0x03AF: strcpy(property->value.text.value, "AF-S Nikkor 24mm f/1.8G ED"); break;
				default: strcpy(property->value.text.value, "Unknown lens"); break;
			}
			return true;
		}
		case ptp_property_CompressionSetting: {
			if (IS_NIKON_EXPEED5_OR_LATER())
				NIKON_PRIVATE_DATA->is_dual_compression = property->value.sw.value >= 8 && property->value.sw.value <= 13;
			else
				NIKON_PRIVATE_DATA->is_dual_compression = property->value.sw.value >= 5 && property->value.sw.value <= 7;
			return true;
		}
	}
	return false;
}

bool ptp_nikon_set_property(indigo_device *device, ptp_property *property) {
	bool result = ptp_set_property(device, property);
	if (property->code == ptp_property_CompressionSetting) {
		if (IS_NIKON_EXPEED5_OR_LATER())
			NIKON_PRIVATE_DATA->is_dual_compression = property->value.sw.value >= 8 && property->value.sw.value <= 13;
		else
			NIKON_PRIVATE_DATA->is_dual_compression = property->value.sw.value >= 5 && property->value.sw.value <= 7;
	}
	return result;
}

bool ptp_nikon_exposure(indigo_device *device) {
	ptp_property *property = ptp_property_supported(device, ptp_property_nikon_ExposureDelayMode);
	bool result = true;
	if (property) {
		uint8_t value;
		if (DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value) {
			if (property->form == ptp_enum_form && property->count == 6)
				value = 10;
			else
				value = 1;
		} else {
			if (property->form == ptp_range_form && property->value.number.max == 3)
				value = 3;
			else
				value = 0;
		}
		result = result && ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_ExposureDelayMode, &value, sizeof(uint8_t));
	}
	property = ptp_property_supported(device, ptp_property_ExposureTime);
	if (property && (property->value.sw.value != 0xffffffff || CCD_EXPOSURE_ITEM->number.value > 0)) { // if shutter time is BULB and exposure time is 0, just wait for external shutter exposure
		if (ptp_operation_supported(device, ptp_operation_nikon_InitiateCaptureRecInMedia)) {
			result = result && ptp_transaction_2_0(device, ptp_operation_nikon_InitiateCaptureRecInMedia, -1, 0);
		} else {
			result = result && ptp_transaction_2_0(device, ptp_operation_InitiateCapture, 0, 0);
		}
		if (property->value.sw.value == 0xffffffff) {
			CCD_EXPOSURE_ITEM->number.value += DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value ? 2 : 0;
			ptp_blob_exposure_timer(device);
			result = result && ptp_transaction_2_0(device, ptp_operation_nikon_TerminateCapture, 0, 0);
		}
	}
	if (result) {
		if (CCD_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE && CCD_PREVIEW_ENABLED_ITEM->sw.value && ptp_nikon_check_dual_compression(device)) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		}
		while (true) {
			if (PRIVATE_DATA->abort_capture || (CCD_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_PREVIEW_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE && CCD_IMAGE_FILE_PROPERTY->state != INDIGO_BUSY_STATE))
				break;
			indigo_usleep(100000);
		}
	}
	if (!result || PRIVATE_DATA->abort_capture) {
		if (CCD_IMAGE_PROPERTY->state != INDIGO_OK_STATE) {
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (CCD_PREVIEW_IMAGE_PROPERTY->state != INDIGO_OK_STATE) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		}
		if (CCD_IMAGE_FILE_PROPERTY->state != INDIGO_OK_STATE) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
	}
	return result && !PRIVATE_DATA->abort_capture;
}

bool ptp_nikon_liveview(indigo_device *device) {
	if (ptp_transaction_0_0(device, ptp_operation_nikon_StartLiveView)) {
		uint8_t *buffer = NULL;
		uint32_t size;
		while (!PRIVATE_DATA->abort_capture && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			if (ptp_transaction_0_0_i(device, ptp_operation_nikon_GetLiveViewImg, (void **)&buffer, &size)) {
				if ((buffer[64] & 0xFF) == 0xFF && (buffer[65] & 0xFF) == 0xD8) {
					if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
					}
					if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
					}
					indigo_process_dslr_image(device, (void *)buffer + 64, size - 64, ".jpeg", true);
					if (PRIVATE_DATA->image_buffer)
						free(PRIVATE_DATA->image_buffer);
					PRIVATE_DATA->image_buffer = buffer;
					buffer = NULL;
					CCD_STREAMING_COUNT_ITEM->number.value--;
					if (CCD_STREAMING_COUNT_ITEM->number.value < 0)
						CCD_STREAMING_COUNT_ITEM->number.value = -1;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				} else if ((buffer[128] & 0xFF) == 0xFF && (buffer[129] & 0xFF) == 0xD8) {
					if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
					}
					if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
					}
					indigo_process_dslr_image(device, (void *)buffer + 128, size - 128, ".jpeg", true);
					if (PRIVATE_DATA->image_buffer)
						free(PRIVATE_DATA->image_buffer);
					PRIVATE_DATA->image_buffer = buffer;
					buffer = NULL;
					CCD_STREAMING_COUNT_ITEM->number.value--;
					if (CCD_STREAMING_COUNT_ITEM->number.value < 0)
						CCD_STREAMING_COUNT_ITEM->number.value = -1;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				} else if ((buffer[384] & 0xFF) == 0xFF && (buffer[385] & 0xFF) == 0xD8) {
					if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
					}
					if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
						CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
					}
					indigo_process_dslr_image(device, (void *)buffer + 384, size - 384, ".jpeg", true);
					if (PRIVATE_DATA->image_buffer)
						free(PRIVATE_DATA->image_buffer);
					PRIVATE_DATA->image_buffer = buffer;
					buffer = NULL;
					CCD_STREAMING_COUNT_ITEM->number.value--;
					if (CCD_STREAMING_COUNT_ITEM->number.value < 0)
						CCD_STREAMING_COUNT_ITEM->number.value = -1;
					indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
				}
			}
			if (buffer)
				free(buffer);
			buffer = NULL;
			indigo_usleep(100000);
		}
		indigo_finalize_video_stream(device);
		ptp_transaction_0_0(device, ptp_operation_nikon_EndLiveView);
		return !PRIVATE_DATA->abort_capture;
	}
	return false;
}

bool ptp_nikon_lock(indigo_device *device) {
	if (ptp_operation_supported(device, ptp_operation_nikon_SetControlMode)) {
		if (DSLR_LOCK_ITEM->sw.value)
			return ptp_transaction_1_0(device, ptp_operation_nikon_SetControlMode, 1) && ptp_refresh_property(device, ptp_property_supported(device, ptp_property_ExposureProgramMode));
		return ptp_transaction_1_0(device, ptp_operation_nikon_SetControlMode, 0) && ptp_refresh_property(device, ptp_property_supported(device, ptp_property_ExposureProgramMode));
	}
	return false;
}

bool ptp_nikon_zoom(indigo_device *device) {
	if (ptp_property_supported(device, ptp_property_nikon_LiveViewImageZoomRatio)) {
		uint8_t value = DSLR_ZOOM_PREVIEW_ON_ITEM->sw.value ? 5 : 0;
		return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_LiveViewImageZoomRatio, &value, sizeof(uint8_t));
	} else {
		if (IS_NIKON_EXPEED6_OR_LATER()) {
			uint16_t value = DSLR_ZOOM_PREVIEW_ON_ITEM->sw.value ? 0x200 : 0;
			return ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_LiveViewZoomArea, &value, sizeof(uint16_t));
		}
	}
	return false;
}

bool ptp_nikon_focus(indigo_device *device, int steps) {
	if (steps == 0)
		return true;
	bool result = true;
	ptp_property *property = ptp_property_supported(device, ptp_property_nikon_LiveViewAFFocus);
	if (property && ptp_operation_supported(device, ptp_operation_nikon_MfDrive)) {
		bool temporary_lv = false;
		if (CCD_STREAMING_PROPERTY->state != INDIGO_BUSY_STATE && ptp_transaction_0_0(device, ptp_operation_nikon_StartLiveView)) {
			temporary_lv = true;
		}
//		if (property->value.number.value != 0) {
//			uint8_t value = 0;
//			result = ptp_transaction_0_1_o(device, ptp_operation_SetDevicePropValue, ptp_property_nikon_LiveViewImageZoomRatio, &value, sizeof(uint8_t));
//		}
		if (result) {
			for (int i = 0; i < 100; i++) {
				if (steps > 0)
					result = ptp_transaction_2_0(device, ptp_operation_nikon_MfDrive, 1, steps);
				else
					result = ptp_transaction_2_0(device, ptp_operation_nikon_MfDrive, 2, -steps);
				if (result)
					break;
				indigo_usleep(10000);
			}
		}
		if (temporary_lv) {
			for (int i = 0; i < 100; i++) {
				result = ptp_transaction_0_0(device, ptp_operation_nikon_EndLiveView);
				if (result)
					break;
				indigo_usleep(10000);
			}
		}
	} else {
		result = false;
	}
	return result;
}

bool ptp_nikon_check_dual_compression(indigo_device *device) {
	return NIKON_PRIVATE_DATA->is_dual_compression;
}
