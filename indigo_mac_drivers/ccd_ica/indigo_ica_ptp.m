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
#import "indigo_ica_ptp_nikon.h"
#import "indigo_ica_ptp_canon.h"

char ptpReadChar(unsigned char** buf) {
  char value = *(SInt8*)(*buf);
  (*buf) += 1;
  return value;
}

void ptpWriteChar(unsigned char** buf, char value) {
  *(SInt8*)(*buf) = value;
  (*buf) += 1;
}

unsigned char ptpReadUnsignedChar(unsigned char** buf) {
  char value = *(UInt8*)(*buf);
  (*buf) += 1;
  return value;
}

void ptpWriteUnsignedChar(unsigned char** buf, unsigned char value) {
  *(UInt8*)(*buf) = value;
  (*buf) += 1;
}

short ptpReadShort(unsigned char** buf) {
  SInt16 value = (SInt16)CFSwapInt16LittleToHost(*(UInt16*)(*buf));
  (*buf) += 2;
  return value;
}

void ptpWriteShort(unsigned char** buf, short value) {
  *(SInt16*)(*buf) = (SInt16)CFSwapInt16HostToLittle(value);
  (*buf) += 2;
}

unsigned short ptpReadUnsignedShort(unsigned char** buf) {
  unsigned short value = CFSwapInt16LittleToHost(*(unsigned short*)(*buf));
  (*buf) += 2;
  return value;
}

void ptpWriteUnsignedShort(unsigned char** buf, unsigned short value) {
  *(unsigned short*)(*buf) = CFSwapInt16HostToLittle(value);
  (*buf) += 2;
}

int ptpReadInt(unsigned char** buf) {
  int value = CFSwapInt32LittleToHost(*(int*)(*buf));
  (*buf) += 4;
  return value;
}

void ptpWriteInt(unsigned char** buf, int value) {
  *(int*)(*buf) = (int)CFSwapInt32HostToLittle(value);
  (*buf) += 4;
}

unsigned int ptpReadUnsignedInt(unsigned char** buf) {
  int value = CFSwapInt32LittleToHost(*(int*)(*buf));
  (*buf) += 4;
  return value;
}

void ptpWriteUnsignedInt(unsigned char** buf, unsigned int value) {
  *(unsigned int*)(*buf) = CFSwapInt32HostToLittle(value);
  (*buf) += 4;
}

long ptpReadLong(unsigned char** buf) {
  long value = (long)CFSwapInt64LittleToHost(*(long*)(*buf));
  (*buf) += 8;
  return value;
}

void ptpWriteLong(unsigned char** buf, long value) {
  *(long*)(*buf) = (long)CFSwapInt64HostToLittle(value);
  (*buf) += 8;
}

unsigned long ptpReadUnsignedLong(unsigned char** buf) {
  unsigned long value = CFSwapInt64LittleToHost(*(unsigned long*)(*buf));
  (*buf) += 8;
  return value;
}

void ptpWriteUnsignedLong(unsigned char** buf, unsigned long value) {
  *(unsigned long*)(*buf) = CFSwapInt64HostToLittle(value);
  (*buf) += 8;
}

NSString *ptpRead128(unsigned char** buf) {
  NSString *value = [NSString stringWithFormat:@"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", (*buf)[0], (*buf)[1], (*buf)[2], (*buf)[3], (*buf)[4], (*buf)[5], (*buf)[6], (*buf)[7], (*buf)[8], (*buf)[9], (*buf)[10], (*buf)[11], (*buf)[12], (*buf)[13], (*buf)[14], (*buf)[15]];
  (*buf) += 16;
  return value;
}

