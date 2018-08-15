//
//  indigo_mount_synscan_driver.c
//  indigo_server
//
//  Created by David Hulse on 07/08/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "indigo_driver.h"
#include "indigo_novas.h"
#include "indigo_mount_driver.h"
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_protocol.h"
#include "indigo_mount_synscan_driver.h"


//  All rates are arcsecs/sec
const double SIDEREAL_RATE = (360.0 * 3600.0) / 86164.090530833;
const double LUNAR_RATE = 14.511415;
const double SOLAR_RATE = 15.0;


//  Home position (arbitrary constant) is defined as:
//      HA == 6 (vertical shaft)
//      -/+90 degrees (pointing at pole)
static const AxisPosition RA_HOME_POSITION = 0x800000;
static const AxisPosition DEC_HOME_POSITION = 0x800000;


#define GET_RELEASE(v)				(int)(((v) >> 16) & 0xFF)
#define GET_REVISION(v)				(int)(((v) >> 8) & 0xFF)
#define GET_PATCH(v)          (int)((v) & 0xFF)


bool synscan_configure(indigo_device* device) {
	//  Start with an assumption about each axis
	PRIVATE_DATA->raAxisMode = PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Get firmware version
	long version;
	if (!synscan_firmware_version(device, &version)) {
		printf("ERROR GETTING FIRMWARE\n");
		return false;
	}
	snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d.%02d", GET_RELEASE(version), GET_REVISION(version), GET_PATCH(version));

	//  Query motor status
	long raMotorStatus = 0;
	long decMotorStatus = 0;
	if (!synscan_motor_status(device, kAxisRA, &raMotorStatus)) {
		printf("ERROR GETTING RA STATUS\n");
		return false;
	}
	if (!synscan_motor_status(device, kAxisDEC, &decMotorStatus)) {
		printf("ERROR GETTING DEC STATUS\n");
		return false;
	}

	//  If the motors are moving, can we read back the config? Do we need to stop them?
	if ((raMotorStatus & kStatusActiveMask) != 0) {
		printf("RA MOTOR IS ACTIVE DURING CONFIGURATION!\n");
		synscan_stop_axis(device, kAxisRA);
	}
	if ((decMotorStatus & kStatusActiveMask) != 0) {
		printf("DEC MOTOR IS ACTIVE DURING CONFIGURATION!\n");
		synscan_stop_axis(device, kAxisDEC);
	}

	//  Force configuration if either motor needs initialising
	//  User might have swapped mounts on us
	if (raMotorStatus == kStatusNotInitialised || decMotorStatus == kStatusNotInitialised || raMotorStatus == 0x000 || decMotorStatus == 0x000) {
		PRIVATE_DATA->mountConfigured = false;
	}

	//  Read all the mount data if we've not already done so
	if (!PRIVATE_DATA->mountConfigured) {
		//  Query axis 360 degree rotation steps
		if (!synscan_total_axis_steps(device, kAxisRA, &PRIVATE_DATA->raTotalSteps))
			return false;
		if (!synscan_total_axis_steps(device, kAxisDEC, &PRIVATE_DATA->decTotalSteps))
			return false;

		//  Query axis worm rotation steps
		if (!synscan_worm_rotation_steps(device, kAxisRA, &PRIVATE_DATA->raWormSteps))
			return false;
		if (!synscan_worm_rotation_steps(device, kAxisDEC, &PRIVATE_DATA->decWormSteps))
			return false;

		//  Query axis step timer frequency
		if (!synscan_step_timer_frequency(device, kAxisRA, &PRIVATE_DATA->raTimerFreq))
			return false;
		if (!synscan_step_timer_frequency(device, kAxisDEC, &PRIVATE_DATA->decTimerFreq))
			return false;

		//  Query axis high speed ratio
		if (!synscan_high_speed_ratio(device, kAxisRA, &PRIVATE_DATA->raHighSpeedFactor))
			return false;
		if (!synscan_high_speed_ratio(device, kAxisDEC, &PRIVATE_DATA->decHighSpeedFactor))
			return false;

//		PRIVATE_DATA->raTotalSteps = PRIVATE_DATA->decTotalSteps = 9024000;
//		PRIVATE_DATA->raWormSteps = PRIVATE_DATA->decWormSteps = 50133;
//		PRIVATE_DATA->raTimerFreq = PRIVATE_DATA->decTimerFreq = 64935;
//		PRIVATE_DATA->raHighSpeedFactor = PRIVATE_DATA->decHighSpeedFactor = 16;

		//  Check if mount understands polarscope LED brightness
		unsigned char brightness = 0;
		if (synscan_set_polarscope_brightness(device, brightness)) {
			MOUNT_POLARSCOPE_PROPERTY->hidden = false;
			MOUNT_POLARSCOPE_BRIGHTNESS_ITEM->number.value = 255;
		}

		//  Determine ZERO positions
		PRIVATE_DATA->raZeroPos = RA_HOME_POSITION - (PRIVATE_DATA->raTotalSteps / 4);     //  HA == 0 (horizontal shaft, rotated clockwise)
		PRIVATE_DATA->decZeroPos = DEC_HOME_POSITION - (PRIVATE_DATA->decTotalSteps / 4);  //  DEC has ZERO both east and west sides. This is the EAST zero.

		//  We do not redefine the ZERO positions for SOUTHERN HEMISPHERE because the mount does not redefine the direction and only understands
		//  step counts that increase going in an anti-clockwise direction. Therefore, we take account of southern hemisphere changes in the
		//  slewing routines.

		//  Dump out mount data
		printf("Total Steps:  RA == %10lu   DEC == %10lu\n", PRIVATE_DATA->raTotalSteps, PRIVATE_DATA->decTotalSteps);
		printf(" Worm Steps:  RA == %10lu   DEC == %10lu\n", PRIVATE_DATA->raWormSteps, PRIVATE_DATA->decWormSteps);
		printf(" Timer Freq:  RA == %10lu   DEC == %10lu\n", PRIVATE_DATA->raTimerFreq, PRIVATE_DATA->decTimerFreq);
		printf("  HS Factor:  RA == %10lu   DEC == %10lu\n", PRIVATE_DATA->raHighSpeedFactor, PRIVATE_DATA->decHighSpeedFactor);
		printf(" Polarscope:  %s\n", PRIVATE_DATA->canSetPolarscopeBrightness ? "YES" : "NO");
		printf("   Home Pos:  RA == %10lu   DEC == %10lu\n", RA_HOME_POSITION, DEC_HOME_POSITION);
		printf("   Zero Pos:  RA == %10lu   DEC == %10lu\n", PRIVATE_DATA->raZeroPos, PRIVATE_DATA->decZeroPos);

		//  Determine mount vendor and model
	}

	//  Initialize motors if necessary
	if (raMotorStatus == kStatusNotInitialised || raMotorStatus == 0x000) {
		if (!synscan_init_axis(device, kAxisRA))
			return false;
		if (!synscan_motor_status(device, kAxisRA, &raMotorStatus))
			return false;
		if (raMotorStatus == kStatusNotInitialised) {
			//  This is a fatal error - can't get the motor to initialize
			printf("RA MOTOR FAILURE: WILL NOT INITIALIZE!\n");
			return false;
		}
		if (!synscan_init_axis_position(device, kAxisRA, RA_HOME_POSITION))
			return false;
		printf("RA MOTOR INITIALIZED!\n");
	}
	else {
		//  if mount was previously configured - assume that configuration is still valid

		//  else this is also a fatal error - we don't know why the motors are not needing INIT
		//  maybe the software restarted??
		//  we could receover by configuring, but the user should be told that the assumption is the mount must be homed

		//  actually if the assumption is that the software restarted AND EQMac previously configured it, then we can recover
		//  by just reading the parameters and trusting the current position and computing where the home position should be

		printf("RA MOTOR OK %06X\n", raMotorStatus);
	}
	if (decMotorStatus == kStatusNotInitialised || decMotorStatus == 0x000) {
		if (!synscan_init_axis(device, kAxisDEC))
			return false;
		if (!synscan_motor_status(device, kAxisDEC, &decMotorStatus))
			return false;
		if (decMotorStatus == kStatusNotInitialised) {
			//  This is a fatal error - can't get the motor to initialize
			printf("DEC MOTOR FAILURE: WILL NOT INITIALIZE!\n");
			return false;
		}
		if (!synscan_init_axis_position(device, kAxisDEC, DEC_HOME_POSITION))
			return false;
		printf("DEC MOTOR INITIALIZED!\n");
	}
	else {
		//  if mount was previously configured - assume that configuration is still valid

		//  else this is also a fatal error - we don't know why the motors are not needing INIT
		//  maybe the software restarted??
		//  we could receover by configuring, but the user should be told that the assumption is the mount must be homed
		printf("DEC MOTOR OK %06X\n", decMotorStatus);
	}

	//  Read the current position
	synscan_get_coords(device);
	printf("   Position:  RA == %g   DEC == %g\n", PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition);

	//  Invalidate axis configurations
	PRIVATE_DATA->raAxisConfig.valid = false;
	PRIVATE_DATA->decAxisConfig.valid = false;

	//  Consider the mount configured once we reach here
	PRIVATE_DATA->mountConfigured = true;
	return true;
}

