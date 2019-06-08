// SynScan simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
// All rights reserved.
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
//
// Code is based on Skywatcher protocol simulator for INDI driver
// Copyright 2012 Geehalel (geehalel AT gmail DOT com)

enum MOTOR_STATUS {
  RUNNING                           = 0x0001,
  SLEWMODE                          = 0X0010,
  BACKWARD                          = 0x0020,
  HIGHSPEED                         = 0x0040,
  INITIALIZED                       = 0x0100
};

enum EXT_INQUIRY_CMD {
  GET_INDEXER_CMD                   = 0x0000,
  GET_FEATURES_CMD                  = 0x0001
};

enum EXT_SETTING_CMD {
  START_PPEC_TRAINING_CMD            = 0x0000,
  STOP_PPEC_TRAINING_CMD             = 0x0001,
  TURN_PPEC_ON_CMD                   = 0x0002,
  TURN_PPEC_OFF_CMD                  = 0X0003,
  ENCODER_ON_CMD                     = 0x0004,
  ENCODER_OFF_CMD                    = 0x0005,
  DISABLE_FULL_CURRENT_LOW_SPEED_CMD = 0x0006,
  ENABLE_FULL_CURRENT_LOW_SPEED_CMD  = 0x0106,
  RESET_HOME_INDEXER_CMD             = 0x0008
};

static const char *version;
static uint32_t ra_nb_teeth;
static uint32_t de_nb_teeth;
static uint32_t ra_gear_ratio_num;
static uint32_t ra_gear_ratio_den;
static uint32_t de_gear_ratio_num;
static uint32_t de_gear_ratio_den;
static uint32_t ra_nb_steps;
static uint32_t de_nb_steps;
static uint32_t ra_microsteps;
static uint32_t de_microsteps;
static uint32_t ra_highspeed_ratio;
static uint32_t de_highspeed_ratio;

static uint32_t ra_steps_360;
static uint32_t de_steps_360;
static uint32_t ra_steps_worm;
static uint32_t de_steps_worm;

static uint32_t ra_position;
static uint8_t ra_microstep;
static uint8_t ra_pwm_index;
static uint32_t ra_worm_period;
static uint32_t ra_target;
static uint32_t ra_target_current;
static uint32_t ra_target_slow;
static uint32_t ra_breaks;

static uint32_t de_position;
static uint8_t de_microstep;
static uint8_t de_pwm_index;
static uint32_t de_worm_period;
static uint32_t de_target;
static uint32_t de_target_current;
static uint32_t de_target_slow;
static uint32_t de_breaks;

// Motor periods in ns
static uint32_t ra_period;
static uint32_t de_period;

// Motor status (Skywatcher protocol)
static uint32_t ra_status;
static uint32_t de_status;

// USART
static char reply[32];
static uint8_t reply_index;
static uint8_t read;
static uint32_t last_ra_time;
static uint32_t last_de_time;
static char hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// features
static uint32_t ra_features = 0;
static uint32_t de_features = 0;

// indexer
static uint32_t ra_indexer = 0;
static uint32_t de_indexer = 0;

static void send_byte(uint8_t c) {
  reply[reply_index++] = c;
}

static void send_string(const char *s) {
  while (*s) {
    send_byte(*s++);
  }
}

static void send_u24(uint32_t n) {
  send_byte(hexa[(n & 0xF0) >> 4]);
  send_byte(hexa[(n & 0x0F)]);
  send_byte(hexa[(n & 0xF000) >> 12]);
  send_byte(hexa[(n & 0x0F00) >> 8]);
  send_byte(hexa[(n & 0xF00000) >> 20]);
  send_byte(hexa[(n & 0x0F0000) >> 16]);
}

static void send_u12(uint32_t n) {
  send_byte(hexa[(n & 0xF0) >> 4]);
  send_byte(hexa[(n & 0x0F)]);
  send_byte(hexa[(n & 0xF00) >> 8]);
}

static void send_u8(uint8_t n) {
  send_byte(hexa[(n & 0xF0) >> 4]);
  send_byte(hexa[(n & 0x0F)]);
}

