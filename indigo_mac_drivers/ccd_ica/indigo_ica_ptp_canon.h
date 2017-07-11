//
//  indigo_ica_canon.h
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "indigo_ica_ptp.h"

enum PTPCanonOperationCodeEnum {
  PTPRequestCodeCanonGetStorageIDs = 0x9101,
  PTPRequestCodeCanonGetStorageInfo = 0x9102,
  PTPRequestCodeCanonGetObjectInfo = 0x9103,
  PTPRequestCodeCanonGetObject = 0x9104,
  PTPRequestCodeCanonDeleteObject = 0x9105,
  PTPRequestCodeCanonFormatStore = 0x9106,
  PTPRequestCodeCanonGetPartialObject = 0x9107,
  PTPRequestCodeCanonGetDeviceInfoEx = 0x9108,
  PTPRequestCodeCanonGetObjectInfoEx = 0x9109,
  PTPRequestCodeCanonGetThumbEx = 0x910A,
  PTPRequestCodeCanonSendPartialObject = 0x910B,
  PTPRequestCodeCanonSetObjectAttributes = 0x910C,
  PTPRequestCodeCanonGetObjectTime = 0x910D,
  PTPRequestCodeCanonSetObjectTime = 0x910E,
  PTPRequestCodeCanonRemoteRelease = 0x910F,
  PTPRequestCodeCanonSetDevicePropValueEx = 0x9110,
  PTPRequestCodeCanonGetRemoteMode = 0x9113,
  PTPRequestCodeCanonSetRemoteMode = 0x9114,
  PTPRequestCodeCanonSetEventMode = 0x9115,
  PTPRequestCodeCanonGetEvent = 0x9116,
  PTPRequestCodeCanonTransferComplete = 0x9117,
  PTPRequestCodeCanonCancelTransfer = 0x9118,
  PTPRequestCodeCanonResetTransfer = 0x9119,
  PTPRequestCodeCanonPCHDDCapacity = 0x911A,
  PTPRequestCodeCanonSetUILock = 0x911B,
  PTPRequestCodeCanonResetUILock = 0x911C,
  PTPRequestCodeCanonKeepDeviceOn = 0x911D,
  PTPRequestCodeCanonSetNullPacketMode = 0x911E,
  PTPRequestCodeCanonUpdateFirmware = 0x911F,
  PTPRequestCodeCanonTransferCompleteDT = 0x9120,
  PTPRequestCodeCanonCancelTransferDT = 0x9121,
  PTPRequestCodeCanonSetWftProfile = 0x9122,
  PTPRequestCodeCanonGetWftProfile = 0x9123,
  PTPRequestCodeCanonSetProfileToWft = 0x9124,
  PTPRequestCodeCanonBulbStart = 0x9125,
  PTPRequestCodeCanonBulbEnd = 0x9126,
  PTPRequestCodeCanonRequestDevicePropValue = 0x9127,
  PTPRequestCodeCanonRemoteReleaseOn = 0x9128,
  PTPRequestCodeCanonRemoteReleaseOff = 0x9129,
  PTPRequestCodeCanonRegistBackgroundImage = 0x912A,
  PTPRequestCodeCanonChangePhotoStudioMode = 0x912B,
  PTPRequestCodeCanonGetPartialObjectEx = 0x912C,
  PTPRequestCodeCanonResetMirrorLockupState = 0x9130,
  PTPRequestCodeCanonPopupBuiltinFlash = 0x9131,
  PTPRequestCodeCanonEndGetPartialObjectEx = 0x9132,
  PTPRequestCodeCanonMovieSelectSWOn = 0x9133,
  PTPRequestCodeCanonMovieSelectSWOff = 0x9134,
  PTPRequestCodeCanonGetCTGInfo = 0x9135,
  PTPRequestCodeCanonGetLensAdjust = 0x9136,
  PTPRequestCodeCanonSetLensAdjust = 0x9137,
  PTPRequestCodeCanonGetMusicInfo = 0x9138,
  PTPRequestCodeCanonCreateHandle = 0x9139,
  PTPRequestCodeCanonSendPartialObjectEx = 0x913A,
  PTPRequestCodeCanonEndSendPartialObjectEx = 0x913B,
  PTPRequestCodeCanonSetCTGInfo = 0x913C,
  PTPRequestCodeCanonSetRequestOLCInfoGroup = 0x913D,
  PTPRequestCodeCanonSetRequestRollingPitchingLevel = 0x913E,
  PTPRequestCodeCanonGetCameraSupport = 0x913F,
  PTPRequestCodeCanonSetRating = 0x9140,
  PTPRequestCodeCanonRequestInnerDevelopStart = 0x9141,
  PTPRequestCodeCanonRequestInnerDevelopParamChange = 0x9142,
  PTPRequestCodeCanonRequestInnerDevelopEnd = 0x9143,
  PTPRequestCodeCanonGpsLoggingDataMode = 0x9144,
  PTPRequestCodeCanonGetGpsLogCurrentHandle = 0x9145,
  PTPRequestCodeCanonInitiateViewfinder = 0x9151,
  PTPRequestCodeCanonTerminateViewfinder = 0x9152,
  PTPRequestCodeCanonGetViewFinderData = 0x9153,
  PTPRequestCodeCanonDoAf = 0x9154,
  PTPRequestCodeCanonDriveLens = 0x9155,
  PTPRequestCodeCanonDepthOfFieldPreview = 0x9156,
  PTPRequestCodeCanonClickWB = 0x9157,
  PTPRequestCodeCanonZoom = 0x9158,
  PTPRequestCodeCanonZoomPosition = 0x9159,
  PTPRequestCodeCanonSetLiveAfFrame = 0x915A,
  PTPRequestCodeCanonTouchAfPosition = 0x915B,
  PTPRequestCodeCanonSetLvPcFlavoreditMode = 0x915C,
  PTPRequestCodeCanonSetLvPcFlavoreditParam = 0x915D,
  PTPRequestCodeCanonAfCancel = 0x9160,
  PTPRequestCodeCanonSetDefaultCameraSetting = 0x91BE,
  PTPRequestCodeCanonGetAEData = 0x91BF,
  PTPRequestCodeCanonNotifyNetworkError = 0x91E8,
  PTPRequestCodeCanonAdapterTransferProgress = 0x91E9,
  PTPRequestCodeCanonTransferComplete2 = 0x91F0,
  PTPRequestCodeCanonCancelTransfer2 = 0x91F1,
  PTPRequestCodeCanonFAPIMessageTX = 0x91FE,
  PTPRequestCodeCanonFAPIMessageRX = 0x91FF
};

