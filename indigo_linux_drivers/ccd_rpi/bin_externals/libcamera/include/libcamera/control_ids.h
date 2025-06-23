/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * Controls ID list
 *
 * This file is auto-generated. Do not edit.
 */

#pragma once

#include <array>
#include <map>
#include <stdint.h>
#include <string>

#include <libcamera/controls.h>

namespace libcamera {

namespace controls {

extern const ControlIdMap controls;


enum {
	AE_ENABLE = 1,
	AE_STATE = 2,
	AE_METERING_MODE = 3,
	AE_CONSTRAINT_MODE = 4,
	AE_EXPOSURE_MODE = 5,
	EXPOSURE_VALUE = 6,
	EXPOSURE_TIME = 7,
	EXPOSURE_TIME_MODE = 8,
	ANALOGUE_GAIN = 9,
	ANALOGUE_GAIN_MODE = 10,
	AE_FLICKER_MODE = 11,
	AE_FLICKER_PERIOD = 12,
	AE_FLICKER_DETECTED = 13,
	BRIGHTNESS = 14,
	CONTRAST = 15,
	LUX = 16,
	AWB_ENABLE = 17,
	AWB_MODE = 18,
	AWB_LOCKED = 19,
	COLOUR_GAINS = 20,
	COLOUR_TEMPERATURE = 21,
	SATURATION = 22,
	SENSOR_BLACK_LEVELS = 23,
	SHARPNESS = 24,
	FOCUS_FO_M = 25,
	COLOUR_CORRECTION_MATRIX = 26,
	SCALER_CROP = 27,
	DIGITAL_GAIN = 28,
	FRAME_DURATION = 29,
	FRAME_DURATION_LIMITS = 30,
	SENSOR_TEMPERATURE = 31,
	SENSOR_TIMESTAMP = 32,
	AF_MODE = 33,
	AF_RANGE = 34,
	AF_SPEED = 35,
	AF_METERING = 36,
	AF_WINDOWS = 37,
	AF_TRIGGER = 38,
	AF_PAUSE = 39,
	LENS_POSITION = 40,
	AF_STATE = 41,
	AF_PAUSE_STATE = 42,
	HDR_MODE = 43,
	HDR_CHANNEL = 44,
	GAMMA = 45,
	DEBUG_METADATA_ENABLE = 46,
	FRAME_WALL_CLOCK = 47,
};


extern const Control<bool> AeEnable;
enum AeStateEnum {
	AeStateIdle = 0,
	AeStateSearching = 1,
	AeStateConverged = 2,
};
extern const std::array<const ControlValue, 3> AeStateValues;
extern const std::map<std::string, int32_t> AeStateNameValueMap;
extern const Control<int32_t> AeState;
enum AeMeteringModeEnum {
	MeteringCentreWeighted = 0,
	MeteringSpot = 1,
	MeteringMatrix = 2,
	MeteringCustom = 3,
};
extern const std::array<const ControlValue, 4> AeMeteringModeValues;
extern const std::map<std::string, int32_t> AeMeteringModeNameValueMap;
extern const Control<int32_t> AeMeteringMode;
enum AeConstraintModeEnum {
	ConstraintNormal = 0,
	ConstraintHighlight = 1,
	ConstraintShadows = 2,
	ConstraintCustom = 3,
};
extern const std::array<const ControlValue, 4> AeConstraintModeValues;
extern const std::map<std::string, int32_t> AeConstraintModeNameValueMap;
extern const Control<int32_t> AeConstraintMode;
enum AeExposureModeEnum {
	ExposureNormal = 0,
	ExposureShort = 1,
	ExposureLong = 2,
	ExposureCustom = 3,
};
extern const std::array<const ControlValue, 4> AeExposureModeValues;
extern const std::map<std::string, int32_t> AeExposureModeNameValueMap;
extern const Control<int32_t> AeExposureMode;
extern const Control<float> ExposureValue;
extern const Control<int32_t> ExposureTime;
enum ExposureTimeModeEnum {
	ExposureTimeModeAuto = 0,
	ExposureTimeModeManual = 1,
};
extern const std::array<const ControlValue, 2> ExposureTimeModeValues;
extern const std::map<std::string, int32_t> ExposureTimeModeNameValueMap;
extern const Control<int32_t> ExposureTimeMode;
extern const Control<float> AnalogueGain;
enum AnalogueGainModeEnum {
	AnalogueGainModeAuto = 0,
	AnalogueGainModeManual = 1,
};
extern const std::array<const ControlValue, 2> AnalogueGainModeValues;
extern const std::map<std::string, int32_t> AnalogueGainModeNameValueMap;
extern const Control<int32_t> AnalogueGainMode;
enum AeFlickerModeEnum {
	FlickerOff = 0,
	FlickerManual = 1,
	FlickerAuto = 2,
};
extern const std::array<const ControlValue, 3> AeFlickerModeValues;
extern const std::map<std::string, int32_t> AeFlickerModeNameValueMap;
extern const Control<int32_t> AeFlickerMode;
extern const Control<int32_t> AeFlickerPeriod;
extern const Control<int32_t> AeFlickerDetected;
extern const Control<float> Brightness;
extern const Control<float> Contrast;
extern const Control<float> Lux;
extern const Control<bool> AwbEnable;
enum AwbModeEnum {
	AwbAuto = 0,
	AwbIncandescent = 1,
	AwbTungsten = 2,
	AwbFluorescent = 3,
	AwbIndoor = 4,
	AwbDaylight = 5,
	AwbCloudy = 6,
	AwbCustom = 7,
};
extern const std::array<const ControlValue, 8> AwbModeValues;
extern const std::map<std::string, int32_t> AwbModeNameValueMap;
extern const Control<int32_t> AwbMode;
extern const Control<bool> AwbLocked;
extern const Control<Span<const float, 2>> ColourGains;
extern const Control<int32_t> ColourTemperature;
extern const Control<float> Saturation;
extern const Control<Span<const int32_t, 4>> SensorBlackLevels;
extern const Control<float> Sharpness;
extern const Control<int32_t> FocusFoM;
extern const Control<Span<const float, 9>> ColourCorrectionMatrix;
extern const Control<Rectangle> ScalerCrop;
extern const Control<float> DigitalGain;
extern const Control<int64_t> FrameDuration;
extern const Control<Span<const int64_t, 2>> FrameDurationLimits;
extern const Control<float> SensorTemperature;
extern const Control<int64_t> SensorTimestamp;
enum AfModeEnum {
	AfModeManual = 0,
	AfModeAuto = 1,
	AfModeContinuous = 2,
};
extern const std::array<const ControlValue, 3> AfModeValues;
extern const std::map<std::string, int32_t> AfModeNameValueMap;
extern const Control<int32_t> AfMode;
enum AfRangeEnum {
	AfRangeNormal = 0,
	AfRangeMacro = 1,
	AfRangeFull = 2,
};
extern const std::array<const ControlValue, 3> AfRangeValues;
extern const std::map<std::string, int32_t> AfRangeNameValueMap;
extern const Control<int32_t> AfRange;
enum AfSpeedEnum {
	AfSpeedNormal = 0,
	AfSpeedFast = 1,
};
extern const std::array<const ControlValue, 2> AfSpeedValues;
extern const std::map<std::string, int32_t> AfSpeedNameValueMap;
extern const Control<int32_t> AfSpeed;
enum AfMeteringEnum {
	AfMeteringAuto = 0,
	AfMeteringWindows = 1,
};
extern const std::array<const ControlValue, 2> AfMeteringValues;
extern const std::map<std::string, int32_t> AfMeteringNameValueMap;
extern const Control<int32_t> AfMetering;
extern const Control<Span<const Rectangle>> AfWindows;
enum AfTriggerEnum {
	AfTriggerStart = 0,
	AfTriggerCancel = 1,
};
extern const std::array<const ControlValue, 2> AfTriggerValues;
extern const std::map<std::string, int32_t> AfTriggerNameValueMap;
extern const Control<int32_t> AfTrigger;
enum AfPauseEnum {
	AfPauseImmediate = 0,
	AfPauseDeferred = 1,
	AfPauseResume = 2,
};
extern const std::array<const ControlValue, 3> AfPauseValues;
extern const std::map<std::string, int32_t> AfPauseNameValueMap;
extern const Control<int32_t> AfPause;
extern const Control<float> LensPosition;
enum AfStateEnum {
	AfStateIdle = 0,
	AfStateScanning = 1,
	AfStateFocused = 2,
	AfStateFailed = 3,
};
extern const std::array<const ControlValue, 4> AfStateValues;
extern const std::map<std::string, int32_t> AfStateNameValueMap;
extern const Control<int32_t> AfState;
enum AfPauseStateEnum {
	AfPauseStateRunning = 0,
	AfPauseStatePausing = 1,
	AfPauseStatePaused = 2,
};
extern const std::array<const ControlValue, 3> AfPauseStateValues;
extern const std::map<std::string, int32_t> AfPauseStateNameValueMap;
extern const Control<int32_t> AfPauseState;
enum HdrModeEnum {
	HdrModeOff = 0,
	HdrModeMultiExposureUnmerged = 1,
	HdrModeMultiExposure = 2,
	HdrModeSingleExposure = 3,
	HdrModeNight = 4,
};
extern const std::array<const ControlValue, 5> HdrModeValues;
extern const std::map<std::string, int32_t> HdrModeNameValueMap;
extern const Control<int32_t> HdrMode;
enum HdrChannelEnum {
	HdrChannelNone = 0,
	HdrChannelShort = 1,
	HdrChannelMedium = 2,
	HdrChannelLong = 3,
};
extern const std::array<const ControlValue, 4> HdrChannelValues;
extern const std::map<std::string, int32_t> HdrChannelNameValueMap;
extern const Control<int32_t> HdrChannel;
extern const Control<float> Gamma;
extern const Control<bool> DebugMetadataEnable;
extern const Control<int64_t> FrameWallClock;

namespace draft {

#define LIBCAMERA_HAS_DRAFT_VENDOR_CONTROLS


enum {
	AE_PRECAPTURE_TRIGGER = 10001,
	NOISE_REDUCTION_MODE = 10002,
	COLOR_CORRECTION_ABERRATION_MODE = 10003,
	AWB_STATE = 10004,
	SENSOR_ROLLING_SHUTTER_SKEW = 10005,
	LENS_SHADING_MAP_MODE = 10006,
	PIPELINE_DEPTH = 10007,
	MAX_LATENCY = 10008,
	TEST_PATTERN_MODE = 10009,
	FACE_DETECT_MODE = 10010,
	FACE_DETECT_FACE_RECTANGLES = 10011,
	FACE_DETECT_FACE_SCORES = 10012,
	FACE_DETECT_FACE_LANDMARKS = 10013,
	FACE_DETECT_FACE_IDS = 10014,
};


enum AePrecaptureTriggerEnum {
	AePrecaptureTriggerIdle = 0,
	AePrecaptureTriggerStart = 1,
	AePrecaptureTriggerCancel = 2,
};
extern const std::array<const ControlValue, 3> AePrecaptureTriggerValues;
extern const std::map<std::string, int32_t> AePrecaptureTriggerNameValueMap;
extern const Control<int32_t> AePrecaptureTrigger;
enum NoiseReductionModeEnum {
	NoiseReductionModeOff = 0,
	NoiseReductionModeFast = 1,
	NoiseReductionModeHighQuality = 2,
	NoiseReductionModeMinimal = 3,
	NoiseReductionModeZSL = 4,
};
extern const std::array<const ControlValue, 5> NoiseReductionModeValues;
extern const std::map<std::string, int32_t> NoiseReductionModeNameValueMap;
extern const Control<int32_t> NoiseReductionMode;
enum ColorCorrectionAberrationModeEnum {
	ColorCorrectionAberrationOff = 0,
	ColorCorrectionAberrationFast = 1,
	ColorCorrectionAberrationHighQuality = 2,
};
extern const std::array<const ControlValue, 3> ColorCorrectionAberrationModeValues;
extern const std::map<std::string, int32_t> ColorCorrectionAberrationModeNameValueMap;
extern const Control<int32_t> ColorCorrectionAberrationMode;
enum AwbStateEnum {
	AwbStateInactive = 0,
	AwbStateSearching = 1,
	AwbConverged = 2,
	AwbLocked = 3,
};
extern const std::array<const ControlValue, 4> AwbStateValues;
extern const std::map<std::string, int32_t> AwbStateNameValueMap;
extern const Control<int32_t> AwbState;
extern const Control<int64_t> SensorRollingShutterSkew;
enum LensShadingMapModeEnum {
	LensShadingMapModeOff = 0,
	LensShadingMapModeOn = 1,
};
extern const std::array<const ControlValue, 2> LensShadingMapModeValues;
extern const std::map<std::string, int32_t> LensShadingMapModeNameValueMap;
extern const Control<int32_t> LensShadingMapMode;
extern const Control<int32_t> PipelineDepth;
extern const Control<int32_t> MaxLatency;
enum TestPatternModeEnum {
	TestPatternModeOff = 0,
	TestPatternModeSolidColor = 1,
	TestPatternModeColorBars = 2,
	TestPatternModeColorBarsFadeToGray = 3,
	TestPatternModePn9 = 4,
	TestPatternModeCustom1 = 256,
};
extern const std::array<const ControlValue, 6> TestPatternModeValues;
extern const std::map<std::string, int32_t> TestPatternModeNameValueMap;
extern const Control<int32_t> TestPatternMode;
enum FaceDetectModeEnum {
	FaceDetectModeOff = 0,
	FaceDetectModeSimple = 1,
	FaceDetectModeFull = 2,
};
extern const std::array<const ControlValue, 3> FaceDetectModeValues;
extern const std::map<std::string, int32_t> FaceDetectModeNameValueMap;
extern const Control<int32_t> FaceDetectMode;
extern const Control<Span<const Rectangle>> FaceDetectFaceRectangles;
extern const Control<Span<const uint8_t>> FaceDetectFaceScores;
extern const Control<Span<const Point>> FaceDetectFaceLandmarks;
extern const Control<Span<const int32_t>> FaceDetectFaceIds;

} /* namespace draft */

namespace rpi {

#define LIBCAMERA_HAS_RPI_VENDOR_CONTROLS


enum {
	STATS_OUTPUT_ENABLE = 20001,
	BCM2835_STATS_OUTPUT = 20002,
	SCALER_CROPS = 20003,
	PISP_STATS_OUTPUT = 20004,
	CNN_OUTPUT_TENSOR = 20005,
	CNN_OUTPUT_TENSOR_INFO = 20006,
	CNN_ENABLE_INPUT_TENSOR = 20007,
	CNN_INPUT_TENSOR = 20008,
	CNN_INPUT_TENSOR_INFO = 20009,
	CNN_KPI_INFO = 20010,
	SYNC_MODE = 20011,
	SYNC_READY = 20012,
	SYNC_TIMER = 20013,
	SYNC_FRAMES = 20014,
};


extern const Control<bool> StatsOutputEnable;
extern const Control<Span<const uint8_t>> Bcm2835StatsOutput;
extern const Control<Span<const Rectangle>> ScalerCrops;
extern const Control<Span<const uint8_t>> PispStatsOutput;
extern const Control<Span<const float>> CnnOutputTensor;
extern const Control<Span<const uint8_t>> CnnOutputTensorInfo;
extern const Control<bool> CnnEnableInputTensor;
extern const Control<Span<const uint8_t>> CnnInputTensor;
extern const Control<Span<const uint8_t>> CnnInputTensorInfo;
extern const Control<Span<const int32_t, 2>> CnnKpiInfo;
enum SyncModeEnum {
	SyncModeOff = 0,
	SyncModeServer = 1,
	SyncModeClient = 2,
};
extern const std::array<const ControlValue, 3> SyncModeValues;
extern const std::map<std::string, int32_t> SyncModeNameValueMap;
extern const Control<int32_t> SyncMode;
extern const Control<bool> SyncReady;
extern const Control<int64_t> SyncTimer;
extern const Control<int32_t> SyncFrames;

} /* namespace rpi */

namespace debug {

#define LIBCAMERA_HAS_DEBUG_VENDOR_CONTROLS




} /* namespace debug */

} /* namespace controls */

} /* namespace libcamera */