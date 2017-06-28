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

enum PTPOperationCode {
  PTPOperationCodeUndefined = 0x1000,
  PTPOperationCodeGetDeviceInfo = 0x1001,
  PTPOperationCodeOpenSession = 0x1002,
  PTPOperationCodeCloseSession = 0x1003,
  PTPOperationCodeGetStorageIDs = 0x1004,
  PTPOperationCodeGetStorageInfo = 0x1005,
  PTPOperationCodeGetNumObjects = 0x1006,
  PTPOperationCodeGetObjectHandles = 0x1007,
  PTPOperationCodeGetObjectInfo = 0x1008,
  PTPOperationCodeGetObject = 0x1009,
  PTPOperationCodeGetThumb = 0x100A,
  PTPOperationCodeDeleteObject = 0x100B,
  PTPOperationCodeSendObjectInfo = 0x100C,
  PTPOperationCodeSendObject = 0x100D,
  PTPOperationCodeInitiateCapture = 0x100E,
  PTPOperationCodeFormatStore = 0x100F,
  PTPOperationCodeResetDevice = 0x1010,
  PTPOperationCodeSelfTest = 0x1011,
  PTPOperationCodeSetObjectProtection = 0x1012,
  PTPOperationCodePowerDown = 0x1013,
  PTPOperationCodeGetDevicePropDesc = 0x1014,
  PTPOperationCodeGetDevicePropValue = 0x1015,
  PTPOperationCodeSetDevicePropValue = 0x1016,
  PTPOperationCodeResetDevicePropValue = 0x1017,
  PTPOperationCodeTerminateOpenCapture = 0x1018,
  PTPOperationCodeMoveObject = 0x1019,
  PTPOperationCodeCopyObject = 0x101A,
  PTPOperationCodeGetPartialObject = 0x101B,
  PTPOperationCodeInitiateOpenCapture = 0x101C,
  PTPOperationCodeGetNumDownloadableObjects = 0x9001,
  PTPOperationCodeGetAllObjectInfo = 0x9002,
  PTPOperationCodeGetUserAssignedDeviceName = 0x9003,

  // MTP codes
  
  PTPOperationCodeMTPGetObjectPropsSupported = 0x9801,
  PTPOperationCodeMTPGetObjectPropDesc = 0x9802,
  PTPOperationCodeMTPGetObjectPropValue = 0x9803,
  PTPOperationCodeMTPSetObjectPropValue = 0x9804,
  PTPOperationCodeMTPGetObjPropList = 0x9805,
  PTPOperationCodeMTPSetObjPropList = 0x9806,
  PTPOperationCodeMTPGetInterdependendPropdesc = 0x9807,
  PTPOperationCodeMTPSendObjectPropList = 0x9808,
  PTPOperationCodeMTPGetObjectReferences = 0x9810,
  PTPOperationCodeMTPSetObjectReferences = 0x9811,
  PTPOperationCodeMTPUpdateDeviceFirmware = 0x9812,
  PTPOperationCodeMTPSkip = 0x9820,

  // Nikon codes
  
  PTPOperationCodeNikonGetProfileAllData = 0x9006,
  PTPOperationCodeNikonSendProfileData = 0x9007,
  PTPOperationCodeNikonDeleteProfile = 0x9008,
  PTPOperationCodeNikonSetProfileData = 0x9009,
  PTPOperationCodeNikonAdvancedTransfer = 0x9010,
  PTPOperationCodeNikonGetFileInfoInBlock = 0x9011,
  PTPOperationCodeNikonCapture = 0x90C0,
  PTPOperationCodeNikonAfDrive = 0x90C1,
  PTPOperationCodeNikonSetControlMode = 0x90C2,
  PTPOperationCodeNikonDelImageSDRAM = 0x90C3,
  PTPOperationCodeNikonGetLargeThumb = 0x90C4,
  PTPOperationCodeNikonCurveDownload = 0x90C5,
  PTPOperationCodeNikonCurveUpload = 0x90C6,
  PTPOperationCodeNikonCheckEvent = 0x90C7,
  PTPOperationCodeNikonDeviceReady = 0x90C8,
  PTPOperationCodeNikonSetPreWBData = 0x90C9,
  PTPOperationCodeNikonGetVendorPropCodes = 0x90CA,
  PTPOperationCodeNikonAfCaptureSDRAM = 0x90CB,
  PTPOperationCodeNikonGetPictCtrlData = 0x90CC,
  PTPOperationCodeNikonSetPictCtrlData = 0x90CD,
  PTPOperationCodeNikonDelCstPicCtrl = 0x90CE,
  PTPOperationCodeNikonGetPicCtrlCapability = 0x90CF,
  PTPOperationCodeNikonGetPreviewImg = 0x9200,
  PTPOperationCodeNikonStartLiveView = 0x9201,
  PTPOperationCodeNikonEndLiveView = 0x9202,
  PTPOperationCodeNikonGetLiveViewImg = 0x9203,
  PTPOperationCodeNikonMfDrive = 0x9204,
  PTPOperationCodeNikonChangeAfArea = 0x9205,
  PTPOperationCodeNikonAfDriveCancel = 0x9206,
  PTPOperationCodeNikonInitiateCaptureRecInMedia = 0x9207,
  PTPOperationCodeNikonGetVendorStorageIDs = 0x9209,
  PTPOperationCodeNikonStartMovieRecInCard = 0x920a,
  PTPOperationCodeNikonEndMovieRec = 0x920b,
  PTPOperationCodeNikonTerminateCapture = 0x920c,
  PTPOperationCodeNikonGetDevicePTPIPInfo = 0x90E0,
  PTPOperationCodeNikonGetPartialObjectHiSpeed = 0x9400
};
typedef unsigned short PTPOperationCode;

