// Rigel Systems nFOCUS focuser simulator for Arduino
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
//   0x06          -> 'n'
//   S             -> '1' while moving, '0' otherwise
//   :RT           -> temperature in 0.1 C, 4 chars ("+275"; "-888" without a probe)
//   :RO / :RF     -> on-time / off-time, 3 digits
//   :RS           -> step mode, 3 digits
//   :COnnn# :CFnnn# :CSnnn#  write on-time, off-time and step mode
//   :FDnnnn#      -> relative move, D = 0 outward, 1 inward, nnnn = 0..999 steps (first digit is a 1 in
//                    the driver's 4-digit field); :F11000# stops the motor

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

// steps per second, as in the C simulator
#define STEP_RATE 80

char cs_value[4] = "001";
char co_value[4] = "005";
char cf_value[4] = "005";
long position = 0;
long target = 0;
unsigned long last_step = 0;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

// Blocks until a byte arrives or the timeout passes; -1 on timeout.
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

void loop() {
  step();
  if (!Serial.available())
    return;
  int command = Serial.read();
  if (command == 0x06) {
    Serial.write('n');
  } else if (command == 'S') {
    Serial.write(position != target ? '1' : '0');
  } else if (command == ':') {
    char name[2];
    if (!read_bytes(name, 2))
      return;
    if (name[0] == 'R') {
      if (name[1] == 'T')
        Serial.write("+275");
      else if (name[1] == 'O')
        Serial.write(co_value, 3);
      else if (name[1] == 'F')
        Serial.write(cf_value, 3);
      else if (name[1] == 'S')
        Serial.write(cs_value, 3);
    } else if (name[0] == 'C') {
      char value[4];
      if (!read_bytes(value, 4))
        return;
      if (name[1] == 'S')
        memcpy(cs_value, value, 3);
      else if (name[1] == 'O')
        memcpy(co_value, value, 3);
      else if (name[1] == 'F')
        memcpy(cf_value, value, 3);
    } else if (name[0] == 'F') {
      // name[1] is the direction, then 4 digits and '#'
      char value[5];
      if (!read_bytes(value, 5))
        return;
      if (name[1] == '1' && !strncmp(value, "1000", 4)) {
        target = position;
      } else {
        int steps = (value[1] - '0') * 100 + (value[2] - '0') * 10 + (value[3] - '0');
        last_step = millis();
        target = position + (name[1] == '1' ? -steps : steps);
      }
    }
  }
}
