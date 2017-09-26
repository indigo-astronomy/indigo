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
 \file indigo_ica_ptp.h
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
// license, under Apple's copyrights in this original Apple software ( the
// "Apple Software" ), to use, reproduce, modify and redistribute the Apple
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
// CONSEQUENTIAL DAMAGES ( INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION ) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND / OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER
// UNDER THEORY OF CONTRACT, TORT ( INCLUDING NEGLIGENCE ), STRICT LIABILITY OR
// OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright ( C ) 2009 Apple Inc. All Rights Reserved.
//
//------------------------------------------------------------------------------------------------------------------------------

#import <Cocoa/Cocoa.h>
#import <ImageCaptureCore/ImageCaptureCore.h>

//------------------------------------------------------------------------------------------------------------------------------

enum PTPRequestCodeEnum {
  PTPRequestCodeUndefined = 0x1000,
  PTPRequestCodeGetDeviceInfo = 0x1001,
  PTPRequestCodeOpenSession = 0x1002,
  PTPRequestCodeCloseSession = 0x1003,
  PTPRequestCodeGetStorageIDs = 0x1004,
  PTPRequestCodeGetStorageInfo = 0x1005,
  PTPRequestCodeGetNumObjects = 0x1006,
  PTPRequestCodeGetObjectHandles = 0x1007,
  PTPRequestCodeGetObjectInfo = 0x1008,
  PTPRequestCodeGetObject = 0x1009,
  PTPRequestCodeGetThumb = 0x100A,
  PTPRequestCodeDeleteObject = 0x100B,
  PTPRequestCodeSendObjectInfo = 0x100C,
  PTPRequestCodeSendObject = 0x100D,
  PTPRequestCodeInitiateCapture = 0x100E,
  PTPRequestCodeFormatStore = 0x100F,
  PTPRequestCodeResetDevice = 0x1010,
  PTPRequestCodeSelfTest = 0x1011,
  PTPRequestCodeSetObjectProtection = 0x1012,
  PTPRequestCodePowerDown = 0x1013,
  PTPRequestCodeGetDevicePropDesc = 0x1014,
  PTPRequestCodeGetDevicePropValue = 0x1015,
  PTPRequestCodeSetDevicePropValue = 0x1016,
  PTPRequestCodeResetDevicePropValue = 0x1017,
  PTPRequestCodeTerminateOpenCapture = 0x1018,
  PTPRequestCodeMoveObject = 0x1019,
  PTPRequestCodeCopyObject = 0x101A,
  PTPRequestCodeGetPartialObject = 0x101B,
  PTPRequestCodeInitiateOpenCapture = 0x101C,
  PTPRequestCodeGetNumDownloadableObjects = 0x9001,
  PTPRequestCodeGetAllObjectInfo = 0x9002,
  PTPRequestCodeGetUserAssignedDeviceName = 0x9003,
  PTPRequestCodeMTPGetObjectPropsSupported = 0x9801,
  PTPRequestCodeMTPGetObjectPropDesc = 0x9802,
  PTPRequestCodeMTPGetObjectPropValue = 0x9803,
  PTPRequestCodeMTPSetObjectPropValue = 0x9804,
  PTPRequestCodeMTPGetObjPropList = 0x9805,
  PTPRequestCodeMTPSetObjPropList = 0x9806,
  PTPRequestCodeMTPGetInterdependendPropdesc = 0x9807,
  PTPRequestCodeMTPSendObjectPropList = 0x9808,
  PTPRequestCodeMTPGetObjectReferences = 0x9810,
  PTPRequestCodeMTPSetObjectReferences = 0x9811,
  PTPRequestCodeMTPUpdateDeviceFirmware = 0x9812,
  PTPRequestCodeMTPSkip = 0x9820,
};

