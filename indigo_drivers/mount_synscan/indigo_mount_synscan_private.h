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

//  Global modes of the mount (only one can be active at any time)
enum GlobalMode {
	kGlobalModeError,
	kGlobalModeParked,
	kGlobalModeIdle,
	kGlobalModeSlewing,
	kGlobalModeParking
};

enum AxisMode {
	kAxisModeIdle,
	kAxisModeStopping,
	kAxisModeTracking,
	kAxisModeGuiding,
	kAxisModeManualSlewing,
	kAxisModeSlewing,
	kAxisModeSlewIdle
};

enum SlewState {
	SLEW_NONE,
	SLEW_PHASE0,
	SLEW_PHASE1,
	SLEW_PHASE2,
	SLEW_PHASE3,
	SLEW_PHASE4
};

enum ParkState {
	PARK_NONE,
	PARK_PHASE0,
	PARK_PHASE1,
	PARK_PHASE2
};

typedef struct {
	int handle;
	bool parked;
	bool park_in_progress;
	char tty_name[INDIGO_VALUE_SIZE];
	//int count_open;
	//int slew_rate;
	//int st4_ra_rate, st4_dec_rate;
	//int vendor_id;
	//pthread_mutex_t serial_mutex;
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec, *park_timer, *slew_timer, *ha_axis_timer, *dec_axis_timer;
	//int guide_rate;
	//indigo_property *command_guide_rate_property;

	int device_count;
	double currentHA;		//  radians
	double currentRA;		//  radians
	double currentDec;	//  radians
	pthread_mutex_t port_mutex;
	//char lastMotionNS, lastMotionWE, lastSlewRate, lastTrackRate;
	//double lastRA, lastDec;
	//char lastUTC[INDIGO_VALUE_SIZE];
	//char product[64];
	indigo_property *operating_mode_property;
	indigo_property *mount_polarscope_property;

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

	//  Last known position (encoders)
	double raPosition;
	double decPosition;
	double raTargetPosition;
	double decTargetPosition;

	//  Axis config cache
	struct AxisConfig raAxisConfig;
	struct AxisConfig decAxisConfig;

	//  Tracking mode
	//enum TrackingMode trackingMode;

	//  Axis slew rates
	//int raSlewRate;
	//int decSlewRate;

	//  Global mode of mount
	enum GlobalMode globalMode;

	//  Axis state
	enum AxisMode raAxisMode;
	enum AxisMode decAxisMode;
	enum AxisMode raDesiredAxisMode;
	enum AxisMode decDesiredAxisMode;
	double raDesiredRate;
	double decDesiredRate;

	//  Slewing data
	enum SlewState slew_state;
	double target_lst;

	//  Parking state
	enum ParkState park_state;

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

/** OPERATING_MODE property pointer, property is optional, read-write and should be fully controlled by device driver.
 */
#define OPERATING_MODE_PROPERTY					(PRIVATE_DATA->operating_mode_property)
#define POLAR_MODE_ITEM                 (OPERATING_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM                 (OPERATING_MODE_PROPERTY->items+1)

#define OPERATING_MODE_PROPERTY_NAME		"OPERATING_MODE"
#define POLAR_MODE_ITEM_NAME            "POLAR"
#define ALTAZ_MODE_ITEM_NAME            "ALTAZ"

#endif /* indigo_mount_synscan_private_h */
