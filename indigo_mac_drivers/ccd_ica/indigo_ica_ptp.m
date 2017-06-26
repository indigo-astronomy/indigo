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
    if ([result hasSuffix:@"\0"])
      result = [result stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\0"]];
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
  return [NSString stringWithFormat:@"PTPVendorExtension_%04x", vendorExtension];
}

- (id)initWithVendorExtension:(PTPVendorExtension)vendorExtension {
	self = [super init];
	if (self) {
		_vendorExtension = vendorExtension;
	}
	return self;
}

- (NSString *)vendorExtensionName {
  return [PTPVendor vendorExtensionName:_vendorExtension];
}

@end
//---------------------------------------------------------------------------------------------------------- PTPOperationRequest

@implementation PTPOperationRequest

+ (NSString *)operationCodeName:(PTPOperationCode)operationCode vendorExtension:(PTPVendorExtension)vendorExtension {
  switch (operationCode) {
    case PTPOperationCodeUndefined: return @"PTPOperationCodeUndefined";
    case PTPOperationCodeGetDeviceInfo: return @"PTPOperationCodeGetDeviceInfo";
    case PTPOperationCodeOpenSession: return @"PTPOperationCodeOpenSession";
    case PTPOperationCodeCloseSession: return @"PTPOperationCodeCloseSession";
    case PTPOperationCodeGetStorageIDs: return @"PTPOperationCodeGetStorageIDs";
    case PTPOperationCodeGetStorageInfo: return @"PTPOperationCodeGetStorageInfo";
    case PTPOperationCodeGetNumObjects: return @"PTPOperationCodeGetNumObjects";
    case PTPOperationCodeGetObjectHandles: return @"PTPOperationCodeGetObjectHandles";
    case PTPOperationCodeGetObjectInfo: return @"PTPOperationCodeGetObjectInfo";
    case PTPOperationCodeGetObject: return @"PTPOperationCodeGetObject";
    case PTPOperationCodeGetThumb: return @"PTPOperationCodeGetThumb";
    case PTPOperationCodeDeleteObject: return @"PTPOperationCodeDeleteObject";
    case PTPOperationCodeSendObjectInfo: return @"PTPOperationCodeSendObjectInfo";
    case PTPOperationCodeSendObject: return @"PTPOperationCodeSendObject";
    case PTPOperationCodeInitiateCapture: return @"PTPOperationCodeInitiateCapture";
    case PTPOperationCodeFormatStore: return @"PTPOperationCodeFormatStore";
    case PTPOperationCodeResetDevice: return @"PTPOperationCodeResetDevice";
    case PTPOperationCodeSelfTest: return @"PTPOperationCodeSelfTest";
    case PTPOperationCodeSetObjectProtection: return @"PTPOperationCodeSetObjectProtection";
    case PTPOperationCodePowerDown: return @"PTPOperationCodePowerDown";
    case PTPOperationCodeGetDevicePropDesc: return @"PTPOperationCodeGetDevicePropDesc";
    case PTPOperationCodeGetDevicePropValue: return @"PTPOperationCodeGetDevicePropValue";
    case PTPOperationCodeSetDevicePropValue: return @"PTPOperationCodeSetDevicePropValue";
    case PTPOperationCodeResetDevicePropValue: return @"PTPOperationCodeResetDevicePropValue";
    case PTPOperationCodeTerminateOpenCapture: return @"PTPOperationCodeTerminateOpenCapture";
    case PTPOperationCodeMoveObject: return @"PTPOperationCodeMoveObject";
    case PTPOperationCodeCopyObject: return @"PTPOperationCodeCopyObject";
    case PTPOperationCodeGetPartialObject: return @"PTPOperationCodeGetPartialObject";
    case PTPOperationCodeInitiateOpenCapture: return @"PTPOperationCodeInitiateOpenCapture";
    case PTPOperationCodeGetNumDownloadableObjects: return @"PTPOperationCodeGetNumDownloadableObjects";
    case PTPOperationCodeGetAllObjectInfo: return @"PTPOperationCodeGetAllObjectInfo";
    case PTPOperationCodeGetUserAssignedDeviceName: return @"PTPOperationCodeGetUserAssignedDeviceName";
    case PTPOperationCodeMTPGetObjectPropsSupported: return @"PTPOperationCodeMTPGetObjectPropsSupported";
    case PTPOperationCodeMTPGetObjectPropDesc: return @"PTPOperationCodeMTPGetObjectPropDesc";
    case PTPOperationCodeMTPGetObjectPropValue: return @"PTPOperationCodeMTPGetObjectPropValue";
    case PTPOperationCodeMTPSetObjectPropValue: return @"PTPOperationCodeMTPSetObjectPropValue";
    case PTPOperationCodeMTPGetObjPropList: return @"PTPOperationCodeMTPGetObjPropList";
    case PTPOperationCodeMTPSetObjPropList: return @"PTPOperationCodeMTPSetObjPropList";
    case PTPOperationCodeMTPGetInterdependendPropdesc: return @"PTPOperationCodeMTPGetInterdependendPropdesc";
    case PTPOperationCodeMTPSendObjectPropList: return @"PTPOperationCodeMTPSendObjectPropList";
    case PTPOperationCodeMTPGetObjectReferences: return @"PTPOperationCodeMTPGetObjectReferences";
    case PTPOperationCodeMTPSetObjectReferences: return @"PTPOperationCodeMTPSetObjectReferences";
    case PTPOperationCodeMTPUpdateDeviceFirmware: return @"PTPOperationCodeMTPUpdateDeviceFirmware";
    case PTPOperationCodeMTPSkip: return @"PTPOperationCodeMTPSkip";
  }
  if (vendorExtension == PTPVendorExtensionNikon) {
    switch (operationCode) {
      case PTPOperationCodeNikonGetProfileAllData: return @"PTPOperationCodeNikonGetProfileAllData";
      case PTPOperationCodeNikonSendProfileData: return @"PTPOperationCodeNikonSendProfileData";
      case PTPOperationCodeNikonDeleteProfile: return @"PTPOperationCodeNikonDeleteProfile";
      case PTPOperationCodeNikonSetProfileData: return @"PTPOperationCodeNikonSetProfileData";
      case PTPOperationCodeNikonAdvancedTransfer: return @"PTPOperationCodeNikonAdvancedTransfer";
      case PTPOperationCodeNikonGetFileInfoInBlock: return @"PTPOperationCodeNikonGetFileInfoInBlock";
      case PTPOperationCodeNikonCapture: return @"PTPOperationCodeNikonCapture";
      case PTPOperationCodeNikonAfDrive: return @"PTPOperationCodeNikonAfDrive";
      case PTPOperationCodeNikonSetControlMode: return @"PTPOperationCodeNikonSetControlMode";
      case PTPOperationCodeNikonDelImageSDRAM: return @"PTPOperationCodeNikonDelImageSDRAM";
      case PTPOperationCodeNikonGetLargeThumb: return @"PTPOperationCodeNikonGetLargeThumb";
      case PTPOperationCodeNikonCurveDownload: return @"PTPOperationCodeNikonCurveDownload";
      case PTPOperationCodeNikonCurveUpload: return @"PTPOperationCodeNikonCurveUpload";
      case PTPOperationCodeNikonCheckEvent: return @"PTPOperationCodeNikonCheckEvent";
      case PTPOperationCodeNikonDeviceReady: return @"PTPOperationCodeNikonDeviceReady";
      case PTPOperationCodeNikonSetPreWBData: return @"PTPOperationCodeNikonSetPreWBData";
      case PTPOperationCodeNikonGetVendorPropCodes: return @"PTPOperationCodeNikonGetVendorPropCodes";
      case PTPOperationCodeNikonAfCaptureSDRAM: return @"PTPOperationCodeNikonAfCaptureSDRAM";
      case PTPOperationCodeNikonGetPictCtrlData: return @"PTPOperationCodeNikonGetPictCtrlData";
      case PTPOperationCodeNikonSetPictCtrlData: return @"PTPOperationCodeNikonSetPictCtrlData";
      case PTPOperationCodeNikonDelCstPicCtrl: return @"PTPOperationCodeNikonDelCstPicCtrl";
      case PTPOperationCodeNikonGetPicCtrlCapability: return @"PTPOperationCodeNikonGetPicCtrlCapability";
      case PTPOperationCodeNikonGetPreviewImg: return @"PTPOperationCodeNikonGetPreviewImg";
      case PTPOperationCodeNikonStartLiveView: return @"PTPOperationCodeNikonStartLiveView";
      case PTPOperationCodeNikonEndLiveView: return @"PTPOperationCodeNikonEndLiveView";
      case PTPOperationCodeNikonGetLiveViewImg: return @"PTPOperationCodeNikonGetLiveViewImg";
      case PTPOperationCodeNikonMfDrive: return @"PTPOperationCodeNikonMfDrive";
      case PTPOperationCodeNikonChangeAfArea: return @"PTPOperationCodeNikonChangeAfArea";
      case PTPOperationCodeNikonAfDriveCancel: return @"PTPOperationCodeNikonAfDriveCancel";
      case PTPOperationCodeNikonInitiateCaptureRecInMedia: return @"PTPOperationCodeNikonInitiateCaptureRecInMedia";
      case PTPOperationCodeNikonGetVendorStorageIDs: return @"PTPOperationCodeNikonGetVendorStorageIDs";
      case PTPOperationCodeNikonStartMovieRecInCard: return @"PTPOperationCodeNikonStartMovieRecInCard";
      case PTPOperationCodeNikonEndMovieRec: return @"PTPOperationCodeNikonEndMovieRec";
      case PTPOperationCodeNikonTerminateCapture: return @"PTPOperationCodeNikonTerminateCapture";
      case PTPOperationCodeNikonGetDevicePTPIPInfo: return @"PTPOperationCodeNikonGetDevicePTPIPInfo";
      case PTPOperationCodeNikonGetPartialObjectHiSpeed: return @"PTPOperationCodeNikonGetPartialObjectHiSpeed";
    }
  }
  return [NSString stringWithFormat:@"PTPOperationCode_%04x", operationCode];
}

