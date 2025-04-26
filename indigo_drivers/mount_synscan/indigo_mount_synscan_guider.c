//
//  indigo_mount_synscan_guider.c
//  indigo
//
//  Created by David Hulse on 24/12/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <indigo/indigo_mount_driver.h>
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_driver.h"
#include "indigo_mount_synscan_guider.h"
#include "indigo_mount_synscan_mount.h"


//static void guider_delay_ms(int millis) {
//	//  Get current time
//	struct timeval now;
//	gettimeofday(&now, NULL);
//
//	//  Convert to milliseconds and compute stop time
//	uint64_t tnow = (now.tv_sec * 1000) + (now.tv_usec / 1000);
//	uint64_t tend = tnow + millis;
//
//	//  Wait until current time reaches TEND
//	while (true) {
//		//  Get current time
//		gettimeofday(&now, NULL);
//		tnow = (now.tv_sec * 1000) + (now.tv_usec / 1000);
//
//		//  This is not ideal as we are busy-waiting here and just consuming CPU cycles doing nothing,
//		//  but if we actually sleep, then the OS might not give control back for much longer than we
//		//  wanted to sleep for.
//		
//		//  Return if we've reached the end time
//		if (tnow >= tend)
//			return;
//	}
//}

void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->timer_count++;
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
		if (PRIVATE_DATA->guiding_thread_exit) {
			PRIVATE_DATA->timer_count--;
			return;
		}

		//  Determine the rate and duration
		double guideRate = synscan_tracking_rate(device->master_device);
		if (pulse_length_ms < 0) {
			guideRate -= GUIDER_RATE_ITEM->number.value * guideRate / 100.0;
			pulse_length_ms = -pulse_length_ms;
		} else {
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
	PRIVATE_DATA->timer_count--;
}

void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->timer_count++;
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
		if (PRIVATE_DATA->guiding_thread_exit) {
			PRIVATE_DATA->timer_count--;
			return;
		}

		//  Determine the rate and duration
		double guideRate = synscan_tracking_rate(device->master_device);
		if (pulse_length_ms < 0) {
			guideRate = -GUIDER_DEC_RATE_ITEM->number.value * guideRate / 100.0;
			pulse_length_ms = -pulse_length_ms;
		} else {
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
	PRIVATE_DATA->timer_count--;
}

static void synscan_connect_timer_callback(indigo_device* device) {
	//  Lock the driver
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	//  Open and configure the mount
	bool result = true;
	if (PRIVATE_DATA->device_count == 0) {
		result = synscan_open(device->master_device);
		if (result) {
			result = synscan_configure(device->master_device);
			if (!result && !PRIVATE_DATA->udp) {
				synscan_close(device);
				if (strcmp(DEVICE_BAUDRATE_ITEM->text.value, "9600-8N1")) {
					strcpy(DEVICE_BAUDRATE_ITEM->text.value, "9600-8N1");
				} else {
					strcpy(DEVICE_BAUDRATE_ITEM->text.value, "115200-8N1");
				}
				indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, "Trying to change baudrate");
				result = synscan_open(device->master_device);
				if (result) {
					result = synscan_configure(device->master_device);
				}
			}
		}
	}
	if (result) {
		PRIVATE_DATA->device_count++;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
		//  Start RA/DEC timer threads
		indigo_set_timer(device, 0, &guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
		indigo_set_timer(device, 0, &guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
	} else {
		synscan_close(device->master_device);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, "Failed to connect to mount");
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	//  Unlock the driver
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

indigo_result synscan_guider_connect(indigo_device* device) {
	//  Handle connect/disconnect commands
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//  CONNECT to the mount
		indigo_set_timer(device, 0, &synscan_connect_timer_callback, NULL);
		return INDIGO_OK;
	} else if (CONNECTION_DISCONNECTED_ITEM->sw.value) {
		//  DISCONNECT from mount
		PRIVATE_DATA->guiding_thread_exit = false;
		if (--PRIVATE_DATA->device_count == 0) {
			synscan_close(device);
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	return indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}
