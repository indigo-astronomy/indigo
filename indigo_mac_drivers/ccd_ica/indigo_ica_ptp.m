// Copyright (c) 2017 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ICA PTP wrapper
 \file indigo_ica_ptp.m
 */


// Code is based on PTPPassThrough example provided by Apple Inc.
// https://developer.apple.com/legacy/library/samplecode/PTPPassThrough
//
// This Apple software is supplied to you by Apple Inc. ("Apple")
// in consideration of your agreement to the following terms, and your use,
// installation, modification or redistribution of this Apple software
// constitutes acceptance of these terms. If you do not agree with these
// terms, please do not use, install, modify or redistribute this Apple
// software.
//
// In consideration of your agreement to abide by the following terms, and
// subject to these terms, Apple grants you a personal, non - exclusive
// license, under Apple's copyrights in this original Apple software (the
// "Apple Software"), to use, reproduce, modify and redistribute the Apple
// Software, with or without modifications, in source and / or binary forms;
// provided that if you redistribute the Apple Software in its entirety and
// without modifications, you must retain this notice and the following text
// and disclaimers in all such redistributions of the Apple Software. Neither
// the name, trademarks, service marks or logos of Apple Inc. may be used to
// endorse or promote products derived from the Apple Software without specific
// prior written permission from Apple. Except as expressly stated in this
// notice, no other rights or licenses, express or implied, are granted by
// Apple herein, including but not limited to any patent rights that may be
// infringed by your derivative works or by other works in which the Apple
// Software may be incorporated.
//
// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO
// WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
// WARRANTIES OF NON - INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION
// ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND / OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER
// UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
// OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2009 Apple Inc. All Rights Reserved.
//
//------------------------------------------------------------------------------------------------------------------------------

#include "indigo_bus.h"

#import "indigo_ica_ptp.h"


static char ptpReadChar(unsigned char** buf) {
  char value = *(SInt8*)(*buf);
  (*buf) += 1;
  return value;
}

static void ptpWriteChar(unsigned char** buf, char value) {
  *(SInt8*)(*buf) = value;
  (*buf) += 1;
}

static unsigned char ptpReadUnsignedChar(unsigned char** buf) {
  char value = *(UInt8*)(*buf);
  (*buf) += 1;
  return value;
}

static void ptpWriteUnsignedChar(unsigned char** buf, unsigned char value) {
  *(UInt8*)(*buf) = value;
  (*buf) += 1;
}

static short ptpReadShort(unsigned char** buf) {
  SInt16 value = (SInt16)CFSwapInt16LittleToHost(*(UInt16*)(*buf));
  (*buf) += 2;
  return value;
}

static void ptpWriteShort(unsigned char** buf, short value) {
  *(SInt16*)(*buf) = (SInt16)CFSwapInt16HostToLittle(value);
  (*buf) += 2;
}

static unsigned short ptpReadUnsignedShort(unsigned char** buf) {
  unsigned short value = CFSwapInt16LittleToHost(*(unsigned short*)(*buf));
  (*buf) += 2;
  return value;
}

static void ptpWriteUnsignedShort(unsigned char** buf, unsigned short value) {
  *(unsigned short*)(*buf) = CFSwapInt16HostToLittle(value);
  (*buf) += 2;
}

static int ptpReadInt(unsigned char** buf) {
  int value = (int)CFSwapInt32LittleToHost(*(int*)(*buf));
  (*buf) += 4;
  return value;
}

static void ptpWriteInt(unsigned char** buf, int value) {
  *(int*)(*buf) = (int)CFSwapInt32HostToLittle(value);
  (*buf) += 4;
}

static unsigned int ptpReadUnsignedInt(unsigned char** buf) {
  int value = CFSwapInt32LittleToHost(*(int*)(*buf));
  (*buf) += 4;
  return value;
}

static void ptpWriteUnsignedInt(unsigned char** buf, unsigned int value) {
  *(unsigned int*)(*buf) = CFSwapInt32HostToLittle(value);
  (*buf) += 4;
}

static long ptpReadLong(unsigned char** buf) {
  long value = (long)CFSwapInt64LittleToHost(*(long*)(*buf));
  (*buf) += 8;
  return value;
}