enum PTPCanonResponseCodeEnum {
  PTPResponseCodeUnknownCommand = 0xA001,
  PTPResponseCodeOperationRefused = 0xA005,
  PTPResponseCodeLensCover = 0xA006,
  PTPResponseCodeBatteryLow = 0xA101,
  PTPResponseCodeNotReady = 0xA102
};

enum PTPCanonEventCodeEnum {
  PTPEventCodeCanonRequestGetEvent = 0xC101,
  PTPEventCodeCanonObjectAddedEx = 0xC181,
  PTPEventCodeCanonObjectRemoved = 0xC182,
  PTPEventCodeCanonRequestGetObjectInfoEx = 0xC183,
  PTPEventCodeCanonStorageStatusChanged = 0xC184,
  PTPEventCodeCanonStorageInfoChanged = 0xC185,
  PTPEventCodeCanonRequestObjectTransfer = 0xC186,
  PTPEventCodeCanonObjectInfoChangedEx = 0xC187,
  PTPEventCodeCanonObjectContentChanged = 0xC188,
  PTPEventCodeCanonPropValueChanged = 0xC189,
  PTPEventCodeCanonAvailListChanged = 0xC18A,
  PTPEventCodeCanonCameraStatusChanged = 0xC18B,
  PTPEventCodeCanonWillSoonShutdown = 0xC18D,
  PTPEventCodeCanonShutdownTimerUpdated = 0xC18E,
  PTPEventCodeCanonRequestCancelTransfer = 0xC18F,
  PTPEventCodeCanonRequestObjectTransferDT = 0xC190,
  PTPEventCodeCanonRequestCancelTransferDT = 0xC191,
  PTPEventCodeCanonStoreAdded = 0xC192,
  PTPEventCodeCanonStoreRemoved = 0xC193,
  PTPEventCodeCanonBulbExposureTime = 0xC194,
  PTPEventCodeCanonRecordingTime = 0xC195,
  PTPEventCodeCanonRequestObjectTransferTS = 0xC1A2,
  PTPEventCodeCanonAfResult = 0xC1A3,
  PTPEventCodeCanonCTGInfoCheckComplete = 0xC1A4,
  PTPEventCodeCanonOLCInfoChanged = 0xC1A5,
  PTPEventCodeCanonRequestObjectTransferFTP = 0xC1F1
};

@interface PTPCanonRequest : PTPRequest
+(NSString *)operationCodeName:(PTPRequestCode)operationCode;
@end

@interface PTPCanonResponse : PTPResponse
+(NSString *)responseCodeName:(PTPResponseCode)responseCode;
@end

@interface PTPCanonEvent : PTPEvent
+(NSString *)eventCodeName:(PTPEventCode)eventCode;
@end


@interface PTPCanonDeviceInfo : PTPDeviceInfo
@end

@interface PTPCanonCamera : PTPCamera
@end
