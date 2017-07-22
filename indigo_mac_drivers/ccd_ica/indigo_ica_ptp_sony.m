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
  property.readOnly = false; ptpReadUnsignedChar(buf); // property.readOnly = !ptpReadUnsignedChar(buf);
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
  BOOL initialized;
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
    initialized = false;
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
    case PTPEventCodeSonyPropertyChanged: {
      [self sendPTPRequest:PTPRequestCodeSonyGetAllDevicePropData];
      break;
    }
    default: {
      [super processEvent:event];
      break;
    }
  }
}

-(void)processPropertyDescription:(PTPProperty *)property {
  switch (property.propertyCode) {
    case PTPPropertyCodeExposureBiasCompensation: {
      NSDictionary *map = @{ @5000: @"+5", @4700: @"+4 2/3", @4500: @"+4 1/2", @4300: @"+4 1/3", @4000: @"+4", @3700: @"+3 2/3", @3500: @"+3 1/2", @3300: @"+3 1/3", @3000: @"+3", @2700: @"+2 2/3", @2500: @"+2 1/2", @2300: @"+2 1/3", @2000: @"+2", @1700: @"+1 2/3", @1500: @"+1 1/2", @1300: @"+1 1/3", @1000: @"+1", @700: @"+2/3", @500: @"+1/2", @300: @"+1/3", @0: @"0", @-300: @"-1/3", @-500: @"-1/2", @-700: @"-2/3", @-1000: @"-1", @-1300: @"-1 1/3", @-1500: @"-1 1/2", @-1700: @"-1 2/3", @-2000: @"-2", @-2300: @"-2 1/3", @-2500: @"-2 1/2", @-2700: @"-2 2/3", @-3000: @"-3", @-3300: @"-3 1/3", @-3500: @"-3 1/2", @-3700: @"-3 2/3", @-4000: @"-4", @-4300: @"-4 1/3", @-4500: @"-4 1/2", @-4700: @"-4 2/3", @-5000: @"-5" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeStillCaptureMode: {
      NSDictionary *map = @{ @1: @"Single shooting", @32787: @"Cont. shooting", @32788: @"Spd priority cont.", @32773: @"Self-timer 2s", @32772: @"Self-time 10s", @32776: @"Self-timer 10s 3x", @32777: @"Self-timer 10s 5x", @33591: @"Bracket 1/3EV 3x cont.", @33655: @"Bracket 2/3EV 3x cont.", @33553: @"Bracket 1EV 3x cont.", @33569: @"Bracket 2EV 3x cont.", @33585: @"Bracket 3EV 3x cont.", @33590: @"Bracket 1/3EV 3x", @33654: @"Bracket 2/3EV 3x", @33552: @"Bracket 1EV 3x", @33568: @"Bracket 2EV 3x", @33584: @"Bracket 3EV 3x", @32792: @"Bracket WB Lo", @32808: @"Bracket WB Hi", @32793: @"Bracket DRO Lo", @32809: @"Bracket DRO Hi" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeWhiteBalance: {
      NSDictionary *map = @{ @2: @"Auto", @4: @"Daylight", @32785: @"Shade", @32784: @"Cloudy", @6: @"Incandescent", @32769: @"Flourescent warm white", @32770: @"Flourescent cool white", @32771: @"Flourescent day white", @32772:@"Flourescent daylight",  @7: @"Flash", @32786: @"C.Temp/Filter", @32803: @"Custom" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeCompressionSetting: {
      NSDictionary *map = @{ @2: @"Standard", @3: @"Fine", @16: @"RAW", @19: @"RAW + JPEG" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeExposureMeteringMode: {
      NSDictionary *map = @{ @1: @"Average", @4: @"Center-spot", @32770: @"Center" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFNumber: {
      property.supportedValues = @[ @350, @400, @450, @500, @560, @630, @710, @800, @900, @1000, @1100, @1300, @1400, @1600, @1800, @2000, @2200 ];
      return [super processPropertyDescription:property];
    }
    case PTPPropertyCodeFocusMode: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"AF-S", @3:@"Macro", @0x8004: @"AF-C", @0x8006: @"DMF" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyImageSize: {
      NSDictionary *map = @{ @1: @"Large", @2: @"Medium", @3: @"Small" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyAspectRatio: {
      NSDictionary *map = @{ @1: @"3:2", @2: @"16:9" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyPictureEffect: {
      NSDictionary *map = @{ @32768: @"Off", @32769: @"Toy camera - normal", @32770: @"Toy camera - cool", @32771: @"Toy camera - warm", @32772: @"Toy camera - green", @32773: @"Toy camera - magenta", @32784: @"Pop Color", @32800: @"Posterisation B/W", @32801: @"Posterisation Color", @32816: @"Retro", @32832: @"Soft high key", @32848: @"Partial color - red", @32849: @"Partial color - green", @32850: @"Partial color - blue", @32851: @"Partial color - yellow", @32864: @"High contrast mono", @32880: @"Soft focus - low", @32881: @"Soft focus - mid", @32882: @"Soft focus - high", @32896: @"HDR painting - low", @32897: @"HDR painting - mid", @32898: @"HDR painting - high", @32912: @"Rich tone mono", @32928: @"Miniature - auto", @32929: @"Miniature - top", @32930: @"Miniature - middle horizontal", @32931: @"Miniature - bottom", @32932: @"Miniature - right", @32933: @"Miniature - middle vertical", @32934: @"Miniature - left", @32944: @"Watercolor", @32960: @"Illustration - low", @32961: @"Illustration - mid", @32962: @"Illustration - high" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyISO: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSNumber *value in property.supportedValues) {
        [values addObject:value.description];
        if (value.intValue == 16777215)
          [labels addObject:@"Auto"];
        else
          [labels addObject:value.description];
      }
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeSonyShutterSpeed: {
      NSArray *values = @[ @"69536", @"68736", @"68036", @"67536", @"67136", @"66786", @"66536", @"66336", @"66176", @"66036", @"65936", @"65856", @"65786", @"65736", @"65696", @"65661", @"65636", @"65616", @"65596", @"65586", @"65576", @"65566", @"65561", @"65556", @"65551", @"65549", @"65546", @"65544", @"65542", @"65541", @"65540", @"65539", @"262154", @"327690", @"393226", @"524298", @"655370", @"851978", @"1048586", @"1310730", @"1638410", @"2097162", @"2621450", @"3276810", @"3932170", @"5242890", @"6553610", @"8519690", @"9830410", @"13107210", @"16384010", @"19660810", ];
      NSArray *labels = @[ @"1/4000", @"1/3200", @"1/2500", @"1/2000", @"1/1600", @"1/1250", @"1/1000", @"1/800", @"1/600", @"1/500", @"1/400", @"1/320", @"1/250", @"1/200", @"1/160", @"1/125", @"1/100", @"1/80", @"1/60", @"1/50", @"1/40", @"1/30", @"1/25", @"1/20", @"1/15", @"1/13", @"1/10", @"1/8", @"1/6", @"1/5", @"1/4", @"1/3", @"0.4\"", @"0.5\"", @"0.6\"", @"0.8\"", @"1\"", @"1.3\"", @"1.6\"", @"2.0\"", @"2.5\"", @"3.2\"", @"4\"", @"5\"", @"6\"", @"8\"", @"10\"", @"13\"", @"15\"", @"20\"", @"25\"", @"30\"" ];
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureProgramMode: {
      NSArray *values = @[ @"32768", @"32769", @"2", @"3", @"4", @"1", @"32848", @"32849", @"32850", @"32851", @"32852", @"32833", @"7", @"32785", @"32789", @"32788", @"32786", @"32787", @"32790", @"32791", @"32792"];
      NSArray *labels = @[ @"Intelligent auto", @"Superior auto", @"P", @"A", @"S", @"M", @"P - movie", @"A - movie", @"S - movie", @"M - movie", @"0x8054", @"Sweep panorama", @"Portrait", @"Sport", @"Macro", @"Landscape", @"Sunset", @"Night scene", @"Handheld twilight", @"Night portrait", @"Anti motion blur" ];
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    default: {
      [super processPropertyDescription:property];
      break;
    }
  }
}

-(void)processConnect {
  [super processConnect];
  initialized = true;
}

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        self.info = [[self.deviceInfoClass alloc] initWithData:data];
        for (NSNumber *code in self.info.propertiesSupported)
          [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
        if ([self operationIsSupported:PTPRequestCodeSonyGetSDIOGetExtDeviceInfo]) {
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
      [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      break;
    }
    case PTPRequestCodeSonyGetDevicePropDesc: {
        //NSLog(@"%@", data);
      break;
    }
    case PTPRequestCodeSonyGetAllDevicePropData: {
        //NSLog(@"%@", data);
      unsigned char* buffer = (unsigned char*)data.bytes;
      unsigned char* buf = buffer;
      unsigned int count = ptpReadUnsignedInt(&buf);
      ptpReadUnsignedInt(&buf);
      for (int i = 0; i < count; i++) {
        PTPProperty *property = ptpReadSonyProperty(&buf);
        NSNumber *codeNumber = [NSNumber numberWithUnsignedShort:property.propertyCode];
        PTPProperty *current = self.info.properties[codeNumber];
        if (current == nil || current.value != property.value) {
          self.info.properties[codeNumber] = property;
          [self processPropertyDescription:property];
        }
      }
      break;
    }
    case PTPRequestCodeSonySetControlDeviceA: {
      NSLog(@"PTPRequestCodeSonySetControlDeviceA: %@", data);
      break;
    }
    case PTPRequestCodeSonySetControlDeviceB: {
      NSLog(@"PTPRequestCodeSonySetControlDeviceB: %@", data);
      break;
    }
    default: {
      [super processRequest:request Response:response inData:data];
      break;
    }
  }
}

-(void)setProperty:(PTPPropertyCode)code operation:(PTPRequestCode)requestCode value:(NSString *)value {
  PTPProperty *property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
  if (property) {
    switch (property.type) {
      case PTPDataTypeCodeSInt8: {
        unsigned char *buffer = malloc(sizeof (char));
        unsigned char *buf = buffer;
        ptpWriteChar(&buf, (char)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (char)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt8: {
        unsigned char *buffer = malloc(sizeof (unsigned char));
        unsigned char *buf = buffer;
        ptpWriteUnsignedChar(&buf, (unsigned char)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (unsigned char)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt16: {
        unsigned char *buffer = malloc(sizeof (short));
        unsigned char *buf = buffer;
        ptpWriteShort(&buf, (short)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (short)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt16: {
        unsigned char *buffer = malloc(sizeof (unsigned short));
        unsigned char *buf = buffer;
        ptpWriteUnsignedShort(&buf, (unsigned short)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (unsigned short)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt32: {
        unsigned char *buffer = malloc(sizeof (int));
        unsigned char *buf = buffer;
        ptpWriteInt(&buf, (int)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (int)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt32: {
        unsigned char *buffer = malloc(sizeof (unsigned int));
        unsigned char *buf = buffer;
        ptpWriteUnsignedInt(&buf, (unsigned int)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (unsigned int)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt64: {
        unsigned char *buffer = malloc(sizeof (long));
        unsigned char *buf = buffer;
        ptpWriteLong(&buf, (long)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (long)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt64: {
        unsigned char *buffer = malloc(sizeof (unsigned long));
        unsigned char *buf = buffer;
        ptpWriteUnsignedLong(&buf, (unsigned long)value.longLongValue);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:sizeof (unsigned long)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUnicodeString: {
        unsigned char *buffer = malloc(256);
        unsigned char *buf = buffer;
        int length = ptpWriteString(&buf, value);
        [self sendPTPRequest:requestCode param1:code data:[NSData dataWithBytes:buffer length:length]];
        free(buffer);
        break;
      }
    }
  }
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  [self setProperty:code operation:PTPRequestCodeSonySetControlDeviceA value:value];
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