double synscan_tracking_rate(indigo_device* device) {
	switch (PRIVATE_DATA->trackingMode) {
		case kTrackingModeSidereal:
			return SIDEREAL_RATE;
		case kTrackingModeLunar:
			return LUNAR_RATE;
		case kTrackingModeSolar:
			return SOLAR_RATE;
		case kTrackingModeOff:
		default:
			return 0.0;
	}
}


static bool synscan_should_reconfigure_axis(indigo_device* device, enum AxisID axis, struct AxisConfig* config) {
	//  Check if we need to re-configure
	//  Mount seems to lose the turbo status and needs to be reconfigured each time
	if (config->turbo)
		return true;

	//  Reference the relevant axis config cache
	struct AxisConfig* cachedConfig = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisConfig : &PRIVATE_DATA->decAxisConfig;

	//  Check if reconfiguration is needed
	if (!cachedConfig->valid)
		return true;
	if (cachedConfig->direction != config->direction)
		return true;
	if (cachedConfig->turbo)
		return true;
	return false;
}

static void synscan_axis_config_for_rate(indigo_device* device, enum AxisID axis, double rate, struct AxisConfig* config) {
	//  Invert RA direction for southern hemisphere
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;
	if (south && axis == kAxisRA)
		rate = -rate;

	//  Capture original rate
	config->slewRate = rate;

	//  Determine direction from rate
	config->direction = kAxisDirectionFwd;
	if  (rate < 0) {
		config->direction = kAxisDirectionRev;
		rate = -rate;
	}

	//  Determine whether turbo mode is required for the specified rate
	config->turbo = false;
	if (rate > 128 * SIDEREAL_RATE) {
		config->turbo = true;
		rate /= (axis == kAxisRA) ? PRIVATE_DATA->raHighSpeedFactor : PRIVATE_DATA->decHighSpeedFactor;
	}

	//  Compute rate code
	rate *= ((axis == kAxisRA) ? PRIVATE_DATA->raTotalSteps : PRIVATE_DATA->decTotalSteps);
	rate /= 3600.0 * 360.0;
	double freq = (axis == kAxisRA) ? PRIVATE_DATA->raTimerFreq : PRIVATE_DATA->decTimerFreq;
	rate = freq / rate;
	config->rateCode = lrint(rate);

	//  Mark as valid
	config->valid = true;
}

bool synscan_configure_axis_for_rate(indigo_device* device, enum AxisID axis, double rate) {
	//  Compute config required
	struct AxisConfig requiredConfig;
	synscan_axis_config_for_rate(device, axis, rate, &requiredConfig);

	//  Determine if simple rate change is sufficient (or full reconfiguration)
	bool reconfigure = synscan_should_reconfigure_axis(device, axis, &requiredConfig);

	//  Reference relevant axis config cache
	struct AxisConfig* cachedConfig = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisConfig : &PRIVATE_DATA->decAxisConfig;

	//  Determine whether backlash compensation will be needed
	//requiredConfig.compensateBacklash = (requiredConfig.direction != cachedConfig->direction);

	//  Invalidate gearing in case of error
	cachedConfig->valid = false;

	//  Set the gearing and slew rate
	bool ok = true;
	if (reconfigure) {
		ok = ok && synscan_set_axis_gearing(device, axis, requiredConfig.direction, requiredConfig.turbo ? kAxisSpeedHigh : kAxisSpeedLow);
		if (!ok)
			return false;
	}
	ok = ok && synscan_set_axis_slew_rate(device, axis, requiredConfig.rateCode);
	if (!ok)
		return false;

	//  Validate the axis config cache
	*cachedConfig = requiredConfig;
	return true;
}

static bool synscan_update_axis_to_rate(indigo_device* device, enum AxisID axis, double rate, bool* result) {
	//  Compute config required
	struct AxisConfig requiredConfig;
	synscan_axis_config_for_rate(device, axis, rate, &requiredConfig);

	//  Determine if simple rate change is sufficient (or full reconfiguration)
	bool reconfigure = synscan_should_reconfigure_axis(device, axis, &requiredConfig);
	if (reconfigure) {
		*result = false;
		return true;
	}
	else {
		*result = true;
	}

	//  Reference relevant axis config cache
	struct AxisConfig* cachedConfig = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisConfig : &PRIVATE_DATA->decAxisConfig;

	//  Invalidate gearing in case of error
	cachedConfig->valid = false;

	bool ok = synscan_set_axis_slew_rate(device, axis, requiredConfig.rateCode);
	if (!ok)
		return false;

	//  Validate the axis config cache
	*cachedConfig = requiredConfig;
	return true;
}

static double ha_steps_to_position(indigo_device* device, AxisPosition steps) {
	double hapos = (double)(steps - PRIVATE_DATA->raZeroPos) / (double)PRIVATE_DATA->raTotalSteps;
	if (hapos < 0)
		hapos += 1;
	return hapos;
}

