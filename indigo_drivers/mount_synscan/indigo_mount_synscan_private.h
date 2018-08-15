//
//  indigo_mount_synscan_private.h
//  indigo
//
//  Created by David Hulse on 07/08/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_mount_synscan_private_h
#define indigo_mount_synscan_private_h

#include <pthread.h>
#include <stdbool.h>
#include "indigo_driver.h"
//#include "indigo_timer.h"
#include "indigo_mount_synscan_driver.h"

#define DRIVER_VERSION			0x0001
#define DRIVER_NAME					"indigo_mount_synscan"

#define PRIVATE_DATA        ((synscan_private_data *)device->private_data)


enum AxisMode {
	kAxisModeIdle = 0,
	kAxisModeTracking = 1,
	kAxisModeGuiding = 2,
	kAxisModeSlewing = 3
};

enum SlewState {
	SLEW_NONE,
	SLEW_START,
	SLEW_STOP,
	SLEW_PHASE0,
	SLEW_PHASE1,
	SLEW_PHASE2,
	SLEW_PHASE3,
	SLEW_PHASE4
};

typedef struct {
	int handle;
	bool parked;
	bool park_in_progress;
	char tty_name[INDIGO_VALUE_SIZE];
	int count_open;
	int slew_rate;
	int st4_ra_rate, st4_dec_rate;
	int vendor_id;
	pthread_mutex_t serial_mutex;
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec, *park_timer, *slew_timer;
	int guide_rate;
	indigo_property *command_guide_rate_property;

	int device_count;
	double currentHAEnc;
	double currentDecEnc;
	double currentHA;
	double currentRA;
	double currentDec;
	pthread_mutex_t port_mutex;
	char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	double lastRA, lastDec;
	char lastUTC[INDIGO_VALUE_SIZE];
	char product[64];
	indigo_property *alignment_mode_property;
	indigo_property *mount_polarscope_property;				///< MOUNT_POLARSCOPE property pointer

	bool mountConfigured;

	//  Mount parameters
	long raTotalSteps;
	long decTotalSteps;
	long raTimerFreq;
	long decTimerFreq;
	long raWormSteps;
	long decWormSteps;
	long raHighSpeedFactor;
	long decHighSpeedFactor;
	bool canSetPolarscopeBrightness;

	//  Zero position
	long raZeroPos;
	long decZeroPos;

	//  Last known position
	double raPosition;
	double decPosition;

	//  Axis config cache
	struct AxisConfig raAxisConfig;
	struct AxisConfig decAxisConfig;

	//  Tracking mode
	enum TrackingMode trackingMode;

	//  Axis slew rates
	int raSlewRate;
	int decSlewRate;

	//  Axis state
	enum AxisMode raAxisMode;
	enum AxisMode decAxisMode;

	//  Slewing data
	enum SlewState slew_state;
	double target_lst;

} synscan_private_data;

//-----------------------------------------------
/** MOUNT_POLARSCOPE property pointer, property is optional, read-write and should be fully controlled by device driver.
 */
#define MOUNT_POLARSCOPE_PROPERTY							    (PRIVATE_DATA->mount_polarscope_property)
#define MOUNT_POLARSCOPE_PROPERTY_NAME						"POLARSCOPE"


/** MOUNT_POLARSCOPE.BRIGHTNESS property item pointer.
 */
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM					(MOUNT_POLARSCOPE_PROPERTY->items+0)
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME			"BRIGHTNESS"


#endif /* indigo_mount_synscan_private_h */
