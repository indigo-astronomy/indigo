// Rigel Systems nSTEP focuser simulator for Arduino
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

int position = 50;
char tt_value[4] = { '+', '0', '0', '0' };
char ts_value[3] = { '0', '0', '0' };
char ta_value[1] = { '0' };
char tb_value[3] = { '0', '0', '0' };
char tc_value[2] = { '3', '0' };
char cw_value[1] = { '0' };
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
    Serial.write('S');
  } else if (command == 'S') {
    Serial.write('0');
  } else if (command == ':') {
    command = Serial.read();
    if (command == 'R') {
      command = Serial.read();
      if (command == 'T') {
        Serial.write("+275");
      } else if (command == 'A') {
        Serial.write(tt_value, sizeof(tt_value));
      } else if (command == 'B') {
        Serial.write(ts_value, sizeof(ts_value));
      } else if (command == 'G') {
        Serial.write(ts_value, sizeof(ta_value));
      } else if (command == 'H') {
        Serial.write(ts_value, sizeof(tc_value));
      } else if (command == 'E') {
        Serial.write(tb_value, sizeof(tb_value));
      } else if (command == 'W') {
        Serial.write(cw_value, sizeof(cw_value));
      } else if (command == 'S') {
        Serial.write(cs_value, sizeof(cs_value));
      } else if (command == 'O') {
        Serial.write(co_value, sizeof(co_value));
      } else if (command == 'P') {
        if (position < 0) {
          Serial.write('-');
        } else {
          Serial.write('+');
        }
        int p = abs(position);
        Serial.write('0' + p / 100000);
        Serial.write('0' + (p / 10000) % 10);
        Serial.write('0' + (p / 1000) % 10);
        Serial.write('0' + (p / 100) % 10);
        Serial.write('0' + (p / 10) % 10);
        Serial.write('0' + p % 10);
      }
    } else if (command == 'F') {
      char dir = Serial.read();
      char mode = Serial.read();
      char steps = Serial.read() - '0';
      steps = steps * 10 + Serial.read() - '0';
      steps = steps * 10 + Serial.read() - '0';
      Serial.read();
      if (dir == '0') {
        position += steps;
      } else {
        position -= steps;      
      }
    } else if (command == 'C') {
      command = Serial.read();
      if (command == 'W') {
        Serial.readBytes(cw_value, sizeof(cw_value));
        Serial.read();
      } else if (command == 'S') {
        Serial.readBytes(cs_value, sizeof(cs_value));
        Serial.read();
      } else if (command == 'O') {
        Serial.readBytes(co_value, sizeof(co_value));
        Serial.read();
      } else if (command == 'C') {
        Serial.read();
      }
    } else if (command == 'T') {
      command = Serial.read();
      if (command == 'S') {
        Serial.readBytes(ts_value, sizeof(ts_value));        
        Serial.read();
      } else if (command == 'T') {
        Serial.readBytes(tt_value, sizeof(tt_value));
        Serial.read();
      } else if (command == 'A') {
        Serial.readBytes(ta_value, sizeof(ta_value));
      } else if (command == 'C') {
        Serial.readBytes(tc_value, sizeof(tc_value));
        Serial.read();
      } else if (command == 'B') {
        Serial.readBytes(tb_value, sizeof(tb_value));
        Serial.read();
      }
    }
  }
}