#define RESERVED_OPERATION_CODE(c) (!(c & 0x9000) && (c > PTPOperationCodeInitiateOpenCapture))
#define VENDOR_SPECIFIC_OPERATION_CODE(c) (c & 0x9000)

//------------------------------------------------------------------------------------------------------------------------------

enum PTPResponseCode {
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
  
  // MTP codes
  
  PTPResponseCodeMTPUndefined = 0xA800,
  PTPResponseCodeMTPInvalid_ObjectPropCode = 0xA801,
  PTPResponseCodeMTPInvalid_ObjectProp_Format = 0xA802,
  PTPResponseCodeMTPInvalid_ObjectProp_Value = 0xA803,
  PTPResponseCodeMTPInvalid_ObjectReference = 0xA804,
  PTPResponseCodeMTPInvalid_Dataset = 0xA806,
  PTPResponseCodeMTPSpecification_By_Group_Unsupported = 0xA807,
  PTPResponseCodeMTPSpecification_By_Depth_Unsupported = 0xA808,
  PTPResponseCodeMTPObject_Too_Large = 0xA809,
  PTPResponseCodeMTPObjectProp_Not_Supported = 0xA80A,
  
  // Nikon codes
  
  PTPResponseCodeNikonHardwareError = 0xA001,
  PTPResponseCodeNikonOutOfFocus = 0xA002,
  PTPResponseCodeNikonChangeCameraModeFailed = 0xA003,
  PTPResponseCodeNikonInvalidStatus = 0xA004,
  PTPResponseCodeNikonSetPropertyNotSupported = 0xA005,
  PTPResponseCodeNikonWbResetError = 0xA006,
  PTPResponseCodeNikonDustReferenceError = 0xA007,
  PTPResponseCodeNikonShutterSpeedBulb = 0xA008,
  PTPResponseCodeNikonMirrorUpSequence = 0xA009,
  PTPResponseCodeNikonCameraModeNotAdjustFNumber = 0xA00A,
  PTPResponseCodeNikonNotLiveView = 0xA00B,
  PTPResponseCodeNikonMfDriveStepEnd = 0xA00C,
  PTPResponseCodeNikonMfDriveStepInsufficiency = 0xA00E,
  PTPResponseCodeNikonAdvancedTransferCancel = 0xA022
};
typedef unsigned short PTPResponseCode;

#define RESERVED_RESPONSE_CODE(c) (!(c & 0xA000) && (c > PTPResponseCodeSpecificationOfDestinationUnsupported))
#define VENDOR_SPECIFIC_RESPONSE_CODE(c) (c & 0xA000)

//------------------------------------------------------------------------------------------------------------------------------

enum PTPEventCode {
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

  // Nikon codes
  
  PTPEventCodeNikonObjectAddedInSDRAM = 0xC101,
  PTPEventCodeNikonCaptureCompleteRecInSdram = 0xC102,
  PTPEventCodeNikonAdvancedTransfer = 0xC103,
  PTPEventCodeNikonPreviewImageAdded = 0xC104
};

typedef unsigned short PTPEventCode;