static void ptpWriteLong(unsigned char** buf, long value) {
  *(long*)(*buf) = (long)CFSwapInt64HostToLittle(value);
  (*buf) += 8;
}

static unsigned long ptpReadUnsignedLong(unsigned char** buf) {
  unsigned long value = CFSwapInt64LittleToHost(*(unsigned long*)(*buf));
  (*buf) += 8;
  return value;
}

static void ptpWriteUnsignedLong(unsigned char** buf, unsigned long value) {
  *(unsigned long*)(*buf) = CFSwapInt64HostToLittle(value);
  (*buf) += 8;
}

static NSArray<NSNumber *> *ptpReadCharArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithChar:ptpReadChar(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadUnsignedCharArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedChar:ptpReadUnsignedChar(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadShortArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithShort:ptpReadShort(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadUnsignedShortArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedShort:ptpReadUnsignedShort(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadIntArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithInt:ptpReadInt(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadUnsignedIntArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadLongArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithLong:ptpReadLong(buf)]];
    return result;
  }
  return nil;
}

static NSArray<NSNumber *> *ptpReadUnsignedLongArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedLong:ptpReadUnsignedLong(buf)]];
    return result;
  }
  return nil;
}

static NSString *ptpReadString(unsigned char** buf) {
  int length = **buf;
  if (length) {
    NSString *result = [NSString stringWithString:[NSString stringWithCharacters: (const unichar *)(*buf + 1) length:length - 1]];
    *buf = (*buf) + length * 2 + 1;
    return result;
  }
  *buf = (*buf) + 1;
  return nil;
}

static unsigned char ptpWriteString(unsigned char **buf, NSString *value) {
  const char *cstr = [value cStringUsingEncoding:NSUnicodeStringEncoding];
  unsigned int length = (unsigned int)value.length + 1;
  if (length < 256) {
    **buf = length;
    memcpy(*buf + 1, cstr, 2 * length);
		*buf = (*buf) + 2 * length + 1;
    return 2 * length + 1;
  }
  return -1;
}

static NSObject *ptpReadValue(PTPDataTypeCode type, unsigned char **buf) {
  switch (type) {
    case PTPDataTypeCodeSInt8:
      return [NSNumber numberWithChar:ptpReadChar(buf)];
    case PTPDataTypeCodeUInt8:
      return [NSNumber numberWithUnsignedChar:ptpReadUnsignedChar(buf)];
    case PTPDataTypeCodeSInt16:
      return [NSNumber numberWithShort:ptpReadShort(buf)];
    case PTPDataTypeCodeUInt16:
      return [NSNumber numberWithUnsignedShort:ptpReadUnsignedShort(buf)];
    case PTPDataTypeCodeSInt32:
      return [NSNumber numberWithInt:ptpReadInt(buf)];
    case PTPDataTypeCodeUInt32:
      return [NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(buf)];
    case PTPDataTypeCodeSInt64:
      return [NSNumber numberWithLong:ptpReadLong(buf)];
    case PTPDataTypeCodeUInt64:
      return [NSNumber numberWithUnsignedLong:ptpReadUnsignedLong(buf)];
    case PTPDataTypeCodeArrayOfSInt8:
      return ptpReadCharArray(buf);
    case PTPDataTypeCodeArrayOfUInt8:
      return ptpReadUnsignedCharArray(buf);
    case PTPDataTypeCodeArrayOfSInt16:
      return ptpReadShortArray(buf);
    case PTPDataTypeCodeArrayOfUInt16:
      return ptpReadUnsignedShortArray(buf);
    case PTPDataTypeCodeArrayOfSInt32:
      return ptpReadIntArray(buf);
    case PTPDataTypeCodeArrayOfUInt32:
      return ptpReadUnsignedIntArray(buf);
    case PTPDataTypeCodeArrayOfSInt64:
      return ptpReadLongArray(buf);
    case PTPDataTypeCodeArrayOfUInt64:
      return ptpReadUnsignedLongArray(buf);
    case PTPDataTypeCodeUnicodeString:
      return ptpReadString(buf);
  }
  
  return nil;
}

