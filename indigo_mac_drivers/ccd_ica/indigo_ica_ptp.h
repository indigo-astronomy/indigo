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

  // MTP codes
  
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
  
  // Apple codes
  
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
  PTPPropertyCodeNikonAFCModePriority = 0xD048,
  PTPPropertyCodeNikonAFSModePriority = 0xD049,
  PTPPropertyCodeNikonGroupDynamicAF = 0xD04A,
  PTPPropertyCodeNikonAFActivation = 0xD04B,
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
  PTPPropertyCodeNikonShootingSpeed = 0xD068,
  PTPPropertyCodeNikonMaximumShots = 0xD069,
  PTPPropertyCodeNikonExposureDelayMode = 0xD06A,
  PTPPropertyCodeNikonLongExposureNoiseReduction = 0xD06B,
  PTPPropertyCodeNikonFileNumberSequence = 0xD06C,
  PTPPropertyCodeNikonControlPanelFinderRearControl = 0xD06D,
  PTPPropertyCodeNikonControlPanelFinderViewfinder = 0xD06E,
  PTPPropertyCodeNikonIllumination = 0xD06F,
  PTPPropertyCodeNikonNrHighISO = 0xD070,
  PTPPropertyCodeNikonSHSETCHGUIDDISP = 0xD071,
  PTPPropertyCodeNikonArtistName = 0xD072,
  PTPPropertyCodeNikonCopyrightInfo = 0xD073,
  PTPPropertyCodeNikonFlashSyncSpeed = 0xD074,
  PTPPropertyCodeNikonFlashShutterSpeed = 0xD075,
  PTPPropertyCodeNikonAAFlashMode = 0xD076,
  PTPPropertyCodeNikonModelingFlash = 0xD077,
  PTPPropertyCodeNikonBracketSet = 0xD078,
  PTPPropertyCodeNikonManualModeBracketing = 0xD079,
  PTPPropertyCodeNikonBracketOrder = 0xD07A,
  PTPPropertyCodeNikonAutoBracketSelection = 0xD07B,
  PTPPropertyCodeNikonBracketingSet = 0xD07C,
  PTPPropertyCodeNikonCenterButtonShootingMode = 0xD080,
  PTPPropertyCodeNikonCenterButtonPlaybackMode = 0xD081,
  PTPPropertyCodeNikonMultiselector = 0xD082,
  PTPPropertyCodeNikonPhotoInfoPlayback = 0xD083,
  PTPPropertyCodeNikonAssignFuncButton = 0xD084,
  PTPPropertyCodeNikonCustomizeCommDials = 0xD085,
  PTPPropertyCodeNikonReverseCommandDial = 0xD086,
  PTPPropertyCodeNikonApertureSetting = 0xD087,
  PTPPropertyCodeNikonMenusAndPlayback = 0xD088,
  PTPPropertyCodeNikonButtonsAndDials = 0xD089,
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
  PTPPropertyCodeNikonMovHiQuality = 0xD0A7,
  PTPPropertyCodeNikonMovMicSensitivity = 0xD0A8,
  PTPPropertyCodeNikonMovWindNoiceReduction = 0xD0AA,
  PTPPropertyCodeNikonLiveViewScreenDisplaySetting = 0xD0B2,
  PTPPropertyCodeNikonMonitorOffDelay = 0xD0B3,
  PTPPropertyCodeNikonISOSensitivity = 0xD0B5,
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
  PTPPropertyCodeNikonRemainingShots = 0xD103,
  PTPPropertyCodeNikonAFLockStatus = 0xD104,
  PTPPropertyCodeNikonAELockStatus = 0xD105,
  PTPPropertyCodeNikonFVLockStatus = 0xD106,
  PTPPropertyCodeNikonAutofocusLCDTopMode2 = 0xD107,
  PTPPropertyCodeNikonAutofocusSensor = 0xD108,
  PTPPropertyCodeNikonFlexibleProgram = 0xD109,
  PTPPropertyCodeNikonLightMeter = 0xD10A,
  PTPPropertyCodeNikonSaveMedia = 0xD10B,
  PTPPropertyCodeNikonUSBSpeed = 0xD10C,
  PTPPropertyCodeNikonCCDNumber = 0xD10D,
  PTPPropertyCodeNikonCameraInclination = 0xD10E,
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
  PTPPropertyCodeNikonLiveViewImageSize = 0xD1AC,
  PTPPropertyCodeNikonBlinkingStatus = 0xD1B0,
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
  PTPPropertyCodeNikonMovieNrHighISO = 0xD236,
  
  // Canon codes
  
  PTPPropertyCodeCanonAperture = 0xD101,
  PTPPropertyCodeCanonShutterSpeed = 0xD102,
  PTPPropertyCodeCanonISOSpeed = 0xD103,
  PTPPropertyCodeCanonExpCompensation = 0xD104,
  PTPPropertyCodeCanonAutoExposureMode = 0xD105,
  PTPPropertyCodeCanonDriveMode = 0xD106,
  PTPPropertyCodeCanonMeteringMode = 0xD107,
  PTPPropertyCodeCanonFocusMode = 0xD108,
  PTPPropertyCodeCanonWhiteBalance = 0xD109,
  PTPPropertyCodeCanonColorTemperature = 0xD10A,
  PTPPropertyCodeCanonWhiteBalanceAdjustA = 0xD10B,
  PTPPropertyCodeCanonWhiteBalanceAdjustB = 0xD10C,
  PTPPropertyCodeCanonWhiteBalanceXA = 0xD10D,
  PTPPropertyCodeCanonWhiteBalanceXB = 0xD10E,
  PTPPropertyCodeCanonColorSpace = 0xD10F,
  PTPPropertyCodeCanonPictureStyle = 0xD110,
  PTPPropertyCodeCanonBatteryPower = 0xD111,
  PTPPropertyCodeCanonBatterySelect = 0xD112,
  PTPPropertyCodeCanonCameraTime = 0xD113,
  PTPPropertyCodeCanonAutoPowerOff = 0xD114,
  PTPPropertyCodeCanonOwner = 0xD115,
  PTPPropertyCodeCanonModelID = 0xD116,
  PTPPropertyCodeCanonPTPExtensionVersion = 0xD119,
  PTPPropertyCodeCanonDPOFVersion = 0xD11A,
  PTPPropertyCodeCanonAvailableShots = 0xD11B,
  PTPPropertyCodeCanonCaptureDestination = 0xD11C,
  PTPPropertyCodeCanonBracketMode = 0xD11D,
  PTPPropertyCodeCanonCurrentStorage = 0xD11E,
  PTPPropertyCodeCanonCurrentFolder = 0xD11F,
  PTPPropertyCodeCanonImageFormat = 0xD120	,
  PTPPropertyCodeCanonImageFormatCF = 0xD121	,
  PTPPropertyCodeCanonImageFormatSD = 0xD122	,
  PTPPropertyCodeCanonImageFormatExtHD = 0xD123	,
  PTPPropertyCodeCanonCompressionS = 0xD130,
  PTPPropertyCodeCanonCompressionM1 = 0xD131,
  PTPPropertyCodeCanonCompressionM2 = 0xD132,
  PTPPropertyCodeCanonCompressionL = 0xD133,
  PTPPropertyCodeCanonAEModeDial = 0xD138,
  PTPPropertyCodeCanonAEModeCustom = 0xD139,
  PTPPropertyCodeCanonMirrorUpSetting = 0xD13A,
  PTPPropertyCodeCanonHighlightTonePriority = 0xD13B,
  PTPPropertyCodeCanonAFSelectFocusArea = 0xD13C,
  PTPPropertyCodeCanonHDRSetting = 0xD13D,
  PTPPropertyCodeCanonPCWhiteBalance1 = 0xD140,
  PTPPropertyCodeCanonPCWhiteBalance2 = 0xD141,
  PTPPropertyCodeCanonPCWhiteBalance3 = 0xD142,
  PTPPropertyCodeCanonPCWhiteBalance4 = 0xD143,
  PTPPropertyCodeCanonPCWhiteBalance5 = 0xD144,
  PTPPropertyCodeCanonMWhiteBalance = 0xD145,
  PTPPropertyCodeCanonMWhiteBalanceEx = 0xD146,
  PTPPropertyCodeCanonUnknownPropD14D = 0xD14D  ,
  PTPPropertyCodeCanonPictureStyleStandard = 0xD150,
  PTPPropertyCodeCanonPictureStylePortrait = 0xD151,
  PTPPropertyCodeCanonPictureStyleLandscape = 0xD152,
  PTPPropertyCodeCanonPictureStyleNeutral = 0xD153,
  PTPPropertyCodeCanonPictureStyleFaithful = 0xD154,
  PTPPropertyCodeCanonPictureStyleBlackWhite = 0xD155,
  PTPPropertyCodeCanonPictureStyleAuto = 0xD156,
  PTPPropertyCodeCanonPictureStyleUserSet1 = 0xD160,
  PTPPropertyCodeCanonPictureStyleUserSet2 = 0xD161,
  PTPPropertyCodeCanonPictureStyleUserSet3 = 0xD162,
  PTPPropertyCodeCanonPictureStyleParam1 = 0xD170,
  PTPPropertyCodeCanonPictureStyleParam2 = 0xD171,
  PTPPropertyCodeCanonPictureStyleParam3 = 0xD172,
  PTPPropertyCodeCanonHighISOSettingNoiseReduction = 0xD178,
  PTPPropertyCodeCanonMovieServoAF = 0xD179,
  PTPPropertyCodeCanonContinuousAFValid = 0xD17A,
  PTPPropertyCodeCanonAttenuator = 0xD17B,
  PTPPropertyCodeCanonUTCTime = 0xD17C,
  PTPPropertyCodeCanonTimezone = 0xD17D,
  PTPPropertyCodeCanonSummertime = 0xD17E,
  PTPPropertyCodeCanonFlavorLUTParams = 0xD17F,
  PTPPropertyCodeCanonCustomFunc1 = 0xD180,
  PTPPropertyCodeCanonCustomFunc2 = 0xD181,
  PTPPropertyCodeCanonCustomFunc3 = 0xD182,
  PTPPropertyCodeCanonCustomFunc4 = 0xD183,
  PTPPropertyCodeCanonCustomFunc5 = 0xD184,
  PTPPropertyCodeCanonCustomFunc6 = 0xD185,
  PTPPropertyCodeCanonCustomFunc7 = 0xD186,
  PTPPropertyCodeCanonCustomFunc8 = 0xD187,
  PTPPropertyCodeCanonCustomFunc9 = 0xD188,
  PTPPropertyCodeCanonCustomFunc10 = 0xD189,
  PTPPropertyCodeCanonCustomFunc11 = 0xD18a,
  PTPPropertyCodeCanonCustomFunc12 = 0xD18b,
  PTPPropertyCodeCanonCustomFunc13 = 0xD18c,
  PTPPropertyCodeCanonCustomFunc14 = 0xD18d,
  PTPPropertyCodeCanonCustomFunc15 = 0xD18e,
  PTPPropertyCodeCanonCustomFunc16 = 0xD18f,
  PTPPropertyCodeCanonCustomFunc17 = 0xD190,
  PTPPropertyCodeCanonCustomFunc18 = 0xD191,
  PTPPropertyCodeCanonCustomFunc19 = 0xD192,
  PTPPropertyCodeCanonInnerDevelop = 0xD193,
  PTPPropertyCodeCanonMultiAspect = 0xD194,
  PTPPropertyCodeCanonMovieSoundRecord = 0xD195,
  PTPPropertyCodeCanonMovieRecordVolume = 0xD196,
  PTPPropertyCodeCanonWindCut = 0xD197,
  PTPPropertyCodeCanonExtenderType = 0xD198,
  PTPPropertyCodeCanonOLCInfoVersion = 0xD199,
  PTPPropertyCodeCanonUnknownPropD19A = 0xD19A ,
  PTPPropertyCodeCanonUnknownPropD19C = 0xD19C ,
  PTPPropertyCodeCanonUnknownPropD19D = 0xD19D ,
  PTPPropertyCodeCanonCustomFuncEx = 0xD1a0,
  PTPPropertyCodeCanonMyMenu = 0xD1a1,
  PTPPropertyCodeCanonMyMenuList = 0xD1a2,
  PTPPropertyCodeCanonWftStatus = 0xD1a3,
  PTPPropertyCodeCanonWftInputTransmission = 0xD1a4,
  PTPPropertyCodeCanonHDDirectoryStructure = 0xD1a5,
  PTPPropertyCodeCanonBatteryInfo = 0xD1a6,
  PTPPropertyCodeCanonAdapterInfo = 0xD1a7,
  PTPPropertyCodeCanonLensStatus = 0xD1a8,
  PTPPropertyCodeCanonQuickReviewTime = 0xD1a9,
  PTPPropertyCodeCanonCardExtension = 0xD1aa,
  PTPPropertyCodeCanonTempStatus = 0xD1ab,
  PTPPropertyCodeCanonShutterCounter = 0xD1ac,
  PTPPropertyCodeCanonSpecialOption = 0xD1ad,
  PTPPropertyCodeCanonPhotoStudioMode = 0xD1ae,
  PTPPropertyCodeCanonSerialNumber = 0xD1af,
  PTPPropertyCodeCanonEVFOutputDevice = 0xD1b0,
  PTPPropertyCodeCanonEVFMode = 0xD1b1,
  PTPPropertyCodeCanonDepthOfFieldPreview = 0xD1b2,
  PTPPropertyCodeCanonEVFSharpness = 0xD1b3,
  PTPPropertyCodeCanonEVFWBMode = 0xD1b4,
  PTPPropertyCodeCanonEVFClickWBCoeffs = 0xD1b5,
  PTPPropertyCodeCanonEVFColorTemp = 0xD1b6,
  PTPPropertyCodeCanonExposureSimMode = 0xD1b7,
  PTPPropertyCodeCanonEVFRecordStatus = 0xD1b8,
  PTPPropertyCodeCanonLvAfSystem = 0xD1ba,
  PTPPropertyCodeCanonMovSize = 0xD1bb,
  PTPPropertyCodeCanonLvViewTypeSelect = 0xD1bc,
  PTPPropertyCodeCanonMirrorDownStatus = 0xD1bd,
  PTPPropertyCodeCanonMovieParam = 0xD1be,
  PTPPropertyCodeCanonMirrorLockupState = 0xD1bf,
  PTPPropertyCodeCanonFlashChargingState = 0xD1C0,
  PTPPropertyCodeCanonAloMode = 0xD1C1,
  PTPPropertyCodeCanonFixedMovie = 0xD1C2,
  PTPPropertyCodeCanonOneShotRawOn = 0xD1C3,
  PTPPropertyCodeCanonErrorForDisplay = 0xD1C4,
  PTPPropertyCodeCanonAEModeMovie = 0xD1C5,
  PTPPropertyCodeCanonBuiltinStroboMode = 0xD1C6,
  PTPPropertyCodeCanonStroboDispState = 0xD1C7,
  PTPPropertyCodeCanonStroboETTL2Metering = 0xD1C8,
  PTPPropertyCodeCanonContinousAFMode = 0xD1C9,
  PTPPropertyCodeCanonMovieParam2 = 0xD1CA,
  PTPPropertyCodeCanonStroboSettingExpComposition = 0xD1CB,
  PTPPropertyCodeCanonMovieParam3 = 0xD1CC,
  PTPPropertyCodeCanonLVMedicalRotate = 0xD1CF,
  PTPPropertyCodeCanonArtist = 0xD1d0,
  PTPPropertyCodeCanonCopyright = 0xD1d1,
  PTPPropertyCodeCanonBracketValue = 0xD1d2,
  PTPPropertyCodeCanonFocusInfoEx = 0xD1d3,
  PTPPropertyCodeCanonDepthOfField = 0xD1d4,
  PTPPropertyCodeCanonBrightness = 0xD1d5,
  PTPPropertyCodeCanonLensAdjustParams = 0xD1d6,
  PTPPropertyCodeCanonEFComp = 0xD1d7,
  PTPPropertyCodeCanonLensName = 0xD1d8,
  PTPPropertyCodeCanonAEB = 0xD1d9,
  PTPPropertyCodeCanonStroboSetting = 0xD1da,
  PTPPropertyCodeCanonStroboWirelessSetting = 0xD1db,
  PTPPropertyCodeCanonStroboFiring = 0xD1dc,
  PTPPropertyCodeCanonLensID = 0xD1dd,
  PTPPropertyCodeCanonLCDBrightness = 0xD1de,
  PTPPropertyCodeCanonCADarkBright = 0xD1df
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
+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension;
+(NSString *)typeName:(PTPDataTypeCode)type;
-(id)initWithCode:(PTPPropertyCode)propertyCode vendorExtension:(PTPVendorExtension)vendorExtension;
-(id)initWithData:(NSData*)data vendorExtension:(PTPVendorExtension)vendorExtension;
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