typedef unsigned short PTPRequestCode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPResponseCodeEnum {
  PTPResponseCodeUndefined = 0x2000,
  PTPResponseCodeOK = 0x2001,
  PTPResponseCodeGeneralError = 0x2002,
  PTPResponseCodeSessionNotOpen = 0x2003,
  PTPResponseCodeInvalidTransactionID = 0x2004,
  PTPResponseCodeOperationNotSupported = 0x2005,
  PTPResponseCodeParameterNotSupported = 0x2006,
  PTPResponseCodeIncompleteTransfer = 0x2007,
  PTPResponseCodeInvalidStorageID = 0x2008,
  PTPResponseCodeInvalidObjectHandle = 0x2009,
  PTPResponseCodeDevicePropNotSupported = 0x200A,
  PTPResponseCodeInvalidObjectFormatCode = 0x200B,
  PTPResponseCodeStoreFull = 0x200C,
  PTPResponseCodeObjectWriteProtected = 0x200D,
  PTPResponseCodeStoreReadOnly = 0x200E,
  PTPResponseCodeAccessDenied = 0x200F,
  PTPResponseCodeNoThumbnailPresent = 0x2010,
  PTPResponseCodeSelfTestFailed = 0x2011,
  PTPResponseCodePartialDeletion = 0x2012,
  PTPResponseCodeStoreNotAvailable = 0x2013,
  PTPResponseCodeSpecificationByFormatUnsupported = 0x2014,
  PTPResponseCodeNoValidObjectInfo = 0x2015,
  PTPResponseCodeInvalidCodeFormat = 0x2016,
  PTPResponseCodeUnknownVendorCode = 0x2017,
  PTPResponseCodeCaptureAlreadyTerminated = 0x2018,
  PTPResponseCodeDeviceBusy = 0x2019,
  PTPResponseCodeInvalidParentObject = 0x201A,
  PTPResponseCodeInvalidDevicePropFormat = 0x201B,
  PTPResponseCodeInvalidDevicePropValue = 0x201C,
  PTPResponseCodeInvalidParameter = 0x201D,
  PTPResponseCodeSessionAlreadyOpen = 0x201E,
  PTPResponseCodeTransactionCancelled = 0x201F,
  PTPResponseCodeSpecificationOfDestinationUnsupported = 0x2020,
  PTPResponseCodeMTPUndefined = 0xA800,
  PTPResponseCodeMTPInvalidObjectPropCode = 0xA801,
  PTPResponseCodeMTPInvalidObjectProp_Format = 0xA802,
  PTPResponseCodeMTPInvalidObjectProp_Value = 0xA803,
  PTPResponseCodeMTPInvalidObjectReference = 0xA804,
  PTPResponseCodeMTPInvalidDataset = 0xA806,
  PTPResponseCodeMTPSpecificationByGroupUnsupported = 0xA807,
  PTPResponseCodeMTPSpecificationByDepthUnsupported = 0xA808,
  PTPResponseCodeMTPObjectTooLarge = 0xA809,
  PTPResponseCodeMTPObjectPropNotSupported = 0xA80A,
};
typedef unsigned short PTPResponseCode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPEventCodeEnum {
  PTPEventCodeUndefined = 0x4000,
  PTPEventCodeCancelTransaction = 0x4001,
  PTPEventCodeObjectAdded = 0x4002,
  PTPEventCodeObjectRemoved = 0x4003,
  PTPEventCodeStoreAdded = 0x4004,
  PTPEventCodeStoreRemoved = 0x4005,
  PTPEventCodeDevicePropChanged = 0x4006,
  PTPEventCodeObjectInfoChanged = 0x4007,
  PTPEventCodeDeviceInfoChanged = 0x4008,
  PTPEventCodeRequestObjectTransfer = 0x4009,
  PTPEventCodeStoreFull = 0x400A,
  PTPEventCodeDeviceReset = 0x400B,
  PTPEventCodeStorageInfoChanged = 0x400C,
  PTPEventCodeCaptureComplete = 0x400D,
  PTPEventCodeUnreportedStatus = 0x400E,
  PTPEventCodeAppleDeviceUnlocked = 0xC001,
  PTPEventCodeAppleUserAssignedNameChanged = 0xC002,
};

