// iOptron simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.

// 0: 0 = GPS off, 1 = GPS on, 2 = GPS data extracted correctly
// 1: 0 = stopped, 1 = tracking with PEC disabled, 2 = slewing, 3 = guiding, 4 = meridian flipping, 5 = tracking with PEC enabled, 6 = parked, 7 = stopped at zero position
// 2: 0 = sidereal rate, 1 = lunar rate, 2 = solar rate, 3 = King rate, 4 = custom rate
// 3: 1 = 1x sidereal tracking rate, 2 = 2x, 3 = 8x, 4 = 16x, 5 = 64x, 6 = 128x, 7 = 256x, 8 = 512x, 9 = maximum speed
// 4: 1 = RS-232 port, 2 = hand controller, 3 = GPS
// 5: 0 = Southern Hemisphere, 1 = Northern Hemisphere.
char status[] = "060521#";

// +OOODYYMMDDHHMMSS
char timestamp[] = "+0000000101000000#";
unsigned long setup_time;

char lat[] = "+173335#";
char lon[] = "+061588#";

char dec[] = "+32400000";
char ra[] = "00000000";
char current[] = "+3240000000000000#";
char zero[] = "+3240000000000000#";

char guiding_rate[] = "050#";

void format(unsigned long value, int length, char *buffer) {
  while (length > 0) {    
    buffer[--length] = value % 10 + '0';
    value /= 10;
  }
}

unsigned long parse(int length, char *buffer) {
  unsigned long value = 0;
  while (length > 0) {    
    value = value * 10 + *buffer++ - '0';
    length--;
  }
}

void slew_to(char *ra, char *dec) {
  // TBD
  strncpy(current, dec, 9);
  strncpy(current + 9, ra, 8);
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  while (!Serial)
    ;
}

void loop() {
  char command[64];
  if (Serial.read() == ':') {
    int length = Serial.readBytesUntil('#', command, sizeof(command));
    command[length] = 0;
    if (strcmp(command, "V") == 0) {
      Serial.write("V1.00#");
    } else if (strcmp(command, "MountInfo") == 0) {
      Serial.write("0045");
    } else if (strcmp(command, "FW1") == 0) {
      Serial.write("161101170322#");
    } else if (strcmp(command, "FW2") == 0) {
      Serial.write("161101161101#");
    } else if (strncmp(command, "ST", 2) == 0) {
      status[1] = command[2];
      Serial.write("1");
    } else if (strncmp(command, "MH", 2) == 0) {
      slew_to(zero + 9, zero);
      status[1] = '7';
      Serial.write("1");
    } else if (strncmp(command, "MSH", 3) == 0) {
      slew_to("00000000", "+32400000");
      status[1] = '7';
      Serial.write("1");
    } else if (strncmp(command, "MP", 2) == 0) {
      if (command[2] == '0') {
        if (status[1] == '6')
          status[1] == '7';
      } else {
        slew_to("00000000", "+32400000");
        status[1] = '6';
      }
      Serial.write("1");
    } else if (strncmp(command, "RT", 2) == 0) {
      status[2] = command[2];
      Serial.write("1");
    } else if (strncmp(command, "SR", 2) == 0) {
      status[3] = command[2];
      Serial.write("1");
    } else if (strncmp(command, "SHE", 3) == 0) {
      status[5] = command[3];
      Serial.write("1");
    } else if (strcmp(command, "GAS") == 0) {
      Serial.write(status, 7);
    } else if (strncmp(command, "St", 2) == 0) {
      strncpy(lat, command + 2, 7);
      Serial.write("1");
    } else if (strcmp(command, "Gt") == 0) {
      Serial.write(lat, 8);
    } else if (strncmp(command, "Sg", 2) == 0) {
      strncpy(lon, command + 2, 7);
      Serial.write("1");
    } else if (strcmp(command, "Gg") == 0) {
      Serial.write(lon, 8);
    } else if (strncmp(command, "SG", 2) == 0) {
      strncpy(timestamp, command + 2, 4);
      Serial.write("1");
    } else if (strncmp(command, "SDS", 3) == 0) {
      timestamp[4] = command[3];
      Serial.write("1");
    } else if (strncmp(command, "SC", 2) == 0) {
      strncpy(timestamp + 5, command + 2, 6);
      Serial.write("1");
    } else if (strncmp(command, "SL", 2) == 0) {
      setup_time = ((command[2] - '0') * 10 + (command[3] - '0')) * 3600 + ((command[4] - '0') * 10 + (command[5] - '0')) * 60 + ((command[6] - '0') * 10 + (command[7] - '0')) - millis() / 1000;
      strncpy(timestamp + 11, command + 2, 6);
      Serial.write("1");
    } else if (strcmp(command, "GLT") == 0) {
      unsigned long now = setup_time + millis() / 1000;
      format(now % 60, 2, timestamp + 15);
      now /= 60;
      format(now % 60, 2, timestamp + 13);
      now /= 60;
      format(now % 24, 2, timestamp + 11);
      Serial.write(timestamp, 18);
    } else if (strncmp(command, "RG", 2) == 0) {
      strncpy(guiding_rate, command + 2, 3);
      Serial.write("1");
    } else if (strcmp(command, "AG") == 0) {
      Serial.write(guiding_rate, 4);
    } else if (strncmp(command, "Sr", 2) == 0) {
      strncpy(ra, command + 2, 8);
      Serial.write("1");
    } else if (strncmp(command, "Sd", 2) == 0) {
      strncpy(dec, command + 2, 9);
      Serial.write("1");
    } else if (strcmp(command, "CM") == 0) {
      strncpy(current, dec, 9);
      strncpy(current + 9, ra, 8);
      Serial.write("1");
    } else if (strcmp(command, "MS") == 0) {
      slew_to(ra, dec);
      Serial.write("1");
    } else if (strcmp(command, "SZP") == 0) {
      strncpy(zero, current, 18);
      Serial.write("1");
    } else if (strcmp(command, "GEC") == 0) {
      Serial.write(current, 18);
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
