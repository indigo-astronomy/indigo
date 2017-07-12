//
//  indigo_ica_canon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include "indigo_bus.h"

#import "indigo_ica_ptp_canon.h"

static NSString *ptpReadCanonImageFormat(unsigned char** buf) {
  NSMutableString *result;
  unsigned int count = ptpReadUnsignedInt(buf);
  ptpReadUnsignedInt(buf);
  ptpReadUnsignedInt(buf);
  unsigned int quality = ptpReadUnsignedInt(buf);
  unsigned int compression = ptpReadUnsignedInt(buf);
  if (compression == 4)
    result = [NSMutableString stringWithString:@"RAW"];
  else if (quality == 0)
    result = [NSMutableString stringWithFormat:@"Large %@JPEG",( compression == 2 ? @"" : @"fine ")];
  else if (quality == 1)
    result = [NSMutableString stringWithFormat:@"Medium %@JPEG",( compression == 2 ? @"" : @"fine ")];
  else if (quality == 2)
    result = [NSMutableString stringWithFormat:@"Small %@JPEG",( compression == 2 ? @"" : @"fine ")];
  else if (quality == 14)
    result = [NSMutableString stringWithFormat:@"S1 %@JPEG",( compression == 2 ? @"" : @"fine ")];
  else if (quality == 15)
    result = [NSMutableString stringWithFormat:@"S2 %@JPEG",( compression == 2 ? @"" : @"fine ")];
  else if (quality == 16)
    result = [NSMutableString stringWithFormat:@"S3 %@JPEG",( compression == 2 ? @"" : @"fine ")];
  if (count == 2) {
    ptpReadUnsignedInt(buf);
    ptpReadUnsignedInt(buf);
    quality = ptpReadUnsignedInt(buf);
    compression = ptpReadUnsignedInt(buf);
    if (compression == 4)
      [result appendString:@" + RAW"];
    else if (quality == 0)
      [result appendFormat:@" + Large %@JPEG",( compression == 2 ? @"" : @"fine ")];
    else if (quality == 1)
      [result appendFormat:@" + Medium %@JPEG",( compression == 2 ? @"" : @"fine ")];
    else if (quality == 2)
      [result appendFormat:@" + Small %@JPEG",( compression == 2 ? @"" : @"fine ")];
    else if (quality == 14)
      [result appendFormat:@" + S1 %@JPEG",( compression == 2 ? @"" : @"fine ")];
    else if (quality == 15)
      [result appendFormat:@" + S2 %@JPEG",( compression == 2 ? @"" : @"fine ")];
    else if (quality == 16)
      [result appendFormat:@" + S3 %@JPEG",( compression == 2 ? @"" : @"fine ")];
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

-(void)checkForEvent {
  if ([self.info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPRequestCodeCanonGetEvent]]) {
    [self sendPTPRequest:PTPRequestCodeCanonGetEvent];
  }
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

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        self.info = [[self.deviceInfoClass alloc] initWithData:data];
        if ([self.info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPRequestCodeInitiateCapture]]) {
          [self.delegate cameraCanCapture:self];
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
      [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      break;
    }
    case PTPRequestCodeCanonGetEvent: {
      //NSLog(@"%@", data);
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
              case PTPPropertyCodeCanonFocusMode:
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
                property.type = PTPDataTypeCodeUInt16;
                break;
              case PTPPropertyCodeCanonAutoExposureMode:
                property.type = PTPDataTypeCodeUInt16;
                property.supportedValues = @[];
                break;
              case PTPPropertyCodeCanonWhiteBalanceAdjustA:
              case PTPPropertyCodeCanonWhiteBalanceAdjustB:
                property.type = PTPDataTypeCodeSInt16;
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
                property.type = PTPDataTypeCodeUInt32;
                break;
              case PTPPropertyCodeCanonOwner:
              case PTPPropertyCodeCanonArtist:
              case PTPPropertyCodeCanonCopyright:
              case PTPPropertyCodeCanonSerialNumber:
              case PTPPropertyCodeCanonLensName:
                property.type = PTPDataTypeCodeUnicodeString;
                break;
            }
            switch (property.type) {
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
              case PTPDataTypeCodeUnicodeString:
                property.defaultValue = property.value = [NSString stringWithCString:(char *)buf encoding:NSASCIIStringEncoding];
                if (property.value == nil)
                  property.value = @"";
                break;
            }
            if (code == PTPPropertyCodeCanonImageFormat || code == PTPPropertyCodeCanonImageFormatCF || code == PTPPropertyCodeCanonImageFormatSD || code == PTPPropertyCodeCanonImageFormatExtHD) {
              property.type = PTPDataTypeCodeUnicodeString;
              property.defaultValue = property.value = ptpReadCanonImageFormat(&buf);
            }
            if (property.type != PTPDataTypeCodeUndefined) {
              self.info.properties[[NSNumber numberWithUnsignedShort:code]] = property;
              [properties addObject:property];
            }
            NSLog(@"PTPEventCodeCanonPropValueChanged %@", property);
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
                [values addObject:ptpReadCanonImageFormat(&buf)];
            } else {
              for (int i = 0; i < count; i++)
                [values addObject:[NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(&buf)]];
            }
            property.supportedValues = values;
            [properties addObject:property];
            NSLog(@"PTPEventCodeCanonAvailListChanged %@", property);
            break;
          }
          default:
            NSLog(@"size %d type 0x%04x", size, type);
            break;
        }
        buf = record + size;
      }
      for (PTPProperty *property in properties) {
        switch (property.propertyCode) {
          case PTPPropertyCodeCanonAperture: {
            NSDictionary *map = @{ @0x08: @"f/1", @0x0B: @"f/1.1", @0x0C: @"f/1.2", @0x0D: @"f/1.2*", @0x10: @"f/1.4", @0x13: @"f/1.6", @0x14: @"f/1.8", @0x15: @"f/1.8*", @0x18: @"f/2", @0x1B: @"f/2.2", @0x1C: @"f/2.5", @0x1D: @"f/2.5*", @0x20: @"f/2.8", @0x23: @"f/3.2", @0x24: @"f/3.5", @0x25: @"f/3.5*", @0x28: @"f/4", @0x2B: @"f/4.5", @0x2C: @"f/4.5", @0x2D: @"f/5.0", @0x30: @"f/5.6", @0x33: @"f/6.3", @0x34: @"f/6.7", @0x35: @"f/7.1", @0x38: @"f/8", @0x3B: @"f/9", @0x3C: @"f/9.5", @0x3D: @"f/10", @0x40: @"f/11", @0x43: @"f/13*", @0x44: @"f/13", @0x45: @"f/14", @0x48: @"f/16", @0x4B: @"f/18", @0x4C: @"f/19", @0x4D: @"f/20", @0x50: @"f/22", @0x53: @"f/25", @0x54: @"f/27", @0x55: @"f/29", @0x58: @"f/32", @0x5B: @"f/36", @0x5C: @"f/38", @0x5D: @"f/40", @0x60: @"f/45", @0x63: @"f/51", @0x64: @"f/54", @0x65: @"f/57", @0x68: @"f/64", @0x6B: @"f/72", @0x6C: @"f/76", @0x6D: @"f/80", @0x70: @"f/91" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonShutterSpeed: {
            NSDictionary *map = @{ @0x0C: @"Bulb", @0x10: @"30s", @0x13: @"25s", @0x14: @"20s", @0x15: @"20*s", @0x18: @"15s", @0x1B: @"13s", @0x1C: @"10s", @0x1D: @"10*s", @0x20: @"8s", @0x23: @"6*s", @0x24: @"6s", @0x25: @"5s", @0x28: @"4s", @0x2B: @"3.2s", @0x2C: @"3s", @0x2D: @"2.5s", @0x30: @"2s", @0x33: @"1.6s", @0x34: @"15s", @0x35: @"1.3s", @0x38: @"1s", @0x3B: @"0.8s", @0x3C: @"0.7s", @0x3D: @"0.6s", @0x40: @"0.5s", @0x43: @"0.4s", @0x44: @"0.3s", @0x45: @"0.3*s", @0x48: @"1/4s", @0x4B: @"1/5s", @0x4C: @"1/6s", @0x4D: @"1/6*s", @0x50: @"1/8s", @0x53: @"1/10*s", @0x54: @"1/10s", @0x55: @"1/13s", @0x58: @"1/15s", @0x5B: @"1/20*s", @0x5C: @"1/20s", @0x5D: @"1/25s", @0x60: @"1/30s", @0x63: @"1/40s", @0x64: @"1/45s", @0x65: @"1/50s", @0x68: @"1/60s", @0x6B: @"1/80s", @0x6C: @"1/90s", @0x6D: @"1/100s", @0x70: @"1/125s", @0x73: @"1/160s", @0x74: @"1/180s", @0x75: @"1/200s", @0x78: @"1/250s", @0x7B: @"1/320s", @0x7C: @"1/350s", @0x7D: @"1/400s", @0x80: @"1/500s", @0x83: @"1/640s", @0x84: @"1/750s", @0x85: @"1/800s", @0x88: @"1/1000s", @0x8B: @"1/1250s", @0x8C: @"1/1500s", @0x8D: @"1/1600s", @0x90: @"1/2000s", @0x93: @"1/2500s", @0x94: @"1/3000s", @0x95: @"1/3200s", @0x98: @"1/4000s", @0x9B: @"1/5000s", @0x9C: @"1/6000s", @0x9D: @"1/6400s", @0xA0: @"1/8000s" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonISOSpeed: {
            NSDictionary *map = @{ @0x00: @"Auto", @0x40: @"50", @0x48: @"100", @0x4b: @"125", @0x4d: @"160", @0x50: @"200", @0x53: @"250", @0x55: @"320", @0x58: @"400", @0x5b: @"500", @0x5d: @"640", @0x60: @"800", @0x63: @"1000", @0x65: @"1250", @0x68: @"1600", @0x6b: @"2000", @0x6d: @"2500", @0x70: @"3200", @0x73: @"4000", @0x75: @"5000", @0x78: @"6400", @0x7b: @"8000", @0x7d: @"10000", @0x80: @"12800", @0x88: @"25600", @0x90: @"51200", @0x98: @"102400" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonExpCompensation: {
            NSDictionary *map = @{ @0x18: @"+3", @0x15: @"+2 2/3", @0x14: @"+2 1/2", @0x13: @"+2 1/3", @0x10: @"+2", @0x0D: @"+1 2/3", @0x0C: @"+1 1/2", @0x0B: @"+1 1/3", @0x08: @"+1", @0x05: @"+2/3", @0x04: @"+1/2", @0x03: @"+1/3", @0x00: @"0", @0xFD: @"–1/3", @0xFC: @"–1/2", @0xFB: @"–2/3", @0xF8: @"–1", @0xF5: @"–1 1/3", @0xF4: @"–1 1/2", @0xF3: @"–1 2/3", @0xF0: @"–2", @0xED: @"–2 1/3", @0xEC: @"–2 1/2", @0xEB: @"–2 2/3", @0xE8: @"–3" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonMeteringMode: {
            NSDictionary *map = @{ @1: @"Spot", @3: @"Evaluative", @4: @"Partial", @5: @"Center-weighted" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCanonDriveMode: {
            NSDictionary *map = @{ @0: @"Program AE", @1: @"Shutter Priority AE", @2: @"Aperture Priority AE", @3: @"Manual Exposure", @4: @"Bulb", @5: @"Auto DEP AE", @6: @"DEP AE", @8: @"Lock", @9: @"Auto", @10: @"Night Scene Portrait", @11: @"Sports", @12: @"Portrait", @13: @"Landscape", @14: @"Close-Up", @15: @"Flash Off", @19: @"Creative Auto", @22: @"Scene Intelligent Auto" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
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
    default: {
      [super processRequest:request Response:response inData:data];
    }
  }
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  // TBD
}

-(void)getLiveViewImage {
  // TBD
}

-(void)lock {
  [self sendPTPRequest:PTPRequestCodeCanonSetUILock];
}

-(void)unlock {
  [self sendPTPRequest:PTPRequestCodeCanonResetUILock];
}

-(void)startLiveViewZoom:(int)zoom x:(int)x y:(int)y {
  // TBD
}

-(void)stopLiveView {
  [super stopLiveView];
  // TBD
}

-(void)startCapture {
  // TBD
  [super startCapture];
}

-(void)stopCapture {
  // TBD
}

-(void)focus:(int)steps {
  // TBD
}

@end