static double dec_steps_to_position(indigo_device* device, AxisPosition steps) {
	double decpos = (double)(steps - PRIVATE_DATA->decZeroPos) / (double)PRIVATE_DATA->decTotalSteps;
	if (decpos < 0)
		decpos += 1;
	return decpos;
}

static AxisPosition ha_position_to_steps(indigo_device* device, double position) {
	if (position > 0.75)
		position -= 1.0;
	return lrint(PRIVATE_DATA->raZeroPos + (position * PRIVATE_DATA->raTotalSteps));
}

static AxisPosition dec_position_to_steps(indigo_device* device, double position) {
	if (position > 0.75)
		position -= 1.0;
	return lrint(PRIVATE_DATA->decZeroPos + (position * PRIVATE_DATA->decTotalSteps));
}

void coords_encoder_to_eq(indigo_device* device, double ha_enc, double dec_enc, double* ha, double* dec) {
	//  Get hemisphere
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;

	//  NORTHERN HEMISPHERE
	//  Convert to Declination in range [-90, +90] with WEST flag
	//
	//  DEG:
	//      [0, 90]         dec = deg,          west = false
	//      [90, 180]       dec = 180 - deg,    west = true
	//      [180, 270]      dec = 180 - deg,    west = true
	//      [270, 360]      dec = deg - 360,    west = false
	//
	//
	//  SOUTHERN HEMISPHERE
	//  Convert to Declination in range [-90, +90] with WEST flag
	//
	//  DEG:
	//      [0, 90]         dec = -deg,         west = true
	//      [90, 180]       dec = deg - 180,    west = false
	//      [180, 270]      dec = deg - 180,    west = false
	//      [270, 360]      dec = 360 - deg,    west = true

	//  Normal DEC
	double epdec = dec_enc;
	if (epdec < 0)
		epdec += 1.0;

	bool west = false;
	if (epdec < 0.25)
		*dec = epdec;
	else if (epdec < 0.75) {
		*dec = 0.5 - epdec;
		west = true;
	}
	else // decPos in [0.75, 1.0]
		*dec = epdec - 1.0;

	//  Adjust for southern hemisphere
	if (south) {
		west = !west;
		*dec = -*dec;
	}
	*dec *= 2.0 * M_PI;
	//eq->west = west;


	//  NORTHERN HEMISPHERE
	//  Convert to HA
	//      HA represents the time since transitting the meridian.
	//      Thus:
	//          [0, 12]     Object is west of meridian
	//          [-12, 0]    Object is east of meridian and will transit in -ha hours
	//
	//  HA Hours:
	//      [0, 6) W        HA = Hours
	//      [0, 6) E        HA = Hours - 12
	//      [6, 12) W       HA = Hours
	//      [6, 12) E       HA = Hours - 12
	//      [12, 18) W      HA = Hours - 24
	//      [12, 18) E      HA = Hours - 12
	//      [18, 24] W      HA = Hours - 24
	//      [18, 24] E      HA = Hours - 12
	//
	//  In practice, the mount cannot position itself with HA Hours > 12 because the counterweights would be up
	//  and the scope would be in danger of hitting the mount. With care (i.e. depending on the DEC), we can allow
	//  HA Hours > 12. However, there is a band of HA Hours centred on 18 that we cannot position to at all. We
	//  can represent this band as 18+/-N, where N is half the width of the band (i.e. [16,20] would mean N=2).
	//
	//
	//  SOUTHERN HEMISPHERE
	//  Convert to HA
	//      HA represents the time since transitting the meridian.
	//      Thus:
	//          [0, 12]     Object is east of meridian
	//          [-12, 0]    Object is west of meridian and will transit in -ha hours
	//
	//  HA Hours:
	//      [6, 0] W        HA = 12 - Hours
	//      [6, 0] E        HA = -Hours
	//      [12, 6] W       HA = 12 - Hours
	//      [12, 6] E       HA = -Hours
	//      [18, 12] W      HA = 12 - Hours
	//      [18, 12] E      HA = 24 - Hours
	//      [24, 18] W      HA = 12 - Hours
	//      [24, 18] E      HA = 24 - Hours

	if (!south) {
		if (!west)
			*ha = ha_enc - 0.5;
		else if (ha_enc < 0.5)
			*ha = ha_enc;
		else
			*ha = ha_enc - 1.0;
	}
	else {
		if (west)
			*ha = 0.5 - ha_enc;
		else if (ha_enc < 0.5)
			*ha = -ha_enc;
		else
			*ha = 1.0 - ha_enc;
	}
	if (*ha < 0)
		*ha += 1.0;
	if (*ha > 1.0)
		*ha -= 1.0;

	*ha *= 2.0 * M_PI;
}

static void coords_eq_to_encoder2(indigo_device* device, double ha, double dec, double haPos[], double decPos[]) {
	//  To do a slew, we have to decide which side of the meridian we'd like to be. Normally that would be
	//  the side that gives maximum time before a meridian flip would be required. If the object has passed
	//  the meridian already, then it is setting and we won't need to flip. If it is approaching the meridian,
	//  we could consider performing an early meridian flip if it is close enough, otherwise ...

	//  Suppose we've decided that DEC should slew eastwards from HOME to the DEC coordinate. How do we decide
	//  where to slew RA?
	//
	//  If we've done our eastward DEC slew from home, the scope is pointing at LST + 6 hours.
	//  If we slew clockwise in RA for 6 hours, we'd be pointing at LST+12, which might be below horizon if DEC < 90 - LAT.
	//  If we slew anti-clockwise for 6 hours, we'd be pointing at LST and needing to do a meridian flip.
	//
	//  If we take that meridian flip, we're still pointing at LST, but now the DEC slew is westward from home.
	//  If we slew anti-clockwise 12 hours, we'd be pointing at LST-12, which might be below horizon if DEC < 90 - LAT.
	//
	//  With an eastward DEC slew, we can point at targets between LST and LST+12.
	//  With a  westward DEC slew, we can point at targets between LST and LST-12.
	//  The east/west sense is reversed for southern hemisphere.

	//  Get hemisphere
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;

	//  Convert DEC to Degrees
	//  [-90,90]
	//
	//  NORTHERN HEMISPHERE
	//
	//  DEC 90              Degrees = 90 W or 90 E
	//  DEC 75              Degrees = 105 W or 75 E
	//  DEC 45              Degrees = 135 W or 45 E
	//  DEC 20              Degrees = 160 W or 20 E
	//  DEC 0               Degrees = 180 W or 0 E
	//  DEC -20             Degrees = 200 W or 340 E
	//  DEC -45             Degrees = 225 W or 315 E
	//  DEC -75             Degrees = 255 W or 285 E
	//  DEC -90             Degrees = 270
	//
	//
	//  SOUTHERN HEMISPHERE
	//
	//  DEC -90             Degrees = 90
	//  DEC -75             Degrees = 105 E or 75 W
	//  DEC -45             Degrees = 135 E or 45 W
	//  DEC -20             Degrees = 160 E or 20 W
	//  DEC 0               Degrees = 180 E or 0 W
	//  DEC  20             Degrees = 200 E or 340 W
	//  DEC  45             Degrees = 225 E or 315 W
	//  DEC  75             Degrees = 255 E or 285 W
	//  DEC  90             Degrees = 270

	//  Generate the two DEC positions (east and west of meridian)
	double degw = M_PI - dec;   //  NORTH variant by default
	if (south) {
		if (dec < 0)
			degw = -dec;
		else
			degw = M_PI + M_PI - dec;
	}

	double dege = M_PI + dec;   //  SOUTH variant by default
	if (!south) {
		if (dec < 0)
			dege = M_PI + M_PI + dec;
		else
			dege = dec;
	}

	//  Rebase the angles to optimise the DEC slew relative to HOME
	if (degw > M_PI + M_PI_2)
		degw -= M_PI + M_PI;
	if (dege > M_PI + M_PI_2)
		dege -= M_PI + M_PI;
	degw /= M_PI + M_PI;
	dege /= M_PI + M_PI;

	//  HA represents hours since the transit of the object
	//  < 0 means object has yet to transit
	//
	//  convert to [-12,12]
	if (ha > M_PI)
		ha -= M_PI + M_PI;
	if (ha < -M_PI)
		ha += M_PI + M_PI;
	assert(ha >= -M_PI && ha <= M_PI);

	//  Compute HA positions to match each DEC position
	double haw;
	if (ha >= 0)
		haw = ha;
	else
		haw = ha + M_PI + M_PI;
	if (south)
		haw = M_PI - ha;

	double hae = ha + M_PI;
	if (south) {
		if (ha >= 0)
			hae = M_PI + M_PI - ha;
		else
			hae = -ha;
	}
	haw /= M_PI + M_PI;
	hae /= M_PI + M_PI;

	assert(haw < 0.5 || hae < 0.5);

	//  Decide whether EAST or WEST provides the "normal" / CW-Down slew and fill in the positions
	if (haw < 0.5) {
		haPos[0] = haw;
		decPos[0] = degw;
		haPos[1] = hae;
		decPos[1] = dege;
	}
	else {
		haPos[0] = hae;
		decPos[0] = dege;
		haPos[1] = haw;
		decPos[1] = degw;
	}

	printf("SOLUTIONS:\n");
	printf("  OK:  %g,   %g\n", haPos[0], decPos[0]);
	printf("  UP:  %g,   %g\n", haPos[1], decPos[1]);
}


