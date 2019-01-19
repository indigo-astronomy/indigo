//
//  indigo_mount_synscan_mount.h
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_mount_h
#define indigo_mount_synscan_mount_h

#include "indigo_driver.h"
#include "indigo_mount_driver.h"

void synscan_mount_connect(indigo_device* device);

//  Manual slewing
void mount_handle_motion_ra(indigo_device *device);
void mount_handle_motion_dec(indigo_device *device);

//  Tracking
double synscan_tracking_rate(indigo_device* device);
void mount_handle_tracking_rate(indigo_device* device);
void mount_handle_tracking(indigo_device *device);

//  Slewing
void mount_handle_coordinates(indigo_device *device);
void mount_handle_aa_coordinates(indigo_device *device);
void mount_handle_park(indigo_device* device);
void mount_handle_abort(indigo_device* device);

//  ST4 Guiding
void mount_handle_st4_guiding_rate(indigo_device *device);

//  Polarscope
void mount_handle_polarscope(indigo_device *device);

#endif /* indigo_mount_synscan_mount_h */