static uint32_t get_u8(const char *cmd) {
  uint32_t res = 0;
  res |= HEX(cmd[3]);
  res <<= 4;
  res |= HEX(cmd[4]);
  read += 2;
  return res;
}

static uint32_t get_u24(const char *cmd) {
  uint32_t res = 0;
  res = HEX(cmd[7]);
  res <<= 4;
  res |= HEX(cmd[8]);
  res <<= 4;
  res |= HEX(cmd[5]);
  res <<= 4;
  res |= HEX(cmd[6]);
  res <<= 4;
  res |= HEX(cmd[3]);
  res <<= 4;
  res |= HEX(cmd[4]);
  read += 6;
  return res;
}

void setup_version(const char *mcversion) {
  version = mcversion;
}

void setup_axes(uint32_t nb_teeth, uint32_t gear_ratio_num, uint32_t gear_ratio_den, uint32_t nb_steps, uint32_t nb_microsteps, uint32_t highspeed) {
  ra_nb_teeth        = de_nb_teeth        = nb_teeth;
  ra_gear_ratio_num  = de_gear_ratio_num  = gear_ratio_num;
  ra_gear_ratio_den  = de_gear_ratio_den  = gear_ratio_den;
  ra_nb_steps        = de_nb_steps        = nb_steps;
  ra_microsteps      = de_microsteps      = nb_microsteps;
  ra_position        = de_position        = 0x800000;
  ra_microstep       = de_microstep       = 0x00;
  ra_worm_period      = de_worm_period      = 0x256;
  ra_target          = de_target          = 0x000001;
  ra_target_current  = de_target_current  = 0x000000;
  ra_target_slow     = de_target_slow     = 400;
  ra_breaks          = de_breaks          = 400;
  ra_status          = de_status          = 0X0010; // lowspeed, forward, slew mode, stopped
  ra_highspeed_ratio = ra_microsteps / highspeed;
  de_highspeed_ratio = de_microsteps / highspeed;
  ra_steps_360       = (ra_nb_teeth * ra_gear_ratio_num * ra_nb_steps * ra_microsteps) / ra_gear_ratio_den;
  de_steps_360       = (de_nb_teeth * de_gear_ratio_num * de_nb_steps * de_microsteps) / de_gear_ratio_den;
  ra_steps_worm      = (ra_gear_ratio_num * ra_nb_steps * ra_microsteps) / ra_gear_ratio_den;
  de_steps_worm      = (de_gear_ratio_num * de_nb_steps * de_microsteps) / de_gear_ratio_den;
  ra_pwm_index        = ra_microsteps - 1;
  de_pwm_index        = de_microsteps - 1;
  last_ra_time = millis();
  last_de_time = millis();
}

void setup_features(uint32_t features) {
  ra_features = de_features = features;
}

static void compute_timer_ra(uint32_t worm_period) {
  uint32_t n = (worm_period * MUL_RA);
  n += ((worm_period * REM_RA) / ra_steps_worm);
  ra_period = n;
}

static void compute_timer_de(uint32_t worm_period) {
  uint32_t n = (worm_period * MUL_DE);
  n += ((worm_period * REM_DE) / de_steps_worm);
  de_period = n;
}

static void compute_ra_position() {
  uint32_t raTime, resTime;
  raTime = millis();
  resTime = raTime - last_ra_time;
  if (GETMOTORPROPERTY(ra_status, RUNNING)) {
    uint32_t stepmul = (GETMOTORPROPERTY(ra_status, HIGHSPEED)) ? ra_highspeed_ratio : 1;
    uint32_t deltastep;
    deltastep = resTime * 1000 * stepmul / ra_period;
    if (!(GETMOTORPROPERTY(ra_status, SLEWMODE))) { // GOTO
      if ((GETMOTORPROPERTY(ra_status, HIGHSPEED)) && (ra_target_current + deltastep >= ra_target - ra_target_slow)) {
        uint32_t hstime, lstime;
        //hstime: time moving @ highspeed
        hstime = ((ra_target - ra_target_slow - ra_target_current) * ra_period) / stepmul;
        lstime = resTime - hstime;
        UNSETMOTORPROPERTY(ra_status, HIGHSPEED); //switch to low speed
        deltastep = ra_target - ra_target_slow - ra_target_current + lstime * 1000 / ra_period;
      }
      if (ra_target_current + deltastep >= ra_target) {
        deltastep = ra_target - ra_target_current;
        SETMOTORPROPERTY(ra_status, SLEWMODE);
        ra_pause();
      } else {
        ra_target_current += deltastep;
      }
    }
    if (GETMOTORPROPERTY(ra_status, BACKWARD)) { //BACKWARD
      ra_position = ra_position - deltastep;
    } else {
      ra_position = ra_position + deltastep;
    }
  }
  last_ra_time = raTime;
}

