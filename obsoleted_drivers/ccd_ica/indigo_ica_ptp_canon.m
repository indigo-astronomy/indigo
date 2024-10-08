//
//  indigo_ica_canon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include <indigo/indigo_bus.h>

#import "indigo_ica_ptp_canon.h"

static long ptpReadCanonImageFormat(unsigned char** buf) {
  long result;
  unsigned int count = ptpReadUnsignedInt(buf);
  unsigned int size = ptpReadUnsignedInt(buf) & 0xFF;
  unsigned int format = ptpReadUnsignedInt(buf) & 0xFF;
  unsigned int quality = ptpReadUnsignedInt(buf) & 0xFF;
  unsigned int compression = ptpReadUnsignedInt(buf) & 0xFF;
  result = size << 24 | format << 16 | quality << 8 | compression;
  if (count == 2) {
    size = ptpReadUnsignedInt(buf);
    format = ptpReadUnsignedInt(buf);
    quality = ptpReadUnsignedInt(buf);
    compression = ptpReadUnsignedInt(buf);
    result = result << 32 | size << 24 | format << 16 | quality << 8 | compression;
  }
  return result;
}

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
    case PTPResponseCodeCanonUnknownCommand: return @"PTPResponseCodeCanonUnknownCommand";
    case PTPResponseCodeCanonOperationRefused: return @"PTPResponseCodeCanonOperationRefused";
    case PTPResponseCodeCanonLensCover: return @"PTPResponseCodeCanonLensCover";
    case PTPResponseCodeCanonBatteryLow: return @"PTPResponseCodeCanonBatteryLow";
    case PTPResponseCodeCanonNotReady: return @"PTPResponseCodeCanonNotReady";
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
    case PTPEventCodeCanonObjectAddedEx2: return @"PTPEventCodeCanonObjectAddedEx2";
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

    case PTPPropertyCodeCanonExExposureLevelIncrements: return @"PTPPropertyCodeCanonExExposureLevelIncrements";
    case PTPPropertyCodeCanonExISOExpansion: return @"PTPPropertyCodeCanonExISOExpansion";
    case PTPPropertyCodeCanonExFlasgSyncSpeedInAvMode: return @"PTPPropertyCodeCanonExFlasgSyncSpeedInAvMode";
    case PTPPropertyCodeCanonExLongExposureNoiseReduction: return @"PTPPropertyCodeCanonExLongExposureNoiseReduction";
    case PTPPropertyCodeCanonExHighISONoiseReduction: return @"PTPPropertyCodeCanonExHighISONoiseReduction";
    case PTPPropertyCodeCanonExHHighlightTonePriority: return @"PTPPropertyCodeCanonExHHighlightTonePriority";
    case PTPPropertyCodeCanonExAutoLightingOptimizer: return @"PTPPropertyCodeCanonExAutoLightingOptimizer";
    case PTPPropertyCodeCanonExAFAssistBeamFiring: return @"PTPPropertyCodeCanonExAFAssistBeamFiring";
    case PTPPropertyCodeCanonExAFDuringLiveView: return @"PTPPropertyCodeCanonExAFDuringLiveView";
    case PTPPropertyCodeCanonExMirrorLockup: return @"PTPPropertyCodeCanonExMirrorLockup";
    case PTPPropertyCodeCanonExShutterAELockButton: return @"PTPPropertyCodeCanonExShutterAELockButton";
    case PTPPropertyCodeCanonExSetButtonWhenShooting: return @"PTPPropertyCodeCanonExSetButtonWhenShooting";
    case PTPPropertyCodeCanonExLCDDisplayWhenPowerOn: return @"PTPPropertyCodeCanonExLCDDisplayWhenPowerOn";
    case PTPPropertyCodeCanonExAddOriginalDecisionData: return @"PTPPropertyCodeCanonExAddOriginalDecisionData";
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
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPCanonRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPCanonEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
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

static struct info {
  const char *name;
  int width, height;
  float pixelSize;
} info[] = {
  { "CANON EOS REBEL XTI", 3888, 2592, 5.7  },
  { "CANON EOS REBEL XT", 3456, 2304, 6.4  },
  { "CANON EOS REBEL XSI", 4272, 2848, 5.19  },
  { "CANON EOS REBEL XS", 3888, 2592, 5.7  },
  { "CANON EOS REBEL T1I", 4752, 3168, 4.69  },
  { "CANON EOS REBEL T2I", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T3I", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T3", 4272, 2848, 5.19  },
  { "CANON EOS REBEL T4I", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T5I", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T5", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T6I", 6000, 4000, 3.71  },
  { "CANON EOS REBEL T6S", 6000, 4000, 3.71  },
  { "CANON EOS REBEL T6", 5184, 3456, 4.3  },
  { "CANON EOS REBEL T7", 6000, 4000, 3.71  },
  { "CANON EOS REBEL T7I", 6000, 4000, 3.71  },
  { "CANON EOS REBEL T100", 5184, 3456, 4.3  },
  { "CANON EOS REBEL SL1", 5184, 3456, 4.3  },
  { "CANON EOS REBEL SL2", 6000, 4000, 3.71  },
  { "CANON EOS KISS X2", 4272, 2848, 5.19  },
  { "CANON EOS KISS X3", 4752, 3168, 4.69  },
  { "CANON EOS KISS X4", 5184, 3456, 4.3  },
  { "CANON EOS KISS X50", 4272, 2848, 5.19  },
  { "CANON EOS KISS X5", 5184, 3456, 4.3  },
  { "CANON EOS KISS X6I", 5184, 3456, 4.3  },
  { "CANON EOS KISS X7I", 5184, 3456, 4.3  },
  { "CANON EOS KISS X70", 5184, 3456, 4.3  },
  { "CANON EOS KISS X7", 5184, 3456, 4.3  },
  { "CANON EOS KISS X8I", 6000, 4000, 3.71  },
  { "CANON EOS KISS X80", 5184, 3456, 4.3  },
  { "CANON EOS KISS F", 3888, 2592, 5.7  },
  { "CANON EOS 1000D", 3888, 2592, 5.7  },
  { "CANON EOS 1100D", 4272, 2848, 5.19  },
  { "CANON EOS 1200D", 5184, 3456, 4.3  },
  { "CANON EOS 1300D", 5184, 3456, 4.3  },
  { "CANON EOS 2000D", 6000, 4000, 3.71  },
  { "CANON EOS 4000D", 5184, 3456, 4.3  },
  { "CANON EOS 8000D", 6000, 4000, 3.71  },
  { "CANON EOS 100D", 5184, 3456, 4.3  },
  { "CANON EOS 200D", 6000, 4000, 3.71  },
  { "CANON EOS 350D", 3456, 2304, 6.4  },
  { "CANON EOS 400D", 3888, 2592, 5.7  },
  { "CANON EOS 450D", 4272, 2848, 5.19  },
  { "CANON EOS 500D", 4752, 3168, 4.69  },
  { "CANON EOS 550D", 5184, 3456, 4.3  },
  { "CANON EOS 600D", 5184, 3456, 4.3  },
  { "CANON EOS 650D", 5184, 3456, 4.3  },
  { "CANON EOS 700D", 5184, 3456, 4.3  },
  { "CANON EOS 750D", 6000, 4000, 3.71  },
  { "CANON EOS 760D", 6000, 4000, 3.71  },
  { "CANON EOS 800D", 6000, 4000, 3.71  },
  { "CANON EOS 20D", 3520, 2344, 6.4  },
  { "CANON EOS 20DA", 3520, 2344, 6.4  },
  { "CANON EOS 30D", 3520, 2344, 6.4  },
  { "CANON EOS 40D", 3888, 2592, 5.7  },
  { "CANON EOS 50D", 4752, 3168, 4.69  },
  { "CANON EOS 60D", 5184, 3456, 4.3  },
  { "CANON EOS 70D", 5472, 3648, 6.54  },
  { "CANON EOS 80D", 6000, 4000, 3.71  },
  { "CANON EOS 1DS MARK III", 5616, 3744, 6.41  },
  { "CANON EOS 1D MARK III", 3888, 2592, 5.7  },
  { "CANON EOS 1D MARK IV", 4896, 3264, 5.69  },
  { "CANON EOS 1D X MARK II", 5472, 3648, 6.54  },
  { "CANON EOS 1D X", 5472, 3648, 6.54  },
  { "CANON EOS 1D C", 5184, 3456, 4.3  },
  { "CANON EOS 5D MARK II", 5616, 3744, 6.41  },
  { "CANON EOS 5DS", 8688, 5792, 4.14  },
  { "CANON EOS 5D", 4368, 2912, 8.2  },
  { "CANON EOS 6D", 5472, 3648, 6.54  },
  { "CANON EOS 7D MARK II", 5472, 3648, 4.07  },
  { "CANON EOS 7D", 5184, 3456, 4.3  },
  { NULL, 0, 0, 0 }
};

@implementation PTPCanonCamera {
  BOOL startPreview;
  BOOL tempLiveView;
  BOOL inPreview;
  BOOL doImageDownload;
  NSMutableDictionary *addedFileName;
  int currentMode;
  int currentShutterSpeed;
  int focusSteps;
  unsigned int *customFuncGroup[10];
  unsigned int liveViewZoom;
}

-(PTPVendorExtension)extension {
  return PTPVendorExtensionCanon;
}

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {
    const char *name = [super.name.uppercaseString cStringUsingEncoding:NSASCIIStringEncoding];
    for (int i = 0; info[i].name; i++)
      if (!strcmp(name, info[i].name)) {
        self.width = info[i].width;
        self.height = info[i].height;
        self.pixelSize = info[i].pixelSize;
        break;
      }
    addedFileName = [NSMutableDictionary dictionary];
  }
  return self;
}

