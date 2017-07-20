//
//  indigo_ica_sony.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include "indigo_bus.h"

#import "indigo_ica_ptp_sony.h"

static PTPSonyProperty *ptpReadSonyProperty(unsigned char** buf) {
  unsigned int code = ptpReadUnsignedShort(buf);
  PTPSonyProperty *property = [[PTPSonyProperty alloc] initWithCode:code];
  property.type = ptpReadUnsignedShort(buf);
  property.readOnly = !ptpReadUnsignedChar(buf);
  ptpReadUnsignedChar(buf);
  property.defaultValue = ptpReadValue(property.type, buf);
  property.value = ptpReadValue(property.type, buf);

  int form = ptpReadUnsignedChar(buf);
  switch (form) {
    case 1: {
      property.min = (NSNumber *)ptpReadValue(property.type, buf);
      property.max = (NSNumber *)ptpReadValue(property.type, buf);
      property.step = (NSNumber *)ptpReadValue(property.type, buf);
      break;
    }
    case 2: {
      int count = ptpReadUnsignedShort(buf);
      NSMutableArray<NSObject*> *values = [NSMutableArray arrayWithCapacity:count];
      for (int i = 0; i < count; i++)
        [values addObject:ptpReadValue(property.type, buf)];
      property.supportedValues = values;
      break;
    }
  }
  
  return property;
}


@implementation PTPSonyRequest

+(NSString *)operationCodeName:(PTPRequestCode)operationCode {
  switch (operationCode) {
    case PTPRequestCodeSonySDIOConnect: return @"PTPRequestCodeSonySDIOConnect";
    case PTPRequestCodeSonyGetSDIOGetExtDeviceInfo: return @"PTPRequestCodeSonyGetSDIOGetExtDeviceInfo";
    case PTPRequestCodeSonyGetDevicePropDesc: return @"PTPRequestCodeSonyGetDevicePropDesc";
    case PTPRequestCodeSonyGetDevicePropertyValue: return @"PTPRequestCodeSonyGetDevicePropertyValue";
    case PTPRequestCodeSonySetControlDeviceA: return @"PTPRequestCodeSonySetControlDeviceA";
    case PTPRequestCodeSonyGetControlDeviceDesc: return @"PTPRequestCodeSonyGetControlDeviceDesc";
    case PTPRequestCodeSonySetControlDeviceB: return @"PTPRequestCodeSonySetControlDeviceB";
    case PTPRequestCodeSonyGetAllDevicePropData: return @"PTPRequestCodeSonyGetAllDevicePropData";
  }
  return [PTPRequest operationCodeName:operationCode];
}

-(NSString *)operationCodeName {
  return [PTPSonyRequest operationCodeName:self.operationCode];
}

@end

@implementation PTPSonyResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode {
  return [PTPResponse responseCodeName:responseCode];
}

-(NSString *)responseCodeName {
  return [PTPSonyResponse responseCodeName:self.responseCode];
}

@end

@implementation PTPSonyEvent

+(NSString *)eventCodeName:(PTPEventCode)eventCode {
  switch (eventCode) {
    case PTPEventCodeSonyObjectAdded: return @"PTPEventCodeSonyObjectAdded";
    case PTPEventCodeSonyObjectRemoved: return @"PTPEventCodeSonyObjectRemoved";
    case PTPEventCodeSonyPropertyChanged: return @"PTPEventCodeSonyPropertyChanged";
  }
  return [PTPEvent eventCodeName:eventCode];
}

-(NSString *)eventCodeName {
  return [PTPSonyEvent eventCodeName:self.eventCode];
}

@end

@implementation PTPSonyProperty : PTPProperty

+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode {
  switch (propertyCode) {
    case PTPPropertyCodeSonyDPCCompensation: return @"PTPPropertyCodeSonyDPCCompensation";
    case PTPPropertyCodeSonyDRangeOptimize: return @"PTPPropertyCodeSonyDRangeOptimize";
    case PTPPropertyCodeSonyImageSize: return @"PTPPropertyCodeSonyImageSize";
    case PTPPropertyCodeSonyShutterSpeed: return @"PTPPropertyCodeSonyShutterSpeed";
    case PTPPropertyCodeSonyColorTemp: return @"PTPPropertyCodeSonyColorTemp";
    case PTPPropertyCodeSonyCCFilter: return @"PTPPropertyCodeSonyCCFilter";
    case PTPPropertyCodeSonyAspectRatio: return @"PTPPropertyCodeSonyAspectRatio";
    case PTPPropertyCodeSonyExposeIndex: return @"PTPPropertyCodeSonyExposeIndex";
    case PTPPropertyCodeSonyPictureEffect: return @"PTPPropertyCodeSonyPictureEffect";
    case PTPPropertyCodeSonyABFilter: return @"PTPPropertyCodeSonyABFilter";
    case PTPPropertyCodeSonyISO: return @"PTPPropertyCodeSonyISO";
    case PTPPropertyCodeSonyMovie: return @"PTPPropertyCodeSonyMovie";
    case PTPPropertyCodeSonyStillImage: return @"PTPPropertyCodeSonyStillImage";
  }
  return [PTPProperty propertyCodeName:propertyCode];
}

