//
//  indigo_mount_synscan_driver.h
//  indigo_server
//
//  Created by David Hulse on 07/08/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_driver_h
#define indigo_mount_synscan_driver_h

#include "indigo_mount_synscan_protocol.h"

//  Type of tracking currently active
enum TrackingMode {
	kTrackingModeOff = 0,
	kTrackingModeSidereal = 1,
	kTrackingModeLunar = 2,
	kTrackingModeSolar = 3
};

typedef long AxisPosition;

struct AxisConfig {
	double slewRate;
	enum AxisDirectionID direction;
	bool turbo;
	long rateCode;
	bool valid;
};

bool synscan_configure(indigo_device* device);
bool synscan_configure_axis_for_rate(indigo_device* device, enum AxisID axis, double rate);
bool synscan_slew_axis_at_rate(indigo_device* device, enum AxisID axis, double rate);
void slew_timer_callback(indigo_device *device);
bool synscan_start_tracking_mode(indigo_device* device, enum TrackingMode mode);
void synscan_stop_and_resume_tracking_for_axis(indigo_device* device, enum AxisID axis);
//bool synscan_slew_to_ra_dec(indigo_device* device, double ra, double dec, bool track);
void synscan_get_coords(indigo_device *device);

void coords_encoder_to_eq(indigo_device* device, double ha_enc, double dec_enc, double* ha, double* dec);


//  All rates are arcsecs/sec
extern const double SIDEREAL_RATE;
extern const double LUNAR_RATE;
extern const double SOLAR_RATE;

#endif /* indigo_mount_synscan_driver_h */
