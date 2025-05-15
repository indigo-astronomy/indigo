// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

#ifndef indigo_ptp_canon_h
#define indigo_ptp_canon_h

#include <indigo/indigo_driver.h>

typedef enum {
	ptp_operation_canon_GetStorageIDs = 0x9101,
	ptp_operation_canon_GetStorageInfo = 0x9102,
	ptp_operation_canon_GetObjectInfo = 0x9103,
	ptp_operation_canon_GetObject = 0x9104,
	ptp_operation_canon_DeleteObject = 0x9105,
	ptp_operation_canon_FormatStore = 0x9106,
	ptp_operation_canon_GetPartialObject = 0x9107,
	ptp_operation_canon_GetDeviceInfoEx = 0x9108,
	ptp_operation_canon_GetObjectInfoEx = 0x9109,
	ptp_operation_canon_GetThumbEx = 0x910A,
	ptp_operation_canon_SendPartialObject = 0x910B,
	ptp_operation_canon_SetObjectAttributes = 0x910C,
	ptp_operation_canon_GetObjectTime = 0x910D,
	ptp_operation_canon_SetObjectTime = 0x910E,
	ptp_operation_canon_RemoteRelease = 0x910F,
	ptp_operation_canon_SetDevicePropValueEx = 0x9110,
	ptp_operation_canon_GetRemoteMode = 0x9113,
	ptp_operation_canon_SetRemoteMode = 0x9114,
	ptp_operation_canon_SetEventMode = 0x9115,
	ptp_operation_canon_GetEvent = 0x9116,
	ptp_operation_canon_TransferComplete = 0x9117,
	ptp_operation_canon_CancelTransfer = 0x9118,
	ptp_operation_canon_ResetTransfer = 0x9119,
	ptp_operation_canon_PCHDDCapacity = 0x911A,
	ptp_operation_canon_SetUILock = 0x911B,
	ptp_operation_canon_ResetUILock = 0x911C,
	ptp_operation_canon_KeepDeviceOn = 0x911D,
	ptp_operation_canon_SetNullPacketMode = 0x911E,
	ptp_operation_canon_UpdateFirmware = 0x911F,
	ptp_operation_canon_TransferCompleteDT = 0x9120,
	ptp_operation_canon_CancelTransferDT = 0x9121,
	ptp_operation_canon_SetWftProfile = 0x9122,
	ptp_operation_canon_GetWftProfile = 0x9123,
	ptp_operation_canon_SetProfileToWft = 0x9124,
	ptp_operation_canon_BulbStart = 0x9125,
	ptp_operation_canon_BulbEnd = 0x9126,
	ptp_operation_canon_RequestDevicePropValue = 0x9127,
	ptp_operation_canon_RemoteReleaseOn = 0x9128,
	ptp_operation_canon_RemoteReleaseOff = 0x9129,
	ptp_operation_canon_RegistBackgroundImage = 0x912A,
	ptp_operation_canon_ChangePhotoStudioMode = 0x912B,
	ptp_operation_canon_GetPartialObjectEx = 0x912C,
	ptp_operation_canon_ResetMirrorLockupState = 0x9130,
	ptp_operation_canon_PopupBuiltinFlash = 0x9131,
	ptp_operation_canon_EndGetPartialObjectEx = 0x9132,
	ptp_operation_canon_MovieSelectSWOn = 0x9133,
	ptp_operation_canon_MovieSelectSWOff = 0x9134,
	ptp_operation_canon_GetCTGInfo = 0x9135,
	ptp_operation_canon_GetLensAdjust = 0x9136,
	ptp_operation_canon_SetLensAdjust = 0x9137,
	ptp_operation_canon_GetMusicInfo = 0x9138,
	ptp_operation_canon_CreateHandle = 0x9139,
	ptp_operation_canon_SendPartialObjectEx = 0x913A,
	ptp_operation_canon_EndSendPartialObjectEx = 0x913B,
	ptp_operation_canon_SetCTGInfo = 0x913C,
	ptp_operation_canon_SetRequestOLCInfoGroup = 0x913D,
	ptp_operation_canon_SetRequestRollingPitchingLevel = 0x913E,
	ptp_operation_canon_GetCameraSupport = 0x913F,
	ptp_operation_canon_SetRating = 0x9140,
	ptp_operation_canon_RequestInnerDevelopStart = 0x9141,
	ptp_operation_canon_RequestInnerDevelopParamChange = 0x9142,
	ptp_operation_canon_RequestInnerDevelopEnd = 0x9143,
	ptp_operation_canon_GpsLoggingDataMode = 0x9144,
	ptp_operation_canon_GetGpsLogCurrentHandle = 0x9145,
	ptp_operation_canon_InitiateViewfinder = 0x9151,
	ptp_operation_canon_TerminateViewfinder = 0x9152,
	ptp_operation_canon_GetViewFinderData = 0x9153,
	ptp_operation_canon_DoAf = 0x9154,
	ptp_operation_canon_DriveLens = 0x9155,
	ptp_operation_canon_DepthOfFieldPreview = 0x9156,
	ptp_operation_canon_ClickWB = 0x9157,
	ptp_operation_canon_Zoom = 0x9158,
	ptp_operation_canon_ZoomPosition = 0x9159,
	ptp_operation_canon_SetLiveAfFrame = 0x915A,
	ptp_operation_canon_TouchAfPosition = 0x915B,
	ptp_operation_canon_SetLvPcFlavoreditMode = 0x915C,
	ptp_operation_canon_SetLvPcFlavoreditParam = 0x915D,
	ptp_operation_canon_AfCancel = 0x9160,
	ptp_operation_canon_SetDefaultCameraSetting = 0x91BE,
	ptp_operation_canon_GetAEData = 0x91BF,
	ptp_operation_canon_NotifyNetworkError = 0x91E8,
	ptp_operation_canon_AdapterTransferProgress = 0x91E9,
	ptp_operation_canon_TransferComplete2 = 0x91F0,
	ptp_operation_canon_CancelTransfer2 = 0x91F1,
	ptp_operation_canon_FAPIMessageTX = 0x91FE,
	ptp_operation_canon_FAPIMessageRX = 0x91FF
} ptp_operation_canon_code;