@protocol PTPDelegateProtocol;

@interface PTPCamera: NSObject<ICCameraDeviceDelegate, ICCameraDeviceDownloadDelegate>

@property (readonly) ICCameraDevice *icCamera;
@property (readonly) NSObject<PTPDelegateProtocol> *delegate;
@property (readonly) NSString *name;
@property (readonly) PTPDeviceInfo *info;

@property NSObject *userData;

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate;

-(PTPRequest *)allocRequest;
-(PTPResponse *)allocResponse;
-(PTPEvent *)allocEvent;
-(PTPDeviceInfo *)allocDeviceInfo;

-(void)requestOpenSession;
-(void)requestCloseSession;
-(void)requestEnableTethering;

-(void)sendPTPRequest:(PTPRequestCode)operationCode;
-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1;
-(void)sendPTPRequest:(PTPRequestCode)operationCode param1:(unsigned int)parameter1 param2:(unsigned int)parameter2;
-(void)setProperty:(PTPPropertyCode)code value:(NSString *)value;
-(void)lock;
-(void)unlock;
-(void)startLiveViewZoom:(int)zoom x:(int)x y:(int)y;
-(void)stopLiveView;
-(void)startCapture;
-(void)stopCapture;
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
-(void)cameraDisconnected:(PTPCamera *)camera;
-(void)cameraRemoved:(PTPCamera *)camera;
-(void)cameraCanCapture:(PTPCamera *)camera;
-(void)cameraCanFocus:(PTPCamera *)camera;
-(void)cameraCanStream:(PTPCamera *)camera;
-(void)cameraFocusDone:(PTPCamera *)camera;
-(void)cameraFocusFailed:(PTPCamera *)camera message:(NSString *)message;
-(void)cameraFrame:(PTPCamera *)camera left:(int)left top:(int)top width:(int)width height:(int)height;
@end

@interface PTPBrowser : NSObject <ICDeviceBrowserDelegate>

@property (readonly) NSObject<PTPDelegateProtocol> *delegate;

-(id)initWithDelegate:(NSObject<PTPDelegateProtocol> *)delegate;
-(void)start;
-(void)stop;
@end

//------------------------------------------------------------------------------------------------------------------------------

