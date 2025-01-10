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

#include <indigo/indigo_driver.h>
//#include "indigo_timer.h"

#include "indigo_mount_synscan_driver.h"

#define DRIVER_VERSION			0x0012
#define DRIVER_NAME					"indigo_mount_synscan"

#define PRIVATE_DATA        ((synscan_private_data *)device->private_data)

//  Global modes of the mount (only one can be active at any time)
enum GlobalMode {
	kGlobalModeError,
	kGlobalModeIdle,
	kGlobalModeSlewing,
	kGlobalModeParking,
	kGlobalModeGoingHome
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

typedef struct {
	int handle;
	bool udp;
	bool parked;
	bool park_in_progress;
	char tty_name[INDIGO_VALUE_SIZE];
	int timer_count;
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec;

	int device_count;
	pthread_mutex_t port_mutex;
	pthread_mutex_t driver_mutex;
	indigo_property *operating_mode_property;
	indigo_property *mount_polarscope_property;
	indigo_property *use_encoders_property;
	indigo_property *use_ppec_property;
	indigo_property *train_ppec_property;
	indigo_property *autohome_property;
	indigo_property *autohome_settings_property;

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
	long raFeatures;
	long decFeatures;

	//  Home position
	AxisPosition raHomePosition;
	AxisPosition decHomePosition;

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

	//  Global mode of mount
	enum GlobalMode globalMode;

	//  Axis state
	enum AxisMode raAxisMode;
	enum AxisMode decAxisMode;

	//  Abort flag
	bool abort_motion;

	//  Pulse guiding
	pthread_mutex_t ha_mutex;
	pthread_mutex_t dec_mutex;
	pthread_cond_t ha_pulse_cond;
	pthread_cond_t dec_pulse_cond;
	bool guiding_thread_exit;
	int ha_pulse_ms;
	int dec_pulse_ms;

} synscan_private_data;

//-----------------------------------------------
#define MOUNT_POLARSCOPE_PROPERTY							    (PRIVATE_DATA->mount_polarscope_property)
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM					(MOUNT_POLARSCOPE_PROPERTY->items+0)

#define MOUNT_POLARSCOPE_PROPERTY_NAME						"POLARSCOPE"
#define MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME			"BRIGHTNESS"

#define MOUNT_OPERATING_MODE_PROPERTY							(PRIVATE_DATA->operating_mode_property)
#define POLAR_MODE_ITEM                 					(MOUNT_OPERATING_MODE_PROPERTY->items+0)
#define ALTAZ_MODE_ITEM                 					(MOUNT_OPERATING_MODE_PROPERTY->items+1)

#define MOUNT_OPERATING_MODE_PROPERTY_NAME		    "MOUNT_OPERATING_MODE"
#define POLAR_MODE_ITEM_NAME            					"POLAR"
#define ALTAZ_MODE_ITEM_NAME            					"ALTAZ"

#define MOUNT_USE_ENCODERS_PROPERTY								(PRIVATE_DATA->use_encoders_property)
#define MOUNT_USE_RA_ENCODER_ITEM                 (MOUNT_USE_ENCODERS_PROPERTY->items+0)
#define MOUNT_USE_DEC_ENCODER_ITEM                (MOUNT_USE_ENCODERS_PROPERTY->items+1)

#define MOUNT_USE_ENCODERS_PROPERTY_NAME		    	"MOUNT_USE_ENCODERS"
#define MOUNT_USE_RA_ENCODER_ITEM_NAME            "RA"
#define MOUNT_USE_DEC_ENCODER_ITEM_NAME           "DEC"

#define MOUNT_AUTOHOME_PROPERTY										(PRIVATE_DATA->autohome_property)
#define MOUNT_AUTOHOME_ITEM		                		(MOUNT_AUTOHOME_PROPERTY->items+0)

#define MOUNT_AUTOHOME_PROPERTY_NAME		    			"MOUNT_AUTOHOME"
#define MOUNT_AUTOHOME_ITEM_NAME            			"AUTOHOME"

#define MOUNT_AUTOHOME_SETTINGS_PROPERTY					(PRIVATE_DATA->autohome_settings_property)
#define MOUNT_AUTOHOME_DEC_OFFSET_ITEM		        (MOUNT_AUTOHOME_SETTINGS_PROPERTY->items+0)

#define MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME		  "MOUNT_AUTOHOME_SETTINGS"
#define MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME       "DEC_OFFSET"

#endif /* indigo_mount_synscan_private_h */
