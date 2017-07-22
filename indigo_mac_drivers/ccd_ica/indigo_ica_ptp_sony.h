//
//  indigo_ica_sony.h
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "indigo_ica_ptp.h"

enum PTPSonyOperationCodeEnum {
  PTPRequestCodeSonySDIOConnect = 0x9201,
  PTPRequestCodeSonyGetSDIOGetExtDeviceInfo = 0x9202,
  PTPRequestCodeSonyGetDevicePropDesc = 0x9203,
  PTPRequestCodeSonyGetDevicePropertyValue = 0x9204,
  PTPRequestCodeSonySetControlDeviceA = 0x9205,
  PTPRequestCodeSonyGetControlDeviceDesc = 0x9206,
  PTPRequestCodeSonySetControlDeviceB = 0x9207,
  PTPRequestCodeSonyGetAllDevicePropData = 0x9209,
};

enum PTPSonyEventCodeEnum {
  PTPEventCodeSonyObjectAdded = 0xC201,
  PTPEventCodeSonyObjectRemoved = 0xC202,
  PTPEventCodeSonyPropertyChanged = 0xC203,
};

enum PTPSonyPropertyCode {
  PTPPropertyCodeSonyDPCCompensation = 0xD200,
  PTPPropertyCodeSonyDRangeOptimize = 0xD201,
  PTPPropertyCodeSonyImageSize = 0xD203,
  PTPPropertyCodeSonyShutterSpeed = 0xD20D,
  PTPPropertyCodeSonyColorTemp = 0xD20F,
  PTPPropertyCodeSonyCCFilter = 0xD210,
  PTPPropertyCodeSonyAspectRatio = 0xD211,
  PTPPropertyCodeSonyExposeIndex = 0xD216,
  PTPPropertyCodeSonyBatteryLevel = 0xD218,
  PTPPropertyCodeSonyPictureEffect = 0xD21B,
  PTPPropertyCodeSonyABFilter = 0xD21C,
  PTPPropertyCodeSonyISO = 0xD21E,
  PTPPropertyCodeSonyMovie = 0xD2C8,
  PTPPropertyCodeSonyStillImage = 0xD2C7,
};

@interface PTPSonyRequest : PTPRequest
+(NSString *)operationCodeName:(PTPRequestCode)operationCode;
@end

@interface PTPSonyResponse : PTPResponse
+(NSString *)responseCodeName:(PTPResponseCode)responseCode;
@end

@interface PTPSonyEvent : PTPEvent
+(NSString *)eventCodeName:(PTPEventCode)eventCode;
@end

@interface PTPSonyProperty : PTPProperty
+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode;
@end

@interface PTPSonyDeviceInfo : PTPDeviceInfo
@end

@interface PTPSonyCamera : PTPCamera
@end