typedef enum {
	ptp_response_canon_UnknownCommand = 0xA001,
	ptp_response_canon_OperationRefused = 0xA005,
	ptp_response_canon_LensCover = 0xA006,
	ptp_response_canon_BatteryLow = 0xA101,
	ptp_response_canon_NotReady = 0xA102
} ptp_response_canon_code;

typedef enum {
	ptp_event_canon_RequestGetEvent = 0xC101,
	ptp_event_canon_ObjectAddedEx = 0xC181,
	ptp_event_canon_ObjectRemoved = 0xC182,
	ptp_event_canon_RequestGetObjectInfoEx = 0xC183,
	ptp_event_canon_StorageStatusChanged = 0xC184,
	ptp_event_canon_StorageInfoChanged = 0xC185,
	ptp_event_canon_RequestObjectTransfer = 0xC186,
	ptp_event_canon_ObjectInfoChangedEx = 0xC187,
	ptp_event_canon_ObjectContentChanged = 0xC188,
	ptp_event_canon_PropValueChanged = 0xC189,
	ptp_event_canon_AvailListChanged = 0xC18A,
	ptp_event_canon_CameraStatusChanged = 0xC18B,
	ptp_event_canon_WillSoonShutdown = 0xC18D,
	ptp_event_canon_ShutdownTimerUpdated = 0xC18E,
	ptp_event_canon_RequestCancelTransfer = 0xC18F,
	ptp_event_canon_RequestObjectTransferDT = 0xC190,
	ptp_event_canon_RequestCancelTransferDT = 0xC191,
	ptp_event_canon_StoreAdded = 0xC192,
	ptp_event_canon_StoreRemoved = 0xC193,
	ptp_event_canon_BulbExposureTime = 0xC194,
	ptp_event_canon_RecordingTime = 0xC195,
	ptp_event_canon_RequestObjectTransferTS = 0xC1A2,
	ptp_event_canon_AfResult = 0xC1A3,
	ptp_event_canon_CTGInfoCheckComplete = 0xC1A4,
	ptp_event_canon_OLCInfoChanged = 0xC1A5,
	ptp_event_canon_ObjectAddedEx2 = 0xC1A7,
	ptp_event_canon_RequestObjectTransferFTP = 0xC1F1
} ptp_event_canon_code;

