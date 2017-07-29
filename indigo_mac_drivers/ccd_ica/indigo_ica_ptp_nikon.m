//
//  indigo_ica_nikon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include "indigo_bus.h"

#import "indigo_ica_ptp_nikon.h"

@implementation PTPNikonRequest

+(NSString *)operationCodeName:(PTPRequestCode)operationCode {
  switch (operationCode) {
    case PTPRequestCodeNikonGetProfileAllData: return @"PTPRequestCodeNikonGetProfileAllData";
    case PTPRequestCodeNikonSendProfileData: return @"PTPRequestCodeNikonSendProfileData";
    case PTPRequestCodeNikonDeleteProfile: return @"PTPRequestCodeNikonDeleteProfile";
    case PTPRequestCodeNikonSetProfileData: return @"PTPRequestCodeNikonSetProfileData";
    case PTPRequestCodeNikonAdvancedTransfer: return @"PTPRequestCodeNikonAdvancedTransfer";
    case PTPRequestCodeNikonGetFileInfoInBlock: return @"PTPRequestCodeNikonGetFileInfoInBlock";
    case PTPRequestCodeNikonCapture: return @"PTPRequestCodeNikonCapture";
    case PTPRequestCodeNikonAfDrive: return @"PTPRequestCodeNikonAfDrive";
    case PTPRequestCodeNikonSetControlMode: return @"PTPRequestCodeNikonSetControlMode";
    case PTPRequestCodeNikonDelImageSDRAM: return @"PTPRequestCodeNikonDelImageSDRAM";
    case PTPRequestCodeNikonGetLargeThumb: return @"PTPRequestCodeNikonGetLargeThumb";
    case PTPRequestCodeNikonCurveDownload: return @"PTPRequestCodeNikonCurveDownload";
    case PTPRequestCodeNikonCurveUpload: return @"PTPRequestCodeNikonCurveUpload";
    case PTPRequestCodeNikonCheckEvent: return @"PTPRequestCodeNikonCheckEvent";
    case PTPRequestCodeNikonDeviceReady: return @"PTPRequestCodeNikonDeviceReady";
    case PTPRequestCodeNikonSetPreWBData: return @"PTPRequestCodeNikonSetPreWBData";
    case PTPRequestCodeNikonGetVendorPropCodes: return @"PTPRequestCodeNikonGetVendorPropCodes";
    case PTPRequestCodeNikonAfCaptureSDRAM: return @"PTPRequestCodeNikonAfCaptureSDRAM";
    case PTPRequestCodeNikonGetPictCtrlData: return @"PTPRequestCodeNikonGetPictCtrlData";
    case PTPRequestCodeNikonSetPictCtrlData: return @"PTPRequestCodeNikonSetPictCtrlData";
    case PTPRequestCodeNikonDelCstPicCtrl: return @"PTPRequestCodeNikonDelCstPicCtrl";
    case PTPRequestCodeNikonGetPicCtrlCapability: return @"PTPRequestCodeNikonGetPicCtrlCapability";
    case PTPRequestCodeNikonGetPreviewImg: return @"PTPRequestCodeNikonGetPreviewImg";
    case PTPRequestCodeNikonStartLiveView: return @"PTPRequestCodeNikonStartLiveView";
    case PTPRequestCodeNikonEndLiveView: return @"PTPRequestCodeNikonEndLiveView";
    case PTPRequestCodeNikonGetLiveViewImg: return @"PTPRequestCodeNikonGetLiveViewImg";
    case PTPRequestCodeNikonMfDrive: return @"PTPRequestCodeNikonMfDrive";
    case PTPRequestCodeNikonChangeAfArea: return @"PTPRequestCodeNikonChangeAfArea";
    case PTPRequestCodeNikonAfDriveCancel: return @"PTPRequestCodeNikonAfDriveCancel";
    case PTPRequestCodeNikonInitiateCaptureRecInMedia: return @"PTPRequestCodeNikonInitiateCaptureRecInMedia";
    case PTPRequestCodeNikonGetVendorStorageIDs: return @"PTPRequestCodeNikonGetVendorStorageIDs";
    case PTPRequestCodeNikonStartMovieRecInCard: return @"PTPRequestCodeNikonStartMovieRecInCard";
    case PTPRequestCodeNikonEndMovieRec: return @"PTPRequestCodeNikonEndMovieRec";
    case PTPRequestCodeNikonTerminateCapture: return @"PTPRequestCodeNikonTerminateCapture";
    case PTPRequestCodeNikonGetDevicePTPIPInfo: return @"PTPRequestCodeNikonGetDevicePTPIPInfo";
    case PTPRequestCodeNikonGetPartialObjectHiSpeed: return @"PTPRequestCodeNikonGetPartialObjectHiSpeed";
  }
  return [PTPRequest operationCodeName:operationCode];
}

-(NSString *)operationCodeName {
  return [PTPNikonRequest operationCodeName:self.operationCode];
}

@end

@implementation PTPNikonResponse

+ (NSString *)responseCodeName:(PTPResponseCode)responseCode {
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
  return [PTPResponse responseCodeName:responseCode];
}

-(NSString *)responseCodeName {
  return [PTPNikonResponse responseCodeName:self.responseCode];
}

@end

@implementation PTPNikonEvent

+(NSString *)eventCodeName:(PTPEventCode)eventCode {
  switch (eventCode) {
    case PTPEventCodeNikonObjectAddedInSDRAM: return @"PTPEventCodeNikonObjectAddedInSDRAM";
    case PTPEventCodeNikonCaptureCompleteRecInSdram: return @"PTPEventCodeNikonCaptureCompleteRecInSdram";
    case PTPEventCodeNikonAdvancedTransfer: return @"PTPEventCodeNikonAdvancedTransfer";
    case PTPEventCodeNikonPreviewImageAdded: return @"PTPEventCodeNikonPreviewImageAdded";
  }
  return [PTPEvent eventCodeName:eventCode];
}

-(NSString *)eventCodeName {
  return [PTPNikonEvent eventCodeName:self.eventCode];
}

@end

@implementation PTPNikonProperty : PTPProperty