NSArray<NSNumber *> *ptpReadCharArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithChar:ptpReadChar(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadUnsignedCharArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedChar:ptpReadUnsignedChar(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadShortArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithShort:ptpReadShort(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadUnsignedShortArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedShort:ptpReadUnsignedShort(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadIntArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithInt:ptpReadInt(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadUnsignedIntArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedInt:ptpReadUnsignedInt(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadLongArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithLong:ptpReadLong(buf)]];
    return result;
  }
  return nil;
}

NSArray<NSNumber *> *ptpReadUnsignedLongArray(unsigned char** buf) {
  int length = ptpReadUnsignedInt(buf);
  if (length) {
    NSMutableArray<NSNumber *> *result = [NSMutableArray array];
    for (int i = 0; i < length; i++)
      [result addObject:[NSNumber numberWithUnsignedLong:ptpReadUnsignedLong(buf)]];
    return result;
  }
  return nil;
}

NSString *ptpReadString(unsigned char** buf) {
  int length = **buf;
  if (length) {
    int fixed;
    const unichar *pnt = (const unichar *)(*buf + 1);
    for (fixed = 0; fixed < length; fixed++) {
      if (*pnt++ == 0) {
        break;
      }
    }
    NSString *result = [NSString stringWithString:[NSString stringWithCharacters: (const unichar *)(*buf + 1) length:fixed]];
    *buf = (*buf) + length * 2 + 1;
    return result;
  }
  *buf = (*buf) + 1;
  return nil;
}

unsigned int ptpWriteString(unsigned char **buf, NSString *value) {
  const char *cstr = [value cStringUsingEncoding:NSUnicodeStringEncoding];
  unsigned int length = (unsigned int)value.length + 1;
  if (length < 256) {
    **buf = length;
    memcpy(*buf + 1, cstr, 2 * length);
    *buf = (*buf) + 2 * length + 1;
    return 2 * length + 1;
  }
  return 0;
}

NSObject *ptpReadValue(PTPDataTypeCode type, unsigned char **buf) {
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
    case PTPDataTypeCodeSInt128:
    case PTPDataTypeCodeUInt128:
      return ptpRead128(buf);
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


//---------------------------------------------------------------------------------------------------------- PTPVendor

@implementation PTPVendor

+ (NSString *)vendorExtensionName:(PTPVendorExtension)vendorExtension {
  switch (vendorExtension) {
    case PTPVendorExtensionEastmanKodak: return @"PTPVendorExtensionEastmanKodak";
    case PTPVendorExtensionMicrosoft: return @"PTPVendorExtensionMicrosoft";
    case PTPVendorExtensionNikon: return @"PTPVendorExtensionNikon";
    case PTPVendorExtensionCanon: return @"PTPVendorExtensionCanon";
    case PTPVendorExtensionPentax: return @"PTPVendorExtensionPentax";
    case PTPVendorExtensionFuji: return @"PTPVendorExtensionFuji";
    case PTPVendorExtensionSony: return @"PTPVendorExtensionSony";
    case PTPVendorExtensionSamsung: return @"PTPVendorExtensionSamsung";
  }
  return [NSString stringWithFormat:@"PTPVendorExtension0x%04x", vendorExtension];
}

-(id)initWithVendorExtension:(PTPVendorExtension)vendorExtension {
  self = [super init];
  if (self) {
    _vendorExtension = vendorExtension;
  }
  return self;
}

-(NSString *)vendorExtensionName {
  return [PTPVendor vendorExtensionName:_vendorExtension];
}

@end

//---------------------------------------------------------------------------------------------------------- PTPRequest

@implementation PTPRequest

+ (NSString *)operationCodeName:(PTPRequestCode)operationCode {
  switch (operationCode) {
    case PTPRequestCodeUndefined: return @"PTPRequestCodeUndefined";
    case PTPRequestCodeGetDeviceInfo: return @"PTPRequestCodeGetDeviceInfo";
    case PTPRequestCodeOpenSession: return @"PTPRequestCodeOpenSession";
    case PTPRequestCodeCloseSession: return @"PTPRequestCodeCloseSession";
    case PTPRequestCodeGetStorageIDs: return @"PTPRequestCodeGetStorageIDs";
    case PTPRequestCodeGetStorageInfo: return @"PTPRequestCodeGetStorageInfo";
    case PTPRequestCodeGetNumObjects: return @"PTPRequestCodeGetNumObjects";
    case PTPRequestCodeGetObjectHandles: return @"PTPRequestCodeGetObjectHandles";
    case PTPRequestCodeGetObjectInfo: return @"PTPRequestCodeGetObjectInfo";
    case PTPRequestCodeGetObject: return @"PTPRequestCodeGetObject";
    case PTPRequestCodeGetThumb: return @"PTPRequestCodeGetThumb";
    case PTPRequestCodeDeleteObject: return @"PTPRequestCodeDeleteObject";
    case PTPRequestCodeSendObjectInfo: return @"PTPRequestCodeSendObjectInfo";
    case PTPRequestCodeSendObject: return @"PTPRequestCodeSendObject";
    case PTPRequestCodeInitiateCapture: return @"PTPRequestCodeInitiateCapture";
    case PTPRequestCodeFormatStore: return @"PTPRequestCodeFormatStore";
    case PTPRequestCodeResetDevice: return @"PTPRequestCodeResetDevice";
    case PTPRequestCodeSelfTest: return @"PTPRequestCodeSelfTest";
    case PTPRequestCodeSetObjectProtection: return @"PTPRequestCodeSetObjectProtection";
    case PTPRequestCodePowerDown: return @"PTPRequestCodePowerDown";
    case PTPRequestCodeGetDevicePropDesc: return @"PTPRequestCodeGetDevicePropDesc";
    case PTPRequestCodeGetDevicePropValue: return @"PTPRequestCodeGetDevicePropValue";
    case PTPRequestCodeSetDevicePropValue: return @"PTPRequestCodeSetDevicePropValue";
    case PTPRequestCodeResetDevicePropValue: return @"PTPRequestCodeResetDevicePropValue";
    case PTPRequestCodeTerminateOpenCapture: return @"PTPRequestCodeTerminateOpenCapture";
    case PTPRequestCodeMoveObject: return @"PTPRequestCodeMoveObject";
    case PTPRequestCodeCopyObject: return @"PTPRequestCodeCopyObject";
    case PTPRequestCodeGetPartialObject: return @"PTPRequestCodeGetPartialObject";
    case PTPRequestCodeInitiateOpenCapture: return @"PTPRequestCodeInitiateOpenCapture";
    case PTPRequestCodeGetNumDownloadableObjects: return @"PTPRequestCodeGetNumDownloadableObjects";
    case PTPRequestCodeGetAllObjectInfo: return @"PTPRequestCodeGetAllObjectInfo";
    case PTPRequestCodeGetUserAssignedDeviceName: return @"PTPRequestCodeGetUserAssignedDeviceName";
    case PTPRequestCodeMTPGetObjectPropsSupported: return @"PTPRequestCodeMTPGetObjectPropsSupported";
    case PTPRequestCodeMTPGetObjectPropDesc: return @"PTPRequestCodeMTPGetObjectPropDesc";
    case PTPRequestCodeMTPGetObjectPropValue: return @"PTPRequestCodeMTPGetObjectPropValue";
    case PTPRequestCodeMTPSetObjectPropValue: return @"PTPRequestCodeMTPSetObjectPropValue";
    case PTPRequestCodeMTPGetObjPropList: return @"PTPRequestCodeMTPGetObjPropList";
    case PTPRequestCodeMTPSetObjPropList: return @"PTPRequestCodeMTPSetObjPropList";
    case PTPRequestCodeMTPGetInterdependendPropdesc: return @"PTPRequestCodeMTPGetInterdependendPropdesc";
    case PTPRequestCodeMTPSendObjectPropList: return @"PTPRequestCodeMTPSendObjectPropList";
    case PTPRequestCodeMTPGetObjectReferences: return @"PTPRequestCodeMTPGetObjectReferences";
    case PTPRequestCodeMTPSetObjectReferences: return @"PTPRequestCodeMTPSetObjectReferences";
    case PTPRequestCodeMTPUpdateDeviceFirmware: return @"PTPRequestCodeMTPUpdateDeviceFirmware";
    case PTPRequestCodeMTPSkip: return @"PTPRequestCodeMTPSkip";
  }
  return [NSString stringWithFormat:@"PTPRequestCode0x%04x", operationCode];
}

-(id)init {
  self = [super init];
  if (self) {
    _numberOfParameters = 0;
  }
  return self;
}

-(NSData*)commandBuffer {
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

-(NSString *)operationCodeName {
  return [PTPRequest operationCodeName:_operationCode];
}

-(NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@", self.operationCodeName];
  if (self.numberOfParameters > 0)
    [s appendFormat:@"[ 0x%08X", self.parameter1];
  if (self.numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", self.parameter2];
  if (self.numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", self.parameter3];
  if (self.numberOfParameters > 3)
    [s appendFormat:@", 0x%08X", self.parameter4];
  if (self.numberOfParameters > 4)
    [s appendFormat:@", 0x%08X", self.parameter5];
  if (self.numberOfParameters > 0)
    [s appendString:@"]"];
  return s;
}

@end

//--------------------------------------------------------------------------------------------------------- PTPResponse

@implementation PTPResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode {
  switch (responseCode) {
    case PTPResponseCodeUndefined: return @"PTPResponseCodeUndefined";
    case PTPResponseCodeOK: return @"PTPResponseCodeOK";
    case PTPResponseCodeGeneralError: return @"PTPResponseCodeGeneralError";
    case PTPResponseCodeSessionNotOpen: return @"PTPResponseCodeSessionNotOpen";
    case PTPResponseCodeInvalidTransactionID: return @"PTPResponseCodeInvalidTransactionID";
    case PTPResponseCodeOperationNotSupported: return @"PTPResponseCodeOperationNotSupported";
    case PTPResponseCodeParameterNotSupported: return @"PTPResponseCodeParameterNotSupported";
    case PTPResponseCodeIncompleteTransfer: return @"PTPResponseCodeIncompleteTransfer";
    case PTPResponseCodeInvalidStorageID: return @"PTPResponseCodeInvalidStorageID";
    case PTPResponseCodeInvalidObjectHandle: return @"PTPResponseCodeInvalidObjectHandle";
    case PTPResponseCodeDevicePropNotSupported: return @"PTPResponseCodeDevicePropNotSupported";
    case PTPResponseCodeInvalidObjectFormatCode: return @"PTPResponseCodeInvalidObjectFormatCode";
    case PTPResponseCodeStoreFull: return @"PTPResponseCodeStoreFull";
    case PTPResponseCodeObjectWriteProtected: return @"PTPResponseCodeObjectWriteProtected";
    case PTPResponseCodeStoreReadOnly: return @"PTPResponseCodeStoreReadOnly";
    case PTPResponseCodeAccessDenied: return @"PTPResponseCodeAccessDenied";
    case PTPResponseCodeNoThumbnailPresent: return @"PTPResponseCodeNoThumbnailPresent";
    case PTPResponseCodeSelfTestFailed: return @"PTPResponseCodeSelfTestFailed";
    case PTPResponseCodePartialDeletion: return @"PTPResponseCodePartialDeletion";
    case PTPResponseCodeStoreNotAvailable: return @"PTPResponseCodeStoreNotAvailable";
    case PTPResponseCodeSpecificationByFormatUnsupported: return @"PTPResponseCodeSpecificationByFormatUnsupported";
    case PTPResponseCodeNoValidObjectInfo: return @"PTPResponseCodeNoValidObjectInfo";
    case PTPResponseCodeInvalidCodeFormat: return @"PTPResponseCodeInvalidCodeFormat";
    case PTPResponseCodeUnknownVendorCode: return @"PTPResponseCodeUnknownVendorCode";
    case PTPResponseCodeCaptureAlreadyTerminated: return @"PTPResponseCodeCaptureAlreadyTerminated";
    case PTPResponseCodeDeviceBusy: return @"PTPResponseCodeDeviceBusy";
    case PTPResponseCodeInvalidParentObject: return @"PTPResponseCodeInvalidParentObject";
    case PTPResponseCodeInvalidDevicePropFormat: return @"PTPResponseCodeInvalidDevicePropFormat";
    case PTPResponseCodeInvalidDevicePropValue: return @"PTPResponseCodeInvalidDevicePropValue";
    case PTPResponseCodeInvalidParameter: return @"PTPResponseCodeInvalidParameter";
    case PTPResponseCodeSessionAlreadyOpen: return @"PTPResponseCodeSessionAlreadyOpen";
    case PTPResponseCodeTransactionCancelled: return @"PTPResponseCodeTransactionCancelled";
    case PTPResponseCodeSpecificationOfDestinationUnsupported: return @"PTPResponseCodeSpecificationOfDestinationUnsupported";
    case PTPResponseCodeMTPUndefined: return @"PTPResponseCodeMTPUndefined";
    case PTPResponseCodeMTPInvalidObjectPropCode: return @"PTPResponseCodeMTPInvalidObjectPropCode";
    case PTPResponseCodeMTPInvalidObjectProp_Format: return @"PTPResponseCodeMTPInvalidObjectProp_Format";
    case PTPResponseCodeMTPInvalidObjectProp_Value: return @"PTPResponseCodeMTPInvalidObjectProp_Value";
    case PTPResponseCodeMTPInvalidObjectReference: return @"PTPResponseCodeMTPInvalidObjectReference";
    case PTPResponseCodeMTPInvalidDataset: return @"PTPResponseCodeMTPInvalidDataset";
    case PTPResponseCodeMTPSpecificationByGroupUnsupported: return @"PTPResponseCodeMTPSpecificationByGroupUnsupported";
    case PTPResponseCodeMTPSpecificationByDepthUnsupported: return @"PTPResponseCodeMTPSpecificationByDepthUnsupported";
    case PTPResponseCodeMTPObjectTooLarge: return @"PTPResponseCodeMTPObjectTooLarge";
    case PTPResponseCodeMTPObjectPropNotSupported: return @"PTPResponseCodeMTPObjectPropNotSupported";
  }
  return [NSString stringWithFormat:@"PTPResponseCode0x%04x", responseCode];
}

-(id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 32))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 32 || type != 3)
    return NULL;
  if (self = [super init]) {
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

-(NSString *)responseCodeName {
  return [PTPResponse responseCodeName:_responseCode];
}

-(NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@", self.responseCodeName];
  if (self.numberOfParameters > 0)
    [s appendFormat:@"[ 0x%08X", self.parameter1];
  if (self.numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", self.parameter2];
  if (self.numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", self.parameter3];
  if (self.numberOfParameters > 3)
    [s appendFormat:@", 0x%08X", self.parameter4];
  if (self.numberOfParameters > 4)
    [s appendFormat:@", 0x%08X", self.parameter5];
  if (self.numberOfParameters > 0)
    [s appendString:@"]"];
  return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPEvent

@implementation PTPEvent

+ (NSString *)eventCodeName:(PTPEventCode)eventCode {
  switch (eventCode) {
    case PTPEventCodeUndefined: return @"PTPEventCodeUndefined";
    case PTPEventCodeCancelTransaction: return @"PTPEventCodeCancelTransaction";
    case PTPEventCodeObjectAdded: return @"PTPEventCodeObjectAdded";
    case PTPEventCodeObjectRemoved: return @"PTPEventCodeObjectRemoved";
    case PTPEventCodeStoreAdded: return @"PTPEventCodeStoreAdded";
    case PTPEventCodeStoreRemoved: return @"PTPEventCodeStoreRemoved";
    case PTPEventCodeDevicePropChanged: return @"PTPEventCodeDevicePropChanged";
    case PTPEventCodeObjectInfoChanged: return @"PTPEventCodeObjectInfoChanged";
    case PTPEventCodeDeviceInfoChanged: return @"PTPEventCodeDeviceInfoChanged";
    case PTPEventCodeRequestObjectTransfer: return @"PTPEventCodeRequestObjectTransfer";
    case PTPEventCodeStoreFull: return @"PTPEventCodeStoreFull";
    case PTPEventCodeDeviceReset: return @"PTPEventCodeDeviceReset";
    case PTPEventCodeStorageInfoChanged: return @"PTPEventCodeStorageInfoChanged";
    case PTPEventCodeCaptureComplete: return @"PTPEventCodeCaptureComplete";
    case PTPEventCodeUnreportedStatus: return @"PTPEventCodeUnreportedStatus";
    case PTPEventCodeAppleDeviceUnlocked: return @"PTPEventCodeAppleDeviceUnlocked";
    case PTPEventCodeAppleUserAssignedNameChanged: return @"PTPEventCodeAppleUserAssignedNameChanged";
  }
  return [NSString stringWithFormat:@"PTPEventCodeCode0x%04x", eventCode];
}

-(id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 24))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 24 || type != 4)
    return NULL;
  if (self = [super init]) {
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

-(id)initWithCode:(PTPEventCode)eventCode parameter1:(unsigned int)parameter1 {
  self = [super init];
  if (self) {
    _eventCode = eventCode;
    _numberOfParameters = 1;
    _parameter1 = parameter1;
  }
  return self;
}

-(NSString *)eventCodeName {
  return [PTPEvent eventCodeName:_eventCode];
}

-(NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@", self.eventCodeName];
  if (self.numberOfParameters > 0)
    [s appendFormat:@"[ 0x%08X", self.parameter1];
  if (self.numberOfParameters > 1)
    [s appendFormat:@", 0x%08X", self.parameter2];
  if (self.numberOfParameters > 2)
    [s appendFormat:@", 0x%08X", self.parameter3];
  if (self.numberOfParameters > 0)
    [s appendString:@"]"];
  return s;
}

@end

//------------------------------------------------------------------------------------------------------------------------------ PTPProperty

@implementation PTPProperty

+ (NSString *)propertyCodeName:(PTPPropertyCode)propertyCode {
  switch (propertyCode) {
    case PTPPropertyCodeUndefined: return @"PTPPropertyCodeUndefined";
    case PTPPropertyCodeBatteryLevel: return @"PTPPropertyCodeBatteryLevel";
    case PTPPropertyCodeFunctionalMode: return @"PTPPropertyCodeFunctionalMode";
    case PTPPropertyCodeImageSize: return @"PTPPropertyCodeImageSize";
    case PTPPropertyCodeCompressionSetting: return @"PTPPropertyCodeCompressionSetting";
    case PTPPropertyCodeWhiteBalance: return @"PTPPropertyCodeWhiteBalance";
    case PTPPropertyCodeRGBGain: return @"PTPPropertyCodeRGBGain";
    case PTPPropertyCodeFNumber: return @"PTPPropertyCodeFNumber";
    case PTPPropertyCodeFocalLength: return @"PTPPropertyCodeFocalLength";
    case PTPPropertyCodeFocusDistance: return @"PTPPropertyCodeFocusDistance";
    case PTPPropertyCodeFocusMode: return @"PTPPropertyCodeFocusMode";
    case PTPPropertyCodeExposureMeteringMode: return @"PTPPropertyCodeExposureMeteringMode";
    case PTPPropertyCodeFlashMode: return @"PTPPropertyCodeFlashMode";
    case PTPPropertyCodeExposureTime: return @"PTPPropertyCodeExposureTime";
    case PTPPropertyCodeExposureProgramMode: return @"PTPPropertyCodeExposureProgramMode";
    case PTPPropertyCodeExposureIndex: return @"PTPPropertyCodeExposureIndex";
    case PTPPropertyCodeExposureBiasCompensation: return @"PTPPropertyCodeExposureBiasCompensation";
    case PTPPropertyCodeDateTime: return @"PTPPropertyCodeDateTime";
    case PTPPropertyCodeCaptureDelay: return @"PTPPropertyCodeCaptureDelay";
    case PTPPropertyCodeStillCaptureMode: return @"PTPPropertyCodeStillCaptureMode";
    case PTPPropertyCodeContrast: return @"PTPPropertyCodeContrast";
    case PTPPropertyCodeSharpness: return @"PTPPropertyCodeSharpness";
    case PTPPropertyCodeDigitalZoom: return @"PTPPropertyCodeDigitalZoom";
    case PTPPropertyCodeEffectMode: return @"PTPPropertyCodeEffectMode";
    case PTPPropertyCodeBurstNumber: return @"PTPPropertyCodeBurstNumber";
    case PTPPropertyCodeBurstInterval: return @"PTPPropertyCodeBurstInterval";
    case PTPPropertyCodeTimelapseNumber: return @"PTPPropertyCodeTimelapseNumber";
    case PTPPropertyCodeTimelapseInterval: return @"PTPPropertyCodeTimelapseInterval";
    case PTPPropertyCodeFocusMeteringMode: return @"PTPPropertyCodeFocusMeteringMode";
    case PTPPropertyCodeUploadURL: return @"PTPPropertyCodeUploadURL";
    case PTPPropertyCodeArtist: return @"PTPPropertyCodeArtist";
    case PTPPropertyCodeCopyrightInfo: return @"PTPPropertyCodeCopyrightInfo";
    case PTPPropertyCodeSupportedStreams: return @"PTPPropertyCodeSupportedStreams";
    case PTPPropertyCodeEnabledStreams: return @"PTPPropertyCodeEnabledStreams";
    case PTPPropertyCodeVideoFormat: return @"PTPPropertyCodeVideoFormat";
    case PTPPropertyCodeVideoResolution: return @"PTPPropertyCodeVideoResolution";
    case PTPPropertyCodeVideoQuality: return @"PTPPropertyCodeVideoQuality";
    case PTPPropertyCodeVideoFrameRate: return @"PTPPropertyCodeVideoFrameRate";
    case PTPPropertyCodeVideoContrast: return @"PTPPropertyCodeVideoContrast";
    case PTPPropertyCodeVideoBrightness: return @"PTPPropertyCodeVideoBrightness";
    case PTPPropertyCodeAudioFormat: return @"PTPPropertyCodeAudioFormat";
    case PTPPropertyCodeAudioBitrate: return @"PTPPropertyCodeAudioBitrate";
    case PTPPropertyCodeAudioSamplingRate: return @"PTPPropertyCodeAudioSamplingRate";
    case PTPPropertyCodeAudioBitPerSample: return @"PTPPropertyCodeAudioBitPerSample";
    case PTPPropertyCodeAudioVolume: return @"PTPPropertyCodeAudioVolume";
    case PTPPropertyCodeMTPSynchronizationPartner: return @"PTPPropertyCodeMTPSynchronizationPartner";
    case PTPPropertyCodeMTPDeviceFriendlyName: return @"PTPPropertyCodeMTPDeviceFriendlyName";
    case PTPPropertyCodeMTPVolumeLevel: return @"PTPPropertyCodeMTPVolumeLevel";
    case PTPPropertyCodeMTPDeviceIcon: return @"PTPPropertyCodeMTPDeviceIcon";
    case PTPPropertyCodeMTPSessionInitiatorInfo: return @"PTPPropertyCodeMTPSessionInitiatorInfo";
    case PTPPropertyCodeMTPPerceivedDeviceType: return @"PTPPropertyCodeMTPPerceivedDeviceType";
    case PTPPropertyCodeMTPPlaybackRate: return @"PTPPropertyCodeMTPPlaybackRate";
    case PTPPropertyCodeMTPPlaybackObject: return @"PTPPropertyCodeMTPPlaybackObject";
    case PTPPropertyCodeMTPPlaybackContainerIndex: return @"PTPPropertyCodeMTPPlaybackContainerIndex";
    case PTPPropertyCodeMTPPlaybackPosition: return @"PTPPropertyCodeMTPPlaybackPosition";
  }
  return [NSString stringWithFormat:@"PTPPropertyCode0x%04x", propertyCode];
}

+ (NSString *)typeName:(PTPDataTypeCode)type {
  switch (type) {
    case PTPDataTypeCodeUndefined: return @"PTPDataTypeCodeUndefined";
    case PTPDataTypeCodeSInt8: return @"PTPDataTypeCodeSInt8";
    case PTPDataTypeCodeUInt8: return @"PTPDataTypeCodeUInt8";
    case PTPDataTypeCodeSInt16: return @"PTPDataTypeCodeSInt16";
    case PTPDataTypeCodeUInt16: return @"PTPDataTypeCodeUInt16";
    case PTPDataTypeCodeSInt32: return @"PTPDataTypeCodeSInt32";
    case PTPDataTypeCodeUInt32: return @"PTPDataTypeCodeUInt32";
    case PTPDataTypeCodeSInt64: return @"PTPDataTypeCodeSInt64";
    case PTPDataTypeCodeUInt64: return @"PTPDataTypeCodeUInt64";
    case PTPDataTypeCodeSInt128: return @"PTPDataTypeCodeSInt128";
    case PTPDataTypeCodeUInt128: return @"PTPDataTypeCodeUInt128";
    case PTPDataTypeCodeArrayOfSInt8: return @"PTPDataTypeCodeArrayOfSInt8";
    case PTPDataTypeCodeArrayOfUInt8: return @"PTPDataTypeCodeArrayOfUInt8";
    case PTPDataTypeCodeArrayOfSInt16: return @"PTPDataTypeCodeArrayOfSInt16";
    case PTPDataTypeCodeArrayOfUInt16: return @"PTPDataTypeCodeArrayOfUInt16";
    case PTPDataTypeCodeArrayOfSInt32: return @"PTPDataTypeCodeArrayOfSInt32";
    case PTPDataTypeCodeArrayOfUInt32: return @"PTPDataTypeCodeArrayOfUInt32";
    case PTPDataTypeCodeArrayOfSInt64: return @"PTPDataTypeCodeArrayOfSInt64";
    case PTPDataTypeCodeArrayOfUInt64: return @"PTPDataTypeCodeArrayOfUInt64";
    case PTPDataTypeCodeArrayOfSInt128: return @"PTPDataTypeCodeArrayOfSInt128";
    case PTPDataTypeCodeArrayOfUInt128: return @"PTPDataTypeCodeArrayOfUInt128";
    case PTPDataTypeCodeUnicodeString: return @"PTPDataTypeCodeUnicodeString";
  }
  return [NSString stringWithFormat:@"PTPDataTypeCode0x%04x", type];
}

-(id)initWithCode:(PTPPropertyCode)propertyCode {
  if ((self = [super init])) {
    _propertyCode = propertyCode;
    _type = PTPDataTypeCodeUndefined;
  }
  return self;
}
-(id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 5))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  if ((self = [super init])) {
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
            _min = (NSNumber *)ptpReadValue(_type, &buf);
            _max = (NSNumber *)ptpReadValue(_type, &buf);
            _step = (NSNumber *)ptpReadValue(_type, &buf);
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

-(NSString *)propertyCodeName {
  return [PTPProperty propertyCodeName:_propertyCode];
}

-(NSString *)typeName {
  return [PTPProperty typeName:_type];
}

-(NSString*)description {
  NSMutableString* s = [NSMutableString stringWithFormat:@"%@ %@ %@", self.propertyCodeName, self.typeName, self.readOnly ? @"ro" : @"rw"];
  if (self.min)
    [s appendFormat:@", min = %@", self.min];
  if (self.max)
    [s appendFormat:@", max = %@", self.max];
  if (self.step)
    [s appendFormat:@", step = %@", self.step];
  if (self.supportedValues) {
    [s appendFormat:@", values = [%@", self.supportedValues.firstObject];
    for (int i = 1; i < self.supportedValues.count; i++)
      [s appendFormat:@", %@", [self.supportedValues objectAtIndex:i]];
    [s appendString:@"]"];
  }
  [s appendFormat:@", default = %@", self.defaultValue];
  [s appendFormat:@", value = %@", self.value];
  return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPDeviceInfo

@implementation PTPDeviceInfo

-(id)initWithData:(NSData*)data {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12))
    return NULL;
  self = [super init];
  unsigned char* buffer = (unsigned char*)[data bytes];
  if (self) {
    unsigned char* buf = buffer;
    _standardVersion = ptpReadUnsignedShort(&buf);
    self.vendorExtension = ptpReadUnsignedInt(&buf);
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
    
    if (self.vendorExtension == PTPVendorExtensionMicrosoft) {
      if ([_manufacturer containsString:@"Nikon"]) {
        self.vendorExtension = PTPVendorExtensionNikon;
        _vendorExtensionVersion = 100;
        _vendorExtensionDesc = @"Nikon & Microsoft PTP Extensions";
      } else if ([_manufacturer containsString:@"Canon"]) {
        self.vendorExtension = PTPVendorExtensionCanon;
        _vendorExtensionVersion = 100;
        _vendorExtensionDesc = @"Canon & Microsoft PTP Extensions";
      }
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
    
    _properties = [NSMutableDictionary dictionary];
  }
  return self;
}

-(NSString *)description {
  if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
    return [self debug];
  return [NSString stringWithFormat:@"%@ %@, PTP V%.2f + %8@ V%.2f", _model, _version, _standardVersion / 100.0, _vendorExtensionDesc, _vendorExtensionVersion / 100.0];
}

-(NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %8@ V%.2f\n", _model, _version, _standardVersion / 100.0, _vendorExtensionDesc, _vendorExtensionVersion / 100.0];
  if (_operationsSupported.count > 0) {
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in _operationsSupported)
      [s appendFormat:@"%@\n", [PTPRequest operationCodeName:code.intValue]];
  }
  if (_eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in _eventsSupported)
      [s appendFormat:@"%@\n", [PTPEvent eventCodeName:code.intValue]];
  }
  if (_propertiesSupported.count > 0) {
    [s appendFormat:@"\nProperties:\n"];
    for (NSNumber *code in _propertiesSupported) {
      PTPProperty *property = _properties[code];
      if (property)
        [s appendFormat:@"%@\n", property];
      else
        [s appendFormat:@"%@\n", [PTPProperty propertyCodeName:code.intValue]];
    }
  }
  return s;
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation NSObject(PTPExtensions)

-(int)intValue {
  return self.description.intValue;
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation PTPCamera {
  BOOL objectAdded;
  NSTimer *ptpEventTimer;
}

-(NSString *)name {
  return _icCamera.name;
}

-(PTPVendorExtension) extension {
  return 0;
}

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super init];
  if (self) {
    icCamera.delegate = self;
    _icCamera = icCamera;
    _delegate = delegate;
    _userData = nil;
    objectAdded = false;
  }
  return self;
}

-(Class)requestClass {
  return PTPRequest.class;
}

-(Class)responseClass {
  return PTPResponse.class;
}

-(Class)eventClass {
  return PTPEvent.class;
}

-(Class)propertyClass {
  return PTPProperty.class;
}

-(Class)deviceInfoClass {
  return PTPDeviceInfo.class;
}

-(void)didRemoveDevice:(ICDevice *)device {
  [ptpEventTimer invalidate];
  [_delegate cameraRemoved:self];
}

-(void)requestOpenSession {
  [_icCamera requestOpenSession];
}

-(void)requestCloseSession {
  [self unlock];
  [_icCamera requestCloseSession];
}

-(void)requestEnableTethering {
  [_icCamera requestEnableTethering];
}

-(void)checkForEvent {
}

-(void)getLiveViewImage {
}

-(void)processEvent:(PTPEvent *)event {
  switch (event.eventCode) {
    case PTPEventCodeDevicePropChanged: {
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:event.parameter1];
      break;
    }
    case PTPEventCodeObjectAdded: {
      objectAdded = true;
      break;
    }
    case PTPEventCodeCaptureComplete: {
      if (objectAdded) {
        objectAdded = false;
      } else {
        [_delegate cameraExposureFailed:self  message:@"Image data out of sequence"];
      }
    }
  }
}

-(void)mapValueList:(PTPProperty *)property map:(NSDictionary *)map {
  NSMutableArray *values = [NSMutableArray array];
  NSMutableArray *labels = [NSMutableArray array];
  if (property.value) {
    for (NSObject *value in property.supportedValues) {
      [values addObject:value.description];
      NSString *label = map[value];
      if (label)
        [labels addObject:label];
      else
        [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
    }
    [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
  } else {
    [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:true];
  }
}

-(void)mapValueInterval:(PTPProperty *)property map:(NSDictionary *)map {
  NSMutableArray *values = [NSMutableArray array];
  NSMutableArray *labels = [NSMutableArray array];
  NSArray *keys = [[map allKeys] sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) { int val1 = [obj1 intValue]; int val2 = [obj2 intValue]; if ( val1 < val2 ) return (NSComparisonResult)NSOrderedAscending; else if ( val1 > val2 ) return (NSComparisonResult)NSOrderedDescending; return (NSComparisonResult)NSOrderedSame; }];
  for (NSObject *value in keys) {
    [values addObject:value.description];
    NSString *label = map[value];
    if (label)
      [labels addObject:label];
    else
      [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
  }
  [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
}

-(void)processPropertyDescription:(PTPProperty *)property {
  switch (property.propertyCode) {
    case PTPPropertyCodeExposureProgramMode: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Program", @3: @"Aperture priority", @4: @"Shutter priority" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFNumber: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSNumber *value in property.supportedValues) {
        [values addObject:value.description];
        [labels addObject:[NSString stringWithFormat:@"f/%g", value.intValue / 100.0]];
      }
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureTime: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSNumber *value in property.supportedValues) {
        int intValue = value.intValue;
        [values addObject:value.description];
        if (intValue == -1)
          [labels addObject:[NSString stringWithFormat:@"Bulb"]];
        else if (intValue == -3)
          [labels addObject:[NSString stringWithFormat:@"Time"]];
        else if (intValue == 1)
          [labels addObject:[NSString stringWithFormat:@"1/8000 s"]];
        else if (intValue == 3)
          [labels addObject:[NSString stringWithFormat:@"1/3200 s"]];
        else if (intValue == 6)
          [labels addObject:[NSString stringWithFormat:@"1/1600 s"]];
        else if (intValue == 12)
          [labels addObject:[NSString stringWithFormat:@"1/800 s"]];
        else if (intValue == 15)
          [labels addObject:[NSString stringWithFormat:@"1/640 s"]];
        else if (intValue == 80)
          [labels addObject:[NSString stringWithFormat:@"1/125 s"]];
        else if (intValue < 100)
          [labels addObject:[NSString stringWithFormat:@"1/%g s", round(1000.0 / value.intValue) * 10]];
        else if (intValue < 10000)
          [labels addObject:[NSString stringWithFormat:@"1/%g s", round(10000.0 / value.intValue)]];
        else
          [labels addObject:[NSString stringWithFormat:@"%g s", value.intValue / 10000.0]];
      }
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeImageSize: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSString *value in property.supportedValues) {
        [values addObject:value];
        [labels addObject:value];
      }
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeCompressionSetting: {
      NSDictionary *map = @{ @0: @"JPEG Basic", @1: @"JPEG Norm", @2: @"JPEG Fine", @4: @"RAW", @5: @"RAW + JPEG Basic", @6: @"RAW + JPEG Norm", @7: @"RAW + JPEG Fine" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeWhiteBalance: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Auto", @3: @"One-push Auto", @4: @"Daylight", @5: @"Fluorescent", @6: @"Incandescent", @7: @"Flash" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeExposureIndex: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSNumber *value in property.supportedValues) {
        [values addObject:value.description];
        [labels addObject:value.description];
      }
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureBiasCompensation: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      for (NSNumber *value in property.supportedValues) {
        [values addObject:value.description];
        [labels addObject:[NSString stringWithFormat:@"%.1f", round(value.intValue / 100.0) / 10.0]];
      }
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeExposureMeteringMode: {
      NSDictionary *map = @{ @1: @"Average", @2: @"Center-Weighted Average", @3: @"Multi-spot", @4: @"Center-spot" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFocusMeteringMode: {
      NSDictionary *map = @{ @1: @"Center-spot", @2: @"Multi-spot" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFocalLength: {
      [_delegate cameraPropertyChanged:self code:property.propertyCode value:(NSNumber *)[NSNumber numberWithInt:property.value.intValue / 100] min:[NSNumber numberWithInt:property.min.intValue / 100] max:[NSNumber numberWithInt:property.max.intValue / 100] step:nil readOnly:true];
      break;
    }
    case PTPPropertyCodeFlashMode: {
      NSDictionary *map = @{ @0: @"Undefined", @1: @"Automatic flash", @2: @"Flash off", @3: @"Fill flash", @4: @"Automatic Red-eye Reduction", @5: @"Red-eye fill flash", @6: @"External sync" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFocusMode: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Automatic", @3:@"Macro" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeStillCaptureMode: {
      NSDictionary *map = @{ @1: @"Single shot", @2: @"Continuous", @3:@"Timelapse" };
      [self mapValueList:property map:map];
      break;
    }
    default: {
      if (property.supportedValues) {
        NSMutableArray *values = [NSMutableArray array];
        for (NSNumber *number in property.supportedValues)
          [values addObject:number.description];
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:values readOnly:property.readOnly];
      } else if (property.type >= PTPDataTypeCodeSInt8 && property.type <= PTPDataTypeCodeUInt64) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:(NSNumber *)property.value min:property.min max:property.max step:property.step readOnly:property.readOnly];
      } else if (property.type == PTPDataTypeCodeSInt128 || property.type == PTPDataTypeCodeUInt128) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:(NSString *)property.value.description readOnly:true];
      } else if (property.type == PTPDataTypeCodeUnicodeString) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:(NSString *)property.value.description readOnly:property.readOnly];
      } else if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
        NSLog(@"Ignored %@", property);
      }
      break;
    }
  }
}

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetStorageIDs: {
      if (response.responseCode == PTPResponseCodeOK) {
        NSLog(@"Initialized %@\n", _info.debug);
        [_delegate cameraConnected:self];
        ptpEventTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(checkForEvent) userInfo:nil repeats:true];
      }
      break;
    }
    case PTPRequestCodeSetDevicePropValue: {
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:request.parameter1];
      break;
    }
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        _info = [[self.deviceInfoClass alloc] initWithData:data];
        if ([_info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPRequestCodeInitiateCapture]]) {
          [_delegate cameraCanCapture:self];
        }
        for (NSNumber *code in _info.propertiesSupported)
          [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
        [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      }
      break;
    }
    case PTPRequestCodeGetDevicePropDesc: {
      PTPProperty *property = [[self.propertyClass alloc] initWithData:data];
      if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
        NSLog(@"Translated to %@", property);
      _info.properties[[NSNumber numberWithUnsignedShort:property.propertyCode]] = property;
      [self processPropertyDescription:property];
    }
  }
}

