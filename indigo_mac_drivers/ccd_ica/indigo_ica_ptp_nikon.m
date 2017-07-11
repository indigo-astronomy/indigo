//
//  indigo_ica_nikon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import "indigo_ica_ptp_nikon.h"

@implementation PTPNikonRequest

+(NSString *)operationCodeName:(PTPRequestCode)operationCode {
  switch (operationCode) {
    case PTPRequestCodeNikonGetProfileAllData: return @"PTPRequestCodeNikonGetProfileAllData";
    case PTPRequestCodeNikonSendProfileData: return @"PTPRequestCodeNikonSendProfileData";
    case PTPRequestCodeNikonDeleteProfile: return @"PTPRequestCodeNikonDeleteProfile";
    case PTPRequestCodeNikonSetProfileData: return @"PTPRequestCodeNikonSetProfileData";
    case PTPRequestCodeNikonAdvancedTransfer: return @"PTPRequestCodeNikonAdvancedTransfer";
    case PTPRequestCodeNikonGetFileInfoInBlock: return @"PTPRequestCodeNikonGetFileInfoInBlock";
    case PTPRequestCodeNikonCapture: return @"PTPRequestCodeNikonCapture";
    case PTPRequestCodeNikonAfDrive: return @"PTPRequestCodeNikonAfDrive";
    case PTPRequestCodeNikonSetControlMode: return @"PTPRequestCodeNikonSetControlMode";
    case PTPRequestCodeNikonDelImageSDRAM: return @"PTPRequestCodeNikonDelImageSDRAM";
    case PTPRequestCodeNikonGetLargeThumb: return @"PTPRequestCodeNikonGetLargeThumb";
    case PTPRequestCodeNikonCurveDownload: return @"PTPRequestCodeNikonCurveDownload";
    case PTPRequestCodeNikonCurveUpload: return @"PTPRequestCodeNikonCurveUpload";
    case PTPRequestCodeNikonCheckEvent: return @"PTPRequestCodeNikonCheckEvent";
    case PTPRequestCodeNikonDeviceReady: return @"PTPRequestCodeNikonDeviceReady";
    case PTPRequestCodeNikonSetPreWBData: return @"PTPRequestCodeNikonSetPreWBData";
    case PTPRequestCodeNikonGetVendorPropCodes: return @"PTPRequestCodeNikonGetVendorPropCodes";
    case PTPRequestCodeNikonAfCaptureSDRAM: return @"PTPRequestCodeNikonAfCaptureSDRAM";
    case PTPRequestCodeNikonGetPictCtrlData: return @"PTPRequestCodeNikonGetPictCtrlData";
    case PTPRequestCodeNikonSetPictCtrlData: return @"PTPRequestCodeNikonSetPictCtrlData";
    case PTPRequestCodeNikonDelCstPicCtrl: return @"PTPRequestCodeNikonDelCstPicCtrl";
    case PTPRequestCodeNikonGetPicCtrlCapability: return @"PTPRequestCodeNikonGetPicCtrlCapability";
    case PTPRequestCodeNikonGetPreviewImg: return @"PTPRequestCodeNikonGetPreviewImg";
    case PTPRequestCodeNikonStartLiveView: return @"PTPRequestCodeNikonStartLiveView";
    case PTPRequestCodeNikonEndLiveView: return @"PTPRequestCodeNikonEndLiveView";
    case PTPRequestCodeNikonGetLiveViewImg: return @"PTPRequestCodeNikonGetLiveViewImg";
    case PTPRequestCodeNikonMfDrive: return @"PTPRequestCodeNikonMfDrive";
    case PTPRequestCodeNikonChangeAfArea: return @"PTPRequestCodeNikonChangeAfArea";
    case PTPRequestCodeNikonAfDriveCancel: return @"PTPRequestCodeNikonAfDriveCancel";
    case PTPRequestCodeNikonInitiateCaptureRecInMedia: return @"PTPRequestCodeNikonInitiateCaptureRecInMedia";
    case PTPRequestCodeNikonGetVendorStorageIDs: return @"PTPRequestCodeNikonGetVendorStorageIDs";
    case PTPRequestCodeNikonStartMovieRecInCard: return @"PTPRequestCodeNikonStartMovieRecInCard";
    case PTPRequestCodeNikonEndMovieRec: return @"PTPRequestCodeNikonEndMovieRec";
    case PTPRequestCodeNikonTerminateCapture: return @"PTPRequestCodeNikonTerminateCapture";
    case PTPRequestCodeNikonGetDevicePTPIPInfo: return @"PTPRequestCodeNikonGetDevicePTPIPInfo";
    case PTPRequestCodeNikonGetPartialObjectHiSpeed: return @"PTPRequestCodeNikonGetPartialObjectHiSpeed";
  }
  return [PTPRequest operationCodeName:operationCode];
}