+(NSString *)propertyCodeName:(PTPPropertyCode)propertyCode {
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
    case PTPPropertyCodeNikonAFCModePriority: return @"PTPPropertyCodeNikonAFCModePriority";
    case PTPPropertyCodeNikonAFSModePriority: return @"PTPPropertyCodeNikonAFSModePriority";
    case PTPPropertyCodeNikonGroupDynamicAF: return @"PTPPropertyCodeNikonGroupDynamicAF";
    case PTPPropertyCodeNikonAFActivation: return @"PTPPropertyCodeNikonAFActivation";
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
    case PTPPropertyCodeNikonISOSensitivity: return @"PTPPropertyCodeNikonISOSensitivity";
    case PTPPropertyCodeNikonImgConfTime: return @"PTPPropertyCodeNikonImgConfTime";
    case PTPPropertyCodeNikonAutoOffTimers: return @"PTPPropertyCodeNikonAutoOffTimers";
    case PTPPropertyCodeNikonAngleLevel: return @"PTPPropertyCodeNikonAngleLevel";
    case PTPPropertyCodeNikonShootingSpeed: return @"PTPPropertyCodeNikonShootingSpeed";
    case PTPPropertyCodeNikonMaximumShots: return @"PTPPropertyCodeNikonMaximumShots";
    case PTPPropertyCodeNikonExposureDelayMode: return @"PTPPropertyCodeNikonExposureDelayMode";
    case PTPPropertyCodeNikonLongExposureNoiseReduction: return @"PTPPropertyCodeNikonLongExposureNoiseReduction";
    case PTPPropertyCodeNikonFileNumberSequence: return @"PTPPropertyCodeNikonFileNumberSequence";
    case PTPPropertyCodeNikonControlPanelFinderRearControl: return @"PTPPropertyCodeNikonControlPanelFinderRearControl";
    case PTPPropertyCodeNikonControlPanelFinderViewfinder: return @"PTPPropertyCodeNikonControlPanelFinderViewfinder";
    case PTPPropertyCodeNikonIllumination: return @"PTPPropertyCodeNikonIllumination";
    case PTPPropertyCodeNikonNrHighISO: return @"PTPPropertyCodeNikonNrHighISO";
    case PTPPropertyCodeNikonSHSETCHGUIDDISP: return @"PTPPropertyCodeNikonSHSETCHGUIDDISP";
    case PTPPropertyCodeNikonArtistName: return @"PTPPropertyCodeNikonArtistName";
    case PTPPropertyCodeNikonCopyrightInfo: return @"PTPPropertyCodeNikonCopyrightInfo";
    case PTPPropertyCodeNikonFlashSyncSpeed: return @"PTPPropertyCodeNikonFlashSyncSpeed";
    case PTPPropertyCodeNikonFlashShutterSpeed: return @"PTPPropertyCodeNikonFlashShutterSpeed";
    case PTPPropertyCodeNikonAAFlashMode: return @"PTPPropertyCodeNikonAAFlashMode";
    case PTPPropertyCodeNikonModelingFlash: return @"PTPPropertyCodeNikonModelingFlash";
    case PTPPropertyCodeNikonBracketSet: return @"PTPPropertyCodeNikonBracketSet";
    case PTPPropertyCodeNikonManualModeBracketing: return @"PTPPropertyCodeNikonManualModeBracketing";
    case PTPPropertyCodeNikonBracketOrder: return @"PTPPropertyCodeNikonBracketOrder";
    case PTPPropertyCodeNikonAutoBracketSelection: return @"PTPPropertyCodeNikonAutoBracketSelection";
    case PTPPropertyCodeNikonBracketingSet: return @"PTPPropertyCodeNikonBracketingSet";
    case PTPPropertyCodeNikonCenterButtonShootingMode: return @"PTPPropertyCodeNikonCenterButtonShootingMode";
    case PTPPropertyCodeNikonCenterButtonPlaybackMode: return @"PTPPropertyCodeNikonCenterButtonPlaybackMode";
    case PTPPropertyCodeNikonMultiselector: return @"PTPPropertyCodeNikonFultiselector";
    case PTPPropertyCodeNikonPhotoInfoPlayback: return @"PTPPropertyCodeNikonPhotoInfoPlayback";
    case PTPPropertyCodeNikonAssignFuncButton: return @"PTPPropertyCodeNikonAssignFuncButton";
    case PTPPropertyCodeNikonCustomizeCommDials: return @"PTPPropertyCodeNikonCustomizeCommDials";
    case PTPPropertyCodeNikonReverseCommandDial: return @"PTPPropertyCodeNikonReverseCommandDial";
    case PTPPropertyCodeNikonApertureSetting: return @"PTPPropertyCodeNikonApertureSetting";
    case PTPPropertyCodeNikonMenusAndPlayback: return @"PTPPropertyCodeNikonMenusAndPlayback";
    case PTPPropertyCodeNikonButtonsAndDials: return @"PTPPropertyCodeNikonButtonsAndDials";
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
    case PTPPropertyCodeNikonMovHiQuality: return @"PTPPropertyCodeNikonMovHiQuality";
    case PTPPropertyCodeNikonMovMicSensitivity: return @"PTPPropertyCodeNikonMovMicSensitivity";
    case PTPPropertyCodeNikonMovWindNoiceReduction: return @"PTPPropertyCodeNikonMovWindNoiceReduction";
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
    case PTPPropertyCodeNikonRemainingShots: return @"PTPPropertyCodeNikonRemainingShots";
    case PTPPropertyCodeNikonAFLockStatus: return @"PTPPropertyCodeNikonAFLockStatus";
    case PTPPropertyCodeNikonAELockStatus: return @"PTPPropertyCodeNikonAELockStatus";
    case PTPPropertyCodeNikonFVLockStatus: return @"PTPPropertyCodeNikonFVLockStatus";
    case PTPPropertyCodeNikonAutofocusLCDTopMode2: return @"PTPPropertyCodeNikonAutofocusLCDTopMode2";
    case PTPPropertyCodeNikonAutofocusSensor: return @"PTPPropertyCodeNikonAutofocusSensor";
    case PTPPropertyCodeNikonFlexibleProgram: return @"PTPPropertyCodeNikonFlexibleProgram";
    case PTPPropertyCodeNikonLightMeter: return @"PTPPropertyCodeNikonLightMeter";
    case PTPPropertyCodeNikonSaveMedia: return @"PTPPropertyCodeNikonSaveMedia";
    case PTPPropertyCodeNikonUSBSpeed: return @"PTPPropertyCodeNikonUSBSpeed";
    case PTPPropertyCodeNikonCCDNumber: return @"PTPPropertyCodeNikonCCDNumber";
    case PTPPropertyCodeNikonCameraInclination: return @"PTPPropertyCodeNikonCameraInclination";
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
    case PTPPropertyCodeNikonLiveViewImageSize: return @"PTPPropertyCodeNikonLiveViewImageSize";
    case PTPPropertyCodeNikonBlinkingStatus: return @"PTPPropertyCodeNikonBlinkingStatus";
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
    case PTPPropertyCodeNikonMovieNrHighISO: return @"PTPPropertyCodeNikonMovieNrHighISO";
  }
  return [PTPProperty propertyCodeName:propertyCode];
}

-(NSString *)propertyCodeName {
  return [PTPNikonProperty propertyCodeName:self.propertyCode];
}

@end


@implementation PTPNikonDeviceInfo

-(NSString *)debug {
  NSMutableString *s = [NSMutableString stringWithFormat:@"%@ %@, PTP V%.2f + %@ V%.2f\n", self.model, self.version, self.standardVersion / 100.0, self.vendorExtensionDesc, self.vendorExtensionVersion / 100.0];
  if (self.operationsSupported.count > 0) {
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPNikonRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPNikonEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
    for (NSNumber *code in self.propertiesSupported) {
      PTPProperty *property = self.properties[code];
      if (property)
        [s appendFormat:@"%@\n", property];
      else
        [s appendFormat:@"%@\n", [PTPNikonProperty propertyCodeName:code.intValue]];
    }
  }
  return s;
}
@end

