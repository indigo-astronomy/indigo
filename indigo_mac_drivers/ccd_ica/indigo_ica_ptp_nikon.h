//
//  indigo_ica_nikon.h
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "indigo_ica_ptp.h"

enum PTPNikonOperationCodeEnum {
  PTPRequestCodeNikonGetProfileAllData = 0x9006,
  PTPRequestCodeNikonSendProfileData = 0x9007,
  PTPRequestCodeNikonDeleteProfile = 0x9008,
  PTPRequestCodeNikonSetProfileData = 0x9009,
  PTPRequestCodeNikonAdvancedTransfer = 0x9010,
  PTPRequestCodeNikonGetFileInfoInBlock = 0x9011,
  PTPRequestCodeNikonCapture = 0x90C0,
  PTPRequestCodeNikonAfDrive = 0x90C1,
  PTPRequestCodeNikonSetControlMode = 0x90C2,
  PTPRequestCodeNikonDelImageSDRAM = 0x90C3,
  PTPRequestCodeNikonGetLargeThumb = 0x90C4,
  PTPRequestCodeNikonCurveDownload = 0x90C5,
  PTPRequestCodeNikonCurveUpload = 0x90C6,
  PTPRequestCodeNikonCheckEvent = 0x90C7,
  PTPRequestCodeNikonDeviceReady = 0x90C8,
  PTPRequestCodeNikonSetPreWBData = 0x90C9,
  PTPRequestCodeNikonGetVendorPropCodes = 0x90CA,
  PTPRequestCodeNikonAfCaptureSDRAM = 0x90CB,
  PTPRequestCodeNikonGetPictCtrlData = 0x90CC,
  PTPRequestCodeNikonSetPictCtrlData = 0x90CD,
  PTPRequestCodeNikonDelCstPicCtrl = 0x90CE,
  PTPRequestCodeNikonGetPicCtrlCapability = 0x90CF,
  PTPRequestCodeNikonGetPreviewImg = 0x9200,
  PTPRequestCodeNikonStartLiveView = 0x9201,
  PTPRequestCodeNikonEndLiveView = 0x9202,
  PTPRequestCodeNikonGetLiveViewImg = 0x9203,
  PTPRequestCodeNikonMfDrive = 0x9204,
  PTPRequestCodeNikonChangeAfArea = 0x9205,
  PTPRequestCodeNikonAfDriveCancel = 0x9206,
  PTPRequestCodeNikonInitiateCaptureRecInMedia = 0x9207,
  PTPRequestCodeNikonGetVendorStorageIDs = 0x9209,
  PTPRequestCodeNikonStartMovieRecInCard = 0x920a,
  PTPRequestCodeNikonEndMovieRec = 0x920b,
  PTPRequestCodeNikonTerminateCapture = 0x920c,
  PTPRequestCodeNikonGetDevicePTPIPInfo = 0x90E0,
  PTPRequestCodeNikonGetPartialObjectHiSpeed = 0x9400,
};

enum PTPNikonResponseCodeEnum {
  PTPResponseCodeNikonHardwareError = 0xA001,
  PTPResponseCodeNikonOutOfFocus = 0xA002,
  PTPResponseCodeNikonChangeCameraModeFailed = 0xA003,
  PTPResponseCodeNikonInvalidStatus = 0xA004,
  PTPResponseCodeNikonSetPropertyNotSupported = 0xA005,
  PTPResponseCodeNikonWbResetError = 0xA006,
  PTPResponseCodeNikonDustReferenceError = 0xA007,
  PTPResponseCodeNikonShutterSpeedBulb = 0xA008,
  PTPResponseCodeNikonMirrorUpSequence = 0xA009,
  PTPResponseCodeNikonCameraModeNotAdjustFNumber = 0xA00A,
  PTPResponseCodeNikonNotLiveView = 0xA00B,
  PTPResponseCodeNikonMfDriveStepEnd = 0xA00C,
  PTPResponseCodeNikonMfDriveStepInsufficiency = 0xA00E,
  PTPResponseCodeNikonAdvancedTransferCancel = 0xA022,
};

enum PTPNikonEventCodeEnum {
  PTPEventCodeNikonObjectAddedInSDRAM = 0xC101,
  PTPEventCodeNikonCaptureCompleteRecInSdram = 0xC102,
  PTPEventCodeNikonAdvancedTransfer = 0xC103,
  PTPEventCodeNikonPreviewImageAdded = 0xC104,
};

@interface PTPNikonRequest : PTPRequest
+(NSString *)operationCodeName:(PTPRequestCode)operationCode;
@end

@interface PTPNikonResponse : PTPResponse
+(NSString *)responseCodeName:(PTPResponseCode)responseCode;
@end

@interface PTPNikonEvent : PTPEvent
+(NSString *)eventCodeName:(PTPEventCode)eventCode;
@end

@interface PTPNikonDeviceInfo : PTPDeviceInfo
@end

@interface PTPNikonCamera : PTPCamera
@end