#define RESERVED_EVENT_CODE(c) (!(c & 0xC000) && (c > PTPEventCodeUnreportedStatus))
#define VENDOR_SPECIFIC_EVENT_CODE(c) (c & 0xC000)

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
  /* PTP v1.1 property codes */
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

  // MTP codes
  
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
  
  // Nikon codes
  
  PTPPropertyCodeNikonShootingBank = 0xD010,
  PTPPropertyCodeNikonShootingBankNameA = 0xD011,
  PTPPropertyCodeNikonShootingBankNameB = 0xD012,
  PTPPropertyCodeNikonShootingBankNameC = 0xD013,
  PTPPropertyCodeNikonShootingBankNameD = 0xD014,
  PTPPropertyCodeNikonResetBank0 = 0xD015,
  PTPPropertyCodeNikonRawCompression = 0xD016,
  PTPPropertyCodeNikonWhiteBalanceAutoBias = 0xD017,
  PTPPropertyCodeNikonWhiteBalanceTungstenBias = 0xD018,
  PTPPropertyCodeNikonWhiteBalanceFluorescentBias = 0xD019,
  PTPPropertyCodeNikonWhiteBalanceDaylightBias = 0xD01A,
  PTPPropertyCodeNikonWhiteBalanceFlashBias = 0xD01B,
  PTPPropertyCodeNikonWhiteBalanceCloudyBias = 0xD01C,
  PTPPropertyCodeNikonWhiteBalanceShadeBias = 0xD01D,
  PTPPropertyCodeNikonWhiteBalanceColorTemperature = 0xD01E,
  PTPPropertyCodeNikonWhiteBalancePresetNo = 0xD01F,
  PTPPropertyCodeNikonWhiteBalancePresetName0 = 0xD020,
  PTPPropertyCodeNikonWhiteBalancePresetName1 = 0xD021,
  PTPPropertyCodeNikonWhiteBalancePresetName2 = 0xD022,
  PTPPropertyCodeNikonWhiteBalancePresetName3 = 0xD023,
  PTPPropertyCodeNikonWhiteBalancePresetName4 = 0xD024,
  PTPPropertyCodeNikonWhiteBalancePresetVal0 = 0xD025,
  PTPPropertyCodeNikonWhiteBalancePresetVal1 = 0xD026,
  PTPPropertyCodeNikonWhiteBalancePresetVal2 = 0xD027,
  PTPPropertyCodeNikonWhiteBalancePresetVal3 = 0xD028,
  PTPPropertyCodeNikonWhiteBalancePresetVal4 = 0xD029,
  PTPPropertyCodeNikonImageSharpening = 0xD02A,
  PTPPropertyCodeNikonToneCompensation = 0xD02B,
  PTPPropertyCodeNikonColorModel = 0xD02C,
  PTPPropertyCodeNikonHueAdjustment = 0xD02D,
  PTPPropertyCodeNikonNonCPULensDataFocalLength = 0xD02E,
  PTPPropertyCodeNikonNonCPULensDataMaximumAperture = 0xD02F,
  PTPPropertyCodeNikonShootingMode = 0xD030,
  PTPPropertyCodeNikonJPEGCompressionPolicy = 0xD031,
  PTPPropertyCodeNikonColorSpace = 0xD032,
  PTPPropertyCodeNikonAutoDXCrop = 0xD033,
  PTPPropertyCodeNikonFlickerReduction = 0xD034,
  PTPPropertyCodeNikonRemoteMode = 0xD035,
  PTPPropertyCodeNikonVideoMode = 0xD036,
  PTPPropertyCodeNikonEffectMode = 0xD037,
  PTPPropertyCodeNikon1Mode = 0xD038,
  PTPPropertyCodeNikonCSMMenuBankSelect = 0xD040,
  PTPPropertyCodeNikonMenuBankNameA = 0xD041,
  PTPPropertyCodeNikonMenuBankNameB = 0xD042,
  PTPPropertyCodeNikonMenuBankNameC = 0xD043,
  PTPPropertyCodeNikonMenuBankNameD = 0xD044,
  PTPPropertyCodeNikonResetBank = 0xD045,
  PTPPropertyCodeNikonA1AFCModePriority = 0xD048,
  PTPPropertyCodeNikonA2AFSModePriority = 0xD049,
  PTPPropertyCodeNikonA3GroupDynamicAF = 0xD04A,
  PTPPropertyCodeNikonA4AFActivation = 0xD04B,
  PTPPropertyCodeNikonFocusAreaIllumManualFocus = 0xD04C,
  PTPPropertyCodeNikonFocusAreaIllumContinuous = 0xD04D,
  PTPPropertyCodeNikonFocusAreaIllumWhenSelected = 0xD04E,
  PTPPropertyCodeNikonFocusAreaWrap = 0xD04F,
  PTPPropertyCodeNikonVerticalAFON = 0xD050,
  PTPPropertyCodeNikonAFLockOn = 0xD051,
  PTPPropertyCodeNikonFocusAreaZone = 0xD052,
  PTPPropertyCodeNikonEnableCopyright = 0xD053,
  PTPPropertyCodeNikonISOAutoTime = 0xD054,
  PTPPropertyCodeNikonEVISOStep = 0xD055,
  PTPPropertyCodeNikonEVStep = 0xD056,
  PTPPropertyCodeNikonEVStepExposureComp = 0xD057,
  PTPPropertyCodeNikonExposureCompensation = 0xD058,
  PTPPropertyCodeNikonCenterWeightArea = 0xD059,
  PTPPropertyCodeNikonExposureBaseMatrix = 0xD05A,
  PTPPropertyCodeNikonExposureBaseCenter = 0xD05B,
  PTPPropertyCodeNikonExposureBaseSpot = 0xD05C,
  PTPPropertyCodeNikonLiveViewAFArea = 0xD05D,
  PTPPropertyCodeNikonAELockMode = 0xD05E,
  PTPPropertyCodeNikonAELAFLMode = 0xD05F,
  PTPPropertyCodeNikonLiveViewAFFocus = 0xD061,
  PTPPropertyCodeNikonMeterOff = 0xD062,
  PTPPropertyCodeNikonSelfTimer = 0xD063,
  PTPPropertyCodeNikonMonitorOff = 0xD064,
  PTPPropertyCodeNikonImgConfTime = 0xD065,
  PTPPropertyCodeNikonAutoOffTimers = 0xD066,
  PTPPropertyCodeNikonAngleLevel = 0xD067,
  PTPPropertyCodeNikonD1ShootingSpeed = 0xD068,
  PTPPropertyCodeNikonD2MaximumShots = 0xD069,
  PTPPropertyCodeNikonExposureDelayMode = 0xD06A,
  PTPPropertyCodeNikonLongExposureNoiseReduction = 0xD06B,
  PTPPropertyCodeNikonFileNumberSequence = 0xD06C,
  PTPPropertyCodeNikonControlPanelFinderRearControl = 0xD06D,
  PTPPropertyCodeNikonControlPanelFinderViewfinder = 0xD06E,
  PTPPropertyCodeNikonD7Illumination = 0xD06F,
  PTPPropertyCodeNikonNrHighISO = 0xD070,
  PTPPropertyCodeNikonSHSETCHGUIDDISP = 0xD071,
  PTPPropertyCodeNikonArtistName = 0xD072,
  PTPPropertyCodeNikonCopyrightInfo = 0xD073,
  PTPPropertyCodeNikonFlashSyncSpeed = 0xD074,
  PTPPropertyCodeNikonFlashShutterSpeed = 0xD075,
  PTPPropertyCodeNikonE3AAFlashMode = 0xD076,
  PTPPropertyCodeNikonE4ModelingFlash = 0xD077,
  PTPPropertyCodeNikonBracketSet = 0xD078,
  PTPPropertyCodeNikonE6ManualModeBracketing = 0xD079,
  PTPPropertyCodeNikonBracketOrder = 0xD07A,
  PTPPropertyCodeNikonE8AutoBracketSelection = 0xD07B,
  PTPPropertyCodeNikonBracketingSet = 0xD07C,
  PTPPropertyCodeNikonF1CenterButtonShootingMode = 0xD080,
  PTPPropertyCodeNikonCenterButtonPlaybackMode = 0xD081,
  PTPPropertyCodeNikonF2Multiselector = 0xD082,
  PTPPropertyCodeNikonF3PhotoInfoPlayback = 0xD083,
  PTPPropertyCodeNikonF4AssignFuncButton = 0xD084,
  PTPPropertyCodeNikonF5CustomizeCommDials = 0xD085,
  PTPPropertyCodeNikonReverseCommandDial = 0xD086,
  PTPPropertyCodeNikonApertureSetting = 0xD087,
  PTPPropertyCodeNikonMenusAndPlayback = 0xD088,
  PTPPropertyCodeNikonF6ButtonsAndDials = 0xD089,
  PTPPropertyCodeNikonNoCFCard = 0xD08A,
  PTPPropertyCodeNikonCenterButtonZoomRatio = 0xD08B,
  PTPPropertyCodeNikonFunctionButton2 = 0xD08C,
  PTPPropertyCodeNikonAFAreaPoint = 0xD08D,
  PTPPropertyCodeNikonNormalAFOn = 0xD08E,
  PTPPropertyCodeNikonCleanImageSensor = 0xD08F,
  PTPPropertyCodeNikonImageCommentString = 0xD090,
  PTPPropertyCodeNikonImageCommentEnable = 0xD091,
  PTPPropertyCodeNikonImageRotation = 0xD092,
  PTPPropertyCodeNikonManualSetLensNo = 0xD093,
  PTPPropertyCodeNikonMovScreenSize = 0xD0A0,
  PTPPropertyCodeNikonMovVoice = 0xD0A1,
  PTPPropertyCodeNikonMovMicrophone = 0xD0A2,
  PTPPropertyCodeNikonMovFileSlot = 0xD0A3,
  PTPPropertyCodeNikonMovRecProhibitCondition = 0xD0A4,
  PTPPropertyCodeNikonManualMovieSetting = 0xD0A6,
  PTPPropertyCodeNikonMovQuality = 0xD0A7,
  PTPPropertyCodeNikonLiveViewScreenDisplaySetting = 0xD0B2,
  PTPPropertyCodeNikonMonitorOffDelay = 0xD0B3,
  PTPPropertyCodeNikonBracketing = 0xD0C0,
  PTPPropertyCodeNikonAutoExposureBracketStep = 0xD0C1,
  PTPPropertyCodeNikonAutoExposureBracketProgram = 0xD0C2,
  PTPPropertyCodeNikonAutoExposureBracketCount = 0xD0C3,
  PTPPropertyCodeNikonWhiteBalanceBracketStep = 0xD0C4,
  PTPPropertyCodeNikonWhiteBalanceBracketProgram = 0xD0C5,
  PTPPropertyCodeNikonLensID = 0xD0E0,
  PTPPropertyCodeNikonLensSort = 0xD0E1,
  PTPPropertyCodeNikonLensType = 0xD0E2,
  PTPPropertyCodeNikonFocalLengthMin = 0xD0E3,
  PTPPropertyCodeNikonFocalLengthMax = 0xD0E4,
  PTPPropertyCodeNikonMaxApAtMinFocalLength = 0xD0E5,
  PTPPropertyCodeNikonMaxApAtMaxFocalLength = 0xD0E6,
  PTPPropertyCodeNikonFinderISODisp = 0xD0F0,
  PTPPropertyCodeNikonAutoOffPhoto = 0xD0F2,
  PTPPropertyCodeNikonAutoOffMenu = 0xD0F3,
  PTPPropertyCodeNikonAutoOffInfo = 0xD0F4,
  PTPPropertyCodeNikonSelfTimerShootNum = 0xD0F5,
  PTPPropertyCodeNikonVignetteCtrl = 0xD0F7,
  PTPPropertyCodeNikonAutoDistortionControl = 0xD0F8,
  PTPPropertyCodeNikonSceneMode = 0xD0F9,
  PTPPropertyCodeNikonSceneMode2 = 0xD0FD,
  PTPPropertyCodeNikonSelfTimerInterval = 0xD0FE,
  PTPPropertyCodeNikonExposureTime = 0xD100,
  PTPPropertyCodeNikonACPower = 0xD101,
  PTPPropertyCodeNikonWarningStatus = 0xD102,
  PTPPropertyCodeNikonMaximumShots = 0xD103,
  PTPPropertyCodeNikonAFLockStatus = 0xD104,
  PTPPropertyCodeNikonAELockStatus = 0xD105,
  PTPPropertyCodeNikonFVLockStatus = 0xD106,
  PTPPropertyCodeNikonAutofocusLCDTopMode2 = 0xD107,
  PTPPropertyCodeNikonAutofocusArea = 0xD108,
  PTPPropertyCodeNikonFlexibleProgram = 0xD109,
  PTPPropertyCodeNikonLightMeter = 0xD10A,
  PTPPropertyCodeNikonRecordingMedia = 0xD10B,
  PTPPropertyCodeNikonUSBSpeed = 0xD10C,
  PTPPropertyCodeNikonCCDNumber = 0xD10D,
  PTPPropertyCodeNikonCameraOrientation = 0xD10E,
  PTPPropertyCodeNikonGroupPtnType = 0xD10F,
  PTPPropertyCodeNikonFNumberLock = 0xD110,
  PTPPropertyCodeNikonExposureApertureLock = 0xD111,
  PTPPropertyCodeNikonTVLockSetting = 0xD112,
  PTPPropertyCodeNikonAVLockSetting = 0xD113,
  PTPPropertyCodeNikonIllumSetting = 0xD114,
  PTPPropertyCodeNikonFocusPointBright = 0xD115,
  PTPPropertyCodeNikonExternalFlashAttached = 0xD120,
  PTPPropertyCodeNikonExternalFlashStatus = 0xD121,
  PTPPropertyCodeNikonExternalFlashSort = 0xD122,
  PTPPropertyCodeNikonExternalFlashMode = 0xD123,
  PTPPropertyCodeNikonExternalFlashCompensation = 0xD124,
  PTPPropertyCodeNikonNewExternalFlashMode = 0xD125,
  PTPPropertyCodeNikonFlashExposureCompensation = 0xD126,
  PTPPropertyCodeNikonHDRMode = 0xD130,
  PTPPropertyCodeNikonHDRHighDynamic = 0xD131,
  PTPPropertyCodeNikonHDRSmoothing = 0xD132,
  PTPPropertyCodeNikonOptimizeImage = 0xD140,
  PTPPropertyCodeNikonSaturation = 0xD142,
  PTPPropertyCodeNikonBWFillerEffect = 0xD143,
  PTPPropertyCodeNikonBWSharpness = 0xD144,
  PTPPropertyCodeNikonBWContrast = 0xD145,
  PTPPropertyCodeNikonBWSettingType = 0xD146,
  PTPPropertyCodeNikonSlot2SaveMode = 0xD148,
  PTPPropertyCodeNikonRawBitMode = 0xD149,
  PTPPropertyCodeNikonActiveDLighting = 0xD14E,
  PTPPropertyCodeNikonFlourescentType = 0xD14F,
  PTPPropertyCodeNikonTuneColourTemperature = 0xD150,
  PTPPropertyCodeNikonTunePreset0 = 0xD151,
  PTPPropertyCodeNikonTunePreset1 = 0xD152,
  PTPPropertyCodeNikonTunePreset2 = 0xD153,
  PTPPropertyCodeNikonTunePreset3 = 0xD154,
  PTPPropertyCodeNikonTunePreset4 = 0xD155,
  PTPPropertyCodeNikonBeepOff = 0xD160,
  PTPPropertyCodeNikonAutofocusMode = 0xD161,
  PTPPropertyCodeNikonAFAssist = 0xD163,
  PTPPropertyCodeNikonPADVPMode = 0xD164,
  PTPPropertyCodeNikonImageReview = 0xD165,
  PTPPropertyCodeNikonAFAreaIllumination = 0xD166,
  PTPPropertyCodeNikonFlashMode = 0xD167,
  PTPPropertyCodeNikonFlashCommanderMode = 0xD168,
  PTPPropertyCodeNikonFlashSign = 0xD169,
  PTPPropertyCodeNikonISOAuto = 0xD16A,
  PTPPropertyCodeNikonRemoteTimeout = 0xD16B,
  PTPPropertyCodeNikonGridDisplay = 0xD16C,
  PTPPropertyCodeNikonFlashModeManualPower = 0xD16D,
  PTPPropertyCodeNikonFlashModeCommanderPower = 0xD16E,
  PTPPropertyCodeNikonAutoFP = 0xD16F,
  PTPPropertyCodeNikonDateImprintSetting = 0xD170,
  PTPPropertyCodeNikonDateCounterSelect = 0xD171,
  PTPPropertyCodeNikonDateCountData = 0xD172,
  PTPPropertyCodeNikonDateCountDisplaySetting = 0xD173,
  PTPPropertyCodeNikonRangeFinderSetting = 0xD174,
  PTPPropertyCodeNikonCSMMenu = 0xD180,
  PTPPropertyCodeNikonWarningDisplay = 0xD181,
  PTPPropertyCodeNikonBatteryCellKind = 0xD182,
  PTPPropertyCodeNikonISOAutoHiLimit = 0xD183,
  PTPPropertyCodeNikonDynamicAFArea = 0xD184,
  PTPPropertyCodeNikonContinuousSpeedHigh = 0xD186,
  PTPPropertyCodeNikonInfoDispSetting = 0xD187,
  PTPPropertyCodeNikonPreviewButton = 0xD189,
  PTPPropertyCodeNikonPreviewButton2 = 0xD18A,
  PTPPropertyCodeNikonAEAFLockButton2 = 0xD18B,
  PTPPropertyCodeNikonIndicatorDisp = 0xD18D,
  PTPPropertyCodeNikonCellKindPriority = 0xD18E,
  PTPPropertyCodeNikonBracketingFramesAndSteps = 0xD190,
  PTPPropertyCodeNikonLiveViewMode = 0xD1A0,
  PTPPropertyCodeNikonLiveViewDriveMode = 0xD1A1,
  PTPPropertyCodeNikonLiveViewStatus = 0xD1A2,
  PTPPropertyCodeNikonLiveViewImageZoomRatio = 0xD1A3,
  PTPPropertyCodeNikonLiveViewProhibitCondition = 0xD1A4,
  PTPPropertyCodeNikonMovieShutterSpeed = 0xD1A8,
  PTPPropertyCodeNikonMovieFNumber = 0xD1A9,
  PTPPropertyCodeNikonMovieISO = 0xD1AA,
  PTPPropertyCodeNikonLiveViewMovieMode = 0xD1AC,
  PTPPropertyCodeNikonExposureDisplayStatus = 0xD1B0,
  PTPPropertyCodeNikonExposureIndicateStatus = 0xD1B1,
  PTPPropertyCodeNikonInfoDispErrStatus = 0xD1B2,
  PTPPropertyCodeNikonExposureIndicateLightup = 0xD1B3,
  PTPPropertyCodeNikonFlashOpen = 0xD1C0,
  PTPPropertyCodeNikonFlashCharged = 0xD1C1,
  PTPPropertyCodeNikonFlashMRepeatValue = 0xD1D0,
  PTPPropertyCodeNikonFlashMRepeatCount = 0xD1D1,
  PTPPropertyCodeNikonFlashMRepeatInterval = 0xD1D2,
  PTPPropertyCodeNikonFlashCommandChannel = 0xD1D3,
  PTPPropertyCodeNikonFlashCommandSelfMode = 0xD1D4,
  PTPPropertyCodeNikonFlashCommandSelfCompensation = 0xD1D5,
  PTPPropertyCodeNikonFlashCommandSelfValue = 0xD1D6,
  PTPPropertyCodeNikonFlashCommandAMode = 0xD1D7,
  PTPPropertyCodeNikonFlashCommandACompensation = 0xD1D8,
  PTPPropertyCodeNikonFlashCommandAValue = 0xD1D9,
  PTPPropertyCodeNikonFlashCommandBMode = 0xD1DA,
  PTPPropertyCodeNikonFlashCommandBCompensation = 0xD1DB,
  PTPPropertyCodeNikonFlashCommandBValue = 0xD1DC,
  PTPPropertyCodeNikonApplicationMode = 0xD1F0,
  PTPPropertyCodeNikonActiveSlot = 0xD1F2,
  PTPPropertyCodeNikonActivePicCtrlItem = 0xD200,
  PTPPropertyCodeNikonChangePicCtrlItem = 0xD201,
  PTPPropertyCodeNikonMovieNrHighISO = 0xD236
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