typedef unsigned short PTPEventCode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPPropertyCode {
  PTPPropertyCodeUndefined = 0x5000,
  PTPPropertyCodeBatteryLevel = 0x5001,
  PTPPropertyCodeFunctionalMode = 0x5002,
  PTPPropertyCodeImageSize = 0x5003,
  PTPPropertyCodeCompressionSetting = 0x5004,
  PTPPropertyCodeWhiteBalance = 0x5005,
  PTPPropertyCodeRGBGain = 0x5006,
  PTPPropertyCodeFNumber = 0x5007,
  PTPPropertyCodeFocalLength = 0x5008,
  PTPPropertyCodeFocusDistance = 0x5009,
  PTPPropertyCodeFocusMode = 0x500A,
  PTPPropertyCodeExposureMeteringMode = 0x500B,
  PTPPropertyCodeFlashMode = 0x500C,
  PTPPropertyCodeExposureTime = 0x500D,
  PTPPropertyCodeExposureProgramMode = 0x500E,
  PTPPropertyCodeExposureIndex = 0x500F,
  PTPPropertyCodeExposureBiasCompensation = 0x5010,
  PTPPropertyCodeDateTime = 0x5011,
  PTPPropertyCodeCaptureDelay = 0x5012,
  PTPPropertyCodeStillCaptureMode = 0x5013,
  PTPPropertyCodeContrast = 0x5014,
  PTPPropertyCodeSharpness = 0x5015,
  PTPPropertyCodeDigitalZoom = 0x5016,
  PTPPropertyCodeEffectMode = 0x5017,
  PTPPropertyCodeBurstNumber = 0x5018,
  PTPPropertyCodeBurstInterval = 0x5019,
  PTPPropertyCodeTimelapseNumber = 0x501A,
  PTPPropertyCodeTimelapseInterval = 0x501B,
  PTPPropertyCodeFocusMeteringMode = 0x501C,
  PTPPropertyCodeUploadURL = 0x501D,
  PTPPropertyCodeArtist = 0x501E,
  PTPPropertyCodeCopyrightInfo = 0x501F,
  PTPPropertyCodeSupportedStreams = 0x5020,
  PTPPropertyCodeEnabledStreams = 0x5021,
  PTPPropertyCodeVideoFormat = 0x5022,
  PTPPropertyCodeVideoResolution = 0x5023,
  PTPPropertyCodeVideoQuality = 0x5024,
  PTPPropertyCodeVideoFrameRate = 0x5025,
  PTPPropertyCodeVideoContrast = 0x5026,
  PTPPropertyCodeVideoBrightness = 0x5027,
  PTPPropertyCodeAudioFormat = 0x5028,
  PTPPropertyCodeAudioBitrate = 0x5029,
  PTPPropertyCodeAudioSamplingRate = 0x502A,
  PTPPropertyCodeAudioBitPerSample = 0x502B,
  PTPPropertyCodeAudioVolume = 0x502C,
  PTPPropertyCodeMTPSynchronizationPartner = 0xD401,
  PTPPropertyCodeMTPDeviceFriendlyName = 0xD402,
  PTPPropertyCodeMTPVolumeLevel = 0xD403,
  PTPPropertyCodeMTPDeviceIcon = 0xD405,
  PTPPropertyCodeMTPSessionInitiatorInfo = 0xD406,
  PTPPropertyCodeMTPPerceivedDeviceType = 0xD407,
  PTPPropertyCodeMTPPlaybackRate = 0xD410,
  PTPPropertyCodeMTPPlaybackObject = 0xD411,
  PTPPropertyCodeMTPPlaybackContainerIndex = 0xD412,
  PTPPropertyCodeMTPPlaybackPosition = 0xD413,
};
typedef unsigned short PTPPropertyCode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPVendorExtension {
  PTPVendorExtensionEastmanKodak = 0x00000001,
  PTPVendorExtensionMicrosoft = 0x00000006,
  PTPVendorExtensionNikon = 0x0000000A,
  PTPVendorExtensionCanon = 0x0000000B,
  PTPVendorExtensionPentax = 0x0000000D,
  PTPVendorExtensionFuji = 0x0000000E,
  PTPVendorExtensionSony = 0x00000011,
  PTPVendorExtensionSamsung = 0x0000001A
};

typedef unsigned short PTPVendorExtension;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPDataTypeCode {
  PTPDataTypeCodeUndefined = 0x0000,
  PTPDataTypeCodeSInt8 = 0x0001,
  PTPDataTypeCodeUInt8 = 0x0002,
  PTPDataTypeCodeSInt16 = 0x0003,
  PTPDataTypeCodeUInt16 = 0x0004,
  PTPDataTypeCodeSInt32 = 0x0005,
  PTPDataTypeCodeUInt32 = 0x0006,
  PTPDataTypeCodeSInt64 = 0x0007,
  PTPDataTypeCodeUInt64 = 0x0008,
  PTPDataTypeCodeSInt128 = 0x0009,
  PTPDataTypeCodeUInt128 = 0x000A,
  PTPDataTypeCodeArrayOfSInt8 = 0x4001,
  PTPDataTypeCodeArrayOfUInt8 = 0x4002,
  PTPDataTypeCodeArrayOfSInt16 = 0x4003,
  PTPDataTypeCodeArrayOfUInt16 = 0x4004,
  PTPDataTypeCodeArrayOfSInt32 = 0x4005,
  PTPDataTypeCodeArrayOfUInt32 = 0x4006,
  PTPDataTypeCodeArrayOfSInt64 = 0x4007,
  PTPDataTypeCodeArrayOfUInt64 = 0x4008,
  PTPDataTypeCodeArrayOfSInt128 = 0x4009,
  PTPDataTypeCodeArrayOfUInt128 = 0x400A,
  PTPDataTypeCodeUnicodeString = 0xFFFF
};
typedef unsigned short PTPDataTypeCode;