static void compute_de_position() {
  uint32_t deTime, resTime;
  deTime = millis();
  resTime = deTime - last_de_time;
  if (GETMOTORPROPERTY(de_status, RUNNING)) {
    uint32_t stepmul = (GETMOTORPROPERTY(de_status, HIGHSPEED)) ? de_highspeed_ratio : 1;
    uint32_t deltastep;
    deltastep = resTime * 1000 * stepmul / de_period;
    if (!(GETMOTORPROPERTY(de_status, SLEWMODE))) { // GOTO
      if ((GETMOTORPROPERTY(de_status, HIGHSPEED)) && (de_target_current + deltastep >= de_target - de_target_slow)) {
        uint32_t hstime, lstime;
        //hstime: time moving @ highspeed
        hstime = ((de_target - de_target_slow - de_target_current) * de_period) / stepmul;
        lstime = resTime - hstime;
        UNSETMOTORPROPERTY(de_status, HIGHSPEED); //switch to low speed
        deltastep = (de_target - de_target_slow - de_target_current) + lstime * 1000 / de_period;
      }
      if (de_target_current + deltastep >= de_target) {
        deltastep = de_target - de_target_current;
        SETMOTORPROPERTY(de_status, SLEWMODE);
        de_pause();
      } else {
        de_target_current += deltastep;
      }
    }
    if (GETMOTORPROPERTY(de_status, BACKWARD)) { //BACKWARD
      de_position = de_position - deltastep;
    } else {
      de_position = de_position + deltastep;
    }
  }
  last_de_time = deTime;
}

static void ra_resume() {
  last_ra_time = millis();
  compute_timer_ra(ra_worm_period);
  if (!(GETMOTORPROPERTY(ra_status, SLEWMODE)))
    ra_target_current = 0;
  SETMOTORPROPERTY(ra_status, RUNNING);
}

static void de_resume() {
  last_de_time = millis();
  compute_timer_de(de_worm_period);
  if (!(GETMOTORPROPERTY(de_status, SLEWMODE)))
    de_target_current = 0;
  SETMOTORPROPERTY(de_status, RUNNING);
}

static void ra_pause() {
  UNSETMOTORPROPERTY(ra_status, RUNNING);
}

static void ra_stop() {
  UNSETMOTORPROPERTY(ra_status, RUNNING);
}

static void de_pause() {
  UNSETMOTORPROPERTY(de_status, RUNNING);
}

static void de_stop() {
  UNSETMOTORPROPERTY(de_status, RUNNING);
}