enum PTPFocusMode {
  PTPFocusModeUndefined = 0x0000,
  PTPFocusModeManual = 0x0001,
  PTPFocusModeAutomatic = 0x0002,
  PTPFocusModeAutomaticMacro = 0x0003
};
typedef unsigned short PTPFocusMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPExposureMeteringMode {
  PTPExposureMeteringModeUndefined = 0x0000,
  PTPExposureMeteringModeAverage = 0x0001,
  PTPExposureMeteringModeCenterWeightedAverage = 0x0002,
  PTPExposureMeteringModeMultispot = 0x0003,
  PTPExposureMeteringModeCenterspot = 0x0004
};
typedef unsigned short PTPExposureMeteringMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPFlashMode {
  PTPFlashModeUndefined = 0x0000,
  PTPFlashModeAutoFlash = 0x0001,
  PTPFlashModeFlashOff = 0x0002,
  PTPFlashModeFillFlash = 0x0003,
  PTPFlashModeRedEyeAuto = 0x0004,
  PTPFlashModeRedEyeFill = 0x0005,
  PTPFlashModeExternalSync = 0x0006
};
typedef unsigned short PTPFlashMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPExposureProgramMode {
  PTPExposureProgramModeUndefined = 0x0000,
  PTPExposureProgramModeManual = 0x0001,
  PTPExposureProgramModeAutomatic = 0x0002,
  PTPExposureProgramModeAperturePriority = 0x0003,
  PTPExposureProgramModeShutterPriority = 0x0004,
  PTPExposureProgramModeCreative = 0x0005,
  PTPExposureProgramModeAction = 0x0006,
  PTPExposureProgramModePortrait = 0x0007
};
typedef unsigned short PTPExposureProgramMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPStillCaptureMode {
  PTPStillCaptureModeUndefined = 0x0000,
  PTPStillCaptureModeNormal = 0x0001,
  PTPStillCaptureModeBurst = 0x0002,
  PTPStillCaptureModeTimelapse = 0x0003
};
typedef unsigned short PTPStillCaptureMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPEffectMode {
  PTPEffectModeUndefined = 0x0000,
  PTPEffectModeStandardColor = 0x0001,
  PTPEffectModeBlackAndWhite = 0x0002,
  PTPEffectModeSepia = 0x0003
};
typedef unsigned short PTPEffectMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPFocusMeteringMode {
  PTPFocusMeteringModeUndefined = 0x0000,
  PTPFocusMeteringModeCenterSpot = 0x0001,
  PTPFocusMeteringModeMultiSpot = 0x0002
};
typedef unsigned short PTPFocusMeteringMode;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPFunctionalMode {
  PTPFunctionalModeStandard = 0x0000,
  PTPFunctionalModeSleepState = 0x0001
};
typedef unsigned short PTPFunctionalMode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPStorageType {
  PTPStorageTypeUndefined = 0x0000,
  PTPStorageTypeFixedROM = 0x0001,
  PTPStorageTypeRemovableROM = 0x0002,
  PTPStorageTypeFixedRAM = 0x0003,
  PTPStorageTypeRemovableRAM = 0x0004
};
typedef unsigned short PTPStorageType;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPFilesystemType {
  PTPFilesystemTypeUndefined = 0x0000,
  PTPFilesystemTypeGenericFlat = 0x0001,
  PTPFilesystemTypeGenericHierarchical = 0x0002,
  PTPFilesystemTypeDCF = 0x0003
};
typedef unsigned short PTPFilesystemType;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPAccessCapability {
  PTPAccessCapabilityReadWrite = 0x0000,
  PTPAccessCapabilityReadOnlyWithoutObjectDeletion = 0x0001,
  PTPAccessCapabilityReadOnlyWithObjectDeletion = 0x0002
};
typedef unsigned short PTPAccessCapability;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPProtectionStatus {
  PTPProtectionStatusNoProtection = 0x0000,
  PTPProtectionStatusReadOnly = 0x0001
};
typedef unsigned short PTPProtectionStatus;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPObjectFormatCode {
  PTPObjectFormatCodeUndefinedNonImageObject = 0x3000,
  PTPObjectFormatCodeAssociation = 0x3001, // e.g., folder
  PTPObjectFormatCodeScript = 0x3002, // device-model-specific script
  PTPObjectFormatCodeExecutable = 0x3003, // device-model-specific binary executable
  PTPObjectFormatCodeText = 0x3004,
  PTPObjectFormatCodeHTML = 0x3005,
  PTPObjectFormatCodeDPOF = 0x3006,
  PTPObjectFormatCodeAIFF = 0x3007,
  PTPObjectFormatCodeWAV = 0x3008,
  PTPObjectFormatCodeMP3 = 0x3009,
  PTPObjectFormatCodeAVI = 0x300A,
  PTPObjectFormatCodeMPEG = 0x300B,
  PTPObjectFormatCodeASF = 0x300C,
  PTPObjectFormatCodeUnknownImageObject = 0x3800,
  PTPObjectFormatCodeEXIF_JPEG = 0x3801,
  PTPObjectFormatCodeTIFF_EP = 0x3802,
  PTPObjectFormatCodeFlashPix = 0x3803,
  PTPObjectFormatCodeBMP = 0x3804,
  PTPObjectFormatCodeCIFF = 0x3805,
  PTPObjectFormatCodeReserved1 = 0x3806,
  PTPObjectFormatCodeGIF = 0x3807,
  PTPObjectFormatCodeJFIF = 0x3808,
  PTPObjectFormatCodePCD = 0x3809,
  PTPObjectFormatCodePICT = 0x380A,
  PTPObjectFormatCodePNG = 0x380B,
  PTPObjectFormatCodeReserved2 = 0x380C,
  PTPObjectFormatCodeTIFF = 0x380D,
  PTPObjectFormatCodeTIFF_IT = 0x380E,
  PTPObjectFormatCodeJP2 = 0x380F, // JPEG 2000 Baseline File Format
  PTPObjectFormatCodeJPX = 0x3810 // JPEG 2000 Extended File Format
};
typedef unsigned short PTPObjectFormatCode;

