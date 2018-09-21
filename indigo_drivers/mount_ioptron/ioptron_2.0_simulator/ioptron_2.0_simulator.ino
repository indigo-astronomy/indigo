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
int rate[] = { 1, 2, 8, 16, 64, 128, 256, 512, 1024 };

// +OOODYYMMDDHHMMSS
char timestamp[] = "+0000000101000000#";
unsigned long setup_time;

char lat[] = "+173335#";
char lon[] = "+061588#";

char dec[] = "+32400000";
char ra[] = "00000000";
char current[] = "+3240000000000000#";
char zero[] = "+3240000000000000#";

long move_dec = 0;
long move_ra = 0;

char guiding_rate[] = "050#";

void format(unsigned long value, char *buffer, int length) {
  while (length > 0) {
    buffer[--length] = value % 10 + '0';
    value /= 10;
  }
}

unsigned long parse(const char *buffer, int length) {
  unsigned long value = 0;
  while (length > 0) {
    value = value * 10 + *buffer++ - '0';
    length--;
  }
  return value;
}

void slew_to(const char *ra, const char *dec) {
  static long target_ra = 0;
  static long target_dec = 0;
  static char target_status = 0;
  static bool do_slew = false;
  static long unsigned last_sec = 0;
  if (ra && dec) {
    target_ra = parse(ra, 8);
    target_dec = parse(dec + 1, 8);
    if (*dec == '-')
      target_dec = -target_dec;
    target_status = status[1];
    if (target_status == '7')
      target_status = '0';
    do_slew = true;
  }
  long unsigned current_sec = millis() / 1000;
  if (do_slew && last_sec < current_sec) {
    do_slew = false;
    last_sec = current_sec;
    long current_ra = parse(current + 9, 8);
    long current_dec = parse(current + 1, 8);
    if (*current == '-')
      current_dec = -current_dec;

    long diff = target_dec - current_dec;    
    if (abs(diff) < 1500000L) {
      current_dec = target_dec;
    } else {
      current_dec = current_dec + (diff > 0 ? 1500000L : -1500000L);
      do_slew = true;
    }
    if (current_dec < 0)
      current[0] = '-';
    else
      current[1] = '+';
    format(abs(current_dec), current + 1, 8);

    diff = target_ra - current_ra;
    if (abs(diff) < 1000000L) {
      current_ra = target_ra;
    } else {
      current_ra = current_ra + (diff > 0 ? 1000000L : -1000000L);
      do_slew = true;
    }
    format(current_ra, current + 9, 8);
    
    if (do_slew)
      status[1] = '2';
    else
      status[1] = target_status;
  }
}

void tracking() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  long unsigned lapse = current_millis - last_millis;
  if (status[1] == '0') {
    long current_ra = parse(current + 9, 8);
    current_ra = (current_ra + lapse) % (24 * 36001000L);
    format(current_ra, current + 9, 8);
  }
  if (move_dec) {
    long current_dec = parse(current + 1, 8);
    if (*current == '-')
      current_dec = -current_dec;
    current_dec += move_dec * lapse;
    if (current_dec < -90 * 360000)
      current_dec = -90 * 360000;
    else if (current_dec > 90 * 360000)
      current_dec = 90 * 360000;
    if (current_dec < 0)
      current[0] = '-';
    else
      current[1] = '+';
    format(abs(current_dec), current + 1, 8);
  }
  if (move_ra) {
    long current_ra = parse(current + 9, 8);
    current_ra = (current_ra + move_ra * lapse) % (24 * 36001000L);
    format(current_ra, current + 9, 8);
  }
  last_millis = current_millis;
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
  slew_to(NULL, NULL);
  tracking();
  if (Serial.available()) {
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
        Serial.write("1");
        slew_to(zero + 9, zero);
        status[1] = '7';
      } else if (strncmp(command, "MSH", 3) == 0) {
        Serial.write("1");
        status[1] = '7';
        slew_to("00000000", "+32400000");
      } else if (strncmp(command, "MP", 2) == 0) {
        Serial.write("1");
        if (command[2] == '0') {
          if (status[1] == '6')
            status[1] = '7';
        } else {
          status[1] = '6';
          slew_to("00000000", "+32400000");
        }
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
        format(now % 60, timestamp + 15, 2);
        now /= 60;
        format(now % 60, timestamp + 13, 2);
        now /= 60;
        format(now % 24, timestamp + 11, 2);
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
        Serial.write("1");
        slew_to(ra, dec);
      } else if (strcmp(command, "Q") == 0) {
        Serial.write("1");
        status[1] = '0';
        slew_to(current + 9, current);
      } else if (strcmp(command, "SZP") == 0) {
        strncpy(zero, current, 18);
        Serial.write("1");
      } else if (strcmp(command, "GEC") == 0) {
        Serial.write(current, 18);
      } else if (strcmp(command, "mn") == 0) {
        move_dec = rate[status[3] - '1'];
      } else if (strcmp(command, "ms") == 0) {
        move_dec = -rate[status[3] - '1'];
      } else if (strcmp(command, "qD") == 0) {
        move_dec = 0;
        Serial.write("1");
      } else if (strcmp(command, "me") == 0) {
        move_ra = rate[status[3] - '1'];
      } else if (strcmp(command, "mw") == 0) {
        move_ra = -rate[status[3] - '1'];
      } else if (strcmp(command, "qR") == 0) {
        move_ra = 0;
        Serial.write("1");
      } else if (strcmp(command, "q") == 0) {
        move_dec = 0;
        move_ra = 0;
        Serial.write("1");
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
}
