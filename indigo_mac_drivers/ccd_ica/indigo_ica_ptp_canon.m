//
//  indigo_ica_canon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import "indigo_ica_ptp_canon.h"

@implementation PTPCanonRequest
+(NSString *)operationCodeName:(PTPRequestCode)operationCode {
  switch (operationCode) {
    case PTPRequestCodeCanonGetStorageIDs: return @"PTPRequestCodeCanonGetStorageIDs";
    case PTPRequestCodeCanonGetStorageInfo: return @"PTPRequestCodeCanonGetStorageInfo";
    case PTPRequestCodeCanonGetObjectInfo: return @"PTPRequestCodeCanonGetObjectInfo";
    case PTPRequestCodeCanonGetObject: return @"PTPRequestCodeCanonGetObject";
    case PTPRequestCodeCanonDeleteObject: return @"PTPRequestCodeCanonDeleteObject";
    case PTPRequestCodeCanonFormatStore: return @"PTPRequestCodeCanonFormatStore";
    case PTPRequestCodeCanonGetPartialObject: return @"PTPRequestCodeCanonGetPartialObject";
    case PTPRequestCodeCanonGetDeviceInfoEx: return @"PTPRequestCodeCanonGetDeviceInfoEx";
    case PTPRequestCodeCanonGetObjectInfoEx: return @"PTPRequestCodeCanonGetObjectInfoEx";
    case PTPRequestCodeCanonGetThumbEx: return @"PTPRequestCodeCanonGetThumbEx";
    case PTPRequestCodeCanonSendPartialObject: return @"PTPRequestCodeCanonSendPartialObject";
    case PTPRequestCodeCanonSetObjectAttributes: return @"PTPRequestCodeCanonSetObjectAttributes";
    case PTPRequestCodeCanonGetObjectTime: return @"PTPRequestCodeCanonGetObjectTime";
    case PTPRequestCodeCanonSetObjectTime: return @"PTPRequestCodeCanonSetObjectTime";
    case PTPRequestCodeCanonRemoteRelease: return @"PTPRequestCodeCanonRemoteRelease";
    case PTPRequestCodeCanonSetDevicePropValueEx: return @"PTPRequestCodeCanonSetDevicePropValueEx";
    case PTPRequestCodeCanonGetRemoteMode: return @"PTPRequestCodeCanonGetRemoteMode";
    case PTPRequestCodeCanonSetRemoteMode: return @"PTPRequestCodeCanonSetRemoteMode";
    case PTPRequestCodeCanonSetEventMode: return @"PTPRequestCodeCanonSetEventMode";
    case PTPRequestCodeCanonGetEvent: return @"PTPRequestCodeCanonGetEvent";
    case PTPRequestCodeCanonTransferComplete: return @"PTPRequestCodeCanonTransferComplete";
    case PTPRequestCodeCanonCancelTransfer: return @"PTPRequestCodeCanonCancelTransfer";
    case PTPRequestCodeCanonResetTransfer: return @"PTPRequestCodeCanonResetTransfer";
    case PTPRequestCodeCanonPCHDDCapacity: return @"PTPRequestCodeCanonPCHDDCapacity";
    case PTPRequestCodeCanonSetUILock: return @"PTPRequestCodeCanonSetUILock";
    case PTPRequestCodeCanonResetUILock: return @"PTPRequestCodeCanonResetUILock";
    case PTPRequestCodeCanonKeepDeviceOn: return @"PTPRequestCodeCanonKeepDeviceOn";
    case PTPRequestCodeCanonSetNullPacketMode: return @"PTPRequestCodeCanonSetNullPacketMode";
    case PTPRequestCodeCanonUpdateFirmware: return @"PTPRequestCodeCanonUpdateFirmware";
    case PTPRequestCodeCanonTransferCompleteDT: return @"PTPRequestCodeCanonTransferCompleteDT";
    case PTPRequestCodeCanonCancelTransferDT: return @"PTPRequestCodeCanonCancelTransferDT";
    case PTPRequestCodeCanonSetWftProfile: return @"PTPRequestCodeCanonSetWftProfile";
    case PTPRequestCodeCanonGetWftProfile: return @"PTPRequestCodeCanonGetWftProfile";
    case PTPRequestCodeCanonSetProfileToWft: return @"PTPRequestCodeCanonSetProfileToWft";
    case PTPRequestCodeCanonBulbStart: return @"PTPRequestCodeCanonBulbStart";
    case PTPRequestCodeCanonBulbEnd: return @"PTPRequestCodeCanonBulbEnd";
    case PTPRequestCodeCanonRequestDevicePropValue: return @"PTPRequestCodeCanonRequestDevicePropValue";
    case PTPRequestCodeCanonRemoteReleaseOn: return @"PTPRequestCodeCanonRemoteReleaseOn";
    case PTPRequestCodeCanonRemoteReleaseOff: return @"PTPRequestCodeCanonRemoteReleaseOff";
    case PTPRequestCodeCanonRegistBackgroundImage: return @"PTPRequestCodeCanonRegistBackgroundImage";
    case PTPRequestCodeCanonChangePhotoStudioMode: return @"PTPRequestCodeCanonChangePhotoStudioMode";
    case PTPRequestCodeCanonGetPartialObjectEx: return @"PTPRequestCodeCanonGetPartialObjectEx";
    case PTPRequestCodeCanonResetMirrorLockupState: return @"PTPRequestCodeCanonResetMirrorLockupState";
    case PTPRequestCodeCanonPopupBuiltinFlash: return @"PTPRequestCodeCanonPopupBuiltinFlash";
    case PTPRequestCodeCanonEndGetPartialObjectEx: return @"PTPRequestCodeCanonEndGetPartialObjectEx";
    case PTPRequestCodeCanonMovieSelectSWOn: return @"PTPRequestCodeCanonMovieSelectSWOn";
    case PTPRequestCodeCanonMovieSelectSWOff: return @"PTPRequestCodeCanonMovieSelectSWOff";
    case PTPRequestCodeCanonGetCTGInfo: return @"PTPRequestCodeCanonGetCTGInfo";
    case PTPRequestCodeCanonGetLensAdjust: return @"PTPRequestCodeCanonGetLensAdjust";
    case PTPRequestCodeCanonSetLensAdjust: return @"PTPRequestCodeCanonSetLensAdjust";
    case PTPRequestCodeCanonGetMusicInfo: return @"PTPRequestCodeCanonGetMusicInfo";
    case PTPRequestCodeCanonCreateHandle: return @"PTPRequestCodeCanonCreateHandle";
    case PTPRequestCodeCanonSendPartialObjectEx: return @"PTPRequestCodeCanonSendPartialObjectEx";
    case PTPRequestCodeCanonEndSendPartialObjectEx: return @"PTPRequestCodeCanonEndSendPartialObjectEx";
    case PTPRequestCodeCanonSetCTGInfo: return @"PTPRequestCodeCanonSetCTGInfo";
    case PTPRequestCodeCanonSetRequestOLCInfoGroup: return @"PTPRequestCodeCanonSetRequestOLCInfoGroup";
    case PTPRequestCodeCanonSetRequestRollingPitchingLevel: return @"PTPRequestCodeCanonSetRequestRollingPitchingLevel";
    case PTPRequestCodeCanonGetCameraSupport: return @"PTPRequestCodeCanonGetCameraSupport";
    case PTPRequestCodeCanonSetRating: return @"PTPRequestCodeCanonSetRating";
    case PTPRequestCodeCanonRequestInnerDevelopStart: return @"PTPRequestCodeCanonRequestInnerDevelopStart";
    case PTPRequestCodeCanonRequestInnerDevelopParamChange: return @"PTPRequestCodeCanonRequestInnerDevelopParamChange";
    case PTPRequestCodeCanonRequestInnerDevelopEnd: return @"PTPRequestCodeCanonRequestInnerDevelopEnd";
    case PTPRequestCodeCanonGpsLoggingDataMode: return @"PTPRequestCodeCanonGpsLoggingDataMode";
    case PTPRequestCodeCanonGetGpsLogCurrentHandle: return @"PTPRequestCodeCanonGetGpsLogCurrentHandle";
    case PTPRequestCodeCanonInitiateViewfinder: return @"PTPRequestCodeCanonInitiateViewfinder";
    case PTPRequestCodeCanonTerminateViewfinder: return @"PTPRequestCodeCanonTerminateViewfinder";
    case PTPRequestCodeCanonGetViewFinderData: return @"PTPRequestCodeCanonGetViewFinderData";
    case PTPRequestCodeCanonDoAf: return @"PTPRequestCodeCanonDoAf";
    case PTPRequestCodeCanonDriveLens: return @"PTPRequestCodeCanonDriveLens";
    case PTPRequestCodeCanonDepthOfFieldPreview: return @"PTPRequestCodeCanonDepthOfFieldPreview";
    case PTPRequestCodeCanonClickWB: return @"PTPRequestCodeCanonClickWB";
    case PTPRequestCodeCanonZoom: return @"PTPRequestCodeCanonZoom";
    case PTPRequestCodeCanonZoomPosition: return @"PTPRequestCodeCanonZoomPosition";
    case PTPRequestCodeCanonSetLiveAfFrame: return @"PTPRequestCodeCanonSetLiveAfFrame";
    case PTPRequestCodeCanonTouchAfPosition: return @"PTPRequestCodeCanonTouchAfPosition";
    case PTPRequestCodeCanonSetLvPcFlavoreditMode: return @"PTPRequestCodeCanonSetLvPcFlavoreditMode";
    case PTPRequestCodeCanonSetLvPcFlavoreditParam: return @"PTPRequestCodeCanonSetLvPcFlavoreditParam";
    case PTPRequestCodeCanonAfCancel: return @"PTPRequestCodeCanonAfCancel";
    case PTPRequestCodeCanonSetDefaultCameraSetting: return @"PTPRequestCodeCanonSetDefaultCameraSetting";
    case PTPRequestCodeCanonGetAEData: return @"PTPRequestCodeCanonGetAEData";
    case PTPRequestCodeCanonNotifyNetworkError: return @"PTPRequestCodeCanonNotifyNetworkError";
    case PTPRequestCodeCanonAdapterTransferProgress: return @"PTPRequestCodeCanonAdapterTransferProgress";
    case PTPRequestCodeCanonTransferComplete2: return @"PTPRequestCodeCanonTransferComplete2";
    case PTPRequestCodeCanonCancelTransfer2: return @"PTPRequestCodeCanonCancelTransfer2";
    case PTPRequestCodeCanonFAPIMessageTX: return @"PTPRequestCodeCanonFAPIMessageTX";
    case PTPRequestCodeCanonFAPIMessageRX: return @"PTPRequestCodeCanonFAPIMessageRX";
  }
  return [PTPRequest operationCodeName:operationCode];
}

