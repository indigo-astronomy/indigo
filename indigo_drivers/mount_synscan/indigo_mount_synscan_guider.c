//
//  indigo_mount_synscan_guider.c
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <sys/time.h>
#include <unistd.h>
#include "indigo_mount_driver.h"
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_driver.h"
#include "indigo_mount_synscan_guider.h"
#include "indigo_mount_synscan_mount.h"


static void guider_delay_ms(int millis) {
	//  Get current time
	struct timeval now;
	gettimeofday(&now, NULL);

	//  Convert to milliseconds and compute stop time
	uint64_t tnow = (now.tv_sec * 1000) + (now.tv_usec / 1000);
	uint64_t tend = tnow + millis;

	//  Wait until current time reaches TEND
	while (true) {
		//  Get current time
		gettimeofday(&now, NULL);
		tnow = (now.tv_sec * 1000) + (now.tv_usec / 1000);

		//  This is not ideal as we are busy-waiting here and just consuming CPU cycles doing nothing,
		//  but if we actually sleep, then the OS might not give control back for much longer than we
		//  wanted to sleep for.
		
		//  Return if we've reached the end time
		if (tnow >= tend)
			return;
	}
}

void guider_timer_callback_ra(indigo_device *device) {
	while (true) {
		//  Wait for pulse or exit
		int pulse_length_ms;
		pthread_mutex_lock(&PRIVATE_DATA->ha_mutex);
		while (!PRIVATE_DATA->guiding_thread_exit && PRIVATE_DATA->ha_pulse_ms == 0)
			pthread_cond_wait(&PRIVATE_DATA->ha_pulse_cond, &PRIVATE_DATA->ha_mutex);
		pulse_length_ms = PRIVATE_DATA->ha_pulse_ms;
		PRIVATE_DATA->ha_pulse_ms = 0;
		pthread_mutex_unlock(&PRIVATE_DATA->ha_mutex);

		//  Exit if requested
		if (PRIVATE_DATA->guiding_thread_exit)
			return;

		//  Determine the rate and duration
		double guideRate = synscan_tracking_rate(device->master_device);
		if (pulse_length_ms < 0) {
			guideRate -= GUIDER_RATE_ITEM->number.value * guideRate / 100.0;
			pulse_length_ms = -pulse_length_ms;
		}
		else {
			guideRate += GUIDER_RATE_ITEM->number.value * guideRate / 100.0;
		}

#if 0
		//  Slew the axis at the specified rate (tracking rate +/- % of tracking rate)
		bool result;
		synscan_update_axis_to_rate(device->master_device, kAxisRA, guideRate, &result);
		PRIVATE_DATA->raAxisMode = kAxisModeGuiding;

		//  Wait for the required duration
		guider_delay_ms(pulse_length_ms);
#endif

		//  Resume tracking
		double axisRate = synscan_tracking_rate(device->master_device);
#if 0
		synscan_update_axis_to_rate(device->master_device, kAxisRA, axisRate, &result);
		PRIVATE_DATA->raAxisMode = kAxisModeTracking;
#endif
		synscan_guide_axis_at_rate(device->master_device, kAxisRA, guideRate, pulse_length_ms, axisRate);
		
		//  Complete the guiding property updates
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

void guider_timer_callback_dec(indigo_device *device) {
	while (true) {
		//  Wait for pulse or exit
		int pulse_length_ms;
		pthread_mutex_lock(&PRIVATE_DATA->dec_mutex);
		while (!PRIVATE_DATA->guiding_thread_exit && PRIVATE_DATA->dec_pulse_ms == 0)
			pthread_cond_wait(&PRIVATE_DATA->dec_pulse_cond, &PRIVATE_DATA->dec_mutex);
		pulse_length_ms = PRIVATE_DATA->dec_pulse_ms;
		PRIVATE_DATA->dec_pulse_ms = 0;
		pthread_mutex_unlock(&PRIVATE_DATA->dec_mutex);
		
		//  Exit if requested
		if (PRIVATE_DATA->guiding_thread_exit)
			return;
		
		//  Determine the rate and duration
		double guideRate = synscan_tracking_rate(device->master_device);
		if (pulse_length_ms < 0) {
			guideRate = -GUIDER_DEC_RATE_ITEM->number.value * guideRate / 100.0;
			pulse_length_ms = -pulse_length_ms;
		}
		else {
			guideRate = GUIDER_DEC_RATE_ITEM->number.value * guideRate / 100.0;
		}

#if 0
		//  Slew the axis at the specified rate (+/- % of tracking rate)
		synscan_slew_axis_at_rate(device->master_device, kAxisDEC, guideRate);
		PRIVATE_DATA->decAxisMode = kAxisModeGuiding;

		//  Wait for the required duration
		//guider_delay_ms(pulse_length_ms);
		
		//  Stop the slew
		synscan_stop_axis(device->master_device, kAxisDEC);
		synscan_wait_for_axis_stopped(device->master_device, kAxisDEC, NULL);
		PRIVATE_DATA->decAxisMode = kAxisModeIdle;
#endif
		synscan_guide_axis_at_rate(device->master_device, kAxisDEC, guideRate, pulse_length_ms, 0);
		
		//  Complete the guiding property updates
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
}