void synscan_get_coords(indigo_device *device) {
	char response[128];
	long haPos, decPos;
	if (synscan_axis_position(device, kAxisRA, &haPos))
		PRIVATE_DATA->raPosition = ha_steps_to_position(device, haPos);
	if (synscan_axis_position(device, kAxisDEC, &decPos))
		PRIVATE_DATA->decPosition = dec_steps_to_position(device, decPos);
}

#if 0
- (void) stop {
	@synchronized(self) {
		//  Cannot do anything from the error mode or if idle or already stopping
		if (self.globalMode == kGlobalModeError || self.globalMode == kGlobalModeIdle || self.globalMode == kGlobalModeStopping)
			return;
	}

	[self updateGlobalModeTo:kGlobalModeStopping];

	//  Cancel whatever else was happening
	[globalQueue cancelAllOperations];

	//  Queue the global stop operation (cannot be cancelled)
	__block NSOperation* op = [NSBlockOperation blockOperationWithBlock:^{
														 printf("STOPPING MOUNT\n");
														 //  Stop both axes
														 bool ok = [raAxis stopAxisWithBlock:nil];
														 ok = ok && [decAxis stopAxisWithBlock:nil];
														 ok = ok && [raAxis waitUntilStoppedWithBlock:(bool(^)(void))^{ return [op isCancelled]; }];
														 ok = ok && [decAxis waitUntilStoppedWithBlock:(bool(^)(void))^{ return [op isCancelled]; }];
														 if (!ok) {
														 //  Go to ERROR mode
														 [self updateGlobalModeTo:kGlobalModeError];
														 return;
														 }

														 //  Update the axis positions now we are stopped

														 //  Set tracking off
														 [self willChangeValueForKey:@"trackingMode"];
														 _trackingMode = kTrackingModeOff;
														 [self didChangeValueForKey:@"trackingMode"];

														 //  Move to IDLE mode
														 [self updateGlobalModeTo:kGlobalModeIdle];
														 }];
	[globalQueue addOperation:op];
}
#endif

void synscan_stop_and_resume_tracking_for_axis(indigo_device* device, enum AxisID axis) {
	 if (axis == kAxisRA) {
		 //  Start tracking in the requested mode
		 //synscan_start_tracking_mode(device, PRIVATE_DATA->trackingMode);
		 synscan_stop_axis(device, axis);
		 PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	 }
	 else {
		 synscan_stop_axis(device, axis);
		 PRIVATE_DATA->decAxisMode = kAxisModeIdle;
	 }
}

bool synscan_slew_axis_at_rate(indigo_device* device, enum AxisID axis, double rate) {
	printf("SLEWING AXIS AT RATE %g\n", rate);

	//  If slewing at rate already, change rate without stopping if we can
	enum AxisMode* axisMode = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisMode : &PRIVATE_DATA->decAxisMode;
	if (*axisMode == kAxisModeTracking) {
		//  Just change the rate if possible
		bool axisUpdated;
		if (!synscan_update_axis_to_rate(device, axis, rate, &axisUpdated)) {
			return false;
		}

		//  Set tracking mode again to update the position predictions
		if (axisUpdated) {
			*axisMode = kAxisModeTracking;
			return true;
		}
	}

	//  If not stopped, move to stopped state first
	if (*axisMode != kAxisModeIdle) {
		//  Stop the axis
		//  FIXME - this needs to be synschronous - except we dont want to block the driver
		if (!synscan_stop_axis(device, axis)) {
			return false;
		}

		//  Move to stopped state
		*axisMode = kAxisModeIdle;
	}

	//  If stopped, just configure and start slewing
	if (*axisMode == kAxisModeIdle) {
		//  Configure and slew the axis
		if (!synscan_configure_axis_for_rate(device, axis, rate) || !synscan_slew_axis(device, axis)) {
			return false;
		}

		//  Move to tracking state
		*axisMode = kAxisModeTracking;
	}
	return true;
}

bool synscan_start_tracking_mode(indigo_device* device, enum TrackingMode mode) {
	bool ok;
	switch (mode) {
	case kTrackingModeSidereal:
		ok = synscan_slew_axis_at_rate(device, kAxisRA, SIDEREAL_RATE);
		break;
	case kTrackingModeLunar:
		ok = synscan_slew_axis_at_rate(device, kAxisRA, LUNAR_RATE);
		break;
	case kTrackingModeSolar:
		ok = synscan_slew_axis_at_rate(device, kAxisRA, SOLAR_RATE);
		break;
	case kTrackingModeOff:
	default:
		ok = synscan_stop_axis(device, kAxisRA);
		PRIVATE_DATA->raAxisMode = kAxisModeIdle;
		break;
	}
	return ok;
}

