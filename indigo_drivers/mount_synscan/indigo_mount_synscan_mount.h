//
//  indigo_mount_synscan_mount.h
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_mount_h
#define indigo_mount_synscan_mount_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_guider_driver.h>

indigo_result synscan_mount_connect(indigo_device* device);

//  Manual slewing
void mount_handle_motion_ra(indigo_device *device);
void mount_handle_motion_dec(indigo_device *device);

//  Tracking
double synscan_tracking_rate_ra(indigo_device* device);
double synscan_tracking_rate_dec(indigo_device* device);
void mount_handle_tracking_rate(indigo_device* device);
void mount_handle_custom_tracking_rate(indigo_device* device);
void mount_handle_tracking(indigo_device *device);

//  Slewing
void mount_handle_coordinates(indigo_device *device);
void mount_handle_aa_coordinates(indigo_device *device);
void mount_handle_park(indigo_device* device);
void mount_handle_home(indigo_device* device);
void mount_handle_abort(indigo_device* device);

//  ST4 Guiding
void mount_handle_st4_guiding_rate(indigo_device *device);

//  Polarscope
void mount_handle_polarscope(indigo_device *device);

// Ext. settings
void mount_handle_encoders(indigo_device *device);
void mount_handle_use_ppec(indigo_device *device);
void mount_handle_train_ppec(indigo_device *device);
void mount_handle_autohome(indigo_device *device);

// RA and DEC rates
extern const double raRates[];
extern const double decRates[];

// Slew rates
extern const int MANUAL_SLEW_RATE_GUIDE;
extern const int MANUAL_SLEW_RATE_CENTERING;
extern const int MANUAL_SLEW_RATE_FIND;
extern const int MANUAL_SLEW_RATE_MAX;

#endif /* indigo_mount_synscan_mount_h */