-(void)didSendPTPCommand:(NSData*)command inData:(NSData*)data response:(NSData*)responseData error:(NSError*)error contextInfo:(void*)contextInfo {
  PTPRequest*  request  = (__bridge PTPRequest*)contextInfo;
  if (responseData == nil) {
    if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
      NSLog(@"Completed %@ with error %@", request, error);
  } else {
    PTPResponse* response = [[PTPResponse alloc] initWithData:responseData];
    if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
      NSLog(@"Completed %@ with %@", request, response);
    [self processRequest:request Response:response inData:data];
  }
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  PTPProperty *property = _info.properties[[NSNumber numberWithUnsignedShort:code]];
  if (property) {
    switch (property.type) {
      case PTPDataTypeCodeSInt8: {
        unsigned char *buffer = malloc(sizeof (char));
        unsigned char *buf = buffer;
        ptpWriteChar(&buf, (char)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (char)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt8: {
        unsigned char *buffer = malloc(sizeof (unsigned char));
        unsigned char *buf = buffer;
        ptpWriteUnsignedChar(&buf, (unsigned char)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned char)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt16: {
        unsigned char *buffer = malloc(sizeof (short));
        unsigned char *buf = buffer;
        ptpWriteShort(&buf, (short)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (short)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt16: {
        unsigned char *buffer = malloc(sizeof (unsigned short));
        unsigned char *buf = buffer;
        ptpWriteUnsignedShort(&buf, (unsigned short)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned short)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt32: {
        unsigned char *buffer = malloc(sizeof (int));
        unsigned char *buf = buffer;
        ptpWriteInt(&buf, (int)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (int)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt32: {
        unsigned char *buffer = malloc(sizeof (unsigned int));
        unsigned char *buf = buffer;
        ptpWriteUnsignedInt(&buf, (unsigned int)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned int)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeSInt64: {
        unsigned char *buffer = malloc(sizeof (long));
        unsigned char *buf = buffer;
        ptpWriteLong(&buf, (long)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (long)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUInt64: {
        unsigned char *buffer = malloc(sizeof (unsigned long));
        unsigned char *buf = buffer;
        ptpWriteUnsignedLong(&buf, (unsigned long)value.longLongValue);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:sizeof (unsigned long)]];
        free(buffer);
        break;
      }
      case PTPDataTypeCodeUnicodeString: {
        unsigned char *buffer = malloc(256);
        unsigned char *buf = buffer;
        int length = ptpWriteString(&buf, value);
        [self sendPTPRequest:PTPRequestCodeSetDevicePropValue param1:code withData:[NSData dataWithBytes:buffer length:length]];
        free(buffer);
        break;
      }
    }
  }
}

-(void)lock {
}

-(void)unlock {
}

-(void)startLiveViewZoom:(int)zoom x:(int)x y:(int)y {
}

-(void)stopLiveView {
}

-(void)startCapture {
  [_icCamera requestTakePicture];
}

-(void)stopCapture {
}

-(void)focus:(int)steps {
  PTPProperty *afMode = _info.properties[[NSNumber numberWithUnsignedShort:PTPPropertyCodeNikonLiveViewAFFocus]];
  if (afMode.value.intValue != 0) {
    [self setProperty:PTPPropertyCodeNikonLiveViewAFFocus value:@"0"];
  }
  if (steps >= 0) {
    [self sendPTPRequest:PTPRequestCodeNikonMfDrive param1:1 param2:steps];
  } else {
    [self sendPTPRequest:PTPRequestCodeNikonMfDrive param1:2 param2:-steps];
  }
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode data:(NSData *)data {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 0;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:data sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 0;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1 {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1 param2:(unsigned int)parameter2 {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 2;
  request.parameter1 = parameter1;
  request.parameter2 = parameter2;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1 param2:(unsigned int)parameter2  param3:(unsigned int)parameter3 {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 3;
  request.parameter1 = parameter1;
  request.parameter2 = parameter2;
  request.parameter3 = parameter3;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1 withData:(NSData *)data {
  PTPRequest *request = [[self.requestClass alloc] init];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [_icCamera requestSendPTPCommand:request.commandBuffer outData:data sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)device:(ICDevice*)camera didOpenSessionWithError:(NSError*)error {
  [self sendPTPRequest:PTPRequestCodeGetDeviceInfo];
}

-(void)device:(ICDevice*)camera didCloseSessionWithError:(NSError*)error {
  [_delegate cameraDisconnected:self];
}

-(void)device:(ICDevice*)camera didEncounterError:(NSError*)error {
  NSLog(@"Error '%@' on '%@'", error.localizedDescription, camera.name);
}

-(void)cameraDevice:(ICCameraDevice*)camera didAddItem:(ICCameraItem*)item {
  if (item.class == ICCameraFile.class) {
    ICCameraFile *file = (ICCameraFile *)item;
    if (file.wasAddedAfterContentCatalogCompleted) {
      objectAdded = true;
      [camera requestDownloadFile:file options:@{ ICDeleteAfterSuccessfulDownload: @TRUE, ICOverwrite: @TRUE, ICDownloadsDirectoryURL: [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:true] } downloadDelegate:self didDownloadSelector:@selector(didDownloadFile:error:options:contextInfo:) contextInfo:nil];
    }
  }
}

-(void)didDownloadFile:(ICCameraFile*)file error:(nullable NSError*)error options:(nullable NSDictionary<NSString*,id>*)options contextInfo:(nullable void*)contextInfo {
  if (error == nil) {
    NSURL *folder = options[ICDownloadsDirectoryURL];
    NSString *name = options[ICSavedFilename];
    if (folder != nil && name != nil) {
      [_delegate cameraFrame:self left:0 top:0 width:-1 height:-1];
      NSURL *url = [NSURL URLWithString:name relativeToURL:folder];
      NSData *data = [NSData dataWithContentsOfURL:url];
      [_delegate cameraExposureDone:self data:data filename:name];
      [NSFileManager.defaultManager removeItemAtURL:url error:nil];
      return;
    }
  }
  [_delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"requestDownloadFile failed (%@)", error.localizedDescription]];
}

-(void)cameraDevice:(ICCameraDevice*)camera didReceivePTPEvent:(NSData*)eventData {
  PTPEvent *event = [[self.eventClass alloc] initWithData:eventData];
  if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
    NSLog(@"Received %@", event);
  switch (_info.vendorExtension) {
    case PTPVendorExtensionNikon: {
      break;
    }
    default: {
      [self processEvent:event];
      break;
    }
  }
}


@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation PTPBrowser {
  ICDeviceBrowser *icBrowser;
  NSMutableArray *cameras;
}

-(id)initWithDelegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super init];
  if (self) {
    _delegate = delegate;
    icBrowser = [[ICDeviceBrowser alloc] init];
    icBrowser.delegate = self;
    icBrowser.browsedDeviceTypeMask = ICDeviceTypeMaskCamera | ICDeviceLocationTypeMaskLocal;
    cameras = [NSMutableArray array];
  }
  return self;
}

-(void)start {
  [icBrowser start];
}

-(void)stop {
  [icBrowser stop];
}

-(void)deviceBrowser:(ICDeviceBrowser*)browser didAddDevice:(ICDevice*)device moreComing:(BOOL)moreComing {
  PTPCamera *camera;
  if (device.usbVendorID == 0x04B0)
    camera = [[PTPNikonCamera alloc] initWithICCamera:(ICCameraDevice *)device delegate:_delegate];
  else if (device.usbVendorID == 0x04A9)
    camera = [[PTPCanonCamera alloc] initWithICCamera:(ICCameraDevice *)device delegate:_delegate];
  else
    camera = [[PTPCamera alloc] initWithICCamera:(ICCameraDevice *)device delegate:_delegate];
  [cameras addObject:camera];
  [_delegate cameraAdded:camera];
}

-(void)deviceBrowser:(ICDeviceBrowser*)browser didRemoveDevice:(ICDevice*)device moreGoing:(BOOL)moreGoing {
  for (PTPCamera *camera in cameras) {
    if (camera.icCamera == device) {
      [cameras removeObject:camera];
      break;
    }
  }
}

@end

//------------------------------------------------------------------------------------------------------------------------------
