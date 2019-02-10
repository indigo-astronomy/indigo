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

#endif /* indigo_mount_synscan_protocol_h */
