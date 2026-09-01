//
//  indigo_mount_synscan_guider.h
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_guider_h
#define indigo_mount_synscan_guider_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_guider_driver.h>

indigo_result synscan_guider_connect(indigo_device* device);
void synscan_stop_guider_threads(indigo_device *device);

void guider_timer_callback_ra(indigo_device *device);
void guider_timer_callback_dec(indigo_device *device);

#endif /* indigo_mount_synscan_guider_h */
