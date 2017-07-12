//
//  indigo_ica_nikon.m
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

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
    [s appendFormat:@"\nOperations:\n"];
    for (NSNumber *code in self.operationsSupported)
      [s appendFormat:@"%@\n", [PTPNikonRequest operationCodeName:code.intValue]];
  }
  if (self.eventsSupported.count > 0) {
    [s appendFormat:@"\nEvents:\n"];
    for (NSNumber *code in self.eventsSupported)
      [s appendFormat:@"%@\n", [PTPNikonEvent eventCodeName:code.intValue]];
  }
  if (self.propertiesSupported.count > 0) {
    [s appendFormat:@"\nProperties:\n"];
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

@implementation PTPNikonCamera

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate {
  self = [super initWithICCamera:icCamera delegate:delegate];
  if (self) {

  }
  return self;
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

@end
