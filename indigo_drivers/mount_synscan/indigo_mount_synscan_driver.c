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
#include <unistd.h>
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


#define GET_RELEASE(v)          (int)((v) & 0xFF)
#define GET_REVISION(v)				(int)(((v) >> 8) & 0xFF)
#define GET_MODEL(v)				(int)(((v) >> 16) & 0xFF)


bool synscan_configure(indigo_device* device) {
	//  Start with an assumption about each axis
	PRIVATE_DATA->raAxisMode = PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Get firmware version
	long version;
	if (!synscan_firmware_version(device, &version)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ERROR GETTING FIRMWARE");
		return false;
	}
	snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d", GET_RELEASE(version), GET_REVISION(version));
	snprintf(MOUNT_INFO_VENDOR_ITEM->text.value, INDIGO_VALUE_SIZE, "Sky-Watcher SynScan");
	switch (GET_MODEL(version)) {
	case 0x00:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ6"); break;
	case 0x01:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "HEQ5"); break;
	case 0x02:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ5"); break;
	case 0x03:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ3"); break;
	case 0x04:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "EQ8"); break;
	case 0x05:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZEQ6"); break;
	case 0x06:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZEQ5"); break;
	case 0x80:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "GT"); break;
	case 0x81:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "MF"); break;
	case 0x82:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "114GT"); break;
	case 0x90:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "DOB"); break;
	case 0xA5:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "AZGTi"); break;
	default:
			snprintf(MOUNT_INFO_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE, "CUSTOM"); break;
	}

	//  Query motor status
	long raMotorStatus = 0;
	long decMotorStatus = 0;
	if (!synscan_motor_status(device, kAxisRA, &raMotorStatus)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ERROR GETTING RA STATUS");
		return false;
	}
	if (!synscan_motor_status(device, kAxisDEC, &decMotorStatus)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "ERROR GETTING DEC STATUS");
		return false;
	}

	//  If the motors are moving, can we read back the config? Do we need to stop them?
	if ((raMotorStatus & kStatusActiveMask) != 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA MOTOR IS ACTIVE DURING CONFIGURATION!");
		synscan_stop_axis(device, kAxisRA);
	}
	if ((decMotorStatus & kStatusActiveMask) != 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "DEC MOTOR IS ACTIVE DURING CONFIGURATION!");
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Total Steps:  RA == %10lu   DEC == %10lu", PRIVATE_DATA->raTotalSteps, PRIVATE_DATA->decTotalSteps);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, " Worm Steps:  RA == %10lu   DEC == %10lu", PRIVATE_DATA->raWormSteps, PRIVATE_DATA->decWormSteps);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, " Timer Freq:  RA == %10lu   DEC == %10lu", PRIVATE_DATA->raTimerFreq, PRIVATE_DATA->decTimerFreq);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  HS Factor:  RA == %10lu   DEC == %10lu", PRIVATE_DATA->raHighSpeedFactor, PRIVATE_DATA->decHighSpeedFactor);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, " Polarscope:  %s", PRIVATE_DATA->canSetPolarscopeBrightness ? "YES" : "NO");
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "   Home Pos:  RA == %10lu   DEC == %10lu", RA_HOME_POSITION, DEC_HOME_POSITION);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "   Zero Pos:  RA == %10lu   DEC == %10lu", PRIVATE_DATA->raZeroPos, PRIVATE_DATA->decZeroPos);
	}

	//  Initialize motors if necessary
	if (raMotorStatus == kStatusNotInitialised || raMotorStatus == 0x000) {
		if (!synscan_init_axis(device, kAxisRA))
			return false;
		if (!synscan_motor_status(device, kAxisRA, &raMotorStatus))
			return false;
		if (raMotorStatus == kStatusNotInitialised) {
			//  This is a fatal error - can't get the motor to initialize
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "RA MOTOR FAILURE: WILL NOT INITIALIZE!");
			return false;
		}
		if (!synscan_init_axis_position(device, kAxisRA, RA_HOME_POSITION))
			return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA MOTOR INITIALIZED!");
	} else {
		//  if mount was previously configured - assume that configuration is still valid

		//  else this is also a fatal error - we don't know why the motors are not needing INIT
		//  maybe the software restarted??
		//  we could receover by configuring, but the user should be told that the assumption is the mount must be homed

		//  actually if the assumption is that the software restarted AND EQMac previously configured it, then we can recover
		//  by just reading the parameters and trusting the current position and computing where the home position should be

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA MOTOR OK %06X", raMotorStatus);
	}
	if (decMotorStatus == kStatusNotInitialised || decMotorStatus == 0x000) {
		if (!synscan_init_axis(device, kAxisDEC))
			return false;
		if (!synscan_motor_status(device, kAxisDEC, &decMotorStatus))
			return false;
		if (decMotorStatus == kStatusNotInitialised) {
			//  This is a fatal error - can't get the motor to initialize
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "DEC MOTOR FAILURE: WILL NOT INITIALIZE!");
			return false;
		}
		if (!synscan_init_axis_position(device, kAxisDEC, DEC_HOME_POSITION))
			return false;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "DEC MOTOR INITIALIZED!");
	} else {
		//  if mount was previously configured - assume that configuration is still valid

		//  else this is also a fatal error - we don't know why the motors are not needing INIT
		//  maybe the software restarted??
		//  we could receover by configuring, but the user should be told that the assumption is the mount must be homed
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "DEC MOTOR OK %06X", decMotorStatus);
	}

	//  Configure the mount modes
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;
	PRIVATE_DATA->globalMode = kGlobalModeIdle;

	//  Invalidate axis configurations
	PRIVATE_DATA->raAxisConfig.valid = false;
	PRIVATE_DATA->decAxisConfig.valid = false;

	//  Read the current position
	synscan_get_coords(device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Position: RA == %g, DEC == %g", PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition);

	//  Consider the mount configured once we reach here
	PRIVATE_DATA->mountConfigured = true;
	return true;
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

static bool synscan_configure_axis_for_rate(indigo_device* device, enum AxisID axis, double rate) {
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

bool synscan_update_axis_to_rate(indigo_device* device, enum AxisID axis, double rate, bool* result) {
	//  Compute config required
	struct AxisConfig requiredConfig;
	synscan_axis_config_for_rate(device, axis, rate, &requiredConfig);

	//  Determine if simple rate change is sufficient (or full reconfiguration)
	bool reconfigure = synscan_should_reconfigure_axis(device, axis, &requiredConfig);
	if (reconfigure) {
		*result = false;
		return true;
	} else {
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

bool synscan_guide_axis_at_rate(indigo_device* device, enum AxisID axis, double rate, int duration, double resume_rate) {
	//  Compute config required
	struct AxisConfig pulseConfig;
	struct AxisConfig resumeConfig;
	synscan_axis_config_for_rate(device, axis, rate, &pulseConfig);
	if (resume_rate != 0)
		synscan_axis_config_for_rate(device, axis, resume_rate, &resumeConfig);

#if 0
	//  Determine if simple rate change is sufficient (or full reconfiguration)
	bool reconfigure = synscan_should_reconfigure_axis(device, axis, &requiredConfig);
	if (reconfigure) {
		//*result = false;
		return true;
	} else {
		//*result = true;
	}
	
	//  Reference relevant axis config cache
	struct AxisConfig* cachedConfig = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisConfig : &PRIVATE_DATA->decAxisConfig;
	
	//  Invalidate gearing in case of error
	cachedConfig->valid = false;
#endif

	//  Do the pulse
	if (axis == kAxisRA)
		return synscan_guide_pulse_ra(device, pulseConfig.rateCode, duration, resumeConfig.rateCode);
	else
		return synscan_guide_pulse_dec(device, pulseConfig.direction, pulseConfig.rateCode, duration);

//	bool ok = synscan_set_axis_slew_rate(device, axis, requiredConfig.rateCode);
//	if (!ok)
//		return false;
//
//	//  Validate the axis config cache
//	*cachedConfig = requiredConfig;
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

void coords_encoder_to_eq(indigo_device* device, double ha_enc, double dec_enc, double* ha, double* dec, int* sop) {
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
	} else // decPos in [0.75, 1.0]
		*dec = epdec - 1.0;

	//  Adjust for southern hemisphere
	if (south) {
		west = !west;
		*dec = -*dec;
	}
	*dec *= 2.0 * M_PI;
	*sop = west ? MOUNT_SIDE_WEST : MOUNT_SIDE_EAST;

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
	} else {
		if (west)
			*ha = 0.5 - ha_enc;
		else if (ha_enc < 0.5)
			*ha = -ha_enc;
		else
			*ha = 1.0 - ha_enc;
	}
	if (*ha < -0.5)
		*ha += 1.0;
	if (*ha >= 0.5)
		*ha -= 1.0;

	//  Convert to radians
	*ha *= 2.0 * M_PI;
}

void coords_eq_to_encoder2(indigo_device* device, double ha, double dec, double haPos[], double decPos[]) {
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

	//  Convert DEC to Degrees of encoder rotation relative to 0 position
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
	assert(ha >= -M_PI && ha <= M_PI);	///  assertion if ha < -M_PI or ha > M_PI

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
//	if (haw < 0.5) {
		haPos[0] = haw;
		decPos[0] = degw;
		haPos[1] = hae;
		decPos[1] = dege;
//	} else {
//		haPos[0] = hae;
//		decPos[0] = dege;
//		haPos[1] = haw;
//		decPos[1] = degw;
//	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SOLUTIONS:");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  WEST:  %g,   %g", haPos[0], decPos[0]);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  EAST:  %g,   %g", haPos[1], decPos[1]);
}

void coords_aa_to_encoder2(indigo_device* device, double az, double alt, double azPos[], double altPos[]) {
	//  Generate the two ALT positions (east and west of meridian)
	double altw = M_PI - alt;
	double alte = alt;
	altw /= M_PI + M_PI;
	alte /= M_PI + M_PI;

	//  Rebase the angles to optimise the DEC slew relative to HOME
//	if (degw > M_PI + M_PI_2)
//		degw -= M_PI + M_PI;
//	if (dege > M_PI + M_PI_2)
//		dege -= M_PI + M_PI;
//	degw /= M_PI + M_PI;
//	dege /= M_PI + M_PI;

	//  Compute AZ positions to match each ALT position
	double azw = az - M_PI;
	double aze = (az <= M_PI) ? az : az - M_PI - M_PI;
	azw /= M_PI + M_PI;
	aze /= M_PI + M_PI;

	assert(azw < 0.5 || aze < 0.5);

	//  Decide whether EAST or WEST provides the "normal" / CW-Down slew and fill in the positions
	//	if (haw < 0.5) {
	azPos[0] = azw;
	altPos[0] = altw;
	azPos[1] = aze;
	altPos[1] = alte;
	//	} else {
	//		haPos[0] = hae;
	//		decPos[0] = dege;
	//		haPos[1] = haw;
	//		decPos[1] = degw;
	//	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SOLUTIONS:");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  WEST:  %g,   %g", azPos[0], altPos[0]);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  EAST:  %g,   %g", azPos[1], altPos[1]);
}


void synscan_get_coords(indigo_device *device) {
	long haPos, decPos;
	//  Get the DEC first since we want the HA position to be changing as little as possible till
	//  we combine it with LST to get RA
	if (synscan_axis_position(device, kAxisDEC, &decPos))
		PRIVATE_DATA->decPosition = dec_steps_to_position(device, decPos);
	if (synscan_axis_position(device, kAxisRA, &haPos))
		PRIVATE_DATA->raPosition = ha_steps_to_position(device, haPos);
	//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "POS DEBUG:  HA %ld   DEC %ld (steps)\n", haPos, decPos);
}

void synscan_stop_and_wait_for_axis(indigo_device* device, enum AxisID axis) {
	enum AxisMode* axisMode = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisMode : &PRIVATE_DATA->decAxisMode;
	if (*axisMode != kAxisModeIdle) {
		synscan_stop_axis(device, axis);

		long axisStatus;
		while (true) {
			//  Get the axis status
			if (!synscan_motor_status(device, axis, &axisStatus)) {
				//  FIXME - this is a serial connection issue - perhaps we should disconnect driver
				break;
			}

			//  Mark axis as stopped if motor has come to rest
			if ((axisStatus & kStatusActiveMask) == 0) {
				*axisMode = kAxisModeIdle;
				break;
			}

			//  Delay a little
			usleep(100000);
		}
	}
}

void synscan_slew_axis_at_rate(indigo_device* device, enum AxisID axis, double rate) {
	//		Stop axis if need be
	synscan_stop_and_wait_for_axis(device, axis);

	//    Start slewing at specified rate
	if (!synscan_configure_axis_for_rate(device, axis, rate) || !synscan_slew_axis(device, axis)) {
		//  FIXME - this is a serial connection issue - perhaps we should disconnect driver
	}
}

bool synscan_slew_axis_to_position(indigo_device* device, enum AxisID axis, double position) {
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
		//			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Doing ANTI-BACKLASH slew extension");
		//		}

		AxisPosition slowdown = delta - 80000;
		if (slowdown < 0)
			slowdown = delta / 2;

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SLEW DEBUG:  axis %s  current pos %ld  requested pos %ld   delta %ld   slowdown %ld\n", axis == kAxisRA ? "RA" : "DEC", currentPosition, steps, delta, slowdown);

		//  Invalidate axis config since we change to absolute slew mode
		cachedConfig->valid = false;

		//  Always keep the direction up to date for backlash detection
		cachedConfig->direction = d;

		//  Initiate the slew
		ok = ok && synscan_set_axis_gearing(device, axis, d, kAxisSpeedAbsSlew);
		ok = ok && synscan_set_axis_step_count(device, axis, delta);
		ok = ok && synscan_set_axis_slowdown(device, axis, slowdown);
		ok = ok && synscan_slew_axis(device, axis);
		enum AxisMode* axisMode = (axis == kAxisRA) ? &PRIVATE_DATA->raAxisMode : &PRIVATE_DATA->decAxisMode;
		*axisMode = kAxisModeSlewing;
	}
	return ok;
}

int synscan_select_best_encoder_point(indigo_device* device, double haPos[], double decPos[]) {
	//  Check if we're allowed to do CW-UP slews
//	NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];
//	NSInteger ra_slew_priority = [prefs integerForKey:@"ra_slew_priority"];
//	bool limitsEnabled = [[prefs objectForKey:@"limits_enabled"] boolValue];

	//  Return the SAFE point if CW-UP slews are disabled or limits are disabled in general
	//if (ra_slew_priority == 0 || !limitsEnabled)
	//	return &eps[0];
	if (haPos[0] <= 0.5)
		return 0;
	else
		return 1;

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
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CWUP HA IS OUTSIDE LIMITS - CANNOT SLEW CW UP");
						return &eps[0];
					}

	//  Check if the CW-UP point is desirable over the normal point
	bool south = [PreferencesController latitude] < 0.0;
	if (!south) {
		//  Desirable if time to flip (reach 0.5) is better
		if (0.5 - cwup_h > 0.5 - eps[0].ha) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CWUP SLEW IS DESIRABLE!");
			return &eps[1];
		}
	} else {
		//  Desirable if time to flip (reach 0.0) is better
		if (cwup_h > eps[0].ha) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CWUP SLEW IS DESIRABLE!");
			return &eps[1];
		}
	}

	//  Return the safe position
	return &eps[0];
#endif
}

int synscan_select_best_encoder_point_aa(indigo_device* device, double azPos[], double altPos[]) {
	if (azPos[0] >= 0.0)
		return 0;
	else
		return 1;
}

void synscan_wait_for_axis_stopped(indigo_device* device, enum AxisID axis, bool* abort) {
	long axisStatus;
	while (true) {
		//  Abort if needed
		if (abort && *abort)
			break;

		//  Poll the axis status
		if (!synscan_motor_status(device, axis, &axisStatus))
			break;
		//  Mark axis as stopped if motor has come to rest
		//  Need to be careful here as if the axis has not started moving yet, we might think it has stopped right away
		if ((axisStatus & kStatusActiveMask) == 0) {
			break;
		}
		usleep(100000);
	}
}