static int synscan_select_best_encoder_point(indigo_device* device, double haPos[], double decPos[]) {
	//  Check if we're allowed to do CW-UP slews
//	NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];
//	NSInteger ra_slew_priority = [prefs integerForKey:@"ra_slew_priority"];
//	bool limitsEnabled = [[prefs objectForKey:@"limits_enabled"] boolValue];

	//  Return the SAFE point if CW-UP slews are disabled or limits are disabled in general
	//if (ra_slew_priority == 0 || !limitsEnabled)
	//	return &eps[0];
	return 0;

#if 0
	//  Load limits from prefs
	NSNumber* leftPosition = [prefs objectForKey:@"left_ra_limit"];
	NSNumber* rightPosition = [prefs objectForKey:@"right_ra_limit"];

	//  Cache RA limit positions
	double leftLimitPosition = 0.5;
	double rightLimitPosition = 0.0;
	if (leftPosition)
		leftLimitPosition = [leftPosition doubleValue];
		if (rightPosition)
			rightLimitPosition = [rightPosition doubleValue];

			//  Rebase right limit
			if (rightLimitPosition > 0.75)
				rightLimitPosition -= 1.0;

				//  Normalise the cwup_h for comparison
				double cwup_h = eps[1].ha;
				if (cwup_h > 0.75)
					cwup_h -= 1.0;

					//  Check if the CW-UP point is within limits
					if (cwup_h > leftLimitPosition || cwup_h < rightLimitPosition) {
						printf("CWUP HA IS OUTSIDE LIMITS - CANNOT SLEW CW UP\n");
						return &eps[0];
					}

	//  Check if the CW-UP point is desirable over the normal point
	bool south = [PreferencesController latitude] < 0.0;
	if (!south) {
		//  Desirable if time to flip (reach 0.5) is better
		if (0.5 - cwup_h > 0.5 - eps[0].ha) {
			printf("CWUP SLEW IS DESIRABLE!\n");
			return &eps[1];
		}
	}
	else {
		//  Desirable if time to flip (reach 0.0) is better
		if (cwup_h > eps[0].ha) {
			printf("CWUP SLEW IS DESIRABLE!\n");
			return &eps[1];
		}
	}

	//  Return the safe position
	return &eps[0];
#endif
}

static bool synscan_slew_axis_to_position(indigo_device* device, enum AxisID axis, double position) {
	//  Reference relevant axis config cache
	struct AxisConfig* cachedConfig = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisConfig : &PRIVATE_DATA->decAxisConfig;

	//  Get axis position
	AxisPosition currentPosition;
	bool ok = synscan_axis_position(device, axis, &currentPosition);
	if (!ok)
		return false;

	//  Convert position argument to steps
	AxisPosition steps = (axis == kAxisRA) ? ha_position_to_steps(device, position) : dec_position_to_steps(device, position);

	//  Compute step delta
	AxisPosition delta = steps - currentPosition;

	//  Initiate axis slew
	if (delta != 0) {
		enum AxisDirectionID d = (delta < 0) ? kAxisDirectionRev : kAxisDirectionFwd;
		delta = labs(delta);

//		//  Check for backlash
//		if (cachedConfig->direction != d) {
//			//  Extend delta by the anti-backlash amount
//			delta += [self antibacklashStepsForAxis:axis];
//			printf("Doing ANTI-BACKLASH slew extension\n");
//		}

		AxisPosition slowdown = delta - 80000;
		if (slowdown < 0)
			slowdown = delta / 2;

		//  Invalidate axis config since we change to absolute slew mode
		cachedConfig->valid = false;

		//  Always keep the direction up to date for backlash detection
		cachedConfig->direction = d;

		//  Initiate the slew
		ok = ok && synscan_set_axis_gearing(device, axis, d, kAxisSpeedAbsSlew);
		ok = ok && synscan_set_axis_step_count(device, axis, delta);
		ok = ok && synscan_set_axis_slowdown(device, axis, slowdown);
		ok = ok && synscan_slew_axis(device, axis);
	}
	return ok;
}

void slew_timer_callback(indigo_device *device) {
	switch (PRIVATE_DATA->slew_state) {
		case SLEW_START:
			//  Make sure the axes are stopped
			{
				if (PRIVATE_DATA->raAxisMode != kAxisModeIdle)
					synscan_stop_axis(device, kAxisRA);
				if (PRIVATE_DATA->decAxisMode != kAxisModeIdle)
					synscan_stop_axis(device, kAxisDEC);

				//  Move to next state
				PRIVATE_DATA->slew_state = SLEW_STOP;
			}
			break;

		case SLEW_STOP:
			//  Wait for both axes to be stopped
			{
				if (PRIVATE_DATA->raAxisMode != kAxisModeIdle) {
					//  Get the axis status
					long axisStatus;
					if (!synscan_motor_status(device, kAxisRA, &axisStatus))
						break;

					//  Mark axis as stopped if motor has come to rest
					if ((axisStatus & kStatusActiveMask) == 0)
						PRIVATE_DATA->raAxisMode = kAxisModeIdle;
				}
				if (PRIVATE_DATA->decAxisMode != kAxisModeIdle) {
					//  Get the axis status
					long axisStatus;
					if (!synscan_motor_status(device, kAxisDEC, &axisStatus))
						break;

					//  Mark axis as stopped if motor has come to rest
					if ((axisStatus & kStatusActiveMask) == 0)
						PRIVATE_DATA->decAxisMode = kAxisModeIdle;
				}

				//  Move to next state once both axes are idle
				if (PRIVATE_DATA->raAxisMode == kAxisModeIdle && PRIVATE_DATA->decAxisMode == kAxisModeIdle)
					PRIVATE_DATA->slew_state = SLEW_PHASE0;
			}
			break;

		case SLEW_PHASE0:
			//  Perform preliminary slew on both axes
			{
				//  Compute initial target positions
				double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value * M_PI / 12.0;
				double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value * M_PI / 180.0;
				double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
				double lst = indigo_lst(lng) * M_PI / 12.0;
				double ha = lst - ra;
				double haPos[2], decPos[2];
				coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

				//  Select best encoder point based on limits
				int idx = synscan_select_best_encoder_point(device, haPos, decPos);
				printf("SLEW TO:  %g   /   %g     (HA %g / DEC %g)\n", haPos[idx], decPos[idx], ha, dec);

				//  Limits check

				//  If beyond safe limits due to tracking, do a pre-slew to get back to the edge of limits

				//  If slewing to beyond safe limits, slew dec to position, slew to RA.

				//  Start the first slew
				bool ok = synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);
				ok = ok && synscan_slew_axis_to_position(device, kAxisDEC, decPos[idx]);

				//  Move to next phase
				PRIVATE_DATA->slew_state = SLEW_PHASE1;
			}
			break;
		case SLEW_PHASE1:
			//  Wait for HA slew to complete
			{
				//  Get the axis status
				long axisStatus;
				if (!synscan_motor_status(device, kAxisRA, &axisStatus))
					break;

				//  Nothing to do if still going
				if ((axisStatus & kStatusActiveMask) != 0)
					break;

				//    Compute precise HA slew for LST + 5 seconds
				double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value * M_PI / 12.0;
				double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value * M_PI / 180.0;
				double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
				PRIVATE_DATA->target_lst = indigo_lst(lng) + (5 / 3600.0);
				double ha = (PRIVATE_DATA->target_lst * M_PI / 12.0) - ra;
				double haPos[2], decPos[2];
				coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

				//  Select best encoder point based on limits
				int idx = synscan_select_best_encoder_point(device, haPos, decPos);
				printf("SLEW TO:  %g   /   %g\n", haPos[idx], decPos[idx]);

				//		Start new HA slew
				bool ok = synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);

				//		Move to next phase
				PRIVATE_DATA->slew_state = SLEW_PHASE2;
			}
			break;
		case SLEW_PHASE2:
			//  Wait for precise HA slew to complete
			{
				//  Get the axis status
				long axisStatus;
				if (!synscan_motor_status(device, kAxisRA, &axisStatus))
					break;

				//  Nothing to do if still going
				if ((axisStatus & kStatusActiveMask) != 0)
					break;

				//	Move to next phase
				PRIVATE_DATA->slew_state = SLEW_PHASE3;
			}
			break;
		case SLEW_PHASE3:
			//  Wait for LST to match
			{
				//  Get current LST
				double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
				double lst = indigo_lst(lng);

				//  Wait for LST to reach target LST
				if (lst < PRIVATE_DATA->target_lst)
					break;

				//    Start tracking if we need to
				if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
					synscan_start_tracking_mode(device, kTrackingModeSidereal);
				}

				//	Move to next phase
				PRIVATE_DATA->slew_state = SLEW_PHASE4;
			}
			break;
		case SLEW_PHASE4:
			//  Wait for DEC slew to complete
			{
				//  Get the axis status
				long axisStatus;
				if (!synscan_motor_status(device, kAxisDEC, &axisStatus))
					break;

				//  Nothing to do if still going
				if ((axisStatus & kStatusActiveMask) != 0)
					break;

				//	Move to next phase
				PRIVATE_DATA->slew_state = SLEW_NONE;
			}
			break;
		case SLEW_NONE:
		default:
			//  Slew is not happening
			break;
	}

	if (PRIVATE_DATA->slew_state != SLEW_NONE) {
		indigo_reschedule_timer(device, 0.25, &PRIVATE_DATA->slew_timer);
	} else {
		PRIVATE_DATA->slew_timer = NULL;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	}
}