//---------------------------------------------------------------------------------------------------------- PTPOperationRequest

@implementation PTPOperationRequest

- (id)init {
	self = [super init];
	if (self) {
		_numberOfParameters = 0;
	}
	return self;
}

- (id)initWithVendorExtension:(PTPVendorExtension)vendorExtension {
  self = [super init];
  if (self) {
    _vendorExtension = vendorExtension;
  }
  return self;
}

- (NSData*)commandBuffer {
  unsigned int len = 12 + 4*_numberOfParameters;
  unsigned char* buffer = (unsigned char*)calloc(len, 1);
  unsigned char* buf = buffer;
  ptpWriteUnsignedInt(&buf, len);
  ptpWriteUnsignedShort(&buf, 1);
  ptpWriteUnsignedShort(&buf, _operationCode);
  ptpWriteUnsignedInt(&buf, 0);
  if (_numberOfParameters > 0)
    ptpWriteUnsignedInt(&buf, _parameter1);
  if (_numberOfParameters > 1)
    ptpWriteUnsignedInt(&buf, _parameter2);
  if (_numberOfParameters > 2)
    ptpWriteUnsignedInt(&buf, _parameter3);
  if (_numberOfParameters > 3)
    ptpWriteUnsignedInt(&buf, _parameter4);
  if (_numberOfParameters > 4)
    ptpWriteUnsignedInt(&buf, _parameter5);
  return [NSData dataWithBytesNoCopy:buffer length:len freeWhenDone:YES];
}

@end

//--------------------------------------------------------------------------------------------------------- PTPOperationResponse

@implementation PTPOperationResponse

- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 32))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 32 || type != 3)
    return NULL;
  if ((self = [super init])) {
    _vendorExtension = vendorExtension;
    unsigned char* buf = buffer + 6;
    _responseCode = ptpReadUnsignedShort(&buf);
    _transactionID = ptpReadUnsignedInt(&buf);
    _numberOfParameters = (size-12) >> 2;
    if (_numberOfParameters > 0)
      _parameter1 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 1)
      _parameter2 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 2)
      _parameter3 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 3)
      _parameter4 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 4)
      _parameter5 = ptpReadUnsignedInt(&buf);
  }
  return self;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPEvent

@implementation PTPEvent

- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 24))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 24 || type != 4)
    return NULL;
  if ((self = [super init])) {
    _vendorExtension = vendorExtension;
    unsigned char* buf = buffer + 6;
    _eventCode = ptpReadUnsignedShort(&buf);
    _transactionID = ptpReadUnsignedInt(&buf);
    _numberOfParameters = (size-12) >> 2;
    if (_numberOfParameters > 0)
      _parameter1 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 1)
      _parameter2 = ptpReadUnsignedInt(&buf);
    if (_numberOfParameters > 2)
      _parameter3 = ptpReadUnsignedInt(&buf);
  }
  return self;
}

- (id)initWithCode:(PTPEventCode)eventCode parameter1:(unsigned int)parameter1 vendorExtension:(PTPVendorExtension)vendorExtension {
  self = [super init];
  if (self) {
    _eventCode = eventCode;
    _numberOfParameters = 1;
    _parameter1 = parameter1;
    _vendorExtension = vendorExtension;
  }
  return self;
}

@end

//------------------------------------------------------------------------------------------------------------------------------ PTPProperty

@implementation PTPProperty

- (id)initWithCode:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension{
  if ((self = [super init])) {
    _vendorExtension = vendorExtension;
    _propertyCode = propertyCode;
    _type = PTPDataTypeCodeUndefined;
  }
  return self;
}
- (id)initWithData:(NSData*)data  vendorExtension:(PTPVendorExtension)vendorExtension{
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 5))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  if ((self = [super init])) {
    _vendorExtension = vendorExtension;
    unsigned char* buf = buffer;
    _propertyCode = ptpReadUnsignedShort(&buf);
    _type = ptpReadUnsignedShort(&buf);
    _readOnly = !ptpReadUnsignedChar(&buf);
    _defaultValue = ptpReadValue(_type, &buf);
    if (buf - buffer < dataLength) {
      _value = ptpReadValue(_type, &buf);
      if (buf - buffer < dataLength) {
        int form = ptpReadUnsignedChar(&buf);
        switch (form) {
          case 1: {
            _min = ptpReadValue(_type, &buf);
            _max = ptpReadValue(_type, &buf);
            _step = ptpReadValue(_type, &buf);
            break;
          }
          case 2: {
            int count = ptpReadUnsignedShort(&buf);
            NSMutableArray<NSObject*> *values = [NSMutableArray arrayWithCapacity:count];
            for (int i = 0; i < count; i++)
              [values addObject:ptpReadValue(_type, &buf)];
            _supportedValues = values;
            break;
          }
        }
      }
    }
  }
  return self;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPDeviceInfo

@implementation PTPDeviceInfo

- (id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12))
    return NULL;
  self = [super init];
  unsigned char* buffer = (unsigned char*)[data bytes];
  if (self) {
    unsigned char* buf = buffer;
    _standardVersion = ptpReadUnsignedShort(&buf);
    _vendorExtensionID = ptpReadUnsignedInt(&buf);
    _vendorExtensionVersion = ptpReadUnsignedShort(&buf);
    _vendorExtensionDesc = ptpReadString(&buf);
    if (buf - buffer >= dataLength)
      return self;
    _functionalMode = ptpReadUnsignedShort(&buf);
    _operationsSupported = ptpReadUnsignedShortArray(&buf);
    if (buf - buffer >= dataLength)
      return self;
    _eventsSupported = ptpReadUnsignedShortArray(&buf);
    if (buf - buffer >= dataLength)
      return self;
    _propertiesSupported = ptpReadUnsignedShortArray(&buf);
    if (buf - buffer >= dataLength)
      return self;
    ptpReadUnsignedShortArray(&buf); // capture formats
    if (buf - buffer >= dataLength)
      return self;
    ptpReadUnsignedShortArray(&buf); // capture formats
    
    if (buf - buffer >= dataLength)
      return self;
    _manufacturer = ptpReadString(&buf);
    
    if (_vendorExtensionID == PTPVendorExtensionMicrosoft && [_manufacturer containsString:@"Nikon"]) {
      _vendorExtensionID = PTPVendorExtensionNikon;
      _vendorExtensionVersion = 100;
      _vendorExtensionDesc = @"Nikon & Microsoft PTP Extensions";
    }
    if (buf - buffer >= dataLength)
      return self;
    _model = ptpReadString(&buf);
    if (buf - buffer >= dataLength)
      return self;
    _version = ptpReadString(&buf);
    if (buf - buffer >= dataLength)
      return self;
    _serial = ptpReadString(&buf);
  }
  return self;
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation NSObject(PTPExtensions)

