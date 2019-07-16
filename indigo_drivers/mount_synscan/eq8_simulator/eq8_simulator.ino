// SkyWatcher EQ8 simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

//#define DEBUG
#define LCD

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

#define TMR_FREQ              1 
#define HIGHSPEED_STEPS       128
#define WORM_STEPS            4 * 3600
#define STEPS_PER_REVOLUTION  (60 * WORM_STEPS)
#define FEATURES              HAS_ENCODER | HAS_PPEC | HAS_HOME_INDEXER | HAS_COMMON_SLEW_START | HAS_HALF_CURRENT_TRACKING

#define HEX(c)                (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

enum MOTOR_STATUS {
  RUNNING     = 0x0001,
  BLOCKED     = 0x0002,
  TRACKING    = 0X0010,
  BACKWARD    = 0x0020,
  HIGHSPEED   = 0x0040,
  INITIALIZED = 0x0100,
  LEVEL_ON    = 0x0300
};

enum EXT_INQUIRY_CMD {
  GET_INDEXER_CMD   = 0x0000,
  GET_FEATURES_CMD  = 0x0001
};

enum FEATURE {
  HAS_ENCODER               = 0x0001,
  HAS_PPEC                  = 0x0002,
  HAS_HOME_INDEXER          = 0x0004,
  IS_AZEQ                   = 0x0008,
  IN_PPEC_TRAINING          = 0x0010,
  IN_PPEC                   = 0x0020,
  HAS_POLAR_LED             = 0x1000,
  HAS_COMMON_SLEW_START     = 0x2000,
  HAS_HALF_CURRENT_TRACKING = 0x4000
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

static char hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static uint32_t axis_timer[2] = { 0, 0 };
static uint32_t axis_t1[2] = { 25, 25 };
static uint16_t axis_status[2] = { 0, 0 };
static uint32_t axis_position[2] = { 0x800000, 0x800000 };
static uint32_t axis_target[2] = { 0, 0 };
static uint32_t axis_home_index[2] = { 0, 0 };

static char *reply_8(uint8_t n) {
  static char buffer[8] = "=00";
  buffer[1] = hexa[(n & 0xF0) >> 4];
  buffer[2] = hexa[(n & 0x0F)];
  return buffer;
}

static char *reply_12(uint16_t n) {
  static char buffer[8] = "=000";
  buffer[1] = hexa[(n & 0xF0) >> 4];
  buffer[2] = hexa[(n & 0x0F)];
  buffer[3] = hexa[(n & 0xF00) >> 8];
  return buffer;
}

static char *reply_24(uint32_t n) {
  static char buffer[8] = "=000000";
  buffer[1] = hexa[(n & 0xF0) >> 4];
  buffer[2] = hexa[(n & 0x0F)];
  buffer[3] = hexa[(n & 0xF000) >> 12];
  buffer[4] = hexa[(n & 0x0F00) >> 8];
  buffer[5] = hexa[(n & 0xF00000) >> 20];
  buffer[6] = hexa[(n & 0x0F0000) >> 16];
  return buffer;
}

static uint32_t parse_24(const char *buffer) {
  uint32_t result = HEX(buffer[4]);
  result = (result << 4) | HEX(buffer[5]);
  result = (result << 4) | HEX(buffer[2]);
  result = (result << 4) | HEX(buffer[3]);
  result = (result << 4) | HEX(buffer[0]);
  result = (result << 4) | HEX(buffer[1]);
  return result;
}

static uint8_t parse_8(const char *buffer) {
  return (HEX(buffer[0]) << 4) | HEX(buffer[1]);
}


static char *process_command(char *buffer) {
  if (buffer[0] != ':')
    return "!3";
  if (buffer[2] != '1' && buffer[2] != '2')
    return "!0";
  int axis = buffer[2] - '1';
  switch (buffer[1]) {
    case 'B':
      axis_status[axis] = 0;
      return "=";
//    case 'D':
//      return reply_24(...);
    case 'E':
      axis_position[axis] = parse_24(buffer + 3);
      return "=";
    case 'G': {
      if (axis_status[axis] & RUNNING)
        return "!2";
      uint8_t mode = parse_8(buffer + 3);
      if (mode & 0x01)
        axis_status[axis] |= BACKWARD;
      else
        axis_status[axis] &= ~BACKWARD;
      switch (mode >> 4) {
        case 0:
          axis_status[axis] &= ~TRACKING;
          axis_status[axis] |= HIGHSPEED;
          break;
        case 1:
          axis_status[axis] |= TRACKING;
          axis_status[axis] &= ~HIGHSPEED;
          break;
        case 2:
          axis_status[axis] &= ~TRACKING;
          axis_status[axis] &= ~HIGHSPEED;
          break;
        case 3:
          axis_status[axis] |= TRACKING;
          axis_status[axis] |= HIGHSPEED;
          break;
      }
      return "=";
    }
    case 'F':
      axis_status[axis] |= INITIALIZED;
      return "=";
    case 'H':
      axis_t1[axis] = 1;
      if (axis_status[axis] & BACKWARD)
        axis_target[axis] = axis_position[axis] - parse_24(buffer + 3);
      else
        axis_target[axis] = axis_position[axis] + parse_24(buffer + 3);
      return "=";
    case 'I':
      axis_t1[axis] = parse_24(buffer + 3);
      return "=";
    case 'J':
      axis_status[axis] |= RUNNING;
      return "=";
    case 'K':
    case 'L':
      axis_status[axis] &= ~RUNNING;
      return "=";
    case 'M':
      return "=";
    case 'O':
      return "=";
//    case 'P':
//      return "=";
    case 'S':
      if (axis_status[axis] & RUNNING)
        return "!2";
      axis_target[axis] = parse_24(buffer + 3);
      return "=";
//    case 'T':
//      return "=";
//    case 'U':
//      return "=";
    case 'V':
      return "=";
    case 'W': {
      switch (parse_24(buffer + 3)) {      
        case START_PPEC_TRAINING_CMD:
          //TBD
          return "=";
        case STOP_PPEC_TRAINING_CMD:
          //TBD
          return "=";
        case TURN_PPEC_ON_CMD:
          //TBD
          return "=";
        case TURN_PPEC_OFF_CMD:
          //TBD
          return "=";
        case ENCODER_ON_CMD:
          //TBD
          return "=";
        case ENCODER_OFF_CMD:
          //TBD
          return "=";
        case DISABLE_FULL_CURRENT_LOW_SPEED_CMD:
          //TBD
          return "=";
        case ENABLE_FULL_CURRENT_LOW_SPEED_CMD:
          //TBD
          return "=";
        case RESET_HOME_INDEXER_CMD:
          //TBD
          return "=";
      }
      return "!0";
    }
    case 'a':
      return reply_24(STEPS_PER_REVOLUTION);
    case 'b':
      return reply_24(1000 / TMR_FREQ);
//    case 'c':
//      return reply_24(...);
//    case 'd':
//      return reply_24(...);
    case 'e':
      return "=020304";
    case 'f':
      return reply_12(axis_status[axis]);
    case 'g':
      return reply_8(HIGHSPEED_STEPS);
//    case 'h':
//      return reply_24(...);
//    case 'i':
//      return reply_24(...);
    case 'j':
      return reply_24(axis_position[axis]);
//    case 'k':
//      return reply_24(...);
//    case 'm':
//      return reply_24(...);
    case 'q': {
      uint32_t id = parse_24(buffer + 3);
      switch (id) {
        case GET_INDEXER_CMD:
          return reply_24(axis_home_index[axis]);
        case GET_FEATURES_CMD:
          return reply_24(FEATURES);
      }
      return "!0";
    }
    case 's':
      return reply_24(WORM_STEPS);
  }
  return "!0";
}

static void process_axis_timer(uint8_t axis) {
  if (++axis_timer[axis] > axis_t1[axis]) {
    axis_timer[axis] = 0;
    uint8_t status = axis_status[axis];
    if (status & RUNNING) {
      uint32_t steps = status & HIGHSPEED ? HIGHSPEED_STEPS : 1;
      if (status & TRACKING) {
        if (status & BACKWARD)
          axis_position[axis] -= steps;
        else
          axis_position[axis] += steps;
      } else {
        if (axis_position[axis] > axis_target[axis]) {
          axis_position[axis] -= HIGHSPEED_STEPS;
          if (axis_position[axis] <= axis_target[axis])
            axis_position[axis] = axis_target[axis];
        } else if (axis_position[axis] < axis_target[axis]) {
          axis_position[axis] += HIGHSPEED_STEPS;
          if (axis_position[axis] >= axis_target[axis])
            axis_position[axis] = axis_target[axis];
        } else {
          axis_status[axis] &= ~RUNNING;
        }
      }
    }
#ifdef LCD
    char buffer[17];
    sprintf(buffer, "%06x %06x %02x", axis_position[axis], axis_target[axis], status & 0xFF);
    lcd.setCursor(0, axis);
    lcd.print(buffer);
#endif
  }
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("EQ8 simulator");
  lcd.setCursor(0, 1);
  lcd.print("Not connected");
#endif
  while (!Serial)
    ;
}

void loop() {
  static uint32_t last_millis = 0;
  uint32_t current_millis = millis();
  if (current_millis - last_millis > TMR_FREQ) {
    process_axis_timer(0);
    process_axis_timer(1);
  }  
  if (Serial.available()) {
    char buffer[16];
    memset(buffer, 0, sizeof(buffer));
    Serial.readBytesUntil('\r', buffer, sizeof(buffer));
#ifdef DEBUG
    Serial.write(buffer);
    Serial.write(" -> ");
#endif
    Serial.write(process_command(buffer));
#ifdef DEBUG
    Serial.write('\n');
#else    
    Serial.write('\r');
#endif
  }
}