#if 0
bool synscan_slew_to_ra_dec(indigo_device* device, double ra, double dec, bool track) {
	//  Get location
	double lngRad = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value * M_PI / 180.0;

	//        //  Mean to observed place
	//
	//        //  Get LST
	//        double lstRad = [AstroMath localSiderealTime:lngRad];
	//
	//        //  Compute required HA
	//        double ra = cp->ra;
	//        double ha = lstRad - ra;
	//        double dec = cp->dec;
	//
	//        //  Decide if we can't slew to target because it is on/below horizon
	//        AltAzPoint aa;
	//        EquatorialPoint _eq = { ha, cp->dec, false };
	//        [Coordinates equatorial:&_eq toHorizon:&aa];
	//
	//        //  FIXME - if limits are enabled, use the configured horizon
	//        if (aa.alt < FLT_EPSILON) {
	//            printf("TARGET is on or below horizon - cannot SLEW\n");
	//            return;
	//        }

	 //  Compute initial target positions
	AxisPosition haPos[2], decPos[2];
	synscan_ra_dec_to_encoder_with_lst_offset(device, ra, dec, 0.0, haPos, decPos);
	//[syncManager celestialPoint:&_cp toEncoder:eps withLSTOffset:0.0];
	 //            EquatorialPoint eq = { ha, dec, false };
	 //            [syncManager catalogToActual:&eq];
	 //            [Coordinates equatorial:&eq toEncoder:&ep];
	 //            //[syncManager mapCatalogPoint:&ep];

	 //  Select best encoder point based on limits
	int idx = synscan_select_best_encoder_point(device, haPos, decPos);
	//EncoderPoint* ep = [self selectBestEncoderPoint:eps];
	printf("SLEW TO:  %g   /   %g\n", haPos[idx], decPos[idx]);

	 //  Limits check

	 //  If beyond safe limits due to tracking, do a pre-slew to get back to the edge of limits

	 //  If slewing to beyond safe limits, slew dec to position, slew to RA.

	 //  Start the first slew
	bool ok = synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);
	ok = ok && synscan_slew_axis_to_position(device, kAxisDec, decPos[idx]);

	//  Set slew state to PHASE1
	//  Start a timer to check every 0.25 secs if the HA slew has finished



	 //  Wait for HA slew to complete
	 ok = ok && [raAxis waitUntilStoppedWithBlock:(bool(^)(void))^{ return [op isCancelled]; }];
	 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
	 return;

	 //  Recompute the RA position using a future LST (5 seconds ahead)
	 double future_lst = [AstroMath localSiderealTime:lngRad] + (5.0*M_PI/43200.0);
	 //            eq.ha = future_lst - ra;
	 //            [syncManager catalogToActual:&eq];
	 //            [Coordinates equatorial:&eq toEncoder:&ep];
	 [syncManager celestialPoint:&_cp toEncoder:eps withLSTOffset:5.0];

	 //  Select best encoder point based on limits
	 ep = [self selectBestEncoderPoint:eps];

	 //[syncManager mapCatalogPoint:&ep];
	 printf("CORRECTIVE SLEW TO:  %g   /   %g\n", ep[0].ha, ep[0].dec);

	 //  Perform corrective slew and wait for HA to finish
	 ok = ok && [raAxis slewAxisToPosition:ep[0].ha withBlock:(bool(^)(void))^{ return [op isCancelled]; }];
	 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
	 return;

	 //  Wait till LST hits target
	 printf("WAITING FOR LST TO MATCH...\n");
	 [self delayWithBlock:^{
		while ([AstroMath localSiderealTime:lngRad] <= future_lst && !op.isCancelled)
		usleep(10000); // 0.1 secs
		}];
	 printf("LST ARRIVED!\n");
	 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
	 return;

	 //  Set tracking rate, gearing, and start tracking at sidereal rate
	 //  FIXME - we need to get the mount tracking and notify the mount controller semantically that this is happening
	 //  We could ask it to start tracking for us.
	 ok = ok && [raAxis slewAxisAtRate:SIDEREAL_RATE];

	 [self willChangeValueForKey:@"trackingMode"];
	 _trackingMode = kTrackingModeSidereal;
	 [self didChangeValueForKey:@"trackingMode"];

	 //  Wait for DEC slew to complete
	 ok = ok && [decAxis waitUntilStoppedWithBlock:(bool(^)(void))^{ return [op isCancelled]; }];

	 //  Signal completion
	 //  Beep
	 //  Call caller's block

	 //  Basso*, Blow, Bottle, Frog, Funk, Glass, Hero, Morse, Ping, Pop, Purr*, Sosumi, Submarine, Tink
	 [[NSSound soundNamed:@"Sosumi"] play];

	 printf("*** SLEW COMPLETE\n");

	 //  Notify the delegate about slew completion
	 //if ([(id)delegate respondsToSelector:@selector(slewDidComplete)]) {
	 //    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
	 //        [delegate slewDidComplete];
	 //    }];
	 //}
}
#endif

