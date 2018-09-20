// Rigel Systems nFOCUS focuser simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
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

char cs_value[3] = { '0', '0', '3' };
char co_value[3] = { '0', '0', '3' };

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command = Serial.read();
  if (command == '#') {
  } else if (command == 0x6) {
    Serial.write('n');
  } else if (command == 'S') {
    Serial.write('0');
  } else if (command == ':') {
    command = Serial.read();
    if (command == 'R') {
      command = Serial.read();
      if (command == 'T') {
        Serial.write("+275");
      } else if (command == 'O') {
        Serial.write(co_value, sizeof(co_value));
      } else if (command == 'S') {
        Serial.write(co_value, sizeof(co_value));
      }
    } else if (command == 'F') {
      Serial.read();
      Serial.read();
      Serial.read();
      Serial.read();
      Serial.read();
      Serial.read();
    } else if (command == 'C') {
      command = Serial.read();
      if (command == 'S') {
        Serial.readBytes(cs_value, sizeof(cs_value));
        Serial.read();
      } else if (command == 'O') {
        Serial.readBytes(co_value, sizeof(co_value));
        Serial.read();
      }
    }
  }
}