//------------------------------------------------------------------------------------------------------------------------------

enum PTPAssociationType {
  PTPAssociationTypeUndefined = 0x0000,
  PTPAssociationTypeGenericFolder = 0x0001,
  PTPAssociationTypeAlbum = 0x0002, // Reserved
  PTPAssociationTypeTimeSequence = 0x0003,
  PTPAssociationTypeHorizontalPanoramic = 0x0004,
  PTPAssociationTypeVerticalPanoramic = 0x0005,
  PTPAssociationType2DPanoramic = 0x0006,
  PTPAssociationTypeAncillaryData = 0x0007
};
typedef unsigned short PTPAssociationType;

// All values with Bit 15 set to 0 - Reserved
// All values with Bit 15 set to 1 - Vendor-defined

//------------------------------------------------------------------------------------------------------------------------------

enum PTPInitFailReason {
  PTPInitFailReasonRejectedInitiator = 0x00000001,
  PTPInitFailReasonBusy = 0x00000002,
  PTPInitFailReasonUnspecified = 0x00000003
};
typedef unsigned short PTPInitFailReason;

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

#define PTP_DEVICE_INFO			@"PTP_DEVICE_INFO"
#define PTP_EVENT_TIMER			@"PTP_EVENT_TIMER"
#define PTP_LIVE_VIEW_TIMER @"PTP_LIVE_VIEW_TIMER"

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPVendor : NSObject
@property PTPVendorExtension vendorExtension;
+ (NSString *)vendorExtensionName:(PTPVendorExtension)vendorExtension;
- (id)initWithVendorExtension:(PTPVendorExtension)vendorExtension;
@end