-(NSString *)operationCodeName {
  return [PTPCanonRequest operationCodeName:self.operationCode];
}

@end

@implementation PTPCanonResponse : PTPResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode {
  switch (responseCode) {
    case PTPResponseCodeUnknownCommand: return @"PTPResponseCodeUnknownCommand";
    case PTPResponseCodeOperationRefused: return @"PTPResponseCodeOperationRefused";
    case PTPResponseCodeLensCover: return @"PTPResponseCodeLensCover";
    case PTPResponseCodeBatteryLow: return @"PTPResponseCodeBatteryLow";
    case PTPResponseCodeNotReady: return @"PTPResponseCodeNotReady";
  }
  return [PTPResponse responseCodeName:responseCode];
}

-(NSString *)responseCodeName {
  return [PTPCanonResponse responseCodeName:self.responseCode];
}

@end

@implementation PTPCanonEvent : PTPEvent

+(NSString *)eventCodeName:(PTPEventCode)eventCode {
  switch (eventCode) {
    case PTPEventCodeCanonRequestGetEvent: return @"PTPEventCodeCanonRequestGetEvent";
    case PTPEventCodeCanonObjectAddedEx: return @"PTPEventCodeCanonObjectAddedEx";
    case PTPEventCodeCanonObjectRemoved: return @"PTPEventCodeCanonObjectRemoved";
    case PTPEventCodeCanonRequestGetObjectInfoEx: return @"PTPEventCodeCanonRequestGetObjectInfoEx";
    case PTPEventCodeCanonStorageStatusChanged: return @"PTPEventCodeCanonStorageStatusChanged";
    case PTPEventCodeCanonStorageInfoChanged: return @"PTPEventCodeCanonStorageInfoChanged";
    case PTPEventCodeCanonRequestObjectTransfer: return @"PTPEventCodeCanonRequestObjectTransfer";
    case PTPEventCodeCanonObjectInfoChangedEx: return @"PTPEventCodeCanonObjectInfoChangedEx";
    case PTPEventCodeCanonObjectContentChanged: return @"PTPEventCodeCanonObjectContentChanged";
    case PTPEventCodeCanonPropValueChanged: return @"PTPEventCodeCanonPropValueChanged";
    case PTPEventCodeCanonAvailListChanged: return @"PTPEventCodeCanonAvailListChanged";
    case PTPEventCodeCanonCameraStatusChanged: return @"PTPEventCodeCanonCameraStatusChanged";
    case PTPEventCodeCanonWillSoonShutdown: return @"PTPEventCodeCanonWillSoonShutdown";
    case PTPEventCodeCanonShutdownTimerUpdated: return @"PTPEventCodeCanonShutdownTimerUpdated";
    case PTPEventCodeCanonRequestCancelTransfer: return @"PTPEventCodeCanonRequestCancelTransfer";
    case PTPEventCodeCanonRequestObjectTransferDT: return @"PTPEventCodeCanonRequestObjectTransferDT";
    case PTPEventCodeCanonRequestCancelTransferDT: return @"PTPEventCodeCanonRequestCancelTransferDT";
    case PTPEventCodeCanonStoreAdded: return @"PTPEventCodeCanonStoreAdded";
    case PTPEventCodeCanonStoreRemoved: return @"PTPEventCodeCanonStoreRemoved";
    case PTPEventCodeCanonBulbExposureTime: return @"PTPEventCodeCanonBulbExposureTime";
    case PTPEventCodeCanonRecordingTime: return @"PTPEventCodeCanonRecordingTime";
    case PTPEventCodeCanonRequestObjectTransferTS: return @"PTPEventCodeCanonRequestObjectTransferTS";
    case PTPEventCodeCanonAfResult: return @"PTPEventCodeCanonAfResult";
    case PTPEventCodeCanonCTGInfoCheckComplete: return @"PTPEventCodeCanonCTGInfoCheckComplete";
    case PTPEventCodeCanonOLCInfoChanged: return @"PTPEventCodeCanonOLCInfoChanged";
    case PTPEventCodeCanonRequestObjectTransferFTP: return @"PTPEventCodeCanonRequestObjectTransferFTP";
  }
  return [PTPEvent eventCodeName:eventCode];
}