-(NSString *)hexString {
	return [NSString stringWithFormat:@"%010lld", self.description.longLongValue];
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation ICCameraDevice(PTPExtensions)

- (void)checkForEvent {
  PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
  if (info.vendorExtensionID == PTPVendorExtensionNikon) {
    [self sendPTPRequest:PTPOperationCodeNikonCheckEvent];
  }
}

- (void)processEvent:(PTPEvent *)event {
	switch (event.eventCode) {
		case PTPEventCodeDevicePropChanged: {
			[self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:event.parameter1];
			break;
		}
  }
}

- (void)didSendPTPCommand:(NSData*)command inData:(NSData*)data response:(NSData*)response error:(NSError*)error contextInfo:(void*)contextInfo {
  PTPOperationRequest*  ptpRequest  = (__bridge PTPOperationRequest*)contextInfo;
  if (ptpRequest) {
    if (indigo_get_log_level() >= INDIGO_LOG_TRACE)
      NSLog(@"Completed %@", [ptpRequest description]);
    switch (ptpRequest.operationCode) {
      case PTPOperationCodeGetStorageIDs: {
        PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
        NSLog(@"Initialized %@\n", info.debug);
        [(PTPDelegate *)self.delegate cameraConnected:self];
        NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(checkForEvent) userInfo:nil repeats:true];
        [self.userData setObject:timer forKey:PTP_EVENT_TIMER];
        break;
      }
    }
  }
  if (response) {
    PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
    PTPOperationResponse* ptpResponse = [[PTPOperationResponse alloc] initWithData:response vendorExtension:info.vendorExtensionID];
		switch (ptpRequest.operationCode) {
			case PTPOperationCodeSetDevicePropValue: {
				switch (ptpRequest.parameter1) {
					case PTPPropertyCodeExposureProgramMode:
					case PTPPropertyCodeFNumber:
					case PTPPropertyCodeExposureTime:
					case PTPPropertyCodeImageSize:
					case PTPPropertyCodeCompressionSetting:
					case PTPPropertyCodeWhiteBalance:
					case PTPPropertyCodeExposureIndex:
						[self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:ptpRequest.parameter1];
						break;
				}
				break;
			}
    }
    if (indigo_get_log_level() >= INDIGO_LOG_TRACE || ptpResponse.responseCode != PTPResponseCodeOK)
      NSLog(@"Received %@", [ptpResponse description]);
  }
  if (data) {
    char *bytes = (void*)[data bytes];
    switch (ptpRequest.operationCode) {
      case PTPOperationCodeGetDeviceInfo: {
        PTPDeviceInfo *info = [[PTPDeviceInfo alloc] initWithData:data];
        [self.userData setObject:info forKey:PTP_DEVICE_INFO];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeExposureProgramMode];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeFNumber];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeExposureTime];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeImageSize];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeCompressionSetting];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeWhiteBalance];
        [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeExposureIndex];
				[self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:PTPPropertyCodeNikonLiveViewStatus];
        [self sendPTPRequest:PTPOperationCodeGetStorageIDs];
        break;
      }
      case PTPOperationCodeGetDevicePropDesc: {
        PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
        PTPProperty *property = [[PTPProperty alloc] initWithData:data vendorExtension:info.vendorExtensionID];
				self.userData[[NSString stringWithFormat:PTP_PROPERTY, property.propertyCode]] = property;
        switch (property.propertyCode) {
          case PTPPropertyCodeExposureProgramMode: {
            NSDictionary *labels = @{ @1: @"Manual", @2: @"Program", @3: @"Aperture priority", @4: @"Shutter priority", @32784: @"Auto", @32785: @"Portrait", @32786: @"Landscape", @32787:@"Macro", @32788: @"Sport", @32789: @"Night portrait", @32790:@"Night landscape", @32791: @"Child", @32792: @"Scene", @32793: @"Effects" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              NSString *label = labels[value];
              if (label)
                supportedValues[value.hexString] = labels[value];
              else
                supportedValues[value.hexString] = [NSString stringWithFormat:@"mode %@", value];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeFNumber: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              supportedValues[value.hexString] = [NSString stringWithFormat:@"f/%g", value.intValue / 100.0];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureTime: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              int intValue = value.intValue;
              if (intValue == -1)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"Bulb"];
              else if (intValue == 1)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/8000 s"];
              else if (intValue == 3)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/3200 s"];
              else if (intValue == 6)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/1600 s"];
              else if (intValue == 12)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/800 s"];
              else if (intValue == 15)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/640 s"];
              else if (intValue == 80)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/125 s"];
              else if (intValue < 100)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/%g s", round(1000.0 / value.intValue) * 10];
              else if (intValue < 10000)
                supportedValues[value.hexString] = [NSString stringWithFormat:@"1/%g s", round(10000.0 / value.intValue)];
              else
                supportedValues[value.hexString] = [NSString stringWithFormat:@"%g s", value.intValue / 10000.0];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeImageSize: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSString *value in property.supportedValues) {
              supportedValues[value] = value;
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCompressionSetting: {
            NSDictionary *labels = @{ @0: @"JPEG Basic", @1: @"JPEG Norm", @2: @"JPEG Fine", @4: @"RAW", @5: @"RAW + JPEG Basic", @6: @"RAW + JPEG Norm", @7: @"RAW + JPEG Fine" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              NSString *label = labels[value];
              if (label)
                supportedValues[value.hexString] = labels[value];
              else
                supportedValues[value.hexString] = [NSString stringWithFormat:@"compression %@", value];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeWhiteBalance: {
            NSDictionary *labels = @{ @1: @"Manual", @2: @"Auto", @3: @"One-push Auto", @4: @"Daylight", @5: @"Fluorescent", @6: @"Incandescent", @7: @"Flash", @32784: @"Cloudy", @32785: @"Shade", @32786: @"Color Temperature", @32787: @"Preset" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              NSString *label = labels[value];
              if (label)
                supportedValues[value.hexString] = labels[value];
              else
                supportedValues[value.hexString] = [NSString stringWithFormat:@"compression %@", value];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureIndex: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *value in property.supportedValues) {
              supportedValues[value.hexString] = value.description;
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:supportedValues readOnly:property.readOnly];
            break;
          }
					case PTPPropertyCodeNikonLiveViewStatus: {
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.hexString supportedValues:nil readOnly:property.readOnly];
						if (property.value.description.intValue)
							[self sendPTPRequest:PTPOperationCodeNikonGetLiveViewImg];
					}
        }
        break;
      }
      case PTPOperationCodeNikonCheckEvent: {
        unsigned char* buffer = (unsigned char*)[data bytes];
        unsigned char* buf = buffer;
        int count = ptpReadUnsignedShort(&buf);
        for (int i = 0; i < count; i++) {
          PTPEventCode code = ptpReadUnsignedShort(&buf);
          unsigned int parameter1 = ptpReadUnsignedInt(&buf);
          PTPEvent *event = [[PTPEvent alloc] initWithCode:code parameter1:parameter1 vendorExtension:self.ptpDeviceInfo.vendorExtensionID];
					if (indigo_get_log_level() >= INDIGO_LOG_TRACE) {
						NSLog(@"Received %@", [event description]);
					}
          [self processEvent:event];
        }
        break;
      }
			case PTPOperationCodeNikonGetLiveViewImg: {
				NSData *image;
				if ((bytes[64] & 0xFF) == 0xFF && (bytes[65] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 64 length:data.length - 64];
				else if ((bytes[128] & 0xFF) == 0xFF && (bytes[129] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 128 length:data.length - 128];
				else if ((bytes[384] & 0xFF) == 0xFF && (bytes[385] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 384 length:data.length - 384];
				if (image) {
					[(PTPDelegate *)self.delegate cameraExposureDone:self data:image filename:@"preview.jpeg"];
					[self sendPTPRequest:PTPOperationCodeNikonGetLiveViewImg];
				}
				break;
			}
      default:
        if (indigo_get_log_level() >= INDIGO_LOG_TRACE) {
          NSLog(@"Received data:");
          [self dumpData:(void *)bytes length:(int)[data length] comment:@"inData"];
        }
        break;
    }
  }
}

-(PTPDeviceInfo *)ptpDeviceInfo {
  return self.userData[PTP_DEVICE_INFO];
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
	PTPProperty *property = self.userData[[NSString stringWithFormat:PTP_PROPERTY, code]];
	if (property) {
		switch (property.type) {
			case PTPDataTypeCodeSInt8: {
				unsigned char *buffer = malloc(sizeof (char));
				unsigned char *buf = buffer;
				ptpWriteChar(&buf, (char)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (char)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeUInt8: {
				unsigned char *buffer = malloc(sizeof (unsigned char));
				unsigned char *buf = buffer;
				ptpWriteUnsignedChar(&buf, (unsigned char)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned char)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeSInt16: {
				unsigned char *buffer = malloc(sizeof (short));
				unsigned char *buf = buffer;
				ptpWriteShort(&buf, (short)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (short)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeUInt16: {
				unsigned char *buffer = malloc(sizeof (unsigned short));
				unsigned char *buf = buffer;
				ptpWriteUnsignedShort(&buf, (unsigned short)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned short)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeSInt32: {
				unsigned char *buffer = malloc(sizeof (int));
				unsigned char *buf = buffer;
				ptpWriteInt(&buf, (int)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (int)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeUInt32: {
				unsigned char *buffer = malloc(sizeof (unsigned int));
				unsigned char *buf = buffer;
				ptpWriteUnsignedInt(&buf, (unsigned int)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned int)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeSInt64: {
				unsigned char *buffer = malloc(sizeof (long));
				unsigned char *buf = buffer;
				ptpWriteLong(&buf, (long)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (long)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeUInt64: {
				unsigned char *buffer = malloc(sizeof (unsigned long));
				unsigned char *buf = buffer;
				ptpWriteUnsignedLong(&buf, (unsigned long)value.longLongValue);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned long)]];
				free(buffer);
				break;
			}
			case PTPDataTypeCodeUnicodeString: {
				unsigned char *buffer = malloc(256);
				unsigned char *buf = buffer;
				int length = ptpWriteString(&buf, value);
				[self sendPTPRequest:PTPOperationCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:length]];
				free(buffer);
				break;
			}
		}
	}
}

-(void)startLiveView {
	PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
	switch (info.vendorExtensionID) {
		case PTPVendorExtensionNikon:
			[self sendPTPRequest:PTPOperationCodeNikonStartLiveView];
			break;
	}
}

-(void)stopLiveView {
	PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
	switch (info.vendorExtensionID) {
		case PTPVendorExtensionNikon:
			[self sendPTPRequest:PTPOperationCodeNikonEndLiveView];
			break;
	}
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtensionID];
  request.operationCode = operationCode;
  request.numberOfParameters = 0;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtensionID];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 withData:(NSData *)data {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtensionID];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [self requestSendPTPCommand:request.commandBuffer outData:data sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation PTPDelegate {
  ICDeviceBrowser* deviceBrowser;
}

- (void)deviceBrowser:(ICDeviceBrowser*)browser didAddDevice:(ICDevice*)camera moreComing:(BOOL)moreComing {
  camera.delegate = self;
  [self cameraAdded:(ICCameraDevice *)camera];
}

- (void)deviceBrowser:(ICDeviceBrowser*)browser didRemoveDevice:(ICDevice*)camera moreGoing:(BOOL)moreGoing {
  [self cameraRemoved:(ICCameraDevice *)camera];
}

- (void)device:(ICDevice*)camera didOpenSessionWithError:(NSError*)error {
  [(ICCameraDevice *)camera sendPTPRequest:PTPOperationCodeGetDeviceInfo];
}

- (void)device:(ICDevice*)camera didCloseSessionWithError:(NSError*)error {
  [self cameraDisconnected:(ICCameraDevice *)camera];
}

- (void)device:(ICDevice*)camera didEncounterError:(NSError*)error {
  NSLog(@"device:'%@' didEncounterError:'%@'", camera.name, error.localizedDescription);
}

- (void)cameraDevice:(ICCameraDevice*)camera didAddItem:(ICCameraItem*)item {
  if (item.class == ICCameraFile.class) {
    ICCameraFile *file = (ICCameraFile *)item;
    if (file.wasAddedAfterContentCatalogCompleted)
      [camera requestDownloadFile:file options:@{ ICDeleteAfterSuccessfulDownload: @TRUE, ICOverwrite: @TRUE, ICDownloadsDirectoryURL: [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:true] } downloadDelegate:self didDownloadSelector:@selector(didDownloadFile:error:options:contextInfo:) contextInfo:nil];
  }
}

- (void)didDownloadFile:(ICCameraFile*)file error:(nullable NSError*)error options:(nullable NSDictionary<NSString*,id>*)options contextInfo:(nullable void*)contextInfo {
  ICCameraDevice *camera = file.device;
  if (error == nil) {
    NSURL *folder = options[ICDownloadsDirectoryURL];
    NSString *name = options[ICSavedFilename];
    if (folder != nil && name != nil) {
      NSURL *url = [NSURL URLWithString:name relativeToURL:folder];
      NSData *data = [NSData dataWithContentsOfURL:url];
			[self cameraExposureDone:camera data:data filename:name];
      [NSFileManager.defaultManager removeItemAtURL:url error:nil];
      return;
    }
  }
  [self cameraExposureFailed:camera];
}


-(void)didRemoveDevice:(ICDevice *)device {
}

- (void)cameraDevice:(ICCameraDevice*)camera didReceivePTPEvent:(NSData*)eventData {
  NSLog(@"Received %@", [[PTPEvent alloc] initWithData:eventData vendorExtension:camera.ptpDeviceInfo.vendorExtensionID]);
}

@end

//------------------------------------------------------------------------------------------------------------------------------