typedef enum {
	ptp_property_canon_Aperture = 0xD101,
	ptp_property_canon_ShutterSpeed = 0xD102,
	ptp_property_canon_ISOSpeed = 0xD103,
	ptp_property_canon_ExpCompensation = 0xD104,
	ptp_property_canon_AutoExposureMode = 0xD105,
	ptp_property_canon_DriveMode = 0xD106,
	ptp_property_canon_MeteringMode = 0xD107,
	ptp_property_canon_FocusMode = 0xD108,
	ptp_property_canon_WhiteBalance = 0xD109,
	ptp_property_canon_ColorTemperature = 0xD10A,
	ptp_property_canon_WhiteBalanceAdjustA = 0xD10B,
	ptp_property_canon_WhiteBalanceAdjustB = 0xD10C,
	ptp_property_canon_WhiteBalanceXA = 0xD10D,
	ptp_property_canon_WhiteBalanceXB = 0xD10E,
	ptp_property_canon_ColorSpace = 0xD10F,
	ptp_property_canon_PictureStyle = 0xD110,
	ptp_property_canon_BatteryPower = 0xD111,
	ptp_property_canon_BatterySelect = 0xD112,
	ptp_property_canon_CameraTime = 0xD113,
	ptp_property_canon_AutoPowerOff = 0xD114,
	ptp_property_canon_Owner = 0xD115,
	ptp_property_canon_ModelID = 0xD116,
	ptp_property_canon_PTPExtensionVersion = 0xD119,
	ptp_property_canon_DPOFVersion = 0xD11A,
	ptp_property_canon_AvailableShots = 0xD11B,
	ptp_property_canon_CaptureDestination = 0xD11C,
	ptp_property_canon_BracketMode = 0xD11D,
	ptp_property_canon_CurrentStorage = 0xD11E,
	ptp_property_canon_CurrentFolder = 0xD11F,
	ptp_property_canon_ImageFormat = 0xD120	,
	ptp_property_canon_ImageFormatCF = 0xD121	,
	ptp_property_canon_ImageFormatSD = 0xD122	,
	ptp_property_canon_ImageFormatExtHD = 0xD123	,
	ptp_property_canon_CompressionS = 0xD130,
	ptp_property_canon_CompressionM1 = 0xD131,
	ptp_property_canon_CompressionM2 = 0xD132,
	ptp_property_canon_CompressionL = 0xD133,
	ptp_property_canon_AEModeDial = 0xD138,
	ptp_property_canon_AEModeCustom = 0xD139,
	ptp_property_canon_MirrorUpSetting = 0xD13A,
	ptp_property_canon_HighlightTonePriority = 0xD13B,
	ptp_property_canon_AFSelectFocusArea = 0xD13C,
	ptp_property_canon_HDRSetting = 0xD13D,
	ptp_property_canon_PCWhiteBalance1 = 0xD140,
	ptp_property_canon_PCWhiteBalance2 = 0xD141,
	ptp_property_canon_PCWhiteBalance3 = 0xD142,
	ptp_property_canon_PCWhiteBalance4 = 0xD143,
	ptp_property_canon_PCWhiteBalance5 = 0xD144,
	ptp_property_canon_MWhiteBalance = 0xD145,
	ptp_property_canon_MWhiteBalanceEx = 0xD146,
	ptp_property_canon_UnknownPropD14D = 0xD14D  ,
	ptp_property_canon_PictureStyleStandard = 0xD150,
	ptp_property_canon_PictureStylePortrait = 0xD151,
	ptp_property_canon_PictureStyleLandscape = 0xD152,
	ptp_property_canon_PictureStyleNeutral = 0xD153,
	ptp_property_canon_PictureStyleFaithful = 0xD154,
	ptp_property_canon_PictureStyleBlackWhite = 0xD155,
	ptp_property_canon_PictureStyleAuto = 0xD156,
	ptp_property_canon_PictureStyleUserSet1 = 0xD160,
	ptp_property_canon_PictureStyleUserSet2 = 0xD161,
	ptp_property_canon_PictureStyleUserSet3 = 0xD162,
	ptp_property_canon_PictureStyleParam1 = 0xD170,
	ptp_property_canon_PictureStyleParam2 = 0xD171,
	ptp_property_canon_PictureStyleParam3 = 0xD172,
	ptp_property_canon_HighISOSettingNoiseReduction = 0xD178,
	ptp_property_canon_MovieServoAF = 0xD179,
	ptp_property_canon_ContinuousAFValid = 0xD17A,
	ptp_property_canon_Attenuator = 0xD17B,
	ptp_property_canon_UTCTime = 0xD17C,
	ptp_property_canon_Timezone = 0xD17D,
	ptp_property_canon_Summertime = 0xD17E,
	ptp_property_canon_FlavorLUTParams = 0xD17F,
	ptp_property_canon_CustomFunc1 = 0xD180,
	ptp_property_canon_CustomFunc2 = 0xD181,
	ptp_property_canon_CustomFunc3 = 0xD182,
	ptp_property_canon_CustomFunc4 = 0xD183,
	ptp_property_canon_CustomFunc5 = 0xD184,
	ptp_property_canon_CustomFunc6 = 0xD185,
	ptp_property_canon_CustomFunc7 = 0xD186,
	ptp_property_canon_CustomFunc8 = 0xD187,
	ptp_property_canon_CustomFunc9 = 0xD188,
	ptp_property_canon_CustomFunc10 = 0xD189,
	ptp_property_canon_CustomFunc11 = 0xD18a,
	ptp_property_canon_CustomFunc12 = 0xD18b,
	ptp_property_canon_CustomFunc13 = 0xD18c,
	ptp_property_canon_CustomFunc14 = 0xD18d,
	ptp_property_canon_CustomFunc15 = 0xD18e,
	ptp_property_canon_CustomFunc16 = 0xD18f,
	ptp_property_canon_CustomFunc17 = 0xD190,
	ptp_property_canon_CustomFunc18 = 0xD191,
	ptp_property_canon_CustomFunc19 = 0xD192,
	ptp_property_canon_InnerDevelop = 0xD193,
	ptp_property_canon_MultiAspect = 0xD194,
	ptp_property_canon_MovieSoundRecord = 0xD195,
	ptp_property_canon_MovieRecordVolume = 0xD196,
	ptp_property_canon_WindCut = 0xD197,
	ptp_property_canon_ExtenderType = 0xD198,
	ptp_property_canon_OLCInfoVersion = 0xD199,
	ptp_property_canon_UnknownPropD19A = 0xD19A ,
	ptp_property_canon_UnknownPropD19C = 0xD19C ,
	ptp_property_canon_UnknownPropD19D = 0xD19D ,
	ptp_property_canon_CustomFuncEx = 0xD1a0,
	ptp_property_canon_MyMenu = 0xD1a1,
	ptp_property_canon_MyMenuList = 0xD1a2,
	ptp_property_canon_WftStatus = 0xD1a3,
	ptp_property_canon_WftInputTransmission = 0xD1a4,
	ptp_property_canon_HDDirectoryStructure = 0xD1a5,
	ptp_property_canon_BatteryInfo = 0xD1a6,
	ptp_property_canon_AdapterInfo = 0xD1a7,
	ptp_property_canon_LensStatus = 0xD1a8,
	ptp_property_canon_QuickReviewTime = 0xD1a9,
	ptp_property_canon_CardExtension = 0xD1aa,
	ptp_property_canon_TempStatus = 0xD1ab,
	ptp_property_canon_ShutterCounter = 0xD1ac,
	ptp_property_canon_SpecialOption = 0xD1ad,
	ptp_property_canon_PhotoStudioMode = 0xD1ae,
	ptp_property_canon_SerialNumber = 0xD1af,
	ptp_property_canon_EVFOutputDevice = 0xD1b0,
	ptp_property_canon_EVFMode = 0xD1b1,
	ptp_property_canon_DepthOfFieldPreview = 0xD1b2,
	ptp_property_canon_EVFSharpness = 0xD1b3,
	ptp_property_canon_EVFWBMode = 0xD1b4,
	ptp_property_canon_EVFClickWBCoeffs = 0xD1b5,
	ptp_property_canon_EVFColorTemp = 0xD1b6,
	ptp_property_canon_ExposureSimMode = 0xD1b7,
	ptp_property_canon_EVFRecordStatus = 0xD1b8,
	ptp_property_canon_LvAfSystem = 0xD1ba,
	ptp_property_canon_MovSize = 0xD1bb,
	ptp_property_canon_LvViewTypeSelect = 0xD1bc,
	ptp_property_canon_MirrorDownStatus = 0xD1bd,
	ptp_property_canon_MovieParam = 0xD1be,
	ptp_property_canon_MirrorLockupState = 0xD1bf,
	ptp_property_canon_FlashChargingState = 0xD1C0,
	ptp_property_canon_AloMode = 0xD1C1,
	ptp_property_canon_FixedMovie = 0xD1C2,
	ptp_property_canon_OneShotRawOn = 0xD1C3,
	ptp_property_canon_ErrorForDisplay = 0xD1C4,
	ptp_property_canon_AEModeMovie = 0xD1C5,
	ptp_property_canon_BuiltinStroboMode = 0xD1C6,
	ptp_property_canon_StroboDispState = 0xD1C7,
	ptp_property_canon_StroboETTL2Metering = 0xD1C8,
	ptp_property_canon_ContinousAFMode = 0xD1C9,
	ptp_property_canon_MovieParam2 = 0xD1CA,
	ptp_property_canon_StroboSettingExpComposition = 0xD1CB,
	ptp_property_canon_MovieParam3 = 0xD1CC,
	ptp_property_canon_LVMedicalRotate = 0xD1CF,
	ptp_property_canon_Artist = 0xD1d0,
	ptp_property_canon_Copyright = 0xD1d1,
	ptp_property_canon_BracketValue = 0xD1d2,
	ptp_property_canon_FocusInfoEx = 0xD1d3,
	ptp_property_canon_DepthOfField = 0xD1d4,
	ptp_property_canon_Brightness = 0xD1d5,
	ptp_property_canon_LensAdjustParams = 0xD1d6,
	ptp_property_canon_EFComp = 0xD1d7,
	ptp_property_canon_LensName = 0xD1d8,
	ptp_property_canon_AEB = 0xD1d9,
	ptp_property_canon_StroboSetting = 0xD1da,
	ptp_property_canon_StroboWirelessSetting = 0xD1db,
	ptp_property_canon_StroboFiring = 0xD1dc,
	ptp_property_canon_LensID = 0xD1dd,
	ptp_property_canon_LCDBrightness = 0xD1de,
	ptp_property_canon_CADarkBright = 0xD1df,
	ptp_property_canon_ExExposureLevelIncrements = 0x8101,
	ptp_property_canon_ExISOExpansion = 0x8103,
	ptp_property_canon_ExFlasSyncSpeedInAvMode = 0x810F,
	ptp_property_canon_ExLongExposureNoiseReduction = 0x8201,
	ptp_property_canon_ExHighISONoiseReduction = 0x8202,
	ptp_property_canon_ExHighlightTonePriority = 0x8203,
	ptp_property_canon_ExAutoLightingOptimizer = 0x8204,
	ptp_property_canon_ExAFAssistBeamFiring = 0x850E,
	ptp_property_canon_ExAFDuringLiveView = 0x8511,
	ptp_property_canon_ExMirrorLockup = 0x860f,
	ptp_property_canon_ExShutterAELockButton = 0x8701,
	ptp_property_canon_ExSetButtonWhenShooting = 0x8704,
	ptp_property_canon_ExLCDDisplayWhenPowerOn = 0x8811,
	ptp_property_canon_ExAddOriginalDecisionData = 0x880F
} ptp_property_canon_code;

