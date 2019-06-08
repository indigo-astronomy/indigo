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


#include "synscan_protocol.h"

void setup() {

// EQ8
setup_version("020304");
setup_axes(435, 1, 1, 200, 64, 2);
setup_features(HAS_ENCODER | HAS_PPEC | HAS_HOME_INDEXER | HAS_POLAR_LED);
// EQ6
//setup_version("020300");
//setup_axes(180, 47, 12, 200, 64, 2);
// HEQ5"
//setup_version("020301");
//setup_axes(135, 47, 9, 200, 64, 2);
// NEQ5"
//setup_version("020302");
//setup_axes(144, 44, 9, 200, 32, 2);
// NEQ3"
//setup_version("020303");
//setup_axes(130, 55, 10, 200, 32, 2);
  
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  if (Serial.available()) {
    char buffer[16];
    memset(buffer, 0, sizeof(buffer));
    Serial.readBytesUntil('\r', buffer, sizeof(buffer));
    Serial.write(process_command(buffer));
  }
}