//------------------------------------------------------------------------------------------------------------------------------

extern char ptpReadChar(unsigned char** buf);
extern void ptpWriteChar(unsigned char** buf, char value);
extern unsigned char ptpReadUnsignedChar(unsigned char** buf);
extern void ptpWriteUnsignedChar(unsigned char** buf, unsigned char value);
extern short ptpReadShort(unsigned char** buf);
extern void ptpWriteShort(unsigned char** buf, short value);
extern unsigned short ptpReadUnsignedShort(unsigned char** buf);
extern void ptpWriteUnsignedShort(unsigned char** buf, unsigned short value);
extern int ptpReadInt(unsigned char** buf);
extern void ptpWriteInt(unsigned char** buf, int value);
extern unsigned int ptpReadUnsignedInt(unsigned char** buf);
extern void ptpWriteUnsignedInt(unsigned char** buf, unsigned int value);
extern long ptpReadLong(unsigned char** buf);
extern void ptpWriteLong(unsigned char** buf, long value);
extern unsigned long ptpReadUnsignedLong(unsigned char** buf);
extern void ptpWriteUnsignedLong(unsigned char** buf, unsigned long value);
extern NSString *ptpRead128(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadCharArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadUnsignedCharArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadShortArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadUnsignedShortArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadIntArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadUnsignedIntArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadLongArray(unsigned char** buf);
extern NSArray<NSNumber *> *ptpReadUnsignedLongArray(unsigned char** buf);
extern NSString *ptpReadString(unsigned char** buf);
extern unsigned int  ptpWriteString(unsigned char **buf, NSString *value);
extern NSObject *ptpReadValue(PTPDataTypeCode type, unsigned char **buf);

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPVendor : NSObject
@property PTPVendorExtension vendorExtension;
+(NSString *)vendorExtensionName:(PTPVendorExtension)vendorExtension;
-(id)initWithVendorExtension:(PTPVendorExtension)vendorExtension;
@end


//------------------------------------------------------------------------------------------------------------------------------

@interface PTPRequest : NSObject
@property PTPRequestCode operationCode;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
@property unsigned int parameter4;
@property unsigned int parameter5;
+(NSString *)operationCodeName:(PTPRequestCode)operationCode;
-(NSData*)commandBuffer;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPResponse : NSObject
@property PTPResponseCode responseCode;
@property unsigned int transactionID;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
@property unsigned int parameter4;
@property unsigned int parameter5;
+(NSString *)responseCodeName:(PTPResponseCode)responseCode;
-(id)initWithData:(NSData*)data;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPEvent : NSObject
@property PTPEventCode eventCode;
@property unsigned int transactionID;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
+(NSString *)eventCodeName:(PTPEventCode)eventCode;
-(id)initWithData:(NSData*)data;
-(id)initWithCode:(PTPEventCode)eventCode parameter1:(unsigned int)parameter1;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPProperty : NSObject
@property PTPPropertyCode propertyCode;
@property PTPDataTypeCode type;
@property BOOL readOnly;
@property NSObject* defaultValue;
@property NSObject* value;
@property NSNumber* min;
@property NSNumber* max;
@property NSNumber* step;
@property NSArray<NSObject*> *supportedValues;
+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode;
+(NSString *)typeName:(PTPDataTypeCode)type;
-(id)initWithCode:(PTPPropertyCode)propertyCode;
-(id)initWithData:(NSData*)data;
@end

//--------------------------------------------------------------------------------------------------------------------- PTPDeviceInfo

@interface PTPDeviceInfo : PTPVendor
@property unsigned short standardVersion;
@property unsigned short vendorExtensionVersion;
@property NSString *vendorExtensionDesc;
@property unsigned short functionalMode;
@property NSArray<NSNumber *> *operationsSupported;
@property NSArray<NSNumber *> *eventsSupported;
@property NSArray<NSNumber *> *propertiesSupported;
@property NSString *manufacturer;
@property NSString *model;
@property NSString *version;
@property NSString *serial;
@property NSMutableDictionary<NSNumber *, PTPProperty *> *properties;
-(id)initWithData:(NSData*)data;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface NSObject(PTPExtensions)

-(int)intValue;

@end

@protocol PTPDelegateProtocol;

@interface PTPCamera: NSObject<ICCameraDeviceDelegate, ICCameraDeviceDownloadDelegate>

@property (readonly) ICCameraDevice *icCamera;
@property (readonly) NSObject<PTPDelegateProtocol> *delegate;
@property (readonly) NSString *name;
@property (readonly) PTPVendorExtension extension;

@property (readonly) Class requestClass;
@property (readonly) Class responseClass;
@property (readonly) Class eventClass;
@property (readonly) Class propertyClass;
@property (readonly) Class deviceInfoClass;

@property BOOL avoidAF;
@property BOOL useMirrorLockup;
@property BOOL deleteDownloadedImage;
@property BOOL zoomPreview;

@property PTPDeviceInfo *info;
@property NSObject *userData;
@property int width;
@property int height;
@property float pixelSize;

@property int imagesPerShot;
@property int remainingCount;

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate;

-(BOOL)operationIsSupported:(PTPRequestCode)code;
-(BOOL)propertyIsSupported:(PTPPropertyCode)code;
-(void)getPreviewImage;
-(void)checkForEvent;
-(void)processEvent:(PTPEvent *)event;
-(void)processPropertyDescription:(PTPProperty *)property;
-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data;
-(void)processConnect;
-(void)mapValueList:(PTPProperty *)property map:(NSDictionary *)map;
-(void)mapValueInterval:(PTPProperty *)property map:(NSDictionary *)map;

-(void)requestOpenSession;
-(void)requestCloseSession;
-(void)requestEnableTethering;

-(void)sendPTPRequest:(PTPRequestCode)code;
-(void)sendPTPRequest:(PTPRequestCode)code data:(NSData *)data;
-(void)sendPTPRequest:(PTPRequestCode)code param1:(unsigned int)parameter1;
-(void)sendPTPRequest:(PTPRequestCode)code param1:(unsigned int)parameter1 data:(NSData *)data;
-(void)sendPTPRequest:(PTPRequestCode)code param1:(unsigned int)parameter1 param2:(unsigned int)parameter2;
-(void)sendPTPRequest:(PTPRequestCode)code param1:(unsigned int)parameter1 param2:(unsigned int)parameter2  param3:(unsigned int)parameter3;
-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value;
-(void)lock;
-(void)unlock;
-(void)startPreview;
-(void)stopPreview;
-(double)startExposure;
-(void)stopExposure;
-(void)startAutofocus;
-(void)stopAutofocus;
-(void)focus:(int)steps;

@end

//------------------------------------------------------------------------------------------------------------------------------

@protocol PTPDelegateProtocol <NSObject>
@optional
-(void)cameraAdded:(PTPCamera *)camera;
-(void)cameraConnected:(PTPCamera *)camera;
-(void)cameraExposureDone:(PTPCamera *)camera data:(NSData *)data filename:(NSString *)filename;
-(void)cameraExposureFailed:(PTPCamera *)camera message:(NSString *)message;
-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSString *)value values:(NSArray<NSString *> *)values labels:(NSArray<NSString *> *)labels readOnly:(BOOL)readOnly;
-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSNumber *)value min:(NSNumber *)min max:(NSNumber *)max step:(NSNumber *)step readOnly:(BOOL)readOnly;
-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSString *)value readOnly:(BOOL)readOnly;
-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code readOnly:(BOOL)readOnly;
-(void)cameraDisconnected:(PTPCamera *)camera;
-(void)cameraRemoved:(PTPCamera *)camera;
-(void)cameraCanExposure:(PTPCamera *)camera;
-(void)cameraCanFocus:(PTPCamera *)camera;
-(void)cameraCanPreview:(PTPCamera *)camera;
-(void)cameraFocusDone:(PTPCamera *)camera;
-(void)cameraFocusFailed:(PTPCamera *)camera message:(NSString *)message;
-(void)cameraFrame:(PTPCamera *)camera left:(int)left top:(int)top width:(int)width height:(int)height;
-(void)log:(NSString *)message;
-(void)debug:(NSString *)message;
@end

@interface PTPBrowser : NSObject <ICDeviceBrowserDelegate>

@property (readonly) NSObject<PTPDelegateProtocol> *delegate;

-(id)initWithDelegate:(NSObject<PTPDelegateProtocol> *)delegate;
-(void)start;
-(void)stop;
@end

//------------------------------------------------------------------------------------------------------------------------------