//------------------------------------------------------------------------------------------------------------------------------

@interface PTPOperationRequest : PTPVendor
@property PTPOperationCode operationCode;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
@property unsigned int parameter4;
@property unsigned int parameter5;
+ (NSString *)operationCodeName:(PTPOperationCode)operationCode vendorExtension:(PTPVendorExtension)vendorExtension;
- (id)initWithVendorExtension:(PTPVendorExtension)vendorExtension;
- (NSData*)commandBuffer;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPOperationResponse : PTPVendor
@property PTPResponseCode responseCode;
@property unsigned int transactionID;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
@property unsigned int parameter4;
@property unsigned int parameter5;
+ (NSString *)responseCodeName:(PTPResponseCode)responseCode vendorExtension:(PTPVendorExtension)vendorExtension;
- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPEvent : PTPVendor
@property PTPEventCode eventCode;
@property unsigned int transactionID;
@property unsigned short numberOfParameters;
@property unsigned int parameter1;
@property unsigned int parameter2;
@property unsigned int parameter3;
+ (NSString *)eventCodeName:(PTPEventCode)eventCode vendorExtension:(PTPVendorExtension)vendorExtension;
- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension;
- (id)initWithCode:(PTPEventCode)eventCode parameter1:(unsigned int)parameter1 vendorExtension:(PTPVendorExtension)vendorExtension;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface PTPProperty : PTPVendor
@property PTPPropertyCode propertyCode;
@property PTPDataTypeCode type;
@property BOOL readOnly;
@property NSObject* defaultValue;
@property NSObject* value;
@property NSNumber* min;
@property NSNumber* max;
@property NSNumber* step;
@property NSArray<NSObject*> *supportedValues;
+ (NSString *)propertyCodeName:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension;
+ (NSString *)typeName:(PTPDataTypeCode)type;
- (id)initWithCode:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension;
- (id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension;
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
- (id)initWithData:(NSData*)data;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface NSObject(PTPExtensions)
-(int)intValue;
@end

//------------------------------------------------------------------------------------------------------------------------------

@interface ICCameraDevice(PTPExtensions)
-(PTPDeviceInfo *)ptpDeviceInfo;
-(void)sendPTPRequest:(PTPOperationCode)operationCode;
-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1;
-(void)sendPTPRequest:(PTPOperationCode)operationCode param1:(unsigned int)parameter1 param2:(unsigned int)parameter2;
-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value;
-(void)startLiveView;
-(void)stopLiveView;
-(void)startCapture;
-(void)stopCapture;
-(void)mfDrive:(int)steps;
@end

@interface ICCameraDevice(PTPDebug)
- (void)dumpData:(void*)data length:(int)length comment:(NSString*)comment;
@end

//------------------------------------------------------------------------------------------------------------------------------

@protocol PTPDelegateProtocol <NSObject>
@optional
- (void)cameraAdded:(ICCameraDevice *)camera;
- (void)cameraConnected:(ICCameraDevice *)camera;
- (void)cameraExposureDone:(ICCameraDevice *)camera data:(NSData *)data filename:(NSString *)filename;
- (void)cameraExposureFailed:(ICCameraDevice *)camera;
- (void)cameraPropertyChanged:(ICCameraDevice *)camera code:(PTPPropertyCode)code value:(NSString *)value values:(NSArray<NSString *> *)values labels:(NSArray<NSString *> *)labels readOnly:(BOOL)readOnly;
- (void)cameraPropertyChanged:(ICCameraDevice *)camera code:(PTPPropertyCode)code value:(NSNumber *)value min:(NSNumber *)min max:(NSNumber *)max step:(NSNumber *)step readOnly:(BOOL)readOnly;
- (void)cameraPropertyChanged:(ICCameraDevice *)camera code:(PTPPropertyCode)code value:(NSString *)value readOnly:(BOOL)readOnly;
- (void)cameraDisconnected:(ICCameraDevice *)camera;
- (void)cameraRemoved:(ICCameraDevice *)camera;
- (void)cameraCanCapture:(ICCameraDevice *)camera;
- (void)cameraCanFocus:(ICCameraDevice *)camera;
- (void)cameraCanStream:(ICCameraDevice *)camera;
@end

@interface PTPDelegate : NSObject <ICDeviceBrowserDelegate, ICCameraDeviceDelegate, ICCameraDeviceDownloadDelegate, PTPDelegateProtocol>

@end

//------------------------------------------------------------------------------------------------------------------------------