-(NSString *)operationCodeName {
  return [PTPNikonRequest operationCodeName:self.operationCode];
}

@end

@implementation PTPNikonResponse : PTPResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode {
  switch (responseCode) {
    case PTPResponseCodeNikonHardwareError: return @"PTPResponseCodeNikonHardwareError";
    case PTPResponseCodeNikonOutOfFocus: return @"PTPResponseCodeNikonOutOfFocus";
    case PTPResponseCodeNikonChangeCameraModeFailed: return @"PTPResponseCodeNikonChangeCameraModeFailed";
    case PTPResponseCodeNikonInvalidStatus: return @"PTPResponseCodeNikonInvalidStatus";
    case PTPResponseCodeNikonSetPropertyNotSupported: return @"PTPResponseCodeNikonSetPropertyNotSupported";
    case PTPResponseCodeNikonWbResetError: return @"PTPResponseCodeNikonWbResetError";
    case PTPResponseCodeNikonDustReferenceError: return @"PTPResponseCodeNikonDustReferenceError";
    case PTPResponseCodeNikonShutterSpeedBulb: return @"PTPResponseCodeNikonShutterSpeedBulb";
    case PTPResponseCodeNikonMirrorUpSequence: return @"PTPResponseCodeNikonMirrorUpSequence";
    case PTPResponseCodeNikonCameraModeNotAdjustFNumber: return @"PTPResponseCodeNikonCameraModeNotAdjustFNumber";
    case PTPResponseCodeNikonNotLiveView: return @"PTPResponseCodeNikonNotLiveView";
    case PTPResponseCodeNikonMfDriveStepEnd: return @"PTPResponseCodeNikonMfDriveStepEnd";
    case PTPResponseCodeNikonMfDriveStepInsufficiency: return @"PTPResponseCodeNikonMfDriveStepInsufficiency";
    case PTPResponseCodeNikonAdvancedTransferCancel: return @"PTPResponseCodeNikonAdvancedTransferCancel";
  }
  return [PTPResponse responseCodeName:responseCode];
}

-(NSString *)responseCodeName {
  return [PTPNikonResponse responseCodeName:self.responseCode];
}

@end

@implementation PTPNikonEvent : PTPEvent

+(NSString *)eventCodeName:(PTPEventCode)eventCode {
  switch (eventCode) {
    case PTPEventCodeNikonObjectAddedInSDRAM: return @"PTPEventCodeNikonObjectAddedInSDRAM";
    case PTPEventCodeNikonCaptureCompleteRecInSdram: return @"PTPEventCodeNikonCaptureCompleteRecInSdram";
    case PTPEventCodeNikonAdvancedTransfer: return @"PTPEventCodeNikonAdvancedTransfer";
    case PTPEventCodeNikonPreviewImageAdded: return @"PTPEventCodeNikonPreviewImageAdded";
  }
  return [PTPEvent eventCodeName:eventCode];
}

-(NSString *)eventCodeName {
  return [PTPNikonEvent eventCodeName:self.eventCode];
}

@end

@implementation PTPNikonDeviceInfo : PTPDeviceInfo

-(NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %@ V%.2f\n", self.model, self.version, self.standardVersion / 100.0, self.vendorExtensionDesc, self.vendorExtensionVersion / 100.0];
  if (self.operationsSupported.count > 0) {
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPNikonRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPNikonEvent eventCodeName:code.intValue]];
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

@implementation PTPNikonCamera

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {

  }
  return self;
}

-(PTPRequest *)allocRequest {
  return [PTPNikonRequest alloc];
}

-(PTPResponse *)allocResponse {
  return [PTPNikonResponse alloc];
}


-(PTPEvent *)allocEvent {
  return [PTPNikonEvent alloc];
}

-(PTPDeviceInfo *)allocDeviceInfo {
  return [PTPNikonDeviceInfo alloc];
}

@end
