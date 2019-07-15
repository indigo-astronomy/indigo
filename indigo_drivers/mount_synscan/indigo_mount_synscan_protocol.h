//
//  indigo_mount_synscan_protocol.h
//  indigo_server
//
//  Created by David Hulse on 07/08/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_protocol_h
#define indigo_mount_synscan_protocol_h

#include <stdbool.h>

enum AxisID {
	kAxisRA = '1',
	kAxisDEC = '2'
};

enum AxisDirectionID {
	kAxisDirectionFwd = 0,
	kAxisDirectionRev = 1
};

enum AxisSpeedID {
	kAxisSpeedHigh = '3',
	kAxisSpeedLow = '1',
	kAxisSpeedAbsSlew = '0'
};

enum MotorStatus {
	kStatusNotInitialised = 0x100,
	kStatusIdleForward = 0x101,
	kStatusActiveForward = 0x111,
	kStatusActiveBackward = 0x311,
	kStatusIdleBackward = 0x301,
	kStatusIdleTurboForward = 0x501,
	kStatusActiveTurboForward = 0x511,
	kStatusIdleTurboBackward = 0x701,
	kStatusActiveTurboBackward = 0x711,
	kStatusActiveMask = 0x010
	//=      0x1 == slewing, 0x0 == slewingTo, 0x2==slewing back, 0x4==turbo       0 == stopped, 1 == moving           0==not init, 1==init ok
};

enum GuideRate {
	kGuideRate_x1_00 = 0,
	kGuideRate_x0_75 = 1,
	kGuideRate_x0_50 = 2,
	kGuideRate_x0_25 = 3,
	kGuideRate_x0_125 = 4
};

enum ExtInquiry {
	kGetHomeIndex = 0x0000,
	kGetFeatures = 0x0001
};

enum ExtSetting {
	kStarPECCTtraining = 0x0000,
	kStopPECCTtraining = 0x0001,
	kTurnPECCOn = 0x0002,
	kTurnPECCOff = 0X0003,
	kTurnEncoderOn = 0x0004,
	kTurnEncoderOff = 0x0005,
	kDisableFullCurrentLowSpeed = 0x0006,
	kEnableFullCurrentLowSpeed = 0x0106,
	kResetHomeIndexer = 0x0008
};

enum Features {
	kHasEncoder = 0x0001,
	kHasPPEC = 0x0002,
	kHasHomeIndexer = 0x0004,
	kIsAZEQ = 0x0008,
	kInPPECTraining = 0x0010,
	kInPPEC = 0x0020,
	kHasPolarLED = 0x1000,
	kHasCommonSlewStart = 0x2000,
	kHasHalfCurrentTracking = 0x4000
};

bool synscan_firmware_version(indigo_device* device, long* v);
bool synscan_total_axis_steps(indigo_device* device, enum AxisID axis, long* v);
bool synscan_worm_rotation_steps(indigo_device* device, enum AxisID axis, long* v);
bool synscan_step_timer_frequency(indigo_device* device, enum AxisID axis, long* v);
bool synscan_high_speed_ratio(indigo_device* device, enum AxisID axis, long* v);
bool synscan_motor_status(indigo_device* device, enum AxisID axis, long* v);
bool synscan_axis_position(indigo_device* device, enum AxisID axis, long* v);
bool synscan_init_axis_position(indigo_device* device, enum AxisID axis, long pos);
bool synscan_init_axis(indigo_device* device, enum AxisID axis);
bool synscan_stop_axis(indigo_device* device, enum AxisID axis);
bool synscan_instant_stop_axis(indigo_device* device, enum AxisID axis);
bool synscan_set_axis_gearing(indigo_device* device, enum AxisID axis, enum AxisDirectionID d, enum AxisSpeedID g);
bool synscan_set_axis_step_count(indigo_device* device, enum AxisID axis, long s);
bool synscan_set_axis_slew_rate(indigo_device* device, enum AxisID axis, long r);
bool synscan_slew_axis(indigo_device* device, enum AxisID axis);
bool synscan_set_axis_slowdown(indigo_device* device, enum AxisID axis, long s);
bool synscan_set_polarscope_brightness(indigo_device* device, unsigned char brightness);
bool synscan_set_st4_guide_rate(indigo_device* device, enum AxisID axis, enum GuideRate rate);
bool synscan_guide_pulse_ra(indigo_device* device, long guide_rate, int duration_ms, long track_rate);
bool synscan_guide_pulse_dec(indigo_device* device, enum AxisDirectionID direction, long guide_rate, int duration_ms);
bool synscan_set_goto_target(indigo_device* device, enum AxisID axis, long target);
bool synscan_ext_inquiry(indigo_device* device, enum AxisID axis, enum ExtInquiry inquiry, long *v);
bool synscan_ext_setting(indigo_device* device, enum AxisID axis, enum ExtSetting setting);

#endif /* indigo_mount_synscan_protocol_h */
