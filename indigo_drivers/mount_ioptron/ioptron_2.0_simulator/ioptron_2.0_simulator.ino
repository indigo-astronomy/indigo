// iOptron simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
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
#define LCD
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif


// 0: 0 = GPS off, 1 = GPS on, 2 = GPS data extracted correctly
// 1: 0 = stopped, 1 = tracking with PEC disabled, 2 = slewing, 3 = guiding, 4 = meridian flipping, 5 = tracking with PEC enabled, 6 = parked, 7 = stopped at zero position
// 2: 0 = sidereal rate, 1 = lunar rate, 2 = solar rate, 3 = King rate, 4 = custom rate
// 3: 1 = 1x sidereal tracking rate, 2 = 2x, 3 = 8x, 4 = 16x, 5 = 64x, 6 = 128x, 7 = 256x, 8 = 512x, 9 = maximum speed
// 4: 1 = RS-232 port, 2 = hand controller, 3 = GPS
// 5: 0 = Southern Hemisphere, 1 = Northern Hemisphere.
char status[] = "260521#";
int rate[] = { 1, 2, 8, 16, 64, 128, 256, 512, 1024 };

// +OOODYYMMDDHHMMSS
char timestamp[] = "+0000000101000000#";
long setup_time;

char lat[] = "+173335#";
char lon[] = "+061588#";

char dec[] = "+32400000";
char ra[] = "00000000";
char current[] = "+3240000000000000#";
char zero[] = "+3240000000000000#";

long dec_rate = 0;
long ra_rate = 0;

char guiding_rate[] = "050#";

void format(long value, bool sign, char *buffer, int length) {
  if (sign) {
    if (value > 0) {
      *buffer++ = '+';
    } else {
      *buffer++ = '-';
      value = -value;
    }
    length--;
  }
  while (length > 0) {
    buffer[--length] = value % 10 + '0';
    value /= 10;
  }
}

long parse(const char *buffer, int length) {
  long value = 0;
  int sign = 1;
  if (*buffer == '+') {
    buffer++;
    length--;
  } else if (*buffer == '-') {
    buffer++;
    length--;
    sign = -1;
  }
  while (length > 0) {
    value = value * 10 + *buffer++ - '0';
    length--;
  }
  return value * sign;
}

void slew_to(const char *dec, const char *ra) {
  static long target_ra = 0;
  static long target_dec = 0;
  static char target_status = 0;
  static bool do_slew = false;
  static long unsigned last_millis = 0;
  if (ra && dec) {
    target_ra = parse(ra, 8);
    target_dec = parse(dec, 9);
    target_status = status[1];
    if (target_status == '7')
      target_status = '0';
    do_slew = true;
    last_millis = millis();
  }
  long unsigned current_millis = millis();
  if (do_slew) {
    long lapse = current_millis - last_millis;
    do_slew = false;
    last_millis = current_millis;
    long current_dec = parse(current, 9);
    long current_ra = parse(current + 9, 8);

    long diff = target_dec - current_dec;    
    if (abs(diff) < 1500L * lapse) {
      current_dec = target_dec;
    } else {
      current_dec = current_dec + (diff > 0 ? 1500L : -1500L) * lapse;
      do_slew = true;
    }
    format(current_dec, true, current, 9);

    diff = target_ra - current_ra;
    if (diff > 12)
      diff = diff - 24;
    else if (diff < -12)
      diff = diff + 24;
    if (abs(diff) < 1000L * lapse) {
      current_ra = target_ra;
    } else {
      current_ra = (current_ra + (diff > 0 ? 1000L : -1000L) * lapse) % (24 * 36001000L);
      do_slew = true;
    }
    format(current_ra, false, current + 9, 8);
    
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
    format(current_ra, false, current + 9, 8);
  }
  if (dec_rate) {
    long current_dec = parse(current, 9);
    current_dec += dec_rate * lapse;
    if (current_dec < -90 * 360000)
      current_dec = -90 * 360000;
    else if (current_dec > 90 * 360000)
      current_dec = 90 * 360000;
    format(current_dec, true, current, 9);
  }
  if (ra_rate) {
    long current_ra = parse(current + 9, 8);
    current_ra = (current_ra + ra_rate * lapse) % (24 * 36001000L);
    format(current_ra, false, current + 9, 8);
  }
  last_millis = current_millis;
}

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("iOptron v2.0");
  lcd.setCursor(0, 1);
  lcd.print("Not connected");
#endif
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
#ifdef LCD
  lcd.setCursor(0, 0);
  lcd.print(current);
  lcd.setCursor(0, 1);
  lcd.print(timestamp + 4);
#endif
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
        slew_to(zero, zero + 9);
        status[1] = '7';
      } else if (strncmp(command, "MSH", 3) == 0) {
        Serial.write("1");
        status[1] = '7';
        slew_to("+32400000", "00000000");
      } else if (strncmp(command, "MP", 2) == 0) {
        Serial.write("1");
        if (command[2] == '0') {
          if (status[1] == '6')
            status[1] = '7';
        } else {
          status[1] = '6';
          slew_to("+32400000", "00000000");
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
        setup_time = ((command[2] - '0') * 10 + (command[3] - '0')) * 3600L + ((command[4] - '0') * 10 + (command[5] - '0')) * 60L + ((command[6] - '0') * 10 + (command[7] - '0')) - millis() / 1000;
        strncpy(timestamp + 11, command + 2, 6);
        Serial.write("1");
      } else if (strcmp(command, "GLT") == 0) {
        long now = setup_time + millis() / 1000;
        format(now % 60, false, timestamp + 15, 2);
        now /= 60;
        format(now % 60, false, timestamp + 13, 2);
        now /= 60;
        format(now % 24, false, timestamp + 11, 2);
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
        slew_to(dec, ra);
      } else if (strcmp(command, "Q") == 0) {
        Serial.write("1");
        status[1] = '0';
        slew_to(current, current + 9);
      } else if (strcmp(command, "SZP") == 0) {
        strncpy(zero, current, 18);
        Serial.write("1");
      } else if (strcmp(command, "GEC") == 0) {
        Serial.write(current, 18);
      } else if (strcmp(command, "mn") == 0) {
        dec_rate = rate[status[3] - '1'];
      } else if (strcmp(command, "ms") == 0) {
        dec_rate = -rate[status[3] - '1'];
      } else if (strcmp(command, "qD") == 0) {
        dec_rate = 0;
        Serial.write("1");
      } else if (strcmp(command, "me") == 0) {
        ra_rate = rate[status[3] - '1'];
      } else if (strcmp(command, "mw") == 0) {
        ra_rate = -rate[status[3] - '1'];
      } else if (strcmp(command, "qR") == 0) {
        ra_rate = 0;
        Serial.write("1");
      } else if (strcmp(command, "q") == 0) {
        dec_rate = 0;
        ra_rate = 0;
        Serial.write("1");
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
}
