//
//  indigo_mount_synscan_guider.c
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <unistd.h>
#include "indigo_mount_driver.h"
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_driver.h"
#include "indigo_mount_synscan_guider.h"
#include "indigo_mount_synscan_mount.h"


void guider_timer_callback_ra(indigo_device *device) {
#if 0
	//  Determine the rate and duration
	double guideRate = synscan_tracking_rate(device);
	int duration = 0;
	if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		guideRate -= GUIDER_RATE_ITEM->number.value * guideRate / 100.0;
		duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	}
	else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		guideRate += GUIDER_RATE_ITEM->number.value * guideRate / 100.0;
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
	}

	//  Slew the axis at the specified rate (tracking rate +/- % of tracking rate)
	synscan_slew_axis_at_rate(device, kAxisRA, guideRate);

	//  Wait for the required duration
	usleep(duration * 1000);

	//  Resume tracking if tracking is currently on
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		bool result;
		double axisRate = synscan_tracking_rate(device);
		synscan_update_axis_to_rate(device, kAxisRA, axisRate, &result);
		PRIVATE_DATA->raAxisMode = kAxisModeTracking;
	}
	else {
		//  Just stop the slew if not tracking - guiding cant really work like this
		synscan_stop_axis(device, kAxisRA);
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	}
#endif
	//  Complete the guiding property updates
	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

void guider_timer_callback_dec(indigo_device *device) {
#if 0
	PRIVATE_DATA->guider_timer_dec = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;
	
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
	}
#endif

	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}
