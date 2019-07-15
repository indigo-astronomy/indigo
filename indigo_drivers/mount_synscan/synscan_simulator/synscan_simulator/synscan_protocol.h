// SynScan simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
// All rights reserved.
//
// Code is based on Skywatcher protocol simulator for INDI driver
// Copyright 2012 Geehalel (geehalel AT gmail DOT com)
//
// The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.


/* Microstepping */
/* 8 microsteps */
#define MICROSTEP_MASK 0x07
// PWM_MASK = 0x1 << LN(MICROSTEPS)
#define PWM_MASK 0x08
// WINDINGB_MASK = 0x2 << LN(MICROSTEPS)
#define WINDINGB_MASK 0x10
// WINDINGA_MASK = 0x3 << LN(MICROSTEPS)
#define WINDINGA_MASK 0x18

#define SETRAPHIA(microstep) RAPHIA = ((((microstep & WINDINGA_MASK) == 0) || ((microstep & WINDINGA_MASK) == WINDINGA_MASK)) ? 1 : 0)
#define SETRAPHIB(microstep) RAPHIB = ((microstep & WINDINGB_MASK) ? 1 : 0)

#define SETDEPHIA(microstep) DEPHIA = (((microstep & WINDINGA_MASK) == 0) || ((microstep & WINDINGA_MASK) == WINDINGA_MASK)) ? 1 : 0
#define SETDEPHIB(microstep) DEPHIB = (microstep & WINDINGB_MASK) ? 1 : 0

/* Timer period */
#define MUL_RA       (1000 / ra_steps_worm)
#define REM_RA       (1000 % ra_steps_worm)
#define MUL_DE       (1000 / de_steps_worm)
#define REM_DE       (1000 % de_steps_worm)

#define SETMOTORPROPERTY(motorstatus, property)   motorstatus |= property
#define UNSETMOTORPROPERTY(motorstatus, property) motorstatus &= ~property
#define GETMOTORPROPERTY(motorstatus, property)   (motorstatus & property)

#define HEX(c) (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

enum FEATURE {
  HAS_ENCODER                 = 0x0001,
  HAS_PPEC                    = 0x0002,
  HAS_HOME_INDEXER            = 0x0004,
  IS_AZEQ                     = 0x0008,
  IN_PPEC_TRAINING            = 0x0010,
  IN_PPEC                     = 0x0020,
  HAS_POLAR_LED               = 0x1000,
  HAS_COMMON_SLEW_START       = 0x2000,  // supports :J3
  HAS_HALF_CURRENT_TRACKING   = 0x4000
};

extern void setup_version(const char *mcversion);
extern void setup_axes(uint32_t nb_teeth, uint32_t gear_ratio_num, uint32_t gear_ratio_den, uint32_t nb_steps, uint32_t nb_microsteps, uint32_t highspeed);
extern void setup_features(uint32_t mask);
extern char *process_command(const char *cmd);