-(NSString *)eventCodeName {
  return [PTPCanonEvent eventCodeName:self.eventCode];
}

@end

@implementation PTPCanonProperty

+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode {
  switch (propertyCode) {
    case PTPPropertyCodeCanonAperture: return @"PTPPropertyCodeCanonAperture";
    case PTPPropertyCodeCanonShutterSpeed: return @"PTPPropertyCodeCanonShutterSpeed";
    case PTPPropertyCodeCanonISOSpeed: return @"PTPPropertyCodeCanonISOSpeed";
    case PTPPropertyCodeCanonExpCompensation: return @"PTPPropertyCodeCanonExpCompensation";
    case PTPPropertyCodeCanonAutoExposureMode: return @"PTPPropertyCodeCanonAutoExposureMode";
    case PTPPropertyCodeCanonDriveMode: return @"PTPPropertyCodeCanonDriveMode";
    case PTPPropertyCodeCanonMeteringMode: return @"PTPPropertyCodeCanonMeteringMode";
    case PTPPropertyCodeCanonFocusMode: return @"PTPPropertyCodeCanonFocusMode";
    case PTPPropertyCodeCanonWhiteBalance: return @"PTPPropertyCodeCanonWhiteBalance";
    case PTPPropertyCodeCanonColorTemperature: return @"PTPPropertyCodeCanonColorTemperature";
    case PTPPropertyCodeCanonWhiteBalanceAdjustA: return @"PTPPropertyCodeCanonWhiteBalanceAdjustA";
    case PTPPropertyCodeCanonWhiteBalanceAdjustB: return @"PTPPropertyCodeCanonWhiteBalanceAdjustB";
    case PTPPropertyCodeCanonWhiteBalanceXA: return @"PTPPropertyCodeCanonWhiteBalanceXA";
    case PTPPropertyCodeCanonWhiteBalanceXB: return @"PTPPropertyCodeCanonWhiteBalanceXB";
    case PTPPropertyCodeCanonColorSpace: return @"PTPPropertyCodeCanonColorSpace";
    case PTPPropertyCodeCanonPictureStyle: return @"PTPPropertyCodeCanonPictureStyle";
    case PTPPropertyCodeCanonBatteryPower: return @"PTPPropertyCodeCanonBatteryPower";
    case PTPPropertyCodeCanonBatterySelect: return @"PTPPropertyCodeCanonBatterySelect";
    case PTPPropertyCodeCanonCameraTime: return @"PTPPropertyCodeCanonCameraTime";
    case PTPPropertyCodeCanonAutoPowerOff: return @"PTPPropertyCodeCanonAutoPowerOff";
    case PTPPropertyCodeCanonOwner: return @"PTPPropertyCodeCanonOwner";
    case PTPPropertyCodeCanonModelID: return @"PTPPropertyCodeCanonModelID";
    case PTPPropertyCodeCanonPTPExtensionVersion: return @"PTPPropertyCodeCanonPTPExtensionVersion";
    case PTPPropertyCodeCanonDPOFVersion: return @"PTPPropertyCodeCanonDPOFVersion";
    case PTPPropertyCodeCanonAvailableShots: return @"PTPPropertyCodeCanonAvailableShots";
    case PTPPropertyCodeCanonCaptureDestination: return @"PTPPropertyCodeCanonCaptureDestination";
    case PTPPropertyCodeCanonBracketMode: return @"PTPPropertyCodeCanonBracketMode";
    case PTPPropertyCodeCanonCurrentStorage: return @"PTPPropertyCodeCanonCurrentStorage";
    case PTPPropertyCodeCanonCurrentFolder: return @"PTPPropertyCodeCanonCurrentFolder";
    case PTPPropertyCodeCanonImageFormat: return @"PTPPropertyCodeCanonImageFormat";
    case PTPPropertyCodeCanonImageFormatCF: return @"PTPPropertyCodeCanonImageFormatCF";
    case PTPPropertyCodeCanonImageFormatSD: return @"PTPPropertyCodeCanonImageFormatSD";
    case PTPPropertyCodeCanonImageFormatExtHD: return @"PTPPropertyCodeCanonImageFormatExtHD";
    case PTPPropertyCodeCanonCompressionS: return @"PTPPropertyCodeCanonCompressionS";
    case PTPPropertyCodeCanonCompressionM1: return @"PTPPropertyCodeCanonCompressionM1";
    case PTPPropertyCodeCanonCompressionM2: return @"PTPPropertyCodeCanonCompressionM2";
    case PTPPropertyCodeCanonCompressionL: return @"PTPPropertyCodeCanonCompressionL";
    case PTPPropertyCodeCanonAEModeDial: return @"PTPPropertyCodeCanonAEModeDial";
    case PTPPropertyCodeCanonAEModeCustom: return @"PTPPropertyCodeCanonAEModeCustom";
    case PTPPropertyCodeCanonMirrorUpSetting: return @"PTPPropertyCodeCanonMirrorUpSetting";
    case PTPPropertyCodeCanonHighlightTonePriority: return @"PTPPropertyCodeCanonHighlightTonePriority";
    case PTPPropertyCodeCanonAFSelectFocusArea: return @"PTPPropertyCodeCanonAFSelectFocusArea";
    case PTPPropertyCodeCanonHDRSetting: return @"PTPPropertyCodeCanonHDRSetting";
    case PTPPropertyCodeCanonPCWhiteBalance1: return @"PTPPropertyCodeCanonPCWhiteBalance1";
    case PTPPropertyCodeCanonPCWhiteBalance2: return @"PTPPropertyCodeCanonPCWhiteBalance2";
    case PTPPropertyCodeCanonPCWhiteBalance3: return @"PTPPropertyCodeCanonPCWhiteBalance3";
    case PTPPropertyCodeCanonPCWhiteBalance4: return @"PTPPropertyCodeCanonPCWhiteBalance4";
    case PTPPropertyCodeCanonPCWhiteBalance5: return @"PTPPropertyCodeCanonPCWhiteBalance5";
    case PTPPropertyCodeCanonMWhiteBalance: return @"PTPPropertyCodeCanonMWhiteBalance";
    case PTPPropertyCodeCanonMWhiteBalanceEx: return @"PTPPropertyCodeCanonMWhiteBalanceEx";
    case PTPPropertyCodeCanonUnknownPropD14D: return @"PTPPropertyCodeCanonUnknownPropD14D";
    case PTPPropertyCodeCanonPictureStyleStandard: return @"PTPPropertyCodeCanonPictureStyleStandard";
    case PTPPropertyCodeCanonPictureStylePortrait: return @"PTPPropertyCodeCanonPictureStylePortrait";
    case PTPPropertyCodeCanonPictureStyleLandscape: return @"PTPPropertyCodeCanonPictureStyleLandscape";
    case PTPPropertyCodeCanonPictureStyleNeutral: return @"PTPPropertyCodeCanonPictureStyleNeutral";
    case PTPPropertyCodeCanonPictureStyleFaithful: return @"PTPPropertyCodeCanonPictureStyleFaithful";
    case PTPPropertyCodeCanonPictureStyleBlackWhite: return @"PTPPropertyCodeCanonPictureStyleBlackWhite";
    case PTPPropertyCodeCanonPictureStyleAuto: return @"PTPPropertyCodeCanonPictureStyleAuto";
    case PTPPropertyCodeCanonPictureStyleUserSet1: return @"PTPPropertyCodeCanonPictureStyleUserSet1";
    case PTPPropertyCodeCanonPictureStyleUserSet2: return @"PTPPropertyCodeCanonPictureStyleUserSet2";
    case PTPPropertyCodeCanonPictureStyleUserSet3: return @"PTPPropertyCodeCanonPictureStyleUserSet3";
    case PTPPropertyCodeCanonPictureStyleParam1: return @"PTPPropertyCodeCanonPictureStyleParam1";
    case PTPPropertyCodeCanonPictureStyleParam2: return @"PTPPropertyCodeCanonPictureStyleParam2";
    case PTPPropertyCodeCanonPictureStyleParam3: return @"PTPPropertyCodeCanonPictureStyleParam3";
    case PTPPropertyCodeCanonHighISOSettingNoiseReduction: return @"PTPPropertyCodeCanonHighISOSettingNoiseReduction";
    case PTPPropertyCodeCanonMovieServoAF: return @"PTPPropertyCodeCanonMovieServoAF";
    case PTPPropertyCodeCanonContinuousAFValid: return @"PTPPropertyCodeCanonContinuousAFValid";
    case PTPPropertyCodeCanonAttenuator: return @"PTPPropertyCodeCanonAttenuator";
    case PTPPropertyCodeCanonUTCTime: return @"PTPPropertyCodeCanonUTCTime";
    case PTPPropertyCodeCanonTimezone: return @"PTPPropertyCodeCanonTimezone";
    case PTPPropertyCodeCanonSummertime: return @"PTPPropertyCodeCanonSummertime";
    case PTPPropertyCodeCanonFlavorLUTParams: return @"PTPPropertyCodeCanonFlavorLUTParams";
    case PTPPropertyCodeCanonCustomFunc1: return @"PTPPropertyCodeCanonCustomFunc1";
    case PTPPropertyCodeCanonCustomFunc2: return @"PTPPropertyCodeCanonCustomFunc2";
    case PTPPropertyCodeCanonCustomFunc3: return @"PTPPropertyCodeCanonCustomFunc3";
    case PTPPropertyCodeCanonCustomFunc4: return @"PTPPropertyCodeCanonCustomFunc4";
    case PTPPropertyCodeCanonCustomFunc5: return @"PTPPropertyCodeCanonCustomFunc5";
    case PTPPropertyCodeCanonCustomFunc6: return @"PTPPropertyCodeCanonCustomFunc6";
    case PTPPropertyCodeCanonCustomFunc7: return @"PTPPropertyCodeCanonCustomFunc7";
    case PTPPropertyCodeCanonCustomFunc8: return @"PTPPropertyCodeCanonCustomFunc8";
    case PTPPropertyCodeCanonCustomFunc9: return @"PTPPropertyCodeCanonCustomFunc9";
    case PTPPropertyCodeCanonCustomFunc10: return @"PTPPropertyCodeCanonCustomFunc10";
    case PTPPropertyCodeCanonCustomFunc11: return @"PTPPropertyCodeCanonCustomFunc11";
    case PTPPropertyCodeCanonCustomFunc12: return @"PTPPropertyCodeCanonCustomFunc12";
    case PTPPropertyCodeCanonCustomFunc13: return @"PTPPropertyCodeCanonCustomFunc13";
    case PTPPropertyCodeCanonCustomFunc14: return @"PTPPropertyCodeCanonCustomFunc14";
    case PTPPropertyCodeCanonCustomFunc15: return @"PTPPropertyCodeCanonCustomFunc15";
    case PTPPropertyCodeCanonCustomFunc16: return @"PTPPropertyCodeCanonCustomFunc16";
    case PTPPropertyCodeCanonCustomFunc17: return @"PTPPropertyCodeCanonCustomFunc17";
    case PTPPropertyCodeCanonCustomFunc18: return @"PTPPropertyCodeCanonCustomFunc18";
    case PTPPropertyCodeCanonCustomFunc19: return @"PTPPropertyCodeCanonCustomFunc19";
    case PTPPropertyCodeCanonInnerDevelop: return @"PTPPropertyCodeCanonInnerDevelop";
    case PTPPropertyCodeCanonMultiAspect: return @"PTPPropertyCodeCanonMultiAspect";
    case PTPPropertyCodeCanonMovieSoundRecord: return @"PTPPropertyCodeCanonMovieSoundRecord";
    case PTPPropertyCodeCanonMovieRecordVolume: return @"PTPPropertyCodeCanonMovieRecordVolume";
    case PTPPropertyCodeCanonWindCut: return @"PTPPropertyCodeCanonWindCut";
    case PTPPropertyCodeCanonExtenderType: return @"PTPPropertyCodeCanonExtenderType";
    case PTPPropertyCodeCanonOLCInfoVersion: return @"PTPPropertyCodeCanonOLCInfoVersion";
    case PTPPropertyCodeCanonUnknownPropD19A: return @"PTPPropertyCodeCanonUnknownPropD19A";
    case PTPPropertyCodeCanonUnknownPropD19C: return @"PTPPropertyCodeCanonUnknownPropD19C";
    case PTPPropertyCodeCanonUnknownPropD19D: return @"PTPPropertyCodeCanonUnknownPropD19D";
    case PTPPropertyCodeCanonCustomFuncEx: return @"PTPPropertyCodeCanonCustomFuncEx";
    case PTPPropertyCodeCanonMyMenu: return @"PTPPropertyCodeCanonMyMenu";
    case PTPPropertyCodeCanonMyMenuList: return @"PTPPropertyCodeCanonMyMenuList";
    case PTPPropertyCodeCanonWftStatus: return @"PTPPropertyCodeCanonWftStatus";
    case PTPPropertyCodeCanonWftInputTransmission: return @"PTPPropertyCodeCanonWftInputTransmission";
    case PTPPropertyCodeCanonHDDirectoryStructure: return @"PTPPropertyCodeCanonHDDirectoryStructure";
    case PTPPropertyCodeCanonBatteryInfo: return @"PTPPropertyCodeCanonBatteryInfo";
    case PTPPropertyCodeCanonAdapterInfo: return @"PTPPropertyCodeCanonAdapterInfo";
    case PTPPropertyCodeCanonLensStatus: return @"PTPPropertyCodeCanonLensStatus";
    case PTPPropertyCodeCanonQuickReviewTime: return @"PTPPropertyCodeCanonQuickReviewTime";
    case PTPPropertyCodeCanonCardExtension: return @"PTPPropertyCodeCanonCardExtension";
    case PTPPropertyCodeCanonTempStatus: return @"PTPPropertyCodeCanonTempStatus";
    case PTPPropertyCodeCanonShutterCounter: return @"PTPPropertyCodeCanonShutterCounter";
    case PTPPropertyCodeCanonSpecialOption: return @"PTPPropertyCodeCanonSpecialOption";
    case PTPPropertyCodeCanonPhotoStudioMode: return @"PTPPropertyCodeCanonPhotoStudioMode";
    case PTPPropertyCodeCanonSerialNumber: return @"PTPPropertyCodeCanonSerialNumber";
    case PTPPropertyCodeCanonEVFOutputDevice: return @"PTPPropertyCodeCanonEVFOutputDevice";
    case PTPPropertyCodeCanonEVFMode: return @"PTPPropertyCodeCanonEVFMode";
    case PTPPropertyCodeCanonDepthOfFieldPreview: return @"PTPPropertyCodeCanonDepthOfFieldPreview";
    case PTPPropertyCodeCanonEVFSharpness: return @"PTPPropertyCodeCanonEVFSharpness";
    case PTPPropertyCodeCanonEVFWBMode: return @"PTPPropertyCodeCanonEVFWBMode";
    case PTPPropertyCodeCanonEVFClickWBCoeffs: return @"PTPPropertyCodeCanonEVFClickWBCoeffs";
    case PTPPropertyCodeCanonEVFColorTemp: return @"PTPPropertyCodeCanonEVFColorTemp";
    case PTPPropertyCodeCanonExposureSimMode: return @"PTPPropertyCodeCanonExposureSimMode";
    case PTPPropertyCodeCanonEVFRecordStatus: return @"PTPPropertyCodeCanonEVFRecordStatus";
    case PTPPropertyCodeCanonLvAfSystem: return @"PTPPropertyCodeCanonLvAfSystem";
    case PTPPropertyCodeCanonMovSize: return @"PTPPropertyCodeCanonMovSize";
    case PTPPropertyCodeCanonLvViewTypeSelect: return @"PTPPropertyCodeCanonLvViewTypeSelect";
    case PTPPropertyCodeCanonMirrorDownStatus: return @"PTPPropertyCodeCanonMirrorDownStatus";
    case PTPPropertyCodeCanonMovieParam: return @"PTPPropertyCodeCanonMovieParam";
    case PTPPropertyCodeCanonMirrorLockupState: return @"PTPPropertyCodeCanonMirrorLockupState";
    case PTPPropertyCodeCanonFlashChargingState: return @"PTPPropertyCodeCanonFlashChargingState";
    case PTPPropertyCodeCanonAloMode: return @"PTPPropertyCodeCanonAloMode";
    case PTPPropertyCodeCanonFixedMovie: return @"PTPPropertyCodeCanonFixedMovie";
    case PTPPropertyCodeCanonOneShotRawOn: return @"PTPPropertyCodeCanonOneShotRawOn";
    case PTPPropertyCodeCanonErrorForDisplay: return @"PTPPropertyCodeCanonErrorForDisplay";
    case PTPPropertyCodeCanonAEModeMovie: return @"PTPPropertyCodeCanonAEModeMovie";
    case PTPPropertyCodeCanonBuiltinStroboMode: return @"PTPPropertyCodeCanonBuiltinStroboMode";
    case PTPPropertyCodeCanonStroboDispState: return @"PTPPropertyCodeCanonStroboDispState";
    case PTPPropertyCodeCanonStroboETTL2Metering: return @"PTPPropertyCodeCanonStroboETTL2Metering";
    case PTPPropertyCodeCanonContinousAFMode: return @"PTPPropertyCodeCanonContinousAFMode";
    case PTPPropertyCodeCanonMovieParam2: return @"PTPPropertyCodeCanonMovieParam2";
    case PTPPropertyCodeCanonStroboSettingExpComposition: return @"PTPPropertyCodeCanonStroboSettingExpComposition";
    case PTPPropertyCodeCanonMovieParam3: return @"PTPPropertyCodeCanonMovieParam3";
    case PTPPropertyCodeCanonLVMedicalRotate: return @"PTPPropertyCodeCanonLVMedicalRotate";
    case PTPPropertyCodeCanonArtist: return @"PTPPropertyCodeCanonArtist";
    case PTPPropertyCodeCanonCopyright: return @"PTPPropertyCodeCanonCopyright";
    case PTPPropertyCodeCanonBracketValue: return @"PTPPropertyCodeCanonBracketValue";
    case PTPPropertyCodeCanonFocusInfoEx: return @"PTPPropertyCodeCanonFocusInfoEx";
    case PTPPropertyCodeCanonDepthOfField: return @"PTPPropertyCodeCanonDepthOfField";
    case PTPPropertyCodeCanonBrightness: return @"PTPPropertyCodeCanonBrightness";
    case PTPPropertyCodeCanonLensAdjustParams: return @"PTPPropertyCodeCanonLensAdjustParams";
    case PTPPropertyCodeCanonEFComp: return @"PTPPropertyCodeCanonEFComp";
    case PTPPropertyCodeCanonLensName: return @"PTPPropertyCodeCanonLensName";
    case PTPPropertyCodeCanonAEB: return @"PTPPropertyCodeCanonAEB";
    case PTPPropertyCodeCanonStroboSetting: return @"PTPPropertyCodeCanonStroboSetting";
    case PTPPropertyCodeCanonStroboWirelessSetting: return @"PTPPropertyCodeCanonStroboWirelessSetting";
    case PTPPropertyCodeCanonStroboFiring: return @"PTPPropertyCodeCanonStroboFiring";
    case PTPPropertyCodeCanonLensID: return @"PTPPropertyCodeCanonLensID";
    case PTPPropertyCodeCanonLCDBrightness: return @"PTPPropertyCodeCanonLCDBrightness";
    case PTPPropertyCodeCanonCADarkBright: return @"PTPPropertyCodeCanonCADarkBright";
  }
  return [PTPProperty propertyCodeName:propertyCode];
}

-(NSString *)propertyCodeName {
  return [PTPCanonProperty propertyCodeName:self.propertyCode];
}

@end

@implementation PTPCanonDeviceInfo : PTPDeviceInfo

-(NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %@ V%.2f\n", self.model, self.version, self.standardVersion / 100.0, self.vendorExtensionDesc, self.vendorExtensionVersion / 100.0];
  if (self.operationsSupported.count > 0) {
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPCanonRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPCanonEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
    [s appendFormat:@"\nProperties:\n"];
    for (NSNumber *code in self.propertiesSupported) {
      PTPProperty *property = self.properties[code];
      if (property)
        [s appendFormat:@"%@\n", property];
      else
        [s appendFormat:@"%@\n", [PTPCanonProperty propertyCodeName:code.intValue]];
    }
  }
  return s;
}

@end

@implementation PTPCanonCamera

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {

  }
  return self;
}

-(Class)requestClass {
  return PTPCanonRequest.class;
}

-(Class)responseClass {
  return PTPCanonResponse.class;
}

-(Class)eventClass {
  return PTPCanonEvent.class;
}

-(Class)propertyClass {
  return PTPCanonProperty.class;
}

-(Class)deviceInfoClass {
  return PTPCanonDeviceInfo.class;
}

@end
