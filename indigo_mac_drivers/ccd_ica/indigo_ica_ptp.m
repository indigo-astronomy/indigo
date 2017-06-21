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

- (NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], _operationCode];
  if (_numberOfParameters > 0)
    [s appendFormat:@"0x%08X", _parameter1];
  if (_numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", _parameter2];
  if (_numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", _parameter3];
  if (_numberOfParameters > 3)
    [s appendFormat:@", 0x%08X", _parameter4];
  if (_numberOfParameters > 4)
    [s appendFormat:@", 0x%08X", _parameter5];
  [s appendString:@"]"];
  return s;
}

- (id)init {
  self = [super init];
  if (self) {
    _numberOfParameters = 0;
  }
  return self;
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

- (NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], _responseCode];
  if (_numberOfParameters > 0)
    [s appendFormat:@"0x%08X", _parameter1];
  if (_numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", _parameter2];
  if (_numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", _parameter3];
  if (_numberOfParameters > 3)
    [s appendFormat:@", 0x%08X", _parameter4];
  if (_numberOfParameters > 4)
    [s appendFormat:@", 0x%08X", _parameter5];
  [s appendString:@"]"];
  return s;
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

- (NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], _eventCode];
  if (_numberOfParameters > 0)
    [s appendFormat:@"0x%08X", _parameter1];
  if (_numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", _parameter2];
  if (_numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", _parameter3];
  [s appendString:@"]"];
  return s;
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
    _writable = ptpReadUnsignedChar(&buf);
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

- (BOOL)isRedefinedFrom:(PTPProperty *)other {
  if (_type != other.type)
    return true;
  if (_writable != other.writable)
    return true;
  if ((_min == nil && other.min != nil) || (_min != nil && other.min == nil))
    return true;
  if (_min && [_min isNotEqualTo:other.min])
    return true;
  if ((_max == nil && other.max != nil) || (_max != nil && other.max == nil))
    return true;
  if (_max && [_max isNotEqualTo:other.max])
    return true;
  if ((_step == nil && other.step != nil) || (_step != nil && other.step == nil))
    return true;
  if (_step && [_step isNotEqualTo:other.step])
    return true;
  if ((_supportedValues == nil && other.supportedValues != nil) || (_supportedValues != nil && other.supportedValues == nil))
    return true;
  if (_supportedValues && ![_supportedValues isEqualToArray:other.supportedValues])
    return true;
  return false;
}

- (NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x 0x%04x %@", [self class], _propertyCode, _type, _writable ? @"rw" : @"ro"];
  if (_min)
    [s appendFormat:@", min = %@", _min];
  if (_max)
    [s appendFormat:@", max = %@", _max];
  if (_step)
    [s appendFormat:@", step = %@", _step];
  if (_supportedValues) {
    [s appendFormat:@", values = [%@", _supportedValues.firstObject];
    for (int i = 1; i < _supportedValues.count; i++)
      [s appendFormat:@", %@", [_supportedValues objectAtIndex:i]];
    [s appendString:@"]"];
  }
  [s appendFormat:@", default = %@", _defaultValue];
  [s appendFormat:@", value = %@", _value];
  return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPDeviceInfo

@implementation PTPDeviceInfo

- (id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  if ((self = [super init])) {
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

- (NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ <%p>:\n", [self class], self];
  [s appendFormat:@"Standard version: %.2f\n", _standardVersion / 100.0];
  [s appendFormat:@"Vendor extension: 0x%08x (%.2f) '%@'\n", _vendorExtensionID, _vendorExtensionVersion / 100.0, _vendorExtensionDesc];
  [s appendFormat:@"Functional mode:  0x%04x\n", _functionalMode];
  [s appendFormat:@"Manufacturer:     %@\n", _manufacturer];
  [s appendFormat:@"Model:            %@ %@\n", _model, _version];
  [s appendFormat:@"Serial:           %@\n", _serial];
  [s appendFormat:@"Operations:       [%04x", _operationsSupported.firstObject.unsignedShortValue];
  for (int i = 1; i < _operationsSupported.count; i++)
    [s appendFormat:@", %04x", [_operationsSupported objectAtIndex:i].unsignedShortValue];
  [s appendFormat:@"]\nEvents:           [%04x", _eventsSupported.firstObject.unsignedShortValue];
  for (int i = 1; i < _eventsSupported.count; i++)
    [s appendFormat:@", %04x", [_eventsSupported objectAtIndex:i].unsignedShortValue];
  [s appendFormat:@"]\nProperties:       [%04x", _propertiesSupported.firstObject.unsignedShortValue];
  for (int i = 1; i < _propertiesSupported.count; i++)
    [s appendFormat:@", %04x", [_propertiesSupported objectAtIndex:i].unsignedShortValue];
  [s appendString:@"]\n"];
  return s;
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation ICCameraDevice(PTPExtensions)

- (void)dumpData:(void*)data length:(int)length comment:(NSString*)comment {
  UInt32	i, j;
  UInt8*  p;
  char	fStr[80];
  char*   fStrP = NULL;
  NSMutableString*  s = [NSMutableString stringWithFormat:@"\n  %@ [%d bytes]:\n\n", comment, length];
  p = (UInt8*)data;
  for ( i = 0; i < length; i++ ) {
    if ( (i % 16) == 0 ) {
      fStrP = fStr;
      fStrP += snprintf( fStrP, 10, "    %4X:", (unsigned int)i );
    }
    if ( (i % 4) == 0 )
      fStrP += snprintf( fStrP, 2, " " );
    fStrP += snprintf( fStrP, 3, "%02X", (UInt8)(*(p+i)) );
    if ( (i % 16) == 15 ) {
      fStrP += snprintf( fStrP, 2, " " );
      for ( j = i-15; j <= i; j++ ) {
        if ( *(p+j) < 32 || *(p+j) > 126 )
          fStrP += snprintf( fStrP, 2, "." );
        else
          fStrP += snprintf( fStrP, 2, "%c", *(p+j) );
      }
      [s appendFormat:@"%s\n", fStr];
    }
    if ( (i % 256) == 255 )
      [s appendString:@"\n"];
  }
  if ( (i % 16) ) {
    for ( j = (i % 16); j < 16; j ++ )
    {
      fStrP += snprintf( fStrP, 3, "  " );
      if ( (j % 4) == 0 )
        fStrP += snprintf( fStrP, 2, " " );
    }
    fStrP += snprintf( fStrP, 2, " " );
    for ( j = i - (i % 16 ); j < length; j++ ) {
      if ( *(p+j) < 32 || *(p+j) > 126 )
        fStrP += snprintf( fStrP, 2, "." );
      else
        fStrP += snprintf( fStrP, 2, "%c", *(p+j) );
    }
    for ( j = (i % 16); j < 16; j ++ ) {
      fStrP += snprintf( fStrP, 2, " " );
    }
    [s appendFormat:@"%s\n", fStr];
  }
  [s appendString:@"\n"];
  NSLog(@"%@", s);
}

- (void)checkForEvent {
  PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
  if (info.vendorExtensionID == PTPVendorExtensionNikon) {
    [self sendPTPRequest:PTPOperationCodeNikonCheckEvent];
  }
}

- (void)processEvent:(PTPEvent *)event {
  if (event.eventCode == PTPEventCodeDevicePropChanged) {
    [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:event.parameter1];
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
        NSLog(@"\nDevice info: %@\n", info);
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
    if (ptpRequest.operationCode == PTPOperationCodeGetDevicePropDesc && ptpResponse.responseCode == PTPResponseCodeDevicePropNotSupported) {
      [(NSMutableDictionary *)info.propertiesSupported removeObjectForKey:[NSNumber numberWithUnsignedShort:ptpRequest.parameter1]];
    } else if (indigo_get_log_level() >= INDIGO_LOG_TRACE || ptpResponse.responseCode != PTPResponseCodeOK)
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
        [self sendPTPRequest:PTPOperationCodeGetStorageIDs];
        break;
      }
      case PTPOperationCodeGetDevicePropDesc: {
        PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
        PTPProperty *property = [[PTPProperty alloc] initWithData:data vendorExtension:info.vendorExtensionID];
        switch (property.propertyCode) {
          case PTPPropertyCodeExposureProgramMode: {
            NSDictionary *labels = @{ @1: @"Manual", @2: @"Program", @3: @"Aperture priority", @4: @"Shutter priority", @32784: @"Auto", @32785: @"Portrait", @32786: @"Landscape", @32787:@"Macro", @32788: @"Sport", @32789: @"Night portrait", @32790:@"Night landscape", @32791: @"Child", @32792: @"Scene", @32793: @"Effects" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              NSString *label = labels[number];
              if (label)
                supportedValues[number] = labels[number];
              else
                supportedValues[number] = [NSString stringWithFormat:@"mode %@", number];
            }
            [(PTPDelegate *)self.delegate cameraExposureProgramChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeFNumber: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              supportedValues[number] = [NSString stringWithFormat:@"f/%g", number.intValue / 100.0];
            }
            [(PTPDelegate *)self.delegate cameraApertureChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeExposureTime: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              int value = number.intValue;
              if (value == -1)
                supportedValues[number] = [NSString stringWithFormat:@"Bulb"];
              else if (value == 1)
                supportedValues[number] = [NSString stringWithFormat:@"1/8000 s"];
              else if (value == 3)
                supportedValues[number] = [NSString stringWithFormat:@"1/3200 s"];
              else if (value == 6)
                supportedValues[number] = [NSString stringWithFormat:@"1/1600 s"];
              else if (value == 12)
                supportedValues[number] = [NSString stringWithFormat:@"1/800 s"];
              else if (value == 15)
                supportedValues[number] = [NSString stringWithFormat:@"1/640 s"];
              else if (value == 80)
                supportedValues[number] = [NSString stringWithFormat:@"1/125 s"];
              else if (value < 100)
                supportedValues[number] = [NSString stringWithFormat:@"1/%g s", round(1000.0 / number.intValue) * 10];
              else if (value < 10000)
                supportedValues[number] = [NSString stringWithFormat:@"1/%g s", round(10000.0 / number.intValue)];
              else
                supportedValues[number] = [NSString stringWithFormat:@"%g s", number.intValue / 10000.0];
            }
            [(PTPDelegate *)self.delegate cameraShutterChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeImageSize: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSString *string in property.supportedValues) {
              supportedValues[string] = string;
            }
            [(PTPDelegate *)self.delegate cameraImageSizeChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeCompressionSetting: {
            NSDictionary *labels = @{ @0: @"JPEG Basic", @1: @"JPEG Norm", @2: @"JPEG Fine", @4: @"RAW", @5: @"RAW + JPEG Basic", @6: @"RAW + JPEG Norm", @7: @"RAW + JPEG Fine" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              NSString *label = labels[number];
              if (label)
                supportedValues[number] = labels[number];
              else
                supportedValues[number] = [NSString stringWithFormat:@"compression %@", number];
            }
            [(PTPDelegate *)self.delegate cameraCompressionChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeWhiteBalance: {
            NSDictionary *labels = @{ @1: @"Manual", @2: @"Auto", @3: @"One-push Auto", @4: @"Daylight", @5: @"Fluorescent", @6: @"Incandescent", @7: @"Flash", @32784: @"Cloudy", @32785: @"Shade", @32786: @"Color Temperature", @32787: @"Preset" };
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              NSString *label = labels[number];
              if (label)
                supportedValues[number] = labels[number];
              else
                supportedValues[number] = [NSString stringWithFormat:@"compression %@", number];
            }
            [(PTPDelegate *)self.delegate cameraWhiteBalanceChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
          }
          case PTPPropertyCodeExposureIndex: {
            NSMutableDictionary *supportedValues = [NSMutableDictionary dictionary];
            for (NSNumber *number in property.supportedValues) {
              supportedValues[number] = [NSString stringWithFormat:@"%d", number.intValue];
            }
            [(PTPDelegate *)self.delegate cameraISOChanged:self value:property.value supportedValues:supportedValues readOnly:!property.writable];
            break;
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
          [self processEvent:event];
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

-(void)sendPTPRequest:(PTPOperationCode)operationCode {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtensionID];
  request.operationCode = operationCode;
  request.numberOfParameters = 0;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtensionID];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:CFBridgingRetain(request)];
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
  NSLog(@"device:%@ didEncounterError:%@", camera, error);
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
      [self cameraExposureDone:camera data:data];
      [NSFileManager.defaultManager removeItemAtURL:url error:nil];
      return;
    }
  }
  [self cameraExposureFailed:camera];
}


-(void)didRemoveDevice:(ICDevice *)device {
}

- (void)cameraDevice:(ICCameraDevice*)camera didReceivePTPEvent:(NSData*)eventData {
  NSLog(@"Received event: %@", [[PTPEvent alloc] initWithData:eventData vendorExtension:camera.ptpDeviceInfo.vendorExtensionID]);
}

@end

//------------------------------------------------------------------------------------------------------------------------------
