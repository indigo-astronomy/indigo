// Rigel Systems nSTEP focuser simulator for Arduino
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
// All rights reserved.
//
// Thanks to Gene Nolan and Leon Palmer for their support.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// Protocol (19200 8N1, no terminator on replies):
//   0x06            -> 'S'
//   S               -> '1' while moving, '0' otherwise
//   :RT             -> temperature in 0.1 C, 4 chars ("+275"; "-888" without a probe)
//   :RP             -> position, sign and 6 digits ("+000050"), follows the motor during a move
//   :RO :RS         -> current and maximum step rate, 3 digits
//   :RA :RB :RE :RG :RW :RH :RC  -> compensation change ("+010"), steps, backlash, mode (0 off, 1 one shot,
//                      2 auto), phase wiring, averaging time, coil mode
//   :COnnn# :CSnnn# :CWn# :CCn  write the step rate, maximum step rate, phase wiring and coil mode
//   :TT+nnn# :TSnnn# :TBnnn# :TCnn# :TAn  write the compensation settings and mode
//   :FDMnnn#        -> relative move, D = 0 outward, 1 inward, M = stepping mode, nnn = 0..999 steps;
//                      :F10000# stops the motor

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

// steps per second, as in the C simulator
#define STEP_RATE 80

long position = 50;
long target = 50;
unsigned long last_step = 0;
char tt_value[5] = "+000";
char ts_value[4] = "000";
char ta_value = '0';
char tb_value[4] = "000";
char tc_value[3] = "30";
char cw_value = '0';
char cc_value = '1';
char cs_value[4] = "001";
char co_value[4] = "003";

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void step() {
  unsigned long now = millis();
  if (position == target) {
    last_step = now;
    return;
  }
  while (position != target && now - last_step >= 1000 / STEP_RATE) {
    position += position < target ? 1 : -1;
    last_step += 1000 / STEP_RATE;
  }
}

// Blocks until a byte arrives or the timeout passes, keeping the motor stepping; -1 on timeout.
int read_byte() {
  unsigned long start = millis();
  while (!Serial.available()) {
    if (millis() - start > 1000)
      return -1;
    step();
  }
  return Serial.read();
}

bool read_bytes(char *buffer, int count) {
  for (int i = 0; i < count; i++) {
    int c = read_byte();
    if (c < 0)
      return false;
    buffer[i] = c;
  }
  return true;
}

void loop() {
  step();
  if (!Serial.available())
    return;
  int command = Serial.read();
  if (command == 0x06) {
    Serial.write('S');
  } else if (command == 'S') {
    Serial.write(position != target ? '1' : '0');
  } else if (command == ':') {
    char name[2];
    if (!read_bytes(name, 2))
      return;
    if (name[0] == 'R') {
      switch (name[1]) {
        case 'T': Serial.write("+275"); break;
        case 'A': Serial.write(tt_value, 4); break;
        case 'B': Serial.write(ts_value, 3); break;
        case 'G': Serial.write(ta_value); break;
        case 'H': Serial.write(tc_value, 2); break;
        case 'E': Serial.write(tb_value, 3); break;
        case 'W': Serial.write(cw_value); break;
        case 'C': Serial.write(cc_value); break;
        case 'S': Serial.write(cs_value, 3); break;
        case 'O': Serial.write(co_value, 3); break;
        case 'P': {
          char reply[8];
          snprintf(reply, sizeof(reply), "%c%06ld", position < 0 ? '-' : '+', position < 0 ? -position : position);
          Serial.write(reply, 7);
          break;
        }
      }
    } else if (name[0] == 'F') {
      // direction in name[1], then the stepping mode, 3 digits and '#'
      char value[5];
      if (!read_bytes(value, 5))
        return;
      int steps = (value[1] - '0') * 100 + (value[2] - '0') * 10 + (value[3] - '0');
      if (name[1] == '1' && value[0] == '0' && steps == 0) {
        target = position;
      } else {
        last_step = millis();
        target = position + (name[1] == '1' ? -steps : steps);
      }
    } else if (name[0] == 'C') {
      if (name[1] == 'C') {
        char value;
        if (read_bytes(&value, 1))
          cc_value = value;
      } else if (name[1] == 'W') {
        char value[2];
        if (read_bytes(value, 2))
          cw_value = value[0];
      } else if (name[1] == 'S' || name[1] == 'O') {
        char value[4];
        if (read_bytes(value, 4))
          memcpy(name[1] == 'S' ? cs_value : co_value, value, 3);
      }
    } else if (name[0] == 'T') {
      if (name[1] == 'A') {
        char value;
        if (read_bytes(&value, 1))
          ta_value = value;
      } else if (name[1] == 'T') {
        char value[5];
        if (read_bytes(value, 5))
          memcpy(tt_value, value, 4);
      } else if (name[1] == 'C') {
        char value[3];
        if (read_bytes(value, 3))
          memcpy(tc_value, value, 2);
      } else if (name[1] == 'S' || name[1] == 'B') {
        char value[4];
        if (read_bytes(value, 4))
          memcpy(name[1] == 'S' ? ts_value : tb_value, value, 3);
      }
    }
  }
}