#if 0
- (void) meridianFlip {
#if 0
	@synchronized(self) {
		//  Can't slew if already slewing
		if (self.globalMode == kGlobalModeSlewing)
			return;

		//  Get current position
		EncoderPoint ep;
		[self currentPosition:&ep];

		//  Unmap current position
		EquatorialPoint eq;
		[syncManager encoderPoint:&ep toEquatorial:&eq];

		//  Get LST
		double lngRad = [PreferencesController longitude];
		double lstRad = [AstroMath localSiderealTime:lngRad];

		//  Compute RA/DEC
		CelestialPoint cp = { lstRad - eq.ha, eq.dec };

		//  Determine encoder solutions
		EncoderPoint eps[2];
		[syncManager celestialPoint:&cp toEncoder:eps withLSTOffset:0.0];

		//  Determine the solution that is flipped with respect to current position
		int flipped;
		if (fabs(eps[0].dec - ep.dec) >= fabs(eps[1].dec - ep.dec))
			flipped = 0;
		else
			flipped = 1;

		//        //  Get hemisphere
		//        bool south = [PreferencesController latitude] < 0;
		//
		//        //  Flip the DEC
		//        double deg;
		//        eq.west = !eq.west;
		//        if (eq.dec >= 0)
		//            deg = eq.west ? (M_PI - eq.dec) : eq.dec;
		//        else
		//            deg = west ? (M_PI - eq.dec) : (M_PI + M_PI + eq.dec);
		//
		//        if (south) {
		//            if (eq.dec < 0)
		//                deg = west ? -eq.dec : (M_PI + eq.dec);
		//            else
		//                deg = west ? (M_PI + M_PI - eq.dec) : (M_PI + eq.dec);
		//        }
		//
		//        //  Rebase the angle to optimise the DEC slew relative to HOME
		//        if (deg > M_PI + M_PI/2.0)
		//            deg -= M_PI + M_PI;
		//
		//        //  Compute flipped DEC position
		//        ep.dec = deg / (M_PI + M_PI);
		//
		//        //  Compute HA position given DEC position
		//        ep.ha = [Coordinates haPosforHA:eq.ha decPos:ep.dec];
		//
		//        //  Map the flipped point
		//        [syncManager mapCatalogPoint:&ep];

		//  Limits check
		NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];
		bool limitsEnabled = [[prefs objectForKey:@"limits_enabled"] boolValue];
		if (limitsEnabled) {
			//  Get RA limits
			NSNumber* leftPosition = [prefs objectForKey:@"left_ra_limit"];
			NSNumber* rightPosition = [prefs objectForKey:@"right_ra_limit"];

			//  Normalise RA limits
			double leftLimitPosition = 0.5;
			double rightLimitPosition = 0.0;
			if (leftPosition)
				leftLimitPosition = [leftPosition doubleValue];
			if (rightPosition)
				rightLimitPosition = [rightPosition doubleValue];
			if (leftLimitPosition > 0.75)
				leftLimitPosition -= 1.0;
			if (rightLimitPosition > 0.75)
				rightLimitPosition -= 1.0;
			double ha = eps[flipped].ha;
			if (ha > 0.75)
				ha -= 1.0;

			//  Check if RA limit exceeded
			bool raLimitViolated = (ha < rightLimitPosition || ha > leftLimitPosition);
			if (raLimitViolated) {
				//  refuse to start the slew
				printf("SLEW OUTWITH LIMITS - REFUSING TO SLEW\n");
				return;
			}
		}

		//  Grab flipped point for use in block
		__block EncoderPoint target = { eps[flipped].ha, eps[flipped].dec };

		//  Create a block to handle the slew
		__block NSOperation* op = [NSBlockOperation blockOperationWithBlock:^{
															 //  Compute initial target positions
															 double raTarget;
															 raTarget = cp.ra;
															 printf("SLEW TO:  %g   /   %g\n", target.ha, target.dec);

															 //  Start the first slew
															 NSOperation* ha_op = [raAxis slewAxisToPosition:target.ha];
															 NSOperation* dec_op = [decAxis slewAxisToPosition:target.dec];

															 //  Wait for HA slew to complete
															 [ha_op waitUntilFinished];
															 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
															 return;

															 //  Recompute the RA position using a future LST (5 seconds ahead)
															 double future_lst = [AstroMath localSiderealTime:lngRad] + (5.0*M_PI/43200.0);

															 EncoderPoint eps[2];
															 [syncManager celestialPoint:&cp toEncoder:eps withLSTOffset:5.0];




															 //            double ha = future_lst - raTarget;
															 //            target.ha = [Coordinates haPosforHA:ha decPos:target.dec];


															 printf("CORRECTIVE SLEW TO:  %g   /   %g\n", eps[flipped].ha, eps[flipped].dec);

															 //  Perform corrective slew
															 ha_op = [raAxis slewAxisToPosition:eps[flipped].ha];

															 //  Wait for RA slew to complete
															 [ha_op waitUntilFinished];
															 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
															 return;

															 //  Wait till LST hits target
															 printf("WAITING FOR LST TO MATCH...\n");
															 [self delayWithBlock:^{
																while ([AstroMath localSiderealTime:lngRad] <= future_lst && !op.isCancelled)
																usleep(10000); // 0.1 secs
																}];
															 printf("LST ARRIVED!\n");
															 if (op.isCancelled)     //  FIXME - does this leave us in an inconsistent state?
															 return;

															 //  Set tracking rate, gearing, and start tracking at sidereal rate
															 //  FIXME - we need to get the mount tracking and notify the mount controller semantically that this is happening
															 //  We could ask it to start tracking for us.
															 [self startTrackingMode:kTrackingModeSidereal];

															 //  Wait for DEC slew to complete
															 [dec_op waitUntilFinished];

															 //  Signal completion
															 //  Beep
															 //  Call caller's block
															 printf("*** SLEW COMPLETE\n");

															 //  Notify the delegate about slew completion
															 //if ([(id)delegate respondsToSelector:@selector(slewDidComplete)]) {
															 //    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
															 //        [delegate slewDidComplete];
															 //    }];
															 //}
															 }];

		//  Add to the global queue
		[globalQueue addOperation:op];
	}
#endif
}