- (id)init {
	self = [super init];
	if (self) {
		_numberOfParameters = 0;
	}
	return self;
}

- (id)initWithVendorExtension:(PTPVendorExtension)vendorExtension {
  return [super initWithVendorExtension:vendorExtension];
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

- (NSString *)operationCodeName {
  return [PTPOperationRequest operationCodeName:_operationCode vendorExtension:self.vendorExtension];
}

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ [", self.operationCodeName];
	if (self.numberOfParameters > 0) {
		[s appendFormat:@"0x%08X", self.parameter1];
	}
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	if (self.numberOfParameters > 3)
		[s appendFormat:@", 0x%08X", self.parameter4];
	if (self.numberOfParameters > 4)
		[s appendFormat:@", 0x%08X", self.parameter5];
	[s appendString:@"]"];
	return s;
}

@end

//--------------------------------------------------------------------------------------------------------- PTPOperationResponse

@implementation PTPOperationResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode vendorExtension:(PTPVendorExtension)vendorExtension {
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
    case PTPResponseCodeMTPInvalid_ObjectPropCode: return @"PTPResponseCodeMTPInvalid_ObjectPropCode";
    case PTPResponseCodeMTPInvalid_ObjectProp_Format: return @"PTPResponseCodeMTPInvalid_ObjectProp_Format";
    case PTPResponseCodeMTPInvalid_ObjectProp_Value: return @"PTPResponseCodeMTPInvalid_ObjectProp_Value";
    case PTPResponseCodeMTPInvalid_ObjectReference: return @"PTPResponseCodeMTPInvalid_ObjectReference";
    case PTPResponseCodeMTPInvalid_Dataset: return @"PTPResponseCodeMTPInvalid_Dataset";
    case PTPResponseCodeMTPSpecification_By_Group_Unsupported: return @"PTPResponseCodeMTPSpecification_By_Group_Unsupported";
    case PTPResponseCodeMTPSpecification_By_Depth_Unsupported: return @"PTPResponseCodeMTPSpecification_By_Depth_Unsupported";
    case PTPResponseCodeMTPObject_Too_Large: return @"PTPResponseCodeMTPObject_Too_Large";
    case PTPResponseCodeMTPObjectProp_Not_Supported: return @"PTPResponseCodeMTPObjectProp_Not_Supported";
  }
  if (vendorExtension == PTPVendorExtensionNikon) {
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
  }
  return [NSString stringWithFormat:@"PTPResponseCode_%04X", responseCode];
}

- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 32))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 32 || type != 3)
    return NULL;
  if ((self = [super initWithVendorExtension:vendorExtension])) {
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

- (NSString *)responseCodeName {
  return [PTPOperationResponse responseCodeName:_responseCode vendorExtension:self.vendorExtension];
}

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ [", self.responseCodeName];
	if (self.numberOfParameters > 0)
		[s appendFormat:@"0x%08X", self.parameter1];
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	if (self.numberOfParameters > 3)
		[s appendFormat:@", 0x%08X", self.parameter4];
	if (self.numberOfParameters > 4)
		[s appendFormat:@", 0x%08X", self.parameter5];
	[s appendString:@"]"];
	return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPEvent

@implementation PTPEvent

+ (NSString *)eventCodeName:(PTPEventCode)eventCode vendorExtension:(PTPVendorExtension)vendorExtension {
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
  if (vendorExtension == PTPVendorExtensionNikon) {
    switch (eventCode) {
      case PTPEventCodeNikonObjectAddedInSDRAM: return @"PTPEventCodeNikonObjectAddedInSDRAM";
      case PTPEventCodeNikonCaptureCompleteRecInSdram: return @"PTPEventCodeNikonCaptureCompleteRecInSdram";
      case PTPEventCodeNikonAdvancedTransfer: return @"PTPEventCodeNikonAdvancedTransfer";
      case PTPEventCodeNikonPreviewImageAdded: return @"PTPEventCodeNikonPreviewImageAdded";
    }
  }
  return [NSString stringWithFormat:@"PTPEventCodeCode_%04X", eventCode];
}

- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension {
  NSUInteger dataLength = [data length];
  if ((data == NULL) || (dataLength < 12) || (dataLength > 24))
    return NULL;
  unsigned char* buffer = (unsigned char*)[data bytes];
  unsigned int size = CFSwapInt32LittleToHost(*(unsigned int*)buffer);
  unsigned short type = CFSwapInt16LittleToHost(*(unsigned short*)(buffer+4));
  if (size < 12 || size > 24 || type != 4)
    return NULL;
  if ((self = [super initWithVendorExtension:vendorExtension])) {
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
  self = [super initWithVendorExtension:vendorExtension];
  if (self) {
    _eventCode = eventCode;
    _numberOfParameters = 1;
    _parameter1 = parameter1;
  }
  return self;
}

- (NSString *)eventCodeName {
	return [PTPEvent eventCodeName:_eventCode vendorExtension:self.vendorExtension];
}

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ [", self.eventCodeName];
	if (self.numberOfParameters > 0)
		[s appendFormat:@"0x%08X", self.parameter1];
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	[s appendString:@"]"];
	return s;
}

@end

//------------------------------------------------------------------------------------------------------------------------------ PTPProperty

@implementation PTPProperty

+ (NSString *)propertyCodeName:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension {
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
  if (vendorExtension == PTPVendorExtensionNikon) {
    switch (propertyCode) {
      case PTPPropertyCodeNikonShootingBank: return @"PTPPropertyCodeNikonShootingBank";
      case PTPPropertyCodeNikonShootingBankNameA: return @"PTPPropertyCodeNikonShootingBankNameA";
      case PTPPropertyCodeNikonShootingBankNameB: return @"PTPPropertyCodeNikonShootingBankNameB";
      case PTPPropertyCodeNikonShootingBankNameC: return @"PTPPropertyCodeNikonShootingBankNameC";
      case PTPPropertyCodeNikonShootingBankNameD: return @"PTPPropertyCodeNikonShootingBankNameD";
      case PTPPropertyCodeNikonResetBank0: return @"PTPPropertyCodeNikonResetBank0";
      case PTPPropertyCodeNikonRawCompression: return @"PTPPropertyCodeNikonRawCompression";
      case PTPPropertyCodeNikonWhiteBalanceAutoBias: return @"PTPPropertyCodeNikonWhiteBalanceAutoBias";
      case PTPPropertyCodeNikonWhiteBalanceTungstenBias: return @"PTPPropertyCodeNikonWhiteBalanceTungstenBias";
      case PTPPropertyCodeNikonWhiteBalanceFluorescentBias: return @"PTPPropertyCodeNikonWhiteBalanceFluorescentBias";
      case PTPPropertyCodeNikonWhiteBalanceDaylightBias: return @"PTPPropertyCodeNikonWhiteBalanceDaylightBias";
      case PTPPropertyCodeNikonWhiteBalanceFlashBias: return @"PTPPropertyCodeNikonWhiteBalanceFlashBias";
      case PTPPropertyCodeNikonWhiteBalanceCloudyBias: return @"PTPPropertyCodeNikonWhiteBalanceCloudyBias";
      case PTPPropertyCodeNikonWhiteBalanceShadeBias: return @"PTPPropertyCodeNikonWhiteBalanceShadeBias";
      case PTPPropertyCodeNikonWhiteBalanceColorTemperature: return @"PTPPropertyCodeNikonWhiteBalanceColorTemperature";
      case PTPPropertyCodeNikonWhiteBalancePresetNo: return @"PTPPropertyCodeNikonWhiteBalancePresetNo";
      case PTPPropertyCodeNikonWhiteBalancePresetName0: return @"PTPPropertyCodeNikonWhiteBalancePresetName0";
      case PTPPropertyCodeNikonWhiteBalancePresetName1: return @"PTPPropertyCodeNikonWhiteBalancePresetName1";
      case PTPPropertyCodeNikonWhiteBalancePresetName2: return @"PTPPropertyCodeNikonWhiteBalancePresetName2";
      case PTPPropertyCodeNikonWhiteBalancePresetName3: return @"PTPPropertyCodeNikonWhiteBalancePresetName3";
      case PTPPropertyCodeNikonWhiteBalancePresetName4: return @"PTPPropertyCodeNikonWhiteBalancePresetName4";
      case PTPPropertyCodeNikonWhiteBalancePresetVal0: return @"PTPPropertyCodeNikonWhiteBalancePresetVal0";
      case PTPPropertyCodeNikonWhiteBalancePresetVal1: return @"PTPPropertyCodeNikonWhiteBalancePresetVal1";
      case PTPPropertyCodeNikonWhiteBalancePresetVal2: return @"PTPPropertyCodeNikonWhiteBalancePresetVal2";
      case PTPPropertyCodeNikonWhiteBalancePresetVal3: return @"PTPPropertyCodeNikonWhiteBalancePresetVal3";
      case PTPPropertyCodeNikonWhiteBalancePresetVal4: return @"PTPPropertyCodeNikonWhiteBalancePresetVal4";
      case PTPPropertyCodeNikonImageSharpening: return @"PTPPropertyCodeNikonImageSharpening";
      case PTPPropertyCodeNikonToneCompensation: return @"PTPPropertyCodeNikonToneCompensation";
      case PTPPropertyCodeNikonColorModel: return @"PTPPropertyCodeNikonColorModel";
      case PTPPropertyCodeNikonHueAdjustment: return @"PTPPropertyCodeNikonHueAdjustment";
      case PTPPropertyCodeNikonNonCPULensDataFocalLength: return @"PTPPropertyCodeNikonNonCPULensDataFocalLength";
      case PTPPropertyCodeNikonNonCPULensDataMaximumAperture: return @"PTPPropertyCodeNikonNonCPULensDataMaximumAperture";
      case PTPPropertyCodeNikonShootingMode: return @"PTPPropertyCodeNikonShootingMode";
      case PTPPropertyCodeNikonJPEGCompressionPolicy: return @"PTPPropertyCodeNikonJPEGCompressionPolicy";
      case PTPPropertyCodeNikonColorSpace: return @"PTPPropertyCodeNikonColorSpace";
      case PTPPropertyCodeNikonAutoDXCrop: return @"PTPPropertyCodeNikonAutoDXCrop";
      case PTPPropertyCodeNikonFlickerReduction: return @"PTPPropertyCodeNikonFlickerReduction";
      case PTPPropertyCodeNikonRemoteMode: return @"PTPPropertyCodeNikonRemoteMode";
      case PTPPropertyCodeNikonVideoMode: return @"PTPPropertyCodeNikonVideoMode";
      case PTPPropertyCodeNikonEffectMode: return @"PTPPropertyCodeNikonEffectMode";
      case PTPPropertyCodeNikon1Mode: return @"PTPPropertyCodeNikon1Mode";
      case PTPPropertyCodeNikonCSMMenuBankSelect: return @"PTPPropertyCodeNikonCSMMenuBankSelect";
      case PTPPropertyCodeNikonMenuBankNameA: return @"PTPPropertyCodeNikonMenuBankNameA";
      case PTPPropertyCodeNikonMenuBankNameB: return @"PTPPropertyCodeNikonMenuBankNameB";
      case PTPPropertyCodeNikonMenuBankNameC: return @"PTPPropertyCodeNikonMenuBankNameC";
      case PTPPropertyCodeNikonMenuBankNameD: return @"PTPPropertyCodeNikonMenuBankNameD";
      case PTPPropertyCodeNikonResetBank: return @"PTPPropertyCodeNikonResetBank";
      case PTPPropertyCodeNikonA1AFCModePriority: return @"PTPPropertyCodeNikonA1AFCModePriority";
      case PTPPropertyCodeNikonA2AFSModePriority: return @"PTPPropertyCodeNikonA2AFSModePriority";
      case PTPPropertyCodeNikonA3GroupDynamicAF: return @"PTPPropertyCodeNikonA3GroupDynamicAF";
      case PTPPropertyCodeNikonA4AFActivation: return @"PTPPropertyCodeNikonA4AFActivation";
      case PTPPropertyCodeNikonFocusAreaIllumManualFocus: return @"PTPPropertyCodeNikonFocusAreaIllumManualFocus";
      case PTPPropertyCodeNikonFocusAreaIllumContinuous: return @"PTPPropertyCodeNikonFocusAreaIllumContinuous";
      case PTPPropertyCodeNikonFocusAreaIllumWhenSelected: return @"PTPPropertyCodeNikonFocusAreaIllumWhenSelected";
      case PTPPropertyCodeNikonFocusAreaWrap: return @"PTPPropertyCodeNikonFocusAreaWrap";
      case PTPPropertyCodeNikonVerticalAFON: return @"PTPPropertyCodeNikonVerticalAFON";
      case PTPPropertyCodeNikonAFLockOn: return @"PTPPropertyCodeNikonAFLockOn";
      case PTPPropertyCodeNikonFocusAreaZone: return @"PTPPropertyCodeNikonFocusAreaZone";
      case PTPPropertyCodeNikonEnableCopyright: return @"PTPPropertyCodeNikonEnableCopyright";
      case PTPPropertyCodeNikonISOAutoTime: return @"PTPPropertyCodeNikonISOAutoTime";
      case PTPPropertyCodeNikonEVISOStep: return @"PTPPropertyCodeNikonEVISOStep";
      case PTPPropertyCodeNikonEVStep: return @"PTPPropertyCodeNikonEVStep";
      case PTPPropertyCodeNikonEVStepExposureComp: return @"PTPPropertyCodeNikonEVStepExposureComp";
      case PTPPropertyCodeNikonExposureCompensation: return @"PTPPropertyCodeNikonExposureCompensation";
      case PTPPropertyCodeNikonCenterWeightArea: return @"PTPPropertyCodeNikonCenterWeightArea";
      case PTPPropertyCodeNikonExposureBaseMatrix: return @"PTPPropertyCodeNikonExposureBaseMatrix";
      case PTPPropertyCodeNikonExposureBaseCenter: return @"PTPPropertyCodeNikonExposureBaseCenter";
      case PTPPropertyCodeNikonExposureBaseSpot: return @"PTPPropertyCodeNikonExposureBaseSpot";
      case PTPPropertyCodeNikonLiveViewAFArea: return @"PTPPropertyCodeNikonLiveViewAFArea";
      case PTPPropertyCodeNikonAELockMode: return @"PTPPropertyCodeNikonAELockMode";
      case PTPPropertyCodeNikonAELAFLMode: return @"PTPPropertyCodeNikonAELAFLMode";
      case PTPPropertyCodeNikonLiveViewAFFocus: return @"PTPPropertyCodeNikonLiveViewAFFocus";
      case PTPPropertyCodeNikonMeterOff: return @"PTPPropertyCodeNikonMeterOff";
      case PTPPropertyCodeNikonSelfTimer: return @"PTPPropertyCodeNikonSelfTimer";
      case PTPPropertyCodeNikonMonitorOff: return @"PTPPropertyCodeNikonMonitorOff";
      case PTPPropertyCodeNikonImgConfTime: return @"PTPPropertyCodeNikonImgConfTime";
      case PTPPropertyCodeNikonAutoOffTimers: return @"PTPPropertyCodeNikonAutoOffTimers";
      case PTPPropertyCodeNikonAngleLevel: return @"PTPPropertyCodeNikonAngleLevel";
      case PTPPropertyCodeNikonD1ShootingSpeed: return @"PTPPropertyCodeNikonD1ShootingSpeed";
      case PTPPropertyCodeNikonD2MaximumShots: return @"PTPPropertyCodeNikonD2MaximumShots";
      case PTPPropertyCodeNikonExposureDelayMode: return @"PTPPropertyCodeNikonExposureDelayMode";
      case PTPPropertyCodeNikonLongExposureNoiseReduction: return @"PTPPropertyCodeNikonLongExposureNoiseReduction";
      case PTPPropertyCodeNikonFileNumberSequence: return @"PTPPropertyCodeNikonFileNumberSequence";
      case PTPPropertyCodeNikonControlPanelFinderRearControl: return @"PTPPropertyCodeNikonControlPanelFinderRearControl";
      case PTPPropertyCodeNikonControlPanelFinderViewfinder: return @"PTPPropertyCodeNikonControlPanelFinderViewfinder";
      case PTPPropertyCodeNikonD7Illumination: return @"PTPPropertyCodeNikonD7Illumination";
      case PTPPropertyCodeNikonNrHighISO: return @"PTPPropertyCodeNikonNrHighISO";
      case PTPPropertyCodeNikonSHSETCHGUIDDISP: return @"PTPPropertyCodeNikonSHSETCHGUIDDISP";
      case PTPPropertyCodeNikonArtistName: return @"PTPPropertyCodeNikonArtistName";
      case PTPPropertyCodeNikonCopyrightInfo: return @"PTPPropertyCodeNikonCopyrightInfo";
      case PTPPropertyCodeNikonFlashSyncSpeed: return @"PTPPropertyCodeNikonFlashSyncSpeed";
      case PTPPropertyCodeNikonFlashShutterSpeed: return @"PTPPropertyCodeNikonFlashShutterSpeed";
      case PTPPropertyCodeNikonE3AAFlashMode: return @"PTPPropertyCodeNikonE3AAFlashMode";
      case PTPPropertyCodeNikonE4ModelingFlash: return @"PTPPropertyCodeNikonE4ModelingFlash";
      case PTPPropertyCodeNikonBracketSet: return @"PTPPropertyCodeNikonBracketSet";
      case PTPPropertyCodeNikonE6ManualModeBracketing: return @"PTPPropertyCodeNikonE6ManualModeBracketing";
      case PTPPropertyCodeNikonBracketOrder: return @"PTPPropertyCodeNikonBracketOrder";
      case PTPPropertyCodeNikonE8AutoBracketSelection: return @"PTPPropertyCodeNikonE8AutoBracketSelection";
      case PTPPropertyCodeNikonBracketingSet: return @"PTPPropertyCodeNikonBracketingSet";
      case PTPPropertyCodeNikonF1CenterButtonShootingMode: return @"PTPPropertyCodeNikonF1CenterButtonShootingMode";
      case PTPPropertyCodeNikonCenterButtonPlaybackMode: return @"PTPPropertyCodeNikonCenterButtonPlaybackMode";
      case PTPPropertyCodeNikonF2Multiselector: return @"PTPPropertyCodeNikonF2Multiselector";
      case PTPPropertyCodeNikonF3PhotoInfoPlayback: return @"PTPPropertyCodeNikonF3PhotoInfoPlayback";
      case PTPPropertyCodeNikonF4AssignFuncButton: return @"PTPPropertyCodeNikonF4AssignFuncButton";
      case PTPPropertyCodeNikonF5CustomizeCommDials: return @"PTPPropertyCodeNikonF5CustomizeCommDials";
      case PTPPropertyCodeNikonReverseCommandDial: return @"PTPPropertyCodeNikonReverseCommandDial";
      case PTPPropertyCodeNikonApertureSetting: return @"PTPPropertyCodeNikonApertureSetting";
      case PTPPropertyCodeNikonMenusAndPlayback: return @"PTPPropertyCodeNikonMenusAndPlayback";
      case PTPPropertyCodeNikonF6ButtonsAndDials: return @"PTPPropertyCodeNikonF6ButtonsAndDials";
      case PTPPropertyCodeNikonNoCFCard: return @"PTPPropertyCodeNikonNoCFCard";
      case PTPPropertyCodeNikonCenterButtonZoomRatio: return @"PTPPropertyCodeNikonCenterButtonZoomRatio";
      case PTPPropertyCodeNikonFunctionButton2: return @"PTPPropertyCodeNikonFunctionButton2";
      case PTPPropertyCodeNikonAFAreaPoint: return @"PTPPropertyCodeNikonAFAreaPoint";
      case PTPPropertyCodeNikonNormalAFOn: return @"PTPPropertyCodeNikonNormalAFOn";
      case PTPPropertyCodeNikonCleanImageSensor: return @"PTPPropertyCodeNikonCleanImageSensor";
      case PTPPropertyCodeNikonImageCommentString: return @"PTPPropertyCodeNikonImageCommentString";
      case PTPPropertyCodeNikonImageCommentEnable: return @"PTPPropertyCodeNikonImageCommentEnable";
      case PTPPropertyCodeNikonImageRotation: return @"PTPPropertyCodeNikonImageRotation";
      case PTPPropertyCodeNikonManualSetLensNo: return @"PTPPropertyCodeNikonManualSetLensNo";
      case PTPPropertyCodeNikonMovScreenSize: return @"PTPPropertyCodeNikonMovScreenSize";
      case PTPPropertyCodeNikonMovVoice: return @"PTPPropertyCodeNikonMovVoice";
      case PTPPropertyCodeNikonMovMicrophone: return @"PTPPropertyCodeNikonMovMicrophone";
      case PTPPropertyCodeNikonMovFileSlot: return @"PTPPropertyCodeNikonMovFileSlot";
      case PTPPropertyCodeNikonMovRecProhibitCondition: return @"PTPPropertyCodeNikonMovRecProhibitCondition";
      case PTPPropertyCodeNikonManualMovieSetting: return @"PTPPropertyCodeNikonManualMovieSetting";
      case PTPPropertyCodeNikonMovQuality: return @"PTPPropertyCodeNikonMovQuality";
      case PTPPropertyCodeNikonLiveViewScreenDisplaySetting: return @"PTPPropertyCodeNikonLiveViewScreenDisplaySetting";
      case PTPPropertyCodeNikonMonitorOffDelay: return @"PTPPropertyCodeNikonMonitorOffDelay";
      case PTPPropertyCodeNikonBracketing: return @"PTPPropertyCodeNikonBracketing";
      case PTPPropertyCodeNikonAutoExposureBracketStep: return @"PTPPropertyCodeNikonAutoExposureBracketStep";
      case PTPPropertyCodeNikonAutoExposureBracketProgram: return @"PTPPropertyCodeNikonAutoExposureBracketProgram";
      case PTPPropertyCodeNikonAutoExposureBracketCount: return @"PTPPropertyCodeNikonAutoExposureBracketCount";
      case PTPPropertyCodeNikonWhiteBalanceBracketStep: return @"PTPPropertyCodeNikonWhiteBalanceBracketStep";
      case PTPPropertyCodeNikonWhiteBalanceBracketProgram: return @"PTPPropertyCodeNikonWhiteBalanceBracketProgram";
      case PTPPropertyCodeNikonLensID: return @"PTPPropertyCodeNikonLensID";
      case PTPPropertyCodeNikonLensSort: return @"PTPPropertyCodeNikonLensSort";
      case PTPPropertyCodeNikonLensType: return @"PTPPropertyCodeNikonLensType";
      case PTPPropertyCodeNikonFocalLengthMin: return @"PTPPropertyCodeNikonFocalLengthMin";
      case PTPPropertyCodeNikonFocalLengthMax: return @"PTPPropertyCodeNikonFocalLengthMax";
      case PTPPropertyCodeNikonMaxApAtMinFocalLength: return @"PTPPropertyCodeNikonMaxApAtMinFocalLength";
      case PTPPropertyCodeNikonMaxApAtMaxFocalLength: return @"PTPPropertyCodeNikonMaxApAtMaxFocalLength";
      case PTPPropertyCodeNikonFinderISODisp: return @"PTPPropertyCodeNikonFinderISODisp";
      case PTPPropertyCodeNikonAutoOffPhoto: return @"PTPPropertyCodeNikonAutoOffPhoto";
      case PTPPropertyCodeNikonAutoOffMenu: return @"PTPPropertyCodeNikonAutoOffMenu";
      case PTPPropertyCodeNikonAutoOffInfo: return @"PTPPropertyCodeNikonAutoOffInfo";
      case PTPPropertyCodeNikonSelfTimerShootNum: return @"PTPPropertyCodeNikonSelfTimerShootNum";
      case PTPPropertyCodeNikonVignetteCtrl: return @"PTPPropertyCodeNikonVignetteCtrl";
      case PTPPropertyCodeNikonAutoDistortionControl: return @"PTPPropertyCodeNikonAutoDistortionControl";
      case PTPPropertyCodeNikonSceneMode: return @"PTPPropertyCodeNikonSceneMode";
      case PTPPropertyCodeNikonSceneMode2: return @"PTPPropertyCodeNikonSceneMode2";
      case PTPPropertyCodeNikonSelfTimerInterval: return @"PTPPropertyCodeNikonSelfTimerInterval";
      case PTPPropertyCodeNikonExposureTime: return @"PTPPropertyCodeNikonExposureTime";
      case PTPPropertyCodeNikonACPower: return @"PTPPropertyCodeNikonACPower";
      case PTPPropertyCodeNikonWarningStatus: return @"PTPPropertyCodeNikonWarningStatus";
      case PTPPropertyCodeNikonMaximumShots: return @"PTPPropertyCodeNikonMaximumShots";
      case PTPPropertyCodeNikonAFLockStatus: return @"PTPPropertyCodeNikonAFLockStatus";
      case PTPPropertyCodeNikonAELockStatus: return @"PTPPropertyCodeNikonAELockStatus";
      case PTPPropertyCodeNikonFVLockStatus: return @"PTPPropertyCodeNikonFVLockStatus";
      case PTPPropertyCodeNikonAutofocusLCDTopMode2: return @"PTPPropertyCodeNikonAutofocusLCDTopMode2";
      case PTPPropertyCodeNikonAutofocusArea: return @"PTPPropertyCodeNikonAutofocusArea";
      case PTPPropertyCodeNikonFlexibleProgram: return @"PTPPropertyCodeNikonFlexibleProgram";
      case PTPPropertyCodeNikonLightMeter: return @"PTPPropertyCodeNikonLightMeter";
      case PTPPropertyCodeNikonRecordingMedia: return @"PTPPropertyCodeNikonRecordingMedia";
      case PTPPropertyCodeNikonUSBSpeed: return @"PTPPropertyCodeNikonUSBSpeed";
      case PTPPropertyCodeNikonCCDNumber: return @"PTPPropertyCodeNikonCCDNumber";
      case PTPPropertyCodeNikonCameraOrientation: return @"PTPPropertyCodeNikonCameraOrientation";
      case PTPPropertyCodeNikonGroupPtnType: return @"PTPPropertyCodeNikonGroupPtnType";
      case PTPPropertyCodeNikonFNumberLock: return @"PTPPropertyCodeNikonFNumberLock";
      case PTPPropertyCodeNikonExposureApertureLock: return @"PTPPropertyCodeNikonExposureApertureLock";
      case PTPPropertyCodeNikonTVLockSetting: return @"PTPPropertyCodeNikonTVLockSetting";
      case PTPPropertyCodeNikonAVLockSetting: return @"PTPPropertyCodeNikonAVLockSetting";
      case PTPPropertyCodeNikonIllumSetting: return @"PTPPropertyCodeNikonIllumSetting";
      case PTPPropertyCodeNikonFocusPointBright: return @"PTPPropertyCodeNikonFocusPointBright";
      case PTPPropertyCodeNikonExternalFlashAttached: return @"PTPPropertyCodeNikonExternalFlashAttached";
      case PTPPropertyCodeNikonExternalFlashStatus: return @"PTPPropertyCodeNikonExternalFlashStatus";
      case PTPPropertyCodeNikonExternalFlashSort: return @"PTPPropertyCodeNikonExternalFlashSort";
      case PTPPropertyCodeNikonExternalFlashMode: return @"PTPPropertyCodeNikonExternalFlashMode";
      case PTPPropertyCodeNikonExternalFlashCompensation: return @"PTPPropertyCodeNikonExternalFlashCompensation";
      case PTPPropertyCodeNikonNewExternalFlashMode: return @"PTPPropertyCodeNikonNewExternalFlashMode";
      case PTPPropertyCodeNikonFlashExposureCompensation: return @"PTPPropertyCodeNikonFlashExposureCompensation";
      case PTPPropertyCodeNikonHDRMode: return @"PTPPropertyCodeNikonHDRMode";
      case PTPPropertyCodeNikonHDRHighDynamic: return @"PTPPropertyCodeNikonHDRHighDynamic";
      case PTPPropertyCodeNikonHDRSmoothing: return @"PTPPropertyCodeNikonHDRSmoothing";
      case PTPPropertyCodeNikonOptimizeImage: return @"PTPPropertyCodeNikonOptimizeImage";
      case PTPPropertyCodeNikonSaturation: return @"PTPPropertyCodeNikonSaturation";
      case PTPPropertyCodeNikonBWFillerEffect: return @"PTPPropertyCodeNikonBWFillerEffect";
      case PTPPropertyCodeNikonBWSharpness: return @"PTPPropertyCodeNikonBWSharpness";
      case PTPPropertyCodeNikonBWContrast: return @"PTPPropertyCodeNikonBWContrast";
      case PTPPropertyCodeNikonBWSettingType: return @"PTPPropertyCodeNikonBWSettingType";
      case PTPPropertyCodeNikonSlot2SaveMode: return @"PTPPropertyCodeNikonSlot2SaveMode";
      case PTPPropertyCodeNikonRawBitMode: return @"PTPPropertyCodeNikonRawBitMode";
      case PTPPropertyCodeNikonActiveDLighting: return @"PTPPropertyCodeNikonActiveDLighting";
      case PTPPropertyCodeNikonFlourescentType: return @"PTPPropertyCodeNikonFlourescentType";
      case PTPPropertyCodeNikonTuneColourTemperature: return @"PTPPropertyCodeNikonTuneColourTemperature";
      case PTPPropertyCodeNikonTunePreset0: return @"PTPPropertyCodeNikonTunePreset0";
      case PTPPropertyCodeNikonTunePreset1: return @"PTPPropertyCodeNikonTunePreset1";
      case PTPPropertyCodeNikonTunePreset2: return @"PTPPropertyCodeNikonTunePreset2";
      case PTPPropertyCodeNikonTunePreset3: return @"PTPPropertyCodeNikonTunePreset3";
      case PTPPropertyCodeNikonTunePreset4: return @"PTPPropertyCodeNikonTunePreset4";
      case PTPPropertyCodeNikonBeepOff: return @"PTPPropertyCodeNikonBeepOff";
      case PTPPropertyCodeNikonAutofocusMode: return @"PTPPropertyCodeNikonAutofocusMode";
      case PTPPropertyCodeNikonAFAssist: return @"PTPPropertyCodeNikonAFAssist";
      case PTPPropertyCodeNikonPADVPMode: return @"PTPPropertyCodeNikonPADVPMode";
      case PTPPropertyCodeNikonImageReview: return @"PTPPropertyCodeNikonImageReview";
      case PTPPropertyCodeNikonAFAreaIllumination: return @"PTPPropertyCodeNikonAFAreaIllumination";
      case PTPPropertyCodeNikonFlashMode: return @"PTPPropertyCodeNikonFlashMode";
      case PTPPropertyCodeNikonFlashCommanderMode: return @"PTPPropertyCodeNikonFlashCommanderMode";
      case PTPPropertyCodeNikonFlashSign: return @"PTPPropertyCodeNikonFlashSign";
      case PTPPropertyCodeNikonISOAuto: return @"PTPPropertyCodeNikonISOAuto";
      case PTPPropertyCodeNikonRemoteTimeout: return @"PTPPropertyCodeNikonRemoteTimeout";
      case PTPPropertyCodeNikonGridDisplay: return @"PTPPropertyCodeNikonGridDisplay";
      case PTPPropertyCodeNikonFlashModeManualPower: return @"PTPPropertyCodeNikonFlashModeManualPower";
      case PTPPropertyCodeNikonFlashModeCommanderPower: return @"PTPPropertyCodeNikonFlashModeCommanderPower";
      case PTPPropertyCodeNikonAutoFP: return @"PTPPropertyCodeNikonAutoFP";
      case PTPPropertyCodeNikonDateImprintSetting: return @"PTPPropertyCodeNikonDateImprintSetting";
      case PTPPropertyCodeNikonDateCounterSelect: return @"PTPPropertyCodeNikonDateCounterSelect";
      case PTPPropertyCodeNikonDateCountData: return @"PTPPropertyCodeNikonDateCountData";
      case PTPPropertyCodeNikonDateCountDisplaySetting: return @"PTPPropertyCodeNikonDateCountDisplaySetting";
      case PTPPropertyCodeNikonRangeFinderSetting: return @"PTPPropertyCodeNikonRangeFinderSetting";
      case PTPPropertyCodeNikonCSMMenu: return @"PTPPropertyCodeNikonCSMMenu";
      case PTPPropertyCodeNikonWarningDisplay: return @"PTPPropertyCodeNikonWarningDisplay";
      case PTPPropertyCodeNikonBatteryCellKind: return @"PTPPropertyCodeNikonBatteryCellKind";
      case PTPPropertyCodeNikonISOAutoHiLimit: return @"PTPPropertyCodeNikonISOAutoHiLimit";
      case PTPPropertyCodeNikonDynamicAFArea: return @"PTPPropertyCodeNikonDynamicAFArea";
      case PTPPropertyCodeNikonContinuousSpeedHigh: return @"PTPPropertyCodeNikonContinuousSpeedHigh";
      case PTPPropertyCodeNikonInfoDispSetting: return @"PTPPropertyCodeNikonInfoDispSetting";
      case PTPPropertyCodeNikonPreviewButton: return @"PTPPropertyCodeNikonPreviewButton";
      case PTPPropertyCodeNikonPreviewButton2: return @"PTPPropertyCodeNikonPreviewButton2";
      case PTPPropertyCodeNikonAEAFLockButton2: return @"PTPPropertyCodeNikonAEAFLockButton2";
      case PTPPropertyCodeNikonIndicatorDisp: return @"PTPPropertyCodeNikonIndicatorDisp";
      case PTPPropertyCodeNikonCellKindPriority: return @"PTPPropertyCodeNikonCellKindPriority";
      case PTPPropertyCodeNikonBracketingFramesAndSteps: return @"PTPPropertyCodeNikonBracketingFramesAndSteps";
      case PTPPropertyCodeNikonLiveViewMode: return @"PTPPropertyCodeNikonLiveViewMode";
      case PTPPropertyCodeNikonLiveViewDriveMode: return @"PTPPropertyCodeNikonLiveViewDriveMode";
      case PTPPropertyCodeNikonLiveViewStatus: return @"PTPPropertyCodeNikonLiveViewStatus";
      case PTPPropertyCodeNikonLiveViewImageZoomRatio: return @"PTPPropertyCodeNikonLiveViewImageZoomRatio";
      case PTPPropertyCodeNikonLiveViewProhibitCondition: return @"PTPPropertyCodeNikonLiveViewProhibitCondition";
      case PTPPropertyCodeNikonMovieShutterSpeed: return @"PTPPropertyCodeNikonMovieShutterSpeed";
      case PTPPropertyCodeNikonMovieFNumber: return @"PTPPropertyCodeNikonMovieFNumber";
      case PTPPropertyCodeNikonMovieISO: return @"PTPPropertyCodeNikonMovieISO";
      case PTPPropertyCodeNikonLiveViewMovieMode: return @"PTPPropertyCodeNikonLiveViewMovieMode";
      case PTPPropertyCodeNikonExposureDisplayStatus: return @"PTPPropertyCodeNikonExposureDisplayStatus";
      case PTPPropertyCodeNikonExposureIndicateStatus: return @"PTPPropertyCodeNikonExposureIndicateStatus";
      case PTPPropertyCodeNikonInfoDispErrStatus: return @"PTPPropertyCodeNikonInfoDispErrStatus";
      case PTPPropertyCodeNikonExposureIndicateLightup: return @"PTPPropertyCodeNikonExposureIndicateLightup";
      case PTPPropertyCodeNikonFlashOpen: return @"PTPPropertyCodeNikonFlashOpen";
      case PTPPropertyCodeNikonFlashCharged: return @"PTPPropertyCodeNikonFlashCharged";
      case PTPPropertyCodeNikonFlashMRepeatValue: return @"PTPPropertyCodeNikonFlashMRepeatValue";
      case PTPPropertyCodeNikonFlashMRepeatCount: return @"PTPPropertyCodeNikonFlashMRepeatCount";
      case PTPPropertyCodeNikonFlashMRepeatInterval: return @"PTPPropertyCodeNikonFlashMRepeatInterval";
      case PTPPropertyCodeNikonFlashCommandChannel: return @"PTPPropertyCodeNikonFlashCommandChannel";
      case PTPPropertyCodeNikonFlashCommandSelfMode: return @"PTPPropertyCodeNikonFlashCommandSelfMode";
      case PTPPropertyCodeNikonFlashCommandSelfCompensation: return @"PTPPropertyCodeNikonFlashCommandSelfCompensation";
      case PTPPropertyCodeNikonFlashCommandSelfValue: return @"PTPPropertyCodeNikonFlashCommandSelfValue";
      case PTPPropertyCodeNikonFlashCommandAMode: return @"PTPPropertyCodeNikonFlashCommandAMode";
      case PTPPropertyCodeNikonFlashCommandACompensation: return @"PTPPropertyCodeNikonFlashCommandACompensation";
      case PTPPropertyCodeNikonFlashCommandAValue: return @"PTPPropertyCodeNikonFlashCommandAValue";
      case PTPPropertyCodeNikonFlashCommandBMode: return @"PTPPropertyCodeNikonFlashCommandBMode";
      case PTPPropertyCodeNikonFlashCommandBCompensation: return @"PTPPropertyCodeNikonFlashCommandBCompensation";
      case PTPPropertyCodeNikonFlashCommandBValue: return @"PTPPropertyCodeNikonFlashCommandBValue";
      case PTPPropertyCodeNikonApplicationMode: return @"PTPPropertyCodeNikonApplicationMode";
      case PTPPropertyCodeNikonActiveSlot: return @"PTPPropertyCodeNikonActiveSlot";
      case PTPPropertyCodeNikonActivePicCtrlItem: return @"PTPPropertyCodeNikonActivePicCtrlItem";
      case PTPPropertyCodeNikonChangePicCtrlItem: return @"PTPPropertyCodeNikonChangePicCtrlItem";
      case PTPPropertyCodeNikonMovieNrHighISO: return @"PTPPropertyCodeNikonMovieNrHighISO";		}
  }
  return [NSString stringWithFormat:@"PTPPropertyCode_%04X", propertyCode];
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
  return [NSString stringWithFormat:@"PTPDataTypeCode_%04X", type];
}

- (id)initWithCode:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension{
  if ((self = [super initWithVendorExtension:vendorExtension])) {
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
  if ((self = [super initWithVendorExtension:vendorExtension])) {
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

- (NSString *)propertyCodeName {
  return [PTPProperty propertyCodeName:_propertyCode vendorExtension:self.vendorExtension];
}

- (NSString *)typeName {
  return [PTPProperty typeName:_type];
}

- (NSString*)description {
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

- (id)initWithData:(NSData*)data {
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
    
    if (self.vendorExtension == PTPVendorExtensionMicrosoft && [_manufacturer containsString:@"Nikon"]) {
      self.vendorExtension = PTPVendorExtensionNikon;
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
    
    _properties = [NSMutableDictionary dictionary];
  }
  return self;
}

- (NSString *)description {
  if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
    return [self debug];
  return [NSString stringWithFormat:@"%@ %@, PTP V%.2f + %8@ V%.2f", _model, _version, _standardVersion / 100.0, _vendorExtensionDesc, _vendorExtensionVersion / 100.0];
}

- (NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %8@ V%.2f\n", _model, _version, _standardVersion / 100.0, _vendorExtensionDesc, _vendorExtensionVersion / 100.0];
  if (_operationsSupported.count > 0) {
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in _operationsSupported)
      [s appendFormat:@"%@\n", [PTPOperationRequest operationCodeName:code.intValue vendorExtension:self.vendorExtension]];
  }
  if (_eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in _eventsSupported)
      [s appendFormat:@"%@\n", [PTPEvent eventCodeName:code.intValue vendorExtension:self.vendorExtension]];
  }
  if (_propertiesSupported.count > 0) {
    [s appendFormat:@"\nProperties:\n"];
    for (NSNumber *code in _propertiesSupported) {
      PTPProperty *property = _properties[code];
      NSLog(@"%@", property);
      if (property)
        [s appendFormat:@"%@\n", property];
      else
        [s appendFormat:@"%@\n", [PTPProperty propertyCodeName:code.intValue vendorExtension:self.vendorExtension]];
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
  if (info.vendorExtension == PTPVendorExtensionNikon && [info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPOperationCodeNikonCheckEvent]]) {
    [self sendPTPRequest:PTPOperationCodeNikonCheckEvent];
  }
}

- (void)getLiveViewImage {
	[self sendPTPRequest:PTPOperationCodeNikonGetLiveViewImg];
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
  if (response) {
    PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
    PTPOperationResponse* ptpResponse = [[PTPOperationResponse alloc] initWithData:response vendorExtension:info.vendorExtension];
		switch (ptpRequest.operationCode) {
			case PTPOperationCodeSetDevicePropValue:
				[self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:ptpRequest.parameter1];
				break;
    }
    if (ptpResponse.responseCode != PTPResponseCodeOK || indigo_get_log_level() >= INDIGO_LOG_DEBUG)
      NSLog(@"Completed %@ with %@", ptpRequest, ptpResponse);
	} else {
		if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
			NSLog(@"Completed %@", ptpRequest);
	}
	switch (ptpRequest.operationCode) {
		case PTPOperationCodeGetStorageIDs: {
			PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
			NSLog(@"Initialized %@\n", info);
			[(PTPDelegate *)self.delegate cameraConnected:self];
			NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(checkForEvent) userInfo:nil repeats:true];
			[self.userData setObject:timer forKey:PTP_EVENT_TIMER];
			break;
		}
	}
  if (data) {
    switch (ptpRequest.operationCode) {
      case PTPOperationCodeGetDeviceInfo: {
        PTPDeviceInfo *info = [[PTPDeviceInfo alloc] initWithData:data];
        [self.userData setObject:info forKey:PTP_DEVICE_INFO];
        if ([info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPOperationCodeInitiateCapture]]) {
          [(PTPDelegate *)self.delegate cameraCanCapture:self];
        }
        if ([info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPOperationCodeNikonMfDrive]]) {
          [(PTPDelegate *)self.delegate cameraCanFocus:self];
        }
        if (info.vendorExtension == PTPVendorExtensionNikon && [info.operationsSupported containsObject:[NSNumber numberWithUnsignedShort:PTPOperationCodeNikonGetVendorPropCodes]]) {
          [self sendPTPRequest:PTPOperationCodeNikonGetVendorPropCodes];
        } else {
          for (NSNumber *code in info.propertiesSupported) {
            [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:code.unsignedShortValue];
          }
          [self sendPTPRequest:PTPOperationCodeGetStorageIDs];
        }
        break;
      }
      case PTPOperationCodeNikonGetVendorPropCodes: {
        unsigned char* buffer = (unsigned char*)[data bytes];
        unsigned char* buf = buffer;
        NSArray *codes = ptpReadUnsignedShortArray(&buf);
        PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
        [(NSMutableArray *)info.propertiesSupported addObjectsFromArray:codes];
        for (NSNumber *code in info.propertiesSupported) {
          [self sendPTPRequest:PTPOperationCodeGetDevicePropDesc param1:code.unsignedShortValue];
        }
        [self sendPTPRequest:PTPOperationCodeGetStorageIDs];
        break;
      }
      case PTPOperationCodeGetDevicePropDesc: {
        PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
        PTPProperty *property = [[PTPProperty alloc] initWithData:data vendorExtension:info.vendorExtension];
        if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
          NSLog(@"Translated to %@", property);
        info.properties[[NSNumber numberWithUnsignedShort:property.propertyCode]] = property;
        switch (property.propertyCode) {
          case PTPPropertyCodeExposureProgramMode: {
            NSDictionary *map = @{ @1: @"Manual", @2: @"Program", @3: @"Aperture priority", @4: @"Shutter priority", @32784: @"Auto", @32785: @"Portrait", @32786: @"Landscape", @32787:@"Macro", @32788: @"Sport", @32789: @"Night portrait", @32790:@"Night landscape", @32791: @"Child", @32792: @"Scene", @32793: @"Effects" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeFNumber: {
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              [labels addObject:[NSString stringWithFormat:@"f/%g", value.intValue / 100.0]];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureTime: {
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              int intValue = value.intValue;
              if (intValue == -1)
                [labels addObject:[NSString stringWithFormat:@"Bulb"]];
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
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeImageSize: {
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSString *value in property.supportedValues) {
              [values addObject:value];
              [labels addObject:value];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeCompressionSetting: {
            NSDictionary *map = @{ @0: @"JPEG Basic", @1: @"JPEG Norm", @2: @"JPEG Fine", @4: @"RAW", @5: @"RAW + JPEG Basic", @6: @"RAW + JPEG Norm", @7: @"RAW + JPEG Fine" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeWhiteBalance: {
            NSDictionary *map = @{ @1: @"Manual", @2: @"Auto", @3: @"One-push Auto", @4: @"Daylight", @5: @"Fluorescent", @6: @"Incandescent", @7: @"Flash", @32784: @"Cloudy", @32785: @"Shade", @32786: @"Color Temperature", @32787: @"Preset" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureIndex: {
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              [labels addObject:value.description];
            }
						[(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureBiasCompensation: {
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              [labels addObject:[NSString stringWithFormat:@"%.1f", round(value.intValue / 100.0) / 10.0]];
            }
            [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeExposureMeteringMode: {
            NSDictionary *map = @{ @1: @"Average", @2: @"Center-Weighted Average", @3: @"Multi-spot", @4: @"Center-spot" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeFocusMeteringMode: {
            NSDictionary *map = @{ @1: @"Center-spot", @2: @"Multi-spot", @32784: @"Single Area", @32785: @"Auto area", @32786: @"3D tracking" };
            NSMutableArray *values = [NSMutableArray array];
            NSMutableArray *labels = [NSMutableArray array];
            for (NSNumber *value in property.supportedValues) {
              [values addObject:value.description];
              NSString *label = map[value];
              if (label)
                [labels addObject:label];
              else
                [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
            }
            [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            break;
          }
          case PTPPropertyCodeFocusMode: {
            if (info.vendorExtension != PTPVendorExtensionNikon) {
              NSDictionary *map = @{ @1: @"Manual", @2: @"Automatic", @3:@"Macro", @32784: @"AF-S", @32785: @"AF-C", @32786: @"AF-A" };
              NSMutableArray *values = [NSMutableArray array];
              NSMutableArray *labels = [NSMutableArray array];
              for (NSNumber *value in property.supportedValues) {
                [values addObject:value.description];
                NSString *label = map[value];
                if (label)
                  [labels addObject:label];
                else
                  [labels addObject:[NSString stringWithFormat:@"0x%04x", value.intValue]];
              }
              [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
            }
            break;
          }
          case PTPPropertyCodeNikonAutofocusMode: {
            if (info.vendorExtension == PTPVendorExtensionNikon) {
              if (property.value.description.intValue == 3) {
                [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:PTPPropertyCodeFocusMode value:@"3" values:@[@"3"] labels:@[@"M (fixed)"] readOnly:true];
              } else {
                [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:PTPPropertyCodeFocusMode value:property.value.description values:@[@"0", @"1", @"2", @"4"] labels:@[@"AF-S", @"AF-C", @"AF-A", @"M"] readOnly:property.readOnly];
              }
            }
            break;
          }
					case PTPPropertyCodeNikonLiveViewStatus: {
            [(PTPDelegate *)self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:nil labels:nil readOnly:property.readOnly];
						if (property.value.description.intValue) {
						//	[self sendPTPRequest:PTPOperationCodeNikonGetLiveViewImg];
							NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(getLiveViewImage) userInfo:nil repeats:true];
							[self.userData setObject:timer forKey:PTP_LIVE_VIEW_TIMER];
						} else {
							NSTimer *timer = self.userData[PTP_LIVE_VIEW_TIMER];
							[timer invalidate];
						}
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
          PTPEvent *event = [[PTPEvent alloc] initWithCode:code parameter1:parameter1 vendorExtension:self.ptpDeviceInfo.vendorExtension];
					if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
						NSLog(@"Translated to %@", [event description]);
					}
          [self processEvent:event];
        }
        break;
      }
			case PTPOperationCodeNikonGetLiveViewImg: {
        char *bytes = (void*)[data bytes];
				NSData *image;
				if ((bytes[64] & 0xFF) == 0xFF && (bytes[65] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 64 length:data.length - 64];
				else if ((bytes[128] & 0xFF) == 0xFF && (bytes[129] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 128 length:data.length - 128];
				else if ((bytes[384] & 0xFF) == 0xFF && (bytes[385] & 0xFF) == 0xD8)
					image = [NSData dataWithBytes:bytes + 384 length:data.length - 384];
				if (image) {
					[(PTPDelegate *)self.delegate cameraExposureDone:self data:image filename:@"preview.jpeg"];
					//[self sendPTPRequest:PTPOperationCodeNikonGetLiveViewImg];
				}
				break;
			}
      default: {
        char *bytes = (void*)[data bytes];
        if (indigo_get_log_level() >= INDIGO_LOG_TRACE) {
          [self dumpData:(void *)bytes length:(int)[data length] comment:@"Received data"];
        }
        break;
      }
    }
  }
}

-(PTPDeviceInfo *)ptpDeviceInfo {
  return self.userData[PTP_DEVICE_INFO];
}

-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value {
  PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
  if (code == PTPPropertyCodeFocusMode && info.vendorExtension == PTPVendorExtensionNikon)
    code = PTPPropertyCodeNikonAutofocusMode;
  PTPProperty *property = info.properties[[NSNumber numberWithUnsignedShort:code]];
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
	switch (info.vendorExtension) {
		case PTPVendorExtensionNikon:
			[self sendPTPRequest:PTPOperationCodeNikonStartLiveView];
			break;
	}
}

-(void)stopLiveView {
	PTPDeviceInfo *info = self.userData[PTP_DEVICE_INFO];
	switch (info.vendorExtension) {
		case PTPVendorExtensionNikon:
			[self sendPTPRequest:PTPOperationCodeNikonEndLiveView];
			break;
	}
}

-(void)mfDrive:(int)steps {
  if (steps >= 0) {
    [self sendPTPRequest:PTPOperationCodeNikonMfDrive param1:1 param2:steps];
  } else {
    [self sendPTPRequest:PTPOperationCodeNikonMfDrive param1:2 param2:-steps];
  }
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtension];
  request.operationCode = operationCode;
  request.numberOfParameters = 0;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtension];
  request.operationCode = operationCode;
  request.numberOfParameters = 1;
  request.parameter1 = parameter1;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 param2:(unsigned int)parameter2 {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtension];
  request.operationCode = operationCode;
  request.numberOfParameters = 2;
  request.parameter1 = parameter1;
  request.parameter2 = parameter2;
  [self requestSendPTPCommand:request.commandBuffer outData:nil sendCommandDelegate:self didSendCommandSelector:@selector(didSendPTPCommand:inData:response:error:contextInfo:) contextInfo:(void *)CFBridgingRetain(request)];
}

-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 withData:(NSData *)data {
  PTPOperationRequest *request = [[PTPOperationRequest alloc] initWithVendorExtension:self.ptpDeviceInfo.vendorExtension];
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
  NSLog(@"Error '%@' on '%@'", error.localizedDescription, camera.name);
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
	NSTimer *timer = ((ICCameraDevice *)device).userData[PTP_LIVE_VIEW_TIMER];
	[timer invalidate];
	timer = ((ICCameraDevice *)device).userData[PTP_EVENT_TIMER];
	[timer invalidate];
}

- (void)cameraDevice:(ICCameraDevice*)camera didReceivePTPEvent:(NSData*)eventData {
	PTPEvent *event = [[PTPEvent alloc] initWithData:eventData vendorExtension:camera.ptpDeviceInfo.vendorExtension];
	if (indigo_get_log_level() >= INDIGO_LOG_DEBUG)
		NSLog(@"Received %@", event);
	[camera processEvent:event];
}

@end

//------------------------------------------------------------------------------------------------------------------------------