typedef struct {
	int mode;
	int shutter;
	int steps;
	uint32_t ex_func_group[16][1024];
	uint64_t image_format;
} canon_private_data;

extern char *ptp_operation_canon_code_label(uint16_t code);
extern char *ptp_response_canon_code_label(uint16_t code);
extern char *ptp_event_canon_code_label(uint16_t code);
extern char *ptp_property_canon_code_name(uint16_t code);
extern char *ptp_property_canon_code_label(uint16_t code);
extern char *ptp_property_canon_value_code_label(indigo_device *device, uint16_t property, uint64_t code);

extern bool ptp_canon_initialise(indigo_device *device);
extern bool ptp_canon_handle_event(indigo_device *device, ptp_event_code code, uint32_t *params);
extern bool ptp_canon_set_property(indigo_device *device, ptp_property *property);
extern bool ptp_canon_exposure(indigo_device *device);
extern bool ptp_canon_liveview(indigo_device *device);
extern bool ptp_canon_lock(indigo_device *device);
extern bool ptp_canon_af(indigo_device *device);
extern bool ptp_canon_zoom(indigo_device *device);
extern bool ptp_canon_focus(indigo_device *device, int steps);
extern bool ptp_canon_set_host_time(indigo_device *device);
extern bool ptp_canon_check_dual_compression(indigo_device *device);

#endif /* indigo_ptp_canon_h */
