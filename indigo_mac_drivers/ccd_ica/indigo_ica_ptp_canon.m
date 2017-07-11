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
        [s appendFormat:@"%@\n", [PTPProperty propertyCodeName:code.intValue vendorExtension:self.vendorExtension]];
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

-(PTPRequest *)allocRequest {
  return [PTPCanonRequest alloc];
}

-(PTPResponse *)allocResponse {
  return [PTPCanonResponse alloc];
}

-(PTPEvent *)allocEvent {
  return [PTPCanonEvent alloc];
}

-(PTPDeviceInfo *)allocDeviceInfo {
  return [PTPCanonDeviceInfo alloc];
}

@end