static struct info {
  const char *name;
  int width, height;
  float pixelSize;
} info[] = {
  { "D1", 2000, 1312, 11.8 },
  { "D1H", 2000, 1312, 11.8 },
  { "D1X", 3008, 2000, 7.87 },
  { "D100", 3008, 2000, 7.87 },
  { "D2H", 2464, 1632, 9.45 },
  { "D2HS", 2464, 1632, 9.45 },
  { "D2X", 4288, 2848, 5.52 },
  { "D2XS", 4288, 2848, 5.52 },
  { "D200", 3872, 2592, 6.12 },
  { "D3", 4256, 2832, 8.45 },
  { "D3S", 4256, 2832, 8.45 },
  { "D3X", 6048, 4032, 5.95 },
  { "D300", 4288, 2848, 5.50 },
  { "D300S", 4288, 2848, 5.50 },
  { "D3000", 3872, 2592, 6.09 },
  { "D3100", 4608, 3072, 4.94 },
  { "D3200", 6016, 4000, 3.92 },
  { "D3300", 6016, 4000, 3.92 },
  { "D3400", 6000, 4000, 3.92 },
  { "D3A", 4256, 2832, 8.45 },
  { "D4", 4928, 3280, 7.30 },
  { "D4S", 4928, 3280, 7.30 },
  { "D40", 3008, 2000, 7.87 },
  { "D40X", 3872, 2592, 6.09 },
  { "D5", 5568, 3712, 6.40 },
  { "D50", 3008, 2000, 7.87 },
  { "D500", 5568, 3712, 4.23 },
  { "D5000", 4288, 2848, 5.50 },
  { "D5100", 4928, 3264, 4.78 },
  { "D5200", 6000, 4000, 3.92 },
  { "D5300", 6000, 4000, 3.92 },
  { "D5500", 6000, 4000, 3.92 },
  { "D5600", 6000, 4000, 3.92 },
  { "D60", 3872, 2592, 6.09 },
  { "D600", 6016, 4016, 5.95 },
  { "D610", 6016, 4016, 5.95 },
  { "D70", 3008, 2000, 7.87 },
  { "D70S", 3008, 2000, 7.87 },
  { "D700", 4256, 2832, 8.45 },
  { "D7000", 4928, 3264, 4.78 },
  { "D7100", 6000, 4000, 3.92 },
  { "D7200", 6000, 4000, 3.92 },
  { "D750", 6016, 4016, 3.92 },
  { "D7500", 5568, 3712, 6.40 },
  { "D80", 3872, 2592, 6.09 },
  { "D800", 7360, 4912, 4.88 },
  { "D800E", 7360, 4912, 4.88 },
  { "D810", 7360, 4912, 4.88 },
  { "D810A", 7360, 4912, 4.88 },
  { "D90", 4288, 2848, 5.50 },
  { "DF", 4928, 3264, 4.78 },
  { NULL, 0, 0, 0 }
};

@implementation PTPNikonCamera {
  unsigned int liveViewX, liveViewY;
  NSString *liveViewZoom;
  NSTimer *ptpPreviewTimer;
}

-(NSString *)name {
  if ([super.name.uppercaseString containsString:@"NIKON"])
    return super.name;
  return [NSString stringWithFormat:@"Nikon %@", super.name];
}

-(PTPVendorExtension) extension {
  return PTPVendorExtensionNikon;
}

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {
    const char *name = [super.name.uppercaseString cStringUsingEncoding:NSASCIIStringEncoding];
    for (int i = 0; info[i].name; i++)
      if (!strcmp(name, info[i].name)) {
        self.width = info[i].width;
        self.height = info[i].height;
        self.pixelSize = info[i].pixelSize;
        break;
      }
  }
  return self;
}

-(void)didRemoveDevice:(ICDevice *)device {
  [ptpPreviewTimer invalidate];
  ptpPreviewTimer = nil;
  [super didRemoveDevice:device];
}


-(Class)requestClass {
  return PTPNikonRequest.class;
}

-(Class)responseClass {
  return PTPNikonResponse.class;
}

-(Class)eventClass {
  return PTPNikonEvent.class;
}

-(Class)propertyClass {
  return PTPNikonProperty.class;
}

-(Class)deviceInfoClass {
  return PTPNikonDeviceInfo.class;
}

-(void)checkForEvent {
  if ([self operationIsSupported:PTPRequestCodeNikonCheckEvent]) {
    [self sendPTPRequest:PTPRequestCodeNikonCheckEvent];
  } else {
    for (NSNumber *code in self.info.propertiesSupported)
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
  }
}