#if 1
- (void) pulseGuideRA:(double)ra DEC:(double)dec {
#if 0
	@synchronized(self) {
		//  Ignore if the mount is not tracking
		if (self.globalMode != kGlobalModeTracking || self.trackingMode == kTrackingModeOff)
			return;

		//  Get tracking base rate
		double base_rate = self.trackingRate;

		//  Get pulse rates
		NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];
		NSInteger raPulseRatePercent = [prefs integerForKey:@"raPulseGuideRate"];
		NSInteger decPulseRatePercent = [prefs integerForKey:@"decPulseGuideRate"];
		NSInteger minimumPulseLength = [prefs integerForKey:@"minPulseLength"];
		double raPulseRate = ((double)raPulseRatePercent / 100.0) * base_rate;
		double decPulseRate = ((double)decPulseRatePercent / 100.0) * base_rate;

		//  Apply RA pulse
		if (raPulseRatePercent > 0 && ra != 0.0) {
			[raPulseQueue addOperationWithBlock:^{
			 NSOperation* op;
			 if (ra < 0) {
			 //  Start a WEST slew pulse
			 op = [raAxis slewAxisAtRate:base_rate+raPulseRate];
			 [self willChangeValueForKey:@"raGuideStatus"];
			 _raGuideStatus = [NSString stringWithFormat:@"W %lums", lrint(fabs(ra) * 1000)];
			 [self didChangeValueForKey:@"raGuideStatus"];
			 }
			 else {
			 //  Start an EAST slew pulse
			 if (base_rate - raPulseRate < FLT_EPSILON /* 0.001 */)
			 op = [raAxis stopAxis];
			 else
			 op = [raAxis slewAxisAtRate:base_rate-raPulseRate];
			 [self willChangeValueForKey:@"raGuideStatus"];
			 _raGuideStatus = [NSString stringWithFormat:@"E %lums", lrint(fabs(ra) * 1000)];
			 [self didChangeValueForKey:@"raGuideStatus"];
			 }

			 //  Delay for the pulse duration
			 printf("Doing RA pulse %g secs\n", ra);
			 [op waitUntilFinished];

			 useconds_t duration = (useconds_t)lrint(fabs(ra) * 1000000);
			 if (duration < minimumPulseLength*1000)
			 duration = (useconds_t)minimumPulseLength*1000;
			 [self delayWithBlock:^{
				usleep(duration);
				}];

			 //  Restore tracking
			 op = [raAxis slewAxisAtRate:base_rate];
			 [op waitUntilFinished];

			 [self willChangeValueForKey:@"raGuideStatus"];
			 _raGuideStatus = nil;
			 [self didChangeValueForKey:@"raGuideStatus"];
			 }];
		}
		else if (_raGuideStatus != nil) {
			[self willChangeValueForKey:@"raGuideStatus"];
			_raGuideStatus = nil;
			[self didChangeValueForKey:@"raGuideStatus"];
		}

		//  FIXME - these guiding status messages could overlap if we pulse both axes at once.
		//          Maybe we need to have separate labels in the UI that light up only when there is a N/S/E/W pulse happening.
		//          We'd have two of those so we can represent both axes pulsing at once.

		//  Apply DEC pulse
		if (decPulseRatePercent > 0 && dec != 0.0) {
			[decPulseQueue addOperationWithBlock:^{
			 NSOperation* op;
			 if (dec < 0) {
			 //  Start a SOUTH slew pulse
			 op = [decAxis slewAxisAtRate:-decPulseRate];
			 [self willChangeValueForKey:@"decGuideStatus"];
			 _decGuideStatus = [NSString stringWithFormat:@"S %lums", lrint(fabs(dec) * 1000)];
			 [self didChangeValueForKey:@"decGuideStatus"];
			 }
			 else {
			 //  Start an NORTH slew pulse
			 op = [decAxis slewAxisAtRate:decPulseRate];
			 [self willChangeValueForKey:@"decGuideStatus"];
			 _decGuideStatus = [NSString stringWithFormat:@"N %lums", lrint(fabs(dec) * 1000)];
			 [self didChangeValueForKey:@"decGuideStatus"];
			 }

			 //  Delay for the pulse duration
			 printf("Doing DEC pulse %g secs\n", dec);
			 [op waitUntilFinished];
			 useconds_t duration = (useconds_t)lrint(fabs(dec) * 1000000);
			 if (duration < minimumPulseLength*1000)
			 duration = (useconds_t)minimumPulseLength*1000;
			 [self delayWithBlock:^{
				usleep(duration);
				}];

			 //  Stop motor
			 op = [decAxis stopAxis];
			 [op waitUntilFinished];

			 //  Restore status
			 [self willChangeValueForKey:@"decGuideStatus"];
			 _decGuideStatus = nil;
			 [self didChangeValueForKey:@"decGuideStatus"];
			 }];
		}
		else if (_decGuideStatus != nil) {
			[self willChangeValueForKey:@"decGuideStatus"];
			_decGuideStatus = nil;
			[self didChangeValueForKey:@"decGuideStatus"];
		}
	}
#endif
}
#endif

- (void) syncTo:(CelestialPoint*)cp {
#if 0
	@synchronized(self) {
		//  Ignore if the mount is not tracking
		if (self.globalMode != kGlobalModeTracking || self.trackingMode == kTrackingModeOff)
			return;

		//  Get current position
		EncoderPoint actual;
		[self currentPosition:&actual];

		//  Get location
		double lngRad = [PreferencesController longitude];

		//  Get LST
		double lstRad = [AstroMath localSiderealTime:lngRad];

		//  Compute required HA
		double ha = lstRad - cp->ra;

		//  Compute unmapped target positions for RA/DEC
		EquatorialPoint eq = { ha, cp->dec, NO };   //  west:NO -->  this isn't actually used but we do this to silence a warning
		EncoderPoint target;
		[Coordinates equatorial:&eq toEncoder:&target];

		//  Do mean to observed on catalog

		//  Add point to SYNC manager
		SyncPoint* p = [SyncPoint new];
		p.lst = lstRad;
		p.catalog_ra = target.ha;
		p.catalog_dec = target.dec;
		p.actual_ra = actual.ha;
		p.actual_dec = actual.dec;
		p.active = true;
		[syncManager addPoint:p];

		printf("SYNC DELTA RA: %g    DEC: %g\n", p.actual_ra - p.catalog_ra, p.actual_dec - p.catalog_dec);

		//  User is telling us that current encoder positions (which are changing due to tracking) represent
		//  the RA/DEC of self.slewTarget.
		//
		//  In order to compute a SYNC-DELTA, we need to capture the current positions and current LST.
		//  Using current LST, we can compute what the target slew positions would have been at that LST, and
		//  we can compute the DELTA.
		//
		//  There may be a slight inaccuracy due to RA changing while we read the position. We can read DEC
		//  first as it doesn't change unless we are doing a custom tracking mode.
		//
		//  The crucial thing is getting as close a match between LST and current positions.

		//  Need to do a test to see how fast the RA encoder changes while tracking.


		//  1.  Read the current RA/DEC positions.
		//  2.  Get the LST
		//  3.  Compute predicted RA/DEC positions from LST.
		//  4.  Compute SYNC-DELTA-RA and SYNC-DELTA-DEC.

		//  5.  Repeat this process 2 more times to get 3 SYNC-DELTA pairs.

		//  With 3 pairs, we can form a model of the mount pointing mechanics and transform catalog coordinates into
		//  mount coordinates.

		//  We form matrices of the three points (actual and catalog).
		//  We determine T from A = [T]C
		//  T = A * Inv(C)

		//  We keep all the sync coordinates in encoder units. We do not need time to be stored.
	}
#endif
}
#endif
