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
  //property.readOnly = false; 
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
    case PTPPropertyCodeSonyFocusStatus: return @"PTPPropertyCodeSonyFocusStatus";
    case PTPPropertyCodeSonyExposeIndex: return @"PTPPropertyCodeSonyExposeIndex";
    case PTPPropertyCodeSonyPictureEffect: return @"PTPPropertyCodeSonyPictureEffect";
    case PTPPropertyCodeSonyABFilter: return @"PTPPropertyCodeSonyABFilter";
    case PTPPropertyCodeSonyISO: return @"PTPPropertyCodeSonyISO";
    case PTPPropertyCodeSonyAutofocus: return @"PTPPropertyCodeSonyAutofocus";
    case PTPPropertyCodeSonyCapture: return @"PTPPropertyCodeSonyCapture";
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
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPSonyRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPSonyEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
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
  unsigned int format;
  unsigned short mode;
  bool waitForCapture;
  unsigned int shutterSpeed;
  unsigned int focusMode;
  bool liveView;
  int retryCount;
  PTPPropertyCode iteratedProperty;
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
    liveView = false;
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
    case PTPEventCodeSonyObjectAdded: {
      self.remainingCount = self.imagesPerShot;
      [self sendPTPRequest:PTPRequestCodeGetObjectInfo param1:event.parameter1];
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
    case PTPPropertyCodeSonyDPCCompensation: {
			int value = property.value.intValue;
			NSArray *values = @[ @"-", property.value.description,  @"+" ];
			NSArray *labels = @[ @"-", [NSString stringWithFormat:@"%+.1f", value / 1000.0],  @"+" ];
      property.readOnly = mode != 2 && mode != 3 && mode != 4 && mode != 1 && mode != 32848 && mode != 32849 && mode != 32850 && mode != 32851;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureBiasCompensation: {
			int value = property.value.intValue;
			NSArray *values = @[ @"-", property.value.description,  @"+" ];
			NSArray *labels = @[ @"-", [NSString stringWithFormat:@"%+.1f", value / 1000.0],  @"+" ];
      property.readOnly = mode != 2 && mode != 3 && mode != 4 && mode != 32848 && mode != 32849 && mode != 32850 && mode != 32851;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeStillCaptureMode: {
      NSDictionary *map = @{ @1: @"Single shooting", @2: @"Cont. shooting HI", @0x8012: @"Cont. shooting LO", @32787: @"Cont. shooting", @32788: @"Spd priority cont.", @32773: @"Self-timer 2s", @0x8003: @"Self-timer 5s", @32772: @"Self-time 10s", @32776: @"Self-timer 10s 3x", @32777: @"Self-timer 10s 5x", @33591: @"Bracket 1/3EV 3x cont.", @33655: @"Bracket 2/3EV 3x cont.", @33553: @"Bracket 1EV 3x cont.", @33569: @"Bracket 2EV 3x cont.", @33585: @"Bracket 3EV 3x cont.", @33590: @"Bracket 1/3EV 3x", @33654: @"Bracket 2/3EV 3x", @33552: @"Bracket 1EV 3x", @33568: @"Bracket 2EV 3x", @33584: @"Bracket 3EV 3x", @32792: @"Bracket WB Lo", @32808: @"Bracket WB Hi", @32793: @"Bracket DRO Lo", @32809: @"Bracket DRO Hi" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeWhiteBalance: {
      NSDictionary *map = @{ @2: @"Auto", @4: @"Daylight", @32785: @"Shade", @32784: @"Cloudy", @6: @"Incandescent", @32769: @"Flourescent warm white", @32770: @"Flourescent cool white", @32771: @"Flourescent day white", @32772:@"Flourescent daylight",  @7: @"Flash", @32786: @"C.Temp/Filter", @32816: @"Underwater", @32800: @"Custom 1", @32801: @"Custom 2", @32802: @"Custom 3", @32803: @"Custom 4" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeCompressionSetting: {
      self.imagesPerShot = property.value.intValue == 19 ? 2 : 1;
      NSDictionary *map = @{ @2: @"Standard", @3: @"Fine", @4: @"Extra Fine", @16: @"RAW", @19: @"RAW + JPEG" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeExposureMeteringMode: {
      NSArray *values = @[ @"1", @"2", @"4" ];
      NSArray *labels = @[ @"Multi", @"Center", @"Spot" ];
      property.readOnly = false;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeFNumber: {
			if (![property.value.description isEqualTo:@"0"]) {
				int value = property.value.intValue;
				NSArray *values = @[ @"-", property.value.description,  @"+" ];
				NSArray *labels = @[ @"-", [NSString stringWithFormat:@"f/%.1f", value / 100.0],  @"+" ];
				property.readOnly = mode != 3 && mode != 1 && mode != 32849 && mode != 32851;
				[self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
			}
      break;
    }
    case PTPPropertyCodeFocusMode: {
      NSArray *values = @[ @"2", @"32772", @"32774", @"1" ];
      NSArray *labels = @[ @"AF-S", @"AF-C", @"DMF", @"MF" ];
      property.readOnly = mode == 32848 || mode == 32849 || mode == 32850 || mode == 32851;
      focusMode = property.value.intValue;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeSonyDRangeOptimize: {
      NSDictionary *map = @{ @1:@"Off", @31:@"DRO Auto", @17:@"DRO Lv1", @18:@"DRO Lv2", @19:@"DRO Lv3", @20:@"DRO Lv4", @21:@"DRO Lv1", @32:@"Auto HDR", @33:@"Auto HDR 1.0EV", @34:@"Auto HDR 2.0EV", @35:@"Auto HDR 3.0EV", @36:@"Auto HDR 4.0EV", @37:@"Auto HDR 5.0EV", @38:@"Auto HDR 6.0EV" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyCCFilter: {
      NSArray *values = @[ @"135", @"134", @"133", @"132", @"131", @"130", @"129", @"128", @"127", @"126", @"125", @"124", @"123", @"122", @"121" ];
      NSArray *labels = @[ @"G7", @"G6", @"G5", @"G4", @"G3", @"G2", @"G1", @"0", @"M1", @"M2", @"M3", @"M4", @"M5", @"M6", @"M7" ];
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeSonyABFilter: {
      NSArray *values = @[ @"135", @"134", @"133", @"132", @"131", @"130", @"129", @"128", @"127", @"126", @"125", @"124", @"123", @"122", @"121" ];
      NSArray *labels = @[ @"A7", @"A6", @"A5", @"A4", @"A3", @"A2", @"A1", @"0", @"B1", @"B2", @"B3", @"B4", @"B5", @"B6", @"B7" ];
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
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
			NSString *value = property.value.description;
			if ([value isEqualToString:@"16777215"])
				value = @"Auto";
			NSArray *values = @[ @"-", property.value.description,  @"+" ];
			NSArray *labels = @[ @"-", value,  @"+" ];
      property.readOnly = mode != 2 && mode != 3 && mode != 4 && mode != 1 && mode != 32848 && mode != 32849 && mode != 32850 && mode != 32851;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeSonyShutterSpeed: {
			shutterSpeed = property.value.intValue;
			NSString *value;
			if (shutterSpeed == 0)
				value = @"Bulb";
			else {
				int a = shutterSpeed >> 16;
				int b = shutterSpeed & 0xFFFF;
				if (b == 10)
					value = [NSString stringWithFormat:@"%g\"", (double)a / b];
				else
					value = [NSString stringWithFormat:@"1/%d", b];
			}
			NSArray *values = @[ @"-", property.value.description,  @"+" ];
			NSArray *labels = @[ @"+", value, @"-" ];
      property.readOnly = mode != 4 && mode != 1 && mode != 32850 && mode != 32851;
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureProgramMode: {
      NSArray *values = @[ @"32768", @"32769", @"2", @"3", @"4", @"1", @"32848", @"32849", @"32850", @"32851", @"32852", @"32833", @"7", @"32785", @"32789", @"32788", @"32786", @"32787", @"32790", @"32791", @"32792"];
      NSArray *labels = @[ @"Intelligent auto", @"Superior auto", @"P", @"A", @"S", @"M", @"P - movie", @"A - movie", @"S - movie", @"M - movie", @"0x8054", @"Sweep panorama", @"Portrait", @"Sport", @"Macro", @"Landscape", @"Sunset", @"Night scene", @"Handheld twilight", @"Night portrait", @"Anti motion blur" ];
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeFlashMode: {
      NSDictionary *map = @{ @0: @"Undefined", @1: @"Automatic flash", @2: @"Flash off", @3: @"Fill flash", @4: @"Automatic Red-eye Reduction", @5: @"Red-eye fill flash", @6: @"External sync", @0x8032: @"Slow Sync", @0x8003: @"Reer Sync" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeSonyFocusStatus: {
      [super processPropertyDescription:property];
      if (focusMode == 1)
        break;
      switch (property.value.intValue) {
        case 2:     // AFS
        case 6: {   // AFC
          if (waitForCapture) {
            waitForCapture = false;
            if (shutterSpeed) {
              [self setProperty:PTPPropertyCodeSonyCapture value:@"2"];
              [self setProperty:PTPPropertyCodeSonyCapture value:@"1"];
              [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
            } else {
              [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
              [self setProperty:PTPPropertyCodeSonyCapture value:@"2"];
            }
          } else {
            [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
          }
          break;
        }
        case 3: {
          if (waitForCapture) {
            waitForCapture = false;
            [self.delegate cameraExposureFailed:self message:@"Failed to focus"];
          }
          [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
        }
      }
      break;
    }
    default: {
      [super processPropertyDescription:property];
      break;
    }
  }
}

-(void)processConnect {
  if ([self propertyIsSupported:PTPPropertyCodeSonyStillImage])
    [self.delegate cameraCanExposure:self];
  [self.delegate cameraCanPreview:self];
  [super processConnect];
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
      unsigned short *codes = (unsigned short *)data.bytes;
      long count = data.length / 2;
      if (self.info.operationsSupported == nil)
        self.info.operationsSupported = [NSMutableArray array];
      if (self.info.eventsSupported == nil)
        self.info.eventsSupported = [NSMutableArray array];
      if (self.info.propertiesSupported == nil)
        self.info.propertiesSupported = [NSMutableArray array];
      for (int i = 1; i < count; i++) {
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
      break;
    }
    case PTPRequestCodeSonyGetAllDevicePropData: {
      unsigned char* buffer = (unsigned char*)data.bytes;
      unsigned char* buf = buffer;
      unsigned int count = ptpReadUnsignedInt(&buf);
      ptpReadUnsignedInt(&buf);
      NSMutableArray *properties = [NSMutableArray array];
      for (int i = 0; i < count; i++) {
        PTPProperty *property = ptpReadSonyProperty(&buf);
        NSNumber *codeNumber = [NSNumber numberWithUnsignedShort:property.propertyCode];
        if (property.propertyCode == PTPPropertyCodeExposureProgramMode)
          mode = property.value.intValue;
        self.info.properties[codeNumber] = property;
        [properties addObject:property];
      }
      for (PTPProperty *property in properties) {
        if (iteratedProperty == 0 || iteratedProperty == property.propertyCode) {
          [self.delegate debug:[property description]];
          [self processPropertyDescription:property];
        }
      }
      break;
    }
    case PTPRequestCodeSonySetControlDeviceA: {
      break;
    }
    case PTPRequestCodeSonySetControlDeviceB: {
      break;
    }
    case PTPRequestCodeGetObjectInfo: {
      unsigned short *buf = (unsigned short *)data.bytes;
      format = buf[2];
      [self sendPTPRequest:PTPRequestCodeGetObject param1:request.parameter1];
      break;
    }
    case PTPRequestCodeGetObject: {
      if (request.parameter1 == 0xffffc002) {
        if (liveView) {
          if (response.responseCode == PTPResponseCodeAccessDenied) {
            if (retryCount < 100) {
              retryCount++;
              usleep(100000);
              [self getPreviewImage];
            } else {
              liveView = false;
              [self.delegate cameraExposureFailed:self message:@"LiveView failed"];
            }
          } else {
            uint8 *start = (uint8 *)data.bytes;
            unsigned long i = data.length;
            retryCount = 0;
            while (i > 0) {
              if (start[0] == 0xFF && start[1] == 0xD8) {
                uint8 *end = start + 2;
                i -= 2;
                while (i > 0) {
                  if (end[0] == 0xFF && end[1] == 0xD9) {
                    data = [NSData dataWithBytes:start length:end-start];
                    [self.delegate cameraExposureDone:self data:data filename:@"preview.jpeg"];
                    break;
                  }
                  end++;
                  i--;
                }
                break;
              }
              start++;
              i--;
            }
            [self getPreviewImage];
          }
        }
      } else {
        self.remainingCount--;
        if (format == 0x3801)
          [self.delegate cameraExposureDone:self data:data filename:@"image.jpeg"];
        else if (format == 0xb101)
          [self.delegate cameraExposureDone:self data:data filename:@"image.arw"];
        if (self.remainingCount > 0)
          [self sendPTPRequest:PTPRequestCodeGetObjectInfo param1:request.parameter1];
      }
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

#define MAX_WAIT  10
#define MAX_DELAY 100000

-(void)iterate:(PTPPropertyCode)code to:(NSString *)value withMap:(long *)map {
  iteratedProperty = code;
  PTPProperty *property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
  int current = property.value.intValue;
  int requested = value.intValue;
  int wait;
  for (int i = 0; map[i] != -1; i++)
    if (current == map[i]) {
      current = i;
      break;
    }
  for (int i = 0; map[i] != -1; i++)
    if (requested == map[i]) {
      requested = i;
      break;
    }
  if (current < requested)
    for (int i = current; i < requested; i++) {
      [self setProperty:code operation:PTPRequestCodeSonySetControlDeviceB value:@"1"];
      for (wait = 0; wait < MAX_WAIT; wait++) {
        [self sendPTPRequest:PTPRequestCodeSonyGetAllDevicePropData];
        usleep(MAX_DELAY);
        property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
        if (property.value.intValue == map[i + 1]) {
          break;
        }
      }
      if (wait == MAX_WAIT)
        break;
    }
  else if (current > requested)
    for (int i = current; i > requested; i--) {
      [self setProperty:code operation:PTPRequestCodeSonySetControlDeviceB value:@"-1"];
      for (wait = 0; wait < MAX_WAIT; wait++) {
        [self sendPTPRequest:PTPRequestCodeSonyGetAllDevicePropData];
        usleep(MAX_DELAY);
        property = self.info.properties[[NSNumber numberWithUnsignedShort:code]];
        if (property.value.intValue == map[i - 1]) {
          break;
        }
      }
      if (wait == MAX_WAIT)
        break;
    }
  iteratedProperty = 0;
  [self sendPTPRequest:PTPRequestCodeSonyGetAllDevicePropData];
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  switch (code) {
		case PTPPropertyCodeSonyDPCCompensation:
		case PTPPropertyCodeExposureBiasCompensation:
		case PTPPropertyCodeSonyISO:
		case PTPPropertyCodeSonyShutterSpeed:
    case PTPPropertyCodeFNumber: {
			if ([value isEqualToString:@"-"]) {
				[self setProperty:code operation:PTPRequestCodeSonySetControlDeviceB value:@"-1"];
			} else if ([value isEqualToString: @"+"]) {
				[self setProperty:code operation:PTPRequestCodeSonySetControlDeviceB value:@"1"];
			}
      break;
    }
    case PTPPropertyCodeSonyCapture:
    case PTPPropertyCodeSonyStillImage:
    case PTPPropertyCodeSonyAutofocus: {
      [self setProperty:code operation:PTPRequestCodeSonySetControlDeviceB value:value];
      break;
    }
    default: {
      [self setProperty:code operation:PTPRequestCodeSonySetControlDeviceA value:value];
      break;
    }
  }
}

-(void)requestEnableTethering {
}

-(void)getPreviewImage {
  [self sendPTPRequest:PTPRequestCodeGetObject param1:0xffffc002];
}

-(void)lock {
}

-(void)unlock {
}

-(void)startPreview {
  self.remainingCount = 0;
  liveView = true;
  retryCount = 0;
  [self getPreviewImage];
}

-(void)stopPreview {
  liveView = false;
}

-(void)startAutofocus {
  [self setProperty:PTPPropertyCodeSonyAutofocus value:@"2"];
}

-(void)stopAutofocus {
  [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
}

-(double)startExposure {
  waitForCapture = true;
  liveView = false;
  [self setProperty:PTPPropertyCodeSonyAutofocus value:@"2"];
  if (focusMode == 1) {
    [self.delegate log:@"Exposure initiated"];
    if (shutterSpeed) {
      waitForCapture = false;
      [self setProperty:PTPPropertyCodeSonyCapture value:@"2"];
      [self setProperty:PTPPropertyCodeSonyCapture value:@"1"];
      [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
    } else {
      [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
      [self setProperty:PTPPropertyCodeSonyCapture value:@"2"];
    }
  }
  return 0;
}

-(void)stopExposure {
  waitForCapture = false;
  [self setProperty:PTPPropertyCodeSonyCapture value:@"1"];
//    [self setProperty:PTPPropertyCodeSonyAutofocus value:@"1"];
//    self.remainingCount = self.imagesPerShot;
//    [self sendPTPRequest:PTPRequestCodeGetObjectInfo param1:0xFFFFC001];
}

-(void)focus:(int)steps {
}

@end