-(void)processEvent:(PTPEvent *)event {
  switch (event.eventCode) {
    case PTPEventCodeNikonObjectAddedInSDRAM: {
      [self.delegate debug:[NSString stringWithFormat:@"Object added to SDRAM"]];
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
    case PTPPropertyCodeExposureProgramMode: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Program", @3: @"Aperture priority", @4: @"Shutter priority", @32784: @"Auto", @32785: @"Portrait", @32786: @"Landscape", @32787:@"Macro", @32788: @"Sport", @32789: @"Night portrait", @32790:@"Night landscape", @32791: @"Child", @32792: @"Scene", @32793: @"Effects" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeWhiteBalance: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Auto", @3: @"One-push Auto", @4: @"Daylight", @5: @"Fluorescent", @6: @"Incandescent", @7: @"Flash", @32784: @"Cloudy", @32785: @"Shade", @32786: @"Color Temperature", @32787: @"Preset" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFocusMeteringMode: {
      NSDictionary *map = @{ @1: @"Center-spot", @2: @"Multi-spot", @32784: @"Single Area", @32785: @"Auto area", @32786: @"3D tracking", @32787: @"21 points", @32788: @"39 points" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeFlashMode: {
      NSDictionary *map = @{ @0: @"Undefined", @1: @"Automatic flash", @2: @"Flash off", @3: @"Fill flash", @4: @"Automatic Red-eye Reduction", @5: @"Red-eye fill flash", @6: @"External sync", @32784: @"Auto", @32785: @"Auto Slow Sync", @32786: @"Rear Curtain Sync + Slow Sync", @32787: @"Red-eye Reduction + Slow Sync" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeStillCaptureMode: {
      NSDictionary *map = @{ @1: @"Single shot", @2: @"Continuous", @3:@"Timelapse", @32784: @"Continuous low speed", @32785: @"Timer", @32786: @"Mirror up", @32787: @"Remote", @32788: @"Timer + Remote", @32789: @"Delayed remote", @32790: @"Quiet shutter release" };
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeNikonLiveViewStatus: {
      if (property.value.description.intValue) {
        [self setProperty:PTPPropertyCodeNikonLiveViewImageZoomRatio value:liveViewZoom];
        [self sendPTPRequest:PTPRequestCodeNikonChangeAfArea param1:liveViewX param2:liveViewY];
        ptpPreviewTimer = [NSTimer scheduledTimerWithTimeInterval:0.3 target:self selector:@selector(getPreviewImage) userInfo:nil repeats:true];
      } else {
        [ptpPreviewTimer invalidate];
        ptpPreviewTimer = nil;
      }
      break;
    }
    case PTPPropertyCodeNikonExternalFlashMode:
    case PTPPropertyCodeNikonColorSpace: {
      NSDictionary *map = @{ @0:@"sRGB", @1:@"Adobe RGB" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonLiveViewImageZoomRatio: {
      NSDictionary *map = @{ @0:@"1x", @1:@"2x", @2:@"3x", @3:@"4x", @4:@"6x", @5:@"8x" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonVignetteCtrl: {
      NSDictionary *map = @{ @0:@"High", @1:@"Normal", @2:@"Low", @3:@"Off" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonBlinkingStatus: {
      NSDictionary *map = @{ @0:@"None", @1:@"Shutter", @2:@"Aperture", @3:@"Shutter + Aperture" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonCleanImageSensor: {
      NSDictionary *map = @{ @0:@"At startup", @1:@"At shutdown", @2:@"At startup + shutdown", @3:@"Off" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonHDRMode: {
      NSDictionary *map = @{ @0:@"Off", @1:@"Low", @2:@"Normal", @3:@"High", @4:@"Extra high", @5:@"Auto" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonMovScreenSize: {
      NSDictionary *map = @{ @0:@"1920x1080 50p", @1:@"1920x1080 25p", @2:@"1920x1080 24p", @3:@"1280x720 50p", @4:@"640x424 25p", @5:@"1920x1080 25p", @6:@"1920x1080 24p", @7:@"1280x720 50p" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonACPower: {
       [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:PTPPropertyCodeBatteryLevel];
      break;
    }
    case PTPPropertyCodeNikonEnableCopyright:
    case PTPPropertyCodeNikonAutoDistortionControl:
    case PTPPropertyCodeNikonAELockStatus:
    case PTPPropertyCodeNikonAFLockStatus:
    case PTPPropertyCodeNikonExternalFlashAttached:
    case PTPPropertyCodeNikonAFCModePriority:
    case PTPPropertyCodeNikonISOAutoTime:
    case PTPPropertyCodeNikonMovHiQuality:
    case PTPPropertyCodeNikonImageRotation:
    case PTPPropertyCodeNikonImageCommentEnable:
    case PTPPropertyCodeNikonManualMovieSetting:
    case PTPPropertyCodeNikonResetBank:
    case PTPPropertyCodeNikonResetBank0:
    case PTPPropertyCodeNikonExposureDelayMode:
    case PTPPropertyCodeNikonLongExposureNoiseReduction:
    case PTPPropertyCodeNikonMovWindNoiceReduction:
    case PTPPropertyCodeNikonBracketing:
    case PTPPropertyCodeNikonNoCFCard:{
      NSDictionary *map = @{ @0:@"Off", @1:@"On" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonEVStep: {
      NSDictionary *map = @{ @0:@"1/3", @1:@"1/2" };
      [self mapValueInterval:property map:map];
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:PTPPropertyCodeExposureBiasCompensation];
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:PTPPropertyCodeNikonFlashExposureCompensation];
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:PTPPropertyCodeNikonExternalFlashCompensation];
      break;
    }
    case PTPPropertyCodeNikonFlashExposureCompensation:
    case PTPPropertyCodeNikonExternalFlashCompensation: {
      NSMutableArray *values = [NSMutableArray array];
      NSMutableArray *labels = [NSMutableArray array];
      int step;
      if (self.info.properties[[NSNumber numberWithUnsignedShort:PTPPropertyCodeNikonEVStep]].value.intValue == 0)
        step = 2;
      else
        step = 3;
      for (int i = property.min.intValue; i <= property.max.intValue; i += step) {
        [values addObject:[NSString stringWithFormat:@"%d", i ]];
        [labels addObject:[NSString stringWithFormat:@"%.1f", i * 0.16666 ]];
      }
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:values labels:labels readOnly:property.readOnly];
      break;
    }
    case PTPPropertyCodeNikonCameraInclination: {
      NSDictionary *map = @{ @0:@"Level", @1:@"Grip is top", @2:@"Grip is bottom", @3:@"Up Down" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonLensID: {
      property.type = PTPDataTypeCodeUnicodeString;
      NSDictionary *lens = @{ @0x0000: @"Fisheye Nikkor 8mm f/2.8 AiS", @0x0001: @"AF Nikkor 50mm f/1.8", @0x0002: @"AF Zoom-Nikkor 35-70mm f/3.3-4.5", @0x0003: @"AF Zoom-Nikkor 70-210mm f/4", @0x0004: @"AF Nikkor 28mm f/2.8", @0x0005: @"AF Nikkor 50mm f/1.4", @0x0006: @"AF Micro-Nikkor 55mm f/2.8", @0x0007: @"AF Zoom-Nikkor 28-85mm f/3.5-4.5", @0x0008: @"AF Zoom-Nikkor 35-105mm f/3.5-4.5", @0x0009: @"AF Nikkor 24mm f/2.8", @0x000A: @"AF Nikkor 300mm f/2.8 IF-ED", @0x000B: @"AF Nikkor 180mm f/2.8 IF-ED", @0x000D: @"AF Zoom-Nikkor 35-135mm f/3.5-4.5", @0x000E: @"AF Zoom-Nikkor 70-210mm f/4", @0x000F: @"AF Nikkor 50mm f/1.8 N", @0x0010: @"AF Nikkor 300mm f/4 IF-ED", @0x0011: @"AF Zoom-Nikkor 35-70mm f/2.8", @0x0012: @"AF Nikkor 70-210mm f/4-5.6", @0x0013: @"AF Zoom-Nikkor 24-50mm f/3.3-4.5", @0x0014: @"AF Zoom-Nikkor 80-200mm f/2.8 ED", @0x0015: @"AF Nikkor 85mm f/1.8", @0x0017: @"Nikkor 500mm f/4 P ED IF", @0x0018: @"AF Zoom-Nikkor 35-135mm f/3.5-4.5 N", @0x001A: @"AF Nikkor 35mm f/2", @0x001B: @"AF Zoom-Nikkor 75-300mm f/4.5-5.6", @0x001C: @"AF Nikkor 20mm f/2.8", @0x001D: @"AF Zoom-Nikkor 35-70mm f/3.3-4.5 N", @0x001E: @"AF Micro-Nikkor 60mm f/2.8", @0x001F: @"AF Micro-Nikkor 105mm f/2.8", @0x0020: @"AF Zoom-Nikkor 80-200mm f/2.8 ED", @0x0021: @"AF Zoom-Nikkor 28-70mm f/3.5-4.5", @0x0022: @"AF DC-Nikkor 135mm f/2", @0x0023: @"Zoom-Nikkor 1200-1700mm f/5.6-8 P ED IF", @0x0024: @"AF Zoom-Nikkor 80-200mm f/2.8D ED", @0x0025: @"AF Zoom-Nikkor 35-70mm f/2.8D", @0x0026: @"AF Zoom-Nikkor 28-70mm f/3.5-4.5D", @0x0027: @"AF-I Nikkor 300mm f/2.8D IF-ED", @0x0028: @"AF-I Nikkor 600mm f/4D IF-ED", @0x002A: @"AF Nikkor 28mm f/1.4D", @0x002B: @"AF Zoom-Nikkor 35-80mm f/4-5.6D", @0x002C: @"AF DC-Nikkor 105mm f/2D", @0x002D: @"AF Micro-Nikkor 200mm f/4D IF-ED", @0x002E: @"AF Nikkor 70-210mm f/4-5.6D", @0x002F: @"AF Zoom-Nikkor 20-35mm f/2.8D IF", @0x0030: @"AF-I Nikkor 400mm f/2.8D IF-ED", @0x0031: @"AF Micro-Nikkor 60mm f/2.8D", @0x0032: @"AF Micro-Nikkor 105mm f/2.8D", @0x0033: @"AF Nikkor 18mm f/2.8D", @0x0034: @"AF Fisheye Nikkor 16mm f/2.8D", @0x0035: @"AF-I Nikkor 500mm f/4D IF-ED", @0x0036: @"AF Nikkor 24mm f/2.8D", @0x0037: @"AF Nikkor 20mm f/2.8D", @0x0038: @"AF Nikkor 85mm f/1.8D", @0x003A: @"AF Zoom-Nikkor 28-70mm f/3.5-4.5D", @0x003B: @"AF Zoom-Nikkor 35-70mm f/2.8D N", @0x003C: @"AF Zoom-Nikkor 80-200mm f/2.8D ED", @0x003D: @"AF Zoom-Nikkor 35-80mm f/4-5.6D", @0x003E: @"AF Nikkor 28mm f/2.8D", @0x003F: @"AF Zoom-Nikkor 35-105mm f/3.5-4.5D", @0x0041: @"AF Nikkor 180mm f/2.8D IF-ED", @0x0042: @"AF Nikkor 35mm f/2D", @0x0043: @"AF Nikkor 50mm f/1.4D", @0x0044: @"AF Zoom-Nikkor 80-200mm f/4.5-5.6D", @0x0045: @"AF Zoom-Nikkor 28-80mm f/3.5-5.6D", @0x0046: @"AF Zoom-Nikkor 35-80mm f/4-5.6D N", @0x0047: @"AF Zoom-Nikkor 24-50mm f/3.3-4.5D", @0x0048: @"AF-S Nikkor 300mm f/2.8D IF-ED", @0x0049: @"AF-S Nikkor 600mm f/4D IF-ED", @0x004A: @"AF Nikkor 85mm f/1.4D IF", @0x004B: @"AF-S Nikkor 500mm f/4D IF-ED", @0x004C: @"AF Zoom-Nikkor 24-120mm f/3.5-5.6D IF", @0x004D: @"AF Zoom-Nikkor 28-200mm f/3.5-5.6D IF", @0x004E: @"AF DC-Nikkor 135mm f/2D", @0x004F: @"IX-Nikkor 24-70mm f/3.5-5.6", @0x0050: @"IX-Nikkor 60-180mm f/4-5.6", @0x0053: @"AF Zoom-Nikkor 80-200mm f/2.8D ED", @0x0054: @"AF Zoom-Micro Nikkor 70-180mm f/4.5-5.6D ED", @0x0056: @"AF Zoom-Nikkor 70-300mm f/4-5.6D ED", @0x0059: @"AF-S Nikkor 400mm f/2.8D IF-ED", @0x005A: @"IX-Nikkor 30-60mm f/4-5.6", @0x005B: @"IX-Nikkor 60-180mm f/4.5-5.6", @0x005D: @"AF-S Zoom-Nikkor 28-70mm f/2.8D IF-ED", @0x005E: @"AF-S Zoom-Nikkor 80-200mm f/2.8D IF-ED", @0x005F: @"AF Zoom-Nikkor 28-105mm f/3.5-4.5D IF", @0x0060: @"AF Zoom-Nikkor 28-80mm f/3.5-5.6D", @0x0061: @"AF Zoom-Nikkor 75-240mm f/4.5-5.6D", @0x0063: @"AF-S Nikkor 17-35mm f/2.8D IF-ED", @0x0064: @"PC Micro-Nikkor 85mm f/2.8D", @0x0065: @"AF VR Zoom-Nikkor 80-400mm f/4.5-5.6D ED", @0x0066: @"AF Zoom-Nikkor 18-35mm f/3.5-4.5D IF-ED", @0x0067: @"AF Zoom-Nikkor 24-85mm f/2.8-4D IF", @0x0068: @"AF Zoom-Nikkor 28-80mm f/3.3-5.6G", @0x0069: @"AF Zoom-Nikkor 70-300mm f/4-5.6G", @0x006A: @"AF-S Nikkor 300mm f/4D IF-ED", @0x006B: @"AF Nikkor ED 14mm f/2.8D", @0x006D: @"AF-S Nikkor 300mm f/2.8D IF-ED II", @0x006E: @"AF-S Nikkor 400mm f/2.8D IF-ED II", @0x006F: @"AF-S Nikkor 500mm f/4D IF-ED II", @0x0070: @"AF-S Nikkor 600mm f/4D IF-ED II", @0x0072: @"Nikkor 45mm f/2.8 P", @0x0074: @"AF-S Zoom-Nikkor 24-85mm f/3.5-4.5G IF-ED", @0x0075: @"AF Zoom-Nikkor 28-100mm f/3.5-5.6G", @0x0076: @"AF Nikkor 50mm f/1.8D", @0x0077: @"AF-S VR Zoom-Nikkor 70-200mm f/2.8G IF-ED", @0x0078: @"AF-S VR Zoom-Nikkor 24-120mm f/3.5-5.6G IF-ED", @0x0079: @"AF Zoom-Nikkor 28-200mm f/3.5-5.6G IF-ED", @0x007A: @"AF-S DX Zoom-Nikkor 12-24mm f/4G IF-ED", @0x007B: @"AF-S VR Zoom-Nikkor 200-400mm f/4G IF-ED", @0x007D: @"AF-S DX Zoom-Nikkor 17-55mm f/2.8G IF-ED", @0x007F: @"AF-S DX Zoom-Nikkor 18-70mm f/3.5-4.5G IF-ED", @0x0080: @"AF DX Fisheye-Nikkor 10.5mm f/2.8G ED", @0x0081: @"AF-S VR Nikkor 200mm f/2G IF-ED", @0x0082: @"AF-S VR Nikkor 300mm f/2.8G IF-ED", @0x0083: @"FSA-L2, EDG 65, 800mm F13 G", @0x0089: @"AF-S DX Zoom-Nikkor 55-200mm f/4-5.6G ED", @0x008A: @"AF-S VR Micro-Nikkor 105mm f/2.8G IF-ED", @0x008B: @"AF-S DX VR Zoom-Nikkor 18-200mm f/3.5-5.6G IF-ED", @0x008C: @"AF-S DX Zoom-Nikkor 18-55mm f/3.5-5.6G ED", @0x008D: @"AF-S VR Zoom-Nikkor 70-300mm f/4.5-5.6G IF-ED", @0x008F: @"AF-S DX Zoom-Nikkor 18-135mm f/3.5-5.6G IF-ED", @0x0090: @"AF-S DX VR Zoom-Nikkor 55-200mm f/4-5.6G IF-ED", @0x0092: @"AF-S Zoom-Nikkor 14-24mm f/2.8G ED", @0x0093: @"AF-S Zoom-Nikkor 24-70mm f/2.8G ED", @0x0094: @"AF-S DX Zoom-Nikkor 18-55mm f/3.5-5.6G ED II", @0x0095: @"PC-E Nikkor 24mm f/3.5D ED", @0x0096: @"AF-S VR Nikkor 400mm f/2.8G ED", @0x0097: @"AF-S VR Nikkor 500mm f/4G ED", @0x0098: @"AF-S VR Nikkor 600mm f/4G ED", @0x0099: @"AF-S DX VR Zoom-Nikkor 16-85mm f/3.5-5.6G ED", @0x009A: @"AF-S DX VR Zoom-Nikkor 18-55mm f/3.5-5.6G", @0x009B: @"PC-E Micro Nikkor 45mm f/2.8D ED", @0x009C: @"AF-S Micro Nikkor 60mm f/2.8G ED", @0x009D: @"PC-E Micro Nikkor 85mm f/2.8D", @0x009E: @"AF-S DX VR Zoom-Nikkor 18-105mm f/3.5-5.6G ED", @0x009F: @"AF-S DX Nikkor 35mm f/1.8G", @0x00A0: @"AF-S Nikkor 50mm f/1.4G", @0x00A1: @"AF-S DX Nikkor 10-24mm f/3.5-4.5G ED", @0x00A2: @"AF-S Nikkor 70-200mm f/2.8G ED VR II", @0x00A3: @"AF-S Nikkor 16-35mm f/4G ED VR", @0x00A4: @"AF-S Nikkor 24mm f/1.4G ED", @0x00A5: @"AF-S Nikkor 28-300mm f/3.5-5.6G ED VR", @0x00A6: @"AF-S Nikkor 300mm f/2.8G IF-ED VR II", @0x00A7: @"AF-S DX Micro Nikkor 85mm f/3.5G ED VR", @0x00A8: @"AF-S Zoom-Nikkor 200-400mm f/4G IF-ED VR II", @0x00A9: @"AF-S Nikkor 200mm f/2G ED VR II", @0x00AA: @"AF-S Nikkor 24-120mm f/4G ED VR", @0x00AC: @"AF-S DX Nikkor 55-300mm f/4.5-5.6G ED VR", @0x00AD: @"AF-S DX Nikkor 18-300mm f/3.5-5.6G ED VR", @0x00AE: @"AF-S Nikkor 85mm f/1.4G", @0x00AF: @"AF-S Nikkor 35mm f/1.4G", @0x00B0: @"AF-S Nikkor 50mm f/1.8G", @0x00B1: @"AF-S DX Micro Nikkor 40mm f/2.8G", @0x00B2: @"AF-S Nikkor 70-200mm f/4G ED VR", @0x00B3: @"AF-S Nikkor 85mm f/1.8G", @0x00B4: @"AF-S Nikkor 24-85mm f/3.5-4.5G ED VR", @0x00B5: @"AF-S Nikkor 28mm f/1.8G", @0x00B6: @"AF-S VR Nikkor 800mm f/5.6E FL ED", @0x00B7: @"AF-S Nikkor 80-400mm f/4.5-5.6G ED VR", @0x00B8: @"AF-S Nikkor 18-35mm f/3.5-4.5G ED", @0x01A0: @"AF-S DX Nikkor 18-140mm f/3.5-5.6G ED VR", @0x01A1: @"AF-S Nikkor 58mm f/1.4G", @0x01A2: @"AF-S DX Nikkor 18-55mm f/3.5-5.6G VR II", @0x01A4: @"AF-S DX Nikkor 18-300mm f/3.5-6.3G ED VR", @0x01A5: @"AF-S Nikkor 35mm f/1.8G ED", @0x01A6: @"AF-S Nikkor 400mm f/2.8E FL ED VR", @0x01A7: @"AF-S DX Nikkor 55-200mm f/4-5.6G ED VR II", @0x01A8: @"AF-S Nikkor 300mm f/4E PF ED VR", @0x01A9: @"AF-S Nikkor 20mm f/1.8G ED", @0x02AA: @"AF-S Nikkor 24-70mm f/2.8E ED VR", @0x02AB: @"AF-S Nikkor 500mm f/4E FL ED VR", @0x02AC: @"AF-S Nikkor 600mm f/4E FL ED VR", @0x02AD: @"AF-S DX Nikkor 16-80mm f/2.8-4E ED VR", @0x02AE: @"AF-S Nikkor 200-500mm f/5.6E ED VR", @0x03A0: @"AF-P DX Nikkor 18-55mm f/3.5-5.6G VR", @0x03A3: @"AF-P DX Nikkor 70–300mm f/4.5–6.3G ED VR", @0x03A4: @"AF-S Nikkor 70-200mm f/2.8E FL ED VR", @0x03A5: @"AF-S Nikkor 105mm f/1.4E ED", @0x03AF: @"AF-S Nikkor 24mm f/1.8G ED" };
      [self.delegate cameraPropertyChanged:self code:property.propertyCode value:lens[property.value] readOnly:true];
      break;
    }
    case PTPPropertyCodeNikonLiveViewImageSize: {
      NSDictionary *map = @{ @1:@"QVGA", @2:@"VGA" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonRawBitMode: {
      NSDictionary *map = @{ @0:@"12 bit", @1:@"14 bit" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonSaveMedia: {
      NSDictionary *map = @{ @0:@"Card", @1:@"SDRAM", @2:@"Card + SDRAM" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonLiveViewAFArea: {
      NSDictionary *map = @{ @0:@"Face priority", @1:@"Wide area", @2:@"Normal area" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonFlourescentType: {
      NSDictionary *map = @{ @0:@"Sodium-vapor", @1:@"Warm-white", @2:@"White", @3:@"Cool-white", @4:@"Day white", @5:@"Daylight", @6:@"High temp. mercury-vapor" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonActiveDLighting: {
      NSDictionary *map = @{ @0:@"High", @1:@"Normal", @2:@"Low", @3:@"Off", @4:@"Extra high", @5:@"Auto" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonActivePicCtrlItem: {
      NSDictionary *map = @{ @0:@"Undefined", @1:@"Standard", @2:@"Neutral", @3:@"Vivid", @4:@"Monochrome", @5:@"Portrait", @6:@"Landscape", @7:@"Flat", @201:@"Custom 1", @202:@"Custom 2", @203:@"Custom 3", @204:@"Custom 4", @205:@"Custom 5", @206:@"Custom 6", @207:@"Custom 7", @208:@"Custom 8", @209:@"Custom 9" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonEffectMode: {
      NSDictionary *map = @{ @0:@"Night Vision", @1:@"Color Sketch", @2:@"Miniature Effect", @3:@"Selective Color", @4:@"Silhouette", @5:@"High Key", @6:@"Low Key" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonSceneMode: {
      NSDictionary *map = @{ @0:@"NightLandscape", @1:@"PartyIndoor", @2:@"BeachSnow", @3:@"Sunset", @4:@"Duskdawn", @5:@"Petportrait", @6:@"Candlelight", @7:@"Blossom", @8:@"AutumnColors", @9:@"Food", @10:@"Silhouette", @11:@"Highkey", @12:@"Lowkey", @13:@"Portrait", @14:@"Landscape", @15:@"Child", @16:@"Sports", @17:@"Closeup", @18:@"NightPortrait" };
      [self mapValueInterval:property map:map];
      break;
    }
    case PTPPropertyCodeNikonLiveViewAFFocus: {
      if (property.value.description.intValue == 3) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"3" values:@[@"3"] labels:@[@"M (fixed)"] readOnly:true];
      } else if (property.max.intValue == 1) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:@[@"0", @"2"] labels:@[@"AF-S", @"AF-F"] readOnly:property.readOnly];
      } else {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:@[@"0", @"2", @"4"] labels:@[@"AF-S", @"AF-F", @"M"] readOnly:property.readOnly];
      }
      break;
    }
    case PTPPropertyCodeFocusMode: {
      NSDictionary *map = @{ @1: @"Manual", @2: @"Automatic", @3:@"Macro", @32784:@"AF-S", @32785:@"AF-C", @32786:@"AF-A", @32787:@"M" };
      if (![self propertyIsSupported:PTPPropertyCodeNikonAutofocusMode]) {
        property.propertyCode = PTPPropertyCodeNikonAutofocusMode;
        property.readOnly = true;
      }
      [self mapValueList:property map:map];
      break;
    }
    case PTPPropertyCodeNikonAutofocusMode: {
      if (property.value.description.intValue == 3) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:@"3" values:@[@"3"] labels:@[@"M (fixed)"] readOnly:true];
      } else if (property.max.intValue == 1) {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:@[@"0", @"1"] labels:@[@"AF-S", @"AF-C"] readOnly:property.readOnly];
      } else {
        [self.delegate cameraPropertyChanged:self code:property.propertyCode value:property.value.description values:@[@"0", @"1", @"2", @"4"] labels:@[@"AF-S", @"AF-C", @"AF-A", @"M"] readOnly:property.readOnly];
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
  if ([self operationIsSupported:PTPRequestCodeInitiateCapture])
    [self.delegate cameraCanExposure:self];
  if ([self operationIsSupported:PTPRequestCodeNikonMfDrive])
    [self.delegate cameraCanFocus:self];
  if ([self operationIsSupported:PTPRequestCodeNikonGetLiveViewImg])
    [self.delegate cameraCanPreview:self];
  [super processConnect];
}

-(void)processRequest:(PTPRequest *)request Response:(PTPResponse *)response inData:(NSData*)data {
  switch (request.operationCode) {
    case PTPRequestCodeGetDeviceInfo: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        self.info = [[self.deviceInfoClass alloc] initWithData:data];
        if ([self operationIsSupported:PTPRequestCodeNikonGetVendorPropCodes]) {
          [self sendPTPRequest:PTPRequestCodeNikonGetVendorPropCodes];
        } else {
          for (NSNumber *code in self.info.propertiesSupported)
            [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
          [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
        }
      }
      break;
    }
    case PTPRequestCodeGetDevicePropValue: {
      switch (request.parameter1) {
        case PTPPropertyCodeNikonLiveViewProhibitCondition: {
          if (response.responseCode == PTPResponseCodeOK) {
            unsigned char *bytes = (void*)[data bytes];
            unsigned int value = ptpReadUnsignedInt(&bytes);
            if (value == 0) {
              [self sendPTPRequest:PTPRequestCodeNikonStartLiveView];
              usleep(100000);
            } else {
              NSMutableString *text = [NSMutableString stringWithFormat:@"LiveViewProhibitCondition 0x%08x", value];
              if (value & 0x80000000)
                [text appendString:@", Exposure Mode is non-P,S,A,M"];
              if (value & 0x01000000)
                [text appendString:@", When Retractable lens is set, the zoom ring does not extend"];
              if (value & 0x00200000)
                [text appendString:@", Bulb warning, ShutterSpeed is Time"];
              if (value & 0x00100000)
                [text appendString:@", Card not formatted"];
              if (value & 0x00080000)
                [text appendString:@", Card error"];
              if (value & 0x00040000)
                [text appendString:@", Card protected"];
              if (value & 0x00020000)
                [text appendString:@", High temperature"];
              if (value & 0x00008000)
                [text appendString:@", Capture command is executing"];
              if (value & 0x00004000)
                [text appendString:@", No memory card is inserted in the camera"];
              if (value & 0x00000800)
                [text appendString:@", Non-CPU lens is attached and ExposureMode is not Manual or Aperture priority"];
              if (value & 0x00000400)
                [text appendString:@", The setting by Aperture ring is valid"];
              if (value & 0x00000200)
                [text appendString:@", TTL error"];
              if (value & 0x00000100)
                [text appendString:@", Battery shortage"];
              if (value & 0x00000080)
                [text appendString:@", Mirror up"];
              if (value & 0x00000040)
                [text appendString:@", Shutter bulb"];
              if (value & 0x00000020)
                [text appendString:@", Aperture ring is not minimum"];
              if (value & 0x00000004)
                [text appendString:@", Sequence error"];
              if (value & 0x00000001)
                [text appendString:@", Recording media is CF/SD card"];
              [ptpPreviewTimer invalidate];
              ptpPreviewTimer = nil;
              [self.delegate cameraExposureFailed:self message:text];
            }
          } else {
            [ptpPreviewTimer invalidate];
            ptpPreviewTimer = nil;
            [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"LiveViewProhibiCondition failed (0x%04x = %@)", response.responseCode, response]];
          }
          break;
        }
      }
      break;
    }
    case PTPRequestCodeNikonDeviceReady: {
      if (response.responseCode == PTPResponseCodeDeviceBusy) {
        usleep(100000);
        [self sendPTPRequest:PTPRequestCodeNikonDeviceReady];
      }
      break;
    }
    case PTPRequestCodeNikonGetVendorPropCodes: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        unsigned char* buffer = (unsigned char*)[data bytes];
        unsigned char* buf = buffer;
        NSArray *codes = ptpReadUnsignedShortArray(&buf);
        [(NSMutableArray *)self.info.propertiesSupported addObjectsFromArray:codes];
        for (NSNumber *code in self.info.propertiesSupported) {
          [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:code.unsignedShortValue];
        }
      }
      [self sendPTPRequest:PTPRequestCodeGetStorageIDs];
      break;
    }
    case PTPRequestCodeNikonInitiateCaptureRecInMedia: {
      if (response.responseCode != PTPResponseCodeOK &&  response.responseCode != PTPResponseCodeDeviceBusy) {
        [self sendPTPRequest:PTPRequestCodeNikonTerminateCapture param1:0 param2:0];
        [ptpPreviewTimer invalidate];
        ptpPreviewTimer = nil;
        [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"InitiateCaptureRecInMedia failed (0x%04x = %@)", response.responseCode, response]];
      }
      break;
    }
    case PTPRequestCodeNikonMfDrive: {
      if (response.responseCode == PTPResponseCodeOK) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
          [self.delegate cameraFocusDone:self];
        });
        
      }
      else
        [self.delegate cameraFocusFailed:self message:[NSString stringWithFormat:@"MfDrive failed (0x%04x = %@)", response.responseCode, response]];
      break;
    }
    case PTPRequestCodeNikonGetLiveViewImg: {
      if (response.responseCode == PTPResponseCodeOK && data) {
        char *bytes = (void*)[data bytes];
        NSData *image;
        if ((bytes[64] & 0xFF) == 0xFF && (bytes[65] & 0xFF) == 0xD8) {
          image = [NSData dataWithBytes:bytes + 64 length:data.length - 64];
          image = [NSData dataWithBytes:bytes + 128 length:data.length - 128];
          unsigned char *buf = (unsigned char *)bytes;
          ptpReadUnsignedShort(&buf); // image width
          ptpReadUnsignedShort(&buf); // image height
          ptpReadUnsignedShort(&buf); // whole width
          ptpReadUnsignedShort(&buf); // whole height
          int frameWidth = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameHeight = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameLeft = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameWidth / 2;
          int frameTop = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameHeight / 2;
          [self.delegate cameraFrame:self left:frameLeft top:frameTop width:frameWidth height:frameHeight];
        } else if ((bytes[128] & 0xFF) == 0xFF && (bytes[129] & 0xFF) == 0xD8) {
          image = [NSData dataWithBytes:bytes + 128 length:data.length - 128];
          unsigned char *buf = (unsigned char *)bytes;
          ptpReadUnsignedShort(&buf); // image width
          ptpReadUnsignedShort(&buf); // image height
          ptpReadUnsignedShort(&buf); // whole width
          ptpReadUnsignedShort(&buf); // whole height
          int frameWidth = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameHeight = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameLeft = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameWidth / 2;
          int frameTop = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameHeight / 2;
          [self.delegate cameraFrame:self left:frameLeft top:frameTop width:frameWidth height:frameHeight];
        } else if ((bytes[384] & 0xFF) == 0xFF && (bytes[385] & 0xFF) == 0xD8) {
          image = [NSData dataWithBytes:bytes + 384 length:data.length - 384];
          unsigned char *buf = (unsigned char *)bytes;
          int header = CFSwapInt32BigToHost(ptpReadUnsignedInt(&buf)); // header size
          if (header == 376) {
            ptpReadUnsignedInt(&buf); // image size
            ptpReadUnsignedShort(&buf); // image width
            ptpReadUnsignedShort(&buf); // image height
          }
          ptpReadUnsignedShort(&buf); // whole width
          ptpReadUnsignedShort(&buf); // whole height
          int frameWidth = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameHeight = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf));
          int frameLeft = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameWidth / 2;
          int frameTop = CFSwapInt16BigToHost(ptpReadUnsignedShort(&buf)) - frameHeight / 2;
          [self.delegate cameraFrame:self left:frameLeft top:frameTop width:frameWidth height:frameHeight];
        }
        if (image) {
          [self.delegate cameraExposureDone:self data:image filename:@"preview.jpeg"];
        } else {
          [ptpPreviewTimer invalidate];
          ptpPreviewTimer = nil;
          [self.delegate cameraExposureFailed:self message:@"JPEG magic not found"];
        }
      } else if (response.responseCode == PTPResponseCodeDeviceBusy) {
        [self.delegate debug:[NSString stringWithFormat:@"PTPResponseCodeDeviceBusy"]];
        usleep(100000);
        [self getPreviewImage];
      } else {
        [self.delegate cameraExposureFailed:self message:[NSString stringWithFormat:@"No data received (0x%04x = %@)", response.responseCode, response]];
        [ptpPreviewTimer invalidate];
        ptpPreviewTimer = nil;
      }
      break;
    }
    case PTPRequestCodeNikonCheckEvent: {
      unsigned char* buffer = (unsigned char*)[data bytes];
      unsigned char* buf = buffer;
      int count = ptpReadUnsignedShort(&buf);
      for (int i = 0; i < count; i++) {
        PTPEventCode code = ptpReadUnsignedShort(&buf);
        unsigned int parameter1 = ptpReadUnsignedInt(&buf);
        PTPEvent *event = [[self.eventClass alloc] initWithCode:code parameter1:parameter1];
        [self.delegate debug:[NSString stringWithFormat:@"Translated to %@", [event description]]];
        [self processEvent:event];
      }
      break;
    }
    case PTPRequestCodeNikonSetControlMode: {
      [self sendPTPRequest:PTPRequestCodeGetDevicePropDesc param1:PTPPropertyCodeExposureProgramMode];
      break;
    }
    default: {
      [super processRequest:request Response:response inData:data];
    }
  }
}

-(void)getPreviewImage {
  [self sendPTPRequest:PTPRequestCodeNikonGetLiveViewImg];
}

-(void)lock {
  [self sendPTPRequest:PTPRequestCodeNikonSetControlMode param1:1];
}

-(void)unlock {
  [self sendPTPRequest:PTPRequestCodeNikonSetControlMode param1:0];
}

-(void)startPreviewZoom:(int)zoom x:(int)x y:(int)y {
  if (zoom < 2)
    zoom = 0;
  else if (zoom < 3)
    zoom = 1;
  else if (zoom < 4)
    zoom = 2;
  else if (zoom < 6)
    zoom = 3;
  else if (zoom < 8)
    zoom = 4;
  else
    zoom = 5;
  liveViewZoom = [NSString stringWithFormat:@"%d", zoom];
  liveViewX = x;
  liveViewY = y;
  [self setProperty:PTPPropertyCodeNikonSaveMedia value:@"1"];
  [self sendPTPRequest:PTPRequestCodeGetDevicePropValue param1:PTPPropertyCodeNikonLiveViewProhibitCondition];
  [self sendPTPRequest:PTPRequestCodeNikonDeviceReady];
}

-(void)stopPreview {
  [ptpPreviewTimer invalidate];
  ptpPreviewTimer = nil;
  [self sendPTPRequest:PTPRequestCodeNikonEndLiveView];
  [self sendPTPRequest:PTPRequestCodeNikonDeviceReady];
  [self setProperty:PTPPropertyCodeNikonSaveMedia value:@"0"];
}

-(void)startExposureWithMirrorLockup:(BOOL)mirrorLockup avoidAF:(BOOL)avoidAF {
  if ([self propertyIsSupported:PTPPropertyCodeNikonExposureDelayMode]) {
    [self setProperty:PTPPropertyCodeNikonExposureDelayMode value:(mirrorLockup ? @"1" : @"0")];
  }
  if ([self operationIsSupported:PTPRequestCodeNikonInitiateCaptureRecInMedia]) {
    [self sendPTPRequest:PTPRequestCodeNikonInitiateCaptureRecInMedia param1:(avoidAF ? 0xFFFFFFFF : 0xFFFFFFFE) param2:0];
    [self sendPTPRequest:PTPRequestCodeNikonDeviceReady];
  }
  else
    [super startExposureWithMirrorLockup:mirrorLockup avoidAF:avoidAF];
}

-(void)stopExposure {
  [self sendPTPRequest:PTPRequestCodeNikonTerminateCapture param1:0 param2:0];
  [self sendPTPRequest:PTPRequestCodeNikonDeviceReady];
}

-(void)focus:(int)steps {
  PTPProperty *afMode = self.info.properties[[NSNumber numberWithUnsignedShort:PTPPropertyCodeNikonLiveViewAFFocus]];
  if (afMode.value.intValue != 0) {
    [self setProperty:PTPPropertyCodeNikonLiveViewAFFocus value:@"0"];
  }
  if (steps >= 0) {
    [self sendPTPRequest:PTPRequestCodeNikonMfDrive param1:1 param2:steps];
  } else {
    [self sendPTPRequest:PTPRequestCodeNikonMfDrive param1:2 param2:-steps];
  }
}

@end