char *process_command(const char *cmd) {
  reply_index = 0;
  read = 1;
  if (cmd[0] != ':') {
    send_byte('!');
    goto next_cmd;
  }
  switch (cmd[1]) {
    /** Syntrek Protocol **/
    case 'e': // Get firmware version
      if (cmd[2] != '1') {
        goto cant_do;
      } else {
        send_byte('=');
        send_string(version);
      }
      break;
    case 'a': // Get number of microsteps per revolution
      if (cmd[2] == '1') {
        send_byte('=');
        send_u24(ra_steps_360);
      } else if (cmd[2] == '2') {
        send_byte('=');
        send_u24(de_steps_360);
      } else {
        goto cant_do;
      }
      break;
    case 'b': // Get number of microsteps per revolution
    case 's': // Get Pec period
      if (cmd[2] == '1') {
        send_byte('=');
        send_u24(ra_steps_worm);
      } else if (cmd[2] == '2') {
        send_byte('=');
        send_u24(de_steps_worm);
      } else {
        goto cant_do;
      }
      break;
    case 'D': // Get Worm period
      if (cmd[2] == '1') {
        send_byte('=');
        send_u24(ra_worm_period);
      } else if (cmd[2] == '2') {
        send_byte('=');
        send_u24(de_worm_period);
      } else {
        goto cant_do;
      }
      break;
    case 'f': // Get motor status
      if (cmd[2] == '1') {
        send_byte('=');
        send_u12(ra_status);
      } else if (cmd[2] == '2') {
        send_byte('=');
        send_u12(de_status);
      } else {
        goto cant_do;
      }
      break;
    case 'j': // Get encoder values
      if (cmd[2] == '1') {
        compute_ra_position();
        send_byte('=');
        send_u24(ra_position);
      } else if (cmd[2] == '2') {
        compute_de_position();
        send_byte('=');
        send_u24(de_position);
      } else {
        goto cant_do;
      }
      break;
    case 'g': // get high speed ratio
      if (cmd[2] == '1') {
        send_byte('=');
        send_u24(ra_highspeed_ratio);
      } else if (cmd[2] == '2') {
        send_byte('=');
        send_u24(de_highspeed_ratio);
      } else {
        goto cant_do;
      }
      break;
    case 'E': // Set encoder values
      if (cmd[2] == '1') {
        ra_position = get_u24(cmd);
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_position = get_u24(cmd);
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'F': // Initialize & activate motors
      if ((cmd[2] != '1') && (cmd[2] != '2') && (cmd[2] != '3')) {
        goto cant_do;
      }
      //init_pwm();
      //init_timers();
      if ((cmd[2] == '1') || (cmd[2] == '3')) {
        //SETRAPWMA(pwm_table[ra_pwm_index]); SETRAPWMB(pwm_table[MICROSTEPS - 1 - ra_pwm_index]);
        //SETRAPHIA(ra_microstep); SETRAPHIB(ra_microstep);
        SETMOTORPROPERTY(ra_status, INITIALIZED);
        //RAENABLE = 1;
      }
      if ((cmd[2] == '2') || (cmd[2] == '3')) {
        //SETDEPWMA(pwm_table[de_pwm_index]); SETDEPWMB(pwm_table[MICROSTEPS - 1 - de_pwm_index]);
        //SETDEPHIA(de_microstep); SETDEPHIB(de_microstep);
        SETMOTORPROPERTY(de_status, INITIALIZED);
        //DEENABLE = 1;
      }
      send_byte('=');
      break;
    case 'J': // Start motor
      if (cmd[2] == '1') {
        ra_resume();
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_resume();
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'K': // Stop motor
      if (cmd[2] == '1') {
        ra_pause();
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_pause();
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'L': // Instant Stop motor
      if (cmd[2] == '1') {
        ra_stop();
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_stop();
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'I': // Set Speed
      if (cmd[2] == '1') {
        ra_worm_period = get_u24(cmd);
        if (GETMOTORPROPERTY(ra_status, RUNNING)) {
            compute_timer_ra(ra_worm_period);
        }
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_worm_period = get_u24(cmd);
        if (GETMOTORPROPERTY(de_status, RUNNING)) {
          compute_timer_de(de_worm_period);
        }
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'G': // Set Mode/Direction
      if ((cmd[2] != '1') && (cmd[2] != '2')) {
        goto cant_do;
      }
      if (cmd[2] == '1') {
        uint8_t modedir = get_u8(cmd);
        if (modedir & 0x0F) {
          SETMOTORPROPERTY(ra_status, BACKWARD);
        } else {
          UNSETMOTORPROPERTY(ra_status, BACKWARD);
        }
        modedir = (modedir >> 4);
        if (modedir == 0) {
          UNSETMOTORPROPERTY(ra_status, SLEWMODE);
          SETMOTORPROPERTY(ra_status, HIGHSPEED);
        } else if (modedir == 1) {
          SETMOTORPROPERTY(ra_status, SLEWMODE);
          UNSETMOTORPROPERTY(ra_status, HIGHSPEED);
        } else if (modedir == 2) {
          UNSETMOTORPROPERTY(ra_status, SLEWMODE);
          UNSETMOTORPROPERTY(ra_status, HIGHSPEED);
        } else if (modedir == 3) {
          SETMOTORPROPERTY(ra_status, SLEWMODE);
          SETMOTORPROPERTY(ra_status, HIGHSPEED);
        }
      }
      if (cmd[2] == '2') {
        uint8_t modedir = get_u8(cmd);
        if (modedir & 0x0F) {
          SETMOTORPROPERTY(de_status, BACKWARD);
        } else {
          UNSETMOTORPROPERTY(de_status, BACKWARD);
        }
        modedir = (modedir >> 4);
        if (modedir == 0) {
          UNSETMOTORPROPERTY(de_status, SLEWMODE);
          SETMOTORPROPERTY(de_status, HIGHSPEED);
        } else if (modedir == 1) {
          SETMOTORPROPERTY(de_status, SLEWMODE);
          UNSETMOTORPROPERTY(de_status, HIGHSPEED);
        } else if (modedir == 2) {
          UNSETMOTORPROPERTY(de_status, SLEWMODE);
          UNSETMOTORPROPERTY(de_status, HIGHSPEED);
        } else if (modedir == 3) {
          SETMOTORPROPERTY(de_status, SLEWMODE);
          SETMOTORPROPERTY(de_status, HIGHSPEED);
        }
      }
      send_byte('=');
      break;
    case 'H': // Set Goto Target
      if (cmd[2] == '1') {
        ra_target = get_u24(cmd);
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_target = get_u24(cmd);
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'M': // Set Goto BreakSteps
      if (cmd[2] == '1') {
        ra_target_slow = get_u24(cmd);
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_target_slow = get_u24(cmd);
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'U': // Set BreakSteps
      if (cmd[2] == '1') {
        ra_breaks = get_u24(cmd);
        send_byte('=');
      } else if (cmd[2] == '2') {
        de_breaks = get_u24(cmd);
        send_byte('=');
      } else {
        goto cant_do;
      }
      break;
    case 'P': // Set ST4 guide Rate
      send_byte('=');
      break;
    case 'V': // Set Polar Scope LED brightness
      send_byte('=');
      break;
    case 'q': // Extended inquiry
      if (ra_features || de_features) {
        uint32_t command = get_u24(cmd);
        switch (command) {
          case GET_INDEXER_CMD:
            if (cmd[2] == '1') {
              send_byte('=');
              send_u24(ra_indexer);
            } else if (cmd[2] == '2') {
              send_byte('=');
              send_u24(de_indexer);
            } else {
              goto cant_do;
            }
            break;
          case GET_FEATURES_CMD:
            if (cmd[2] == '1') {
              send_byte('=');
              send_u24(ra_features);
            } else if (cmd[2] == '2') {
              send_byte('=');
              send_u24(de_features);
            } else {
              goto cant_do;
            }
            break;
          default:
            goto cant_do;
        }
      } else {
        goto cant_do;
      }
      break;
    case 'W': // Extended setting
      if (ra_features || de_features) {
        uint32_t command = get_u24(cmd);
        switch (command) {
          case START_PPEC_TRAINING_CMD:
          case STOP_PPEC_TRAINING_CMD:
          case TURN_PPEC_ON_CMD:
          case TURN_PPEC_OFF_CMD:
          case ENCODER_ON_CMD:
          case ENCODER_OFF_CMD:
          case DISABLE_FULL_CURRENT_LOW_SPEED_CMD:
          case ENABLE_FULL_CURRENT_LOW_SPEED_CMD:
            send_byte('=');
            break;
          case RESET_HOME_INDEXER_CMD:
            if (cmd[2] == '1') {
              ra_indexer = 0;
              send_byte('=');
            } else if (cmd[2] == '2') {
              de_indexer = 0;
              send_byte('=');
            } else {
              goto cant_do;
            }
            break;
          default:
            goto cant_do;
        }
      } else {
        goto cant_do;
      }
      break;
    default:
      goto cant_do;
  }
  read += 3; // 2 + '\r'
  goto next_cmd;
cant_do:
  send_byte('!'); // Can't execute command
next_cmd:
  send_byte('\x0d');
  reply[reply_index] = '\0';
  return reply;
}