-(void)didRemoveDevice:(ICDevice *)device {
  inPreview = false;
  [super didRemoveDevice:device];
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

-(void)checkForEvent {
  [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
}

-(void)processEvent:(PTPEvent *)event {
  switch (event.eventCode) {
      // TBD
    default: {
      [super processEvent:event];
      break;
    }
  }
}

-(void)processConnect {
  if ([self operationIsSupported:PTPRequestCodeCanonRemoteRelease])
    [self.delegate cameraCanExposure:self];
  if ([self operationIsSupported:PTPRequestCodeCanonDriveLens])
    [self.delegate cameraCanFocus:self];
  if ([self operationIsSupported:PTPRequestCodeCanonGetViewFinderData])
    [self.delegate cameraCanPreview:self];
  //[self setProperty:PTPPropertyCodeCanonEVFOutputDevice value:@"1"];
  [self setProperty:PTPPropertyCodeCanonExMirrorLockup value:@"0"];
  [super processConnect];
}

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        self.info = [[self.deviceInfoClass alloc] initWithData:data];
        if ([self operationIsSupported:PTPRequestCodeInitiateCapture]) {
          [self.delegate cameraCanExposure:self];
        }
        [self sendPTPRequest:PTPRequestCodeCanonGetDeviceInfoEx];
      }
      break;
    }
    case PTPRequestCodeCanonGetDeviceInfoEx: {
      unsigned char* buffer = (unsigned char*)[data bytes];
      unsigned char* buf = buffer;
      ptpReadInt(&buf); // size
      NSArray *events = ptpReadUnsignedIntArray(&buf);
      NSArray *codes = ptpReadUnsignedIntArray(&buf);
      for (NSNumber *code in events)
        [(NSMutableArray *)self.info.eventsSupported addObject:[NSNumber numberWithUnsignedShort:code.unsignedShortValue]];
      for (NSNumber *code in codes)
        [(NSMutableArray *)self.info.propertiesSupported addObject:[NSNumber numberWithUnsignedShort:code.unsignedShortValue]];
      for (NSNumber *code in self.info.propertiesSupported) {
        unsigned short ui = code.unsignedShortValue;
        if ((ui & 0xD100) != 0xD100)
          [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:ui];
      }
      [self sendPTPRequest:PTPRequestCodeCanonSetRemoteMode param1:1];
      [self sendPTPRequest:PTPRequestCodeCanonSetEventMode param1:1];
      [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
      [self sendPTPRequest:PTPRequestCodeCanonRequestDevicePropValue param1:PTPPropertyCodeCanonOwner];
      [self sendPTPRequest:PTPRequestCodeCanonRequestDevicePropValue param1:PTPPropertyCodeCanonArtist];
      [self sendPTPRequest:PTPRequestCodeCanonRequestDevicePropValue param1:PTPPropertyCodeCanonCopyright];
      [self sendPTPRequest:PTPRequestCodeCanonRequestDevicePropValue param1:PTPPropertyCodeCanonSerialNumber];
      [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
      [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      break;
    }
    case PTPRequestCodeCanonGetEvent: {
      long length = data.length;
      unsigned char* buffer = (unsigned char*)[data bytes];
      unsigned char* buf = buffer;
      unsigned char* record;
      NSMutableArray<PTPProperty *> *properties = [NSMutableArray array];
      while (buf - buffer < length) {
        record = buf;
        int size = ptpReadUnsignedInt(&buf);
        int type = ptpReadUnsignedInt(&buf);
        if (size == 8 && type == 0)
          break;
        switch (type) {
          case PTPEventCodeCanonPropValueChanged: {
            unsigned int code = ptpReadUnsignedInt(&buf);
            PTPProperty *property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
            if (property == nil)
              property = [[self.propertyClass alloc] initWithCode:code];
            switch (code) {
              case PTPPropertyCodeCanonBatteryPower:
              case PTPPropertyCodeCanonBatterySelect:
              case PTPPropertyCodeCanonModelID:
              case PTPPropertyCodeCanonPTPExtensionVersion:
              case PTPPropertyCodeCanonDPOFVersion:
              case PTPPropertyCodeCanonAvailableShots:
              case PTPPropertyCodeCanonCurrentStorage:
              case PTPPropertyCodeCanonCurrentFolder:
              case PTPPropertyCodeCanonMyMenu:
              case PTPPropertyCodeCanonMyMenuList:
              case PTPPropertyCodeCanonHDDirectoryStructure:
              case PTPPropertyCodeCanonBatteryInfo:
              case PTPPropertyCodeCanonAdapterInfo:
              case PTPPropertyCodeCanonAutoExposureMode:
              case PTPPropertyCodeCanonLensStatus:
              case PTPPropertyCodeCanonCardExtension:
              case PTPPropertyCodeCanonTempStatus:
              case PTPPropertyCodeCanonShutterCounter:
              case PTPPropertyCodeCanonSerialNumber:
              case PTPPropertyCodeCanonDepthOfFieldPreview:
              case PTPPropertyCodeCanonEVFRecordStatus:
              case PTPPropertyCodeCanonLvAfSystem:
              case PTPPropertyCodeCanonFocusInfoEx:
              case PTPPropertyCodeCanonDepthOfField:
              case PTPPropertyCodeCanonBrightness:
              case PTPPropertyCodeCanonEFComp:
              case PTPPropertyCodeCanonLensName:
              case PTPPropertyCodeCanonLensID:
              case PTPPropertyCodeCanonCustomFunc1:
              case PTPPropertyCodeCanonCustomFunc2:
              case PTPPropertyCodeCanonCustomFunc3:
              case PTPPropertyCodeCanonCustomFunc4:
              case PTPPropertyCodeCanonCustomFunc5:
              case PTPPropertyCodeCanonCustomFunc6:
              case PTPPropertyCodeCanonCustomFunc7:
              case PTPPropertyCodeCanonCustomFunc8:
              case PTPPropertyCodeCanonCustomFunc9:
              case PTPPropertyCodeCanonCustomFunc10:
              case PTPPropertyCodeCanonCustomFunc11:
              case PTPPropertyCodeCanonCustomFunc12:
              case PTPPropertyCodeCanonCustomFunc13:
              case PTPPropertyCodeCanonCustomFunc14:
              case PTPPropertyCodeCanonCustomFunc15:
              case PTPPropertyCodeCanonCustomFunc16:
              case PTPPropertyCodeCanonCustomFunc17:
              case PTPPropertyCodeCanonCustomFunc18:
              case PTPPropertyCodeCanonCustomFunc19:
              case PTPPropertyCodeCanonCustomFuncEx:
              case PTPPropertyCodeCanonAutoPowerOff:
              case PTPPropertyCodeCanonDriveMode:
                property.readOnly = true;
                break;
              default:
                property.readOnly = false;
                break;
            }
            switch (code) {
              case PTPPropertyCodeCanonPictureStyle:
              case PTPPropertyCodeCanonWhiteBalance:
              case PTPPropertyCodeCanonMeteringMode:
              case PTPPropertyCodeCanonExpCompensation:
                property.type = PTPDataTypeCodeUInt8;
                break;
              case PTPPropertyCodeCanonAperture:
              case PTPPropertyCodeCanonShutterSpeed:
              case PTPPropertyCodeCanonISOSpeed:
              case PTPPropertyCodeCanonFocusMode:
              case PTPPropertyCodeCanonColorSpace:
              case PTPPropertyCodeCanonBatteryPower:
              case PTPPropertyCodeCanonBatterySelect:
              case PTPPropertyCodeCanonPTPExtensionVersion:
              case PTPPropertyCodeCanonDriveMode:
              case PTPPropertyCodeCanonAEB:
              case PTPPropertyCodeCanonBracketMode:
              case PTPPropertyCodeCanonQuickReviewTime:
              case PTPPropertyCodeCanonEVFMode:
              case PTPPropertyCodeCanonEVFOutputDevice:
              case PTPPropertyCodeCanonAutoPowerOff:
              case PTPPropertyCodeCanonEVFRecordStatus:
              case PTPPropertyCodeCanonAutoExposureMode:
              case PTPPropertyCodeCanonMirrorUpSetting:
              case PTPPropertyCodeCanonMirrorLockupState:
                property.type = PTPDataTypeCodeUInt16;
                break;
              case PTPPropertyCodeCanonWhiteBalanceAdjustA:
              case PTPPropertyCodeCanonWhiteBalanceAdjustB:
                property.type = PTPDataTypeCodeSInt32;
                break;
              case PTPPropertyCodeCanonCameraTime:
              case PTPPropertyCodeCanonUTCTime:
              case PTPPropertyCodeCanonSummertime:
              case PTPPropertyCodeCanonAvailableShots:
              case PTPPropertyCodeCanonCaptureDestination:
              case PTPPropertyCodeCanonWhiteBalanceXA:
              case PTPPropertyCodeCanonWhiteBalanceXB:
              case PTPPropertyCodeCanonCurrentStorage:
              case PTPPropertyCodeCanonCurrentFolder:
              case PTPPropertyCodeCanonShutterCounter:
              case PTPPropertyCodeCanonModelID:
              case PTPPropertyCodeCanonLensID:
              case PTPPropertyCodeCanonStroboFiring:
              case PTPPropertyCodeCanonAFSelectFocusArea:
              case PTPPropertyCodeCanonContinousAFMode:
              case PTPPropertyCodeCanonColorTemperature:
              case PTPPropertyCodeCanonWftStatus:
              case PTPPropertyCodeCanonLensStatus:
              case PTPPropertyCodeCanonCardExtension:
              case PTPPropertyCodeCanonTempStatus:
              case PTPPropertyCodeCanonPhotoStudioMode:
              case PTPPropertyCodeCanonDepthOfFieldPreview:
              case PTPPropertyCodeCanonEVFSharpness:
              case PTPPropertyCodeCanonEVFWBMode:
              case PTPPropertyCodeCanonEVFClickWBCoeffs:
              case PTPPropertyCodeCanonEVFColorTemp:
              case PTPPropertyCodeCanonExposureSimMode:
              case PTPPropertyCodeCanonLvAfSystem:
              case PTPPropertyCodeCanonMovSize:
              case PTPPropertyCodeCanonDepthOfField:
              case PTPPropertyCodeCanonLvViewTypeSelect:
              case PTPPropertyCodeCanonAloMode:
              case PTPPropertyCodeCanonBrightness:
                property.type = PTPDataTypeCodeUInt32;
                break;
              case PTPPropertyCodeCanonOwner:
              case PTPPropertyCodeCanonArtist:
              case PTPPropertyCodeCanonCopyright:
              case PTPPropertyCodeCanonSerialNumber:
              case PTPPropertyCodeCanonLensName:
                property.type = PTPDataTypeCodeUnicodeString;
                break;
              default:
                property.type = PTPDataTypeCodeUndefined;
                break;
            }
            switch (property.type) {
              case PTPDataTypeCodeUndefined: {
                switch (code) {
                  case PTPPropertyCodeCanonCustomFunc1:
                  case PTPPropertyCodeCanonCustomFunc2:
                  case PTPPropertyCodeCanonCustomFunc3:
                  case PTPPropertyCodeCanonCustomFunc4:
                  case PTPPropertyCodeCanonCustomFunc5:
                  case PTPPropertyCodeCanonCustomFunc6:
                  case PTPPropertyCodeCanonCustomFunc7:
                  case PTPPropertyCodeCanonCustomFunc8:
                  case PTPPropertyCodeCanonCustomFunc9:
                  case PTPPropertyCodeCanonCustomFunc10:
                  case PTPPropertyCodeCanonCustomFunc11:
                  case PTPPropertyCodeCanonCustomFunc12:
                  case PTPPropertyCodeCanonCustomFunc13:
                  case PTPPropertyCodeCanonCustomFunc14:
                  case PTPPropertyCodeCanonCustomFunc15:
                  case PTPPropertyCodeCanonCustomFunc16:
                  case PTPPropertyCodeCanonCustomFunc17:
                  case PTPPropertyCodeCanonCustomFunc18:
                  case PTPPropertyCodeCanonCustomFunc19: {
                    property.type = PTPDataTypeCodeUnicodeString;
                    NSMutableString *string = [NSMutableString stringWithFormat:@"%02x", *buf++];
                    while (buf - record < size) {
                      [string appendFormat:@" %02x", *buf++];
                    }
                    property.defaultValue = NULL;
                    property.value = string.description;
                    break;
                  }
                  case PTPPropertyCodeCanonCustomFuncEx: {
                    ptpReadUnsignedInt(&buf);
                    unsigned int *customFuncEx = (unsigned int *)buf;
                    int offset = 0;
                    unsigned int groupCount = customFuncEx[offset++];
                    for (int i = 0; i < groupCount; i++) {
                      unsigned int group = customFuncEx[offset++];
                      unsigned int groupSize = customFuncEx[offset++];
                      if (customFuncGroup[group]) {
                        if (customFuncGroup[group][3] != groupSize) {
                          customFuncGroup[group] = realloc(customFuncGroup[group], groupSize + 12);
                          [self.delegate debug:[NSString stringWithFormat:@"customFuncEx - group %d buffer reallocated", group]];
                        }
                      } else {
                        customFuncGroup[group] = malloc(groupSize + 12);
                        customFuncGroup[group][0] = groupSize + 12; // total size
                        customFuncGroup[group][1] = 1; // group count
                        customFuncGroup[group][2] = group; // group
                        customFuncGroup[group][3] = groupSize; // group size
                        [self.delegate debug:[NSString stringWithFormat:@"customFuncEx - group %d buffer allocated", group]];
                      }
                      memcpy(customFuncGroup[group] + 4, customFuncEx + offset, groupSize - 4);
                      NSMutableString *s = [NSMutableString stringWithFormat:@"0x%x", customFuncGroup[group][0]];
                      for (int i = 1; i < groupSize / 4 + 3; i++) {
                        [s appendFormat:@", 0x%x", customFuncGroup[group][i]];
                      }
                      [self.delegate debug:[NSString stringWithFormat:@"customFuncEx - group %d [%@]", group, s]];
                      unsigned int itemCount = customFuncEx[offset++];
                      for (int j = 0; j < itemCount; j++) {
                        unsigned int item = customFuncEx[offset++];
                        unsigned int valueSize = customFuncEx[offset++];
                        unsigned short itemCode = item | 0x8000;
                        NSNumber *itemNumber = [NSNumber numberWithUnsignedShort:itemCode];
                        PTPProperty *itemProperty = self.info.properties[itemNumber];
                        if (itemProperty == nil)
                          itemProperty = [[self.propertyClass alloc] initWithCode:itemCode];
                        itemProperty.type = PTPDataTypeCodeUInt16;
                        NSMutableString *value = [NSMutableString stringWithFormat:@"%u", customFuncEx[offset++]];
                        for (int k = 1; k < valueSize; k++)
                          [value appendFormat:@"%u", customFuncEx[offset++]];
                        itemProperty.defaultValue = itemProperty.value = value.description;
                        if (![self.info.propertiesSupported containsObject:itemNumber])
                          [(NSMutableArray *)self.info.propertiesSupported addObject:itemNumber];
                        self.info.properties[[NSNumber numberWithUnsignedShort:itemCode]] = itemProperty;
                        [properties addObject:itemProperty];
                        [self.delegate debug:[NSString stringWithFormat:@"PTPEventCodeCanonPropValueChanged %@", itemProperty]];
                      }
                    }
                    break;
                  }
                  case PTPPropertyCodeCanonImageFormat:
                  case PTPPropertyCodeCanonImageFormatCF:
                  case PTPPropertyCodeCanonImageFormatSD:
                  case PTPPropertyCodeCanonImageFormatExtHD: {
                    property.type = PTPDataTypeCodeUInt64;
                    property.defaultValue = property.value = [NSNumber numberWithUnsignedLong:ptpReadCanonImageFormat(&buf)];
                    break;
                  }
                }
                break;
              }
              case PTPDataTypeCodeUInt8:
                property.defaultValue = property.value = [NSNumber numberWithUnsignedChar:ptpReadUnsignedChar(&buf)];
                break;
              case PTPDataTypeCodeUInt16:
                property.defaultValue = property.value = [NSNumber numberWithUnsignedShort:ptpReadUnsignedShort(&buf)];
                break;
              case PTPDataTypeCodeSInt16:
                property.defaultValue = property.value = [NSNumber numberWithShort:ptpReadShort(&buf)];
                break;
              case PTPDataTypeCodeUInt32:
                property.defaultValue = property.value = [NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(&buf)];
                break;
              case PTPDataTypeCodeSInt32:
                property.defaultValue = property.value = [NSNumber numberWithInt:ptpReadInt(&buf)];
                break;
              case PTPDataTypeCodeUnicodeString:
                property.defaultValue = property.value = [NSString stringWithCString:(char *)buf encoding:NSUTF8StringEncoding];
                if (property.value == nil)
                  property.value = @"";
                break;
            }
            if (property.type != PTPDataTypeCodeUndefined) {
              self.info.properties[[NSNumber numberWithUnsignedShort:code]] = property;
              [properties addObject:property];
            }
            if (code == PTPPropertyCodeCanonAutoExposureMode) {
              currentMode = property.value.intValue;
            }
            [self.delegate debug:[NSString stringWithFormat:@"PTPEventCodeCanonPropValueChanged %@", property]];
            break;
          }
          case PTPEventCodeCanonAvailListChanged: {
            unsigned int code = ptpReadUnsignedInt(&buf);
            PTPProperty *property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
            if (property == nil)
              property = [[self.propertyClass alloc] initWithCode:code];
            unsigned int type = ptpReadUnsignedInt(&buf);
            unsigned int count = ptpReadUnsignedInt(&buf);
            if (count == 0 || count >= 0xFFFF || type != 3)
              break;
            NSMutableArray *values = [NSMutableArray array];
            if (code == PTPPropertyCodeCanonImageFormat || code == PTPPropertyCodeCanonImageFormatCF || code == PTPPropertyCodeCanonImageFormatSD || code == PTPPropertyCodeCanonImageFormatExtHD) {
              for (int i = 0; i < count; i++)
                [values addObject:[NSNumber numberWithUnsignedLong:ptpReadCanonImageFormat(&buf)]];
            } else {
              for (int i = 0; i < count; i++)
                [values addObject:[NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(&buf)]];
            }
            property.supportedValues = values;
            [properties addObject:property];
            [self.delegate debug:[NSString stringWithFormat:@"PTPEventCodeCanonAvailListChanged %@", property]];
            break;
          }
          case PTPEventCodeCanonObjectAddedEx: {
            unsigned int obj = ptpReadUnsignedInt(&buf);
            if (self->doImageDownload) {
              NSNumber *key = [NSNumber numberWithUnsignedInt:obj];
              NSString *value = [NSString stringWithCString:(char *)(buf + 0x1C) encoding:NSUTF8StringEncoding];
              [self.delegate debug:[NSString stringWithFormat:@"PTPEventCodeCanonObjectAddedEx '%@' = '%@'", key, value]];
              [addedFileName setObject:value forKey:key];
              self.remainingCount++;
              [self sendPTPRequest:PTPRequestCodeCanonGetObject param1:obj];
            } else if (self.deleteDownloadedImage) {
                [self sendPTPRequest:PTPRequestCodeCanonDeleteObject param1:obj];
            }
            break;
          }
          case PTPEventCodeCanonObjectAddedEx2: {
            unsigned int obj = ptpReadUnsignedInt(&buf);
            if (self->doImageDownload) {
              NSNumber *key = [NSNumber numberWithUnsignedInt:obj];
              NSString *value = [NSString stringWithCString:(char *)(buf + 0x20) encoding:NSUTF8StringEncoding];
              [self.delegate debug:[NSString stringWithFormat:@"PTPEventCodeCanonObjectAddedEx2 '%@' = '%@'", key, value]];
              [addedFileName setObject:value forKey:key];
              self.remainingCount++;
              [self sendPTPRequest:PTPRequestCodeCanonGetObject param1:obj];
            } else if (self.deleteDownloadedImage) {
              [self sendPTPRequest:PTPRequestCodeCanonDeleteObject param1:obj];
            }
            break;
          }
          default:
            [self.delegate debug:[NSString stringWithFormat:@"%@ + %dbytes", [PTPCanonEvent eventCodeName:type], size]];
            break;
        }
        buf = record + size;
      }
      for (PTPProperty *property in properties) {
        switch (property.propertyCode) {
          case PTPPropertyCodeCanonWhiteBalanceAdjustA:
          case PTPPropertyCodeCanonWhiteBalanceAdjustB: {
            NSMutableArray *values = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              int i = value.intValue;
              [values addObject:[NSString stringWithFormat:@"%d", i]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:values readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonModelID:
          case PTPPropertyCodeCanonCurrentFolder:
          case PTPPropertyCodeCanonCameraTime: {
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonAperture: {
            NSDictionary *map = @{ @0x08: @"f/1", @0x0B: @"f/1.1", @0x0C: @"f/1.2", @0x0D: @"f/1.2", @0x10: @"f/1.4", @0x13: @"f/1.6", @0x14: @"f/1.8", @0x15: @"f/1.8", @0x18: @"f/2", @0x1B: @"f/2.2", @0x1C: @"f/2.5", @0x1D: @"f/2.5", @0x20: @"f/2.8", @0x23: @"f/3.2", @0x24: @"f/3.5", @0x25: @"f/3.5", @0x28: @"f/4", @0x2B: @"f/4.5", @0x2C: @"f/4.5", @0x2D: @"f/5.0", @0x30: @"f/5.6", @0x33: @"f/6.3", @0x34: @"f/6.7", @0x35: @"f/7.1", @0x38: @"f/8", @0x3B: @"f/9", @0x3C: @"f/9.5", @0x3D: @"f/10", @0x40: @"f/11", @0x43: @"f/13", @0x44: @"f/13", @0x45: @"f/14", @0x48: @"f/16", @0x4B: @"f/18", @0x4C: @"f/19", @0x4D: @"f/20", @0x50: @"f/22", @0x53: @"f/25", @0x54: @"f/27", @0x55: @"f/29", @0x58: @"f/32", @0x5B: @"f/36", @0x5C: @"f/38", @0x5D: @"f/40", @0x60: @"f/45", @0x63: @"f/51", @0x64: @"f/54", @0x65: @"f/57", @0x68: @"f/64", @0x6B: @"f/72", @0x6C: @"f/76", @0x6D: @"f/80", @0x70: @"f/91" };
            property.readOnly = currentMode != 2 && currentMode != 3 && currentMode != 4;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonShutterSpeed: {
            NSDictionary *map = @{ @0x0C: @"Bulb", @0x10: @"30s", @0x13: @"25s", @0x14: @"20s", @0x15: @"20s", @0x18: @"15s", @0x1B: @"13s", @0x1C: @"10s", @0x1D: @"10s", @0x20: @"8s", @0x23: @"6s", @0x24: @"6s", @0x25: @"5s", @0x28: @"4s", @0x2B: @"3.2s", @0x2C: @"3s", @0x2D: @"2.5s", @0x30: @"2s", @0x33: @"1.6s", @0x34: @"15s", @0x35: @"1.3s", @0x38: @"1s", @0x3B: @"0.8s", @0x3C: @"0.7s", @0x3D: @"0.6s", @0x40: @"0.5s", @0x43: @"0.4s", @0x44: @"0.3s", @0x45: @"0.3s", @0x48: @"1/4s", @0x4B: @"1/5s", @0x4C: @"1/6s", @0x4D: @"1/6s", @0x50: @"1/8s", @0x53: @"1/10s", @0x54: @"1/10s", @0x55: @"1/13s", @0x58: @"1/15s", @0x5B: @"1/20s", @0x5C: @"1/20s", @0x5D: @"1/25s", @0x60: @"1/30s", @0x63: @"1/40s", @0x64: @"1/45s", @0x65: @"1/50s", @0x68: @"1/60s", @0x6B: @"1/80s", @0x6C: @"1/90s", @0x6D: @"1/100s", @0x70: @"1/125s", @0x73: @"1/160s", @0x74: @"1/180s", @0x75: @"1/200s", @0x78: @"1/250s", @0x7B: @"1/320s", @0x7C: @"1/350s", @0x7D: @"1/400s", @0x80: @"1/500s", @0x83: @"1/640s", @0x84: @"1/750s", @0x85: @"1/800s", @0x88: @"1/1000s", @0x8B: @"1/1250s", @0x8C: @"1/1500s", @0x8D: @"1/1600s", @0x90: @"1/2000s", @0x93: @"1/2500s", @0x94: @"1/3000s", @0x95: @"1/3200s", @0x98: @"1/4000s", @0x9B: @"1/5000s", @0x9C: @"1/6000s", @0x9D: @"1/6400s", @0xA0: @"1/8000s" };
            property.readOnly = currentMode != 1 && currentMode != 3;
            currentShutterSpeed = property.value.intValue;
            if (property.readOnly) {
              if (currentMode == 4)
                [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Bulb" values:@[ @"Bulb" ] labels:@[ @"Bulb" ] readOnly:true];
              else
                [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            } else {
              [self mapValueList:property map:map];
            }
            break;
          }
          case PTPPropertyCodeCanonISOSpeed: {
            NSDictionary *map = @{ @0x00: @"Auto", @0x40: @"50", @0x48: @"100", @0x4b: @"125", @0x4d: @"160", @0x50: @"200", @0x53: @"250", @0x55: @"320", @0x58: @"400", @0x5b: @"500", @0x5d: @"640", @0x60: @"800", @0x63: @"1000", @0x65: @"1250", @0x68: @"1600", @0x6b: @"2000", @0x6d: @"2500", @0x70: @"3200", @0x73: @"4000", @0x75: @"5000", @0x78: @"6400", @0x7b: @"8000", @0x7d: @"10000", @0x80: @"12800", @0x83: @"16000", @0x85: @"20000", @0x88: @"25600", @0x90: @"51200", @0x98: @"102400" };
            property.readOnly = currentMode >= 8;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExpCompensation: {
            NSDictionary *map = @{ @0x28: @"+5", @0x25: @"+4 2/3", @0x24: @"+4 1/2", @0x23: @"+4 1/3", @0x20: @"+4", @0x1D: @"+3 2/3", @0x1C: @"+3 1/2", @0x1B: @"+3 1/3", @0x18: @"+3", @0x15: @"+2 2/3", @0x14: @"+2 1/2", @0x13: @"+2 1/3", @0x10: @"+2", @0x0D: @"+1 2/3", @0x0C: @"+1 1/2", @0x0B: @"+1 1/3", @0x08: @"+1", @0x05: @"+2/3", @0x04: @"+1/2", @0x03: @"+1/3", @0x00: @"0", @0xFD: @"-1/3", @0xFC: @"-1/2", @0xFB: @"-2/3", @0xF8: @"-1", @0xF5: @"-1 1/3", @0xF4: @"-1 1/2", @0xF3: @"-1 2/3", @0xF0: @"-2", @0xED: @"-2 1/3", @0xEC: @"-2 1/2", @0xEB: @"-2 2/3", @0xE8: @"-3", @0xE5: @"-3 1/3", @0xE4: @"-3 1/2", @0xE3: @"-3 2/3", @0xE0: @"-4", @0xDD: @"-4 1/3", @0xDC: @"-4 1/2", @0xDB: @"-4 2/3", @0xD8: @"-5" };
            property.readOnly = currentMode >= 8 || currentMode == 3;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonMeteringMode: {
            NSDictionary *map = @{ @1: @"Spot", @3: @"Evaluative", @4: @"Partial", @5: @"Center-weighted" };
            property.readOnly = currentMode >= 8;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonAutoExposureMode: {
            NSDictionary *map = @{ @0: @"Program AE", @1: @"Shutter Priority AE", @2: @"Aperture Priority AE", @3: @"Manual Exposure", @4: @"Bulb", @5: @"Auto DEP AE", @6: @"DEP AE", @8: @"Lock", @9: @"Auto", @10: @"Night Scene Portrait", @11: @"Sports", @12: @"Portrait", @13: @"Landscape", @14: @"Close-Up", @15: @"Flash Off", @19: @"Creative Auto", @22: @"Scene Intelligent Auto" };
            [self mapValueInterval:property map:map];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonAperture readOnly:currentMode != 2 && currentMode != 3 && currentMode != 4];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonShutterSpeed readOnly:currentMode != 1 && currentMode != 3];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonISOSpeed readOnly:currentMode >= 8];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonExpCompensation readOnly:currentMode >= 8 || currentMode == 3];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonWhiteBalance readOnly:currentMode >= 8];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonFocusMode readOnly:currentMode >= 8];
            [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeCanonPictureStyle readOnly:currentMode >= 8];
            break;
          }
          case PTPPropertyCodeCanonEVFWBMode:
          case PTPPropertyCodeCanonWhiteBalance: {
            NSDictionary *map = @{ @0: @"Auto", @1: @"Daylight", @2: @"Cloudy", @3: @"Tungsten", @4: @"Fluorescent", @5: @"Flash", @6: @"Manual", @8: @"Shade", @9: @"Color temperature", @10: @"Custom white balance: PC-1", @11: @"Custom white balance: PC-2", @12: @"Custom white balance: PC-3", @15: @"Manual 2", @16: @"Manual 3", @18: @"Manual 4", @19: @"Manual 5", @20: @"Custom white balance: PC-4", @21: @"Custom white balance: PC-5" };
            property.readOnly = currentMode >= 8;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonFocusMode: {
            NSDictionary *map = @{ @0: @"One-Shot AF", @1: @"AI Servo AF", @2: @"AI Focus AF", @3: @"Manual" };
            property.readOnly = currentMode >= 8;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonPictureStyle: {
            NSDictionary *map = @{ @0x81: @"Standard", @0x82: @"Portrait", @0x83: @"Landscape", @0x84: @"Neutral", @0x85: @"Faithful", @0x86: @"Monochrome", @0x87:@"Auto", @0x88: @"Fine detail", @0x21: @"User 1", @0x22: @"User 2", @0x23: @"User 3", @0x41: @"PC 1", @0x42: @"PC 2", @0x43: @"PC 3" };
            property.readOnly = currentMode >= 8;
            if (property.readOnly)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"Auto" values:@[ @"Auto" ] labels:@[ @"Auto" ] readOnly:true];
            else
              [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonColorSpace: {
            NSDictionary *map = @{ @1: @"sRGB", @2: @"Adobe" };
            [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonCaptureDestination: {
            NSDictionary *map = @{ };
            [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonEVFOutputDevice: {
            [self.delegate cameraCanPreview:self];
            NSDictionary *map = @{ @1: @"TFT", @2: @"PC" };
            [self mapValueList:property map:map];
            if (startPreview && property.value.intValue) {
              startPreview = false;
              inPreview = true;
              [self getPreviewImage];
            } else if (tempLiveView && property.value.intValue) {
              inPreview = true;
            } else if (property.value.intValue == 0) {
              inPreview = false;
            }
            break;
          }
          case PTPPropertyCodeCanonEVFMode: {
            NSDictionary *map = @{ @0: @"Disable", @1: @"Enable" };
            [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonDriveMode: {
            NSDictionary *map = @{ @0x00: @"Single shot", @0x01: @"Continuos", @0x02: @"Video", @0x04: @"High speed continuous", @0x05: @"Low speed continuous", @0x06: @"Silent", @0x07: @"10s self timer + continuous", @0x10: @"10s self timer", @0x11: @"2s self timer", @0x12: @"14fps high speed", @0x13: @"Silent single shot", @0x14: @"Silent continuous", @0x15: @"Silent high speed continuous", @0x16: @"Silent low speed continuous"};
            [self mapValueList:property map:map];
            break;
          }
          case PTPPropertyCodeCanonImageFormat:
          case PTPPropertyCodeCanonImageFormatCF:
          case PTPPropertyCodeCanonImageFormatSD:
          case PTPPropertyCodeCanonImageFormatExtHD: { // size << 24 | format << 16 | quality << 8 | compression;
            NSDictionary *map = @{ @0x10010003:@"Large fine JPEG", @0x10010002:@"Large JPEG", @0x10010103:@"Medium fine JPEG", @0x10010102:@"Medium JPEG", @0x10010203:@"Small fine JPEG", @0x10010202:@"Small JPEG", @0x10010503:@"M1 fine JPEG", @0x10010502:@"M1 JPEG", @0x10010603:@"M2 fine JPEG", @0x10010602:@"M2 JPEG", @0x10010e03:@"S1 fine JPEG", @0x10010e02:@"S1 JPEG", @0x10010f03:@"S2 fine JPEG", @0x10010f02:@"S2 JPEG", @0x10011003:@"S3 fine JPEG", @0x10011002:@"S3 JPEG", @0x10060002:@"CRW", @0x10060004:@"RAW", @0x10060104:@"MRAW", @0x10060204:@"SRAW", @0x10060006:@"CR2" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              long l = value.longValue;
              long i1 = (l >> 32) & 0xFFFFFFFF;
              long i2 = l & 0xFFFFFFFF;
              NSMutableString *label = [NSMutableString string];
              if (i1 != 0) {
                NSString *l1 = map[[NSNumber numberWithInteger:i1]];
                if (l1)
                  [label appendFormat:@"%@ + ", l1];
              }
              NSString *l2 = map[[NSNumber numberWithInteger:i2]];
              if (l2)
                [label appendString:l2];
              [values addObject:value.description];
              [labels addObject:label.description];
            }
            if (property.value)
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            else
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:true];
            break;
          }
          case PTPPropertyCodeCanonBatteryPower: {
            switch (property.value.intValue) {
              case 1:
                [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeBatteryLevel value:@50 min:@0 max:@100 step:@25 readOnly:true];
                break;
              case 2:
                [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeBatteryLevel value:@100 min:@0 max:@100 step:@25 readOnly:true];
                break;
              case 4:
                [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeBatteryLevel value:@75 min:@0 max:@100 step:@25 readOnly:true];
                break;
              case 5:
                [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeBatteryLevel value:@25 min:@0 max:@100 step:@25 readOnly:true];
                break;
              default:
                [self.delegate cameraPropertyChanged:self code:PTPPropertyCodeBatteryLevel value:@0 min:@0 max:@100 step:@25 readOnly:true];
                break;
            }
            break;
          }
          case PTPPropertyCodeBatteryLevel: {
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:(NSNumber *)property.value min:@0 max:@100 step:@25 readOnly:true];
            break;
          }
          case PTPPropertyCodeCanonExExposureLevelIncrements: {
            NSDictionary *map = @{ @0: @"1/3", @1: @"1/2" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExISOExpansion: {
            NSDictionary *map = @{ @0: @"Off", @1: @"On" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExFlasgSyncSpeedInAvMode: {
            NSDictionary *map = @{ @0: @"Auto", @1: @"1/200s" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExLongExposureNoiseReduction: {
            NSDictionary *map = @{ @0: @"Off", @1: @"Auto", @2: @"On" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExHighISONoiseReduction: {
            NSDictionary *map = @{ @0: @"Off", @1: @"On" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExHHighlightTonePriority: {
            NSDictionary *map = @{ @0: @"Enable", @1: @"Disable" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExAutoLightingOptimizer: {
            NSDictionary *map = @{ @0: @"Enable", @1: @"Disable" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExAFAssistBeamFiring: {
            NSDictionary *map = @{ @0: @"Enable", @1: @"Disable", @2: @"Only external flash emits" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExAFDuringLiveView: {
            NSDictionary *map = @{ @0: @"Disable", @1: @"Quick mode", @2: @"Live mode" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonMirrorUpSetting:
          case PTPPropertyCodeCanonExMirrorLockup: {
            NSDictionary *map = @{ @0: @"Disable", @1: @"Enable" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExShutterAELockButton: {
            NSDictionary *map = @{ @0: @"AF/AE Lock", @1: @"AE Lock/AF", @2: @"AF/AF lock, no AE lock", @3: @"AE/AF, no AE lock" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExSetButtonWhenShooting: {
            NSDictionary *map = @{ @0: @"LCD monitor On/Off", @1: @"Change quality", @2: @"Flash exposure compensation", @3: @"Menu display", @4: @"Disable" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExLCDDisplayWhenPowerOn: {
            NSDictionary *map = @{ @0: @"Display", @1: @"Retain power off status" };
            [self mapValueInterval:property map:map];
            break;
          }
          case PTPPropertyCodeCanonExAddOriginalDecisionData: {
            NSDictionary *map = @{ @0: @"Off", @1: @"On" };
            [self mapValueInterval:property map:map];
            break;
          }
          default: {
            if (property.supportedValues) {
              NSMutableArray *values = [NSMutableArray array];
              for (NSObject *value in property.supportedValues)
                [values addObject:value.description];
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:values readOnly:property.readOnly];
            } else if (property.type == PTPDataTypeCodeUnicodeString) {
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description readOnly:property.readOnly];
            } else {
              [self.delegate cameraPropertyChanged:self code:property.propertyCode value:(NSNumber *)property.value min:(NSNumber *)property.min max:(NSNumber *)property.max step:property.step readOnly:property.readOnly];
            }
            break;
          }
        }
      }
      break;
    }
    case PTPRequestCodeCanonGetObject: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        NSNumber *key = [NSNumber numberWithUnsignedInt:request.parameter1];
        NSString *name = addedFileName[key];
        [addedFileName removeObjectForKey:key];
        self.remainingCount--;
        if (name) {
          [self.delegate cameraExposureDone:self data:data filename:name];
          if (self.deleteDownloadedImage)
            [self sendPTPRequest:PTPRequestCodeCanonDeleteObject param1:request.parameter1];
        }
      } else {
        [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"Download failed (0x%04x = %@)", response.responseCode, response]];
      }
      break;
    }
    case PTPRequestCodeCanonGetViewFinderData: {
      if (response.responseCode == PTPResponseCodeOK) {
        unsigned char *bytes = (unsigned char *)[data bytes];
        unsigned char *buf = bytes;
        unsigned long size = data.length;
        NSData *image = nil;
        while (buf - bytes < size) {
          unsigned int length = ptpReadUnsignedInt(&buf);
          unsigned int type = ptpReadUnsignedInt(&buf);
          //[self.delegate debug:[NSString stringWithFormat:@"PTPRequestCodeCanonGetViewFinderData data contains %d type record, %db", type, length]];
          if (type == 1) {
            image = [NSData dataWithBytes:buf length:length - 8];
            buf += length - 8;
            break;
          } else {
            buf += length - 8;
            continue;
          }
        }
        if (image) {
          self.remainingCount = 0;
          [self.delegate cameraExposureDone:self data:image filename:@"preview.jpeg"];
          [self getPreviewImage];
        } else {
          [self stopPreview];
          [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"No preview data received"]];
        }
      } else if ((inPreview || startPreview || tempLiveView) && response.responseCode == PTPResponseCodeCanonNotReady) {
        [self getPreviewImage];
      } else {
        [self stopPreview];
        [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"Preview failed (0x%04x = %@)", response.responseCode, response]];
      }
      break;
    }
    case PTPRequestCodeCanonDriveLens: {
      if (response.responseCode == PTPResponseCodeOK) {
        if (focusSteps == 0) {
          if (tempLiveView) {
            tempLiveView = false;
            [self setProperty:PTPPropertyCodeCanonEVFOutputDevice value:@"0"];
          }
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
            [self.delegate cameraFocusDone:self];
          });
        } else if (focusSteps > 0) {
          focusSteps--;
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
            [self sendPTPRequest:PTPRequestCodeCanonDriveLens param1:1];
          });
        } else {
          focusSteps++;
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
            [self sendPTPRequest:PTPRequestCodeCanonDriveLens param1:0x8001];
          });
        }
      } else {
        [self.delegate cameraFocusFailed:self message:[NSString stringWithFormat:@"DriveLens failed (0x%04x = %@)", response.responseCode, response]];
      }
      break;
    }      
    case PTPRequestCodeCanonRemoteReleaseOn:
    case PTPRequestCodeCanonRemoteRelease: {
      if (response.responseCode != PTPResponseCodeOK || response.parameter1 == 1)
        [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"RemoteRelease failed (0x%04x = %@)", response.responseCode, response]];
      break;
    }
    case PTPRequestCodeCanonZoom: {
      break;
    }
    default: {
      [super processRequest:request Response:response inData:data];
      break;
    }
  }
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  PTPProperty *property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
  if (property) {
    unsigned char *buffer = NULL;
    unsigned int size = 0;
    unsigned short propertyCode = property.propertyCode;
    switch (propertyCode) {
      case PTPPropertyCodeCanonImageFormat:
      case PTPPropertyCodeCanonImageFormatCF:
      case PTPPropertyCodeCanonImageFormatSD:
      case PTPPropertyCodeCanonImageFormatExtHD: {
        long l = (long)value.longLongValue;
        long i1 = (l >> 32) & 0xFFFFFFFF;
        long i2 = l & 0xFFFFFFFF;
        int count = i1 == 0 ? 1 : 2;
        buffer = malloc(size = 8 + 4 * (4 * count + 1));
        unsigned char *buf = buffer + 8;
        ptpWriteUnsignedInt(&buf, count);
        if (count == 2) {
          ptpWriteUnsignedInt(&buf, (i1 >> 24) & 0xFF);
          ptpWriteUnsignedInt(&buf, (i1 >> 16) & 0xFF);
          ptpWriteUnsignedInt(&buf, (i1 >> 8) & 0xFF);
          ptpWriteUnsignedInt(&buf, i1 & 0xFF);
        }
        ptpWriteUnsignedInt(&buf, (i2 >> 24) & 0xFF);
        ptpWriteUnsignedInt(&buf, (i2 >> 16) & 0xFF);
        ptpWriteUnsignedInt(&buf, (i2 >> 8) & 0xFF);
        ptpWriteUnsignedInt(&buf, i2 & 0xFF);
        self.imagesPerShot = count;
        break;
      }
      default: {
        if ((propertyCode & 0xF000) == 0x8000) {
          for (int i = 0; buffer == NULL && i < 10; i++) {
            unsigned int *customFuncEx = customFuncGroup[i];
            if (customFuncEx) {
              int offset = 4;
              unsigned int itemCount = customFuncEx[offset++];
              for (int j = 0; j < itemCount; j++) {
                unsigned int item = customFuncEx[offset++];
                unsigned int valueSize = customFuncEx[offset++];
                unsigned short itemCode = item | 0x8000;
                if (itemCode == propertyCode) {
                  NSArray *values = [value componentsSeparatedByString:@" "];
                  for (int k = 0; k < valueSize; k++)
                    customFuncEx[offset++] = [values[k] intValue];
                  buffer = malloc(size = customFuncEx[3] + 20);
                  memcpy(buffer + 8, customFuncEx, size - 8);
                  propertyCode = PTPPropertyCodeCanonCustomFuncEx;
                  break;
                } else {
                  offset += valueSize;
                }
              }
            }
          }
        } else {
          switch (property.type) {
            case PTPDataTypeCodeSInt8:
            case PTPDataTypeCodeUInt8: {
              buffer = malloc(size = 12);
							memset(buffer, 0, 12);
              *(char *)(buffer + 8) = value.intValue;
              break;
            }
            case PTPDataTypeCodeSInt16:
            case PTPDataTypeCodeUInt16: {
              buffer = malloc(size = 12);
							memset(buffer, 0, 12);
              *(int16_t *)(buffer + 8) = CFSwapInt16HostToLittle(value.intValue);
              break;
            }
            case PTPDataTypeCodeSInt32:
            case PTPDataTypeCodeUInt32: {
              buffer = malloc(size = 12);
              *(int *)(buffer + 8) = CFSwapInt32LittleToHost(value.intValue);
              break;
            }
            case PTPDataTypeCodeUnicodeString: {
              const char *s = [value cStringUsingEncoding:NSUTF8StringEncoding];
              if (s) {
                buffer = malloc(size = 8 + 1 + (unsigned int)strlen(s));
                strcpy((char *)buffer + 8, s);
              }
              break;
            }
          }
        }
        break;
      }
    }
    if (buffer) {
      unsigned char *buf = buffer;
      ptpWriteUnsignedInt(&buf, size);
      ptpWriteUnsignedInt(&buf, propertyCode);
      [self sendPTPRequest:PTPRequestCodeCanonSetDevicePropValueEx data:[NSData dataWithBytesNoCopy:buffer length:size freeWhenDone:YES]];
    }
  }
}

-(void)setZoomPreview:(BOOL)zoomPreview {
  [self sendPTPRequest:PTPRequestCodeCanonZoom param1:liveViewZoom = (zoomPreview ? 5 : 1)];
  [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
}

-(void)getPreviewImage {
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
    [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
    [self sendPTPRequest:PTPRequestCodeCanonGetViewFinderData param1:0x00100000];
  });
}

-(void)requestEnableTethering {
}

-(void)requestCloseSession {
  if (inPreview)
    [self stopPreview];
  [self sendPTPRequest:PTPRequestCodeCanonSetRemoteMode param1:0];
  [self sendPTPRequest:PTPRequestCodeCanonSetEventMode param1:0];
  [super requestCloseSession];
  for (int i = 0; i < 10; i++) {
    if (customFuncGroup[i]) {
      free(customFuncGroup[i]);
      customFuncGroup[i] = NULL;
    }
  }
}

-(void)lock {
  [self sendPTPRequest:PTPRequestCodeCanonSetUILock];
}

-(void)unlock {
  [self sendPTPRequest:PTPRequestCodeCanonResetUILock];
}

-(void)startPreview {
  self.remainingCount = 0;
  if (self.zoomPreview)
    liveViewZoom = 5;
  else
    liveViewZoom = 1;
  startPreview = true;
  [self setProperty:PTPPropertyCodeCanonEVFMode value:@"1"];
  [self setProperty:PTPPropertyCodeCanonEVFOutputDevice value:@"2"];
}

-(void)stopPreview {
  inPreview = false;
  tempLiveView = false;
  [self setProperty:PTPPropertyCodeCanonEVFOutputDevice value:@"0"];
  //[self setProperty:PTPPropertyCodeCanonEVFMode value:@"0"];
}

-(void)startBurst {
  self.remainingCount = 0;
  self->doImageDownload = true;
  [self setProperty:PTPPropertyCodeCanonDriveMode value:@"1"];
  [self sendPTPRequest:PTPRequestCodeCanonRemoteReleaseOn param1:3 param2:(self.avoidAF ? 1 : 0)];
}

-(void)stopBurst {
  self->doImageDownload = false;
  [self sendPTPRequest:PTPRequestCodeCanonRemoteReleaseOff param1:3];
}

-(double)startExposure {
  self.remainingCount = 0;
  self->doImageDownload = true;
  double delay = 0;
  if ([self operationIsSupported:PTPRequestCodeCanonRemoteReleaseOn]) {
    if (self.useMirrorLockup) {
      if ([self propertyIsSupported:PTPPropertyCodeCanonMirrorUpSetting])
        [self setProperty:PTPPropertyCodeCanonMirrorUpSetting value:@"1"];
      else
        [self setProperty:PTPPropertyCodeCanonExMirrorLockup value:@"1"];
      [self setProperty:PTPPropertyCodeCanonDriveMode value:@"17"];
      delay = 2.0;
    } else {
      if ([self propertyIsSupported:PTPPropertyCodeCanonMirrorUpSetting])
        [self setProperty:PTPPropertyCodeCanonMirrorUpSetting value:@"0"];
      else
        [self setProperty:PTPPropertyCodeCanonExMirrorLockup value:@"0"];
      [self setProperty:PTPPropertyCodeCanonDriveMode value:@"0"];
    }
    [self.delegate log:@"Exposure initiated"];
    [self sendPTPRequest:PTPRequestCodeCanonRemoteReleaseOn param1:3 param2:(self.avoidAF ? 1 : 0)];
    if (currentShutterSpeed != 0x0C && currentMode != 4) {
      [self sendPTPRequest:PTPRequestCodeCanonRemoteReleaseOff param1:3];
    }
  } else {
    [self.delegate log:@"Exposure initiated"];
    if (currentShutterSpeed == 0x0C || currentMode == 4) {
      [self sendPTPRequest:PTPRequestCodeCanonBulbStart];
    } else {
      [self sendPTPRequest:PTPRequestCodeCanonRemoteRelease];
    }
  }
  return delay;
}

-(void)stopExposure {
  //self->doImageDownload = false;
  if ([self operationIsSupported:PTPRequestCodeCanonRemoteReleaseOff]) {
    [self sendPTPRequest:PTPRequestCodeCanonRemoteReleaseOff param1:3];
  } else {
    [self sendPTPRequest:PTPRequestCodeCanonBulbEnd];
  }
}

-(void)startAutofocus {
  [self sendPTPRequest:PTPRequestCodeCanonDoAf];
}

-(void)stopAutofocus {
  [self sendPTPRequest:PTPRequestCodeCanonAfCancel];
}

-(void)focus:(int)steps {
  if (steps != 0 && !inPreview) {
    tempLiveView = true;
    [self setProperty:PTPPropertyCodeCanonEVFMode value:@"1"];
    [self setProperty:PTPPropertyCodeCanonEVFOutputDevice value:@"2"];
  }
  if (steps == 0) {
    focusSteps = 0;
  } else if (steps > 0) {
    focusSteps = steps - 1;
    [self sendPTPRequest:PTPRequestCodeCanonDriveLens param1:1];
  } else if (steps < 0) {
    focusSteps = steps + 1;
    [self sendPTPRequest:PTPRequestCodeCanonDriveLens param1:0x8001];
  }
}

@end