-(NSString *)propertyCodeName {
  return [PTPSonyProperty propertyCodeName:self.propertyCode];
}

@end


@implementation PTPSonyDeviceInfo

-(NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %@ V%.2f\n", self.model, self.version, self.standardVersion / 100.0, self.vendorExtensionDesc, self.vendorExtensionVersion / 100.0];
  if (self.operationsSupported.count > 0) {
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPSonyRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPSonyEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
    [s appendFormat:@"\nProperties:\n"];
    for (NSNumber *code in self.propertiesSupported) {
      PTPProperty *property = self.properties[code];
      if (property)
        [s appendFormat:@"%@\n", property];
      else
        [s appendFormat:@"%@\n", [PTPSonyProperty propertyCodeName:code.intValue]];
    }
  }
  return s;
}
@end

@implementation PTPSonyCamera {
}

-(NSString *)name {
  return [NSString stringWithFormat:@"Sony %@", super.name];
}

-(PTPVendorExtension) extension {
  return PTPVendorExtensionSony;
}

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {
  }
  return self;
}

-(void)didRemoveDevice:(ICDevice *)device {
  [super didRemoveDevice:device];
}


-(Class)requestClass {
  return PTPSonyRequest.class;
}

-(Class)responseClass {
  return PTPSonyResponse.class;
}

-(Class)eventClass {
  return PTPSonyEvent.class;
}

-(Class)propertyClass {
  return PTPSonyProperty.class;
}

-(Class)deviceInfoClass {
  return PTPSonyDeviceInfo.class;
}

-(void)checkForEvent {
}

-(void)processEvent:(PTPEvent *)event {
  switch (event.eventCode) {
      
      
    default: {
      [super processEvent:event];
      break;
    }
  }
}

-(void)processPropertyDescription:(PTPProperty *)property {
  switch (property.propertyCode) {
      
      
     default: {
      [super processPropertyDescription:property];
      break;
    }
  }
}

-(void)processConnect {

  [super processConnect];
}

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        self.info = [[self.deviceInfoClass alloc] initWithData:data];
        for (NSNumber *code in self.info.propertiesSupported)
          [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
        if ([self.info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPRequestCodeSonyGetSDIOGetExtDeviceInfo]]) {
          [self sendPTPRequest:PTPRequestCodeSonySDIOConnect param1:1 param2:0 param3:0];
          [self sendPTPRequest:PTPRequestCodeSonySDIOConnect param1:2 param2:0 param3:0];
          [self sendPTPRequest:PTPRequestCodeSonyGetSDIOGetExtDeviceInfo param1:0xC8];
        } else {
          [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
        }
      }
      break;
    }
    case PTPRequestCodeSonyGetSDIOGetExtDeviceInfo: {
      NSLog(@"%@", data);
      unsigned short *codes = (unsigned short *)data.bytes;
      long count = data.length / 2;
      if (self.info.operationsSupported == nil)
        self.info.operationsSupported = [NSMutableArray array];
      if (self.info.eventsSupported == nil)
        self.info.eventsSupported = [NSMutableArray array];
      if (self.info.propertiesSupported == nil)
        self.info.propertiesSupported = [NSMutableArray array];
      for (int i = 0; i < count; i++) {
        unsigned short code = codes[i];
        if ((code & 0x7000) == 0x1000) {
          [(NSMutableArray *)self.info.operationsSupported addObject:[NSNumber numberWithUnsignedShort:code]];
        } else if ((code & 0x7000) == 0x4000) {
          [(NSMutableArray *)self.info.eventsSupported addObject:[NSNumber numberWithUnsignedShort:code]];
        } else if ((code & 0x7000) == 0x5000) {
          [(NSMutableArray *)self.info.propertiesSupported addObject:[NSNumber numberWithUnsignedShort:code]];
        }
      }
      [self sendPTPRequest:PTPRequestCodeSonySDIOConnect param1:3 param2:0 param3:0];
      [self sendPTPRequest:PTPRequestCodeSonyGetAllDevicePropData];
      break;
    }
    case PTPRequestCodeSonyGetDevicePropDesc: {
      NSLog(@"%@", data);
      break;
    }
    case PTPRequestCodeSonyGetAllDevicePropData: {
      NSLog(@"%@", data);
      unsigned char* buffer = (unsigned char*)data.bytes;
      unsigned char* buf = buffer;
      unsigned int count = ptpReadUnsignedInt(&buf);
      ptpReadUnsignedInt(&buf);
      for (int i = 0; i < count; i++) {
        PTPProperty *property = ptpReadSonyProperty(&buf);
        self.info.properties[[NSNumber numberWithUnsignedShort:property.propertyCode]] = property;
        [self processPropertyDescription:property];
      }
      [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      break;
    }
    default: {
      [super processRequest:request Response:response inData:data];
      break;
    }
  }
}

-(void)getPreviewImage {
}

-(void)lock {
}

-(void)unlock {
}

-(void)startPreviewZoom:(int)zoom x:(int)x y:(int)y {

}

-(void)stopPreview {
}

-(void)startExposureWithMirrorLockup:(BOOL)mirrorLockup avoidAF:(BOOL)avoidAF {
}

-(void)stopExposure {
}

-(void)focus:(int)steps {
}

@end
